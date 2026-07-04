/*
 * lv_pro_music_list_activity.c
 *
 */

#include "lv_pro_music_list_activity.h"
#include "lv_pro_music_activity.h"
#include "../../widget/lv_pro_res.h"
#include "../../widget/lv_pro_media_item.h"
#include "../middle_ware/lv_pro_res_media_player_int.h"
#include "lv_drivers/indev/evdev.h"

lv_obj_t *music_info_activity;
lv_group_t *music_info_group;

static lv_obj_t *music_item[64];
lv_obj_t *music_list_activity;
lv_group_t *music_list_group;


static void music_info_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t *key = lv_event_get_param(e);

    if (code == LV_EVENT_KEY) {
        if ((*key == LV_KEY_ENTER) || (*key == LV_KEY_BACK)) {
            lv_pro_music_ctrlbar_timer_en(true);
            lv_pro_music_info_exit();
            set_current_ui_group(music_group);
        }
    }
}

static void music_list_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    char *user_data = (char*) lv_event_get_user_data(e);
    uint32_t *key = lv_event_get_param(e);

    if (code == LV_EVENT_KEY) {
        if (*key == LV_KEY_ENTER) {
            if (!strcmp(user_data, "Item")) {
                list_head_t *my_media_list = lv_pro_res_media_get_media_list();
                for (int i = 0; i < list_get_total_num(my_media_list); i++) {
                    if (lv_pro_media_item_get_play(music_item[i])) {
                        lv_pro_media_item_set_play(music_item[i], false);
                    }
                    if (lv_obj_get_parent(obj) == music_item[i]) {
                        lv_pro_media_item_set_play(lv_obj_get_parent(obj), true);
                        lv_pro_media_msg_enqueue(MSG_TYPE_MEDIA_PLAY_LIST, i + 1, false);
                    }
                }
            }
        } else if (*key == LV_KEY_BACK) {
            lv_pro_music_ctrlbar_timer_en(true);
            lv_pro_music_list_exit();
            set_current_ui_group(music_group);
        }
    }
}

