
/*
 * lv_pro_ebook_list_activity.c
 *
 */

#include "lv_pro_ebook_list_activity.h"

#include "../../widget/lv_pro_media_item.h"
#include "../../widget/lv_pro_res.h"
#include "../middle_ware/lv_pro_res_mid_list.h"
#include "../middle_ware/lv_pro_res_ebook_player_int.h"
#include "lv_drivers/indev/evdev.h"
#include "lv_pro_ebook_activity.h"
#include "lvgl/lvgl.h"

static lv_group_t *activity_group;

static lv_obj_t *ebook_info_activity;
static lv_group_t *ebook_info_group;

static lv_obj_t *ebook_list_activity;
static lv_group_t *ebook_list_group;

static lv_obj_t *ebook_item[64];

static void lv_pro_ebook_info_exit(void);
static void lv_pro_ebook_list_exit(void);

static void ebook_info_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    const uint32_t *key_param = lv_event_get_param(e);
    uint32_t key = (key_param != NULL) ? *key_param : 0;

    if (code == LV_EVENT_KEY) {
        if ((key == LV_KEY_ENTER) || (key == LV_KEY_BACK)) {
            lv_pro_ebook_ctrlbar_timer_en(true);
            lv_pro_ebook_info_exit();
            set_current_ui_group(activity_group);
        } else if (!is_global_key_go_new_channel(key)) {
            lv_pro_ebook_info_exit();
            set_current_ui_group(activity_group);
        }
    }
}

static void ebook_list_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    const uint32_t *key_param = lv_event_get_param(e);
    uint32_t key = (key_param != NULL) ? *key_param : 0;
    int ret = 0;
    int cur_item_id = 0;
    lv_obj_t *old_item = NULL, *cur_item = NULL;

    if (code == LV_EVENT_KEY) {
        if (key == LV_KEY_ENTER) {
            list_head_t *my_media_list = lv_pro_ebook_res_get_media_list();
            for (int i = 0; i < list_get_total_num(my_media_list); i++) {
                if (lv_pro_media_item_get_play(ebook_item[i])) {
                    old_item = ebook_item[i];
                }
                if (lv_obj_get_parent(obj) == ebook_item[i]) {
                    cur_item_id = i;
                    cur_item = ebook_item[i];
                }
                lv_pro_media_item_set_play(ebook_item[i], false);
            }
            // lv_pro_ebook_res_play_mode(2, cur_item_id + 1);
            int ret = lv_pro_ebook_res_trySwitch_ebook(2, cur_item_id + 1);
            if (ret == 0) {
                lv_pro_media_item_set_play(cur_item, true);
            } else {
                lv_pro_msgbox_msg_open_on_top(&IDB_Icon_unsupported, lv_get_string(STR_FILE_FAIL),
                                              500);
                lv_pro_media_item_set_play(old_item, true);
            }

        } else if (!is_global_key_go_new_channel(key) || key == LV_KEY_BACK) {
            if (key == LV_KEY_BACK) lv_pro_ebook_ctrlbar_timer_en(true);
            lv_pro_ebook_list_exit();
            set_current_ui_group(activity_group);
        }
    }
}

