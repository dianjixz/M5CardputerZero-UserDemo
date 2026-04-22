#pragma once

#include "ui_app_page.hpp"
#include <unordered_map>
#include <list>
#include <string>

// ==================== 5个槽位的标准坐标 ====================
static const lv_coord_t LP_SLOT_X[] = {-177, -99,   0,  99, 177,  -177,  -99,  0,   99,  177  };
static const lv_coord_t LP_SLOT_Y[] = {    4,  -6, -16,  -6,   4,    57,   57, 50,   57,   57  };
static const lv_coord_t LP_SLOT_W[] = {   61,  81, 101,  81,  61                               };
static const lv_coord_t LP_SLOT_H[] = {   61,  81, 101,  81,  61                               };

struct lp_app_item
{
    std::string Name;
    std::string Icon;
    std::string Exec;
    bool terminal;
};

class UILaunchPage : public home_base
{
public:
    UILaunchPage() : home_base()
    {
        // -------- 初始化 app 列表 --------
        app_list_.push_back(lp_app_item{"Python",  "A:/dist/images/PYTHON_logo.png",  "python3",         true });
        app_list_.push_back(lp_app_item{"STORE",   "A:/dist/images/Store_logo.png",   "launch_store",    false});
        app_list_.push_back(lp_app_item{"CLI",     "A:/dist/images/CLI_logo.png",     "bash",            true });
        app_list_.push_back(lp_app_item{"CLAW",    "A:/dist/images/CLAW_logo.png",    "launch_claw",     false});
        app_list_.push_back(lp_app_item{"SETTING", "A:/dist/images/SETTING_logo.png", "launch_setting",  false});
        app_list_.push_back(lp_app_item{"STORE1",  "A:/dist/images/Store_logo.png",   "launch_store1",   false});
        app_list_.push_back(lp_app_item{"MUSIC",   "A:/dist/images/MUSIC_logo.png",   "launch_music",    false});
        current_app_ = 2;

        creat_UI();
        init_circles();
        // 初始化指示点
        update_indicator();
    }
    ~UILaunchPage() {}

    // ==================== 对外接口 ====================

    // 向右切换（内容向左移动，即"右翻"）
    void switch_you()
    {
        if (is_animating_) {
            delay_switch(&snap_timer_you_, [this](){ this->switch_you(); });
            return;
        }
        is_animating_ = true;

        // 1. 显示 pos0 处的面板（它即将滑入视野）
        lv_obj_clear_flag(circle_[0], LV_OBJ_FLAG_HIDDEN);

        // 2. 四个面板同时向右移一个槽位
        zuopanelout2you_Animation(circle_[0], 0, NULL);
        zuopanel2you_Animation   (circle_[1], 0, NULL);
        switchpanel2you_Animation(circle_[2], 0, NULL);
        youpanel2you_Animation   (circle_[3], 0, [](lv_anim_t *a){
            // 通过用户数据回调回到对象
            UILaunchPage *self = (UILaunchPage *)lv_anim_get_user_data(a);
            if (self) self->snap_all_panels();
        });
        // 最后一帧动画需要携带 this 指针
        // 由于 youpanel2you_Animation 的 ready_cb 不支持用户数据，
        // 改用成员计时器方案（与原 ui_events.c 保持一致）
        // —— 重新使用 50ms 定时器完成位置校正 ——
        // 注：youpanel2you_Animation 第三参数已传 NULL；校正由定时器完成
        start_snap_timer();

        // 3. 将 pos4 面板瞬移到 pos0
        snap_panel_to_slot(circle_[4], 0);

        // 4. 显示 label pos0 处的标签
        lv_obj_clear_flag(label_[0], LV_OBJ_FLAG_HIDDEN);

        // 5. 四个标签同时向右移一个槽位
        zuolabelout2you_Animation(label_[0], 0, NULL);
        zuolabel2you_Animation   (label_[1], 0, NULL);
        switchlabel2you_Animation(label_[2], 0, NULL);
        youlabel2you_Animation   (label_[3], 0, NULL);

        // 6. 将 label pos4 的标签瞬移到 label pos0
        snap_label_to_slot(label_[4], 5);
        update_you(circle_[4], label_[4]);

        // 7. 旋转数组（循环）
        disable_center_click();
        rotate_right(circle_, 0, 4);
        enable_center_click();
        rotate_right(label_,  0, 4);

        // 8. 更新指示点
        update_indicator();
    }

