#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "lv_common.h"
#include "api.h"
#include "factory_test.h"
#include "page.h"
#include "lv_string_id.h"
#include "sys_param.h"
#include "lv_system_page.h"
#include "../lv_pro_launcher.h"

factory_info_t *factory_info;


void factory_data_init(void)
{
    int len;

    if (!factory_info) {
        factory_info = (void *)malloc(sizeof(factory_info_t));
    }
    memset(factory_info, 0, sizeof(factory_info_t));

    //private_data_read(FACTORY_KEY_NAME, factory_info, sizeof(factory_info_t), &len);
}

void factory_data_reset(void)
{
    if (factory_info) {
        memset(factory_info, 0, sizeof(factory_info_t));
       // private_data_write(FACTORY_KEY_NAME, factory_info, sizeof(factory_info_t));
    }
}

void factory_data_exit(int update)
{
    if (factory_info) {
        if (update)
            //private_data_write(FACTORY_KEY_NAME, factory_info, sizeof(factory_info_t));

        free(factory_info);
    }
}

int check_factory_status(void)
{
    if (!factory_info)
        factory_data_init();

    if (factory_info->status) {
        switch_page(PAGE_FACTORY_TEST);
    } else {
        factory_data_exit(0);
    }
}

static void factory_reset_msgbox_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t *key = lv_event_get_param(e);
    lv_obj_t *cur_obj = lv_event_get_current_target(e);
    char *user_data = lv_event_get_user_data(e);

    if (code == LV_EVENT_KEY) {
        if (*key == LV_KEY_ENTER) {
            if(lv_msgbox_get_active_btn(cur_obj) == 0){
                factory_data_reset();
            }

            set_current_ui_group(factory_ui_group);
            lv_msgbox_close(cur_obj);
        } else if (*key == LV_KEY_BACK) {
            lv_msgbox_close(cur_obj);
            set_current_ui_group(factory_ui_group);
        }
    }
}

void create_ui_reset_factory_msgbox(void)
{
    lv_obj_t *mbox;

    static const char *btns[3];
    btns[0] = "确认";
    btns[1] = "关闭";
    btns[2] = "";
    lv_group_remove_all_objs(mbox_item_group);
    set_current_ui_group(mbox_item_group);
    mbox = lv_msgbox_create(NULL, "清除数据", "是否清除数据？", btns, false);

    lv_obj_add_event_cb(mbox, factory_reset_msgbox_event_cb, LV_EVENT_KEY, NULL);
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

static void factory_recovery_msgbox_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t *key = lv_event_get_param(e);
    lv_obj_t *cur_obj = lv_event_get_current_target(e);
    char *user_data = lv_event_get_user_data(e);

    if (code == LV_EVENT_KEY) {
        if (*key == LV_KEY_ENTER) {
            if(lv_msgbox_get_active_btn(cur_obj) == 0){
                factory_reset_and_clear_networkfiles();
                factory_reset_setting();
            }
            set_current_ui_group(factory_ui_group);
            lv_msgbox_close(cur_obj);
        } else if (*key == LV_KEY_BACK) {
            lv_msgbox_close(cur_obj);
            set_current_ui_group(factory_ui_group);
        }
    }
}

void create_ui_recovery_factory_msgbox(void)
{
    lv_obj_t *mbox;

    static const char *btns[3];
    btns[0] = "确认";
    btns[1] = "关闭";
    btns[2] = "";
    lv_group_remove_all_objs(mbox_item_group);
    set_current_ui_group(mbox_item_group);
    mbox = lv_msgbox_create(NULL, "恢复出厂设置", "是否恢复出厂设置？", btns, false);

    lv_obj_add_event_cb(mbox, factory_recovery_msgbox_event_cb, LV_EVENT_KEY, NULL);
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