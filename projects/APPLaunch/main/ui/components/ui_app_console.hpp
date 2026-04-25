#pragma once
#include "ui_app_page.hpp"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <unordered_map>
#include <list>
#include <memory>
#include <string>
#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
#include <pty.h>
#include <termios.h>
#include <errno.h>
#include <vector>
#include <sstream>
#include <keyboard_input.h>

// ============================================================
//  终端控制台  UIConsolePage
//  屏幕分辨率: 320 x 170  (顶栏20px, ui_APP_Container 320x150)
//
//  功能:
//    - 完整 VT100/ANSI 终端仿真（移植自 src/vt100.c 状态机）
//      支持: 光标移动, 擦除, SGR属性, 插入/删除行/字符,
//            DEC 私有模式 (DECTCEM, DECAWM, DECSCNM等),
//            SCP/RCP 光标保存恢复, 滚动区域, 设备属性等
//    - PTY 子进程管理（fork + openpty）
//    - 行级脏标记渲染，减少 LVGL 刷新开销
//    - 光标闪烁（500ms 定时器）
//    - 键盘输入转发至 PTY（evdev keycode + utf8 / LV_KEY_*）
//
//  对外接口:
//    exec(std::string cmd)  — 启动一条命令（支持带参数的命令字符串）
// ============================================================
class UIConsolePage : public app_base
{
    /* ------------------------------------------------------------------ */
    /*  终端规格                                                            */
    /* ------------------------------------------------------------------ */
    static constexpr int TERM_W = 320;
    static constexpr int TERM_H = 150;
    static constexpr int CHAR_W = 7;
    static constexpr int CHAR_H = 12;
    static constexpr int COLS = TERM_W / CHAR_W; /* 45 */
    static constexpr int ROWS = TERM_H / CHAR_H; /* 12 */

    /* 绿字黑底 */
    static constexpr uint32_t FIXED_FG = 0x00FF00u;
    static constexpr uint32_t FIXED_BG = 0x000000u;

    /* ------------------------------------------------------------------ */
    /*  UI 对象                                                             */
    /* ------------------------------------------------------------------ */
    lv_obj_t *terminal_container = nullptr;
    lv_obj_t *term_canvas = nullptr;

    /* 整行渲染：ROWS 个 label 替代大量 cell label */
    lv_obj_t *row_labels[ROWS] = {};
    /* 光标：独立一个反色 label，仅创建时设置一次样式 */
    lv_obj_t *cursor_label = nullptr;

    /* 行级 dirty 比对缓存（含末尾 '\0'） */
    char row_rendered[ROWS][COLS + 1] = {};

public:
    bool terminal_sysplause = true;

public:
    UIConsolePage() : app_base()
    {
        console_data_init();
        creat_console_UI();
        event_handler_init();
    }

    ~UIConsolePage()
    {
        terminal_active = false;
        if (poll_timer)
        {
            lv_timer_delete(poll_timer);
            poll_timer = nullptr;
        }
        if (cursor_timer)
        {
            lv_timer_delete(cursor_timer);
            cursor_timer = nullptr;
        }
        stop_pty();
    }

    // ==================== 对外接口 ====================

    /**
     * 启动一条命令。
     * 命令字符串按空格拆分，首 token 为可执行文件路径，其余为参数。
     * 若已有子进程运行，先终止再重新启动。
     */
    void exec(std::string cmd)
    {
        if (child_pid > 0)
            stop_pty();

        terminal_active = true;
        vt100_cur_row = 0;
        vt100_cur_col = 0;
        vt100_cur_attr = 0;
        vt100_cur_fg = 7;
        vt100_cur_bg = 0;
        vt100_saved_row = 0; vt100_saved_col = 0;
        vt100_saved_attr = 0; vt100_saved_fg = 7; vt100_saved_bg = 0;
        vt100_auto_wrap = true;
        vt100_cursor_visible_flag = true;
        vt100_decckm = false;
        vt100_esc_state = VT100_ST_NORMAL;
        vt100_esc_len = 0;
        vt100_param_count = 0;
        vt100_param_val = 0;
        vt100_priv_mode = false;
        vt100_sec_mode = false;
        vt100_skip_until_st = false;
        memset(vt100_params, 0, sizeof(vt100_params));
        waiting_key_to_exit = false;

        vt100_screen_clear_all();
        /* 强制首次全量渲染 */
        memset(row_rendered, 0, sizeof(row_rendered));
        vt100_render_all();

        /* 按空格拆分命令字符串 */
        std::vector<std::string> tokens;
        std::istringstream iss(cmd);
        std::string token;
        while (iss >> token)
            tokens.push_back(token);

        if (tokens.empty())
        {
            const char *err = "Error: empty command\r\n";
            vt100_process_bytes(err, (int)strlen(err));
            vt100_render_all();
            terminal_active = false;
            return;
        }

        std::string executable = tokens[0];
        std::vector<std::string> args(tokens.begin() + 1, tokens.end());

        if (!start_pty(executable, args))
        {
            const char *err = "Error: openpty/fork failed\r\n";
            vt100_process_bytes(err, (int)strlen(err));
            vt100_render_all();
            terminal_active = false;
            return;
        }

        if (!poll_timer)
            poll_timer = lv_timer_create(UIConsolePage::s_poll_cb, 30, this);
        if (!cursor_timer)
            cursor_timer = lv_timer_create(UIConsolePage::s_cursor_blink_cb, 500, this);
    }

private:
    /* ================================================================== */
    /*  VT100 字符网格状态                                                  */
    /* ================================================================== */
    char vt100_screen[ROWS][COLS] = {};

    /* 光标 */
    int vt100_cur_row = 0;
    int vt100_cur_col = 0;