void lv_pro_music_info_init(void) {
    music_info_activity = lv_obj_create(lv_layer_top());
    lv_obj_set_size(music_info_activity, LV_PCT(INFO_WIDTH_PCT), LV_PCT(INFO_HEIGHT_PCT));
    lv_obj_align(music_info_activity,  LV_ALIGN_TOP_RIGHT, 0, LV_PCT(INFO_ALIGN_PCT));
    lv_obj_set_style_bg_opa(music_info_activity, LV_OPA_50, 0);
    lv_obj_add_style(music_info_activity, &lv_pro_res.set_bg_black, 0);
    music_info_group = lv_group_create();

    lv_obj_t *title_cont = lv_obj_create(music_info_activity);
    lv_obj_set_size(title_cont, LV_PCT(100), LV_PCT(17));
    lv_obj_add_style(title_cont, &lv_pro_res.set_bg_transp, 0);
    lv_obj_align(title_cont, LV_ALIGN_TOP_MID, 0, 0);

    list_head_t *my_media_list = lv_pro_res_media_get_media_list();
    lv_obj_t *title_label = lv_label_create(title_cont);
    lv_obj_set_style_text_font(title_label, &GENERAL_FONT_MID, 0);
    lv_label_set_recolor(title_label, true);
    lv_label_set_text_fmt(title_label, "%s", my_media_list->current_node->name);
    lv_obj_set_style_text_color(title_label, lv_color_white(), 0);
    lv_label_set_long_mode(title_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_align(title_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(title_label, LV_PCT(90));
    lv_obj_center(title_label);

    lv_obj_t *main_cont = lv_obj_create(music_info_activity);
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

    struct MusicInfo mi;
    memset(&mi, 0, sizeof(struct MusicInfo));
    lv_pro_res_music_get_info(&mi);
    lv_pro_media_info_create_text(main_cont, &GENERAL_FONT_MID, lv_get_string(STR_INFO_ALBUM), mi.album);
    lv_pro_media_info_create_text(main_cont, &GENERAL_FONT_MID, lv_get_string(STR_INFO_ARTIST), mi.artist);
    lv_pro_media_info_create_text(main_cont, &GENERAL_FONT_MID, lv_get_string(STR_INFO_SIZE), mi.filesize);

    lv_obj_t *exit_cont = lv_btn_create(music_info_activity);
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

    lv_group_add_obj(music_info_group, exit_cont);
    lv_obj_add_event_cb(exit_cont, music_info_event_cb, LV_EVENT_KEY, NULL);
}

void lv_pro_music_info_exit(void) {
    if (music_info_group) {
        lv_group_remove_all_objs(music_info_group);
        lv_group_del(music_info_group);
        music_info_group = NULL;
        lv_group_set_default(NULL);
    }
    if (music_info_activity) {
        lv_obj_del(music_info_activity);
        music_info_activity = NULL;
    }
}

void lv_pro_music_refresh_info(void) {
    if (music_info_activity) {
        lv_obj_t *title_label = lv_obj_get_child(lv_obj_get_child(music_info_activity, 0), 0);
        list_head_t *my_media_list = lv_pro_res_media_get_media_list();
        lv_label_set_text_fmt(title_label, "%s", my_media_list->current_node->name);
        lv_obj_t *cont0 = lv_obj_get_child(lv_obj_get_child(music_info_activity, 1), 0);
        lv_obj_t *cont1 = lv_obj_get_child(lv_obj_get_child(music_info_activity, 1), 1);
        lv_obj_t *cont2 = lv_obj_get_child(lv_obj_get_child(music_info_activity, 1), 2);
        struct MusicInfo mi;
        memset(&mi, 0, sizeof(struct MusicInfo));
        lv_pro_res_music_get_info(&mi);
        lv_label_set_text(lv_obj_get_child(cont0, 1), mi.album);
        lv_label_set_text(lv_obj_get_child(cont1, 1), mi.artist);
        lv_label_set_text(lv_obj_get_child(cont2, 1), mi.filesize);
    }
}


void lv_pro_music_list_init(void) {
    music_list_activity = lv_obj_create(lv_layer_top());
    lv_obj_set_size(music_list_activity, LV_PCT(LIST_WIDTH_PCT), LV_PCT(LIST_HEIGHT_PCT));
    lv_obj_align(music_list_activity, LV_ALIGN_TOP_RIGHT, 0, LV_PCT(LIST_ALIGN_PCT));
    lv_obj_set_style_bg_opa(music_list_activity, LV_OPA_50, 0);
    lv_obj_add_style(music_list_activity, &lv_pro_res.set_bg_black, 0);
    music_list_group = lv_group_create();

    lv_obj_t *main_cont = lv_obj_create(music_list_activity);
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

    list_head_t *my_media_list = lv_pro_res_media_get_media_list();
    int cur_index = list_get_cur_index(my_media_list);
    list_skip_to_head(my_media_list);

    const char *playing_img = "A:/usr/share/lv_projector/media_playing01.png";

    for (int i = 0; i < list_get_total_num(my_media_list); i++) {
        music_item[i] = lv_pro_media_item_create(main_cont);
        lv_obj_set_size(music_item[i], LV_PCT(100), LV_PCT(15));
        lv_pro_media_item_set_src(music_item[i], &GENERAL_FONT_MID,
                &lv_pro_res.set_bg_transp, playing_img,
                my_media_list->current_node->name);
        lv_obj_add_event_cb(music_item[i], music_list_event_cb, LV_EVENT_KEY, "Item");
        lv_obj_t *music_item_btn = lv_pro_media_item_get_btn(music_item[i]);
        lv_group_add_obj(music_list_group, music_item_btn);
        if (i == cur_index - 1) {
            lv_group_focus_obj(music_item_btn);
        }
        list_get_next_node(my_media_list);
    }

    lv_pro_media_item_set_play(music_item[cur_index - 1], true);

    list_skip_to_index(my_media_list, cur_index);
}

void lv_pro_music_list_exit(void) {
    if (music_list_group) {
        lv_group_remove_all_objs(music_list_group);
        lv_group_del(music_list_group);
        music_list_group = NULL;
        lv_group_set_default(NULL);
    }
    if (music_list_activity) {
        lv_obj_del(music_list_activity);
        music_list_activity = NULL;
    }
}

void lv_pro_music_refresh_list(void) {
    if (music_list_activity) {
        list_head_t *my_media_list = lv_pro_res_media_get_media_list();
        int cur_index = list_get_cur_index(my_media_list);
        int pre_index = cur_index == 1 ? list_get_total_num(my_media_list) : cur_index-1;
        lv_pro_media_item_set_play(music_item[pre_index - 1], false);
        lv_pro_media_item_set_play(music_item[cur_index - 1], true);
    }
}
