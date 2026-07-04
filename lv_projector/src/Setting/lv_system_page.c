#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "lv_pro_setting.h"
#include "lv_image_page.h"
#include "lv_keystone_page.h"
#include "lv_sound_page.h"
#include "../widget/lv_pro_res.h"
#include "../widget/lv_pro_setting_item.h"
#include "../lv_pro_launcher.h"
#include "../Common/setting/system_setting.h"
#include "../Common/setting/setting_common.h"
#include "ota/ota_update.h"
#include "lv_string_id.h"
#include "lv_common.h"
#include "sys_param.h"
#include "ksc/lv_ksc.h"
#include "../System/hdmi/hdmi_resolution.h"
#include "lv_drivers/display/sunxifb.h"
#include "lv_pro_launcher.h"
#include "../Media/middle_ware/lv_pro_res_media_player_int.h"
/*
*	system setting
*/
lv_obj_t *sub_system_page;
lv_obj_t *system_obj;
lv_group_t *Setting_system_group;
lv_group_t *mbox_item_group;

lv_group_t *language_item_group;
lv_obj_t *language_cont_obj;
lv_obj_t *language_label;
lv_obj_t *language_title_lable;

lv_obj_t *projection_cont_obj;
lv_obj_t *projection_label;

lv_group_t *homepage_bg_item_group;
lv_obj_t *homepage_bg_cont_obj;
lv_obj_t *homepage_bg_label;
lv_obj_t *homepage_bg_item0;
lv_obj_t *homepage_bg_item1;
lv_obj_t *homepage_bg_item2;
lv_obj_t *homepage_bg_title_lable;

lv_obj_t *ota_cont_obj;

lv_obj_t *reset_setting_cont_obj;

lv_group_t *auto_sleep_item_group;
lv_obj_t *auto_sleep_cont_obj;
lv_obj_t *auto_sleep_label;
lv_obj_t *auto_sleep_title_lable;

lv_group_t *osd_timer_item_group;
lv_obj_t *osd_timer_cont_obj;
lv_obj_t *osd_timer_label;
lv_obj_t *osd_timer_title_lable;

lv_obj_t *sub_language_page;
lv_obj_t *sub_bg_page;
lv_obj_t *sub_auto_sleep_page;
lv_obj_t *sub_osd_timer_page;

lv_obj_t *version_cont_obj;

lv_obj_t *sub_resolution_page;
lv_obj_t *resolution_cont_obj;
lv_obj_t *resolution_label;
lv_group_t *resolution_item_group;
static hdmi_resolution_t *resolution_list = NULL;
static int resolution_count = 0;

void update_system_label(void)
{
    lv_label_set_text(lv_obj_get_child(language_cont_obj, 1), lv_get_string(STR_OSD_LANG));
    lv_label_set_text(lv_obj_get_child(projection_cont_obj, 1), lv_get_string(STR_FLIP));
    lv_label_set_text(lv_obj_get_child(homepage_bg_cont_obj, 0), lv_get_string(STR_HOME_BG));
    lv_label_set_text(lv_obj_get_child(reset_setting_cont_obj, 0), lv_get_string(STR_RESTORE_FACTORY_DEFAULT));
    lv_label_set_text(lv_obj_get_child(ota_cont_obj, 1), lv_get_string(STR_UPGRADE));
    lv_label_set_text(lv_obj_get_child(auto_sleep_cont_obj, 1), lv_get_string(STR_AUTO_SLEEP));
    lv_label_set_text(lv_obj_get_child(osd_timer_cont_obj, 1), lv_get_string(STR_OSD_TIMER));
    lv_label_set_text(lv_obj_get_child(version_cont_obj, 0), lv_get_string(STR_VERSION_INFO));
    
    if (resolution_list && resolution_count > 0) {
        int current_mode = hdmi_get_current_resolution();
        int i;
        for (i = 0; i < resolution_count; i++) {
            if (resolution_list[i].mode == current_mode) {
                lv_label_set_text(lv_obj_get_child(resolution_cont_obj, 1), resolution_list[i].name);
                break;
            }
        }
    }
}

