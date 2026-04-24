#include "ui.h"
#include <stdio.h>
#include <string.h>
#if defined(__linux__)
#include <linux/input.h>
#else
#ifndef KEY_UP
#define KEY_UP 103
#endif
#ifndef KEY_DOWN
#define KEY_DOWN 108
#endif
#ifndef KEY_LEFT
#define KEY_LEFT 105
#endif
#ifndef KEY_RIGHT
#define KEY_RIGHT 106
#endif
#ifndef KEY_ENTER
#define KEY_ENTER 28
#endif
#ifndef KEY_ESC
#define KEY_ESC 1
#endif
#ifndef KEY_Z
#define KEY_Z 44
#endif
#ifndef KEY_C
#define KEY_C 46
#endif
#endif







typedef void (*switch_cb_t)(lv_event_t *);
struct lv_timer_data_t
{
    switch_cb_t cb_fun;
	lv_timer_t **snap_timer;
};


#define ROTATE_LEFT(arr, start, end) do {                       \
    typeof((arr)[0]) _tmp = (arr)[(start)];                     \
    memmove(&(arr)[(start)], &(arr)[(start) + 1],              \
            ((end) - (start)) * sizeof((arr)[0]));              \
    (arr)[(end)] = _tmp;                                        \
} while (0)

#define ROTATE_RIGHT(arr, start, end) do {                      \
    typeof((arr)[0]) _tmp = (arr)[(end)];                       \
    memmove(&(arr)[(start) + 1], &(arr)[(start)],              \
            ((end) - (start)) * sizeof((arr)[0]));              \
    (arr)[(start)] = _tmp;                                      \
} while (0)





lv_obj_t *launch_circle[100];

// ==================== 5个槽位的标准坐标 ====================
static const lv_coord_t SLOT_X[] = {-177, -99,   0,  99, 177,  -177,  -99,  0,   99,  177  };
static const lv_coord_t SLOT_Y[] = {   4,  -6, -16,  -6,   4,    57,   57, 50,   57,   57  };
static const lv_coord_t SLOT_W[] = {  61,  81, 101,  81,  61                               };
static const lv_coord_t SLOT_H[] = {  61,  81, 101,  81,  61                               };

static bool is_animating = false;

static int Panel_current_pos = 2; // 当前位置

static int switch_current_pos = 11; // 当前位置

// ==================== 初始化 ====================
void launch_circle_init()
{
    launch_circle[0] = ui_outPanelzuo;   // pos0 隐藏
    launch_circle[1] = ui_zuoPanel;      // pos1 左
    launch_circle[2] = ui_switchPanel;   // pos2 中心
    launch_circle[3] = ui_youPanel;      // pos3 右
    launch_circle[4] = ui_outPanelyou;   // pos4 隐藏


	launch_circle[5] = ui_zuoLabelout;   // pos0 隐藏
    launch_circle[6] = ui_zuoLabel;      // pos1 左
    launch_circle[7] = ui_switchLabel;   // pos2 中心
    launch_circle[8] = ui_youLabel;      // pos3 右
    launch_circle[9] = ui_youLabelout;   // pos4 隐藏


    launch_circle[10] = ui_Panel4;   // pos4 隐藏
    launch_circle[11] = ui_Panel3;   // pos4 隐藏
    launch_circle[12] = ui_Panel5;   // pos4 隐藏
    launch_circle[13] = ui_Panel6;   // pos4 隐藏
    launch_circle[14] = ui_Panel7;   // pos4 隐藏
    launch_circle[15] = ui_Panel8;   // pos4 隐藏
    launch_circle[16] = ui_Panel9;   // pos4 隐藏
    launch_circle[17] = ui_Panel10;   // pos4 隐藏

}


static void switchpanleEnable(int obj_index, int enable) {
    lv_obj_t *obj = launch_circle[obj_index];
	if(enable){
		lv_obj_set_width(obj, 10);
		lv_obj_set_height(obj, 10);
		lv_obj_set_align(obj, LV_ALIGN_CENTER);
		lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
		lv_obj_set_style_bg_color(obj, lv_color_hex(0xCCCC33), LV_PART_MAIN | LV_STATE_DEFAULT);
		lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
		lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0x4A4C4A), LV_PART_MAIN | LV_STATE_DEFAULT);
		lv_obj_set_style_border_color(obj, lv_color_hex(0xCCCC33), LV_PART_MAIN | LV_STATE_DEFAULT);
		lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
	}else
	{
		lv_obj_set_width(obj, 5);
		lv_obj_set_height(obj, 5);
		lv_obj_set_align(obj, LV_ALIGN_CENTER);
		lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
		lv_obj_set_style_bg_color(obj, lv_color_hex(0x4A4C4A), LV_PART_MAIN | LV_STATE_DEFAULT);
		lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
		lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0x4A4C4A), LV_PART_MAIN | LV_STATE_DEFAULT);
		lv_obj_set_style_border_color(obj, lv_color_hex(0x4A4C4A), LV_PART_MAIN | LV_STATE_DEFAULT);
		lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
	}
}

