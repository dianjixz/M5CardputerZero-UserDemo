#pragma once
#include "ui_app_page.hpp"
#include <unordered_map>
#include <string>
#include <vector>
#include <iostream>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <libgen.h>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <cstdlib>

// ============================================================
//  音乐播放器  UIMusicPage
//  屏幕分辨率: 320 x 170  (顶栏20px, ui_APP_Container 320x150)
//
//  视图状态:
//    VIEW_MAIN       — 主播放界面
//    VIEW_FOLDER_SEL — 'i'(Tab) 弹出的目录浏览器
//    VIEW_PLAYLIST   — 'p' 弹出的播放列表
//
//  按键映射 (经 main.cpp _evdev_process_key 映射后):
//    LV_KEY_UP    → 播放（主界面） / 列表上移
//    LV_KEY_DOWN  → 暂停（主界面） / 列表下移
//    LV_KEY_LEFT  → 上一首（主界面） / 返回上级目录（目录浏览器）
//    LV_KEY_RIGHT → 下一首（主界面） / 进入子目录（目录浏览器）
//    'i'(Tab)     → 弹出目录浏览器（从 / 开始）
//    'p'          → 弹出播放列表
//    LV_KEY_ENTER → 目录浏览器：确认当前目录载入mp3 / 播放列表：播放选中曲目
//    LV_KEY_ESC   → 返回主界面 / 退出App
// ============================================================
class UIMusicPage : public app_base
{
    enum class PlayState { STOPPED, PLAYING, PAUSED };
    enum class ViewState { MAIN, FOLDER_SEL, PLAYLIST };
public:
    UIMusicPage() : app_base()
    {
        creat_UI();
        event_handler_init();
    }
    ~UIMusicPage()
    {
        stop_playback();
    }
    // ==================== 对外接口 ====================
    void prev_track()
    {
        if (playlist_.empty()) return;
        if (current_track_ > 0)
            --current_track_;
        else
            current_track_ = (int)playlist_.size() - 1;
        if (play_state_ == PlayState::PLAYING)
            start_playback();
        update_main_ui();
    }
    void next_track()
    {
        if (playlist_.empty()) return;
        current_track_ = (current_track_ + 1) % (int)playlist_.size();
        if (play_state_ == PlayState::PLAYING)
            start_playback();
        update_main_ui();
    }
    void play()
    {
        if (playlist_.empty()) return;
        start_playback();
        play_state_ = PlayState::PLAYING;
        update_main_ui();
    }
    void pause()
    {
        if (play_state_ != PlayState::PLAYING) return;
        stop_playback();
        play_state_ = PlayState::PAUSED;
        update_main_ui();
    }
private:
    // ==================== 数据成员 ====================
    std::unordered_map<std::string, lv_obj_t *> ui_obj_;
    std::string              browse_dir_;
    std::vector<std::string> browse_entries_;
    std::string              music_dir_;
    std::vector<std::string> playlist_;
    int                      current_track_ = 0;
    PlayState play_state_ = PlayState::STOPPED;
    ViewState view_state_ = ViewState::MAIN;
    pid_t player_pid_ = -1;

    // ==================== POSIX 路径工具 ====================
    // 父目录（基于 POSIX dirname(3)）
    static std::string path_parent(const std::string &path)
    {
        if (path.empty()) return "/";
        // dirname 会修改输入 buffer, 需要 copy
        std::vector<char> buf(path.begin(), path.end());
        buf.push_back('\0');
        char *p = dirname(buf.data());
        return std::string(p ? p : "/");
    }
    // 文件名（基于 POSIX basename(3)）
    static std::string path_basename(const std::string &path)
    {
        if (path.empty()) return "";
        std::vector<char> buf(path.begin(), path.end());
        buf.push_back('\0');
        char *p = basename(buf.data());
        return std::string(p ? p : "");
    }
    // 路径拼接（自动避免重复 '/'）
    static std::string path_join(const std::string &base, const std::string &name)
    {
        if (base.empty()) return name;
        if (name.empty()) return base;
        if (base == "/") return "/" + name;
        if (!base.empty() && base.back() == '/') return base + name;
        return base + "/" + name;
    }

