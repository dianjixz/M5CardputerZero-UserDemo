#include "../ui.h"
#include <list>
#include <iostream>
#include <memory>
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

/* ============================================================
 * 卡片数据结构
 * ============================================================ */
typedef struct {
    const char  *icon;          /* LVGL内置图标 */
    const char  *title;         /* 标题 */
    const char  *value;         /* 主数值 */
    const char  *subtitle;      /* 副标题 */
    lv_color_t   accent_color;  /* 强调色 */
    int32_t      bar_value;     /* 进度条值 0-100, -1不显示 */
} card_data_t;

/* ============================================================
 * 卡片对象结构体
 * ============================================================ */
struct app_card
{
    lv_obj_t *card;
    lv_obj_t *accent_bar;
    lv_obj_t *icon_bg;
    lv_obj_t *icon;
    lv_obj_t *title;
    lv_obj_t *value;
    lv_obj_t *subtitle;
    lv_obj_t *line;
};

/* ============================================================
 * 卡片数据配置
 * ============================================================ */
static const card_data_t card_list[] = {
    {
        .icon         = LV_SYMBOL_WARNING,
        .title        = "测试",
        .value        = "1000",
        .subtitle     = "GAME",
        .accent_color = {0},
        .bar_value    = -1
    },
    {
        .icon         = LV_SYMBOL_TINT,
        .title        = "你好",
        .value        = "100",
        .subtitle     = "GAME",
        .accent_color = {0},
        .bar_value    = -1
    },
    {
        .icon         = LV_SYMBOL_CHARGE,
        .title        = "Battery",
        .value        = "200",
        .subtitle     = "TOOL",
        .accent_color = {0},
        .bar_value    = -1
    },
    {
        .icon         = LV_SYMBOL_HOME,
        .title        = "Devices",
        .value        = "100",
        .subtitle     = "FILE",
        .accent_color = {0},
        .bar_value    = -1
    },
    {
        .icon         = LV_SYMBOL_WIFI,
        .title        = "Network",
        .value        = "10",
        .subtitle     = "SETTING",
        .accent_color = {0},
        .bar_value    = -1
    },
    {
        .icon         = LV_SYMBOL_BELL,
        .title        = "Notifications",
        .value        = "3 New",
        .subtitle     = "Unread Messages",
        .accent_color = {0},
        .bar_value    = -1
    },
};


