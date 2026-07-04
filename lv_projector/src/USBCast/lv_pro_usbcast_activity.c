#include <stdlib.h>
#include "widget/lv_pro_res.h"
#include "../lv_pro_launcher.h"
#include "lvgl/lvgl.h"
#include "lv_pro_wiredsp.h"

lv_obj_t *usbcast_page;
lv_group_t *usbcast_group;
static uint8_t usbcast_key_flag;

int get_usbcast_key_flag(void)
{
    return usbcast_key_flag;
}

int set_usbcast_key_flag(int flag)
{
    usbcast_key_flag = flag;
    return 0;
}

static void usbcast_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t *key = lv_event_get_param(e);

    if (code == LV_EVENT_KEY) {
        if (!is_global_key_go_new_channel(*key)) {
            set_usbcast_key_flag(1);
        } else if (*key == LV_KEY_BACK) {
            set_usbcast_key_flag(1);
            USBCast_StopService();
            USBCast_exit();
            USBCast_init();
            USBCast_StartService();
            USBCastExitShow();
            set_current_ui_group(WiredSP_group);
        }
    }
}

void USBCast_activity_exit(void)
{
    if (!usbcast_page)
        return;

    lv_group_remove_all_objs(usbcast_group);
    lv_obj_del(usbcast_page);
    lv_group_del(usbcast_group);
    usbcast_group = NULL;
    usbcast_page = NULL;
    lv_group_set_default(NULL);
}

void USBCast_activity_init(void)
{
    if (!usbcast_page)
        usbcast_page = lv_obj_create(lv_layer_top());

    if (!usbcast_group)
        usbcast_group = lv_group_create();

    lv_obj_set_size(usbcast_page, lv_pct(100), lv_pct(100));
    lv_obj_add_style(usbcast_page, &lv_pro_res.set_bg_black, 0);
    lv_obj_clear_flag(usbcast_page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(usbcast_page, LV_OPA_TRANSP, 0);
    lv_group_add_obj(usbcast_group, usbcast_page);
    lv_obj_add_event_cb(usbcast_page, usbcast_event_handler, LV_EVENT_KEY, NULL);

    set_current_ui_group(usbcast_group);
}

int USBCastExitShow(void)
{
    if (WiredSP_activity)
        lv_obj_clear_flag(WiredSP_activity, LV_OBJ_FLAG_HIDDEN);
    lv_disp_get_default()->driver->screen_transp = 0;
    lv_disp_set_bg_opa(lv_disp_get_default(), LV_OPA_COVER);
    lv_memset_00(lv_disp_get_default()->driver->draw_buf->buf_act,
                lv_disp_get_default()->driver->draw_buf->size * sizeof(lv_color32_t));

    USBCast_activity_exit();
    return 0;
}

int USBCastEntryShow(void)
{
    USBCast_activity_init();

    if (WiredSP_activity)
        lv_obj_add_flag(WiredSP_activity, LV_OBJ_FLAG_HIDDEN);
    lv_disp_get_default()->driver->screen_transp = 1;
    lv_disp_set_bg_opa(lv_disp_get_default(), LV_OPA_TRANSP);
    lv_memset_00(lv_disp_get_default()->driver->draw_buf->buf_act,
                lv_disp_get_default()->driver->draw_buf->size * sizeof(lv_color32_t));

    set_usbcast_key_flag(0);
    return 0;
}
