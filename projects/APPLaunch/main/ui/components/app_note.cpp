#include "../ui.h"
#include <list>
#include <iostream>
#include <string>
/* ============================================================
 * 颜色主题定义
 * ============================================================ */
#define COLOR_BG        lv_color_hex(0xF0F4F8)   /* 背景色 */
#define COLOR_CARD_BG   lv_color_hex(0xFFFFFF)   /* 卡片背景 */
#define COLOR_TITLE     lv_color_hex(0x2D3748)   /* 标题文字 */
#define COLOR_VALUE     lv_color_hex(0x1A202C)   /* 数值文字 */
#define COLOR_SUBTITLE  lv_color_hex(0x718096)   /* 副标题 */
/* 卡片强调色 */
#define COLOR_BLUE      lv_color_hex(0x4299E1)
#define COLOR_TEAL      lv_color_hex(0x38B2AC)
#define COLOR_GREEN     lv_color_hex(0x48BB78)
#define COLOR_ORANGE    lv_color_hex(0xED8936)
#define COLOR_PURPLE    lv_color_hex(0x9F7AEA)
#define COLOR_RED       lv_color_hex(0xFC8181)

/* 选中状态轮廓颜色 */
#define COLOR_SELECTED_OUTLINE  lv_color_hex(0x1191F9)



lv_obj_t *creat_note_label(lv_obj_t *parent, const std::string &text)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text.c_str());
    lv_obj_set_width(label, lv_pct(100));  /* 设置宽度为容器宽度减去边距 */
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);  /* 开启自动换行 */
    lv_obj_set_style_text_color(label, COLOR_SUBTITLE, 0);
    lv_obj_set_style_text_font(label, g_font_cn_20, 0);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 8, 0);
    return label;
}

/* ============================================================
 * 主界面创建函数
 * ============================================================ */
std::list<lv_obj_t*> note_objects;  /* 可选：保存卡片对象指针的列表，便于后续操作 */
void create_note_info(lv_obj_t *scr)
{
    lv_color_t colors[] = {
        COLOR_BLUE,
        COLOR_TEAL,
        COLOR_GREEN,
        COLOR_ORANGE,
        COLOR_PURPLE,
        COLOR_RED,
    };

    /* ---- 屏幕背景 ---- */
    lv_obj_set_style_bg_color(scr, COLOR_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    /* ============================================================
     * 主 Container（全屏 Flow 布局，无顶部标题栏）
     * ============================================================ */
    lv_obj_t *container = lv_obj_create(scr);
    lv_obj_set_size(container, lv_pct(100), lv_pct(100));
    lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(container, COLOR_BG, 0);
    lv_obj_set_style_bg_opa(container, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_radius(container, 0, 0);
    lv_obj_set_style_pad_all(container, 16, 0);
    lv_obj_set_style_pad_gap(container, 14, 0);

    /* 启用 Flow 布局（横向自动换行） */
    lv_obj_set_layout(container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(container,
                          LV_FLEX_ALIGN_START,   /* main-axis: 左对齐 */
                          LV_FLEX_ALIGN_CENTER,  /* cross-axis: 垂直居中 */
                          LV_FLEX_ALIGN_START);  /* track:      顶部对齐 */

    /* 可滚动 */
    lv_obj_add_flag(container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(container, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(container, LV_SCROLLBAR_MODE_AUTO);

    /* ============================================================
     * 循环创建卡片
     * 索引 0 → selected = true（首个卡片进入选中状态）
     * 其余  → selected = false
     * ============================================================ */
    creat_note_label(container, "nihao,zheshiyige shuoming");
    creat_note_label(container, "nihao,zheshiyige shuomingaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    creat_note_label(container, "nihao,zheshiyige shuoming阿斯asdasas你好，则是设您焊丝蒂哈斯d");




    // uint32_t count = sizeof(card_list) / sizeof(card_list[0]);
    // for (uint32_t i = 0; i < count; i++) {
    //     bool selected = (i == 0);   /* ← 仅第一张卡片为选中态 */
    //     lv_obj_t *card = create_card(container, &card_list[i], colors[i], selected);
    //     card_objects.push_back(card);
    // }
    // selected_card = card_objects.front();
}

