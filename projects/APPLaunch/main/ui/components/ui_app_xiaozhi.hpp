#pragma once

#include "ui_app_page.hpp"
#include "application.hpp"
#include "audio_pipeline.hpp"
#if defined(APPLAUNCH_USE_SDL_AUDIO)
#include "audio_pipeline_sdl.hpp"
#endif
#include "config.hpp"
#include "hal.hpp"
#include "mac_text_renderer.hpp"
#include "types.hpp"
#include "ws_client.hpp"

#include "lvgl/src/misc/cache/instance/lv_image_cache.h"

#include <cstdio>
#include <cctype>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

LV_FONT_DECLARE(lv_font_source_han_sans_sc_14_cjk)
LV_FONT_DECLARE(lv_font_source_han_sans_sc_16_cjk)

class UIXiaoZhiPage : public app_base
{
public:
    UIXiaoZhiPage() : app_base()
    {
        creat_UI();
        event_handler_init();
        launch_xiaozhi();
    }

private:
    class AppLaunchHal final : public xiaozhi::Hal
    {
    public:
        bool init() override { return true; }
        void poll() override {}
        void onButtonPressed(std::function<void()> cb) override { press_cb_ = std::move(cb); }
        void onButtonReleased(std::function<void()> cb) override { release_cb_ = std::move(cb); }

        void emitPress()
        {
            if (press_cb_) {
                press_cb_();
            }
        }

        void emitRelease()
        {
            if (release_cb_) {
                release_cb_();
            }
        }

    private:
        std::function<void()> press_cb_;
        std::function<void()> release_cb_;
    };

    class AppLaunchUi final : public xiaozhi::Ui
    {
    public:
        explicit AppLaunchUi(UIXiaoZhiPage *owner) : owner_(owner) {}

        bool init() override
        {
            if (owner_ != nullptr) {
                owner_->apply_state("正在启动小智", "正在初始化内嵌会话...", "binding");
            }
            return true;
        }

        void renderState(xiaozhi::AppState state, const std::string& text, const std::string& emoji) override
        {
            if (owner_ != nullptr) {
                owner_->apply_state(
                    owner_->state_name(state),
                    owner_->localize_detail(state, text),
                    owner_->resolve_server_emoji(emoji, state));
            }
        }

    private:
        UIXiaoZhiPage *owner_ = nullptr;
    };

    std::unordered_map<std::string, lv_obj_t *> ui_obj_;
    std::unique_ptr<xiaozhi::Application> application_;
    AppLaunchHal *hal_ = nullptr;
    lv_timer_t *app_timer_ = nullptr;
    std::string status_ = "正在启动小智";
    std::string detail_ = "等待内嵌运行时启动...";
    std::string emoji_ = "🔗";

#if defined(__APPLE__)
    struct RenderedTextSlot {
        lv_obj_t *obj = nullptr;
        std::string disk_path;
        std::string file_path;
        std::string last_text;
        std::vector<std::string> generated_disk_paths;
        uint32_t version = 0;
        int width = 0;
        int height = 0;
        MacTextRenderStyle style{};
    };

    RenderedTextSlot title_slot_;
    RenderedTextSlot emoji_slot_;
    RenderedTextSlot status_slot_;
    RenderedTextSlot detail_slot_;
    RenderedTextSlot hint_slot_;
#endif

    static const lv_font_t *font14()
    {
        return g_font_cn_14 ? g_font_cn_14 : &lv_font_source_han_sans_sc_14_cjk;
    }

    static const lv_font_t *font12()
    {
        return g_font_cn_12 ? g_font_cn_12 : &lv_font_source_han_sans_sc_14_cjk;
    }

    static const lv_font_t *font16()
    {
        return g_font_cn_20 ? g_font_cn_20 : &lv_font_source_han_sans_sc_16_cjk;
    }

#if defined(__APPLE__)
    std::string render_cache_basename(const char *slot_name, uint32_t version) const
    {
        return "applaunch_xiaozhi_" + std::to_string(reinterpret_cast<uintptr_t>(this)) + "_" + slot_name + "_" + std::to_string(version) + ".png";
    }

    std::string render_disk_cache_path(const char *slot_name, uint32_t version) const
    {
        return "dist/images/" + render_cache_basename(slot_name, version);
    }

    std::string render_lvgl_cache_path(const char *slot_name, uint32_t version) const
    {
        return "A:/dist/images/" + render_cache_basename(slot_name, version);
    }