    /* SGR 当前属性 (解析追踪，渲染仍为单色) */
    int vt100_cur_attr = 0;   /* ATTR_BOLD=1, ATTR_UNDERLINE=2, ATTR_BLINK=4, ATTR_REVERSE=8 */
    int vt100_cur_fg = 7;     /* ANSI 8色: 0-7, COLOR_DEFAULT=9 */
    int vt100_cur_bg = 0;

    /* 光标保存/恢复 (SCP/RCP, DECSC/DECRC) */
    int vt100_saved_row = 0, vt100_saved_col = 0;
    int vt100_saved_attr = 0, vt100_saved_fg = 7, vt100_saved_bg = 0;

    /* DECAWM — 自动换行 */
    bool vt100_auto_wrap = true;

    /* DECTCEM — 光标显隐 */
    bool vt100_cursor_visible_flag = true;

    /* DECCKM — 应用光标键模式 (affects what sequence keyboard sends) */
    bool vt100_decckm = false;

    /* ── 完整 VT100 状态机 ─────────────────────────────────── */
    enum vt_state {
        VT100_ST_NORMAL,    /* 打印字符 */
        VT100_ST_ESC,       /* 收到 ESC */
        VT100_ST_CSI,       /* ESC [ — 收集参数 */
        VT100_ST_CSI_QM,    /* ESC [? — DEC 私有模式 */
        VT100_ST_CSI_GT,    /* ESC [> — Secondary DA / xterm 扩展 */
        VT100_ST_OSC,       /* ESC ] — 操作系统命令 */
        VT100_ST_DCS,       /* ESC P — 设备控制字符串 */
    };
    vt_state vt100_esc_state = VT100_ST_NORMAL;
    char vt100_esc_buf[64] = {};
    int vt100_esc_len = 0;

    /* CSI 参数 */
    static constexpr int VT100_MAX_PARAMS = 16;
    int vt100_params[VT100_MAX_PARAMS] = {};
    int vt100_param_count = 0;
    int vt100_param_val = 0;
    bool vt100_priv_mode = false;
    bool vt100_sec_mode = false;
    bool vt100_skip_until_st = false;

    /* ── PTY ──────────────────────────────────────────────── */
    int pty_master = -1;
    pid_t child_pid = -1;

    lv_timer_t *poll_timer = nullptr;
    lv_timer_t *cursor_timer = nullptr;
    bool vt100_cursor_vis = false;
    bool terminal_active = false;
    bool waiting_key_to_exit = false;

    /* ================================================================== */
    /*  初始化                                                              */
    /* ================================================================== */
    void console_data_init()
    {
        memset(vt100_screen, ' ', sizeof(vt100_screen));
        memset(row_rendered, 0, sizeof(row_rendered));
        memset(row_labels, 0, sizeof(row_labels));
        memset(vt100_esc_buf, 0, sizeof(vt100_esc_buf));
        memset(vt100_params, 0, sizeof(vt100_params));
    }

