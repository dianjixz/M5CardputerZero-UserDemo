#pragma once
#include "ui_app_page.hpp"
#include <unordered_map>
#include <string>
#include <vector>
#include <functional>

// ============================================================
//  系统设置界面  UISetupPage
//  屏幕分辨率: 320 x 170  (顶栏20px, ui_APP_Container 320x150)
//
//  视图状态:
//    VIEW_MAIN    — 主菜单列表（上下选择，每行有 icon + 名称 + > 右箭头）
//    VIEW_SUB     — 二级设置页面（按 ENTER 进入，按 ESC 返回主菜单）
// ============================================================

class UISetupPage : public app_base
{
    // ==================== 视图状态 ====================
    enum class ViewState { MAIN, SUB };

    // ==================== 单条菜单项描述 ====================
    struct MenuItem
    {
        const char *icon;        // 图标字符（Emoji / ASCII 符号）
        const char *label;       // 显示名称
        const char *sub_title;   // 二级页面标题
        // 二级页面内容构建回调（在 sub_content 容器中创建子控件）
        std::function<void(lv_obj_t *container)> build_sub;
        // 二级页面按键处理回调（可为空）
        std::function<void(uint32_t key)>        on_sub_key;
    };

public:
    UISetupPage() : app_base()
    {
        menu_init();
        creat_UI();
        event_handler_init();
    }
    ~UISetupPage() {}

private:
    // ==================== 数据成员 ====================
    std::unordered_map<std::string, lv_obj_t *> ui_obj_;
    std::vector<MenuItem> menu_items_;
    int   selected_idx_  = 0;   // 当前高亮行
    ViewState view_state_ = ViewState::MAIN;

    // 每行高度与可见行数（320x150 内容区，标题栏22px，余128px）
    static constexpr int ITEM_H       = 28;   // 每行高度
    static constexpr int VISIBLE_ROWS = 4;    // 可见行数
    static constexpr int LIST_Y       = 22;   // 列表起始 Y（标题栏下方）
    static constexpr int LIST_H       = 128;  // 列表区域高度