static void resolution_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t *key = lv_event_get_param(e);
    lv_obj_t *target_obj = lv_event_get_target(e);
    int value;

    if (code == LV_EVENT_KEY) {
        if (*key == LV_KEY_ENTER) {
            value = lv_group_get_obj_id(resolution_item_group, target_obj);
            if (value >= 0 && value < resolution_count) {
                printf("select resolution: %s, mode=%d\n", resolution_list[value].name, resolution_list[value].mode);
                lv_label_set_text(resolution_label, resolution_list[value].name);
                hdmi_set_resolution(resolution_list[value].mode);
                lv_pro_media_reset_screen_resolution();
                update_launcher_fresh_layout();
            }

            update_all_obj_for_page(sub_system_page);

            lv_obj_del(sub_resolution_page);
            sub_resolution_page = NULL;
            lv_menu_back_event_cb_ex(target_obj, setting_menu);
            set_current_ui_group(Setting_system_group);
            set_setting_show_for_video();
            update_osd_timer(1);
        } else if (*key == LV_KEY_BACK || !is_global_key_go_new_channel(*key)) {
            lv_obj_del(sub_resolution_page);
            sub_resolution_page = NULL;
            update_osd_timer(1);

            if (*key == LV_KEY_BACK) {
                lv_menu_back_event_cb_ex(target_obj, setting_menu);
                set_current_ui_group(Setting_system_group);
                set_setting_show_for_video();
            } else {
                lv_obj_add_flag(Setting_activity, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

void lv_resolution_init(void)
{
    del_osd_timer();

    if (!sub_resolution_page)
        sub_resolution_page = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(sub_resolution_page);
    lv_obj_set_style_bg_color(sub_resolution_page, COLOR_BLACK, 0);
    lv_obj_set_size(sub_resolution_page, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(sub_resolution_page, LV_OPA_80, 0);
    set_setting_hidden_for_video(sub_resolution_page);

    lv_obj_t *title_cont = lv_obj_create(sub_resolution_page);
    lv_obj_align(title_cont, LV_ALIGN_CENTER, 0, -250);
    lv_obj_set_size(title_cont, 480, 80);
    lv_obj_set_style_border_width(title_cont, 0, 0);
    lv_obj_set_style_pad_all(title_cont, 0, 0);
    lv_obj_set_style_radius(title_cont, 0, 0);
    lv_obj_set_style_shadow_width(title_cont, 0, 0);
    lv_obj_set_style_bg_color(title_cont, lv_color_make(125,125,125), 0);

    lv_obj_t *title_lable = lv_label_create(title_cont);
    lv_obj_set_style_text_font(title_lable, &GENERAL_FONT_MID, 0);
    lv_label_set_recolor(title_lable, true);
    lv_label_set_text_fmt(title_lable, "#ffffff %s #", lv_get_string(STR_INFO_RES));
    lv_obj_center(title_lable);

    lv_obj_t *main_cont = lv_obj_create(sub_resolution_page);
    lv_obj_align_to(main_cont, title_cont, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_set_size(main_cont, 480, 420);
    lv_obj_set_style_border_width(main_cont, 0, 0);
    lv_obj_set_style_pad_all(main_cont, 0, 0);
    lv_obj_set_style_pad_row(main_cont, 2, 0);
    lv_obj_set_style_pad_top(main_cont, 4, 0);
    lv_obj_set_style_pad_bottom(main_cont, 4, 0);
    lv_obj_set_style_radius(main_cont, 0, 0);
    lv_obj_set_style_shadow_width(main_cont, 0, 0);
    lv_obj_set_scrollbar_mode(main_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(main_cont, COLOR_BLACK, 0);
    lv_obj_set_flex_flow(main_cont, LV_FLEX_FLOW_COLUMN);

    hdmi_get_resolutions(&resolution_list, &resolution_count);

    int i;
    for (i = 0; i < resolution_count; i++) {
        lv_obj_t *res_item = lv_pro_setting_item_create(main_cont);
        lv_obj_set_size(res_item, 480, 80);
        lv_pro_setting_item_set_src(res_item, &GENERAL_FONT_MID, &lv_pro_res.set_bg_blue, resolution_list[i].name);
        lv_group_add_obj(resolution_item_group, lv_pro_setting_item_get_btn(res_item));
        lv_obj_add_event_cb(lv_pro_setting_item_get_btn(res_item), resolution_event_cb, LV_EVENT_KEY, "Resolution");
    }
}

void update_all_lable_and_config(void)
{
    update_launcher_label();

    update_setting_lable();
    update_all_obj_for_page(sub_image_page);

    update_system_label();
    update_all_obj_for_page(sub_system_page);

    update_keystone_label();
    update_all_obj_for_page(sub_keystone_page);

    update_all_obj_for_page(sub_sound_page);
}

char *lv_get_projector_version_info(void)
{
    static char version_info_v[32] = {0};
    snprintf(version_info_v, sizeof(version_info_v), "%s%u", VERSION_NAME, lv_get_sys_param(P_VERSION_INFO));
    return version_info_v;
}

static void osd_timer_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t *key = lv_event_get_param(e);
    lv_obj_t *target_obj = lv_event_get_target(e);
    int value;

    if (code == LV_EVENT_KEY) {
        if (*key == LV_KEY_ENTER) {
            value = lv_group_get_obj_id(osd_timer_item_group, target_obj);
            switch (value) {
                case OSD_TIMER_OFF_ID:
                    lv_label_set_text(osd_timer_label, lv_get_string(STR_RESTORE_CLOSE));
                    lv_set_sys_param(P_OSD_TIME, OSD_TIMER_OFF_ID);
                    break;
                case OSD_TIMER_5S_ID:
                    lv_label_set_text(osd_timer_label, lv_get_string(STR_OSD_TIME_5S));
                    lv_set_sys_param(P_OSD_TIME, OSD_TIMER_5S_ID);
                    break;
                case OSD_TIMER_10S_ID:
                    lv_label_set_text(osd_timer_label, lv_get_string(STR_OSD_TIME_10S));
                    lv_set_sys_param(P_OSD_TIME, OSD_TIMER_10S_ID);
                    break;
                case OSD_TIMER_15S_ID:
                    lv_label_set_text(osd_timer_label, lv_get_string(STR_OSD_TIME_15S));
                    lv_set_sys_param(P_OSD_TIME, OSD_TIMER_15S_ID);
                    break;
                case OSD_TIMER_20S_ID:
                    lv_label_set_text(osd_timer_label, lv_get_string(STR_OSD_TIME_20S));
                    lv_set_sys_param(P_OSD_TIME, OSD_TIMER_20S_ID);
                    break;
                case OSD_TIMER_25S_ID:
                    lv_label_set_text(osd_timer_label, lv_get_string(STR_OSD_TIME_25S));
                    lv_set_sys_param(P_OSD_TIME, OSD_TIMER_25S_ID);
                    break;
                case OSD_TIMER_30S_ID:
                    lv_label_set_text(osd_timer_label, lv_get_string(STR_OSD_TIME_30S));
                    lv_set_sys_param(P_OSD_TIME, OSD_TIMER_30S_ID);
                    break;
                default:
                    value = OSD_TIMER_10S_ID;
                    lv_label_set_text(osd_timer_label, lv_get_string(STR_OSD_TIME_10S));
                    lv_set_sys_param(P_OSD_TIME, OSD_TIMER_10S_ID);
                    break;
            }
            set_osd_timer(value);

            lv_obj_del(sub_osd_timer_page);
            sub_osd_timer_page = NULL;
            lv_menu_back_event_cb_ex(target_obj, setting_menu);
            set_current_ui_group(Setting_system_group);
            set_setting_show_for_video();
        } else if (*key == LV_KEY_BACK || !is_global_key_go_new_channel(*key)) {
            lv_obj_del(sub_osd_timer_page);
            sub_osd_timer_page = NULL;

            if (*key == LV_KEY_BACK) {
                lv_menu_back_event_cb_ex(target_obj, setting_menu);
                set_current_ui_group(Setting_system_group);
                set_setting_show_for_video();
            } else {
                lv_obj_add_flag(Setting_activity, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

static void auto_sleep_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t *key = lv_event_get_param(e);
    lv_obj_t *target_obj = lv_event_get_target(e);
    int value;

    if (code == LV_EVENT_KEY) {
        if (*key == LV_KEY_ENTER) {
            value = lv_group_get_obj_id(auto_sleep_item_group, target_obj);
            switch (value) {
                case AUTO_SLEEP_OFF_ID:
                    lv_label_set_text(auto_sleep_label, lv_get_string(STR_RESTORE_CLOSE));
                    lv_set_sys_param(P_AUTOSLEEP, AUTO_SLEEP_OFF_ID);
                    break;
                case AUTO_SLEEP_10M_ID:
                    lv_label_set_text(auto_sleep_label, lv_get_string(STR_AUTO_SLEEP_10M));
                    lv_set_sys_param(P_AUTOSLEEP, AUTO_SLEEP_10M_ID);
                    break;
                case AUTO_SLEEP_30M_ID:
                    lv_label_set_text(auto_sleep_label, lv_get_string(STR_AUTO_SLEEP_30M));
                    lv_set_sys_param(P_AUTOSLEEP, AUTO_SLEEP_30M_ID);
                    break;
                case AUTO_SLEEP_60M_ID:
                    lv_label_set_text(auto_sleep_label, lv_get_string(STR_AUTO_SLEEP_1H));
                    lv_set_sys_param(P_AUTOSLEEP, AUTO_SLEEP_60M_ID);
                    break;
                case AUTO_SLEEP_90M_ID:
                    lv_label_set_text(auto_sleep_label, lv_get_string(STR_AUTO_SLEEP_90M));
                    lv_set_sys_param(P_AUTOSLEEP, AUTO_SLEEP_120M_ID);
                    break;
                case AUTO_SLEEP_120M_ID:
                    lv_label_set_text(auto_sleep_label, lv_get_string(STR_AUTO_SLEEP_2H));
                    lv_set_sys_param(P_AUTOSLEEP, AUTO_SLEEP_120M_ID);
                    break;
                default:
                    value = AUTO_SLEEP_60M_ID;
                    lv_label_set_text(auto_sleep_label, lv_get_string(STR_AUTO_SLEEP_1H));
                    lv_set_sys_param(P_AUTOSLEEP, AUTO_SLEEP_60M_ID);
                    break;
            }
            set_system_auto_sleep(value);

            lv_obj_del(sub_auto_sleep_page);
            sub_auto_sleep_page = NULL;
            lv_menu_back_event_cb_ex(target_obj, setting_menu);
            set_current_ui_group(Setting_system_group);
            set_setting_show_for_video();
        } else if (*key == LV_KEY_BACK || !is_global_key_go_new_channel(*key)) {
            lv_obj_del(sub_auto_sleep_page);
            sub_auto_sleep_page = NULL;

            if (*key == LV_KEY_BACK) {
                lv_menu_back_event_cb_ex(target_obj, setting_menu);
                set_current_ui_group(Setting_system_group);
                set_setting_show_for_video();
            } else {
                lv_obj_add_flag(Setting_activity, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

static void language_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t *key = lv_event_get_param(e);
    lv_obj_t *target_obj = lv_event_get_target(e);
    int value;

    if (code == LV_EVENT_KEY) {
        if (*key == LV_KEY_ENTER) {
            value = lv_group_get_obj_id(language_item_group, target_obj);
            if (value >= LANGUAGE_ENGLISH && value < LANGUAGE_MAX) {
                lv_set_sys_param(P_OSD_LANGUAGE, value);
                lv_label_set_text(language_label, lv_get_string(STR_LANG));
                lv_set_sys_param(P_OSD_LANGUAGE, value);
                update_all_lable_and_config();
            }

            lv_obj_del(sub_language_page);
            sub_language_page = NULL;
            lv_menu_back_event_cb_ex(target_obj, setting_menu);
            set_current_ui_group(Setting_system_group);
            set_setting_show_for_video();
        } else if (*key == LV_KEY_BACK || !is_global_key_go_new_channel(*key)) {
            lv_obj_del(sub_language_page);
            sub_language_page = NULL;

            if (*key == LV_KEY_BACK ) {
                lv_menu_back_event_cb_ex(target_obj, setting_menu);
                set_current_ui_group(Setting_system_group);
                set_setting_show_for_video();
            } else {
                lv_obj_add_flag(Setting_activity, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

static void backgroud_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t *key = lv_event_get_param(e);
    lv_obj_t *target_obj = lv_event_get_target(e);
    int value;

    if (code == LV_EVENT_KEY) {
        if (*key == LV_KEY_ENTER) {
            value = lv_group_get_obj_id(homepage_bg_item_group, target_obj);
            switch (value) {
                case BG_COLOR_0_ID:
                    lv_label_set_text(homepage_bg_label, lv_get_string(STR_COLOR_DARK_BLUE));
                    lv_obj_set_style_bg_color(launcher_activity, lv_color_hex(BG_COLOR_0_RGB), 0);
                    lv_set_sys_param(P_COLOR_BG, BG_COLOR_0_ID);
                    break;
                case BG_COLOR_1_ID:
                    lv_label_set_text(homepage_bg_label, lv_get_string(STR_COLOR_BLUE));
                    lv_obj_set_style_bg_color(launcher_activity, lv_color_hex(BG_COLOR_1_RGB), 0);
                    lv_set_sys_param(P_COLOR_BG, BG_COLOR_1_ID);
                    break;
                case BG_COLOR_2_ID:
                    lv_label_set_text(homepage_bg_label, lv_get_string(STR_COLOR_GREY));
                    lv_obj_set_style_bg_color(launcher_activity, lv_color_hex(BG_COLOR_2_RGB), 0);
                    lv_set_sys_param(P_COLOR_BG, BG_COLOR_2_ID);
                    break;
                default:
                    break;
            }
            lv_obj_del(sub_bg_page);
            sub_bg_page = NULL;
            lv_menu_back_event_cb_ex(target_obj, setting_menu);
            set_current_ui_group(Setting_system_group);
            set_setting_show_for_video();
        } else if (*key == LV_KEY_BACK || !is_global_key_go_new_channel(*key)) {
            lv_obj_del(sub_bg_page);
            sub_bg_page = NULL;

            if (*key == LV_KEY_BACK ) {
                lv_menu_back_event_cb_ex(target_obj, setting_menu);
                set_current_ui_group(Setting_system_group);
                set_setting_show_for_video();
            } else {
                lv_obj_add_flag(Setting_activity, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

static void reset_msgbox_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t *key = lv_event_get_param(e);
    lv_obj_t *cur_obj = lv_event_get_current_target(e);
    char *user_data = lv_event_get_user_data(e);

    if (code == LV_EVENT_KEY) {
        if (*key == LV_KEY_ENTER) {
            if(lv_msgbox_get_active_btn(cur_obj) == 0){
                lv_set_sys_param(P_FLIP_MODE, factory_get_projection_mode(1));
                factory_reset_and_clear_networkfiles();
                factory_reset_setting();
            }
            set_current_ui_group(Setting_system_group);
            lv_msgbox_close(cur_obj);
        } else if (*key == LV_KEY_BACK || !is_global_key_go_new_channel(*key)) {
            lv_msgbox_close(cur_obj);

            if (*key == LV_KEY_BACK)
                set_current_ui_group(Setting_system_group);
            else
                lv_obj_add_flag(Setting_activity, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void create_ui_reset_setting_msgbox(void)
{
    lv_obj_t *mbox;

    static const char *btns[3];
    btns[0] = lv_get_string(STR_RESTORE_OK_1);
    btns[1] = lv_get_string(STR_RESTORE_CLOSE);
    btns[2] = "";
    lv_group_remove_all_objs(mbox_item_group);
    set_current_ui_group(mbox_item_group);
    mbox = lv_msgbox_create(NULL, lv_get_string(STR_RESTORE_FACTORY_DEFAULT), lv_get_string(STR_FACTORY_RESET_CONTENT), btns, false);

    lv_obj_add_event_cb(mbox, reset_msgbox_event_cb, LV_EVENT_KEY, NULL);
    lv_event_send(lv_group_get_focused(mbox_item_group), LV_EVENT_FOCUSED, evdev_indev);

    lv_obj_set_size(mbox, LV_PCT(35), LV_PCT(35));
    lv_obj_align(mbox, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(mbox, lv_color_hex(0xf0f0f0), 0);
    lv_obj_set_flex_align(mbox, LV_FLEX_ALIGN_SPACE_AROUND , LV_FLEX_ALIGN_CENTER , LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(mbox, 30, 0);
    lv_obj_set_scrollbar_mode(mbox, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(mbox, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * title = lv_msgbox_get_title(mbox);
    lv_obj_set_size(title, LV_PCT(100), LV_PCT(20));
    lv_obj_set_style_text_font(title, &GENERAL_FONT_MID, 0);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t *label = lv_msgbox_get_content(mbox);
    lv_obj_set_size(label, LV_PCT(100), LV_PCT(35));
    lv_obj_set_style_text_font(label, &GENERAL_FONT_MID, 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0 , 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(label, lv_color_black(), 0);

    lv_obj_t *btns_obj = lv_msgbox_get_btns(mbox);
    lv_obj_set_size(btns_obj, LV_PCT(80), LV_PCT(20));
    lv_obj_set_style_text_font(btns_obj, &GENERAL_FONT_MID, 0);
    lv_obj_set_style_pad_gap(btns_obj, 60, 0);
    lv_obj_set_style_shadow_width(btns_obj, 0, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(btns_obj, 5, LV_PART_ITEMS | LV_STATE_DEFAULT);

    lv_obj_t *bg = lv_obj_get_parent(mbox);
    lv_obj_set_style_bg_opa(bg, LV_OPA_70, 0);
    lv_obj_set_style_bg_color(bg, lv_palette_main(LV_PALETTE_GREY), 0);
}

void update_home_background(void)
{
    switch (lv_get_sys_param(P_COLOR_BG)) {
        case BG_COLOR_0_ID:
            lv_label_set_text(homepage_bg_label, lv_get_string(STR_COLOR_DARK_BLUE));
            lv_obj_set_style_bg_color(launcher_activity, lv_color_hex(BG_COLOR_0_RGB), 0);
            break;
        case BG_COLOR_1_ID:
            lv_label_set_text(homepage_bg_label, lv_get_string(STR_COLOR_BLUE));
            lv_obj_set_style_bg_color(launcher_activity, lv_color_hex(BG_COLOR_1_RGB), 0);
            break;
        case BG_COLOR_2_ID:
            lv_label_set_text(homepage_bg_label, lv_get_string(STR_COLOR_GREY));
            lv_obj_set_style_bg_color(launcher_activity, lv_color_hex(BG_COLOR_2_RGB), 0);
            break;
        default:
            break;
    }
}

static void set_projection_mode(void)
{
#if ENABLE_KSC
    char *text;
    int value = lv_get_sys_param(P_FLIP_MODE);
    value = value + 1;
    switch (value) {
        case PROJECTION_FRONT_ID:
            text = lv_get_string(STR_FRONT_TABLE);
            break;
        case PROJECTION_CEILING_FRONT_ID:
            text = lv_get_string(STR_FRONT_CEILING);
            break;
        case PROJECTION_REAR_ID:
            text = lv_get_string(STR_BACK_TABLE);
            break;
        case PROJECTION_REAR_CEILING_ID:
            text = lv_get_string(STR_BACK_CEILING);
            break;
        default:
            value = PROJECTION_FRONT_ID;
            text = lv_get_string(STR_FRONT_TABLE);
            break;
    }
    lv_set_sys_param(P_FLIP_MODE, value);
    lv_label_set_text(projection_label, text);
    set_keystone_mirror(value);
#endif
}

void update_auto_sleep(int on)
{
    int value = lv_get_sys_param(P_AUTOSLEEP);

    switch (value) {
        case AUTO_SLEEP_OFF_ID:
            lv_label_set_text(auto_sleep_label, lv_get_string(STR_RESTORE_CLOSE));
            break;
        case AUTO_SLEEP_10M_ID:
            lv_label_set_text(auto_sleep_label, lv_get_string(STR_AUTO_SLEEP_10M));
            break;
        case AUTO_SLEEP_30M_ID:
            lv_label_set_text(auto_sleep_label, lv_get_string(STR_AUTO_SLEEP_30M));
            break;
        case AUTO_SLEEP_60M_ID:
            lv_label_set_text(auto_sleep_label, lv_get_string(STR_AUTO_SLEEP_1H));
            break;
        case AUTO_SLEEP_90M_ID:
            lv_label_set_text(auto_sleep_label, lv_get_string(STR_AUTO_SLEEP_90M));
            break;
        case AUTO_SLEEP_120M_ID:
            lv_label_set_text(auto_sleep_label, lv_get_string(STR_AUTO_SLEEP_2H));
            break;
        default:
            lv_label_set_text(auto_sleep_label, lv_get_string(STR_AUTO_SLEEP_1H));
            break;
    }
    if (on)
        set_system_auto_sleep(value);
}

void update_osd_timer(int on)
{
    int value = lv_get_sys_param(P_OSD_TIME);

    switch (value) {
        case OSD_TIMER_OFF_ID:
            lv_label_set_text(osd_timer_label, lv_get_string(STR_RESTORE_CLOSE));
            break;
        case OSD_TIMER_5S_ID:
            lv_label_set_text(osd_timer_label, lv_get_string(STR_OSD_TIME_5S));
            break;
        case OSD_TIMER_10S_ID:
            lv_label_set_text(osd_timer_label, lv_get_string(STR_OSD_TIME_10S));
            break;
        case OSD_TIMER_15S_ID:
            lv_label_set_text(osd_timer_label, lv_get_string(STR_OSD_TIME_15S));
            break;
         case OSD_TIMER_20S_ID:
            lv_label_set_text(osd_timer_label, lv_get_string(STR_OSD_TIME_20S));
            break;
        case OSD_TIMER_25S_ID:
            lv_label_set_text(osd_timer_label, lv_get_string(STR_OSD_TIME_25S));
            break;
        case OSD_TIMER_30S_ID:
            lv_label_set_text(osd_timer_label, lv_get_string(STR_OSD_TIME_30S));
            break;
        default:
            value = OSD_TIMER_10S_ID;
            lv_label_set_text(osd_timer_label, lv_get_string(STR_OSD_TIME_10S));
            break;
    }
    if (on)
        set_osd_timer(value);
}

void update_projection_mode(void)
{
    int value = lv_get_sys_param(P_FLIP_MODE);
    char *text;
    switch (value) {
        case PROJECTION_FRONT_ID:
            text = lv_get_string(STR_FRONT_TABLE);
            break;
        case PROJECTION_CEILING_FRONT_ID:
            text = lv_get_string(STR_FRONT_CEILING);
            break;
        case PROJECTION_REAR_ID:
            text = lv_get_string(STR_BACK_TABLE);
            break;
        case PROJECTION_REAR_CEILING_ID:
            text = lv_get_string(STR_BACK_CEILING);
            break;
        default:
            value = PROJECTION_FRONT_ID;
            text = lv_get_string(STR_FRONT_TABLE);
            break;
    }
    lv_label_set_text(projection_label, text);

}

static void system_page_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t *key = lv_event_get_param(e);
    struct _lv_obj_t * target_obj = lv_event_get_target(e);

    if (code == LV_EVENT_KEY) {
        if (*key == LV_KEY_ENTER) {
            if (target_obj == language_cont_obj) {
                lv_language_init();
                set_current_ui_group(language_item_group);
                lv_group_focus_obj_for_id(lv_group_get_head_obj_ex(language_item_group), lv_get_sys_param(P_OSD_LANGUAGE), true);
            } else if (target_obj == projection_cont_obj) {
                set_projection_mode();
            } else if (target_obj == homepage_bg_cont_obj) {
                lv_backgroud_init();
                set_current_ui_group(homepage_bg_item_group);
                lv_group_focus_obj_for_id(lv_group_get_head_obj_ex(homepage_bg_item_group), lv_get_sys_param(P_COLOR_BG), true);
            } else if (target_obj == reset_setting_cont_obj) {
                create_ui_reset_setting_msgbox();
            } else if (target_obj == ota_cont_obj) {
                del_osd_timer();
                if (!check_local_ota_file()) {
                    create_ui_ota_msgbox(LOCAL_OTA);
                } else  {
                    create_ui_ota_msgbox(LOCAL_OTA_NO_USB);
                }
            } else if (target_obj == auto_sleep_cont_obj) {
                lv_auto_sleep_init();
                set_current_ui_group(auto_sleep_item_group);
                lv_group_focus_obj_for_id(lv_group_get_head_obj_ex(auto_sleep_item_group), lv_get_sys_param(P_AUTOSLEEP), true);
            } else if (target_obj == osd_timer_cont_obj) {
                lv_osd_timer_init();
                set_current_ui_group(osd_timer_item_group);
                lv_group_focus_obj_for_id(lv_group_get_head_obj_ex(osd_timer_item_group), lv_get_sys_param(P_OSD_TIME), true);
            } else if (target_obj == resolution_cont_obj) {
                lv_resolution_init();
                set_current_ui_group(resolution_item_group);
            } else if (target_obj == version_cont_obj) {
                if (!check_factor_test())
                    switch_page(PAGE_FACTORY_TEST);
            }
        } else if (*key == LV_KEY_BACK || *key == LV_KEY_LEFT) {
            set_current_ui_group(Setting_group);
        }
    }
}


void lv_language_init(void)
{
    lv_obj_t * item;

    /* language page */
    if (!sub_language_page)
        sub_language_page = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(sub_language_page);
    lv_obj_set_style_bg_color(sub_language_page, COLOR_BLACK, 0);
    lv_obj_set_size(sub_language_page, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(sub_language_page, LV_OPA_80, 0);
    set_setting_hidden_for_video(sub_language_page);

    lv_obj_t * title_cont = lv_obj_create(sub_language_page);
    lv_obj_align(title_cont, LV_ALIGN_CENTER, 0, -250);
    lv_obj_set_size(title_cont, 480, 80);
    lv_obj_set_style_border_width(title_cont, 0, 0);
    lv_obj_set_style_pad_all(title_cont, 0, 0);
    lv_obj_set_style_radius(title_cont, 0, 0);
    lv_obj_set_style_shadow_width(title_cont, 0, 0);
    lv_obj_set_style_bg_color(title_cont, lv_color_hex(0x0000A0), 0);

    language_title_lable = lv_label_create(title_cont);
    lv_obj_set_style_text_font(language_title_lable, &GENERAL_FONT_MID, 0);
    lv_obj_add_style(language_title_lable, &lv_pro_res.label_white, 0);
    lv_label_set_recolor(language_title_lable, true);
    lv_label_set_text_fmt(language_title_lable, "#ffffff %s #", lv_get_string(STR_OSD_LANG_TITLE));
    lv_obj_center(language_title_lable);

    lv_obj_t * main_cont = lv_obj_create(sub_language_page);
    lv_obj_align_to(main_cont, title_cont, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_set_size(main_cont, 480, 420);
    lv_obj_set_style_border_width(main_cont, 0, 0);
    lv_obj_set_style_pad_all(main_cont, 0, 0);
    lv_obj_set_style_pad_row(main_cont, 2, 0);
    lv_obj_set_style_pad_top(main_cont, 4, 0);
    lv_obj_set_style_pad_bottom(main_cont, 4, 0);
    lv_obj_set_style_radius(main_cont, 0, 0);
    lv_obj_set_style_shadow_width(main_cont, 0, 0);
    lv_obj_set_scrollbar_mode(main_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(main_cont, COLOR_BLACK, 0);
    lv_obj_set_flex_flow(main_cont, LV_FLEX_FLOW_COLUMN);

    char *language_list[8];
    language_list[0] = lv_get_string_by_langid(LANGUAGE_ENGLISH, STR_LANG);
    language_list[1] = lv_get_string_by_langid(LANGUAGE_CHINESE, STR_LANG);
    language_list[2] = lv_get_string_by_langid(LANGUAGE_FRENCH, STR_LANG);
    language_list[3] = lv_get_string_by_langid(LANGUAGE_GERMAN, STR_LANG);
    language_list[4] = lv_get_string_by_langid(LANGUAGE_JAPANESE, STR_LANG);
    language_list[5] = lv_get_string_by_langid(LANGUAGE_ITALIAN, STR_LANG);
    language_list[6] = lv_get_string_by_langid(LANGUAGE_SPANISH, STR_LANG);
    language_list[7] = lv_get_string_by_langid(LANGUAGE_RUSSIAN, STR_LANG);
    for (int i = 0; i < 8; i++) {
        item = lv_pro_setting_item_create(main_cont);
        lv_obj_set_size(item, 480, 80);
        lv_pro_setting_item_set_src(item, &GENERAL_FONT_MID, &lv_pro_res.set_bg_blue, language_list[i]);
        lv_group_add_obj(language_item_group, lv_pro_setting_item_get_btn(item));
        lv_obj_add_event_cb(lv_pro_setting_item_get_btn(item), language_event_cb, LV_EVENT_KEY, NULL);
    }
}

void lv_backgroud_init(void)
{
    /* backgroud page */
    if (!sub_bg_page)
        sub_bg_page = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(sub_bg_page);
    lv_obj_set_style_bg_color(sub_bg_page, COLOR_BLACK, 0);
    lv_obj_set_size(sub_bg_page, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(sub_bg_page, LV_OPA_80, 0);
    set_setting_hidden_for_video(sub_bg_page);

    lv_obj_t * title_cont = lv_obj_create(sub_bg_page);
    lv_obj_align(title_cont, LV_ALIGN_CENTER, 0, -250);
    lv_obj_set_size(title_cont, 480, 80);
    lv_obj_set_style_border_width(title_cont, 0, 0);
    lv_obj_set_style_pad_all(title_cont, 0, 0);
    lv_obj_set_style_radius(title_cont, 0, 0);
    lv_obj_set_style_shadow_width(title_cont, 0, 0);
    lv_obj_set_style_bg_color(title_cont, lv_color_hex(0x0000A0), 0);

    homepage_bg_title_lable = lv_label_create(title_cont);
    lv_obj_set_style_text_font(homepage_bg_title_lable, &GENERAL_FONT_MID, 0);
    lv_obj_add_style(homepage_bg_title_lable, &lv_pro_res.label_white, 0);
    lv_label_set_recolor(homepage_bg_title_lable, true);
    lv_label_set_text_fmt(homepage_bg_title_lable, "#ffffff %s #", lv_get_string(STR_HOME_BG));
    lv_obj_center(homepage_bg_title_lable);

    lv_obj_t * main_cont = lv_obj_create(sub_bg_page);
    lv_obj_align_to(main_cont, title_cont, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_set_size(main_cont, 480, 420);
    lv_obj_set_style_border_width(main_cont, 0, 0);
    lv_obj_set_style_pad_all(main_cont, 0, 0);
    lv_obj_set_style_pad_row(main_cont, 2, 0);
    lv_obj_set_style_pad_top(main_cont, 4, 0);
    lv_obj_set_style_pad_bottom(main_cont, 4, 0);
    lv_obj_set_style_radius(main_cont, 0, 0);
    lv_obj_set_style_shadow_width(main_cont, 0, 0);
    lv_obj_set_scrollbar_mode(main_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(main_cont, COLOR_BLACK, 0);
    lv_obj_set_flex_flow(main_cont, LV_FLEX_FLOW_COLUMN);

    homepage_bg_item0 = lv_pro_setting_item_create(main_cont);
    lv_obj_set_size(homepage_bg_item0, 480, 80);
    lv_pro_setting_item_set_src(homepage_bg_item0, &GENERAL_FONT_MID, &lv_pro_res.set_bg_blue, lv_get_string(STR_COLOR_DARK_BLUE));
    lv_group_add_obj(homepage_bg_item_group, lv_pro_setting_item_get_btn(homepage_bg_item0));
    lv_obj_add_event_cb(lv_pro_setting_item_get_btn(homepage_bg_item0), backgroud_event_cb, LV_EVENT_KEY, BG_COLOR_0_NAME);

    homepage_bg_item1 = lv_pro_setting_item_create(main_cont);
    lv_obj_set_size(homepage_bg_item1, 480, 80);
    lv_pro_setting_item_set_src(homepage_bg_item1, &GENERAL_FONT_MID, &lv_pro_res.set_bg_blue, lv_get_string(STR_COLOR_BLUE));
    lv_group_add_obj(homepage_bg_item_group, lv_pro_setting_item_get_btn(homepage_bg_item1));
    lv_obj_add_event_cb(lv_pro_setting_item_get_btn(homepage_bg_item1), backgroud_event_cb, LV_EVENT_KEY, BG_COLOR_1_NAME);

    homepage_bg_item2 = lv_pro_setting_item_create(main_cont);
    lv_obj_set_size(homepage_bg_item2, 480, 80);
    lv_pro_setting_item_set_src(homepage_bg_item2, &GENERAL_FONT_MID, &lv_pro_res.set_bg_blue, lv_get_string(STR_COLOR_GREY));
    lv_group_add_obj(homepage_bg_item_group, lv_pro_setting_item_get_btn(homepage_bg_item2));
    lv_obj_add_event_cb(lv_pro_setting_item_get_btn(homepage_bg_item2), backgroud_event_cb, LV_EVENT_KEY, BG_COLOR_2_NAME);
}

void lv_auto_sleep_init(void)
{
    lv_obj_t * item;

    /* Auto sleep page */
    if (!sub_auto_sleep_page)
        sub_auto_sleep_page = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(sub_auto_sleep_page);
    lv_obj_set_style_bg_color(sub_auto_sleep_page, COLOR_BLACK, 0);
    lv_obj_set_size(sub_auto_sleep_page, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(sub_auto_sleep_page, LV_OPA_80, 0);
    set_setting_hidden_for_video(sub_auto_sleep_page);

    lv_obj_t * title_cont = lv_obj_create(sub_auto_sleep_page);
    lv_obj_align(title_cont, LV_ALIGN_CENTER, 0, -250);
    lv_obj_set_size(title_cont, 480, 80);
    lv_obj_set_style_border_width(title_cont, 0, 0);
    lv_obj_set_style_pad_all(title_cont, 0, 0);
    lv_obj_set_style_radius(title_cont, 0, 0);
    lv_obj_set_style_shadow_width(title_cont, 0, 0);
    lv_obj_set_style_bg_color(title_cont, lv_color_hex(0x0000A0), 0);

    auto_sleep_title_lable = lv_label_create(title_cont);
    lv_obj_set_style_text_font(auto_sleep_title_lable, &GENERAL_FONT_MID, 0);
    lv_obj_add_style(auto_sleep_title_lable, &lv_pro_res.label_white, 0);
    lv_label_set_recolor(auto_sleep_title_lable, true);
    lv_label_set_text_fmt(auto_sleep_title_lable, "#ffffff %s #", lv_get_string(STR_AUTO_SLEEP));
    lv_obj_center(auto_sleep_title_lable);

    lv_obj_t * main_cont = lv_obj_create(sub_auto_sleep_page);
    lv_obj_align_to(main_cont, title_cont, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_set_size(main_cont, 480, 420);
    lv_obj_set_style_border_width(main_cont, 0, 0);
    lv_obj_set_style_pad_all(main_cont, 0, 0);
    lv_obj_set_style_pad_row(main_cont, 2, 0);
    lv_obj_set_style_pad_top(main_cont, 4, 0);
    lv_obj_set_style_pad_bottom(main_cont, 4, 0);
    lv_obj_set_style_radius(main_cont, 0, 0);
    lv_obj_set_style_shadow_width(main_cont, 0, 0);
    lv_obj_set_scrollbar_mode(main_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(main_cont, COLOR_BLACK, 0);
    lv_obj_set_flex_flow(main_cont, LV_FLEX_FLOW_COLUMN);

    char *auto_sleep_list[6];
    auto_sleep_list[0] = lv_get_string(STR_RESTORE_CLOSE);
    auto_sleep_list[1] = lv_get_string(STR_AUTO_SLEEP_10M);
    auto_sleep_list[2] = lv_get_string(STR_AUTO_SLEEP_30M);
    auto_sleep_list[3] = lv_get_string(STR_AUTO_SLEEP_1H);
    auto_sleep_list[4] = lv_get_string(STR_AUTO_SLEEP_90M);
    auto_sleep_list[5] = lv_get_string(STR_AUTO_SLEEP_2H);
    for (int i = 0; i < sizeof(auto_sleep_list) / sizeof(char *); i++) {
        item = lv_pro_setting_item_create(main_cont);
        lv_obj_set_size(item, 480, 80);
        lv_pro_setting_item_set_src(item, &GENERAL_FONT_MID, &lv_pro_res.set_bg_blue, auto_sleep_list[i]);
        lv_group_add_obj(auto_sleep_item_group, lv_pro_setting_item_get_btn(item));
        lv_obj_add_event_cb(lv_pro_setting_item_get_btn(item), auto_sleep_event_cb, LV_EVENT_KEY, NULL);
    }
}

void lv_osd_timer_init(void)
{
    lv_obj_t * item;

    /* osd timer page */
    if (!sub_osd_timer_page)
        sub_osd_timer_page = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(sub_osd_timer_page);
    lv_obj_set_style_bg_color(sub_osd_timer_page, COLOR_BLACK, 0);
    lv_obj_set_size(sub_osd_timer_page, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(sub_osd_timer_page, LV_OPA_80, 0);
    set_setting_hidden_for_video(sub_osd_timer_page);

    lv_obj_t * title_cont = lv_obj_create(sub_osd_timer_page);
    lv_obj_align(title_cont, LV_ALIGN_CENTER, 0, -250);
    lv_obj_set_size(title_cont, 480, 80);
    lv_obj_set_style_border_width(title_cont, 0, 0);
    lv_obj_set_style_pad_all(title_cont, 0, 0);
    lv_obj_set_style_radius(title_cont, 0, 0);
    lv_obj_set_style_shadow_width(title_cont, 0, 0);
    lv_obj_set_style_bg_color(title_cont, lv_color_hex(0x0000A0), 0);

    osd_timer_title_lable = lv_label_create(title_cont);
    lv_obj_set_style_text_font(osd_timer_title_lable, &GENERAL_FONT_MID, 0);
    lv_obj_add_style(osd_timer_title_lable, &lv_pro_res.label_white, 0);
    lv_label_set_recolor(osd_timer_title_lable, true);
    lv_label_set_text_fmt(osd_timer_title_lable, "#ffffff %s #", lv_get_string(STR_OSD_TIMER));
    lv_obj_center(osd_timer_title_lable);

    lv_obj_t * main_cont = lv_obj_create(sub_osd_timer_page);
    lv_obj_align_to(main_cont, title_cont, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_set_size(main_cont, 480, 420);
    lv_obj_set_style_border_width(main_cont, 0, 0);
    lv_obj_set_style_pad_all(main_cont, 0, 0);
    lv_obj_set_style_pad_row(main_cont, 2, 0);
    lv_obj_set_style_pad_top(main_cont, 4, 0);
    lv_obj_set_style_pad_bottom(main_cont, 4, 0);
    lv_obj_set_style_radius(main_cont, 0, 0);
    lv_obj_set_style_shadow_width(main_cont, 0, 0);
    lv_obj_set_scrollbar_mode(main_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(main_cont, COLOR_BLACK, 0);
    lv_obj_set_flex_flow(main_cont, LV_FLEX_FLOW_COLUMN);

    char *osd_timer_list[7];
    osd_timer_list[0] = lv_get_string(STR_RESTORE_CLOSE);
    osd_timer_list[1] = lv_get_string(STR_OSD_TIME_5S);
    osd_timer_list[2] = lv_get_string(STR_OSD_TIME_10S);
    osd_timer_list[3] = lv_get_string(STR_OSD_TIME_15S);
    osd_timer_list[4] = lv_get_string(STR_OSD_TIME_20S);
    osd_timer_list[5] = lv_get_string(STR_OSD_TIME_25S);
    osd_timer_list[6] = lv_get_string(STR_OSD_TIME_30S);
    for (int i = 0; i < sizeof(osd_timer_list) / sizeof(char *); i++) {
        item = lv_pro_setting_item_create(main_cont);
        lv_obj_set_size(item, 480, 80);
        lv_pro_setting_item_set_src(item, &GENERAL_FONT_MID, &lv_pro_res.set_bg_blue, osd_timer_list[i]);
        lv_group_add_obj(osd_timer_item_group, lv_pro_setting_item_get_btn(item));
        lv_obj_add_event_cb(lv_pro_setting_item_get_btn(item), osd_timer_event_cb, LV_EVENT_KEY, NULL);
    }
}


void lv_system_page_init(void)
{
    lv_obj_t * section;

    language_item_group = lv_group_create();
    homepage_bg_item_group  = lv_group_create();
    auto_sleep_item_group = lv_group_create();
    osd_timer_item_group = lv_group_create();
    resolution_item_group = lv_group_create();

    /* system */
    sub_system_page = lv_menu_page_create(setting_menu, NULL);
    lv_obj_set_scrollbar_mode(sub_system_page, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_hor(sub_system_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(setting_menu), 0), 0);
    lv_menu_separator_create(sub_system_page);
    section = lv_menu_section_create(sub_system_page);
    lv_obj_set_style_bg_color(section, lv_color_hex(0x031FFF), 0);
    lv_obj_set_style_pad_all(section, 10, 0);
    lv_obj_set_style_pad_row(section, 30, 0);
    lv_obj_set_size(section, lv_pct(100), lv_pct(95));

    language_cont_obj = create_text(section, NULL, lv_get_string(STR_OSD_LANG), lv_get_string(STR_LANG));
    language_label = lv_obj_get_child(language_cont_obj, 1);
    lv_obj_add_flag(language_cont_obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    set_setting_outline_style(language_cont_obj);
    lv_group_add_obj(Setting_system_group, language_cont_obj);
    lv_obj_add_event_cb(language_cont_obj, system_page_event_handler, LV_EVENT_KEY, NULL);

    projection_cont_obj = create_text(section, NULL, lv_get_string(STR_FLIP), lv_get_string(STR_FRONT_TABLE));
    projection_label = lv_obj_get_child(projection_cont_obj, 1);
    lv_obj_add_flag(projection_cont_obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    set_setting_outline_style(projection_cont_obj);
    lv_group_add_obj(Setting_system_group, projection_cont_obj);
    lv_obj_add_event_cb(projection_cont_obj, system_page_event_handler, LV_EVENT_KEY, NULL);

    homepage_bg_cont_obj = create_text(section, NULL, lv_get_string(STR_HOME_BG), "Black");
    homepage_bg_label = lv_obj_get_child(homepage_bg_cont_obj, 1);
    lv_obj_add_flag(homepage_bg_cont_obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    set_setting_outline_style(homepage_bg_cont_obj);
    lv_group_add_obj(Setting_system_group, homepage_bg_cont_obj);
    lv_obj_add_event_cb(homepage_bg_cont_obj, system_page_event_handler, LV_EVENT_KEY, NULL);

    reset_setting_cont_obj = create_text(section, NULL, lv_get_string(STR_RESTORE_FACTORY_DEFAULT), NULL);
    lv_obj_set_style_text_font(lv_obj_get_child(reset_setting_cont_obj, 0), &GENERAL_FONT_MID, 0);
    lv_obj_add_flag(reset_setting_cont_obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    set_setting_outline_style(reset_setting_cont_obj);
    lv_group_add_obj(Setting_system_group, reset_setting_cont_obj);
    lv_obj_add_event_cb(reset_setting_cont_obj, system_page_event_handler, LV_EVENT_KEY, NULL);

    ota_cont_obj = create_text(section, NULL, lv_get_string(STR_UPGRADE), NULL);
    lv_obj_add_flag(ota_cont_obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    set_setting_outline_style(ota_cont_obj);
    lv_group_add_obj(Setting_system_group, ota_cont_obj);
    lv_obj_add_event_cb(ota_cont_obj, system_page_event_handler, LV_EVENT_KEY, NULL);

    auto_sleep_cont_obj = create_text(section, NULL, lv_get_string(STR_AUTO_SLEEP), "60m");
    auto_sleep_label = lv_obj_get_child(auto_sleep_cont_obj, 1);
    lv_obj_add_flag(auto_sleep_cont_obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    set_setting_outline_style(auto_sleep_cont_obj);
    lv_group_add_obj(Setting_system_group, auto_sleep_cont_obj);
    lv_obj_add_event_cb(auto_sleep_cont_obj, system_page_event_handler, LV_EVENT_KEY, NULL);

    osd_timer_cont_obj = create_text(section, NULL, lv_get_string(STR_OSD_TIMER), "10s");
    osd_timer_label = lv_obj_get_child(osd_timer_cont_obj, 1);
    lv_obj_add_flag(osd_timer_cont_obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    set_setting_outline_style(osd_timer_cont_obj);
    lv_group_add_obj(Setting_system_group, osd_timer_cont_obj);
    lv_obj_add_event_cb(osd_timer_cont_obj, system_page_event_handler, LV_EVENT_KEY, NULL);

    resolution_cont_obj = create_text(section, NULL, lv_get_string(STR_INFO_RES), "1080p@60Hz");
    resolution_label = lv_obj_get_child(resolution_cont_obj, 1);
    lv_obj_add_flag(resolution_cont_obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    set_setting_outline_style(resolution_cont_obj);
    lv_group_add_obj(Setting_system_group, resolution_cont_obj);
    lv_obj_add_event_cb(resolution_cont_obj, system_page_event_handler, LV_EVENT_KEY, NULL);
    hdmi_set_resolution_change_callback(sunxifb_refresh_resolution);

    hdmi_resolution_init();
    hdmi_hotplug_start();
    hdmi_get_resolutions(&resolution_list, &resolution_count);
    {
        int saved_mode = hdmi_load_saved_resolution();
        int i;
        for (i = 0; i < resolution_count; i++) {
            if (resolution_list[i].mode == saved_mode) {
                lv_label_set_text(resolution_label, resolution_list[i].name);
                break;
            }
        }
    }

    version_cont_obj = create_text(section, NULL, lv_get_string(STR_VERSION_INFO), lv_get_projector_version_info());
    lv_obj_add_flag(version_cont_obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    set_setting_outline_style(version_cont_obj);
    lv_group_add_obj(Setting_system_group, version_cont_obj);
    lv_obj_add_event_cb(version_cont_obj, system_page_event_handler, LV_EVENT_KEY, NULL);
}

void lv_system_page_exit(void)
{
    if (sub_osd_timer_page) {
        lv_obj_del(sub_osd_timer_page);
        sub_osd_timer_page = NULL;
    }
    if (sub_auto_sleep_page) {
        lv_obj_del(sub_auto_sleep_page);
        sub_auto_sleep_page = NULL;
    }
    if (sub_language_page) {
        lv_obj_del(sub_language_page);
        sub_language_page = NULL;
    }
    if (sub_bg_page) {
        lv_obj_del(sub_bg_page);
        sub_bg_page = NULL;
    }
    if (sub_resolution_page) {
        lv_obj_del(sub_resolution_page);
        sub_resolution_page = NULL;
    }
    
    if (resolution_list) {
        hdmi_free_resolutions(resolution_list);
        resolution_list = NULL;
        resolution_count = 0;
    }
    
    hdmi_resolution_deinit();
}
