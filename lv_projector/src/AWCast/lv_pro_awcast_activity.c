#include <stdlib.h>
#include "widget/lv_pro_res.h"
#include "../lv_pro_launcher.h"
#include "lvgl/lvgl.h"
#include "lv_pro_wirelesssp.h"

lv_obj_t *awcast_page;
lv_group_t *awcast_group;
uint8_t AWCast_show_flag;

int get_awcast_show_flag(void)
{
    return AWCast_show_flag;
}

int set_awcast_show_flag(int flag)
{
    AWCast_show_flag = flag;
    return 0;
}

static void AWCast_activity_exit(void)
{
    if (!awcast_page)
        return;

    lv_group_remove_all_objs(awcast_group);
    lv_obj_del(awcast_page);
    lv_group_del(awcast_group);
    awcast_group = NULL;
    awcast_page = NULL;
    lv_group_set_default(NULL);
    set_current_ui_group(WirelessSP_group);
}

static void AWCast_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t *key = lv_event_get_param(e);

    if (code == LV_EVENT_KEY) {
        if (!is_global_key_go_new_channel(*key)) {
            if (get_awcast_show_flag()) {
                set_awcast_show_flag(0);

                AWCast_activity_exit();
            }
        } else if (*key == LV_KEY_BACK) {
            if (get_awcast_show_flag()) {
                AWCastExitShow();
                AWCast_reset();
            }
        }
    }
}

static void AWCast_activity_init(void)
{
    if (!awcast_page)
        awcast_page = lv_obj_create(lv_layer_top());

    if (!awcast_group)
        awcast_group = lv_group_create();

    lv_obj_set_size(awcast_page, lv_pct(100), lv_pct(100));
    lv_obj_add_style(awcast_page, &lv_pro_res.set_bg_black, 0);
    lv_obj_clear_flag(awcast_page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(awcast_page, LV_OPA_TRANSP, 0);
    lv_group_add_obj(awcast_group, awcast_page);
    lv_obj_add_event_cb(awcast_page, AWCast_event_handler, LV_EVENT_KEY, NULL);

    set_current_ui_group(awcast_group);
}

int AWCastExitShow(void)
{
    if (!get_awcast_show_flag())
        return;
    set_awcast_show_flag(0);

    lv_disp_get_default()->driver->screen_transp = 0;
    lv_memset_00(lv_disp_get_default()->driver->draw_buf->buf_act,
                 lv_disp_get_default()->driver->draw_buf->size * sizeof(lv_color32_t));

    lv_obj_clear_flag(WirelessSP_activity, LV_OBJ_FLAG_HIDDEN);
    lv_disp_set_bg_opa(lv_disp_get_default(), LV_OPA_COVER);

    AWCast_activity_exit();

    return 0;
}

int AWCastEntryShow(void)
{
    AWCast_activity_init();

    lv_obj_add_flag(WirelessSP_activity, LV_OBJ_FLAG_HIDDEN);
    lv_disp_get_default()->driver->screen_transp = 1;
    lv_disp_set_bg_opa(lv_disp_get_default(), LV_OPA_TRANSP);
    lv_memset_00(lv_disp_get_default()->driver->draw_buf->buf_act,
                lv_disp_get_default()->driver->draw_buf->size * sizeof(lv_color32_t));

    set_awcast_show_flag(1);
    return 0;
}
