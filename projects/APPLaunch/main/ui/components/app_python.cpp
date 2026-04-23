/**
 * app_python.cpp
 * 基于伪终端（PTY）的 Python 交互终端实现
 *
 * 架构：
 *   openpty() 创建 master/slave 伪终端对
 *   → fork() 子进程在 slave 端执行 python3
 *   → 通过 TIOCSWINSZ 向 slave 设置终端窗口大小（列×行）
 *   → LVGL 在 ui_Container3 上构建字符网格终端（VT100 子集）
 *   → LVGL 定时器轮询 master 读端，解析 VT100 控制序列并更新字符网格
 *   → 键盘输入写入 master 写端，传递给子进程
 *
 * 颜色策略：
 *   LVGL label 不支持内联 ANSI 颜色，所有字符统一使用固定绿字黑底。
 *   ANSI SGR（\033[...m）序列在解析时直接丢弃，不影响字符输出。
 *   子进程 TERM=dumb，从源头减少程序主动发出的颜色序列。
 *
 * 终端规格（12px 等宽字体，320×144 容器）：
 *   列数 COLS = 53  （320 / 6 ≈ 53，每字符约 6px 宽）
 *   行数 ROWS = 12  （144 / 12 = 12，每字符 12px 高）
 */

#include "../ui.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <signal.h>
#if defined(__APPLE__)
#include <util.h>
#else
#include <pty.h>
#endif
#include <termios.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif
void ui_event_pythonapp_cpp(lv_event_t *e);
void app_python_handle_key(uint32_t key);
#ifdef __cplusplus
}
#endif

/* ------------------------------------------------------------------ */
/*  终端规格                                                            */
/* ------------------------------------------------------------------ */
#define TERM_W      320
#define TERM_H      144
#define CHAR_W      7    /* LiberationMono 12px 每字符约 7px 宽 */
#define CHAR_H      12
#define COLS        (TERM_W / CHAR_W)   /* 45 */
#define ROWS        (TERM_H / CHAR_H)   /* 12 */

/* 统一颜色：绿字黑底，不处理 ANSI 颜色序列 */
#define FIXED_FG    0x00FF00u
#define FIXED_BG    0x000000u

/* ------------------------------------------------------------------ */
/*  字符网格                                                            */
/* ------------------------------------------------------------------ */
static char screen[ROWS][COLS];   /* 仅存储字符，颜色固定 */

static int  cur_row = 0;
static int  cur_col = 0;

/* ------------------------------------------------------------------ */
/*  VT100 解析状态机                                                    */
/* ------------------------------------------------------------------ */
enum EscState { ESC_NORMAL, ESC_ESC, ESC_CSI, ESC_OSC };
static EscState esc_state = ESC_NORMAL;
static char     esc_buf[64];
static int      esc_len = 0;

/* ------------------------------------------------------------------ */
/*  LVGL UI 对象                                                        */
/* ------------------------------------------------------------------ */
static lv_obj_t *term_canvas = NULL;
static lv_obj_t *cell_labels[ROWS][COLS];

/* ------------------------------------------------------------------ */
/*  PTY / 进程                                                          */
/* ------------------------------------------------------------------ */
static int   pty_master = -1;
static pid_t child_pid  = -1;

/* ------------------------------------------------------------------ */
/*  LVGL 定时器                                                         */
/* ------------------------------------------------------------------ */
static lv_timer_t *poll_timer   = NULL;
static lv_timer_t *cursor_timer = NULL;
static bool        cursor_vis   = true;
static bool        terminal_active = false;

/* 上一帧光标位置，用于恢复旧格子颜色 */
static int prev_cursor_row = -1;
static int prev_cursor_col = -1;

/* ================================================================== */
/*  字符网格操作                                                        */
/* ================================================================== */

static void screen_clear_all(void) {
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            screen[r][c] = ' ';
}

static void clear_row_from(int row, int from_col) {
    for (int c = from_col; c < COLS; c++)
        screen[row][c] = ' ';
}

static void scroll_up(void) {
    for (int r = 0; r < ROWS - 1; r++)
        memcpy(screen[r], screen[r+1], COLS);
    clear_row_from(ROWS - 1, 0);
}

static void put_char(char ch) {
    if (ch == '\r') { cur_col = 0; return; }
    if (ch == '\n') {
        cur_col = 0;
        if (++cur_row >= ROWS) { scroll_up(); cur_row = ROWS - 1; }
        return;
    }
    if (ch == '\b') { if (cur_col > 0) cur_col--; return; }
    if (ch == '\t') {
        /* 展开为到下一个 4 空格制表位 */
        int next_tab = (cur_col / 4 + 1) * 4;
        if (next_tab > COLS) next_tab = COLS;
        while (cur_col < next_tab) put_char(' ');
        return;
    }
    if ((unsigned char)ch < 32) return;

    if (cur_col >= COLS) {
        cur_col = 0;
        if (++cur_row >= ROWS) { scroll_up(); cur_row = ROWS - 1; }
    }
    screen[cur_row][cur_col++] = ch;
}