    // 向左切换
    void switch_zuo()
    {
        if (is_animating_) {
            delay_switch(&snap_timer_zuo_, [this](){ this->switch_zuo(); });
            return;
        }
        is_animating_ = true;

        // 1. 显示 pos4 处的面板
        lv_obj_clear_flag(circle_[4], LV_OBJ_FLAG_HIDDEN);

        // 2. 四个面板同时向左移一个槽位
        zuopanelout2zuo_Animation(circle_[4], 0, NULL);
        youpanel2zuo_Animation   (circle_[3], 0, NULL);
        switchpanel2zuo_Animation(circle_[2], 0, NULL);
        zuopanel2zuo_Animation   (circle_[1], 0, NULL);

        start_snap_timer();

        // 3. 将 pos0 面板瞬移到 pos4
        snap_panel_to_slot(circle_[0], 4);

        // 4. 显示 label pos4 处的标签
        lv_obj_clear_flag(label_[4], LV_OBJ_FLAG_HIDDEN);

        // 5. 四个标签同时向左移一个槽位
        zuolabelout2zuo_Animation(label_[4], 0, NULL);
        youlabel2zuo_Animation   (label_[3], 0, NULL);
        switchlabel2zuo_Animation(label_[2], 0, NULL);
        zuolabel2zuo_Animation   (label_[1], 0, NULL);

        // 6. 将 label pos0 的标签瞬移到 label pos4
        snap_label_to_slot(label_[0], 9);
        update_zuo(circle_[0], label_[0]);

        // 7. 旋转数组
        disable_center_click();
        rotate_left(circle_, 0, 4);
        enable_center_click();
        rotate_left(label_,  0, 4);

        // 8. 更新指示点
        update_indicator();
    }

    // 启动当前中心 App
    void launch_app()
    {
        // auto it = std::next(app_list_.begin(), current_app_);
        // if (it->Exec == "launch_store") {
        //     lv_disp_load_scr(ui_AppStore);
        //     lv_indev_set_group(lv_indev_get_next(NULL), AppStoregroup);
        // } else if (it->Exec == "launch_store1") {
        //     lv_disp_load_scr(ui_AppStore1);
        // } else if (it->Exec == "launch_setting") {
        //     printf("Launching SETTING...\n");
        // } else if (it->Exec == "launch_claw") {
        //     printf("Launching CLAW...\n");
        // } else if (it->Exec == "launch_music") {
        //     printf("Launching MUSIC...\n");
        // } else {
        //     if (it->terminal) {
        //         launch_exec_in_terminal(&(*it));
        //     } else {
        //         launch_exec(&(*it));
        //     }
        // }
    }

private:
    // ==================== 数据成员 ====================
    std::list<lp_app_item> app_list_;
    int current_app_ = 2;

    // panel 数组 [0..4]，label 数组 [0..4]
    lv_obj_t *circle_[5] = {};
    lv_obj_t *label_[5]  = {};

    // 指示点数组（复用 ui_obj 中的 Container 子对象）
    lv_obj_t *indicator_dots_[8] = {};
    int  indicator_count_ = 0;
    int  indicator_current_ = 0; // 当前激活的点（对应 current_app_ 的相对位置）

    bool is_animating_ = false;
    lv_timer_t *snap_timer_you_ = NULL;
    lv_timer_t *snap_timer_zuo_ = NULL;
    lv_timer_t *snap_timer_     = NULL; // 通用位置校正定时器

    std::unordered_map<std::string, lv_obj_t *> ui_obj_;

