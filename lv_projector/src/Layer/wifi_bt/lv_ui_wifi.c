#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "../prompt/lv_ui_prompt.h"
#include "lv_common.h"
#include "lvgl/lvgl.h"

#if ENABLE_WIFI
lv_obj_t *wifi_show;
LV_IMG_DECLARE(wifi_con);
LV_IMG_DECLARE(wifi_discon);

static void wifi_show_event_handle(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    main_page_prompt_data_t *msg = (main_page_prompt_data_t *)lv_event_get_param(e);

    if (code == LV_EVENT_REFRESH) {
        if (!strcmp(msg->status, "disconn")) {
            lv_img_set_src(lv_obj_get_child(obj, 0), &wifi_discon);
        } else if (!strcmp(msg->status, "conn")) {
            lv_img_set_src(lv_obj_get_child(obj, 0), &wifi_con);
        }

        if (lv_obj_get_child_cnt(obj) > 1) {
            lv_label_set_text(lv_obj_get_child(obj, 1), msg->devName);
            lv_coord_t str_width = lv_txt_get_width(msg->devName, strlen(msg->devName),
                                                    &GENERAL_FONT_SMALL, 0, LV_TEXT_FLAG_NONE);
            str_width = str_width * 2;
            if (str_width > 220) str_width = 220;
            lv_obj_set_width(obj, 40 + str_width);
            lv_obj_set_width(lv_obj_get_child(obj, 1), str_width);
        }
    }
    if (msg) {
        free(msg->devName);
        free(msg);
    }
}
#endif

#if ENABLE_BT

lv_obj_t *bt_show;
LV_IMG_DECLARE(bt_con);
LV_IMG_DECLARE(bt_discon);

static void bt_show_event_handle(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    main_page_prompt_data_t *msg = (main_page_prompt_data_t *)lv_event_get_param(e);

    if (code == LV_EVENT_REFRESH) {
        if (!strcmp(msg->status, "disconn")) {
            lv_img_set_src(lv_obj_get_child(obj, 0), &bt_discon);
        } else if (!strcmp(msg->status, "conn")) {
            lv_img_set_src(lv_obj_get_child(obj, 0), &bt_con);
        }
        if (lv_obj_get_child_cnt(obj) > 1) {
            lv_label_set_text(lv_obj_get_child(obj, 1), msg->devName);
            lv_coord_t str_width = lv_txt_get_width(msg->devName, strlen(msg->devName),
                                                    &GENERAL_FONT_SMALL, 0, LV_TEXT_FLAG_NONE);
            str_width = str_width * 2;
            if (str_width > 220) str_width = 220;
            lv_obj_set_width(obj, 40 + str_width);
            lv_obj_set_width(lv_obj_get_child(obj, 1), str_width);
        }
    }

    if (msg) {
        free(msg->devName);
        free(msg);
    }
}
#endif

void display_ui_wifi_bt_icon(void)
{
#if ENABLE_BT
    lv_obj_clear_flag(bt_show, LV_OBJ_FLAG_HIDDEN);
#endif
#if ENABLE_WIFI
    lv_obj_clear_flag(wifi_show, LV_OBJ_FLAG_HIDDEN);
#endif
}

void hide_ui_wifi_bt_icon(void)
{
#if ENABLE_BT
    lv_obj_add_flag(bt_show, LV_OBJ_FLAG_HIDDEN);
#endif
#if ENABLE_WIFI
    lv_obj_add_flag(wifi_show, LV_OBJ_FLAG_HIDDEN);
#endif
}

void create_ui_wifi_bt_icon(lv_obj_t *parent)
{
#if ENABLE_BT
    bt_show = main_page_prompt_create(parent, &bt_discon);
    lv_obj_add_event_cb(bt_show, bt_show_event_handle, LV_EVENT_REFRESH, 0);
#endif
#if ENABLE_WIFI
    wifi_show = main_page_prompt_create(parent, &wifi_discon);
    lv_obj_add_event_cb(wifi_show, wifi_show_event_handle, LV_EVENT_REFRESH, 0);
#endif
}