    void cleanup_rendered_assets(RenderedTextSlot &slot)
    {
        for (const std::string &path : slot.generated_disk_paths) {
            if (!path.empty()) {
                std::remove(path.c_str());
            }
        }
        slot.generated_disk_paths.clear();
        slot.disk_path.clear();
        slot.file_path.clear();
        slot.last_text.clear();
    }

    void update_rendered_text(RenderedTextSlot &slot, const char *slot_name, const std::string &text)
    {
        if (slot.obj == nullptr) {
            return;
        }

        if (text.empty()) {
            slot.last_text.clear();
            lv_obj_add_flag(slot.obj, LV_OBJ_FLAG_HIDDEN);
            return;
        }

        if (slot.last_text == text && !slot.file_path.empty()) {
            lv_obj_clear_flag(slot.obj, LV_OBJ_FLAG_HIDDEN);
            return;
        }

        const uint32_t version = ++slot.version;
        const std::string disk_path = render_disk_cache_path(slot_name, version);
        const std::string lvgl_path = render_lvgl_cache_path(slot_name, version);
        if (!render_mac_text_png(disk_path, text, slot.width, slot.height, slot.style)) {
            return;
        }

        slot.generated_disk_paths.push_back(disk_path);
        slot.disk_path = disk_path;
        slot.file_path = lvgl_path;
        slot.last_text = text;
        lv_image_cache_drop(slot.file_path.c_str());
        lv_img_set_src(slot.obj, slot.file_path.c_str());
        lv_obj_set_size(slot.obj, slot.width, slot.height);
        lv_obj_clear_flag(slot.obj, LV_OBJ_FLAG_HIDDEN);
    }

    void refresh_rendered_texts()
    {
        update_rendered_text(title_slot_, "title", "xiaozhi");
        update_rendered_text(emoji_slot_, "emoji", emoji_);
        update_rendered_text(status_slot_, "status", status_);
        update_rendered_text(detail_slot_, "detail", detail_);
        update_rendered_text(hint_slot_, "hint", "回车：唤醒对话    ESC：返回主页");
    }
#endif

    static const char *emoji_icon_path(const std::string &emoji_key)
    {
        if (emoji_key == "😶") {
            return "A:/dist/images/detail_info.png";
        }
        if (emoji_key == "🙂" || emoji_key == "😆" || emoji_key == "😂" || emoji_key == "😍" || emoji_key == "😳" ||
            emoji_key == "😲" || emoji_key == "😉" || emoji_key == "😎" || emoji_key == "😌" || emoji_key == "🤤" ||
            emoji_key == "😘" || emoji_key == "😏" || emoji_key == "😴" || emoji_key == "😜" || emoji_key == "😊") {
            return "A:/dist/images/chat.png";
        }
        if (emoji_key == "😔" || emoji_key == "😠" || emoji_key == "😭" || emoji_key == "😱" || emoji_key == "🙄") {
            return "A:/dist/images/setting.png";
        }
        if (emoji_key == "🎙️" || emoji_key == "🎤" || emoji_key == "rec" || emoji_key == "listening") {
            return "A:/dist/images/rec.png";
        }
        if (emoji_key == "🤔" || emoji_key == ".." || emoji_key == "thinking") {
            return "A:/dist/images/detail_info.png";
        }
        if (emoji_key == "🔊" || emoji_key == "🔈" || emoji_key == "<<" || emoji_key == "speaking") {
            return "A:/dist/images/music.png";
        }
        if (emoji_key == "❌" || emoji_key == "!!" || emoji_key == "error") {
            return "A:/dist/images/setting.png";
        }
        if (emoji_key == "😄" || emoji_key == ":)" || emoji_key == "🙂" || emoji_key == "idle") {
            return "A:/dist/images/chat.png";
        }
        if (emoji_key == "🔗" || emoji_key == "[]" || emoji_key == "binding") {
            return "A:/dist/images/mesh.png";
        }
        if (emoji_key == "😊" || emoji_key == "😁" || emoji_key == "😀") {
            return "A:/dist/images/chat.png";
        }
        return "A:/dist/images/detail_info.png";
    }