/* ================================================================== */
/*  VT100 CSI 序列处理（仅光标移动和清屏/清行，丢弃 SGR 颜色）          */
/* ================================================================== */
static void handle_csi(const char *seq, int len) {
    if (len == 0) return;
    char final = seq[len - 1];

    /* 解析数字参数 */
    int params[8] = {0};
    int np = 0, cur_num = 0;
    bool has_num = false;
    for (int i = 0; i < len - 1 && np < 8; i++) {
        if (seq[i] >= '0' && seq[i] <= '9') {
            cur_num = cur_num * 10 + (seq[i] - '0');
            has_num = true;
        } else if (seq[i] == ';') {
            params[np++] = cur_num;
            cur_num = 0; has_num = false;
        }
    }
    if (has_num) params[np++] = cur_num;

    int p0 = (np >= 1) ? params[0] : 0;
    int p1 = (np >= 2) ? params[1] : 0;

    switch (final) {
        case 'A': cur_row -= (p0 ? p0 : 1); if (cur_row < 0) cur_row = 0; break;
        case 'B': cur_row += (p0 ? p0 : 1); if (cur_row >= ROWS) cur_row = ROWS-1; break;
        case 'C': cur_col += (p0 ? p0 : 1); if (cur_col >= COLS) cur_col = COLS-1; break;
        case 'D': cur_col -= (p0 ? p0 : 1); if (cur_col < 0) cur_col = 0; break;
        case 'H': case 'f':
            cur_row = (p0 > 0 ? p0 - 1 : 0);
            cur_col = (p1 > 0 ? p1 - 1 : 0);
            if (cur_row >= ROWS) cur_row = ROWS-1;
            if (cur_col >= COLS) cur_col = COLS-1;
            break;
        case 'J':
            if (p0 == 2 || p0 == 3) { screen_clear_all(); cur_row = 0; cur_col = 0; }
            else if (p0 == 0) {
                clear_row_from(cur_row, cur_col);
                for (int r = cur_row+1; r < ROWS; r++) clear_row_from(r, 0);
            }
            break;
        case 'K':
            if      (p0 == 0) clear_row_from(cur_row, cur_col);
            else if (p0 == 1) { for (int c = 0; c <= cur_col; c++) screen[cur_row][c] = ' '; }
            else if (p0 == 2) clear_row_from(cur_row, 0);
            break;
        /* 'm'（SGR 颜色）：直接忽略，不修改任何颜色 */
        default: break;
    }
}

/* ================================================================== */
/*  输入字节流解析（VT100 状态机）                                       */
/* ================================================================== */
static void process_bytes(const char *data, int len) {
    for (int i = 0; i < len; i++) {
        unsigned char c = (unsigned char)data[i];
        switch (esc_state) {
            case ESC_NORMAL:
                if (c == 0x1b) { esc_state = ESC_ESC; esc_len = 0; }
                else            put_char((char)c);
                break;
            case ESC_ESC:
                if (c == '[') { esc_state = ESC_CSI; esc_len = 0; }
                else if (c == ']') { esc_state = ESC_OSC; esc_len = 0; }  /* OSC 序列开始 */
                else if (c == 'c') {
                    screen_clear_all(); cur_row = 0; cur_col = 0;
                    esc_state = ESC_NORMAL;
                } else {
                    esc_state = ESC_NORMAL;  /* 其他双字节序列，忽略 */
                }
                break;
            case ESC_OSC:
                /* OSC 序列以 BEL(\x07) 或 ST(ESC \) 结束，整体丢弃 */
                if (c == 0x07)       esc_state = ESC_NORMAL;  /* BEL 终止 */
                else if (c == 0x1b)  esc_state = ESC_ESC;     /* ST 前缀，进入 ESC 状态 */
                /* 其他字节均为 OSC 内容，继续丢弃 */
                break;
            case ESC_CSI:
                if (esc_len < (int)(sizeof(esc_buf) - 1))
                    esc_buf[esc_len++] = (char)c;
                if (c >= 0x40 && c <= 0x7E) {   /* CSI 终止字符 */
                    esc_buf[esc_len] = '\0';
                    handle_csi(esc_buf, esc_len);
                    esc_state = ESC_NORMAL;
                    esc_len = 0;
                }
                break;
        }
    }
}

