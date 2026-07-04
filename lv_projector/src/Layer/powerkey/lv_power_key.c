#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include "lvgl/lvgl.h"
#include "sys_param.h"
#include "lv_string_id.h"
#include "lv_common.h"
#include "lv_system_page.h"
#include "../widget/lv_pro_res.h"

bool powerkey_is_work = false;
static lv_timer_t *powerkey_longpress_timer;
static lv_obj_t *powerkey_mbox;

void check_powerkey_mbox(void)
{
    if (powerkey_mbox) {
        lv_msgbox_close(powerkey_mbox);
        powerkey_mbox = NULL;
        powerkey_is_work = false;
    }
}

static void create_ui_Shutdown_msgbox(void)
{
    lv_obj_t *shutdown_box = lv_obj_create(lv_layer_top());
    lv_obj_set_size(shutdown_box, lv_pct(25), lv_pct(25));
    lv_obj_center(shutdown_box);

    lv_obj_t *label = lv_label_create(shutdown_box);
    lv_obj_set_style_text_font(label, &GENERAL_FONT_MID, 0);
    lv_label_set_recolor(label, true);
    lv_label_set_text_fmt(label, "#0x404040 %s #", lv_get_string(STR_SHOUTDOWN));
    lv_obj_center(label);
}

static void del_powerkey_longpress_time(void)
{
    if (powerkey_longpress_timer) {
        lv_timer_del(powerkey_longpress_timer);
        powerkey_longpress_timer = NULL;
    }
}

static void powerkey_longpress_timer_callback(lv_timer_t *timer)
{
    if (!is_volume_mute())
        lv_set_mute(true);
    create_ui_Shutdown_msgbox();
    system_power_off();
}

static void powerkey_msgbox_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t *key = lv_event_get_param(e);
    lv_obj_t *cur_obj = lv_event_get_current_target(e);
    if (code == LV_EVENT_KEY) {
        if (*key == LV_KEY_ENTER) {
            if(lv_msgbox_get_active_btn(cur_obj) == 0) {
                if (!is_volume_mute())
                    lv_set_mute(true);
                create_ui_Shutdown_msgbox();
                system_power_off();
            }

            del_powerkey_longpress_time();
            recovery_prev_ui_group();
            lv_msgbox_close(powerkey_mbox);
            powerkey_mbox = NULL;
            powerkey_is_work = false;
        } else if (*key == LV_KEY_BACK) {
            del_powerkey_longpress_time();
            recovery_prev_ui_group();
            lv_msgbox_close(powerkey_mbox);
            powerkey_mbox = NULL;
            powerkey_is_work = false;
        } else if (!is_global_key_go_new_channel(*key)) {
            del_powerkey_longpress_time();
            lv_msgbox_close(powerkey_mbox);
            powerkey_mbox = NULL;
            powerkey_is_work = false;
        }
    }
}

void create_ui_powerkey_msgbox(void)
{
    if (powerkey_is_work) {
        if (get_longpress_flag()) {
            lv_timer_resume(powerkey_longpress_timer);
        } else {
            lv_timer_pause(powerkey_longpress_timer);
            lv_timer_reset(powerkey_longpress_timer);
        }
        return;
    }

    static const char *btns[3];
    btns[0] = lv_get_string(STR_RESTORE_OK_1);
    btns[1] = lv_get_string(STR_RESTORE_CLOSE);
    btns[2] = "";

    lv_group_remove_all_objs(mbox_item_group);
    set_current_ui_group(mbox_item_group);
    powerkey_mbox = lv_msgbox_create(NULL, NULL, lv_get_string(STR_ENTER_STANDBY), btns, false);
    lv_obj_set_size(powerkey_mbox, LV_PCT(25), LV_PCT(25));
    lv_obj_add_event_cb(powerkey_mbox, powerkey_msgbox_event_cb, LV_EVENT_KEY, NULL);

    lv_event_send(lv_group_get_focused(mbox_item_group), LV_EVENT_FOCUSED, NULL);
    lv_obj_align(powerkey_mbox, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(powerkey_mbox, lv_color_hex(0xf0f0f0), 0);
    lv_obj_set_flex_align(powerkey_mbox, LV_FLEX_ALIGN_SPACE_AROUND , LV_FLEX_ALIGN_CENTER , LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(powerkey_mbox, 30, 0);
    lv_obj_set_scrollbar_mode(powerkey_mbox, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(powerkey_mbox, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *label = lv_msgbox_get_content(powerkey_mbox);
    lv_obj_set_size(label, LV_PCT(100), LV_PCT(35));
    lv_obj_set_style_text_font(label, &GENERAL_FONT_MID, 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0 , 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(label, lv_color_black(), 0);

    lv_obj_t *btns_obj = lv_msgbox_get_btns(powerkey_mbox);
    lv_obj_set_size(btns_obj, LV_PCT(80), LV_PCT(20));
    lv_obj_set_style_text_font(btns_obj, &GENERAL_FONT_MID, 0);
    lv_obj_set_style_pad_gap(btns_obj, 60, 0);
    lv_obj_set_style_shadow_width(btns_obj, 0, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(btns_obj, 5, LV_PART_ITEMS | LV_STATE_DEFAULT);

    lv_obj_t *bg = lv_obj_get_parent(powerkey_mbox);
    lv_obj_set_style_bg_opa(bg, LV_OPA_70, 0);
    lv_obj_set_style_bg_color(bg, lv_palette_main(LV_PALETTE_GREY), 0);

    powerkey_longpress_timer = lv_timer_create(powerkey_longpress_timer_callback,2500,NULL);
    lv_timer_pause(powerkey_longpress_timer);

    powerkey_is_work = true;
}
