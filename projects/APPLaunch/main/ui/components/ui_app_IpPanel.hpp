#pragma once
#include "ui_app_page.hpp"
#include <unordered_map>
#include <string>
#include <vector>
#include <functional>
#include <algorithm>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <net/if.h>

// ============================================================
//  IP面板界面  UIIpPanelPage
//  屏幕分辨率: 320 x 170  (顶栏20px, ui_APP_Container 320x150)
//
//  视图状态:
//    VIEW_MAIN    — 列表（每秒自动刷新，将网口信息显示在列表中）
// ============================================================

class UIIpPanelPage : public app_base
{
    // ==================== 单条网口信息 ====================
    struct NetIfInfo
    {
        std::string iface;   // 网口名称，如 eth0 / wlan0
        std::string ip;      // IPv4 地址，无地址时为 "N/A"
        std::string mask;    // 子网掩码
        bool        up;      // 是否 UP
    };

public:
    UIIpPanelPage() : app_base()
    {
        creat_UI();
        event_handler_init();
    }
    ~UIIpPanelPage() {}

private:
    // ==================== 数据成员 ====================
    std::unordered_map<std::string, lv_obj_t *> ui_obj_;
    std::vector<NetIfInfo> iface_list_;   // 当前网口列表
    int selected_idx_ = 0;               // 当前高亮行

    // 布局常量（内容区 320x150，标题栏22px，剩余 128px）
    static constexpr int ITEM_H       = 32;   // 每行高度
    static constexpr int VISIBLE_ROWS = 4;    // 可见行数（128/32 = 4）
    static constexpr int TITLE_H      = 22;   // 标题栏高度
    static constexpr int LIST_H       = 128;  // 列表区域高度

    // 定时器句柄
    lv_timer_t *refresh_timer_ = nullptr;

    // ==================== 读取系统网口信息 ====================
    void fetch_iface_list()
    {
        iface_list_.clear();

        struct ifaddrs *ifas = nullptr;
        if (getifaddrs(&ifas) != 0)
            return;

        // 先收集所有有 IPv4 地址的接口
        std::unordered_map<std::string, NetIfInfo> map;
        for (struct ifaddrs *ifa = ifas; ifa; ifa = ifa->ifa_next)
        {
            if (!ifa->ifa_name) continue;
            std::string name(ifa->ifa_name);

            // 跳过 loopback
            if (name == "lo") continue;

            if (map.find(name) == map.end())
            {
                NetIfInfo info;
                info.iface = name;
                info.ip    = "N/A";
                info.mask  = "N/A";
                info.up    = (ifa->ifa_flags & IFF_UP) != 0;
                map[name]  = info;
            }

            // 仅处理 IPv4
            if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET)
            {
                char buf[INET_ADDRSTRLEN];
                struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
                inet_ntop(AF_INET, &sa->sin_addr, buf, sizeof(buf));
                map[name].ip = buf;

                if (ifa->ifa_netmask)
                {
                    struct sockaddr_in *sm = (struct sockaddr_in *)ifa->ifa_netmask;
                    inet_ntop(AF_INET, &sm->sin_addr, buf, sizeof(buf));
                    map[name].mask = buf;
                }
                map[name].up = (ifa->ifa_flags & IFF_UP) != 0;
            }
        }
        freeifaddrs(ifas);

        for (auto &kv : map)
            iface_list_.push_back(kv.second);

        // 按接口名排序，保证顺序稳定
        std::sort(iface_list_.begin(), iface_list_.end(),
                  [](const NetIfInfo &a, const NetIfInfo &b) {
                      return a.iface < b.iface;
                  });

