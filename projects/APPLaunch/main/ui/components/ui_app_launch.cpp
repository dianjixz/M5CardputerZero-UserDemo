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
#include <functional>
#include <chrono>
#include "ui_launch_page.hpp"
#include "ui_app_store.hpp"
#include "ui_app_music.hpp"
#include "ui_app_setup.hpp"
#include "ui_app_xiaozhi.hpp"
#include "ui_app_console.hpp"
#include "ui_app_IpPanel.hpp"

// 前向声明
class app_launch_S;

static lv_obj_t *ensure_panel_icon(lv_obj_t *panel)
{
    if (panel == NULL) {
        return NULL;
    }

    lv_obj_t *child = lv_obj_get_child(panel, 0);
    if (child == NULL) {
        child = lv_img_create(panel);
        lv_obj_center(child);
        lv_obj_clear_flag(child, (lv_obj_flag_t)(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));
    }

    return child;
}

static void set_panel_icon(lv_obj_t *panel, const char *icon)
{
    if (panel == NULL) {
        return;
    }

    lv_obj_set_style_bg_img_src(panel, icon, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_image_opa(panel, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *img = ensure_panel_icon(panel);
    if (img == NULL) {
        return;
    }

    lv_img_set_src(img, icon);
    lv_obj_set_style_img_opa(img, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(img);
}

// ============================================================
// 类型标签
// ============================================================
template <class PageT>
struct page_t
{
    using type = PageT;
};
template <class PageT>
inline constexpr page_t<PageT> page_v{};

// ============================================================
// app:统一的应用描述 + 发射器
// ============================================================
struct app
{
    std::string Name;
    std::string Icon;
    std::function<void(app_launch_S *)> launch;

    // ① 外部命令
    app(std::string name,
        std::string icon,
        std::string exec,
        bool terminal);

    // ① 外部命令
    app(std::string name,
        std::string icon,
        std::string exec,
        bool terminal, bool sysplause);

    // ② 内置 UI 页面
    template <class PageT>
    app(std::string name,
        std::string icon,
        page_t<PageT> /*tag*/);
};

// ============================================================
// app_launch_S
// ============================================================
class app_launch_S
{
private:
    std::list<app> app_list;
    int current_app = 0;

public:
    std::shared_ptr<void> app_Page;

public:
    app_launch_S()
    {
        app_list.emplace_back("Python",
                              "A:/dist/images/PYTHON_logo.png", "python3", true, false);
        app_list.emplace_back("STORE",
                              "A:/dist/images/Store_logo.png", page_v<UIStorePage>);
        app_list.emplace_back("CLI",
                              "A:/dist/images/CLI_logo.png", "bash", true, false);
        app_list.emplace_back("CLAW",
                              "A:/dist/images/CLAW_logo.png", "/home/pi/zeroclaw agent", true);
        app_list.emplace_back("SETTING",
                              "A:/dist/images/SETTING_logo.png", page_v<UISetupPage>);
        app_list.emplace_back("MUSIC",
                              "A:/dist/images/MUSIC_logo.png", page_v<UIMusicPage>);
        app_list.emplace_back("AUDIO_PLAYER",
        app_list.emplace_back("XIAOZHI",
                              "A:/dist/images/CLI_logo.png", page_v<UIXiaoZhiPage>);
        app_list.emplace_back("NIHAO",
                              "A:/dist/images/MUSIC_logo.png",
                              "tinyplay -D1 -d0 /home/pi/zhou.wav",
                              true);
        app_list.emplace_back("IP_PANEL",
                              "A:/dist/images/ssh.png", page_v<UIIpPanelPage>);
        app_list.emplace_back("MATH",
                              "A:/dist/images/math.png", 
                              "/home/pi/M5CardputerZero-Calculator-linux-aarch64", false);

        set_panel_icon(ui_outPanelzuo, "A:/dist/images/PYTHON_logo.png");
        set_panel_icon(ui_zuoPanel, "A:/dist/images/Store_logo.png");
        set_panel_icon(ui_switchPanel, "A:/dist/images/CLI_logo.png");
        set_panel_icon(ui_youPanel, "A:/dist/images/CLAW_logo.png");
        set_panel_icon(ui_outPanelyou, "A:/dist/images/SETTING_logo.png");
    }

    void launch_app()
    {
        auto it = std::next(app_list.begin(), current_app);
        it->launch(this);
    }

    static void lv_go_back_home(void *arg)
    {
        auto self = (app_launch_S *)arg;
        lv_timer_enable(true);
        lv_indev_set_group(lv_indev_get_next(NULL), Screen1group);
        lv_disp_load_scr(ui_Screen1);
        lv_refr_now(NULL);
        if (self->app_Page)
            self->app_Page.reset();
    }

    void go_back_home()
    {
        lv_async_call(lv_go_back_home, this);
    }

    // 改为接收 std::string，不再依赖 app::Exec 成员
    void launch_Exec_in_terminal(const std::string &exec, bool sysplause = true)
    {
        printf("Launching terminal app: %s\n", exec.c_str());
        auto p = std::make_shared<UIConsolePage>();
        app_Page = p;
        lv_disp_load_scr(p->get_ui());
        lv_indev_set_group(lv_indev_get_next(NULL), p->get_key_group());
        p->go_back_home = std::bind(&app_launch_S::go_back_home, this);
        p->terminal_sysplause = sysplause;
        p->exec(exec);
    }

    void launch_Exec(const std::string &exec)
    {
        printf("Launching external app: %s\n", exec.c_str());
        lv_disp_t *disp = lv_disp_get_default();
        lv_indev_t *indev = lv_indev_get_next(NULL);
        LVGL_RUN_FLAGE = 0;
        if (indev)
            lv_indev_set_group(indev, NULL);
        lv_timer_enable(false);
        lv_refr_now(disp);

        pid_t pid = fork();
        if (pid == 0)
        {
            execlp(exec.c_str(), exec.c_str(), (char *)NULL);
            perror("execlp failed");
            _exit(EXIT_FAILURE);
        }
        else if (pid > 0)
        {
            pid_t pid_ret;
            int status;
            int end_status = 0;
            std::chrono::time_point<std::chrono::steady_clock> start_time;
            std::chrono::time_point<std::chrono::steady_clock> end_time;
            int ctrl_c_count = 0;
            for(;;)
            {
                if(end_status == 0)
                {
                    pid_ret = waitpid(pid, &status, WNOHANG);
                    if (pid_ret > 0)
                        break;
                    usleep(100000); // 100ms
                    if(LVGL_HOME_KEY_FLAGE)
                    {
                        end_status = 1;
                        start_time = std::chrono::steady_clock::now();
                    }
                }
                if(end_status == 1)
                {
                    if(LVGL_HOME_KEY_FLAGE)
                    {
                        end_time = std::chrono::steady_clock::now();
                        if(std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count() >= 5)
                        {
                            // kill(pid, SIGINT);
                            end_status = 2;
                        }
                    }
                    else
                    {
                        end_status = 0;
                    }
                }
                if(end_status == 2)
                {
                    ctrl_c_count ++;
                    kill(pid, SIGINT);
                    usleep(100000); // 100ms
                    pid_ret = waitpid(pid, &status, WNOHANG);
                    if (pid_ret > 0)
                        break;
                    if(ctrl_c_count >= 30)
                    {
                        // kill(pid, SIGKILL);
                        end_status = 3;
                        ctrl_c_count = 0 ;
                    }
                }
                if(end_status == 3)
                {
                    ctrl_c_count ++;
                    kill(pid, SIGKILL);
                    usleep(100000); // 100ms
                    pid_ret = waitpid(pid, &status, WNOHANG);
                    if (pid_ret > 0)
                        break;
                    if (pid_ret < 0)
                        break;
                    if(ctrl_c_count >= 300)
                    {
                        break;
                    }
                }
            }
            
            // waitpid(pid, &status, 0);
            if (WIFEXITED(status)) {
                printf("App %s exited normally, code=%d\n", exec.c_str(), WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("App %s killed by signal %d\n", exec.c_str(), WTERMSIG(status));
            }
            lv_timer_enable(true);
            if (indev)
                lv_indev_set_group(indev, Screen1group);
            lv_disp_load_scr(ui_Screen1);
            lv_obj_invalidate(lv_screen_active());
            lv_refr_now(disp);
        }
        else
        {
            perror("fork failed");
            lv_timer_enable(true);
            if (indev)
                lv_indev_set_group(indev, lv_group_get_default());
        }
        LVGL_RUN_FLAGE = 1;
    }

    void zuo(lv_obj_t *panel, lv_obj_t *label)
    {
        current_app = current_app == (int)app_list.size() - 1 ? 0 : current_app + 1;
        int next_app = current_app;
        next_app = next_app == (int)app_list.size() - 1 ? 0 : next_app + 1;
        next_app = next_app == (int)app_list.size() - 1 ? 0 : next_app + 1;
        auto it = std::next(app_list.begin(), next_app);
        lv_label_set_text(label, it->Name.c_str());
        set_panel_icon(panel, it->Icon.c_str());
    }

    void you(lv_obj_t *panel, lv_obj_t *label)
    {
        current_app = current_app == 0 ? (int)app_list.size() - 1 : current_app - 1;
        int next_app = current_app;
        next_app = next_app == 0 ? (int)app_list.size() - 1 : next_app - 1;
        next_app = next_app == 0 ? (int)app_list.size() - 1 : next_app - 1;
        auto it = std::next(app_list.begin(), next_app);
        lv_label_set_text(label, it->Name.c_str());
        set_panel_icon(panel, it->Icon.c_str());
    }

    ~app_launch_S() {}
};

// ============================================================
// app 构造函数的实现(放到 app_launch_S 定义之后)
// ============================================================
inline app::app(std::string name,
                std::string icon,
                std::string exec,
                bool terminal)
    : Name(std::move(name)), Icon(std::move(icon))
{
    launch = [exec = std::move(exec), terminal](app_launch_S *ctx)
    {
        if (terminal)
            ctx->launch_Exec_in_terminal(exec);
        else
            ctx->launch_Exec(exec);
    };
}

inline app::app(std::string name,
                std::string icon,
                std::string exec,
                bool terminal,
                bool sysplause)
    : Name(std::move(name)), Icon(std::move(icon))
{
    launch = [exec = std::move(exec), terminal, sysplause](app_launch_S *ctx)
    {
        if (terminal)
            ctx->launch_Exec_in_terminal(exec, sysplause);
        else
            ctx->launch_Exec(exec);
    };
}

template <class PageT>
app::app(std::string name,
         std::string icon,
         page_t<PageT> /*tag*/)
    : Name(std::move(name)), Icon(std::move(icon))
{
    launch = [](app_launch_S *self)
    {
        auto p = std::make_shared<PageT>();
        self->app_Page = p;
        lv_disp_load_scr(p->get_ui());
        lv_indev_set_group(lv_indev_get_next(NULL),
                           p->get_key_group());
        p->go_back_home =
            std::bind(&app_launch_S::go_back_home, self);
    };
}

// ============================================================
std::unique_ptr<app_launch_S> app_launch_Ser;

extern "C"
{
    void ui_info_bind()
    {
        app_launch_Ser = std::make_unique<app_launch_S>();
    }
    void cpp_app_zuo(lv_obj_t *panel, lv_obj_t *label)
    {
        app_launch_Ser->zuo(panel, label);
    }
    void cpp_app_you(lv_obj_t *panel, lv_obj_t *label)
    {
        app_launch_Ser->you(panel, label);
    }
    void cpp_app_launch()
    {
        app_launch_Ser->launch_app();
    }
}