static void switchpanleEnableClick(int obj_index, int enable) {
    lv_obj_t *obj = launch_circle[obj_index];
	if(enable){
		lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
	}else
	{
		lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
	}
}

// ==================== 将面板强制设定到指定槽位 ====================
static void snap_panel_to_slot(lv_obj_t *panel, int slot)
{
    lv_obj_set_x(panel, SLOT_X[slot]);
    lv_obj_set_y(panel, SLOT_Y[slot]);
    lv_obj_set_width(panel, SLOT_W[slot]);
    lv_obj_set_height(panel, SLOT_H[slot]);

    if (slot == 0 || slot == 4) {
        lv_obj_add_flag(panel, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(panel, LV_OBJ_FLAG_HIDDEN);
    }
}

// ==================== 将标签强制设定到指定槽位 ====================
static void snap_label_to_slot(lv_obj_t *label, int slot)
{
    lv_obj_set_x(label, SLOT_X[slot]);
    lv_obj_set_y(label, SLOT_Y[slot]);

    if (slot == 5 || slot == 9) {
        lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(label, LV_OBJ_FLAG_HIDDEN);
    }
}

// ==================== 动画结束后校正所有面板位置 ====================
static void snap_all_panels(lv_anim_t *a)
{
	printf("snap_all_panels\r\n");
    for (int i = 0; i < 5; i++) {
        snap_panel_to_slot(launch_circle[i], i);
    }
    for (int i = 5; i < 10; i++) {
        snap_label_to_slot(launch_circle[i], i);
    }
    is_animating = false;
}


static void snap_timer_cb(lv_timer_t *timer)
{
	struct lv_timer_data_t *data = (struct lv_timer_data_t *)lv_timer_get_user_data(timer);
    switch_cb_t cb_fun = data->cb_fun;
	lv_timer_t **snap_timer = data->snap_timer;
	lv_mem_free(data);
	*snap_timer = NULL;
	cb_fun(NULL);
    // lv_timer_set_repeat_count 设为1会自动删除
}

static void delay_cb_fun(lv_timer_t **snap_timer, switch_cb_t cb)
{
    if (*snap_timer) {
        return ;
    }
	struct lv_timer_data_t *data = lv_mem_alloc(sizeof(struct lv_timer_data_t));
	data->cb_fun = cb;
	data->snap_timer = snap_timer;
    // 动画时长500ms，550ms后校正位置
    *snap_timer = lv_timer_create(snap_timer_cb, 50, data);
    lv_timer_set_repeat_count(*snap_timer, 1);
}

// ==================== 向右切换（点右箭头） ====================
void switchyou(lv_event_t *e)
{
	static lv_timer_t *snap_timer = NULL;

    // 动画进行中，先强制完成上一轮
    if (is_animating) {
        // snap_all_panels(NULL);
		delay_cb_fun(&snap_timer, &switchyou);
		return ;
    }

    is_animating = true;

    // 1. 显示 pos0 处的面板（它即将滑入视野）
    lv_obj_clear_flag(launch_circle[0], LV_OBJ_FLAG_HIDDEN);

    // 2. 四个面板同时向右移一个槽位，最后一个动画绑定完成回调
    zuopanelout2you_Animation(launch_circle[0], 0, NULL);   // pos0 → pos1
    zuopanel2you_Animation(launch_circle[1], 0, NULL);       // pos1 → pos2
    switchpanel2you_Animation(launch_circle[2], 0, NULL);    // pos2 → pos3
    youpanel2you_Animation(launch_circle[3], 0, snap_all_panels);       // pos3 → pos4

    // 3. 将 pos4 的面板瞬移到 pos0（循环）并隐藏
    snap_panel_to_slot(launch_circle[4], 0);
    

    // 4. 显示 pos5(label pos0) 处的标签（它即将滑入视野）
    lv_obj_clear_flag(launch_circle[5], LV_OBJ_FLAG_HIDDEN);

    // 5. 四个标签同时向右移一个槽位（与面板方向相同）
    zuolabelout2you_Animation(launch_circle[5], 0, NULL);    // label pos0 → pos1
    zuolabel2you_Animation(launch_circle[6], 0, NULL);       // label pos1 → pos2
    switchlabel2you_Animation(launch_circle[7], 0, NULL);    // label pos2 → pos3
    youlabel2you_Animation(launch_circle[8], 0, NULL);       // label pos3 → pos4

    // 6. 将 pos9(label pos4) 的标签瞬移到 pos5(label pos0)（循环）并隐藏
    snap_label_to_slot(launch_circle[9], 5);
    cpp_app_you(launch_circle[4], launch_circle[9]);

    switchpanleEnableClick(2, 0);
	ROTATE_RIGHT(launch_circle, 0, 4);
    switchpanleEnableClick(2, 1);
	ROTATE_RIGHT(launch_circle, 5, 9);

    
    switchpanleEnable(switch_current_pos, 0);
    switch_current_pos = switch_current_pos == 17 ? 10 : switch_current_pos + 1;
    switchpanleEnable(switch_current_pos, 1);


}

// ==================== 向左切换（点左箭头） ====================
void switchzuo(lv_event_t *e)
{
	static lv_timer_t *snap_timer = NULL;
    if (is_animating) {
        // snap_all_panels(NULL);
		delay_cb_fun(&snap_timer, &switchzuo);
		return ;
    }

    printf("switchzuo\r\n");
    is_animating = true;

    // 1. 显示 pos4 处的面板（它即将滑入视野）
    lv_obj_clear_flag(launch_circle[4], LV_OBJ_FLAG_HIDDEN);

    // 2. 四个面板同时向左移一个槽位，最后一个动画绑定完成回调
    zuopanelout2zuo_Animation(launch_circle[4], 0, NULL);    // pos4 → pos3
    youpanel2zuo_Animation(launch_circle[3], 0, NULL);       // pos3 → pos2
    switchpanel2zuo_Animation(launch_circle[2], 0, NULL);    // pos2 → pos1
    zuopanel2zuo_Animation(launch_circle[1], 0, snap_all_panels);       // pos1 → pos0

    // 3. 将 pos0 的面板瞬移到 pos4（循环）并隐藏
    snap_panel_to_slot(launch_circle[0], 4);

    // 4. 显示 pos9(label pos4) 处的标签（它即将滑入视野）
    lv_obj_clear_flag(launch_circle[9], LV_OBJ_FLAG_HIDDEN);

    // 5. 四个标签同时向左移一个槽位（与面板方向相同）
    zuolabelout2zuo_Animation(launch_circle[9], 0, NULL);    // label pos4 → pos3
    youlabel2zuo_Animation(launch_circle[8], 0, NULL);       // label pos3 → pos2
    switchlabel2zuo_Animation(launch_circle[7], 0, NULL);    // label pos2 → pos1
    zuolabel2zuo_Animation(launch_circle[6], 0, NULL);       // label pos1 → pos0

    // 6. 将 pos5(label pos0) 的标签瞬移到 pos9(label pos4)（循环）并隐藏
    snap_label_to_slot(launch_circle[5], 9);
    cpp_app_zuo(launch_circle[0], launch_circle[5]);

    switchpanleEnableClick(2, 0);
	ROTATE_LEFT(launch_circle, 0, 4);
    switchpanleEnableClick(2, 1);

	ROTATE_LEFT(launch_circle, 5, 9);

    switchpanleEnable(switch_current_pos, 0);
    switch_current_pos = switch_current_pos == 10 ? 17 : switch_current_pos - 1;
    switchpanleEnable(switch_current_pos, 1);

    
}

void go_back_home(lv_event_t * e)
{
	lv_disp_load_scr(ui_Screen1);
    lv_indev_set_group(lv_indev_get_next(NULL), Screen1group);
}






static bool launcher_key_event_ready(lv_event_t *e)
{
    const lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_KEY) {
        return true;
    }
    if (code == LV_EVENT_KEYBOARD) {
        return LV_EVENT_KEYBOARD_GET_KEY_STATE(e) == 0;
    }
    return false;
}

