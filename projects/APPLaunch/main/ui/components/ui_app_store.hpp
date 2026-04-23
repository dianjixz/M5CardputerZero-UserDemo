#pragma once
#include "ui_app_page.hpp"
#include <unordered_map>
#include <string>
#include <list>
#include <set>
#include <iostream>
#include <linux/input.h>
struct store_app_info
{
    std::string name;
    std::string version;
    std::string logo_icon;

    std::string class_name;
    std::string description;
};

class UIStorePage : public app_base
{
public:
    std::list<store_app_info> app_list;    // 全部app列表
    std::set<std::string> class_name_list; // 分类列表

    std::set<std::string>::const_iterator current_class;               // 当前分类
    std::list<std::list<store_app_info>::const_iterator> current_list; // 当前显示的app列表
    std::list<std::list<store_app_info>::const_iterator>::const_iterator current_app; // 当前选中的app

    bool in_detail_view_ = false; // 是否在详情面板模式

public:
    UIStorePage() : app_base()
    {
        app_list_load();
        creat_UI();
        event_handler_init();
    }
    ~UIStorePage() {}

    // ==================== 对外接口 ====================

    // 向右切换（下一个分类）
    void switch_you()
    {
        auto next = std::next(current_class);
        if (next != class_name_list.end())
        {
            current_class = next;
        }
        else
        {
            current_class = class_name_list.begin();
        }
        update_current_list();
        update_ui();
    }

    // 向左切换（上一个分类）
    void switch_zuo()
    {
        if (current_class != class_name_list.begin())
        {
            --current_class;
        }
        else
        {
            current_class = std::prev(class_name_list.end());
        }
        update_current_list();
        update_ui();
    }

    // 向下滚动（选择下一个应用）
    void switch_down()
    {
        lv_obj_t *roller = ui_obj_["ui_roller"];
        uint16_t sel = lv_roller_get_selected(roller);
        uint16_t cnt = lv_roller_get_option_cnt(roller);
        if (sel + 1 < cnt)
        {
            lv_roller_set_selected(roller, sel + 1, LV_ANIM_ON);
        }
    }

    // 向上滚动（选择上一个应用）
    void switch_up()
    {
        lv_obj_t *roller = ui_obj_["ui_roller"];
        uint16_t sel = lv_roller_get_selected(roller);
        if (sel > 0)
        {
            lv_roller_set_selected(roller, sel - 1, LV_ANIM_ON);
        }
    }

private:
    // ==================== 数据成员 ====================
    std::unordered_map<std::string, lv_obj_t *> ui_obj_;