/* ================================================================== */
/*  LVGL 字符网格渲染                                                   */
/* ================================================================== */
static void render_cell(int row, int col) {
    lv_obj_t *lbl = cell_labels[row][col];
    if (!lbl) return;
    char buf[2] = { screen[row][col] ? screen[row][col] : ' ', '\0' };
    lv_label_set_text(lbl, buf);
    /* 同时确保颜色为固定值（防止残留反色） */
    lv_obj_set_style_bg_color(lbl, lv_color_hex(FIXED_BG), 0);
    lv_obj_set_style_bg_opa(lbl, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(FIXED_FG), 0);
}

static void render_all(void) {
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            render_cell(r, c);
    /* render_all 之后光标状态已被覆盖为正常颜色，重置记录 */
    prev_cursor_row = -1;
    prev_cursor_col = -1;
    cursor_vis = true;
}

/* ================================================================== */
/*  PTY 操作                                                            */
/* ================================================================== */
static bool start_pty(void) {
    int master, slave;
    struct winsize ws;
    ws.ws_col = COLS; ws.ws_row = ROWS;
    ws.ws_xpixel = TERM_W; ws.ws_ypixel = TERM_H;

    if (openpty(&master, &slave, NULL, NULL, &ws) < 0) return false;

    pid_t pid = fork();
    if (pid < 0) { close(master); close(slave); return false; }
    if (pid == 0) {
        setsid();
        dup2(slave, STDIN_FILENO);
        dup2(slave, STDOUT_FILENO);
        dup2(slave, STDERR_FILENO);
        ioctl(slave, TIOCSCTTY, 0);
        close(master); close(slave);

        /* TERM=dumb：让程序不主动发送颜色/样式转义序列 */
        setenv("TERM",             "dumb", 1);
        setenv("PYTHONUNBUFFERED", "1",    1);
        setenv("NO_COLOR",         "1",    1);   /* 通用无颜色环境变量 */
        execlp("python3", "python3", (char*)NULL);
        _exit(127);
    }

    close(slave);
    pty_master = master;
    child_pid  = pid;

    int flags = fcntl(pty_master, F_GETFL, 0);
    fcntl(pty_master, F_SETFL, flags | O_NONBLOCK);
    return true;
}

static void stop_pty(void) {
    if (pty_master >= 0) { close(pty_master); pty_master = -1; }
    if (child_pid > 0) {
        kill(child_pid, SIGTERM);
        waitpid(child_pid, NULL, 0);
        child_pid = -1;
    }
}

/* ================================================================== */
/*  LVGL 定时器回调                                                     */
/* ================================================================== */
static void poll_cb(lv_timer_t *t) {
    (void)t;
    if (pty_master < 0 || !terminal_active) return;

    char buf[256];
    ssize_t n;
    bool changed = false;

    while ((n = read(pty_master, buf, sizeof(buf))) > 0) {
        process_bytes(buf, (int)n);
        changed = true;
    }
    if (changed) render_all();

    /* 检测子进程是否已退出（read 返回 EIO 表示 slave 端已关闭） */
    bool child_exited = false;
    if (n < 0 && errno == EIO) {
        child_exited = true;
    } else if (child_pid > 0) {
        int status = 0;
        if (waitpid(child_pid, &status, WNOHANG) == child_pid) {
            child_pid = -1;
            child_exited = true;
        }
    }

    if (child_exited) {
        terminal_active = false;
        /* 切换回 home 主界面 */
        lv_disp_load_scr(ui_Screen1);
        lv_indev_set_group(lv_indev_get_next(NULL), Screen1group);
    }
}

/* 将某格恢复为正常固定颜色 */
static void restore_cell_color(int row, int col) {
    if (row < 0 || row >= ROWS || col < 0 || col >= COLS) return;
    lv_obj_t *lbl = cell_labels[row][col];
    if (!lbl) return;
    lv_obj_set_style_bg_color(lbl, lv_color_hex(FIXED_BG), 0);
    lv_obj_set_style_bg_opa(lbl, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(FIXED_FG), 0);
}

static void cursor_blink_cb(lv_timer_t *t) {
    (void)t;
    if (cur_row < 0 || cur_row >= ROWS) return;
    int col = (cur_col < COLS) ? cur_col : COLS - 1;

    /* 光标已移动：先恢复旧位置的格子颜色 */
    if (prev_cursor_row != cur_row || prev_cursor_col != col) {
        restore_cell_color(prev_cursor_row, prev_cursor_col);
        cursor_vis = true;   /* 新位置从"显示"状态开始 */
    }
    prev_cursor_row = cur_row;
    prev_cursor_col = col;

    lv_obj_t *lbl = cell_labels[cur_row][col];
    if (!lbl) return;

    cursor_vis = !cursor_vis;
    if (cursor_vis) {
        /* 显示光标：反色 */
        lv_obj_set_style_bg_color(lbl, lv_color_hex(FIXED_FG), 0);
        lv_obj_set_style_bg_opa(lbl, LV_OPA_COVER, 0);
        lv_obj_set_style_text_color(lbl, lv_color_hex(FIXED_BG), 0);
    } else {
        /* 隐藏光标：恢复正常颜色 */
        restore_cell_color(cur_row, col);
    }
}