    /* ================================================================== */
    /*  UI 构建（终端区域，挂载到 ui_APP_Container）                        */
    /* ================================================================== */
    void creat_console_UI()
    {
        terminal_container = lv_obj_create(ui_APP_Container);
        lv_obj_remove_style_all(terminal_container);
        lv_obj_set_size(terminal_container, TERM_W, TERM_H);
        lv_obj_set_pos(terminal_container, 0, 0);
        lv_obj_set_style_bg_color(terminal_container, lv_color_hex(FIXED_BG), 0);
        lv_obj_set_style_bg_opa(terminal_container, LV_OPA_COVER, 0);
        lv_obj_clear_flag(terminal_container,
                          (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));

        term_canvas = lv_obj_create(terminal_container);
        lv_obj_set_size(term_canvas, TERM_W, TERM_H);
        lv_obj_set_pos(term_canvas, 0, 0);
        lv_obj_set_style_bg_color(term_canvas, lv_color_hex(FIXED_BG), 0);
        lv_obj_set_style_bg_opa(term_canvas, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(term_canvas, 0, 0);
        lv_obj_set_style_pad_all(term_canvas, 0, 0);
        lv_obj_set_style_radius(term_canvas, 0, 0);
        lv_obj_remove_flag(term_canvas,
                           (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));

        /* --------------------- 行级 label ------------------------ */
        const lv_font_t *mono_font = g_font_mono_12 ? g_font_mono_12 : g_font_cn_12;
        for (int r = 0; r < ROWS; r++)
        {
            lv_obj_t *lbl = lv_label_create(term_canvas);
            lv_obj_set_style_text_font(lbl, mono_font, 0);
            lv_obj_set_style_text_color(lbl, lv_color_hex(FIXED_FG), 0);
            lv_obj_set_style_bg_opa(lbl, LV_OPA_TRANSP, 0);
            lv_obj_set_style_pad_all(lbl, 0, 0);
            lv_obj_set_style_text_letter_space(lbl, 0, 0);
            lv_obj_set_style_text_line_space(lbl, 0, 0);
            lv_label_set_long_mode(lbl, LV_LABEL_LONG_CLIP);
            lv_obj_set_size(lbl, TERM_W, CHAR_H);
            lv_obj_set_pos(lbl, 0, r * CHAR_H);
            lv_label_set_text(lbl, "");
            row_labels[r] = lbl;
        }
        memset(row_rendered, 0, sizeof(row_rendered));

        /* --------------------- 光标 label（反色块）---------------- */
        cursor_label = lv_label_create(term_canvas);
        lv_obj_set_style_text_font(cursor_label, mono_font, 0);
        lv_obj_set_style_text_color(cursor_label, lv_color_hex(FIXED_BG), 0);
        lv_obj_set_style_bg_color(cursor_label, lv_color_hex(FIXED_FG), 0);
        lv_obj_set_style_bg_opa(cursor_label, LV_OPA_COVER, 0);
        lv_obj_set_style_pad_all(cursor_label, 0, 0);
        lv_obj_set_style_text_letter_space(cursor_label, 0, 0);
        lv_label_set_long_mode(cursor_label, LV_LABEL_LONG_CLIP);
        lv_obj_set_size(cursor_label, CHAR_W, CHAR_H);
        lv_label_set_text(cursor_label, " ");
        lv_obj_set_pos(cursor_label, 0, 0);
        lv_obj_add_flag(cursor_label, LV_OBJ_FLAG_HIDDEN);
    }

    /* ================================================================== */
    /*  事件绑定                                                            */
    /* ================================================================== */
    void event_handler_init()
    {
        lv_obj_add_event_cb(ui_root, UIConsolePage::static_lvgl_handler, LV_EVENT_ALL, this);
    }

    static void static_lvgl_handler(lv_event_t *e)
    {
        UIConsolePage *self = static_cast<UIConsolePage *>(lv_event_get_user_data(e));
        if (self)
            self->event_handler(e);
    }

    void event_handler(lv_event_t *e)
    {
        if (lv_event_get_code(e) == LV_EVENT_KEYBOARD)
        {
            struct key_item *elm = (struct key_item *)lv_event_get_param(e);
            if (waiting_key_to_exit && (elm->key_state == 0))
            {
                if (terminal_sysplause)
                {
                    terminal_sysplause = false;
                }
                else
                {
                    waiting_key_to_exit = false;
                    if (go_back_home)
                        go_back_home();
                }
            }
            else
            {
                if (pty_master >= 0 && terminal_active)
                {
                    if (elm->key_state)
                        write_key_to_pty(elm->key_code, elm->utf8);
                }
            }
            printf("Received LV_EVENT_KEYBOARD event: elm=%s\n", elm->sym_name);
        }
    }

    /* ================================================================== */
    /*  LVGL 定时器静态包装                                                 */
    /* ================================================================== */
    static void s_poll_cb(lv_timer_t *t)
    {
        auto self = (UIConsolePage *)lv_timer_get_user_data(t);
        if (self)
            self->vt100_poll_cb(t);
    }

    static void s_cursor_blink_cb(lv_timer_t *t)
    {
        auto self = (UIConsolePage *)lv_timer_get_user_data(t);
        if (self)
            self->vt100_cursor_blink_cb(t);

        static int end_status = 0;
        static std::chrono::time_point<std::chrono::steady_clock> start_time;
        static std::chrono::time_point<std::chrono::steady_clock> end_time;
        pid_t pid_ret;
        if (end_status == 0)
        {
            if (LVGL_HOME_KEY_FLAGE)
            {
                end_status = 1;
                start_time = std::chrono::steady_clock::now();
            }
        }
        if (end_status == 1)
        {
            if (LVGL_HOME_KEY_FLAGE)
            {
                end_time = std::chrono::steady_clock::now();
                if (std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count() >= 5)
                {
                    end_status = 0;
                    kill(self->child_pid, SIGKILL);
                    // self->stop_pty();
                    // self->terminal_active = false;
                }
            }
            else
            {
                end_status = 0;
            }
        }
    }

    /* ================================================================== */
    /*  VT100 辅助函数                                                      */
    /* ================================================================== */

    static int clamp(int v, int lo, int hi)
    {
        if (v < lo) return lo;
        if (v > hi) return hi;
        return v;
    }

    /** defparam: 缺省参数或显式0均返回默认值。
     *  idx >= param_count → 参数缺失 → dfl
     *  params[idx] == 0 → 显式0或缺失 → dfl
     */
    int defparam(int idx, int dfl)
    {
        return (idx >= vt100_param_count || vt100_params[idx] == 0) ? dfl : vt100_params[idx];
    }

    /* ================================================================== */
    /*  字符网格操作                                                        */
    /* ================================================================== */
    void vt100_screen_clear_all()
    {
        for (int r = 0; r < ROWS; r++)
            memset(vt100_screen[r], ' ', COLS);
    }

    void vt100_clear_row_from(int row, int from_col)
    {
        if (row < 0 || row >= ROWS) return;
        if (from_col < 0) from_col = 0;
        if (from_col >= COLS) return;
        memset(&vt100_screen[row][from_col], ' ', COLS - from_col);
    }

    void vt100_scroll_up(int top, int bot, int n)
    {
        if (top < 0) top = 0;
        if (bot >= ROWS) bot = ROWS - 1;
        if (n <= 0 || n > bot - top + 1) return;
        for (int r = top; r <= bot - n; r++)
            memcpy(vt100_screen[r], vt100_screen[r + n], COLS);
        for (int r = bot - n + 1; r <= bot; r++)
            memset(vt100_screen[r], ' ', COLS);
    }

    void vt100_scroll_down(int top, int bot, int n)
    {
        if (top < 0) top = 0;
        if (bot >= ROWS) bot = ROWS - 1;
        if (n <= 0 || n > bot - top + 1) return;
        for (int r = bot; r >= top + n; r--)
            memcpy(vt100_screen[r], vt100_screen[r - n], COLS);
        for (int r = top; r < top + n; r++)
            memset(vt100_screen[r], ' ', COLS);
    }

    void vt100_erase_display(int mode)
    {
        if (mode == 0) {
            vt100_clear_row_from(vt100_cur_row, vt100_cur_col);
            for (int r = vt100_cur_row + 1; r < ROWS; r++)
                vt100_clear_row_from(r, 0);
        } else if (mode == 1) {
            for (int r = 0; r < vt100_cur_row; r++)
                vt100_clear_row_from(r, 0);
            for (int c = 0; c <= vt100_cur_col && c < COLS; c++)
                vt100_screen[vt100_cur_row][c] = ' ';
        } else {
            vt100_screen_clear_all();
        }
    }

    void vt100_erase_line(int mode)
    {
        int start, end;
        if (mode == 0) {
            start = vt100_cur_col; end = COLS - 1;
        } else if (mode == 1) {
            start = 0; end = vt100_cur_col;
        } else {
            start = 0; end = COLS - 1;
        }
        for (int c = start; c <= end && c < COLS; c++)
            vt100_screen[vt100_cur_row][c] = ' ';
    }

    void vt100_insert_lines(int n)
    {
        if (n < 1) n = 1;
        vt100_scroll_down(vt100_cur_row, ROWS - 1, n);
    }

    void vt100_delete_lines(int n)
    {
        if (n < 1) n = 1;
        vt100_scroll_up(vt100_cur_row, ROWS - 1, n);
    }

    void vt100_insert_chars(int n)
    {
        if (n < 1) n = 1;
        for (int i = COLS - 1; i >= vt100_cur_col + n; i--)
            vt100_screen[vt100_cur_row][i] = vt100_screen[vt100_cur_row][i - n];
        for (int i = 0; i < n && vt100_cur_col + i < COLS; i++)
            vt100_screen[vt100_cur_row][vt100_cur_col + i] = ' ';
    }

    void vt100_delete_chars(int n)
    {
        if (n < 1) n = 1;
        for (int i = vt100_cur_col; i < COLS - n; i++)
            vt100_screen[vt100_cur_row][i] = vt100_screen[vt100_cur_row][i + n];
        for (int i = COLS - n; i < COLS; i++)
            vt100_screen[vt100_cur_row][i] = ' ';
    }

    /* ================================================================== */
    /*  字符输出 (含 DECAWM 自动换行)                                       */
    /* ================================================================== */
    void vt100_put_char(char ch)
    {
        if (ch == '\r')
        {
            vt100_cur_col = 0;
            return;
        }
        if (ch == '\n')
        {
            vt100_cur_col = 0;
            if (++vt100_cur_row >= ROWS)
            {
                vt100_scroll_up(0, ROWS - 1, 1);
                vt100_cur_row = ROWS - 1;
            }
            return;
        }
        if (ch == '\b')
        {
            if (vt100_cur_col > 0)
                vt100_cur_col--;
            return;
        }
        if (ch == '\t')
        {
            int next_tab = (vt100_cur_col / 8 + 1) * 8;
            if (next_tab >= COLS) next_tab = COLS - 1;
            while (vt100_cur_col < next_tab)
                vt100_put_char(' ');
            return;
        }
        /* C0 controls (0x00-0x1F) — 忽略 */
        if ((unsigned char)ch < 0x20)
            return;
        /* DEL (0x7F) */
        if ((unsigned char)ch == 0x7F)
            return;

        /* ── DECAWM 自动换行检查 ── */
        if (vt100_cur_col >= COLS)
        {
            if (vt100_auto_wrap)
            {
                vt100_cur_col = 0;
                if (++vt100_cur_row >= ROWS)
                {
                    vt100_scroll_up(0, ROWS - 1, 1);
                    vt100_cur_row = ROWS - 1;
                }
            }
            else
            {
                vt100_cur_col = COLS - 1;  /* 覆盖最后一列 */
            }
        }
        vt100_screen[vt100_cur_row][vt100_cur_col++] = ch;
    }

    /* ================================================================== */
    /*  ESC 序列处理 (非 CSI)                                               */
    /* ================================================================== */
    void vt100_handle_esc(char c)
    {
        switch (c) {
        case 'D':  /* IND — Index: 光标下移，必要时滚屏 */
            if (vt100_cur_row == ROWS - 1)
                vt100_scroll_up(0, ROWS - 1, 1);
            else
                vt100_cur_row++;
            break;
        case 'M':  /* RI — Reverse Index: 光标上移 */
            if (vt100_cur_row == 0)
                vt100_scroll_down(0, ROWS - 1, 1);
            else
                vt100_cur_row--;
            break;
        case 'E':  /* NEL — Next Line */
            vt100_cur_col = 0;
            if (vt100_cur_row == ROWS - 1)
                vt100_scroll_up(0, ROWS - 1, 1);
            else
                vt100_cur_row++;
            break;
        case 'H':  /* HTS — Horizontal Tab Set (acknowledge) */
            break;
        case '7':  /* DECSC — Save Cursor */
            vt100_saved_row  = vt100_cur_row;
            vt100_saved_col  = vt100_cur_col;
            vt100_saved_attr = vt100_cur_attr;
            vt100_saved_fg   = vt100_cur_fg;
            vt100_saved_bg   = vt100_cur_bg;
            break;
        case '8':  /* DECRC — Restore Cursor */
            vt100_cur_row  = clamp(vt100_saved_row, 0, ROWS - 1);
            vt100_cur_col  = clamp(vt100_saved_col, 0, COLS - 1);
            vt100_cur_attr = vt100_saved_attr;
            vt100_cur_fg   = vt100_saved_fg;
            vt100_cur_bg   = vt100_saved_bg;
            break;
        case 'c':  /* RIS — Reset to Initial State */
            vt100_screen_clear_all();
            vt100_cur_row = 0;
            vt100_cur_col = 0;
            vt100_cur_attr = 0;
            vt100_cur_fg = 7;
            vt100_cur_bg = 0;
            vt100_auto_wrap = true;
            vt100_cursor_visible_flag = true;
            vt100_decckm = false;
            break;
        case '=':  /* DECKPAM — Keypad Application Mode */
        case '>':  /* DECKPNM — Keypad Numeric Mode */
            break;
        case '(':  /* SCS — Designate G0 charset */
        case ')':  /* SCS — Designate G1 charset */
        case '*':  /* SCS — Designate G2 charset */
        case '+':  /* SCS — Designate G3 charset */
        case '#':  /* DEC line attributes */
            /* consume next byte; we ignore */
            vt100_esc_state = VT100_ST_NORMAL;
            break;
        default:
            break;
        }
    }

    /* ================================================================== */
    /*  CSI 序列分发 (完整 VT100 指令集)                                    */
    /* ================================================================== */
    void vt100_handle_csi(char final)
    {
        /* ── DEC 私有模式 (ESC [? ... h/l) ── */
        if (vt100_priv_mode) {
            switch (final) {
            case 'h': /* DECSET */
                if (vt100_params[0] == 1)  { vt100_decckm = true; fprintf(stderr, "[VT100-DBG] DECCKM ON (app cursor keys)\n"); }  /* DECCKM */
                if (vt100_params[0] == 5)  {}  /* DECSCNM — reverse video */
                if (vt100_params[0] == 6)  {}  /* DECOM — origin mode */
                if (vt100_params[0] == 7)  { vt100_auto_wrap = true; }  /* DECAWM */
                if (vt100_params[0] == 25) { vt100_cursor_visible_flag = true; }  /* DECTCEM */
                if (vt100_params[0] == 1049) {} /* alt screen */
                break;
            case 'l': /* DECRST */
                if (vt100_params[0] == 1)  { vt100_decckm = false; }
                if (vt100_params[0] == 5)  {}
                if (vt100_params[0] == 6)  {}
                if (vt100_params[0] == 7)  { vt100_auto_wrap = false; }  /* DECAWM off */
                if (vt100_params[0] == 25) { vt100_cursor_visible_flag = false; } /* DECTCEM hide */
                if (vt100_params[0] == 1049) {}
                break;
            default:
                break;
            }
            return;
        }

        /* ── Secondary DA / xterm 查询 (ESC [> ... c) ── */
        if (vt100_sec_mode) {
            fprintf(stderr, "[VT100-DBG] handle_csi SEC_MODE final='%c'(0x%02X) param[0]=%d\n",
                    final, (unsigned char)final, vt100_params[0]);
            switch (final) {
            case 'c': /* Secondary Device Attributes */
                /* Reply: VT100 (type 0), firmware v10, no options */
                fprintf(stderr, "[VT100-DBG] SDA reply: \\033[>0;10;0c\n");
                if (pty_master >= 0)
                    write(pty_master, "\033[>0;10;0c", 10);
                break;
            case 'm': /* xterm set-modifyOtherKeys — ignore */
                fprintf(stderr, "[VT100-DBG] SDA: set-modifyOtherKeys ignored\n");
                break;
            default:
                fprintf(stderr, "[VT100-DBG] SDA: unhandled final byte\n");
                break;
            }
            return;
        }

        switch (final) {
        /* ── 光标移动 ───────────────────── */
        case 'A': /* CUU */
            vt100_cur_row -= defparam(0, 1);
            if (vt100_cur_row < 0) vt100_cur_row = 0;
            break;
        case 'B': /* CUD */
            vt100_cur_row += defparam(0, 1);
            if (vt100_cur_row >= ROWS) vt100_cur_row = ROWS - 1;
            break;
        case 'C': /* CUF */
            vt100_cur_col += defparam(0, 1);
            if (vt100_cur_col >= COLS) vt100_cur_col = COLS - 1;
            break;
        case 'D': /* CUB */
            vt100_cur_col -= defparam(0, 1);
            if (vt100_cur_col < 0) vt100_cur_col = 0;
            break;
        case 'H': /* CUP — Cursor Position */
        case 'f': /* HVP — Horizontal Vertical Position */
            vt100_cur_row = defparam(0, 1) - 1;  /* 1-based → 0-based */
            vt100_cur_col = defparam(1, 1) - 1;
            if (vt100_cur_row < 0) vt100_cur_row = 0;
            if (vt100_cur_row >= ROWS) vt100_cur_row = ROWS - 1;
            if (vt100_cur_col < 0) vt100_cur_col = 0;
            if (vt100_cur_col >= COLS) vt100_cur_col = COLS - 1;
            break;
        case 'G': /* CHA — Cursor Horizontal Absolute */
            vt100_cur_col = defparam(0, 1) - 1;
            if (vt100_cur_col < 0) vt100_cur_col = 0;
            if (vt100_cur_col >= COLS) vt100_cur_col = COLS - 1;
            break;
        case 'd': /* VPA — Vertical Position Absolute */
            vt100_cur_row = defparam(0, 1) - 1;
            if (vt100_cur_row < 0) vt100_cur_row = 0;
            if (vt100_cur_row >= ROWS) vt100_cur_row = ROWS - 1;
            break;
        case 's': /* SCP — Save Cursor Position (ANSI) */
            vt100_saved_row  = vt100_cur_row;
            vt100_saved_col  = vt100_cur_col;
            vt100_saved_attr = vt100_cur_attr;
            vt100_saved_fg   = vt100_cur_fg;
            vt100_saved_bg   = vt100_cur_bg;
            break;
        case 'u': /* RCP — Restore Cursor Position (ANSI) */
            vt100_cur_row  = clamp(vt100_saved_row, 0, ROWS - 1);
            vt100_cur_col  = clamp(vt100_saved_col, 0, COLS - 1);
            vt100_cur_attr = vt100_saved_attr;
            vt100_cur_fg   = vt100_saved_fg;
            vt100_cur_bg   = vt100_saved_bg;
            break;

        /* ── 擦除 ───────────────────────── */
        case 'J': /* ED — Erase Display */
            vt100_erase_display(defparam(0, 0));
            break;
        case 'K': /* EL — Erase Line */
            vt100_erase_line(defparam(0, 0));
            break;

        /* ── 插入 / 删除 ────────────────── */
        case 'L': /* IL — Insert Lines */
            vt100_insert_lines(defparam(0, 1));
            break;
        case 'M': /* DL — Delete Lines (note: CSI M ≠ ESC M) */
            vt100_delete_lines(defparam(0, 1));
            break;
        case '@': /* ICH — Insert Characters */
            vt100_insert_chars(defparam(0, 1));
            break;
        case 'P': /* DCH — Delete Characters */
            vt100_delete_chars(defparam(0, 1));
            break;

        /* ── SGR — Select Graphic Rendition ── */
        case 'm':
            for (int i = 0; i < vt100_param_count; i++) {
                int val = vt100_params[i];
                if (val == 0) {
                    vt100_cur_attr = 0;
                    vt100_cur_fg = 7;   /* white (rendered as green) */
                    vt100_cur_bg = 0;
                } else if (val == 1) {
                    vt100_cur_attr |= 1;   /* ATTR_BOLD */
                } else if (val == 4) {
                    vt100_cur_attr |= 2;   /* ATTR_UNDERLINE */
                } else if (val == 5) {
                    vt100_cur_attr |= 4;   /* ATTR_BLINK */
                } else if (val == 7) {
                    vt100_cur_attr |= 8;   /* ATTR_REVERSE */
                } else if (val == 22) {
                    vt100_cur_attr &= ~1;
                } else if (val == 24) {
                    vt100_cur_attr &= ~2;
                } else if (val == 25) {
                    vt100_cur_attr &= ~4;
                } else if (val == 27) {
                    vt100_cur_attr &= ~8;
                } else if (30 <= val && val <= 37) {
                    vt100_cur_fg = val - 30;
                } else if (40 <= val && val <= 47) {
                    vt100_cur_bg = val - 40;
                } else if (val == 39) {
                    vt100_cur_fg = 7;
                } else if (val == 49) {
                    vt100_cur_bg = 0;
                }
            }
            break;

        /* ── 滚动区域 ────────────────────── */
        case 'r': /* DECSTBM — Set Top and Bottom Margins */
            /* acknowledged but not fully implemented */
            break;

        /* ── 设备控制 ────────────────────── */
        case 'c': /* DA — Device Attributes: 回复 \033[?1;0c (VT100) */
            if (pty_master >= 0) {
                const char *reply = "\033[?1;0c";
                write(pty_master, reply, strlen(reply));
            }
            break;
        case 'n': /* DSR — Device Status Report */
            fprintf(stderr, "[VT100-DBG] DSR query param[0]=%d\n", vt100_params[0]);
            if (vt100_params[0] == 5) {
                fprintf(stderr, "[VT100-DBG] DSR 5: reply \\033[0n (OK)\n");
                if (pty_master >= 0) write(pty_master, "\033[0n", 4);
            } else if (vt100_params[0] == 6) {
                /* Cursor Position Report */
                char buf[32];
                int len = snprintf(buf, sizeof(buf), "\033[%d;%dR",
                                   vt100_cur_row + 1, vt100_cur_col + 1);
                fprintf(stderr, "[VT100-DBG] DSR 6: cursor=(%d,%d) reply=%s\n",
                        vt100_cur_row + 1, vt100_cur_col + 1, buf);
                if (pty_master >= 0) write(pty_master, buf, len);
            }
            break;

        /* ── 模式设置 ────────────────────── */
        case 'h': /* SM — Set Mode (non-private) */
        case 'l': /* RM — Reset Mode (non-private) */
            break;

        default:
            break;
        }
    }

    /* ================================================================== */
    /*  主字节流解析 (完整 VT100 状态机, 移植自 src/vt100.c)                  */
    /* ================================================================== */
    void vt100_process_bytes(const char *data, int len)
    {
        /* ── DEBUG: dump raw PTY data ── */
        if (len > 0 && len < 256) {
            fprintf(stderr, "[VT100-DBG] process_bytes len=%d hex=", len);
            for (int di = 0; di < len && di < 80; di++)
                fprintf(stderr, "%02X ", (unsigned char)data[di]);
            fprintf(stderr, "\n");
        }
        for (int i = 0; i < len; i++)
        {
            unsigned char c = (unsigned char)data[i];

            /* ── 字符串终止检测 ───────────── */
            /* skip_until_st 期间 vt100_esc_state 保持不变。
             * OSC: BEL(0x07), ST(0x9C), 或裸反斜杠均可终止。
             * DCS: 仅 BEL 或 ST 终止; 反斜杠不是 DCS 终止符
             *      (DCS 使用两字节 ESC \ 序列)。
             * 因此 vt100_esc_state == VT100_ST_OSC 守卫是必要的。 */
            if (vt100_skip_until_st) {
                if (c == 0x07 || c == 0x9C ||
                    (c == '\\' && vt100_esc_state == VT100_ST_OSC)) {
                    vt100_skip_until_st = false;
                    vt100_esc_state = VT100_ST_NORMAL;
                }
                continue;
            }

            switch (vt100_esc_state) {

            case VT100_ST_NORMAL:
                if (c == 0x1B) {
                    vt100_esc_state = VT100_ST_ESC;
                    vt100_param_count = 0;
                    vt100_param_val = 0;
                    vt100_priv_mode = false;
                    vt100_sec_mode = false;
                    memset(vt100_params, 0, sizeof(vt100_params));
                } else {
                    vt100_put_char((char)c);
                }
                break;

            case VT100_ST_ESC:
                if (c == '[') {
                    vt100_esc_state = VT100_ST_CSI;
                } else if (c == ']') {
                    vt100_esc_state = VT100_ST_OSC;
                    vt100_skip_until_st = true;
                } else if (c == 'P') {
                    vt100_esc_state = VT100_ST_DCS;
                    vt100_skip_until_st = true;
                } else {
                    vt100_handle_esc((char)c);
                    vt100_esc_state = VT100_ST_NORMAL;
                }
                break;

            case VT100_ST_CSI:
                if (c == '?') {
                    vt100_priv_mode = true;
                    vt100_esc_state = VT100_ST_CSI_QM;
                } else if (c == '>') {
                    fprintf(stderr, "[VT100-DBG] CSI '>' prefix detected! sec_mode=1\n");
                    vt100_sec_mode = true;
                    vt100_esc_state = VT100_ST_CSI_GT;
                } else if (c >= '0' && c <= '9') {
                    vt100_param_val = vt100_param_val * 10 + (c - '0');
                } else if (c == ';') {
                    if (vt100_param_count < VT100_MAX_PARAMS)
                        vt100_params[vt100_param_count++] = vt100_param_val;
                    vt100_param_val = 0;
                } else if (c >= 0x20 && c <= 0x2F) {
                    /* Intermediate byte — ignored */
                } else {
                    /* Final byte (0x40-0x7E) */
                    if (vt100_param_count < VT100_MAX_PARAMS)
                        vt100_params[vt100_param_count++] = vt100_param_val;
                    vt100_handle_csi((char)c);
                    vt100_esc_state = VT100_ST_NORMAL;
                }
                break;

            case VT100_ST_CSI_QM:
                if (c >= '0' && c <= '9') {
                    vt100_param_val = vt100_param_val * 10 + (c - '0');
                } else if (c == ';') {
                    if (vt100_param_count < VT100_MAX_PARAMS)
                        vt100_params[vt100_param_count++] = vt100_param_val;
                    vt100_param_val = 0;
                } else if (c >= 0x20 && c <= 0x2F) {
                    /* intermediate */
                } else {
                    /* Final byte */
                    if (vt100_param_count < VT100_MAX_PARAMS)
                        vt100_params[vt100_param_count++] = vt100_param_val;
                    vt100_handle_csi((char)c);
                    vt100_esc_state = VT100_ST_NORMAL;
                }
                break;

            case VT100_ST_CSI_GT:
                /* ESC [> — Secondary DA / xterm extensions */
                if (c >= '0' && c <= '9') {
                    vt100_param_val = vt100_param_val * 10 + (c - '0');
                } else if (c == ';') {
                    if (vt100_param_count < VT100_MAX_PARAMS)
                        vt100_params[vt100_param_count++] = vt100_param_val;
                    vt100_param_val = 0;
                } else if (c >= 0x20 && c <= 0x2F) {
                    /* intermediate */
                } else {
                    /* Final byte */
                    if (vt100_param_count < VT100_MAX_PARAMS)
                        vt100_params[vt100_param_count++] = vt100_param_val;
                    vt100_handle_csi((char)c);
                    vt100_esc_state = VT100_ST_NORMAL;
                }
                break;

            case VT100_ST_OSC:
            case VT100_ST_DCS:
                /* Handled by skip_until_st at top of loop */
                vt100_esc_state = VT100_ST_NORMAL;
                break;

            default:
                vt100_esc_state = VT100_ST_NORMAL;
                break;
            }
        }
    }

    /* ================================================================== */
    /*  行级渲染：仅在该行内容变化时才调用 lv_label_set_text                */
    /* ================================================================== */
    static inline char sanitize_ch(char ch)
    {
        unsigned char c = (unsigned char)ch;
        if (c < 32 || c > 126)
            return ' ';
        return (char)c;
    }

    void vt100_render_row(int r)
    {
        if (r < 0 || r >= ROWS)
            return;

        char buf[COLS + 1];
        for (int c = 0; c < COLS; c++)
            buf[c] = sanitize_ch(vt100_screen[r][c]);
        buf[COLS] = '\0';

        if (memcmp(buf, row_rendered[r], COLS + 1) == 0)
            return; /* 行未变化，跳过 */

        memcpy(row_rendered[r], buf, COLS + 1);
        lv_label_set_text(row_labels[r], buf);
    }

    void vt100_render_all()
    {
        for (int r = 0; r < ROWS; r++)
            vt100_render_row(r);
        update_cursor_position_only();
    }

    /** 仅更新光标 label 的位置与字符，不改变显示/隐藏状态 */
    void update_cursor_position_only()
    {
        if (!cursor_label)
            return;
        int row = vt100_cur_row;
        int col = vt100_cur_col;
        if (row < 0) row = 0;
        if (row >= ROWS) row = ROWS - 1;
        if (col < 0) col = 0;
        if (col >= COLS) col = COLS - 1;

        char under = sanitize_ch(vt100_screen[row][col]);
        char s[2] = {under == ' ' ? ' ' : under, '\0'};
        const char *old = lv_label_get_text(cursor_label);
        if (!old || old[0] != s[0] || old[1] != s[1])
            lv_label_set_text(cursor_label, s);

        lv_obj_set_pos(cursor_label, col * CHAR_W, row * CHAR_H);
    }

    void show_cursor(bool show)
    {
        if (!cursor_label)
            return;
        if (show)
        {
            if (lv_obj_has_flag(cursor_label, LV_OBJ_FLAG_HIDDEN))
                lv_obj_clear_flag(cursor_label, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            if (!lv_obj_has_flag(cursor_label, LV_OBJ_FLAG_HIDDEN))
                lv_obj_add_flag(cursor_label, LV_OBJ_FLAG_HIDDEN);
        }
        vt100_cursor_vis = show;
    }

    /* ================================================================== */
    /*  PTY 管理                                                            */
    /* ================================================================== */
    bool start_pty(const std::string &cmd, const std::vector<std::string> &args = {})
    {
        int master, slave;
        struct winsize ws;
        ws.ws_col = COLS;
        ws.ws_row = ROWS;
        ws.ws_xpixel = TERM_W;
        ws.ws_ypixel = TERM_H;
        if (openpty(&master, &slave, NULL, NULL, &ws) < 0)
            return false;

        pid_t pid = fork();
        if (pid < 0)
        {
            close(master);
            close(slave);
            return false;
        }

        if (pid == 0)
        {
            setsid();
            dup2(slave, STDIN_FILENO);
            dup2(slave, STDOUT_FILENO);
            dup2(slave, STDERR_FILENO);
            ioctl(slave, TIOCSCTTY, 0);
            close(master);
            close(slave);
            setenv("TERM", "vt100", 1);
            setenv("PYTHONUNBUFFERED", "1", 1);
            setenv("NO_COLOR", "1", 1);

            std::vector<char *> argv;
            argv.push_back(const_cast<char *>(cmd.c_str()));
            for (const auto &a : args)
                argv.push_back(const_cast<char *>(a.c_str()));
            argv.push_back(nullptr);
            execvp(cmd.c_str(), argv.data());
            _exit(127);
        }

        close(slave);
        pty_master = master;
        child_pid = pid;
        int flags = fcntl(pty_master, F_GETFL, 0);
        fcntl(pty_master, F_SETFL, flags | O_NONBLOCK);
        return true;
    }

    void stop_pty()
    {
        if (pty_master >= 0)
        {
            close(pty_master);
            pty_master = -1;
        }
        if (child_pid > 0)
        {
            kill(child_pid, SIGTERM);
            waitpid(child_pid, NULL, 0);
            child_pid = -1;
        }
    }

    /* ================================================================== */
    /*  定时器回调                                                          */
    /* ================================================================== */
    void vt100_poll_cb(lv_timer_t *t)
    {
        (void)t;
        if (pty_master < 0 || !terminal_active)
            return;

        char buf[1024];
        ssize_t n;
        bool changed = false;
        int read_errno = 0;

        while ((n = read(pty_master, buf, sizeof(buf))) > 0)
        {
            fprintf(stderr, "[VT100-DBG] poll_cb read %zd bytes from pty_master\n", n);
            vt100_process_bytes(buf, (int)n);
            changed = true;
        }
        read_errno = errno;

        if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
            fprintf(stderr, "[VT100-DBG] poll_cb read error errno=%d\n", errno);

        if (changed)
            vt100_render_all();

        bool child_exited = false;
        if (n < 0 && read_errno == EIO)
        {
            child_exited = true;
        }
        else if (child_pid > 0)
        {
            int status = 0;
            if (waitpid(child_pid, &status, WNOHANG) == child_pid)
            {
                child_pid = -1;
                child_exited = true;
            }
        }

        if (child_exited)
        {
            terminal_active = false;
            const char *hint = "\r\n-- Press any key to exit --";
            vt100_process_bytes(hint, (int)strlen(hint));
            vt100_render_all();
            waiting_key_to_exit = true;
            if (pty_master >= 0)
            {
                close(pty_master);
                pty_master = -1;
            }
        }
    }

    void vt100_cursor_blink_cb(lv_timer_t *t)
    {
        (void)t;
        if (!cursor_label)
            return;

        update_cursor_position_only();

        if (!terminal_active)
        {
            show_cursor(false);
            return;
        }

        /* 尊重 DECTCEM 光标可见性设置 */
        if (!vt100_cursor_visible_flag)
        {
            show_cursor(false);
            return;
        }

        show_cursor(!vt100_cursor_vis);
    }

    /* ================================================================== */
    /*  按键处理                                                            */
    /* ================================================================== */

    /**
     * 将来自物理键盘（evdev keycode + utf8）转为终端字节序列写入 PTY。
     * evdev keycode 参考（linux/input-event-codes.h）：
     *   KEY_ESC=1  KEY_BACKSPACE=14  KEY_ENTER=28
     *   KEY_UP=103 KEY_LEFT=105 KEY_RIGHT=106 KEY_DOWN=108
     */
    void write_key_to_pty(uint32_t evdev_key, const char *utf8_str)
    {
        if (!terminal_active || pty_master < 0)
            return;
        char buf[8];
        int len = 0;

        switch (evdev_key)
        {
        case 28:
            buf[0] = '\r'; len = 1; break;  /* KEY_ENTER      */
        case 14:
            buf[0] = 0x7f; len = 1; break;  /* KEY_BACKSPACE  */
        case 1:
            buf[0] = 0x1b; len = 1; break;  /* KEY_ESC        */
        case 103:
            /* KEY_UP: normal=\033[A, application=\033OA */
            if (vt100_decckm) {
                buf[0]=0x1b; buf[1]='O'; buf[2]='A'; len=3;
            } else {
                buf[0]=0x1b; buf[1]='['; buf[2]='A'; len=3;
            }
            break;
        case 108:
            if (vt100_decckm) {
                buf[0]=0x1b; buf[1]='O'; buf[2]='B'; len=3;
            } else {
                buf[0]=0x1b; buf[1]='['; buf[2]='B'; len=3;
            }
            break;
        case 106:
            if (vt100_decckm) {
                buf[0]=0x1b; buf[1]='O'; buf[2]='C'; len=3;
            } else {
                buf[0]=0x1b; buf[1]='['; buf[2]='C'; len=3;
            }
            break;
        case 105:
            if (vt100_decckm) {
                buf[0]=0x1b; buf[1]='O'; buf[2]='D'; len=3;
            } else {
                buf[0]=0x1b; buf[1]='['; buf[2]='D'; len=3;
            }
            break;
        default:
            len = (int)strlen(utf8_str);
            if (len > 0 && len <= (int)sizeof(buf))
                memcpy(buf, utf8_str, (size_t)len);
            else
                len = 0;
            break;
        }

        if (len > 0) {
            fprintf(stderr, "[VT100-DBG] write_key evdev=%u decckm=%d len=%d hex=",
                    evdev_key, vt100_decckm, len);
            for (int ki = 0; ki < len; ki++)
                fprintf(stderr, "%02X ", (unsigned char)buf[ki]);
            fprintf(stderr, "\n");
            write(pty_master, buf, (size_t)len);
        }
    }

};
