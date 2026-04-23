#pragma once
#include "../ui.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unordered_map>
#include <list>
#include <memory>
#include <string>
#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
#include <vector>
#include <keyboard_input.h>
#include <functional>
#define APP_CONSOLE_EXIT_EVENT (lv_event_code_t)(LV_EVENT_LAST + 1)


class home_base
{
private:
    lv_obj_t *ui_TOP_logo;
    lv_obj_t *ui_TOP_time;
    lv_obj_t *ui_TOP_time_Label;
    lv_obj_t *ui_TOP_Power;
    lv_obj_t *ui_TOP_power_Label;

public:
    lv_group_t *key_group;
    lv_obj_t *ui_APP_Container;
    lv_obj_t *ui_root;
    std::function<void(void)> go_back_home;
    lv_obj_t *get_ui() { return ui_root; }
    lv_group_t *get_key_group() { return key_group; }


public:
    home_base()
    {
        creat_base_UI();
        creat_input_group();
        UI_bind_event();
    }
    ~home_base()
    {
        lv_obj_del(ui_root);
    }

private:
    /* ================================================================== */
    /*  UI 构建                                                             */
    /* ================================================================== */
    void creat_base_UI()
    {
        ui_root = lv_obj_create(NULL);
        lv_obj_clear_flag(ui_root, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
        lv_obj_set_style_bg_color(ui_root, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(ui_root, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

        ui_TOP_logo = lv_img_create(ui_root);
        lv_img_set_src(ui_TOP_logo, ui_img_zero_png);
        lv_obj_set_width(ui_TOP_logo, LV_SIZE_CONTENT);   /// 49
        lv_obj_set_height(ui_TOP_logo, LV_SIZE_CONTENT);    /// 12
        lv_obj_set_x(ui_TOP_logo, 5);
        lv_obj_set_y(ui_TOP_logo, 5);
        lv_obj_add_flag(ui_TOP_logo, LV_OBJ_FLAG_ADV_HITTEST);     /// Flags
        lv_obj_clear_flag(ui_TOP_logo, LV_OBJ_FLAG_SCROLLABLE);      /// Flags

        ui_TOP_time = lv_obj_create(ui_root);
        lv_obj_set_width(ui_TOP_time, 45);
        lv_obj_set_height(ui_TOP_time, 16);
        lv_obj_set_x(ui_TOP_time, 237);
        lv_obj_set_y(ui_TOP_time, 5);
        lv_obj_clear_flag(ui_TOP_time, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
        lv_obj_set_style_radius(ui_TOP_time, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(ui_TOP_time, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(ui_TOP_time, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_img_src(ui_TOP_time, ui_img_time_png, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(ui_TOP_time, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        ui_TOP_time_Label = lv_label_create(ui_TOP_time);
        lv_obj_set_width(ui_TOP_time_Label, LV_SIZE_CONTENT);   /// 1
        lv_obj_set_height(ui_TOP_time_Label, LV_SIZE_CONTENT);    /// 1
        lv_obj_set_align(ui_TOP_time_Label, LV_ALIGN_CENTER);
        lv_label_set_text(ui_TOP_time_Label, "15:21");
        lv_obj_set_style_text_color(ui_TOP_time_Label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(ui_TOP_time_Label, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

        ui_TOP_Power = lv_bar_create(ui_root);
        lv_bar_set_value(ui_TOP_Power, 96, LV_ANIM_OFF);
        lv_bar_set_start_value(ui_TOP_Power, 0, LV_ANIM_OFF);
        lv_obj_set_width(ui_TOP_Power, 29);
        lv_obj_set_height(ui_TOP_Power, 13);
        lv_obj_set_x(ui_TOP_Power, 286);
        lv_obj_set_y(ui_TOP_Power, 5);
        lv_obj_set_style_radius(ui_TOP_Power, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(ui_TOP_Power, lv_color_hex(0x484847), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(ui_TOP_Power, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_set_style_radius(ui_TOP_Power, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(ui_TOP_Power, lv_color_hex(0x666633), LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(ui_TOP_Power, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

        ui_TOP_power_Label = lv_label_create(ui_TOP_Power);
        lv_obj_set_width(ui_TOP_power_Label, LV_SIZE_CONTENT);   /// 1
        lv_obj_set_height(ui_TOP_power_Label, LV_SIZE_CONTENT);    /// 1
        lv_obj_set_align(ui_TOP_power_Label, LV_ALIGN_CENTER);
        lv_label_set_text(ui_TOP_power_Label, "96%");
        lv_obj_set_style_text_color(ui_TOP_power_Label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(ui_TOP_power_Label, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

        ui_APP_Container = lv_obj_create(ui_root);
        lv_obj_remove_style_all(ui_APP_Container);
        lv_obj_set_width(ui_APP_Container, 320);
        lv_obj_set_height(ui_APP_Container, 150);
        lv_obj_set_x(ui_APP_Container, 0);
        lv_obj_set_y(ui_APP_Container, 10);
        lv_obj_set_align(ui_APP_Container, LV_ALIGN_CENTER);
        lv_obj_clear_flag(ui_APP_Container, (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));      /// Flags
    }

    void creat_input_group()
    {
        key_group = lv_group_create();
    }

    void UI_bind_event()
    {
    }
};


class app_base
{
private:
    lv_obj_t *ui_TOP_logo;
    lv_obj_t *ui_TOP_time;
    lv_obj_t *ui_TOP_time_Label;
    lv_obj_t *ui_TOP_SignalStrength;
    lv_obj_t *ui_TOP_SignalStrength_one;
    lv_obj_t *ui_TOP_SignalStrength_two;
    lv_obj_t *ui_TOP_SignalStrength_three;
    lv_obj_t *ui_TOP_SignalStrength_four;
    lv_obj_t *ui_TOP_Power;
    lv_obj_t *ui_TOP_power_Label;

public:
    lv_group_t *key_group;
    lv_obj_t *ui_APP_Container;
    lv_obj_t *ui_root;
    lv_obj_t *get_ui() { return ui_root; }
    lv_group_t *get_key_group() { return key_group; }
    std::function<void(void)> go_back_home;

public:
    app_base()
    {
        creat_base_UI();
        creat_input_group();
        UI_bind_event();
    }
    ~app_base()
    {
        lv_obj_del(ui_root);
        // lv_obj_del_async(ui_root);
    }

private:
    /* ================================================================== */
    /*  UI 构建                                                             */
    /* ================================================================== */
    void creat_base_UI()
    {
        ui_root = lv_obj_create(NULL);
        lv_obj_clear_flag(ui_root, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
        lv_obj_set_style_bg_color(ui_root, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(ui_root, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

        ui_TOP_logo = lv_img_create(ui_root);
        lv_img_set_src(ui_TOP_logo, ui_img_zero_logo_w_png);
        lv_obj_set_width(ui_TOP_logo, LV_SIZE_CONTENT);   /// 58
        lv_obj_set_height(ui_TOP_logo, LV_SIZE_CONTENT);    /// 12
        lv_obj_set_x(ui_TOP_logo, 5);
        lv_obj_set_y(ui_TOP_logo, 5);
        lv_obj_add_flag(ui_TOP_logo, LV_OBJ_FLAG_ADV_HITTEST);     /// Flags
        lv_obj_clear_flag(ui_TOP_logo, LV_OBJ_FLAG_SCROLLABLE);      /// Flags

        ui_TOP_time = lv_obj_create(ui_root);
        lv_obj_set_width(ui_TOP_time, 40);
        lv_obj_set_height(ui_TOP_time, 13);
        lv_obj_set_x(ui_TOP_time, 206);
        lv_obj_set_y(ui_TOP_time, 3);
        lv_obj_clear_flag(ui_TOP_time, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
        lv_obj_set_style_radius(ui_TOP_time, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(ui_TOP_time, lv_color_hex(0x333333), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(ui_TOP_time, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(ui_TOP_time, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        ui_TOP_time_Label = lv_label_create(ui_TOP_time);
        lv_obj_set_width(ui_TOP_time_Label, LV_SIZE_CONTENT);   /// 1
        lv_obj_set_height(ui_TOP_time_Label, LV_SIZE_CONTENT);    /// 1
        lv_obj_set_align(ui_TOP_time_Label, LV_ALIGN_CENTER);
        lv_label_set_text(ui_TOP_time_Label, "19:45");
        lv_obj_set_style_text_color(ui_TOP_time_Label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(ui_TOP_time_Label, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(ui_TOP_time_Label, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);

        ui_TOP_SignalStrength = lv_obj_create(ui_root);
        lv_obj_set_width(ui_TOP_SignalStrength, 30);
        lv_obj_set_height(ui_TOP_SignalStrength, 13);
        lv_obj_set_x(ui_TOP_SignalStrength, 248);
        lv_obj_set_y(ui_TOP_SignalStrength, 3);
        lv_obj_clear_flag(ui_TOP_SignalStrength, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
        lv_obj_set_style_radius(ui_TOP_SignalStrength, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(ui_TOP_SignalStrength, lv_color_hex(0x333333), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(ui_TOP_SignalStrength, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(ui_TOP_SignalStrength, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        ui_TOP_SignalStrength_one = lv_obj_create(ui_TOP_SignalStrength);
        lv_obj_set_width(ui_TOP_SignalStrength_one, 5);
        lv_obj_set_height(ui_TOP_SignalStrength_one, 3);
        lv_obj_set_x(ui_TOP_SignalStrength_one, -11);
        lv_obj_set_y(ui_TOP_SignalStrength_one, 2);
        lv_obj_set_align(ui_TOP_SignalStrength_one, LV_ALIGN_CENTER);
        lv_obj_clear_flag(ui_TOP_SignalStrength_one, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
        lv_obj_set_style_radius(ui_TOP_SignalStrength_one, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(ui_TOP_SignalStrength_one, lv_color_hex(0x00CCFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(ui_TOP_SignalStrength_one, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(ui_TOP_SignalStrength_one, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        ui_TOP_SignalStrength_two = lv_obj_create(ui_TOP_SignalStrength);
        lv_obj_set_width(ui_TOP_SignalStrength_two, 5);
        lv_obj_set_height(ui_TOP_SignalStrength_two, 6);
        lv_obj_set_x(ui_TOP_SignalStrength_two, -4);
        lv_obj_set_y(ui_TOP_SignalStrength_two, 1);
        lv_obj_set_align(ui_TOP_SignalStrength_two, LV_ALIGN_CENTER);
        lv_obj_clear_flag(ui_TOP_SignalStrength_two, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
        lv_obj_set_style_radius(ui_TOP_SignalStrength_two, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(ui_TOP_SignalStrength_two, lv_color_hex(0x00CCFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(ui_TOP_SignalStrength_two, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(ui_TOP_SignalStrength_two, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        ui_TOP_SignalStrength_three = lv_obj_create(ui_TOP_SignalStrength);
        lv_obj_set_width(ui_TOP_SignalStrength_three, 5);
        lv_obj_set_height(ui_TOP_SignalStrength_three, 7);
        lv_obj_set_x(ui_TOP_SignalStrength_three, 3);
        lv_obj_set_y(ui_TOP_SignalStrength_three, 0);
        lv_obj_set_align(ui_TOP_SignalStrength_three, LV_ALIGN_CENTER);
        lv_obj_clear_flag(ui_TOP_SignalStrength_three, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
        lv_obj_set_style_radius(ui_TOP_SignalStrength_three, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(ui_TOP_SignalStrength_three, lv_color_hex(0x00CCFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(ui_TOP_SignalStrength_three, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(ui_TOP_SignalStrength_three, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        ui_TOP_SignalStrength_four = lv_obj_create(ui_TOP_SignalStrength);
        lv_obj_set_width(ui_TOP_SignalStrength_four, 5);
        lv_obj_set_height(ui_TOP_SignalStrength_four, 9);
        lv_obj_set_x(ui_TOP_SignalStrength_four, 10);
        lv_obj_set_y(ui_TOP_SignalStrength_four, -1);
        lv_obj_set_align(ui_TOP_SignalStrength_four, LV_ALIGN_CENTER);
        lv_obj_clear_flag(ui_TOP_SignalStrength_four, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
        lv_obj_set_style_radius(ui_TOP_SignalStrength_four, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(ui_TOP_SignalStrength_four, lv_color_hex(0x4D4D4D), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(ui_TOP_SignalStrength_four, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(ui_TOP_SignalStrength_four, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        ui_TOP_Power = lv_obj_create(ui_root);
        lv_obj_set_width(ui_TOP_Power, 38);
        lv_obj_set_height(ui_TOP_Power, 13);
        lv_obj_set_x(ui_TOP_Power, 280);
        lv_obj_set_y(ui_TOP_Power, 3);
        lv_obj_clear_flag(ui_TOP_Power, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
        lv_obj_set_style_radius(ui_TOP_Power, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(ui_TOP_Power, lv_color_hex(0x333333), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(ui_TOP_Power, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(ui_TOP_Power, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        ui_TOP_power_Label = lv_label_create(ui_TOP_Power);
        lv_obj_set_width(ui_TOP_power_Label, LV_SIZE_CONTENT);   /// 1
        lv_obj_set_height(ui_TOP_power_Label, LV_SIZE_CONTENT);    /// 1
        lv_obj_set_align(ui_TOP_power_Label, LV_ALIGN_CENTER);
        lv_label_set_text(ui_TOP_power_Label, "100%");
        lv_obj_set_style_text_color(ui_TOP_power_Label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(ui_TOP_power_Label, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(ui_TOP_power_Label, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);

        ui_APP_Container = lv_obj_create(ui_root);
        lv_obj_remove_style_all(ui_APP_Container);
        lv_obj_set_width(ui_APP_Container, 320);
        lv_obj_set_height(ui_APP_Container, 150);
        lv_obj_set_x(ui_APP_Container, 0);
        lv_obj_set_y(ui_APP_Container, 10);
        lv_obj_set_align(ui_APP_Container, LV_ALIGN_CENTER);
        lv_obj_clear_flag(ui_APP_Container, (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));      /// Flags
    }

    void creat_input_group()
    {
        key_group = lv_group_create();
        lv_group_add_obj(key_group, ui_root);
    }

    // static void static_event_handler(lv_event_t * e)
    // {
    //     app_base *instance = static_cast<app_base *>(lv_event_get_user_data(e));
    //     if (instance)
    //     {
    //         instance->event_handler(e);
    //     }
    // }

    // virtual void event_handler(lv_event_t * e)
    // {

    // }

    void UI_bind_event()
    {
    }
};
