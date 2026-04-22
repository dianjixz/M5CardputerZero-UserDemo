#include "../ui.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <unordered_map>
#include <list>
#include <memory>
#include <string>
#include <functional>
#include "ui_launch_page.hpp"
#include "ui_app_store.hpp"
#include "ui_app_music.hpp"
#include "ui_app_setup.hpp"
#include "ui_app_console.hpp"

// ==================== 运行APP ====================
struct app
{
    std::string Name;
    std::string Icon;
    std::string Exec;
    bool terminal; // 是否在终端中运行
};
std::list<app> app_list;

class app_launch_S
{
private:
    std::list<app> app_list;
    int current_app;
public:
    std::shared_ptr<void> app_Page;
public:
    app_launch_S()
    {
        // 固定图标，不允许用户修改
        app_list.push_back(app{"Python", "A:/dist/images/PYTHON_logo.png", "python3", true});
        app_list.push_back(app{"STORE", "A:/dist/images/Store_logo.png", "launch_store1"});
        app_list.push_back(app{"CLI", "A:/dist/images/CLI_logo.png", "bash", true});
        app_list.push_back(app{"CLAW", "A:/dist/images/CLAW_logo.png", "/home/pi/zeroclaw agent", true});
        app_list.push_back(app{"SETTING", "A:/dist/images/SETTING_logo.png", "launch_setting"});
        current_app = 2;

        // 动态图标，允许用户自定义
        // app_list.push_back(app{"STORE1", "A:/dist/images/Store_logo.png", "launch_store1"});
        app_list.push_back(app{"MUSIC", "A:/dist/images/MUSIC_logo.png", "launch_music"});
        app_list.push_back(app{"NIHAO", "A:/dist/images/MUSIC_logo.png", "/home/nihao/w2T/github/M5CardputerZero-UserDemo/projects/UserDemo/nihao", true});
    }
    void launch_app()
    {
        auto it = std::next(app_list.begin(), current_app);
        if (it->Exec == "launch_python")
        {
            printf("Launching Python...\n");
            lv_disp_load_scr(ui_pythonapp);
            lv_indev_set_group(lv_indev_get_next(NULL), AppPythongroup); // 使用Python应用专用输入组
        }
        else if (it->Exec == "launch_store")
        {
            // 打开商店界面
            lv_disp_load_scr(ui_AppStore);
            lv_indev_set_group(lv_indev_get_next(NULL), AppStoregroup);
        }
        else if (it->Exec == "launch_store1")
        {
            auto p = std::make_shared<UIStorePage>();
            app_Page = p;
            lv_disp_load_scr(p->get_ui());
            lv_indev_set_group(lv_indev_get_next(NULL), p->get_key_group());
            p->go_back_home = std::bind(&app_launch_S::go_back_home, this);
        }
        else if (it->Exec == "launch_cli")
        {
            printf("Launching CLI...\n");
        }
        else if (it->Exec == "launch_claw")
        {
            printf("Launching CLAW...\n");
        }
        else if (it->Exec == "launch_setting")
        {
            printf("Launching SETTING...\n");
            auto p = std::make_shared<UISetupPage>();
            app_Page = p;
            lv_disp_load_scr(p->get_ui());
            lv_indev_set_group(lv_indev_get_next(NULL), p->get_key_group());
            p->go_back_home = std::bind(&app_launch_S::go_back_home, this);
        }
        else if (it->Exec == "launch_music")
        {
            printf("Launching MUSIC...\n");
            auto p = std::make_shared<UIMusicPage>();
            app_Page = p;
            lv_disp_load_scr(p->get_ui());
            lv_indev_set_group(lv_indev_get_next(NULL), p->get_key_group());
            p->go_back_home = std::bind(&app_launch_S::go_back_home, this);
        }
        else
        {
            if (it->terminal)
            {
                launch_Exec_in_terminal(&(*it));
            }
            else
            {
                launch_Exec(&(*it));
            }
        }
    }

