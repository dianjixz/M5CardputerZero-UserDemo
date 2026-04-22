#include "ui.h"
#include <stdio.h>
#include <string.h>
lv_group_t *Screen1group = NULL;
lv_group_t *AppStoregroup = NULL;
lv_group_t *APPNotegroup = NULL;
lv_group_t *AppPythongroup = NULL;




void input_group_init(void)
{
    Screen1group = lv_group_create();
    AppStoregroup = lv_group_create();
    APPNotegroup = lv_group_create();
    AppPythongroup = lv_group_create();
    lv_group_add_obj(Screen1group, ui_Screen1);
    lv_indev_set_group(lv_indev_get_next(NULL), Screen1group);
}