static uint32_t launcher_get_key(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_KEY) {
        return lv_event_get_key(e);
    }
    return LV_EVENT_KEYBOARD_GET_KEY(e);
}

#define UI_DEFINE_UI_EVENT_FUN(event_fun, call_fun) void event_fun(lv_event_t * e) { \
    if(launcher_key_event_ready(e)) { \
        call_fun(e); \
    } \
}

UI_DEFINE_UI_EVENT_FUN(ui_event_Screen1, main_key_switch)

#undef UI_DEFINE_UI_EVENT_FUN

void app_launch(lv_event_t * e)
{
    cpp_app_launch();
}

void main_key_switch(lv_event_t * e)
{
    uint32_t key = launcher_get_key(e);
    printf("按下: %d\r\n", key);
    switch(key) {
        case KEY_UP:
        case LV_KEY_UP:
            break;
        case KEY_DOWN:
        case LV_KEY_DOWN:
            break;
        case KEY_LEFT:
        case LV_KEY_LEFT:
        case KEY_Z:
            switchyou(NULL);
            break;
        case KEY_RIGHT:
        case LV_KEY_RIGHT:
        case KEY_C:
            switchzuo(NULL);
            break;
        case KEY_ENTER:
        case LV_KEY_ENTER:
            printf("按下: ENTER\r\n");
            app_launch(NULL);
            break;
        case KEY_ESC:
        case LV_KEY_ESC:
            break;
        default:
            break;
    }
}