    static std::string normalize_emotion_key(const std::string &value)
    {
        std::string key;
        key.reserve(value.size());
        for (char c : value) {
            if (std::isspace(static_cast<unsigned char>(c)) != 0) {
                continue;
            }
            key.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
        return key;
    }

    static std::string emotion_name_to_emoji(const std::string &value)
    {
        static const std::unordered_map<std::string, std::string> kEmotionToEmoji = {
            {"neutral", "😶"}, {"happy", "🙂"}, {"laughing", "😆"}, {"funny", "😂"},
            {"sad", "😔"}, {"angry", "😠"}, {"crying", "😭"}, {"loving", "😍"},
            {"embarrassed", "😳"}, {"surprised", "😲"}, {"shocked", "😱"}, {"thinking", "🤔"},
            {"winking", "😉"}, {"cool", "😎"}, {"relaxed", "😌"}, {"delicious", "🤤"},
            {"kissy", "😘"}, {"confident", "😏"}, {"sleepy", "😴"}, {"silly", "😜"},
            {"confused", "🙄"}, {"smile", "😊"},
        };

        const std::string key = normalize_emotion_key(value);
        const auto it = kEmotionToEmoji.find(key);
        if (it != kEmotionToEmoji.end()) {
            return it->second;
        }
        return value;
    }

    std::string resolve_server_emoji(const std::string &value, xiaozhi::AppState state) const
    {
        if (value.empty()) {
            return state_emoji_text(state);
        }

        const std::string mapped = emotion_name_to_emoji(value);
        if (mapped.empty()) {
            return state_emoji_text(state);
        }
        return mapped;
    }

    void creat_UI()
    {
        constexpr int kPageWidth = 320;
        constexpr int kPad = 12;
        constexpr int kTitleBarHeight = 30;
        constexpr int kEmojiSize = 40;
        constexpr int kEmojiGap = 8;
        constexpr int kBodyTop = kTitleBarHeight + 8;
        constexpr int kEmojiX = kPageWidth - kPad - kEmojiSize;
        constexpr int kEmojiY = kBodyTop + 2;
        constexpr int kTextRight = kEmojiX - kEmojiGap;
        constexpr int kTextWidthNarrow = kTextRight - kPad;
        constexpr int kTextWidthFull = kPageWidth - (kPad * 2);

        lv_obj_t *bg = lv_obj_create(ui_APP_Container);
        lv_obj_set_size(bg, kPageWidth, 150);
        lv_obj_set_pos(bg, 0, 0);
        lv_obj_set_style_radius(bg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(bg, lv_color_hex(0x0B1220), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(bg, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(bg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(bg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(bg, LV_OBJ_FLAG_SCROLLABLE);
        ui_obj_["bg"] = bg;

        lv_obj_t *title_bar = lv_obj_create(bg);
        lv_obj_set_size(title_bar, kPageWidth, kTitleBarHeight);
        lv_obj_set_pos(title_bar, 0, 0);
        lv_obj_set_style_radius(title_bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(title_bar, lv_color_hex(0x19324D), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(title_bar, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(title_bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    #if defined(__APPLE__)
        lv_obj_t *title = lv_img_create(title_bar);
        lv_obj_set_pos(title, kPad, -10);
        title_slot_.obj = title;
        title_slot_.width = kTextWidthFull;
        title_slot_.height = kTitleBarHeight - 4;
        title_slot_.style = {255, 255, 255, 255, 15.0f, true, false, false};

        lv_obj_t *emoji = lv_img_create(bg);
        lv_obj_set_pos(emoji, kEmojiX, kEmojiY);
        emoji_slot_.obj = emoji;
        emoji_slot_.width = kEmojiSize;
        emoji_slot_.height = kEmojiSize;
        emoji_slot_.style = {255, 255, 255, 255, 34.0f, false, true, false};
    #else
        lv_obj_t *title = lv_label_create(title_bar);
        lv_label_set_text(title, "xiaozhi");
        lv_obj_set_align(title, LV_ALIGN_LEFT_MID);
        lv_obj_set_x(title, kPad);
        lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(title, font14(), LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t *emoji = lv_img_create(bg);
        lv_img_set_src(emoji, emoji_icon_path(emoji_));
        lv_img_set_zoom(emoji, 96);
        lv_obj_set_pos(emoji, kEmojiX, kEmojiY);
    #endif
        ui_obj_["emoji"] = emoji;

        lv_obj_t *status =
    #if defined(__APPLE__)
            lv_img_create(bg);
        lv_obj_set_pos(status, kPad, kBodyTop);
        status_slot_.obj = status;
        status_slot_.width = kTextWidthNarrow;
        status_slot_.height = 24;
        status_slot_.style = {245, 247, 250, 255, 18.0f, true, false, false};
    #else
            lv_label_create(bg);
        lv_label_set_text(status, status_.c_str());
        lv_obj_set_pos(status, kPad, kBodyTop);
        lv_obj_set_width(status, kTextWidthNarrow);
        lv_label_set_long_mode(status, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_color(status, lv_color_hex(0xF5F7FA), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(status, font16(), LV_PART_MAIN | LV_STATE_DEFAULT);
    #endif
        ui_obj_["status"] = status;

        lv_obj_t *detail =
    #if defined(__APPLE__)
            lv_img_create(bg);
        lv_obj_set_pos(detail, kPad, kBodyTop + 30);
        detail_slot_.obj = detail;
        detail_slot_.width = kTextWidthNarrow;
        detail_slot_.height = 44;
        detail_slot_.style = {137, 164, 194, 255, 13.0f, false, false, true};
    #else
            lv_label_create(bg);
        lv_label_set_text(detail, detail_.c_str());
        lv_obj_set_pos(detail, kPad, kBodyTop + 30);
        lv_obj_set_width(detail, kTextWidthNarrow);
        lv_label_set_long_mode(detail, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_color(detail, lv_color_hex(0x89A4C2), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(detail, font12(), LV_PART_MAIN | LV_STATE_DEFAULT);
    #endif
        ui_obj_["detail"] = detail;

        lv_obj_t *hint =
    #if defined(__APPLE__)
            lv_img_create(bg);
        lv_obj_set_pos(hint, kPad, 126);
        hint_slot_.obj = hint;
        hint_slot_.width = kTextWidthFull;
        hint_slot_.height = 16;
        hint_slot_.style = {82, 101, 122, 255, 11.0f, false, false, false};
    #else
            lv_label_create(bg);
        lv_label_set_text(hint, "回车：唤醒对话    ESC：返回主页");
        lv_obj_set_pos(hint, kPad, 126);
        lv_obj_set_width(hint, kTextWidthFull);
        lv_label_set_long_mode(hint, LV_LABEL_LONG_CLIP);
        lv_obj_set_style_text_color(hint, lv_color_hex(0x52657A), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(hint, font12(), LV_PART_MAIN | LV_STATE_DEFAULT);
    #endif

    #if defined(__APPLE__)
        refresh_rendered_texts();
    #endif
    }

    static const char *state_name(xiaozhi::AppState state)
    {
        switch (state) {
        case xiaozhi::AppState::Binding:
            return "等待绑定";
        case xiaozhi::AppState::Idle:
            return "准备就绪";
        case xiaozhi::AppState::Listening:
            return "正在聆听";
        case xiaozhi::AppState::Thinking:
            return "正在思考";
        case xiaozhi::AppState::Speaking:
            return "正在播报";
        case xiaozhi::AppState::Error:
            return "发生错误";
        }
        return "xiaozhi";
    }

    static const char *state_emoji_text(xiaozhi::AppState state)
    {
        switch (state) {
        case xiaozhi::AppState::Binding:
            return "🔗";
        case xiaozhi::AppState::Idle:
            return "😄";
        case xiaozhi::AppState::Listening:
            return "🎙️";
        case xiaozhi::AppState::Thinking:
            return "🤔";
        case xiaozhi::AppState::Speaking:
            return "🔊";
        case xiaozhi::AppState::Error:
            return "❌";
        }
        return "❓";
    }

    static std::string localize_detail(xiaozhi::AppState state, const std::string &text)
    {
        if (text.empty()) {
            return {};
        }

        if (text.rfind("code ", 0) == 0) {
            const size_t code_start = 5;
            const size_t code_end = text.find(' ', code_start);
            const std::string code = text.substr(code_start, code_end == std::string::npos ? std::string::npos : code_end - code_start);
            if (text.find("open app to bind") != std::string::npos) {
                return "请打开小智 App 完成绑定，绑定码：" + code;
            }
            if (text.find("waiting bind") != std::string::npos) {
                return "等待绑定中，绑定码：" + code;
            }
        }

        if (text == "Initializing embedded session...") return "正在初始化内嵌会话...";
        if (text == "Waiting for embedded runtime...") return "等待内嵌运行时启动...";
        if (text == "paired connecting") return "设备已配对，正在连接服务...";
        if (text == "binding complete connecting") return "绑定完成，正在连接服务...";
        if (text == "connecting") return "正在连接服务...";
        if (text == "Ready") return "按回车开始对话";
        if (text == "listening server vad") return "正在听你说话...";
        if (text == "server vad stop waiting response") return "已停止录音，等待小智回复...";
        if (text == "server text") return "收到服务端文本消息";
        if (text == "tts start") return "开始播放语音回复";
        if (text == "tts stop") return "语音播放结束";
        if (text == "user wakeup interrupt") return "已打断当前播报，重新唤醒";
        if (text == "server ended conversation") return "本轮对话已结束";
        if (text == "stopped") return "小智会话已停止";
        if (text == "failed to initialize subsystem") return "初始化子系统失败";
        if (text.rfind("ota failed ", 0) == 0) return "OTA 检查失败：" + text.substr(11);

        if (state == xiaozhi::AppState::Listening) {
            return text.empty() ? "正在听你说话..." : text;
        }
        return text;
    }

    void update_labels()
    {
    #if defined(__APPLE__)
        refresh_rendered_texts();
    #else
        if (ui_obj_.count("status") && ui_obj_["status"]) {
            lv_label_set_text(ui_obj_["status"], status_.c_str());
        }
        if (ui_obj_.count("detail") && ui_obj_["detail"]) {
            lv_label_set_text(ui_obj_["detail"], detail_.c_str());
        }
        if (ui_obj_.count("emoji") && ui_obj_["emoji"]) {
            lv_img_set_src(ui_obj_["emoji"], emoji_icon_path(emoji_));
            lv_img_set_zoom(ui_obj_["emoji"], 72);
        }
    #endif
    }

    void apply_state(const std::string &status, const std::string &detail, const std::string &emoji)
    {
        status_ = status;
        detail_ = detail;
        emoji_ = emoji;
        update_labels();
    }

    void event_handler_init()
    {
        lv_obj_add_event_cb(ui_root, UIXiaoZhiPage::static_lvgl_handler, LV_EVENT_ALL, this);
    }

    static void static_lvgl_handler(lv_event_t *e)
    {
        auto *self = static_cast<UIXiaoZhiPage *>(lv_event_get_user_data(e));
        if (self) {
            self->event_handler(e);
        }
    }

    void event_handler(lv_event_t *e)
    {
        if (lv_event_get_code(e) != LV_EVENT_KEY) {
            return;
        }

        const uint32_t key = lv_event_get_key(e);
        switch (key) {
        case LV_KEY_ENTER:
            if (hal_ != nullptr) {
                hal_->emitPress();
                hal_->emitRelease();
            }
            break;
        case LV_KEY_ESC:
            stop_xiaozhi();
            if (go_back_home) {
                go_back_home();
            }
            break;
        default:
            break;
        }
    }

    void launch_xiaozhi()
    {
        stop_xiaozhi();

        auto cfg = xiaozhi::loadConfig();
        auto hal = std::make_unique<AppLaunchHal>();
        hal_ = hal.get();
        auto ui = std::make_unique<AppLaunchUi>(this);
        auto ws = std::make_unique<xiaozhi::WsClientBridge>();
    #if defined(APPLAUNCH_USE_SDL_AUDIO)
        auto audio = std::make_unique<xiaozhi::AudioPipelineSdl>();
    #else
        auto audio = std::make_unique<xiaozhi::AudioPipelineStub>();
    #endif

        auto app = std::make_unique<xiaozhi::Application>(
            std::move(cfg),
            std::move(hal),
            std::move(ui),
            std::move(ws),
            std::move(audio));

        if (!app->start()) {
            hal_ = nullptr;
            apply_state("小智启动失败", "请检查音频设备、OTA 接口和 ws_bridge 路径。", "❌");
            return;
        }

        application_ = std::move(app);
        if (!app_timer_) {
            app_timer_ = lv_timer_create(UIXiaoZhiPage::static_poll_cb, 16, this);
        }
    }

    void stop_xiaozhi()
    {
        if (application_) {
            application_->stop();
            application_.reset();
        }
        hal_ = nullptr;
    }

    static void static_poll_cb(lv_timer_t *timer)
    {
        auto *self = static_cast<UIXiaoZhiPage *>(lv_timer_get_user_data(timer));
        if (self) {
            self->poll_child();
        }
    }

    void poll_child()
    {
        if (!application_) {
            return;
        }

        application_->tick();
        if (!application_->isRunning()) {
            stop_xiaozhi();
        }
    }

public:
    ~UIXiaoZhiPage()
    {
        if (app_timer_) {
            lv_timer_delete(app_timer_);
            app_timer_ = nullptr;
        }
        stop_xiaozhi();
#if defined(__APPLE__)
        cleanup_rendered_assets(title_slot_);
        cleanup_rendered_assets(emoji_slot_);
        cleanup_rendered_assets(status_slot_);
        cleanup_rendered_assets(detail_slot_);
        cleanup_rendered_assets(hint_slot_);
#endif
    }
};