    static void lv_go_back_home(void *arg)
    {
        auto self = (app_launch_S*)arg;
        // 恢复LVGL任务处理
        lv_timer_enable(true);

        // 重新绑定输入设备到当前界面组
        lv_indev_set_group(lv_indev_get_next(NULL), Screen1group);

        // 重新加载主屏幕，刷新界面
        lv_disp_load_scr(ui_Screen1);
        lv_refr_now(NULL);

        if(self->app_Page)
        {
            self->app_Page.reset(); // 清空app_page
        }
    }

    void go_back_home()
    {
        lv_async_call(lv_go_back_home, this);
    }

    void launch_Exec_in_terminal(struct app *it)
    {
        printf("Launching terminal app: %s\n", it->Exec.c_str());
        auto p = std::make_shared<UIConsolePage>();
        app_Page = p;
        lv_disp_load_scr(p->get_ui());
        lv_indev_set_group(lv_indev_get_next(NULL), p->get_key_group());
        p->go_back_home = std::bind(&app_launch_S::go_back_home, this);
        p->exec(it->Exec);
    }

    void launch_Exec(struct app *it)
    {
        printf("Launching external app: %s\n", it->Exec.c_str());

        // 保存当前LVGL状态，暂停渲染
        lv_disp_t *disp = lv_disp_get_default();
        lv_indev_t *indev = lv_indev_get_next(NULL);

        // 释放输入设备控制权
        if (indev)
        {
            lv_indev_set_group(indev, NULL);
        }

        // 停止LVGL任务处理
        lv_timer_enable(false);

        // 同步屏幕内容
        lv_refr_now(disp);

        pid_t pid = fork();
        if (pid == 0)
        {
            // 子进程：执行目标程序
            execlp(it->Exec.c_str(), it->Exec.c_str(), NULL);
            // 如果执行失败，退出子进程
            perror("execlp failed");
            _exit(EXIT_FAILURE);
        }
        else if (pid > 0)
        {
            // 父进程：等待子进程结束
            int status;
            waitpid(pid, &status, 0);

            printf("App %s exited with status %d\n", it->Exec.c_str(), WEXITSTATUS(status));

            // 恢复LVGL任务处理
            lv_timer_enable(true);

            // 重新绑定输入设备到当前界面组
            if (indev)
            {
                // lv_indev_set_group(indev, lv_group_get_default());
                lv_indev_set_group(lv_indev_get_next(NULL), Screen1group);
            }

            // 重新加载主屏幕，刷新界面
            lv_disp_load_scr(ui_Screen1);
            lv_refr_now(disp);
        }
        else
        {
            // fork失败
            perror("fork failed");

            // 恢复LVGL状态
            lv_timer_enable(true);
            if (indev)
            {
                lv_indev_set_group(indev, lv_group_get_default());
            }
        }
    }

    void zuo(lv_obj_t *panel, lv_obj_t *label)
    {
        current_app = current_app == app_list.size() - 1 ? 0 : current_app + 1;
        int next_app = current_app;
        next_app = next_app == app_list.size() - 1 ? 0 : next_app + 1;
        next_app = next_app == app_list.size() - 1 ? 0 : next_app + 1;
        auto it = std::next(app_list.begin(), next_app);
        lv_label_set_text(label, it->Name.c_str());
        lv_obj_set_style_bg_img_src(panel, it->Icon.c_str(), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    void you(lv_obj_t *panel, lv_obj_t *label)
    {
        current_app = current_app == 0 ? app_list.size() - 1 : current_app - 1;
        int next_app = current_app;
        next_app = next_app == 0 ? app_list.size() - 1 : next_app - 1;
        next_app = next_app == 0 ? app_list.size() - 1 : next_app - 1;
        auto it = std::next(app_list.begin(), next_app);
        lv_label_set_text(label, it->Name.c_str());
        lv_obj_set_style_bg_img_src(panel, it->Icon.c_str(), LV_PART_MAIN | LV_STATE_DEFAULT);
    }



    ~app_launch_S()
    {
    }
};

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