    // ==================== 菜单数据初始化 ====================
    void menu_init()
    {
        // ---- WiFi ----
        menu_items_.push_back({
            LV_SYMBOL_WIFI,
            "Wi-Fi",
            "Wi-Fi Settings",
            [](lv_obj_t *c) {
                auto make_row = [&](lv_obj_t *parent, const char *k, const char *v) {
                    lv_obj_t *row = lv_obj_create(parent);
                    lv_obj_set_size(row, 296, 28);
                    lv_obj_set_style_bg_opa(row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_radius(row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

                    lv_obj_t *lk = lv_label_create(row);
                    lv_label_set_text(lk, k);
                    lv_obj_set_pos(lk, 0, 6);
                    lv_obj_set_style_text_color(lk, lv_color_hex(0x58A6FF), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(lk, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);

                    lv_obj_t *lv2 = lv_label_create(row);
                    lv_label_set_text(lv2, v);
                    lv_obj_set_pos(lv2, 110, 6);
                    lv_obj_set_style_text_color(lv2, lv_color_hex(0xE6EDF3), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(lv2, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                };
                make_row(c, "Status :", "Connected");
                make_row(c, "SSID   :", "MyHomeWiFi");
                make_row(c, "IP     :", "192.168.1.100");
                make_row(c, "Signal :", "-55 dBm");
            },
            nullptr
        });

        // ---- Bluetooth ----
        menu_items_.push_back({
            LV_SYMBOL_BLUETOOTH,
            "Bluetooth",
            "Bluetooth Settings",
            [](lv_obj_t *c) {
                lv_obj_t *lbl = lv_label_create(c);
                lv_label_set_text(lbl, LV_SYMBOL_BLUETOOTH "  Bluetooth: OFF");
                lv_obj_set_pos(lbl, 0, 8);
                lv_obj_set_style_text_color(lbl, lv_color_hex(0xADD8E6), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

                lv_obj_t *hint = lv_label_create(c);
                lv_label_set_text(hint, "Press ENTER to toggle");
                lv_obj_set_pos(hint, 0, 36);
                lv_obj_set_style_text_color(hint, lv_color_hex(0x555555), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_font(hint, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
            },
            nullptr
        });

        // ---- Display ----
        menu_items_.push_back({
            LV_SYMBOL_IMAGE,
            "Display",
            "Display Settings",
            [](lv_obj_t *c) {
                // 亮度条
                lv_obj_t *lbl = lv_label_create(c);
                lv_label_set_text(lbl, "Brightness");
                lv_obj_set_pos(lbl, 0, 4);
                lv_obj_set_style_text_color(lbl, lv_color_hex(0xE6EDF3), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);

                lv_obj_t *bar = lv_bar_create(c);
                lv_obj_set_size(bar, 250, 12);
                lv_obj_set_pos(bar, 0, 26);
                lv_bar_set_range(bar, 0, 100);
                lv_bar_set_value(bar, 75, LV_ANIM_OFF);
                lv_obj_set_style_radius(bar, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_bg_color(bar, lv_color_hex(0x333333), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_bg_opa(bar, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_radius(bar, 4, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                lv_obj_set_style_bg_color(bar, lv_color_hex(0x1F6FEB), LV_PART_INDICATOR | LV_STATE_DEFAULT);
                lv_obj_set_style_bg_opa(bar, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

                lv_obj_t *lbl2 = lv_label_create(c);
                lv_label_set_text(lbl2, "Rotation : 0°");
                lv_obj_set_pos(lbl2, 0, 50);
                lv_obj_set_style_text_color(lbl2, lv_color_hex(0xE6EDF3), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_font(lbl2, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
            },
            nullptr
        });

        // ---- Sound ----
        menu_items_.push_back({
            LV_SYMBOL_AUDIO,
            "Sound",
            "Sound Settings",
            [](lv_obj_t *c) {
                lv_obj_t *lbl = lv_label_create(c);
                lv_label_set_text(lbl, "Volume");
                lv_obj_set_pos(lbl, 0, 4);
                lv_obj_set_style_text_color(lbl, lv_color_hex(0xE6EDF3), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);

                lv_obj_t *bar = lv_bar_create(c);
                lv_obj_set_size(bar, 250, 12);
                lv_obj_set_pos(bar, 0, 26);
                lv_bar_set_range(bar, 0, 100);
                lv_bar_set_value(bar, 60, LV_ANIM_OFF);
                lv_obj_set_style_radius(bar, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_bg_color(bar, lv_color_hex(0x333333), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_bg_opa(bar, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_radius(bar, 4, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                lv_obj_set_style_bg_color(bar, lv_color_hex(0x2ECC71), LV_PART_INDICATOR | LV_STATE_DEFAULT);
                lv_obj_set_style_bg_opa(bar, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

                lv_obj_t *lbl2 = lv_label_create(c);
                lv_label_set_text(lbl2, "Mute     : OFF");
                lv_obj_set_pos(lbl2, 0, 50);
                lv_obj_set_style_text_color(lbl2, lv_color_hex(0xE6EDF3), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_font(lbl2, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
            },
            nullptr
        });

        // ---- Power ----
        menu_items_.push_back({
            LV_SYMBOL_POWER,
            "Power",
            "Power Settings",
            [](lv_obj_t *c) {
                const char *lines[] = {
                    "Battery    : 87%",
                    "Charging   : No",
                    "Sleep Timer: 5 min",
                    "Auto Off   : 30 min",
                };
                for (int i = 0; i < 4; ++i) {
                    lv_obj_t *lbl = lv_label_create(c);
                    lv_label_set_text(lbl, lines[i]);
                    lv_obj_set_pos(lbl, 0, 4 + i * 26);
                    lv_obj_set_style_text_color(lbl, lv_color_hex(0xE6EDF3), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
            },
            nullptr
        });

        // ---- About ----
        menu_items_.push_back({
            LV_SYMBOL_LIST,
            "About",
            "About Device",
            [](lv_obj_t *c) {
                const char *lines[] = {
                    "Device  : M5Cardputer Zero",
                    "FW Ver  : v1.0.0",
                    "LVGL    : 8.3.x",
                    "Build   : " __DATE__,
                };
                for (int i = 0; i < 4; ++i) {
                    lv_obj_t *lbl = lv_label_create(c);
                    lv_label_set_text(lbl, lines[i]);
                    lv_obj_set_pos(lbl, 0, 4 + i * 26);
                    lv_obj_set_style_text_color(lbl, lv_color_hex(i == 0 ? 0x58A6FF : 0xE6EDF3),
                                                LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
            },
            nullptr
        });
    }

    // ==================== UI 构建（主菜单） ====================
    void creat_UI()
    {
        // ---- 背景 ----
        lv_obj_t *bg = lv_obj_create(ui_APP_Container);
        lv_obj_set_size(bg, 320, 150);
        lv_obj_set_pos(bg, 0, 0);
        lv_obj_set_style_radius(bg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(bg, lv_color_hex(0x0D1117), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(bg, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(bg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(bg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(bg, LV_OBJ_FLAG_SCROLLABLE);
        ui_obj_["bg"] = bg;

        // ---- 标题栏 ----
        lv_obj_t *title_bar = lv_obj_create(bg);
        lv_obj_set_size(title_bar, 320, 22);
        lv_obj_set_pos(title_bar, 0, 0);
        lv_obj_set_style_radius(title_bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(title_bar, lv_color_hex(0x1F3A5F), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(title_bar, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(title_bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(title_bar, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *lbl_title = lv_label_create(title_bar);
        lv_label_set_text(lbl_title, LV_SYMBOL_SETTINGS "  Settings");
        lv_obj_set_align(lbl_title, LV_ALIGN_LEFT_MID);
        lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t *lbl_hint = lv_label_create(title_bar);
        lv_label_set_text(lbl_hint, "UP/DN:select  ENTER:open  ESC:back");
        lv_obj_set_align(lbl_hint, LV_ALIGN_RIGHT_MID);
        lv_obj_set_x(lbl_hint, -4);
        lv_obj_set_style_text_color(lbl_hint, lv_color_hex(0x7EA8D8), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lbl_hint, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);

        // ---- 菜单列表容器（带滚动） ----
        lv_obj_t *list_cont = lv_obj_create(bg);
        lv_obj_set_size(list_cont, 320, LIST_H);
        lv_obj_set_pos(list_cont, 0, LIST_Y);
        lv_obj_set_style_radius(list_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(list_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(list_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(list_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(list_cont, LV_OBJ_FLAG_SCROLLABLE);
        ui_obj_["list_cont"] = list_cont;

        // 构建各行
        build_menu_rows();
    }

    // ==================== 构建菜单行 ====================
    void build_menu_rows()
    {
        lv_obj_t *list_cont = ui_obj_["list_cont"];

        // 删除旧行（如果存在）
        lv_obj_clean(list_cont);

        int item_count = (int)menu_items_.size();
        // 根据 selected_idx_ 计算滚动偏移，使选中行尽量居中
        int visible = LIST_H / ITEM_H;
        int offset_idx = selected_idx_ - visible / 2;
        if (offset_idx < 0) offset_idx = 0;
        if (offset_idx > item_count - visible) offset_idx = item_count - visible;
        if (offset_idx < 0) offset_idx = 0;

        for (int vi = 0; vi < visible && (vi + offset_idx) < item_count; ++vi)
        {
            int mi = vi + offset_idx;
            bool is_sel = (mi == selected_idx_);
            create_menu_row(list_cont, vi, mi, is_sel);
        }
    }

    // 创建单行菜单项
    void create_menu_row(lv_obj_t *parent, int visual_row, int menu_idx, bool selected)
    {
        const MenuItem &item = menu_items_[menu_idx];

        // 行背景
        lv_obj_t *row = lv_obj_create(parent);
        lv_obj_set_size(row, 318, ITEM_H - 2);
        lv_obj_set_pos(row, 1, visual_row * ITEM_H + 1);
        lv_obj_set_style_radius(row, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        if (selected)
        {
            lv_obj_set_style_bg_color(row, lv_color_hex(0x1F3A5F), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(row, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            // 选中行左侧高亮竖条
            lv_obj_t *sel_bar = lv_obj_create(row);
            lv_obj_set_size(sel_bar, 3, ITEM_H - 6);
            lv_obj_set_pos(sel_bar, 2, 2);
            lv_obj_set_style_radius(sel_bar, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(sel_bar, lv_color_hex(0x1F6FEB), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(sel_bar, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(sel_bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_clear_flag(sel_bar, LV_OBJ_FLAG_SCROLLABLE);
        }
        else
        {
            lv_obj_set_style_bg_color(row, lv_color_hex(0x161B22), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(row, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        }

        // 分割线（非最后行）
        if (menu_idx < (int)menu_items_.size() - 1)
        {
            lv_obj_t *div = lv_obj_create(parent);
            lv_obj_set_size(div, 310, 1);
            lv_obj_set_pos(div, 5, (visual_row + 1) * ITEM_H);
            lv_obj_set_style_radius(div, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(div, lv_color_hex(0x21262D), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(div, 200, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(div, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_clear_flag(div, LV_OBJ_FLAG_SCROLLABLE);
        }

        // Icon 标签（左侧）
        lv_obj_t *lbl_icon = lv_label_create(row);
        lv_label_set_text(lbl_icon, item.icon);
        lv_obj_set_pos(lbl_icon, 8, (ITEM_H - 16) / 2 - 1);
        lv_obj_set_style_text_color(lbl_icon,
            selected ? lv_color_hex(0x58A6FF) : lv_color_hex(0x4A7ABF),
            LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lbl_icon, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

        // 名称标签（中间）
        lv_obj_t *lbl_name = lv_label_create(row);
        lv_label_set_text(lbl_name, item.label);
        lv_obj_set_pos(lbl_name, 30, (ITEM_H - 16) / 2 - 1);
        lv_obj_set_width(lbl_name, 240);
        lv_label_set_long_mode(lbl_name, LV_LABEL_LONG_CLIP);
        lv_obj_set_style_text_color(lbl_name,
            selected ? lv_color_hex(0xFFFFFF) : lv_color_hex(0xCCCCCC),
            LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lbl_name, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

        // 右箭头符号（右侧）
        lv_obj_t *lbl_arrow = lv_label_create(row);
        lv_label_set_text(lbl_arrow, LV_SYMBOL_RIGHT);
        lv_obj_set_pos(lbl_arrow, 298, (ITEM_H - 14) / 2 - 1);
        lv_obj_set_style_text_color(lbl_arrow,
            selected ? lv_color_hex(0x58A6FF) : lv_color_hex(0x3A4A5A),
            LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lbl_arrow, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    // ==================== 打开二级页面 ====================
    void open_sub_page(int idx)
    {
        if (idx < 0 || idx >= (int)menu_items_.size()) return;
        view_state_ = ViewState::SUB;

        const MenuItem &item = menu_items_[idx];

        // ---- 二级页面面板（覆盖整个 ui_APP_Container） ----
        lv_obj_t *panel = lv_obj_create(ui_APP_Container);
        lv_obj_set_size(panel, 320, 150);
        lv_obj_set_pos(panel, 0, 0);
        lv_obj_set_style_radius(panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(panel, lv_color_hex(0x0D1117), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(panel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
        ui_obj_["sub_panel"] = panel;

        // ---- 二级标题栏 ----
        lv_obj_t *title_bar = lv_obj_create(panel);
        lv_obj_set_size(title_bar, 320, 22);
        lv_obj_set_pos(title_bar, 0, 0);
        lv_obj_set_style_radius(title_bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(title_bar, lv_color_hex(0x1F6FEB), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(title_bar, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(title_bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(title_bar, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

        // 二级标题：icon + 名称
        char title_buf[64];
        snprintf(title_buf, sizeof(title_buf), "%s  %s", item.icon, item.sub_title);
        lv_obj_t *lbl_title = lv_label_create(title_bar);
        lv_label_set_text(lbl_title, title_buf);
        lv_obj_set_align(lbl_title, LV_ALIGN_LEFT_MID);
        lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

        // 右侧提示
        lv_obj_t *lbl_hint = lv_label_create(title_bar);
        lv_label_set_text(lbl_hint, LV_SYMBOL_LEFT "  ESC: Back");
        lv_obj_set_align(lbl_hint, LV_ALIGN_RIGHT_MID);
        lv_obj_set_x(lbl_hint, -6);
        lv_obj_set_style_text_color(lbl_hint, lv_color_hex(0xAECBFA), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lbl_hint, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);

        // ---- 内容区域（可滚动） ----
        lv_obj_t *content = lv_obj_create(panel);
        lv_obj_set_size(content, 316, 124);
        lv_obj_set_pos(content, 2, 24);
        lv_obj_set_style_radius(content, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(content, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(content, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_hor(content, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_ver(content, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_scroll_dir(content, LV_DIR_VER);
        lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_AUTO);
        lv_obj_set_style_width(content, 3, LV_PART_SCROLLBAR | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(content, lv_color_hex(0x1F6FEB), LV_PART_SCROLLBAR | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(content, 200, LV_PART_SCROLLBAR | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(content, 2, LV_PART_SCROLLBAR | LV_STATE_DEFAULT);
        ui_obj_["sub_content"] = content;

        // 调用菜单项的构建回调
        if (item.build_sub)
            item.build_sub(content);
    }

    // ==================== 关闭二级页面，返回主菜单 ====================
    void close_sub_page()
    {
        if (ui_obj_.count("sub_panel") && ui_obj_["sub_panel"])
        {
            lv_obj_del(ui_obj_["sub_panel"]);
            ui_obj_["sub_panel"]   = nullptr;
            ui_obj_["sub_content"] = nullptr;
        }
        view_state_ = ViewState::MAIN;
    }

    // ==================== 事件绑定 ====================
    void event_handler_init()
    {
        lv_obj_add_event_cb(ui_root, UISetupPage::static_lvgl_handler, LV_EVENT_ALL, this);
    }
    static void static_lvgl_handler(lv_event_t *e)
    {
        UISetupPage *self = static_cast<UISetupPage *>(lv_event_get_user_data(e));
        if (self) self->event_handler(e);
    }
    void event_handler(lv_event_t *e)
    {
        if (lv_event_get_code(e) != LV_EVENT_KEY) return;
        uint32_t key = lv_event_get_key(e);

        switch (view_state_)
        {
        case ViewState::MAIN: handle_main_key(key); break;
        case ViewState::SUB:  handle_sub_key(key);  break;
        }
    }

    // ================================================================
    //  主菜单按键
    // ================================================================
    void handle_main_key(uint32_t key)
    {
        int count = (int)menu_items_.size();
        switch (key)
        {
        case LV_KEY_UP:
            if (selected_idx_ > 0)
            {
                --selected_idx_;
                build_menu_rows();
            }
            break;

        case LV_KEY_DOWN:
            if (selected_idx_ < count - 1)
            {
                ++selected_idx_;
                build_menu_rows();
            }
            break;

        case LV_KEY_ENTER:
        case LV_KEY_RIGHT:
            open_sub_page(selected_idx_);
            break;

        case LV_KEY_ESC:
            if (go_back_home) go_back_home();
            break;

        default:
            break;
        }
    }

    // ================================================================
    //  二级页面按键
    // ================================================================
    void handle_sub_key(uint32_t key)
    {
        // 优先交给菜单项自定义处理
        const MenuItem &item = menu_items_[selected_idx_];
        if (item.on_sub_key)
        {
            item.on_sub_key(key);
            if (key == LV_KEY_ESC)
                close_sub_page();
            return;
        }

        lv_obj_t *content = ui_obj_.count("sub_content") ? ui_obj_["sub_content"] : nullptr;
        switch (key)
        {
        case LV_KEY_UP:
            if (content)
                lv_obj_scroll_by(content, 0, -20, LV_ANIM_ON);
            break;

        case LV_KEY_DOWN:
            if (content)
                lv_obj_scroll_by(content, 0, 20, LV_ANIM_ON);
            break;

        case LV_KEY_ESC:
            close_sub_page();
            break;

        default:
            break;
        }
    }
};
