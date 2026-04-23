#ifndef __MAIN__H__
#define __MAIN__H__

#include <sys/queue.h>
#include <pthread.h>
#ifdef __cplusplus
extern "C" {
#endif

// 修饰键位图
#define KBD_MOD_SHIFT  (1u << 0)
#define KBD_MOD_CTRL   (1u << 1)
#define KBD_MOD_ALT    (1u << 2)
#define KBD_MOD_LOGO   (1u << 3)
#define KBD_MOD_CAPS   (1u << 4)
#define KBD_MOD_NUM    (1u << 5)

// 按键状态
#define KBD_KEY_RELEASED  0
#define KBD_KEY_PRESSED   1
#define KBD_KEY_REPEATED  2

struct key_item {
    uint32_t key_code;      // Linux evdev 键码
    uint32_t keysym;        // 主 XKB keysym (xkb_keysym_t)
    uint32_t codepoint;     // 对应的 Unicode 码点，无则为 0
    uint32_t mods;          // 修饰键位图 (KBD_MOD_*)
    int      key_state;     // 0=释放, 1=按下, 2=重复
    char     sym_name[65];  // XKB keysym 名字
    char     utf8[16];      // UTF-8 字符（兼容 compose 产出的多字节）
    char     flage;         // 判断是否需要 free
    STAILQ_ENTRY(key_item) entries;
};

STAILQ_HEAD(keyboard_queue_t, key_item);
extern struct keyboard_queue_t keyboard_queue;
extern pthread_mutex_t keyboard_mutex;
extern volatile int LVGL_HOME_KEY_FLAGE;
extern volatile int LVGL_RUN_FLAGE;
void *keyboard_read_thread(void *argv);
#ifdef __cplusplus
}
#endif
#endif