void app_card_selected(lv_obj_t *card, bool selected)
{
    if (selected) {
        lv_obj_set_style_outline_color(card, COLOR_SELECTED_OUTLINE, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_outline_opa  (card, LV_OPA_COVER,           LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_outline_width(card, 2,                       LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_outline_pad  (card, 2,                       LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
        /* 只需将 opa 设为透明 + width 设为 0，颜色无实际影响 */
        lv_obj_set_style_outline_color(card, lv_color_black(),        LV_PART_MAIN | LV_STATE_DEFAULT); // ✅ 颜色任意
        lv_obj_set_style_outline_opa  (card, LV_OPA_TRANSP,           LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_outline_width(card, 0,                       LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_outline_pad  (card, 0,                       LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

/* ============================================================
 * 创建单个卡片
 * ============================================================ */
 static std::unique_ptr<struct app_card> create_card(lv_obj_t *parent,
                              const card_data_t *data,
                              lv_color_t accent,
                              bool selected)        /* ← 新增 selected 参数 */
{
    std::unique_ptr<struct app_card> card = std::make_unique<struct app_card>();

    /* --- 卡片主体 --- */
    card->card = lv_obj_create(parent);
    lv_obj_set_size(card->card, 130, 100);
    lv_obj_set_style_bg_color(card->card, COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(card->card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card->card, 16, 0);
    lv_obj_set_style_border_width(card->card, 0, 0);
    lv_obj_set_style_pad_all(card->card, 8, 0);
    lv_obj_add_event_cb(card->card, ui_event_app_card, LV_EVENT_ALL, NULL);

    /* 卡片阴影 */
    lv_obj_set_style_shadow_color(card->card, lv_color_hex(0xC0CCE0), 0);
    lv_obj_set_style_shadow_width(card->card, 12, 0);
    lv_obj_set_style_shadow_ofs_x(card->card, 2, 0);
    lv_obj_set_style_shadow_ofs_y(card->card, 4, 0);
    lv_obj_set_style_shadow_opa(card->card, LV_OPA_50, 0);

    /* ============================================================
     * 选中状态：轮廓描边
     * LV_PART_MAIN | LV_STATE_DEFAULT 直接写入 main style
     * ============================================================ */
    app_card_selected(card->card, selected);

    /* 卡片左侧顶部强调色竖条 */
    card->accent_bar = lv_obj_create(card->card);
    lv_obj_set_size(card->accent_bar, 4, 32);
    lv_obj_set_style_bg_color(card->accent_bar, accent, 0);
    lv_obj_set_style_bg_opa(card->accent_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card->accent_bar, 2, 0);
    lv_obj_set_style_border_width(card->accent_bar, 0, 0);
    lv_obj_set_style_pad_all(card->accent_bar, 0, 0);
    lv_obj_align(card->accent_bar, LV_ALIGN_TOP_LEFT, 0, 0);

    /* --- 图标圆形背景 --- */
    card->icon_bg = lv_obj_create(card->card);
    lv_obj_set_size(card->icon_bg, 32, 32);
    lv_obj_set_style_bg_color(card->icon_bg, accent, 0);
    lv_obj_set_style_bg_opa(card->icon_bg, LV_OPA_20, 0);
    lv_obj_set_style_radius(card->icon_bg, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(card->icon_bg, 0, 0);
    lv_obj_set_style_pad_all(card->icon_bg, 0, 0);
    lv_obj_align(card->icon_bg, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_clear_flag(card->icon_bg, LV_OBJ_FLAG_CLICKABLE);

    /* 图标 */
    card->icon = lv_label_create(card->icon_bg);
    lv_label_set_text(card->icon, data->icon);
    lv_obj_set_style_text_color(card->icon, accent, 0);
    lv_obj_set_style_text_font(card->icon, &lv_font_montserrat_16, 0);
    lv_obj_center(card->icon);
    lv_obj_clear_flag(card->icon, LV_OBJ_FLAG_CLICKABLE);

    /* --- 标题 --- */
    card->title = lv_label_create(card->card);
    lv_label_set_text(card->title, data->title);
    lv_obj_set_style_text_color(card->title, COLOR_SUBTITLE, 0);
    lv_obj_set_style_text_font(card->title, g_font_cn_14, 0);
    lv_obj_align(card->title, LV_ALIGN_TOP_LEFT, 8, 0);

    /* --- 主数值 --- */
    card->value = lv_label_create(card->card);
    lv_label_set_text(card->value, data->value);
    lv_obj_set_style_text_color(card->value, COLOR_VALUE, 0);
    lv_obj_set_style_text_font(card->value, &lv_font_montserrat_20, 0);
    lv_obj_align(card->value, LV_ALIGN_TOP_LEFT, 4, 36);

    /* --- 副标题 --- */
    card->subtitle = lv_label_create(card->card);
    lv_label_set_text(card->subtitle, data->subtitle);
    lv_obj_set_style_text_color(card->subtitle, COLOR_SUBTITLE, 0);
    lv_obj_set_style_text_font(card->subtitle, g_font_cn_14, 0);
    lv_obj_align(card->subtitle, LV_ALIGN_TOP_LEFT, 4, 58);

    /* --- 分割线 --- */
    card->line = lv_obj_create(card->card);
    lv_obj_set_size(card->line, lv_pct(100), 1);
    lv_obj_set_style_bg_color(card->line, lv_color_hex(0xE2E8F0), 0);
    lv_obj_set_style_bg_opa(card->line, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(card->line, 0, 0);
    lv_obj_set_style_pad_all(card->line, 0, 0);
    lv_obj_align(card->line, LV_ALIGN_TOP_LEFT, 0, 78);

    return card;
}

/* ============================================================
 * 主界面创建函数
 * ============================================================ */
std::list<std::unique_ptr<struct app_card>> card_objects;  /* 保存卡片对象指针的列表 */
lv_obj_t *selected_card = nullptr;
int selected_index = 0;  /* 当前选中卡片的索引 */
const int CARDS_PER_ROW = 2;  /* 每行显示2个卡片 */
void create_dashboard(lv_obj_t *scr)
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

    uint32_t count = sizeof(card_list) / sizeof(card_list[0]);
    for (uint32_t i = 0; i < count; i++) {
        bool selected = (i == 0);   /* ← 仅第一张卡片为选中态 */
        auto card = create_card(container, &card_list[i], colors[i], selected);
        /* 把卡片添加到AppStore输入组 */
        lv_group_add_obj(AppStoregroup, card->card);
        card_objects.push_back(std::move(card));
    }
    selected_card = card_objects.front()->card;
    selected_index = 0;
}

void app_card_click(lv_event_t * e)
{
    lv_obj_t *tmp_obj = selected_card;

    lv_obj_t *obj  = (lv_obj_t *)lv_event_get_target(e);
    int index = 0;
    for(auto& card : card_objects) {
        if (card->card == obj) {
            if(card->card == tmp_obj) {
                // todo: 处理卡片点击事件
                // std::cout<< "卡片被点击: " << lv_label_get_text(card->title) << std::endl;
                lv_disp_load_scr(ui_APPNote);
                lv_indev_set_group(lv_indev_get_next(NULL), APPNotegroup);
            } else {
                selected_card = card->card;
                selected_index = index;
                app_card_selected(card->card, true);
                // 滚动container使选中的卡片在Y轴中间
                lv_obj_scroll_to_view(card->card, LV_ANIM_ON | LV_SCROLL_SNAP_CENTER);
            }
        } else if(tmp_obj == card->card) {
            app_card_selected(card->card, false);
        }
        index++;
    }

}

/* 处理应用商店键盘事件 */
void app_store_key_handler(uint32_t key)
{
    int total_cards = card_objects.size();
    if (total_cards == 0) return;

    int new_index = selected_index;

    switch(key) {
        case LV_KEY_LEFT:
            /* 左移：如果不是行首，左移一个 */
            if (selected_index % CARDS_PER_ROW > 0) {
                new_index = selected_index - 1;
            }
            break;

        case LV_KEY_RIGHT:
            /* 右移：如果不是行尾，右移一个 */
            if (selected_index % CARDS_PER_ROW < CARDS_PER_ROW - 1 && selected_index + 1 < total_cards) {
                new_index = selected_index + 1;
            }
            break;

        case LV_KEY_UP:
            /* 上移：切换到上一行相同位置 */
            if (selected_index >= CARDS_PER_ROW) {
                new_index = selected_index - CARDS_PER_ROW;
            }
            break;

        case LV_KEY_DOWN:
            /* 下移：切换到下一行相同位置 */
            if (selected_index + CARDS_PER_ROW < total_cards) {
                new_index = selected_index + CARDS_PER_ROW;
            }
            break;

        case LV_KEY_ENTER:
            /* 回车键：模拟点击当前选中卡片 */
            if (selected_card) {
                lv_obj_send_event(selected_card, LV_EVENT_CLICKED, NULL);
            }
            return;

        default:
            return;
    }

    /* 如果索引变化，更新选中状态 */
    if (new_index != selected_index) {
        /* 取消旧的选中状态 */
        app_card_selected(selected_card, false);

        /* 找到新的卡片 */
        int i = 0;
        for(auto& card : card_objects) {
            if (i == new_index) {
                selected_card = card->card;
                selected_index = new_index;
                app_card_selected(card->card, true);
                /* 滚动到视图中 */
                lv_obj_scroll_to_view(card->card, LV_ANIM_ON | LV_SCROLL_SNAP_CENTER);
                break;
            }
            i++;
        }
    }
}