        // 选中索引越界保护
        if (selected_idx_ >= (int)iface_list_.size())
            selected_idx_ = iface_list_.empty() ? 0 : (int)iface_list_.size() - 1;
    }

    // ==================== UI 构建 ====================
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
        lv_obj_set_size(title_bar, 320, TITLE_H);
        lv_obj_set_pos(title_bar, 0, 0);
        lv_obj_set_style_radius(title_bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(title_bar, lv_color_hex(0x1F3A5F), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(title_bar, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(title_bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(title_bar, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

        // 标题文字
        lv_obj_t *lbl_title = lv_label_create(title_bar);
        lv_label_set_text(lbl_title, LV_SYMBOL_WIFI "  IP Panel");
        lv_obj_set_align(lbl_title, LV_ALIGN_LEFT_MID);
        lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

        // 右侧操作提示
        lv_obj_t *lbl_hint = lv_label_create(title_bar);
        lv_label_set_text(lbl_hint, "UP/DN:select  ESC:back");
        lv_obj_set_align(lbl_hint, LV_ALIGN_RIGHT_MID);
        lv_obj_set_x(lbl_hint, -4);
        lv_obj_set_style_text_color(lbl_hint, lv_color_hex(0x7EA8D8), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lbl_hint, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);

        // ---- 列表容器 ----
        lv_obj_t *list_cont = lv_obj_create(bg);
        lv_obj_set_size(list_cont, 320, LIST_H);
        lv_obj_set_pos(list_cont, 0, TITLE_H);
        lv_obj_set_style_radius(list_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(list_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(list_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(list_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(list_cont, LV_OBJ_FLAG_SCROLLABLE);
        ui_obj_["list_cont"] = list_cont;

        // 首次刷新并渲染
        fetch_iface_list();
        build_list_rows();

        // ---- 每秒自动刷新定时器 ----
        refresh_timer_ = lv_timer_create(UIIpPanelPage::static_timer_cb, 1000, this);
    }

    // ==================== 构建列表行 ====================
    void build_list_rows()
    {
        lv_obj_t *list_cont = ui_obj_["list_cont"];
        lv_obj_clean(list_cont);

        if (iface_list_.empty())
        {
            // 无网口时显示提示
            lv_obj_t *lbl = lv_label_create(list_cont);
            lv_label_set_text(lbl, "No network interface found.");
            lv_obj_set_pos(lbl, 10, 50);
            lv_obj_set_style_text_color(lbl, lv_color_hex(0x555555), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
            return;
        }

        int item_count = (int)iface_list_.size();
        int visible    = LIST_H / ITEM_H;

        // 计算滚动偏移使选中项尽量居中
        int offset = selected_idx_ - visible / 2;
        if (offset < 0) offset = 0;
        if (offset > item_count - visible) offset = item_count - visible;
        if (offset < 0) offset = 0;

        for (int vi = 0; vi < visible && (vi + offset) < item_count; ++vi)
        {
            int  mi  = vi + offset;
            bool sel = (mi == selected_idx_);
            create_row(list_cont, vi, mi, sel);
        }
    }

    // ==================== 创建单行 ====================
    void create_row(lv_obj_t *parent, int visual_row, int idx, bool selected)
    {
        const NetIfInfo &info = iface_list_[idx];

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
            // 左侧高亮竖条
            lv_obj_t *sel_bar = lv_obj_create(row);
            lv_obj_set_size(sel_bar, 3, ITEM_H - 8);
            lv_obj_set_pos(sel_bar, 2, 3);
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

        // 分割线（非最后可见行）
        if (idx < (int)iface_list_.size() - 1)
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

        // 状态圆点（绿=UP，灰=DOWN）
        lv_obj_t *dot = lv_obj_create(row);
        lv_obj_set_size(dot, 6, 6);
        lv_obj_set_pos(dot, 8, (ITEM_H - 2 - 6) / 2);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(dot,
            info.up ? lv_color_hex(0x2ECC71) : lv_color_hex(0x444444),
            LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(dot, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(dot, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);

        // 接口名（左侧）
        lv_obj_t *lbl_iface = lv_label_create(row);
        lv_label_set_text(lbl_iface, info.iface.c_str());
        lv_obj_set_pos(lbl_iface, 20, 2);
        lv_obj_set_width(lbl_iface, 72);
        lv_label_set_long_mode(lbl_iface, LV_LABEL_LONG_CLIP);
        lv_obj_set_style_text_color(lbl_iface,
            selected ? lv_color_hex(0x58A6FF) : lv_color_hex(0x4A7ABF),
            LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lbl_iface, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);

        // IP 地址（中间）
        lv_obj_t *lbl_ip = lv_label_create(row);
        lv_label_set_text(lbl_ip, info.ip.c_str());
        lv_obj_set_pos(lbl_ip, 96, 2);
        lv_obj_set_width(lbl_ip, 130);
        lv_label_set_long_mode(lbl_ip, LV_LABEL_LONG_CLIP);
        lv_obj_set_style_text_color(lbl_ip,
            selected ? lv_color_hex(0xFFFFFF) : lv_color_hex(0xCCCCCC),
            LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lbl_ip, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);

        // 子网掩码（右侧，小字）
        lv_obj_t *lbl_mask = lv_label_create(row);
        lv_label_set_text(lbl_mask, info.mask.c_str());
        lv_obj_set_pos(lbl_mask, 96, 17);
        lv_obj_set_width(lbl_mask, 130);
        lv_label_set_long_mode(lbl_mask, LV_LABEL_LONG_CLIP);
        lv_obj_set_style_text_color(lbl_mask,
            lv_color_hex(0x555555),
            LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lbl_mask, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);

        // UP/DOWN 状态文字（右侧）
        lv_obj_t *lbl_state = lv_label_create(row);
        lv_label_set_text(lbl_state, info.up ? "UP" : "DOWN");
        lv_obj_set_pos(lbl_state, 266, 2);
        lv_obj_set_style_text_color(lbl_state,
            info.up ? lv_color_hex(0x2ECC71) : lv_color_hex(0x555555),
            LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lbl_state, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    // ==================== 定时刷新回调 ====================
    static void static_timer_cb(lv_timer_t *timer)
    {
        UIIpPanelPage *self = static_cast<UIIpPanelPage *>(lv_timer_get_user_data(timer));
        if (self) self->on_timer_refresh();
    }
    void on_timer_refresh()
    {
        fetch_iface_list();
        build_list_rows();
    }

    // ==================== 事件绑定 ====================
    void event_handler_init()
    {
        lv_obj_add_event_cb(ui_root, UIIpPanelPage::static_lvgl_handler, LV_EVENT_ALL, this);
    }
    static void static_lvgl_handler(lv_event_t *e)
    {
        UIIpPanelPage *self = static_cast<UIIpPanelPage *>(lv_event_get_user_data(e));
        if (self) self->event_handler(e);
    }
    void event_handler(lv_event_t *e)
    {
        if (lv_event_get_code(e) != LV_EVENT_KEY) return;
        uint32_t key = lv_event_get_key(e);

        int count = (int)iface_list_.size();
        switch (key)
        {
        case LV_KEY_UP:
            if (selected_idx_ > 0)
            {
                --selected_idx_;
                build_list_rows();
            }
            break;

        case LV_KEY_DOWN:
            if (selected_idx_ < count - 1)
            {
                ++selected_idx_;
                build_list_rows();
            }
            break;

        case LV_KEY_ESC:
            // 销毁定时器后返回主页
            if (refresh_timer_)
            {
                lv_timer_del(refresh_timer_);
                refresh_timer_ = nullptr;
            }
            if (go_back_home) go_back_home();
            break;

        default:
            break;
        }
    }
};
