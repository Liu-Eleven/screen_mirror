#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "lvgl/lvgl.h"

lv_obj_t *activate_show;

void del_activate_icon(void)
{
    lv_obj_del(activate_show);
    activate_show = NULL;
}

void display_ui_activate_icon(void)
{
    if (activate_show)
        lv_obj_clear_flag(activate_show, LV_OBJ_FLAG_HIDDEN);
}

void hide_ui_activate_icon(void)
{
    if (activate_show)
        lv_obj_add_flag(activate_show, LV_OBJ_FLAG_HIDDEN);
}

void create_ui_activate_icon(lv_obj_t *parent)
{
    char *png_path[] = {"A:/usr/share/lv_projector/activate/activate_discon.png"};

    if (!activate_show)
        activate_show = main_page_prompt_create(parent, png_path[0]);
}