/* ================================================================== */
/*  按键处理（对外接口）                                                 */
/* ================================================================== */
void app_python_handle_key(uint32_t key) {
    if (!terminal_active || pty_master < 0) return;
    char buf[8]; int len = 0;
    if      (key == LV_KEY_ENTER)     { buf[0] = '\r'; len = 1; }
    else if (key == LV_KEY_BACKSPACE) { buf[0] = 0x7f; len = 1; }
    else if (key == LV_KEY_ESC)       { buf[0] = 0x1b; len = 1; }  /* ESC 发送 \x1b 给终端 */
    else if (key == LV_KEY_UP)        { buf[0]=0x1b; buf[1]='['; buf[2]='A'; len=3; }
    else if (key == LV_KEY_DOWN)      { buf[0]=0x1b; buf[1]='['; buf[2]='B'; len=3; }
    else if (key == LV_KEY_RIGHT)     { buf[0]=0x1b; buf[1]='['; buf[2]='C'; len=3; }
    else if (key == LV_KEY_LEFT)      { buf[0]=0x1b; buf[1]='['; buf[2]='D'; len=3; }
    else if (key >= 32 && key <= 126) { buf[0] = (char)key; len = 1; }
    else if (key < 32)                { buf[0] = (char)key; len = 1; }
    if (len > 0) write(pty_master, buf, (size_t)len);
}

/* ================================================================== */
/*  UI 创建 / 销毁                                                      */
/* ================================================================== */
static void create_terminal_ui(void) {
    lv_obj_t *parent = ui_Container3;
    lv_obj_set_style_bg_color(parent, lv_color_hex(FIXED_BG), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    term_canvas = lv_obj_create(parent);
    lv_obj_set_size(term_canvas, TERM_W, TERM_H);
    lv_obj_set_pos(term_canvas, 0, 0);
    lv_obj_set_style_bg_color(term_canvas, lv_color_hex(FIXED_BG), 0);
    lv_obj_set_style_bg_opa(term_canvas, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(term_canvas, 0, 0);
    lv_obj_set_style_pad_all(term_canvas, 0, 0);
    lv_obj_set_style_radius(term_canvas, 0, 0);
    lv_obj_remove_flag(term_canvas, (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));

    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            lv_obj_t *lbl = lv_label_create(term_canvas);
            lv_label_set_text(lbl, " ");
            lv_obj_set_style_text_font(lbl, g_font_mono_12 ? g_font_mono_12 : g_font_cn_12, 0);
            lv_obj_set_style_text_color(lbl, lv_color_hex(FIXED_FG), 0);
            lv_obj_set_style_bg_color(lbl, lv_color_hex(FIXED_BG), 0);
            lv_obj_set_style_bg_opa(lbl, LV_OPA_COVER, 0);
            lv_obj_set_style_pad_all(lbl, 0, 0);
            lv_obj_set_size(lbl, CHAR_W, CHAR_H);
            lv_obj_set_pos(lbl, c * CHAR_W, r * CHAR_H);
            cell_labels[r][c] = lbl;
        }
    }
}

static void destroy_terminal_ui(void) {
    if (term_canvas) { lv_obj_del(term_canvas); term_canvas = NULL; }
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            cell_labels[r][c] = NULL;
}

/* ================================================================== */
/*  生命周期                                                            */
/* ================================================================== */
void app_python_init(void) {
    terminal_active = true;
    cur_row = 0; cur_col = 0;
    esc_state = ESC_NORMAL; esc_len = 0;
    screen_clear_all();
    create_terminal_ui();
    if (!start_pty()) {
        process_bytes("Error: openpty/fork failed\r\n", 27);
        render_all();
    } else {
        poll_timer   = lv_timer_create(poll_cb,         30,  NULL);
        cursor_timer = lv_timer_create(cursor_blink_cb, 500, NULL);
    }
}

void app_python_deinit(void) {
    terminal_active = false;
    if (poll_timer)   { lv_timer_del(poll_timer);   poll_timer   = NULL; }
    if (cursor_timer) { lv_timer_del(cursor_timer);  cursor_timer = NULL; }
    stop_pty();
    destroy_terminal_ui();
}

/* ================================================================== */
/*  屏幕事件回调（对外 C 接口）                                          */
/* ================================================================== */
void ui_event_pythonapp_cpp(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if      (code == LV_EVENT_SCREEN_LOAD_START)   app_python_init();
    else if (code == LV_EVENT_SCREEN_UNLOAD_START) app_python_deinit();
}
