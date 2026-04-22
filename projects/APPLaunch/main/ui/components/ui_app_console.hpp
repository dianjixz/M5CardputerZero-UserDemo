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
//    - VT100/ANSI 终端仿真（支持 CSI 光标移动、清屏、擦除等）
//    - PTY 子进程管理（fork + openpty）
//    - 行级脏标记渲染，减少 LVGL 刷新开销
//    - 光标闪烁（500ms 定时器）
//    - 键盘输入转发至 PTY（evdev keycode + utf8 / LV_KEY_*）
//
//  对外接口:
//    exec(std::string cmd)  — 启动一条命令（支持带参数的命令字符串）
//    handle_key(uint32_t)   — 转发 LV_KEY_* 按键
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
    static constexpr int COLS   = TERM_W / CHAR_W;   /* 45 */
    static constexpr int ROWS   = TERM_H / CHAR_H;   /* 12 */

    /* 绿字黑底 */
    static constexpr uint32_t FIXED_FG = 0x00FF00u;
    static constexpr uint32_t FIXED_BG = 0x000000u;

    /* ------------------------------------------------------------------ */
    /*  UI 对象                                                             */
    /* ------------------------------------------------------------------ */
    lv_obj_t *terminal_container = nullptr;
    lv_obj_t *term_canvas        = nullptr;

    /* 整行渲染：ROWS 个 label 替代大量 cell label */
    lv_obj_t *row_labels[ROWS]   = {};
    /* 光标：独立一个反色 label，仅创建时设置一次样式 */
    lv_obj_t *cursor_label       = nullptr;

    /* 行级 dirty 比对缓存（含末尾 '\0'） */
    char row_rendered[ROWS][COLS + 1] = {};

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
        if (poll_timer)   { lv_timer_delete(poll_timer);   poll_timer   = nullptr; }
        if (cursor_timer) { lv_timer_delete(cursor_timer); cursor_timer = nullptr; }
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
        if (child_pid > 0) stop_pty();

        terminal_active     = true;
        vt100_cur_row       = 0;
        vt100_cur_col       = 0;
        vt100_esc_state     = VT100_ESC_NORMAL;
        vt100_esc_len       = 0;
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

        if (tokens.empty()) {
            const char *err = "Error: empty command\r\n";
            vt100_process_bytes(err, (int)strlen(err));
            vt100_render_all();
            terminal_active = false;
            return;
        }

        std::string executable = tokens[0];
        std::vector<std::string> args(tokens.begin() + 1, tokens.end());

        if (!start_pty(executable, args)) {
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

    /** 转发来自 LV_EVENT_KEY 的按键（LV_KEY_* 值域） */
    void handle_key(uint32_t key)
    {
        if (waiting_key_to_exit) {
            waiting_key_to_exit = false;
            if (go_back_home) go_back_home();
            return;
        }
        app_console_handle_lv_key(key);
    }

private:
    /* ================================================================== */
    /*  VT100 字符网格状态                                                  */
    /* ================================================================== */
    char vt100_screen[ROWS][COLS] = {};
    int  vt100_cur_row = 0;
    int  vt100_cur_col = 0;

    enum vt100_EscState {
        VT100_ESC_NORMAL,
        VT100_ESC_ESC,
        VT100_ESC_CSI,
        VT100_ESC_OSC
    };
    vt100_EscState vt100_esc_state = VT100_ESC_NORMAL;
    char           vt100_esc_buf[64] = {};
    int            vt100_esc_len     = 0;

    int   pty_master = -1;
    pid_t child_pid  = -1;

    lv_timer_t *poll_timer   = nullptr;
    lv_timer_t *cursor_timer = nullptr;
    bool vt100_cursor_vis    = false;
    bool terminal_active     = false;
    bool waiting_key_to_exit = false;

    /* ================================================================== */
    /*  初始化                                                              */
    /* ================================================================== */
    void console_data_init()
    {
        memset(vt100_screen,   ' ', sizeof(vt100_screen));
        memset(row_rendered,    0,   sizeof(row_rendered));
        memset(row_labels,      0,   sizeof(row_labels));
        memset(vt100_esc_buf,   0,   sizeof(vt100_esc_buf));
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
        for (int r = 0; r < ROWS; r++) {
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
        if (self) self->event_handler(e);
    }

    void event_handler(lv_event_t *e)
    {
        lv_event_code_t code = lv_event_get_code(e);
        if (code == LV_EVENT_KEY) {
            // uint32_t key = lv_event_get_key(e);
            // handle_key(key);
        }else if (code == LV_EVENT_KEYBOARD) {
            struct key_item * elm = (struct key_item *)lv_event_get_param(e);
            if (waiting_key_to_exit) {
                waiting_key_to_exit = false;
                if (go_back_home) go_back_home();
            } else {
                if (pty_master >= 0 && terminal_active) {
                    if (!elm->key_state)
                        write_key_to_pty(elm->key_code, elm->utf8);
                }
            }
            printf("Received LV_EVENT_KEYBOARD event: elm=%p\n", elm);
        }
    }


    /* ================================================================== */
    /*  LVGL 定时器静态包装                                                 */
    /* ================================================================== */
    static void s_poll_cb(lv_timer_t *t)
    {
        auto self = (UIConsolePage *)lv_timer_get_user_data(t);
        /* 出队列：读取物理键盘事件 */
        // {
        //     pthread_mutex_lock(&keyboard_mutex);
        //     if (!STAILQ_EMPTY(&keyboard_queue))
        //     {
        //         struct key_item *elm = STAILQ_FIRST(&keyboard_queue);
        //         STAILQ_REMOVE_HEAD(&keyboard_queue, entries);

        //         if (self->waiting_key_to_exit) {
        //             self->waiting_key_to_exit = false;
        //             if (self->go_back_home) self->go_back_home();
        //         } else {
        //             if (self->pty_master >= 0 && self->terminal_active) {
        //                 if (!elm->key_state)
        //                     self->write_key_to_pty(elm->key_code, elm->utf8);
        //             }
        //         }
        //         free(elm);
        //     }
        //     pthread_mutex_unlock(&keyboard_mutex);
        // }
        if (self) self->vt100_poll_cb(t);
    }

    static void s_cursor_blink_cb(lv_timer_t *t)
    {
        auto self = (UIConsolePage *)lv_timer_get_user_data(t);
        if (self) self->vt100_cursor_blink_cb(t);
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

    void vt100_scroll_up()
    {
        for (int r = 0; r < ROWS - 1; r++)
            memcpy(vt100_screen[r], vt100_screen[r + 1], COLS);
        vt100_clear_row_from(ROWS - 1, 0);
    }

    void vt100_put_char(char ch)
    {
        if (ch == '\r') { vt100_cur_col = 0; return; }
        if (ch == '\n') {
            vt100_cur_col = 0;
            if (++vt100_cur_row >= ROWS) { vt100_scroll_up(); vt100_cur_row = ROWS - 1; }
            return;
        }
        if (ch == '\b') { if (vt100_cur_col > 0) vt100_cur_col--; return; }
        if (ch == '\t') {
            int next_tab = (vt100_cur_col / 4 + 1) * 4;
            if (next_tab > COLS) next_tab = COLS;
            while (vt100_cur_col < next_tab) vt100_put_char(' ');
            return;
        }
        if ((unsigned char)ch < 32) return;

        if (vt100_cur_col >= COLS) {
            vt100_cur_col = 0;
            if (++vt100_cur_row >= ROWS) { vt100_scroll_up(); vt100_cur_row = ROWS - 1; }
        }
        vt100_screen[vt100_cur_row][vt100_cur_col++] = ch;
    }

    /* ================================================================== */
    /*  VT100 CSI 序列处理                                                  */
    /* ================================================================== */
    void vt100_handle_csi(const char *seq, int len)
    {
        if (len == 0) return;
        char final = seq[len - 1];

        int params[8] = {0};
        int np = 0, cur_num = 0;
        bool has_num = false;
        for (int i = 0; i < len - 1 && np < 8; i++) {
            if (seq[i] >= '0' && seq[i] <= '9') {
                cur_num = cur_num * 10 + (seq[i] - '0');
                has_num = true;
            } else if (seq[i] == ';') {
                params[np++] = cur_num;
                cur_num = 0;
                has_num = false;
            }
        }
        if (has_num) params[np++] = cur_num;
        int p0 = (np >= 1) ? params[0] : 0;
        int p1 = (np >= 2) ? params[1] : 0;

        switch (final) {
        case 'A':
            vt100_cur_row -= (p0 ? p0 : 1);
            if (vt100_cur_row < 0) vt100_cur_row = 0;
            break;
        case 'B':
            vt100_cur_row += (p0 ? p0 : 1);
            if (vt100_cur_row >= ROWS) vt100_cur_row = ROWS - 1;
            break;
        case 'C':
            vt100_cur_col += (p0 ? p0 : 1);
            if (vt100_cur_col >= COLS) vt100_cur_col = COLS - 1;
            break;
        case 'D':
            vt100_cur_col -= (p0 ? p0 : 1);
            if (vt100_cur_col < 0) vt100_cur_col = 0;
            break;
        case 'H':
        case 'f':
            vt100_cur_row = (p0 > 0 ? p0 - 1 : 0);
            vt100_cur_col = (p1 > 0 ? p1 - 1 : 0);
            if (vt100_cur_row >= ROWS) vt100_cur_row = ROWS - 1;
            if (vt100_cur_col >= COLS) vt100_cur_col = COLS - 1;
            break;
        case 'J':
            if (p0 == 2 || p0 == 3) {
                vt100_screen_clear_all();
                vt100_cur_row = 0;
                vt100_cur_col = 0;
            } else if (p0 == 0) {
                vt100_clear_row_from(vt100_cur_row, vt100_cur_col);
                for (int r = vt100_cur_row + 1; r < ROWS; r++)
                    vt100_clear_row_from(r, 0);
            }
            break;
        case 'K':
            if (p0 == 0)
                vt100_clear_row_from(vt100_cur_row, vt100_cur_col);
            else if (p0 == 1) {
                for (int c = 0; c <= vt100_cur_col && c < COLS; c++)
                    vt100_screen[vt100_cur_row][c] = ' ';
            }
            else if (p0 == 2)
                vt100_clear_row_from(vt100_cur_row, 0);
            break;
        default:
            break;
        }
    }

    /* ================================================================== */
    /*  输入字节流解析                                                      */
    /* ================================================================== */
    void vt100_process_bytes(const char *data, int len)
    {
        for (int i = 0; i < len; i++) {
            unsigned char c = (unsigned char)data[i];
            switch (vt100_esc_state) {
            case VT100_ESC_NORMAL:
                if (c == 0x1b) { vt100_esc_state = VT100_ESC_ESC; vt100_esc_len = 0; }
                else vt100_put_char((char)c);
                break;
            case VT100_ESC_ESC:
                if (c == '[') { vt100_esc_state = VT100_ESC_CSI; vt100_esc_len = 0; }
                else if (c == ']') { vt100_esc_state = VT100_ESC_OSC; vt100_esc_len = 0; }
                else if (c == 'c') {
                    vt100_screen_clear_all();
                    vt100_cur_row = 0;
                    vt100_cur_col = 0;
                    vt100_esc_state = VT100_ESC_NORMAL;
                }
                else vt100_esc_state = VT100_ESC_NORMAL;
                break;
            case VT100_ESC_OSC:
                if (c == 0x07) vt100_esc_state = VT100_ESC_NORMAL;
                else if (c == 0x1b) vt100_esc_state = VT100_ESC_ESC;
                break;
            case VT100_ESC_CSI:
                if (vt100_esc_len < (int)(sizeof(vt100_esc_buf) - 1))
                    vt100_esc_buf[vt100_esc_len++] = (char)c;
                if (c >= 0x40 && c <= 0x7E) {
                    vt100_esc_buf[vt100_esc_len] = '\0';
                    vt100_handle_csi(vt100_esc_buf, vt100_esc_len);
                    vt100_esc_state = VT100_ESC_NORMAL;
                    vt100_esc_len = 0;
                }
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
        if (c < 32 || c > 126) return ' ';
        return (char)c;
    }

    void vt100_render_row(int r)
    {
        if (r < 0 || r >= ROWS) return;

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
        if (!cursor_label) return;
        int row = vt100_cur_row;
        int col = vt100_cur_col;
        if (row < 0) row = 0;
        if (row >= ROWS) row = ROWS - 1;
        if (col < 0) col = 0;
        if (col >= COLS) col = COLS - 1;

        char under = sanitize_ch(vt100_screen[row][col]);
        char s[2]  = { under == ' ' ? ' ' : under, '\0' };
        const char *old = lv_label_get_text(cursor_label);
        if (!old || old[0] != s[0] || old[1] != s[1])
            lv_label_set_text(cursor_label, s);

        lv_obj_set_pos(cursor_label, col * CHAR_W, row * CHAR_H);
    }

    void show_cursor(bool show)
    {
        if (!cursor_label) return;
        if (show) {
            if (lv_obj_has_flag(cursor_label, LV_OBJ_FLAG_HIDDEN))
                lv_obj_clear_flag(cursor_label, LV_OBJ_FLAG_HIDDEN);
        } else {
            if (!lv_obj_has_flag(cursor_label, LV_OBJ_FLAG_HIDDEN))
                lv_obj_add_flag(cursor_label, LV_OBJ_FLAG_HIDDEN);
        }
        vt100_cursor_vis = show;
    }

    /* ================================================================== */
    /*  PTY 管理                                                            */
    /* ================================================================== */
    bool start_pty(const std::string& cmd, const std::vector<std::string>& args = {})
    {
        int master, slave;
        struct winsize ws;
        ws.ws_col    = COLS;
        ws.ws_row    = ROWS;
        ws.ws_xpixel = TERM_W;
        ws.ws_ypixel = TERM_H;
        if (openpty(&master, &slave, NULL, NULL, &ws) < 0)
            return false;

        pid_t pid = fork();
        if (pid < 0) { close(master); close(slave); return false; }

        if (pid == 0) {
            setsid();
            dup2(slave, STDIN_FILENO);
            dup2(slave, STDOUT_FILENO);
            dup2(slave, STDERR_FILENO);
            ioctl(slave, TIOCSCTTY, 0);
            close(master);
            close(slave);
            setenv("TERM", "xterm-256color", 1);
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
        child_pid  = pid;
        int flags = fcntl(pty_master, F_GETFL, 0);
        fcntl(pty_master, F_SETFL, flags | O_NONBLOCK);
        return true;
    }

    void stop_pty()
    {
        if (pty_master >= 0) { close(pty_master); pty_master = -1; }
        if (child_pid > 0) {
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
        if (pty_master < 0 || !terminal_active) return;

        char buf[1024];
        ssize_t n;
        bool changed  = false;
        int read_errno = 0;

        while ((n = read(pty_master, buf, sizeof(buf))) > 0) {
            vt100_process_bytes(buf, (int)n);
            changed = true;
        }
        read_errno = errno;

        if (changed)
            vt100_render_all();

        bool child_exited = false;
        if (n < 0 && read_errno == EIO) {
            child_exited = true;
        } else if (child_pid > 0) {
            int status = 0;
            if (waitpid(child_pid, &status, WNOHANG) == child_pid) {
                child_pid    = -1;
                child_exited = true;
            }
        }

        if (child_exited) {
            terminal_active = false;
            const char *hint = "\r\n-- Press any key to exit --";
            vt100_process_bytes(hint, (int)strlen(hint));
            vt100_render_all();
            waiting_key_to_exit = true;
            if (pty_master >= 0) { close(pty_master); pty_master = -1; }
        }
    }

    void vt100_cursor_blink_cb(lv_timer_t *t)
    {
        (void)t;
        if (!cursor_label) return;

        update_cursor_position_only();

        if (!terminal_active) {
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
        if (!terminal_active || pty_master < 0) return;
        char buf[8];
        int  len = 0;

        switch (evdev_key) {
        case 28:  buf[0] = '\r';  len = 1; break;               /* KEY_ENTER      */
        case 14:  buf[0] = 0x7f;  len = 1; break;               /* KEY_BACKSPACE  */
        case 1:   buf[0] = 0x1b;  len = 1; break;               /* KEY_ESC        */
        case 103: buf[0]=0x1b; buf[1]='['; buf[2]='A'; len=3; break; /* KEY_UP    */
        case 108: buf[0]=0x1b; buf[1]='['; buf[2]='B'; len=3; break; /* KEY_DOWN  */
        case 106: buf[0]=0x1b; buf[1]='['; buf[2]='C'; len=3; break; /* KEY_RIGHT */
        case 105: buf[0]=0x1b; buf[1]='['; buf[2]='D'; len=3; break; /* KEY_LEFT  */
        default:
            len = (int)strlen(utf8_str);
            if (len > 0 && len <= (int)sizeof(buf))
                memcpy(buf, utf8_str, (size_t)len);
            else
                len = 0;
            break;
        }

        if (len > 0) write(pty_master, buf, (size_t)len);
    }

    /** 将 LVGL LV_KEY_* 按键转为终端字节序列写入 PTY */
    void app_console_handle_lv_key(uint32_t key)
    {
        if (!terminal_active || pty_master < 0) return;
        char buf[8];
        int  len = 0;

        if      (key == LV_KEY_ENTER)     { buf[0] = '\r';  len = 1; }
        else if (key == LV_KEY_BACKSPACE) { buf[0] = 0x7f;  len = 1; }
        else if (key == LV_KEY_ESC)       { buf[0] = 0x1b;  len = 1; }
        else if (key == LV_KEY_UP)        { buf[0]=0x1b; buf[1]='['; buf[2]='A'; len=3; }
        else if (key == LV_KEY_DOWN)      { buf[0]=0x1b; buf[1]='['; buf[2]='B'; len=3; }
        else if (key == LV_KEY_RIGHT)     { buf[0]=0x1b; buf[1]='['; buf[2]='C'; len=3; }
        else if (key == LV_KEY_LEFT)      { buf[0]=0x1b; buf[1]='['; buf[2]='D'; len=3; }
        else if (key >= 32 && key <= 126) { buf[0] = (char)key; len = 1; }
        else if (key < 32)                { buf[0] = (char)key; len = 1; }

        if (len > 0) write(pty_master, buf, (size_t)len);
    }
};