    // ==================== UI 构建（主界面） ====================
    void creat_UI()
    {
        lv_obj_t *bg = lv_obj_create(ui_APP_Container);
        lv_obj_set_size(bg, 320, 150);
        lv_obj_set_pos(bg, 0, 0);
        lv_obj_set_style_radius(bg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(bg, lv_color_hex(0x0D1117), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(bg, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(bg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(bg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(bg, LV_OBJ_FLAG_SCROLLABLE);
        ui_obj_["ui_bg"] = bg;

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
        lv_label_set_text(lbl_title, "Music Player");
        lv_obj_set_align(lbl_title, LV_ALIGN_LEFT_MID);
        lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t *lbl_hint = lv_label_create(title_bar);
        lv_label_set_text(lbl_hint, "i:Folder  p:List  ESC:Back");
        lv_obj_set_align(lbl_hint, LV_ALIGN_RIGHT_MID);
        lv_obj_set_x(lbl_hint, -4);
        lv_obj_set_style_text_color(lbl_hint, lv_color_hex(0x7EA8D8), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lbl_hint, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t *content = lv_obj_create(bg);
        lv_obj_set_size(content, 320, 128);
        lv_obj_set_pos(content, 0, 22);
        lv_obj_set_style_radius(content, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(content, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(content, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(content, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
        ui_obj_["ui_content"] = content;

        lv_obj_t *cover = lv_obj_create(content);
        lv_obj_set_size(cover, 96, 96);
        lv_obj_set_pos(cover, 8, 16);
        lv_obj_set_style_radius(cover, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(cover, lv_color_hex(0x1A2A4A), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(cover, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(cover, lv_color_hex(0x3A5A8A), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(cover, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(cover, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_t *lbl_note = lv_label_create(cover);
        lv_label_set_text(lbl_note, "MUSIC");
        lv_obj_set_align(lbl_note, LV_ALIGN_CENTER);
        lv_obj_set_style_text_color(lbl_note, lv_color_hex(0x4A7ABF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lbl_note, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t *lbl_track = lv_label_create(content);
        lv_label_set_text(lbl_track, "No track");
        lv_obj_set_pos(lbl_track, 114, 8);
        lv_obj_set_width(lbl_track, 198);
        lv_label_set_long_mode(lbl_track, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_style_text_color(lbl_track, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lbl_track, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
        ui_obj_["ui_lbl_track"] = lbl_track;

        lv_obj_t *lbl_count = lv_label_create(content);
        lv_label_set_text(lbl_count, "0 / 0");
        lv_obj_set_pos(lbl_count, 114, 30);
        lv_obj_set_style_text_color(lbl_count, lv_color_hex(0x8AABCF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lbl_count, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
        ui_obj_["ui_lbl_count"] = lbl_count;

        lv_obj_t *lbl_dir = lv_label_create(content);
        lv_label_set_text(lbl_dir, "Dir: (none)");
        lv_obj_set_pos(lbl_dir, 114, 48);
        lv_obj_set_width(lbl_dir, 198);
        lv_label_set_long_mode(lbl_dir, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_style_text_color(lbl_dir, lv_color_hex(0x6A8FAF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lbl_dir, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
        ui_obj_["ui_lbl_dir"] = lbl_dir;

        lv_obj_t *lbl_state = lv_label_create(content);
        lv_label_set_text(lbl_state, "[STOPPED]");
        lv_obj_set_pos(lbl_state, 114, 65);
        lv_obj_set_style_text_color(lbl_state, lv_color_hex(0xFFD700), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lbl_state, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
        ui_obj_["ui_lbl_state"] = lbl_state;

        lv_obj_t *lbl_keys = lv_label_create(content);
        lv_label_set_text(lbl_keys, "UP:Play  DOWN:Pause  LEFT/RIGHT:Prev/Next");
        lv_obj_set_pos(lbl_keys, 4, 112);
        lv_obj_set_width(lbl_keys, 312);
        lv_label_set_long_mode(lbl_keys, LV_LABEL_LONG_CLIP);
        lv_obj_set_style_text_color(lbl_keys, lv_color_hex(0x4A5A6A), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lbl_keys, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    // ==================== 事件绑定 ====================
    void event_handler_init()
    {
        lv_obj_add_event_cb(ui_root, UIMusicPage::static_lvgl_handler, LV_EVENT_ALL, this);
    }
    static void static_lvgl_handler(lv_event_t *e)
    {
        UIMusicPage *self = static_cast<UIMusicPage *>(lv_event_get_user_data(e));
        if (self) self->event_handler(e);
    }
    void event_handler(lv_event_t *e)
    {
        if (lv_event_get_code(e) != LV_EVENT_KEY) return;
        uint32_t key = lv_event_get_key(e);
        printf("[Music] key:%u  view:%d\n", key, (int)view_state_);
        switch (view_state_)
        {
        case ViewState::MAIN:       handle_main_key(key);     break;
        case ViewState::FOLDER_SEL: handle_folder_key(key);   break;
        case ViewState::PLAYLIST:   handle_playlist_key(key); break;
        }
    }

    // ================================================================
    //  主界面按键
    // ================================================================
    void handle_main_key(uint32_t key)
    {
        switch (key)
        {
        case LV_KEY_UP:    play();                break;
        case LV_KEY_DOWN:  pause();               break;
        case LV_KEY_LEFT:  prev_track();          break;
        case LV_KEY_RIGHT: next_track();          break;
        case 15:           open_folder_browser(); break; // (Tab)
        case 'p':          open_playlist();       break;
        case LV_KEY_ESC:   go_back_home();        break;
        default: break;
        }
    }

    // ================================================================
    //  目录浏览器按键
    // ================================================================
    void handle_folder_key(uint32_t key)
    {
        lv_obj_t *roller = ui_obj_.count("ui_folder_roller") ? ui_obj_["ui_folder_roller"] : nullptr;
        if (!roller) return;
        switch (key)
        {
        case LV_KEY_UP:
        {
            uint16_t sel = lv_roller_get_selected(roller);
            if (sel > 0)
                lv_roller_set_selected(roller, sel - 1, LV_ANIM_ON);
            break;
        }
        case LV_KEY_DOWN:
        {
            // 使用自维护的 entries 数量，避免依赖 lv_roller_get_option_cnt
            uint16_t sel = lv_roller_get_selected(roller);
            uint16_t cnt = (uint16_t)browse_entries_.size();
            if (cnt > 0 && sel + 1 < cnt)
                lv_roller_set_selected(roller, sel + 1, LV_ANIM_ON);
            break;
        }
        case LV_KEY_RIGHT:
        {
            // RIGHT 仅进入真实子目录，不处理 ".." (返回上级用 LEFT)
            uint16_t sel = lv_roller_get_selected(roller);
            if (sel < (uint16_t)browse_entries_.size())
            {
                const std::string &entry = browse_entries_[sel];
                if (entry != "..")
                {
                    std::string target = path_join(browse_dir_, entry);
                    struct stat st;
                    if (stat(target.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
                        navigate_to(target);
                }
            }
            break;
        }
        case LV_KEY_LEFT:
        {
            // LEFT 返回上级目录
            navigate_to(path_parent(browse_dir_));
            break;
        }
        case LV_KEY_ENTER:
        {
            // ENTER: 若当前高亮的是真实子目录则进入，否则从 browse_dir_ 载入音乐
            uint16_t sel = lv_roller_get_selected(roller);
            bool entered = false;
            if (sel < (uint16_t)browse_entries_.size())
            {
                const std::string &entry = browse_entries_[sel];
                if (entry != "..")
                {
                    std::string target = path_join(browse_dir_, entry);
                    struct stat st;
                    if (stat(target.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
                    {
                        navigate_to(target);
                        entered = true;
                    }
                }
            }
            if (!entered)
            {
                load_music_from_folder(browse_dir_);
                close_folder_browser();
            }
            break;
        }
        case LV_KEY_ESC:
            close_folder_browser();
            break;
        default:
            break;
        }
    }

    // ================================================================
    //  播放列表按键
    // ================================================================
    void handle_playlist_key(uint32_t key)
    {
        lv_obj_t *roller = ui_obj_.count("ui_playlist_roller") ? ui_obj_["ui_playlist_roller"] : nullptr;
        switch (key)
        {
        case LV_KEY_UP:
            if (roller)
            {
                uint16_t sel = lv_roller_get_selected(roller);
                if (sel > 0)
                    lv_roller_set_selected(roller, sel - 1, LV_ANIM_ON);
            }
            break;
        case LV_KEY_DOWN:
            if (roller)
            {
                // 使用 playlist_ 实际大小
                uint16_t sel = lv_roller_get_selected(roller);
                uint16_t cnt = (uint16_t)playlist_.size();
                if (cnt > 0 && sel + 1 < cnt)
                    lv_roller_set_selected(roller, sel + 1, LV_ANIM_ON);
            }
            break;
        case LV_KEY_ENTER:
            if (roller && !playlist_.empty())
            {
                uint16_t sel = lv_roller_get_selected(roller);
                if (sel < (uint16_t)playlist_.size())
                {
                    current_track_ = (int)sel;
                    start_playback();
                    play_state_ = PlayState::PLAYING;
                }
            }
            close_playlist();
            update_main_ui();
            break;
        case 'p':
        case LV_KEY_ESC:
            close_playlist();
            break;
        default:
            break;
        }
    }

    // ================================================================
    //  目录浏览器 — 打开 / 导航 / 关闭
    // ================================================================
    void open_folder_browser()
    {
        view_state_ = ViewState::FOLDER_SEL;
        browse_dir_ = "/";

        lv_obj_t *panel = lv_obj_create(ui_APP_Container);
        lv_obj_set_size(panel, 316, 148);
        lv_obj_set_pos(panel, 2, 1);
        lv_obj_set_style_radius(panel, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(panel, lv_color_hex(0x0D1B2A), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(panel, 250, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(panel, lv_color_hex(0x1F6FEB), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(panel, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
        ui_obj_["ui_folder_panel"] = panel;

        lv_obj_t *lbl_path = lv_label_create(panel);
        lv_label_set_text(lbl_path, browse_dir_.c_str());
        lv_obj_set_pos(lbl_path, 4, 3);
        lv_obj_set_width(lbl_path, 308);
        lv_label_set_long_mode(lbl_path, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_style_text_color(lbl_path, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lbl_path, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
        ui_obj_["ui_folder_path_lbl"] = lbl_path;

        lv_obj_t *lbl_h = lv_label_create(panel);
        lv_label_set_text(lbl_h, "UP/DN:sel  RIGHT/ENTER:enter dir  LEFT:up  ESC:cancel");
        lv_obj_set_pos(lbl_h, 2, 132);
        lv_obj_set_width(lbl_h, 312);
        lv_label_set_long_mode(lbl_h, LV_LABEL_LONG_CLIP);
        lv_obj_set_style_text_color(lbl_h, lv_color_hex(0x3A5A7A), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lbl_h, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t *roller = lv_roller_create(panel);
        lv_obj_set_size(roller, 308, 114);
        lv_obj_set_pos(roller, 4, 16);
        lv_obj_set_style_radius(roller, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(roller, lv_color_hex(0x0D1B2A), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(roller, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(roller, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(roller, lv_color_hex(0xCCDDEE), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(roller, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(roller, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(roller, lv_color_hex(0x1F3A5F), LV_PART_SELECTED | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(roller, 220, LV_PART_SELECTED | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(roller, lv_color_hex(0xFFFFFF), LV_PART_SELECTED | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(roller, &lv_font_montserrat_12, LV_PART_SELECTED | LV_STATE_DEFAULT);
        ui_obj_["ui_folder_roller"] = roller;

        refresh_folder_roller();
    }

    // 导航到指定目录
    void navigate_to(const std::string &dir)
    {
        browse_dir_ = dir.empty() ? std::string("/") : dir;
        if (ui_obj_.count("ui_folder_path_lbl") && ui_obj_["ui_folder_path_lbl"])
            lv_label_set_text(ui_obj_["ui_folder_path_lbl"], browse_dir_.c_str());
        refresh_folder_roller();
    }

    // 读取 browse_dir_ 下的子目录，填充 roller
    void refresh_folder_roller()
    {
        browse_entries_.clear();

        // 非根目录时添加 ".." 返回项
        if (browse_dir_ != "/")
            browse_entries_.push_back("..");

        DIR *dp = opendir(browse_dir_.c_str());
        if (dp)
        {
            struct dirent *ent;
            while ((ent = readdir(dp)) != nullptr)
            {
                // 跳过 "." ".." 和隐藏项
                if (ent->d_name[0] == '.') continue;

                std::string full = path_join(browse_dir_, ent->d_name);
                struct stat st;
                if (stat(full.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
                    browse_entries_.push_back(ent->d_name);
            }
            closedir(dp);
        }
        else
        {
            printf("[Music] opendir failed: %s\n", browse_dir_.c_str());
        }

        // 排序（".." 保持在首位）
        if (!browse_entries_.empty() && browse_entries_[0] == "..")
            std::sort(browse_entries_.begin() + 1, browse_entries_.end());
        else
            std::sort(browse_entries_.begin(), browse_entries_.end());

        // 构建 roller 选项字符串
        std::string options;
        if (browse_entries_.empty())
        {
            // 保证 roller 非空 —— 但同时保持 browse_entries_ 为空，避免误进入
            options = "(empty)";
        }
        else
        {
            for (size_t i = 0; i < browse_entries_.size(); ++i)
            {
                const std::string &e = browse_entries_[i];
                if (i) options += '\n';
                if (e == "..")
                    options += "../";
                else
                    options += e + "/";
            }
        }

        lv_obj_t *roller = ui_obj_.count("ui_folder_roller") ? ui_obj_["ui_folder_roller"] : nullptr;
        if (roller)
        {
            lv_roller_set_options(roller, options.c_str(), LV_ROLLER_MODE_NORMAL);
            // 适当可视行数（让最后一项也能被滚到中心选中）
            lv_roller_set_visible_row_count(roller, 5);
            lv_roller_set_selected(roller, 0, LV_ANIM_OFF);
        }
    }

    void close_folder_browser()
    {
        if (ui_obj_.count("ui_folder_panel") && ui_obj_["ui_folder_panel"])
        {
            lv_obj_del(ui_obj_["ui_folder_panel"]);
            ui_obj_["ui_folder_panel"]    = nullptr;
            ui_obj_["ui_folder_roller"]   = nullptr;
            ui_obj_["ui_folder_path_lbl"] = nullptr;
        }
        view_state_ = ViewState::MAIN;
    }

    // ================================================================
    //  播放列表弹窗
    // ================================================================
    void open_playlist()
    {
        if (playlist_.empty()) return;
        view_state_ = ViewState::PLAYLIST;

        lv_obj_t *panel = lv_obj_create(ui_APP_Container);
        lv_obj_set_size(panel, 316, 148);
        lv_obj_set_pos(panel, 2, 1);
        lv_obj_set_style_radius(panel, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(panel, lv_color_hex(0x0D1B2A), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(panel, 250, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(panel, lv_color_hex(0x00AA66), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(panel, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
        ui_obj_["ui_playlist_panel"] = panel;

        lv_obj_t *lbl_t = lv_label_create(panel);
        char title_buf[48];
        snprintf(title_buf, sizeof(title_buf), "Playlist  %d tracks", (int)playlist_.size());
        lv_label_set_text(lbl_t, title_buf);
        lv_obj_set_pos(lbl_t, 6, 3);
        lv_obj_set_style_text_color(lbl_t, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lbl_t, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t *lbl_h = lv_label_create(panel);
        lv_label_set_text(lbl_h, "UP/DOWN: select   ENTER: play   p/ESC: cancel");
        lv_obj_set_pos(lbl_h, 2, 132);
        lv_obj_set_style_text_color(lbl_h, lv_color_hex(0x2A6A4A), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lbl_h, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);

        // 文件名列表（使用 POSIX basename）
        std::string options;
        for (size_t i = 0; i < playlist_.size(); ++i)
        {
            if (i) options += '\n';
            options += path_basename(playlist_[i]);
        }

        lv_obj_t *roller = lv_roller_create(panel);
        lv_roller_set_options(roller, options.c_str(), LV_ROLLER_MODE_NORMAL);
        lv_roller_set_visible_row_count(roller, 5);
        uint16_t init_sel = 0;
        if (current_track_ >= 0 && current_track_ < (int)playlist_.size())
            init_sel = (uint16_t)current_track_;
        lv_roller_set_selected(roller, init_sel, LV_ANIM_OFF);
        lv_obj_set_size(roller, 308, 114);
        lv_obj_set_pos(roller, 4, 16);
        lv_obj_set_style_radius(roller, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(roller, lv_color_hex(0x0D1B2A), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(roller, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(roller, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(roller, lv_color_hex(0xCCDDCC), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(roller, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(roller, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(roller, lv_color_hex(0x1A4A2A), LV_PART_SELECTED | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(roller, 220, LV_PART_SELECTED | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(roller, lv_color_hex(0xFFFFFF), LV_PART_SELECTED | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(roller, &lv_font_montserrat_12, LV_PART_SELECTED | LV_STATE_DEFAULT);
        ui_obj_["ui_playlist_roller"] = roller;
    }

    void close_playlist()
    {
        if (ui_obj_.count("ui_playlist_panel") && ui_obj_["ui_playlist_panel"])
        {
            lv_obj_del(ui_obj_["ui_playlist_panel"]);
            ui_obj_["ui_playlist_panel"]  = nullptr;
            ui_obj_["ui_playlist_roller"] = nullptr;
        }
        view_state_ = ViewState::MAIN;
    }

    // ================================================================
    //  载入指定目录中的 mp3 文件到播放列表
    // ================================================================
    void load_music_from_folder(const std::string &dir)
    {
        playlist_.clear();
        music_dir_ = dir;

        DIR *dp = opendir(dir.c_str());
        if (!dp)
        {
            printf("[Music] Cannot open dir: %s\n", dir.c_str());
            update_main_ui();
            return;
        }

        struct dirent *ent;
        while ((ent = readdir(dp)) != nullptr)
        {
            std::string fname = ent->d_name;
            if (fname.size() < 4) continue;
            if (fname[0] == '.') continue;

            // 必须是常规文件
            std::string full = path_join(dir, fname);
            struct stat st;
            if (stat(full.c_str(), &st) != 0) continue;
            if (!S_ISREG(st.st_mode)) continue;

            std::string ext = fname.substr(fname.size() - 4);
            for (auto &c : ext) c = (char)tolower((unsigned char)c);
            if (ext == ".mp3")
                playlist_.push_back(full);
        }
        closedir(dp);

        std::sort(playlist_.begin(), playlist_.end());
        current_track_ = 0;
        play_state_    = PlayState::STOPPED;
        stop_playback();

        printf("[Music] Loaded %d mp3 from %s\n", (int)playlist_.size(), dir.c_str());
        update_main_ui();
    }

    // ================================================================
    //  播放控制（mpg123 / ffplay 子进程）
    // ================================================================
    void start_playback()
    {
        stop_playback();
        if (playlist_.empty()) return;
        if (current_track_ < 0 || current_track_ >= (int)playlist_.size()) return;

        const std::string &file = playlist_[current_track_];
        printf("[Music] Playing: %s\n", file.c_str());

        pid_t pid = fork();
        if (pid == 0)
        {
            execlp("mpg123", "mpg123", "-q", file.c_str(), (char *)nullptr);
            execlp("ffplay", "ffplay", "-nodisp", "-autoexit", "-loglevel", "quiet", file.c_str(), (char *)nullptr);
            _exit(EXIT_FAILURE);
        }
        else if (pid > 0)
        {
            player_pid_ = pid;
        }
        else
        {
            perror("[Music] fork failed");
        }
    }

    void stop_playback()
    {
        if (player_pid_ > 0)
        {
            kill(player_pid_, SIGTERM);
            waitpid(player_pid_, nullptr, WNOHANG);
            player_pid_ = -1;
        }
    }

    // ================================================================
    //  刷新主界面标签
    // ================================================================
    void update_main_ui()
    {
        if (!playlist_.empty() && current_track_ < (int)playlist_.size())
        {
            std::string fname = path_basename(playlist_[current_track_]);
            lv_label_set_text(ui_obj_["ui_lbl_track"], fname.c_str());
        }
        else
        {
            lv_label_set_text(ui_obj_["ui_lbl_track"], "No track");
        }

        char buf[32];
        snprintf(buf, sizeof(buf), "%d / %d",
                 playlist_.empty() ? 0 : current_track_ + 1,
                 (int)playlist_.size());
        lv_label_set_text(ui_obj_["ui_lbl_count"], buf);

        std::string dir_show = "Dir: " + (music_dir_.empty() ? std::string("(none)") : music_dir_);
        lv_label_set_text(ui_obj_["ui_lbl_dir"], dir_show.c_str());

        const char *state_str = "[STOPPED]";
        if (play_state_ == PlayState::PLAYING)      state_str = "[ PLAYING ]";
        else if (play_state_ == PlayState::PAUSED)  state_str = "[ PAUSED  ]";
        lv_label_set_text(ui_obj_["ui_lbl_state"], state_str);
    }
};