    // ==================== UI 构建 ====================
    void creat_UI()
    {
        // ---- pos1 左面板 ----
        circle_[1] = lv_obj_create(ui_APP_Container);
        lv_obj_set_width(circle_[1], 81);
        lv_obj_set_height(circle_[1], 81);
        lv_obj_set_x(circle_[1], LP_SLOT_X[1]);
        lv_obj_set_y(circle_[1], LP_SLOT_Y[1]);
        lv_obj_set_align(circle_[1], LV_ALIGN_CENTER);
        lv_obj_clear_flag(circle_[1], (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));
        lv_obj_set_style_radius(circle_[1], 17, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(circle_[1], lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(circle_[1], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_img_src(circle_[1], ui_img_store_logo_png, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(circle_[1], lv_color_hex(0x333333), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(circle_[1], 255, LV_PART_MAIN | LV_STATE_DEFAULT);

        // ---- pos2 中心面板 ----
        circle_[2] = lv_obj_create(ui_APP_Container);
        lv_obj_set_width(circle_[2], 101);
        lv_obj_set_height(circle_[2], 101);
        lv_obj_set_x(circle_[2], LP_SLOT_X[2]);
        lv_obj_set_y(circle_[2], LP_SLOT_Y[2]);
        lv_obj_set_align(circle_[2], LV_ALIGN_CENTER);
        lv_obj_clear_flag(circle_[2], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(circle_[2], 22, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(circle_[2], lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(circle_[2], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_img_src(circle_[2], ui_img_cli_logo_png, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(circle_[2], lv_color_hex(0x333333), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(circle_[2], 255, LV_PART_MAIN | LV_STATE_DEFAULT);

        // ---- pos3 右面板 ----
        circle_[3] = lv_obj_create(ui_APP_Container);
        lv_obj_set_width(circle_[3], 81);
        lv_obj_set_height(circle_[3], 81);
        lv_obj_set_x(circle_[3], LP_SLOT_X[3]);
        lv_obj_set_y(circle_[3], LP_SLOT_Y[3]);
        lv_obj_set_align(circle_[3], LV_ALIGN_CENTER);
        lv_obj_clear_flag(circle_[3], (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));
        lv_obj_set_style_radius(circle_[3], 17, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(circle_[3], lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(circle_[3], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_img_src(circle_[3], ui_img_claw_logo_png, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(circle_[3], lv_color_hex(0x333333), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(circle_[3], 255, LV_PART_MAIN | LV_STATE_DEFAULT);

        // ---- pos0 隐藏面板（最左外） ----
        circle_[0] = lv_obj_create(ui_APP_Container);
        lv_obj_set_width(circle_[0], LP_SLOT_W[0]);
        lv_obj_set_height(circle_[0], LP_SLOT_H[0]);
        lv_obj_set_x(circle_[0], LP_SLOT_X[0]);
        lv_obj_set_y(circle_[0], LP_SLOT_Y[0]);
        lv_obj_set_align(circle_[0], LV_ALIGN_CENTER);
        lv_obj_add_flag(circle_[0], LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(circle_[0], (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));
        lv_obj_set_style_radius(circle_[0], 17, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(circle_[0], lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(circle_[0], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_img_src(circle_[0], ui_img_python_logo_png, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(circle_[0], lv_color_hex(0x333333), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(circle_[0], 255, LV_PART_MAIN | LV_STATE_DEFAULT);

        // ---- pos4 隐藏面板（最右外） ----
        circle_[4] = lv_obj_create(ui_APP_Container);
        lv_obj_set_width(circle_[4], LP_SLOT_W[4]);
        lv_obj_set_height(circle_[4], LP_SLOT_H[4]);
        lv_obj_set_x(circle_[4], LP_SLOT_X[4]);
        lv_obj_set_y(circle_[4], LP_SLOT_Y[4]);
        lv_obj_set_align(circle_[4], LV_ALIGN_CENTER);
        lv_obj_add_flag(circle_[4], LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(circle_[4], (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));
        lv_obj_set_style_radius(circle_[4], 17, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(circle_[4], lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(circle_[4], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_img_src(circle_[4], ui_img_setting_logo_png, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(circle_[4], lv_color_hex(0x333333), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(circle_[4], 255, LV_PART_MAIN | LV_STATE_DEFAULT);

        // ---- 左标签 pos6 ----
        label_[1] = lv_label_create(ui_APP_Container);
        lv_obj_set_width(label_[1], LV_SIZE_CONTENT);
        lv_obj_set_height(label_[1], LV_SIZE_CONTENT);
        lv_obj_set_x(label_[1], LP_SLOT_X[6]);
        lv_obj_set_y(label_[1], LP_SLOT_Y[6]);
        lv_obj_set_align(label_[1], LV_ALIGN_CENTER);
        lv_label_set_text(label_[1], "STORE");
        lv_obj_set_style_text_color(label_[1], lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(label_[1], 255, LV_PART_MAIN | LV_STATE_DEFAULT);

        // ---- 中心标签 pos7 ----
        label_[2] = lv_label_create(ui_APP_Container);
        lv_obj_set_width(label_[2], LV_SIZE_CONTENT);
        lv_obj_set_height(label_[2], LV_SIZE_CONTENT);
        lv_obj_set_x(label_[2], LP_SLOT_X[7]);
        lv_obj_set_y(label_[2], LP_SLOT_Y[7]);
        lv_obj_set_align(label_[2], LV_ALIGN_CENTER);
        lv_label_set_text(label_[2], "CLI");
        lv_obj_set_style_text_color(label_[2], lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(label_[2], 255, LV_PART_MAIN | LV_STATE_DEFAULT);

        // ---- 右标签 pos8 ----
        label_[3] = lv_label_create(ui_APP_Container);
        lv_obj_set_width(label_[3], LV_SIZE_CONTENT);
        lv_obj_set_height(label_[3], LV_SIZE_CONTENT);
        lv_obj_set_x(label_[3], LP_SLOT_X[8]);
        lv_obj_set_y(label_[3], LP_SLOT_Y[8]);
        lv_obj_set_align(label_[3], LV_ALIGN_CENTER);
        lv_label_set_text(label_[3], "CLAW");
        lv_obj_set_style_text_color(label_[3], lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(label_[3], 255, LV_PART_MAIN | LV_STATE_DEFAULT);

        // ---- 左外标签 pos5（隐藏） ----
        label_[0] = lv_label_create(ui_APP_Container);
        lv_obj_set_width(label_[0], LV_SIZE_CONTENT);
        lv_obj_set_height(label_[0], LV_SIZE_CONTENT);
        lv_obj_set_x(label_[0], LP_SLOT_X[5]);
        lv_obj_set_y(label_[0], LP_SLOT_Y[5]);
        lv_obj_set_align(label_[0], LV_ALIGN_CENTER);
        lv_label_set_text(label_[0], "Python");
        lv_obj_add_flag(label_[0], LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_text_color(label_[0], lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(label_[0], 255, LV_PART_MAIN | LV_STATE_DEFAULT);

        // ---- 右外标签 pos9（隐藏） ----
        label_[4] = lv_label_create(ui_APP_Container);
        lv_obj_set_width(label_[4], LV_SIZE_CONTENT);
        lv_obj_set_height(label_[4], LV_SIZE_CONTENT);
        lv_obj_set_x(label_[4], LP_SLOT_X[9]);
        lv_obj_set_y(label_[4], LP_SLOT_Y[9]);
        lv_obj_set_align(label_[4], LV_ALIGN_CENTER);
        lv_label_set_text(label_[4], "SETTING");
        lv_obj_add_flag(label_[4], LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_text_color(label_[4], lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(label_[4], 255, LV_PART_MAIN | LV_STATE_DEFAULT);

        // ---- 左右箭头按钮 ----
        ui_obj_["ui_youbut"] = lv_btn_create(ui_APP_Container);
        lv_obj_set_width(ui_obj_["ui_youbut"], 17);
        lv_obj_set_height(ui_obj_["ui_youbut"], 23);
        lv_obj_set_x(ui_obj_["ui_youbut"], 150);
        lv_obj_set_y(ui_obj_["ui_youbut"], -14);
        lv_obj_set_align(ui_obj_["ui_youbut"], LV_ALIGN_CENTER);
        lv_obj_add_flag(ui_obj_["ui_youbut"], LV_OBJ_FLAG_SCROLL_ON_FOCUS);
        lv_obj_clear_flag(ui_obj_["ui_youbut"], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(ui_obj_["ui_youbut"], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(ui_obj_["ui_youbut"], lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(ui_obj_["ui_youbut"], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_img_src(ui_obj_["ui_youbut"], ui_img_you_png, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_color(ui_obj_["ui_youbut"], lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_opa(ui_obj_["ui_youbut"], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_event_cb(ui_obj_["ui_youbut"], [](lv_event_t *e){
            UILaunchPage *self = (UILaunchPage *)lv_event_get_user_data(e);
            if (self) self->switch_you();
        }, LV_EVENT_CLICKED, this);

        ui_obj_["ui_zuobut"] = lv_btn_create(ui_APP_Container);
        lv_obj_set_width(ui_obj_["ui_zuobut"], 17);
        lv_obj_set_height(ui_obj_["ui_zuobut"], 23);
        lv_obj_set_x(ui_obj_["ui_zuobut"], -151);
        lv_obj_set_y(ui_obj_["ui_zuobut"], -14);
        lv_obj_set_align(ui_obj_["ui_zuobut"], LV_ALIGN_CENTER);
        lv_obj_add_flag(ui_obj_["ui_zuobut"], LV_OBJ_FLAG_SCROLL_ON_FOCUS);
        lv_obj_clear_flag(ui_obj_["ui_zuobut"], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(ui_obj_["ui_zuobut"], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(ui_obj_["ui_zuobut"], lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(ui_obj_["ui_zuobut"], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_img_src(ui_obj_["ui_zuobut"], ui_img_zuo_png, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_color(ui_obj_["ui_zuobut"], lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_opa(ui_obj_["ui_zuobut"], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_event_cb(ui_obj_["ui_zuobut"], [](lv_event_t *e){
            UILaunchPage *self = (UILaunchPage *)lv_event_get_user_data(e);
            if (self) self->switch_zuo();
        }, LV_EVENT_CLICKED, this);

        // ---- 指示点容器 ----
        ui_obj_["ui_dot_container"] = lv_obj_create(ui_APP_Container);
        lv_obj_remove_style_all(ui_obj_["ui_dot_container"]);
        lv_obj_set_width(ui_obj_["ui_dot_container"], 320);
        lv_obj_set_height(ui_obj_["ui_dot_container"], 10);
        lv_obj_set_x(ui_obj_["ui_dot_container"], 0);
        lv_obj_set_y(ui_obj_["ui_dot_container"], 60);
        lv_obj_set_align(ui_obj_["ui_dot_container"], LV_ALIGN_CENTER);
        lv_obj_clear_flag(ui_obj_["ui_dot_container"], (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));
        lv_obj_set_style_layout(ui_obj_["ui_dot_container"], LV_LAYOUT_FLEX, LV_PART_MAIN);
        lv_obj_set_style_flex_flow(ui_obj_["ui_dot_container"], LV_FLEX_FLOW_ROW, LV_PART_MAIN);
        lv_obj_set_style_flex_main_place(ui_obj_["ui_dot_container"], LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_set_style_flex_cross_place(ui_obj_["ui_dot_container"], LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_set_style_pad_column(ui_obj_["ui_dot_container"], 4, LV_PART_MAIN);

        // 创建与 app_list_ 数量对应的指示点
        indicator_count_ = (int)app_list_.size();
        for (int i = 0; i < indicator_count_; i++) {
            indicator_dots_[i] = lv_obj_create(ui_obj_["ui_dot_container"]);
            lv_obj_set_width(indicator_dots_[i], 5);
            lv_obj_set_height(indicator_dots_[i], 5);
            lv_obj_clear_flag(indicator_dots_[i], LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_radius(indicator_dots_[i], LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(indicator_dots_[i], lv_color_hex(0x4A4C4A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(indicator_dots_[i], 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(indicator_dots_[i], lv_color_hex(0x4A4C4A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(indicator_dots_[i], 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }

    // ==================== 初始化 circle / label 数组的内容 ====================
    void init_circles()
    {
        // 根据当前 current_app_ 设置可见面板的图标/标签
        // 面板排布: circle_[0]隐藏(left-out), [1]左, [2]中心, [3]右, [4]隐藏(right-out)
        // 对应 app 索引: current_app_-2, current_app_-1, current_app_, current_app_+1, current_app_+2
        int sz = (int)app_list_.size();
        auto app_at = [&](int idx) -> lp_app_item & {
            idx = ((idx % sz) + sz) % sz;
            return *std::next(app_list_.begin(), idx);
        };

        // 初始化5个面板图标
        lv_obj_set_style_bg_img_src(circle_[0], app_at(current_app_ - 2).Icon.c_str(), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_img_src(circle_[1], app_at(current_app_ - 1).Icon.c_str(), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_img_src(circle_[2], app_at(current_app_    ).Icon.c_str(), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_img_src(circle_[3], app_at(current_app_ + 1).Icon.c_str(), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_img_src(circle_[4], app_at(current_app_ + 2).Icon.c_str(), LV_PART_MAIN | LV_STATE_DEFAULT);

        // 初始化5个标签文字
        lv_label_set_text(label_[0], app_at(current_app_ - 2).Name.c_str());
        lv_label_set_text(label_[1], app_at(current_app_ - 1).Name.c_str());
        lv_label_set_text(label_[2], app_at(current_app_    ).Name.c_str());
        lv_label_set_text(label_[3], app_at(current_app_ + 1).Name.c_str());
        lv_label_set_text(label_[4], app_at(current_app_ + 2).Name.c_str());
    }

    // ==================== 位置校正（动画结束后调用） ====================
    void snap_all_panels()
    {
        for (int i = 0; i < 5; i++) {
            snap_panel_to_slot(circle_[i], i);
        }
        for (int i = 0; i < 5; i++) {
            snap_label_to_slot(label_[i], i + 5);
        }
        is_animating_ = false;
    }

    static void snap_timer_cb_(lv_timer_t *timer)
    {
        UILaunchPage *self = (UILaunchPage *)lv_timer_get_user_data(timer);
        if (self) self->snap_all_panels();
        // lv_timer_set_repeat_count 设为1会自动删除
    }

    void start_snap_timer()
    {
        if (snap_timer_) return;
        snap_timer_ = lv_timer_create(snap_timer_cb_, 50, this);
        lv_timer_set_repeat_count(snap_timer_, 1);
        // 自动删除后需清空指针
        // 由于 snap_all_panels() 在回调中被调用，在回调里也清空
    }

    // ==================== 延迟切换（防抖） ====================
    struct DelayData {
        UILaunchPage *self;
        lv_timer_t  **timer_ptr;
        bool          is_you; // true=switch_you, false=switch_zuo
    };

    static void delay_timer_cb_(lv_timer_t *timer)
    {
        DelayData *d = (DelayData *)lv_timer_get_user_data(timer);
        UILaunchPage *self = d->self;
        bool is_you = d->is_you;
        *(d->timer_ptr) = NULL;
        lv_free(d);
        if (is_you) self->switch_you();
        else        self->switch_zuo();
    }

    template<typename Fn>
    void delay_switch(lv_timer_t **timer_ptr, Fn /*fn*/)
    {
        // 已有等待定时器则不重复创建
        if (*timer_ptr) return;
        bool is_you = (timer_ptr == &snap_timer_you_);
        DelayData *d = (DelayData *)lv_malloc(sizeof(DelayData));
        d->self      = this;
        d->timer_ptr = timer_ptr;
        d->is_you    = is_you;
        *timer_ptr = lv_timer_create(delay_timer_cb_, 50, d);
        lv_timer_set_repeat_count(*timer_ptr, 1);
    }

    // ==================== 槽位 snap 辅助 ====================
    static void snap_panel_to_slot(lv_obj_t *panel, int slot)
    {
        lv_obj_set_x(panel, LP_SLOT_X[slot]);
        lv_obj_set_y(panel, LP_SLOT_Y[slot]);
        lv_obj_set_width(panel,  LP_SLOT_W[slot < 5 ? slot : 4]);
        lv_obj_set_height(panel, LP_SLOT_H[slot < 5 ? slot : 4]);
        if (slot == 0 || slot == 4) {
            lv_obj_add_flag(panel, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_clear_flag(panel, LV_OBJ_FLAG_HIDDEN);
        }
    }

    static void snap_label_to_slot(lv_obj_t *label, int slot)
    {
        lv_obj_set_x(label, LP_SLOT_X[slot]);
        lv_obj_set_y(label, LP_SLOT_Y[slot]);
        if (slot == 5 || slot == 9) {
            lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_clear_flag(label, LV_OBJ_FLAG_HIDDEN);
        }
    }

    // ==================== 数组循环旋转 ====================
    static void rotate_left(lv_obj_t **arr, int start, int end)
    {
        lv_obj_t *tmp = arr[start];
        for (int i = start; i < end; i++) arr[i] = arr[i + 1];
        arr[end] = tmp;
    }

    static void rotate_right(lv_obj_t **arr, int start, int end)
    {
        lv_obj_t *tmp = arr[end];
        for (int i = end; i > start; i--) arr[i] = arr[i - 1];
        arr[start] = tmp;
    }

    // ==================== 中心面板可点击性 ====================
    void disable_center_click()
    {
        lv_obj_clear_flag(circle_[2], LV_OBJ_FLAG_CLICKABLE);
    }
    void enable_center_click()
    {
        lv_obj_add_flag(circle_[2], LV_OBJ_FLAG_CLICKABLE);
    }

    // ==================== 更新指示点 ====================
    void update_indicator()
    {
        // 激活点跟随 current_app_
        for (int i = 0; i < indicator_count_; i++) {
            if (i == current_app_) {
                // 激活：大一点、亮色
                lv_obj_set_width(indicator_dots_[i], 10);
                lv_obj_set_height(indicator_dots_[i], 10);
                lv_obj_set_style_bg_color(indicator_dots_[i], lv_color_hex(0xCCCC33), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_border_color(indicator_dots_[i], lv_color_hex(0xCCCC33), LV_PART_MAIN | LV_STATE_DEFAULT);
            } else {
                lv_obj_set_width(indicator_dots_[i], 5);
                lv_obj_set_height(indicator_dots_[i], 5);
                lv_obj_set_style_bg_color(indicator_dots_[i], lv_color_hex(0x4A4C4A), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_border_color(indicator_dots_[i], lv_color_hex(0x4A4C4A), LV_PART_MAIN | LV_STATE_DEFAULT);
            }
        }
    }

    // ==================== 更新隐藏面板的图标/标签（切换后补入新内容） ====================
    void update_you(lv_obj_t *panel, lv_obj_t *label)
    {
        // 向右切换时：panel/label 来自原 pos4，被移到 pos0（循环 you 方向）
        // current_app_ 在 rotate 前已更新（you 方向 current_app_ 减小）
        // 此处 panel 对应 current_app_-2 的内容（即从右侧补入最左侧的元素）
        current_app_ = current_app_ == 0 ? (int)app_list_.size() - 1 : current_app_ - 1;
        int sz = (int)app_list_.size();
        int prev2 = ((current_app_ - 2) % sz + sz) % sz;
        auto it = std::next(app_list_.begin(), prev2);
        lv_label_set_text(label, it->Name.c_str());
        lv_obj_set_style_bg_img_src(panel, it->Icon.c_str(), LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    void update_zuo(lv_obj_t *panel, lv_obj_t *label)
    {
        // 向左切换时：panel/label 来自原 pos0，被移到 pos4（循环 zuo 方向）
        current_app_ = current_app_ == (int)app_list_.size() - 1 ? 0 : current_app_ + 1;
        int sz = (int)app_list_.size();
        int next2 = (current_app_ + 2) % sz;
        auto it = std::next(app_list_.begin(), next2);
        lv_label_set_text(label, it->Name.c_str());
        lv_obj_set_style_bg_img_src(panel, it->Icon.c_str(), LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    // ==================== App 启动辅助 ====================
    void launch_exec_in_terminal(lp_app_item *it)
    {
        printf("Launching terminal app: %s\n", it->Exec.c_str());
        // 简单实现：直接 fork+exec，不带终端 UI
        launch_exec(it);
    }

    void launch_exec(lp_app_item *it)
    {
        printf("Launching external app: %s\n", it->Exec.c_str());
        lv_disp_t *disp  = lv_disp_get_default();
        lv_indev_t *indev = lv_indev_get_next(NULL);
        if (indev) lv_indev_set_group(indev, NULL);
        lv_timer_enable(false);
        lv_refr_now(disp);

        pid_t pid = fork();
        if (pid == 0) {
            execlp(it->Exec.c_str(), it->Exec.c_str(), NULL);
            perror("execlp failed");
            _exit(EXIT_FAILURE);
        } else if (pid > 0) {
            int status;
            waitpid(pid, &status, 0);
            printf("App %s exited with status %d\n", it->Exec.c_str(), WEXITSTATUS(status));
            lv_timer_enable(true);
            if (indev) lv_indev_set_group(lv_indev_get_next(NULL), Screen1group);
            lv_disp_load_scr(ui_Screen1);
            lv_refr_now(disp);
        } else {
            perror("fork failed");
            lv_timer_enable(true);
            if (indev) lv_indev_set_group(indev, lv_group_get_default());
        }
    }
};