    // ==================== UI 构建 ====================
    void creat_UI()
    {
        // ---- 底部导航栏 (ui_Panel18) ----
        ui_obj_["ui_panel_nav"] = lv_obj_create(ui_APP_Container);
        lv_obj_set_width(ui_obj_["ui_panel_nav"], 115);
        lv_obj_set_height(ui_obj_["ui_panel_nav"], 19);
        lv_obj_set_x(ui_obj_["ui_panel_nav"], 3);
        lv_obj_set_y(ui_obj_["ui_panel_nav"], 127);
        lv_obj_clear_flag(ui_obj_["ui_panel_nav"], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(ui_obj_["ui_panel_nav"], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(ui_obj_["ui_panel_nav"], lv_color_hex(0x333380), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(ui_obj_["ui_panel_nav"], 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(ui_obj_["ui_panel_nav"], 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        // ---- 左箭头 (ui_Image9) ----
        ui_obj_["ui_img_left"] = lv_img_create(ui_obj_["ui_panel_nav"]);
        lv_img_set_src(ui_obj_["ui_img_left"], ui_img_zuo_logo_png);
        lv_obj_set_width(ui_obj_["ui_img_left"], LV_SIZE_CONTENT);
        lv_obj_set_height(ui_obj_["ui_img_left"], LV_SIZE_CONTENT);
        lv_obj_set_x(ui_obj_["ui_img_left"], -50);
        lv_obj_set_y(ui_obj_["ui_img_left"], 0);
        lv_obj_set_align(ui_obj_["ui_img_left"], LV_ALIGN_CENTER);
        lv_obj_add_flag(ui_obj_["ui_img_left"], LV_OBJ_FLAG_ADV_HITTEST);
        lv_obj_clear_flag(ui_obj_["ui_img_left"], LV_OBJ_FLAG_SCROLLABLE);

        // ---- 右箭头 (ui_Image11) ----
        ui_obj_["ui_img_right"] = lv_img_create(ui_obj_["ui_panel_nav"]);
        lv_img_set_src(ui_obj_["ui_img_right"], ui_img_you_logo_png);
        lv_obj_set_width(ui_obj_["ui_img_right"], LV_SIZE_CONTENT);
        lv_obj_set_height(ui_obj_["ui_img_right"], LV_SIZE_CONTENT);
        lv_obj_set_x(ui_obj_["ui_img_right"], 50);
        lv_obj_set_y(ui_obj_["ui_img_right"], 0);
        lv_obj_set_align(ui_obj_["ui_img_right"], LV_ALIGN_CENTER);
        lv_obj_add_flag(ui_obj_["ui_img_right"], LV_OBJ_FLAG_ADV_HITTEST);
        lv_obj_clear_flag(ui_obj_["ui_img_right"], LV_OBJ_FLAG_SCROLLABLE);

        // ---- 分类名称标签 (ui_Label12) ----
        ui_obj_["ui_label_cat"] = lv_label_create(ui_obj_["ui_panel_nav"]);
        lv_obj_set_width(ui_obj_["ui_label_cat"], LV_SIZE_CONTENT);
        lv_obj_set_height(ui_obj_["ui_label_cat"], LV_SIZE_CONTENT);
        lv_obj_set_align(ui_obj_["ui_label_cat"], LV_ALIGN_CENTER);
        lv_label_set_text(ui_obj_["ui_label_cat"], current_class->c_str());
        lv_obj_set_style_text_color(ui_obj_["ui_label_cat"], lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(ui_obj_["ui_label_cat"], 255, LV_PART_MAIN | LV_STATE_DEFAULT);

        // ---- detail_info 图像 (ui_Image13) ----
        ui_obj_["ui_img_detail"] = lv_img_create(ui_APP_Container);
        lv_img_set_src(ui_obj_["ui_img_detail"], ui_img_detail_info_png);
        lv_obj_set_width(ui_obj_["ui_img_detail"], LV_SIZE_CONTENT);
        lv_obj_set_height(ui_obj_["ui_img_detail"], LV_SIZE_CONTENT);
        lv_obj_set_x(ui_obj_["ui_img_detail"], 121);
        lv_obj_set_y(ui_obj_["ui_img_detail"], 132);
        lv_obj_add_flag(ui_obj_["ui_img_detail"], LV_OBJ_FLAG_ADV_HITTEST);
        lv_obj_clear_flag(ui_obj_["ui_img_detail"], LV_OBJ_FLAG_SCROLLABLE);

        // ---- down_logo 图像 (ui_Image15) ----
        ui_obj_["ui_img_down"] = lv_img_create(ui_APP_Container);
        lv_img_set_src(ui_obj_["ui_img_down"], ui_img_down_logo_png);
        lv_obj_set_width(ui_obj_["ui_img_down"], LV_SIZE_CONTENT);
        lv_obj_set_height(ui_obj_["ui_img_down"], LV_SIZE_CONTENT);
        lv_obj_set_x(ui_obj_["ui_img_down"], 53);
        lv_obj_set_y(ui_obj_["ui_img_down"], 108);
        lv_obj_add_flag(ui_obj_["ui_img_down"], LV_OBJ_FLAG_ADV_HITTEST);
        lv_obj_clear_flag(ui_obj_["ui_img_down"], LV_OBJ_FLAG_SCROLLABLE);

        // ---- up_logo 图像 (ui_Image17) ----
        ui_obj_["ui_img_up"] = lv_img_create(ui_APP_Container);
        lv_img_set_src(ui_obj_["ui_img_up"], ui_img_up_logo_png);
        lv_obj_set_width(ui_obj_["ui_img_up"], LV_SIZE_CONTENT);
        lv_obj_set_height(ui_obj_["ui_img_up"], LV_SIZE_CONTENT);
        lv_obj_set_x(ui_obj_["ui_img_up"], 53);
        lv_obj_set_y(ui_obj_["ui_img_up"], 2);
        lv_obj_add_flag(ui_obj_["ui_img_up"], LV_OBJ_FLAG_ADV_HITTEST);
        lv_obj_clear_flag(ui_obj_["ui_img_up"], LV_OBJ_FLAG_SCROLLABLE);

        // ---- 应用图标面板 (ui_Panel19) ----
        ui_obj_["ui_panel_icon"] = lv_obj_create(ui_APP_Container);
        lv_obj_set_width(ui_obj_["ui_panel_icon"], 81);
        lv_obj_set_height(ui_obj_["ui_panel_icon"], 81);
        lv_obj_set_x(ui_obj_["ui_panel_icon"], 20);
        lv_obj_set_y(ui_obj_["ui_panel_icon"], 20);
        lv_obj_clear_flag(ui_obj_["ui_panel_icon"], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(ui_obj_["ui_panel_icon"], 16, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(ui_obj_["ui_panel_icon"], lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(ui_obj_["ui_panel_icon"], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_img_src(ui_obj_["ui_panel_icon"], ui_img_camera_png, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(ui_obj_["ui_panel_icon"], lv_color_hex(0x4D4D4D), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(ui_obj_["ui_panel_icon"], 255, LV_PART_MAIN | LV_STATE_DEFAULT);

        // ---- 应用列表滚轮 (ui_Roller1) ----
        ui_obj_["ui_roller"] = lv_roller_create(ui_APP_Container);

        std::string options;
        for(auto app : current_list)
            options += app->name + "      " + app->version + "\n";
        if (!options.empty() && options.back() == '\n')
            options.pop_back();
        std::cout << options << std::endl;
        lv_roller_set_options(ui_obj_["ui_roller"], options.c_str(), LV_ROLLER_MODE_NORMAL);
        lv_roller_set_selected(ui_obj_["ui_roller"], 0, LV_ANIM_OFF);
        lv_obj_set_width(ui_obj_["ui_roller"], 196);
        lv_obj_set_height(ui_obj_["ui_roller"], 117);
        lv_obj_set_x(ui_obj_["ui_roller"], 50);
        lv_obj_set_y(ui_obj_["ui_roller"], -14);
        lv_obj_set_align(ui_obj_["ui_roller"], LV_ALIGN_CENTER);
        lv_obj_set_style_text_align(ui_obj_["ui_roller"], LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_decor(ui_obj_["ui_roller"], LV_TEXT_DECOR_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(ui_obj_["ui_roller"], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(ui_obj_["ui_roller"], lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(ui_obj_["ui_roller"], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(ui_obj_["ui_roller"], 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_set_style_bg_color(ui_obj_["ui_roller"], lv_color_hex(0xFFFFFF), LV_PART_SELECTED | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(ui_obj_["ui_roller"], 0, LV_PART_SELECTED | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(ui_obj_["ui_roller"], &lv_font_montserrat_20, LV_PART_SELECTED | LV_STATE_DEFAULT);
    }

    // ==================== app列表初始化 ====================
    void app_list_load()
    {
        app_list.push_back(store_app_info{"Camera", "v0.1", "A:/dist/images/camera.png", "TOOLS", "camera"});
        app_list.push_back(store_app_info{"Chat", "v0.1", "A:/dist/images/chat.png", "TOOLS", "chat"});
        app_list.push_back(store_app_info{"Claw", "v0.1", "A:/dist/images/claw.png", "TOOLS", "claw"});
        app_list.push_back(store_app_info{"Cli", "v0.1", "A:/dist/images/CLI.png", "TOOLS", "cli"});
        app_list.push_back(store_app_info{"Email", "v0.1", "A:/dist/images/email.png", "TOOLS", "email"});
        app_list.push_back(store_app_info{"Gmage", "v0.1", "A:/dist/images/gmae.png", "GMAGE", "gmage"});
        app_list.push_back(store_app_info{"Hack", "v0.1", "A:/dist/images/hack.png", "HACK", "hack"});
        app_list.push_back(store_app_info{"Math", "v0.1", "A:/dist/images/math.png", "TOOLS", "math"});
        app_list.push_back(store_app_info{"Mesh", "v0.1", "A:/dist/images/mesh.png", "TOOLS", "mesh"});
        app_list.push_back(store_app_info{"Music", "v0.1", "A:/dist/images/music.png", "GMAGE", "music"});
        app_list.push_back(store_app_info{"Python", "v0.1", "A:/dist/images/python.png", "TOOLS", "python"});
        app_list.push_back(store_app_info{"Rec", "v0.1", "A:/dist/images/rec.png", "TOOLS", "rec"});
        app_list.push_back(store_app_info{"Setting", "v0.1", "A:/dist/images/setting.png", "TOOLS", "setting"});
        app_list.push_back(store_app_info{"SSH", "v0.1", "A:/dist/images/ssh.png", "TOOLS", "ssh"});

        for (const auto &app : app_list)
        {
            class_name_list.insert(app.class_name);
        }

        current_class = class_name_list.begin();

        for (auto it = app_list.cbegin(); it != app_list.cend(); ++it)
        {
            if (it->class_name == *current_class)
            {
                current_list.push_back(it);
            }
        }
    }

    void event_handler_init()
    {
        lv_obj_add_event_cb(ui_root, UIStorePage::static_lvgl_handler, LV_EVENT_ALL, this);
    }

    static void static_lvgl_handler(lv_event_t *e)
    {
        UIStorePage *self = static_cast<UIStorePage *>(lv_event_get_user_data(e));
        if (self)
        {
            self->event_handler(e);
        }
    }

    // ==================== 更新当前分类的app列表 ====================
    void update_current_list()
    {
        current_list.clear();
        for (auto it = app_list.cbegin(); it != app_list.cend(); ++it)
        {
            if (it->class_name == *current_class)
            {
                current_list.push_back(it);
            }
        }
    }

    // ==================== 显示详情面板 ====================
    void show_detail_panel()
    {
        // 隐藏主界面所有控件
        lv_obj_add_flag(ui_obj_["ui_panel_nav"],  LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_obj_["ui_img_detail"], LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_obj_["ui_img_down"],   LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_obj_["ui_img_up"],     LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_obj_["ui_panel_icon"], LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_obj_["ui_roller"],     LV_OBJ_FLAG_HIDDEN);

        // 获取当前选中app
        uint16_t sel = lv_roller_get_selected(ui_obj_["ui_roller"]);
        auto app_it = *std::next(current_list.begin(), sel);

        // ---- 外层容器：填满 ui_APP_Container，flex列布局 ----
        lv_obj_t *panel = lv_obj_create(ui_APP_Container);
        lv_obj_set_size(panel, 320, 150);
        lv_obj_set_pos(panel, 0, 0);
        lv_obj_set_style_radius(panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(panel, lv_color_hex(0x0D1117), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(panel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
        ui_obj_["ui_detail_panel"] = panel;

        // ---- 顶部标题栏 ----
        lv_obj_t *title_bar = lv_obj_create(panel);
        lv_obj_set_size(title_bar, 320, 22);
        lv_obj_set_pos(title_bar, 0, 0);
        lv_obj_set_style_radius(title_bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(title_bar, lv_color_hex(0x1F6FEB), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(title_bar, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(title_bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(title_bar, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(title_bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

        // 标题：App名 + 版本
        lv_obj_t *lbl_title = lv_label_create(title_bar);
        std::string title_str = app_it->name + "  " + app_it->version;
        lv_label_set_text(lbl_title, title_str.c_str());
        lv_obj_set_align(lbl_title, LV_ALIGN_LEFT_MID);
        lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

        // 右侧提示 [ESC返回]
        lv_obj_t *lbl_hint = lv_label_create(title_bar);
        lv_label_set_text(lbl_hint, "ESC Back");
        lv_obj_set_align(lbl_hint, LV_ALIGN_RIGHT_MID);
        lv_obj_set_x(lbl_hint, -6);
        lv_obj_set_style_text_color(lbl_hint, lv_color_hex(0xAECBFA), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lbl_hint, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);

        // ---- 可滚动内容区域 ----
        lv_obj_t *scroll_area = lv_obj_create(panel);
        lv_obj_set_size(scroll_area, 320, 128);
        lv_obj_set_pos(scroll_area, 0, 22);
        lv_obj_set_style_radius(scroll_area, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(scroll_area, lv_color_hex(0x0D1117), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(scroll_area, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(scroll_area, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(scroll_area, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        // 启用纵向滚动，关闭横向
        lv_obj_set_scroll_dir(scroll_area, LV_DIR_VER);
        lv_obj_add_flag(scroll_area, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(scroll_area, LV_SCROLLBAR_MODE_AUTO);
        lv_obj_set_style_width(scroll_area, 3, LV_PART_SCROLLBAR | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(scroll_area, lv_color_hex(0x1F6FEB), LV_PART_SCROLLBAR | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(scroll_area, 200, LV_PART_SCROLLBAR | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(scroll_area, 2, LV_PART_SCROLLBAR | LV_STATE_DEFAULT);
        // flex列布局，让内容从上到下排列
        lv_obj_set_layout(scroll_area, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(scroll_area, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(scroll_area, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_set_style_pad_left(scroll_area, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(scroll_area, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(scroll_area, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(scroll_area, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_row(scroll_area, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
        ui_obj_["ui_detail_scroll"] = scroll_area;

        // ---- 分割线辅助 lambda ----
        auto make_divider = [&]() {
            lv_obj_t *div = lv_obj_create(scroll_area);
            lv_obj_set_size(div, lv_pct(100), 1);
            lv_obj_set_style_bg_color(div, lv_color_hex(0x21262D), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(div, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(div, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(div, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(div, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_clear_flag(div, LV_OBJ_FLAG_SCROLLABLE);
        };

        // ---- 行辅助 lambda：键值对显示 ----
        auto make_kv_row = [&](const char *key, const std::string &value) {
            lv_obj_t *row = lv_obj_create(scroll_area);
            lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
            lv_obj_set_style_bg_opa(row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_layout(row, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_set_style_pad_column(row, 6, LV_PART_MAIN | LV_STATE_DEFAULT);

            lv_obj_t *lk = lv_label_create(row);
            lv_label_set_text(lk, key);
            lv_obj_set_style_text_color(lk, lv_color_hex(0x58A6FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(lk, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);

            lv_obj_t *lv2 = lv_label_create(row);
            lv_label_set_text(lv2, value.c_str());
            lv_obj_set_style_text_color(lv2, lv_color_hex(0xE6EDF3), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(lv2, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_long_mode(lv2, LV_LABEL_LONG_WRAP);
            lv_obj_set_width(lv2, 220);
        };

        // ---- 填充内容行 ----
        make_kv_row("Name    :", app_it->name);
        make_divider();
        make_kv_row("Version :", app_it->version);
        make_divider();
        make_kv_row("Class   :", app_it->class_name);
        make_divider();
        make_kv_row("Icon    :", app_it->logo_icon);
        make_divider();
        make_kv_row("Desc    :", app_it->description.empty() ? "(no description)" : app_it->description);

        in_detail_view_ = true;
    }

    // ==================== 隐藏详情面板，返回主界面 ====================
    void hide_detail_panel()
    {
        if (ui_obj_.count("ui_detail_panel") && ui_obj_["ui_detail_panel"])
        {
            lv_obj_del(ui_obj_["ui_detail_panel"]);
            ui_obj_["ui_detail_panel"] = nullptr;
            ui_obj_["ui_detail_scroll"] = nullptr;
        }

        // 恢复主界面所有控件
        lv_obj_clear_flag(ui_obj_["ui_panel_nav"],  LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui_obj_["ui_img_detail"], LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui_obj_["ui_img_down"],   LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui_obj_["ui_img_up"],     LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui_obj_["ui_panel_icon"], LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui_obj_["ui_roller"],     LV_OBJ_FLAG_HIDDEN);

        in_detail_view_ = false;
    }

    // ==================== 刷新UI显示 ====================
    void update_ui()
    {
        // 更新分类标签
        lv_label_set_text(ui_obj_["ui_label_cat"], current_class->c_str());

        // 重建roller选项（不在末尾加换行，避免roller出现多余空选项）
        std::string options;
        for (auto app : current_list)
            options += app->name + "      " + app->version + "\n";
        if (!options.empty() && options.back() == '\n')
            options.pop_back();
        lv_roller_set_options(ui_obj_["ui_roller"], options.c_str(), LV_ROLLER_MODE_NORMAL);
        lv_roller_set_selected(ui_obj_["ui_roller"], 0, LV_ANIM_OFF);

        // 更新图标为当前分类第一个app
        if (!current_list.empty())
        {
            lv_obj_set_style_bg_img_src(ui_obj_["ui_panel_icon"],
                current_list.front()->logo_icon.c_str(),
                LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }


    void event_handler(lv_event_t *e)
    {
        lv_event_code_t event_code = lv_event_get_code(e);
        if (event_code == LV_EVENT_KEY)
        {
            uint32_t key = lv_event_get_key(e);


            printf("Enter key:%d\n", key);

            // ---- 详情面板模式 ----
            if (in_detail_view_)
            {
                lv_obj_t *scroll = ui_obj_.count("ui_detail_scroll") ? ui_obj_["ui_detail_scroll"] : nullptr;
                switch (key)
                {
                case LV_KEY_UP:
                case 'f':
                case KEY_F:
                    if (scroll)
                        lv_obj_scroll_by(scroll, 0, -20, LV_ANIM_ON);
                    break;
                case LV_KEY_DOWN:
                case 'x':
                case KEY_X:
                    if (scroll)
                        lv_obj_scroll_by(scroll, 0, 20, LV_ANIM_ON);
                    break;
                case LV_KEY_ESC:
                    hide_detail_panel();
                    break;
                default:
                    break;
                }
                return;
            }

            // ---- 主界面模式 ----
            switch (key)
            {
            case LV_KEY_UP:
            case 'f':
            case KEY_F:
                {
                    uint16_t sel = lv_roller_get_selected(ui_obj_["ui_roller"]);
                    if(sel > 0) {
                        sel = sel - 1;
                        lv_roller_set_selected(ui_obj_["ui_roller"], sel, LV_ANIM_ON);
                        auto it = std::next(current_list.begin(), sel);
                        lv_obj_set_style_bg_img_src(ui_obj_["ui_panel_icon"], (*it)->logo_icon.c_str(), LV_PART_MAIN | LV_STATE_DEFAULT);
                    }
                }
                break;
            case LV_KEY_DOWN:
            case 'x':
            case KEY_X:
                {
                    uint16_t sel = lv_roller_get_selected(ui_obj_["ui_roller"]);
                    uint16_t cnt = (uint16_t)current_list.size(); // 以current_list大小为上界，防止越界
                    if(sel + 1 < cnt) {
                        sel = sel + 1;
                        lv_roller_set_selected(ui_obj_["ui_roller"], sel, LV_ANIM_ON);
                        auto it = std::next(current_list.begin(), sel);
                        lv_obj_set_style_bg_img_src(ui_obj_["ui_panel_icon"], (*it)->logo_icon.c_str(), LV_PART_MAIN | LV_STATE_DEFAULT);
                    }
                }
                break;
            case LV_KEY_LEFT:
            case 'z':
            case KEY_Z:
                switch_zuo();
                break;
            case LV_KEY_RIGHT:
            case 'c':
            case KEY_C:
                switch_you();
                break;
            case KEY_TAB:           /* Tab 键经 main.cpp 映射为 'i'，避免被 LVGL group 拦截 */
                if (!current_list.empty())
                    show_detail_panel();
                break;
            case LV_KEY_ENTER:
                /* code */
                break;
            case LV_KEY_ESC:
                go_back_home();
                break;
            default:
                break;
            }
        }
    }
};