void lv_pro_ebook_info_init(void)
{
    activity_group = lv_group_get_default();

    ebook_info_activity = lv_obj_create(lv_layer_top());
    lv_obj_set_size(ebook_info_activity, LV_PCT(INFO_WIDTH_PCT), LV_PCT(INFO_HEIGHT_PCT));
    lv_obj_align(ebook_info_activity, LV_ALIGN_TOP_RIGHT, 0, LV_PCT(INFO_ALIGN_PCT));
    lv_obj_set_style_bg_opa(ebook_info_activity, LV_OPA_50, 0);
    lv_obj_add_style(ebook_info_activity, &lv_pro_res.set_bg_black, 0);
    ebook_info_group = lv_group_create();

    lv_obj_t *title_cont = lv_obj_create(ebook_info_activity);
    lv_obj_set_size(title_cont, LV_PCT(100), LV_PCT(17));
    lv_obj_add_style(title_cont, &lv_pro_res.set_bg_transp, 0);
    lv_obj_align(title_cont, LV_ALIGN_TOP_MID, 0, 0);

    list_head_t *my_media_list = lv_pro_ebook_res_get_media_list();
    lv_obj_t *title_label = lv_label_create(title_cont);
    lv_obj_set_style_text_font(title_label, &GENERAL_FONT_MID, 0);
    lv_label_set_recolor(title_label, true);
    lv_label_set_text_fmt(title_label, "#ffffff %s #", my_media_list->current_node->name);
    lv_label_set_long_mode(title_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_align(title_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(title_label, LV_PCT(90));
    lv_obj_center(title_label);

    lv_obj_t *main_cont = lv_obj_create(ebook_info_activity);
    lv_obj_align_to(main_cont, title_cont, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_set_size(main_cont, LV_PCT(100), LV_PCT(66));
    lv_obj_set_style_border_width(main_cont, 0, 0);
    lv_obj_set_style_pad_all(main_cont, 0, 0);
    lv_obj_set_style_pad_row(main_cont, 2, 0);
    lv_obj_set_style_pad_top(main_cont, 5, 0);
    lv_obj_set_style_pad_bottom(main_cont, 5, 0);
    lv_obj_set_style_radius(main_cont, 0, 0);
    lv_obj_set_style_shadow_width(main_cont, 0, 0);
    lv_obj_set_scrollbar_mode(main_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_style(main_cont, &lv_pro_res.set_bg_transp, 0);
    lv_obj_set_flex_flow(main_cont, LV_FLEX_FLOW_COLUMN);

    struct EBookInfo pi;
    memset(&pi, 0, sizeof(struct EBookInfo));
    lv_pro_ebook_res_get_info(&pi);
    lv_pro_media_info_create_text(main_cont, &GENERAL_FONT_MID, lv_get_string(STR_INFO_SIZE),
                                  pi.filesize);

    lv_obj_t *exit_cont = lv_btn_create(ebook_info_activity);
    lv_obj_set_size(exit_cont, LV_PCT(100), LV_PCT(17));
    lv_obj_set_style_border_width(exit_cont, 0, 0);
    lv_obj_set_style_pad_all(exit_cont, 0, 0);
    lv_obj_set_style_radius(exit_cont, 0, 0);
    lv_obj_set_style_shadow_width(exit_cont, 0, 0);
    lv_obj_set_style_bg_color(exit_cont, lv_color_hex(0x0000ff), 0);
    lv_obj_set_style_bg_opa(exit_cont, LV_OPA_50, 0);
    lv_obj_align(exit_cont, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_obj_t *exit_label = lv_label_create(exit_cont);
    lv_obj_set_style_text_font(exit_label, &GENERAL_FONT_MID, 0);
    lv_label_set_recolor(exit_label, true);
    lv_label_set_text_fmt(exit_label, "#ffffff %s #", lv_get_string(STR_INFO_EXIT));
    lv_obj_center(exit_label);

    lv_group_add_obj(ebook_info_group, exit_cont);
    lv_obj_add_event_cb(exit_cont, ebook_info_event_cb, LV_EVENT_KEY, NULL);

    set_current_ui_group(ebook_info_group);
}

static void lv_pro_ebook_info_exit(void)
{
    if (ebook_info_group) {
        lv_group_remove_all_objs(ebook_info_group);
        lv_group_del(ebook_info_group);
        ebook_info_group = NULL;
    }
    if (ebook_info_activity) {
        lv_obj_del(ebook_info_activity);
        ebook_info_activity = NULL;
    }
    lv_group_set_default(NULL);
}

void lv_pro_ebook_list_init(void)
{
    activity_group = lv_group_get_default();

    ebook_list_activity = lv_obj_create(lv_layer_top());
    lv_obj_set_size(ebook_list_activity, LV_PCT(LIST_WIDTH_PCT), LV_PCT(LIST_HEIGHT_PCT));
    lv_obj_align(ebook_list_activity, LV_ALIGN_TOP_RIGHT, 0, LV_PCT(LIST_ALIGN_PCT));
    lv_obj_set_style_bg_opa(ebook_list_activity, LV_OPA_50, 0);
    lv_obj_add_style(ebook_list_activity, &lv_pro_res.set_bg_black, 0);

    ebook_list_group = lv_group_create();

    lv_obj_t *main_cont = lv_obj_create(ebook_list_activity);
    lv_obj_align(main_cont, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_size(main_cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_border_width(main_cont, 0, 0);
    lv_obj_set_style_pad_all(main_cont, 5, 0);
    lv_obj_set_style_pad_row(main_cont, 2, 0);
    lv_obj_set_style_pad_top(main_cont, 5, 0);
    lv_obj_set_style_pad_bottom(main_cont, 5, 0);
    lv_obj_set_style_radius(main_cont, 0, 0);
    lv_obj_set_style_shadow_width(main_cont, 0, 0);
    lv_obj_set_scrollbar_mode(main_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_style(main_cont, &lv_pro_res.set_bg_transp, 0);
    lv_obj_set_flex_flow(main_cont, LV_FLEX_FLOW_COLUMN);

    list_head_t *my_media_list = lv_pro_ebook_res_get_media_list();
    int cur_index = list_get_cur_index(my_media_list);
    list_skip_to_head(my_media_list);

    const char *playing_img = "A:/usr/share/lv_projector/media_playing01.png";

    for (int i = 0; i < list_get_total_num(my_media_list); i++) {
        ebook_item[i] = lv_pro_media_item_create(main_cont);
        lv_obj_set_size(ebook_item[i], LV_PCT(100), LV_PCT(15));
        lv_pro_media_item_set_src(ebook_item[i], &GENERAL_FONT_MID, &lv_pro_res.set_bg_transp,
                                  playing_img, my_media_list->current_node->name);
        lv_obj_add_event_cb(ebook_item[i], ebook_list_event_cb, LV_EVENT_KEY, NULL);
        lv_obj_t *ebook_item_btn = lv_pro_media_item_get_btn(ebook_item[i]);
        lv_group_add_obj(ebook_list_group, ebook_item_btn);
        if (i == cur_index - 1) {
            lv_group_focus_obj(ebook_item_btn);
        }
        list_get_next_node(my_media_list);
    }

    set_current_ui_group(ebook_list_group);

    lv_pro_media_item_set_play(ebook_item[cur_index - 1], true);

    list_skip_to_index(my_media_list, cur_index);
}

static void lv_pro_ebook_list_exit(void)
{
    if (ebook_list_group) {
        lv_group_remove_all_objs(ebook_list_group);
        lv_group_del(ebook_list_group);
        ebook_list_group = NULL;
    }

    if (ebook_list_activity) {
        lv_obj_del(ebook_list_activity);
        ebook_list_activity = NULL;
    }

    lv_group_set_default(NULL);
}
