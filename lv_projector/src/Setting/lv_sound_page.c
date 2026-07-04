#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "lv_pro_setting.h"
#include "lv_sound_page.h"
#include "../widget/lv_pro_res.h"
#include "../widget/lv_pro_setting_item.h"
#include "../lv_pro_launcher.h"
#include "../Common/setting/sound_setting.h"
#include "../Common/setting/setting_common.h"
#include "../Common/language/string/lv_string_id.h"
#include "lv_common.h"
#include "sys_param.h"

#include "audio/audio_api.h"

/*
*	system setting
*/
lv_obj_t *sub_sound_page;
lv_obj_t *sound_obj;
lv_group_t *Setting_sound_group;

lv_obj_t *sub_sound_mode_page;
lv_group_t *sound_mode_item_group;
lv_obj_t *sound_mode_cont_obj;
lv_obj_t *sound_mode_label;
lv_obj_t *sound_mode_title_lable;

lv_obj_t *sub_bass_page;
lv_obj_t *bass_cont_obj;
lv_obj_t *bass_label;
lv_obj_t *bass_slider_label;
lv_obj_t *bass_slider_bar;

lv_obj_t *sub_treble_page;
lv_obj_t *treble_cont_obj;
lv_obj_t *treble_label;
lv_obj_t *treble_slider_label;
lv_obj_t *treble_slider_bar;


lv_obj_t *sub_sound_output_page;
lv_group_t *sound_output_item_group;
lv_obj_t *sound_output_cont_obj;
lv_obj_t *sound_output_label;
lv_obj_t *sound_output_title_lable;


void update_sound_label(void)
{
    int value;
    int str_value[32];
    lv_label_set_text(lv_obj_get_child(sound_mode_cont_obj, 0), lv_get_string(STR_SOUND_MODE));
    lv_label_set_text(lv_obj_get_child(sound_output_cont_obj, 0), lv_get_string(STR_SOUND_OUTPUT));
    lv_label_set_text(lv_obj_get_child(bass_cont_obj, 0), lv_get_string(STR_BASS));
    lv_label_set_text(lv_obj_get_child(treble_cont_obj, 0), lv_get_string(STR_TREBLE));

    value = lv_get_sys_param(P_SOUND_MODE);
    if (value == SOUND_MODE_STANDARD)
        lv_label_set_text(sound_mode_label, lv_get_string(STR_STANDARD));
    else if (value == SOUND_MODE_MUSIC)
        lv_label_set_text(sound_mode_label, lv_get_string(STR_MUSIC));
    else if (value == SOUND_MODE_MOVIC)
        lv_label_set_text(sound_mode_label, lv_get_string(STR_MOVIE));
    else if (value == SOUND_MODE_SPORTS)
        lv_label_set_text(sound_mode_label, lv_get_string(STR_SPORTS));
    else if (value == SOUND_MODE_USER)
        lv_label_set_text(sound_mode_label, lv_get_string(STR_USER));

    value = lv_get_sys_param(P_SOUND_OUT_MODE);
    if (value == SOUND_OUTPUT_SPEAKER)
        lv_label_set_text(sound_output_label, lv_get_string(STR_SOUND_SPEAKER));
    else if (value == SOUND_OUTPUT_ARC)
        lv_label_set_text(sound_output_label, lv_get_string(STR_SOUND_ARC));
    else if (value == SOUND_OUTPUT_BT)
        lv_label_set_text(sound_output_label, lv_get_string(STR_BT_NAME));
    else if (value == SOUND_OUTPUT_HEADPHONE)
        lv_label_set_text(sound_output_label, lv_get_string(STR_SOUND_HEADPHONE));
    else if (value == SOUND_OUTPUT_OWA)
        lv_label_set_text(sound_output_label, lv_get_string(STR_SOUND_OWA));

    value = lv_get_sys_param(P_SOUND_BASS);
    memset(str_value, 0, sizeof(str_value));
    sprintf(str_value, "%d", value);
    lv_label_set_text(bass_label, str_value);

    value = lv_get_sys_param(P_SOUND_TREBLE);
    memset(str_value, 0, sizeof(str_value));
    sprintf(str_value, "%d", value);
    lv_label_set_text(treble_label, str_value);
}


static void set_slider_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * slider = lv_event_get_target(e);
    char buf[10];
    lv_snprintf(buf, sizeof(buf), "%d", (int)lv_slider_get_value(slider));

    if (code == LV_EVENT_VALUE_CHANGED) {
        if (slider == bass_slider_bar) {
            lv_label_set_text(bass_slider_label, buf);
            lv_label_set_text(bass_label, buf);
        } else if (slider == treble_slider_bar) {
            lv_label_set_text(treble_slider_label, buf);
            lv_label_set_text(treble_label, buf);
        }
    }
}

void sound_create_slider_value(lv_obj_t * obj, lv_obj_t ** slider_obj, lv_obj_t ** value_obj,
        const char * txt, int32_t min, int32_t max, int32_t val)
{
    char buf[4];
    lv_snprintf(buf, sizeof(buf), "%d", val);

    *slider_obj = lv_slider_create(obj);
    lv_obj_set_size(*slider_obj, 50, 500);
    lv_slider_set_range(*slider_obj, min, max);
    lv_slider_set_value(*slider_obj, val, LV_ANIM_OFF);
    lv_obj_align(*slider_obj, LV_ALIGN_RIGHT_MID, -100, 0);
    lv_obj_set_style_bg_color(*slider_obj, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_color(*slider_obj, lv_color_hex(0x00ffff), LV_PART_INDICATOR);

    *value_obj = lv_label_create(*slider_obj);
    lv_label_set_text(*value_obj, buf);
    lv_label_set_long_mode(*value_obj, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_font(*value_obj, &GENERAL_FONT_BIG, 0);
    lv_obj_set_style_text_color(*value_obj, lv_color_hex(0xffffff), 0);
    lv_obj_center(*value_obj);

    lv_obj_add_event_cb(*slider_obj, set_slider_event_cb, LV_EVENT_VALUE_CHANGED, txt);
}

static int set_sound_slider_value(struct _lv_obj_t *sliber_label, int key)
{
    struct _lv_obj_t *text;
    struct _lv_obj_t *sliber_bar;
    sys_param_id id;
    int min, max;
    int value;
    char value_str[32] = {0};

    if (sliber_label == bass_slider_label)
    {
       text = bass_label;
       sliber_bar = bass_slider_bar;
       id = P_SOUND_BASS;
    } else if (sliber_label == treble_slider_label) {
       text = treble_label;
       sliber_bar = treble_slider_bar;
       id = P_SOUND_TREBLE;
    } else
        return -1;
    min = lv_bar_get_min_value(sliber_bar);
    max = lv_bar_get_max_value(sliber_bar);
    value = lv_get_sys_param(id);

    //printf("min %d, max %d, value %d\n", min, max, value);
    if (key == LV_KEY_UP) {
        value = value + 1;

    } else if (LV_KEY_DOWN) {
        value = value - 1;
    }
    if (value < min || value > max)
        return -1;

    sprintf(value_str, "%d", value);
    lv_label_set_text(text, value_str);
    lv_label_set_text(sliber_label, value_str);
    lv_slider_set_value(sliber_bar, value, LV_ANIM_OFF);

    lv_set_sys_param(id, value);
    //printf("%s value = %d\n", __func__, value);
    if (sliber_label == bass_slider_label)
    {
        if (audio_set_eq_bass_gain(value) < 0) {
            printf("audio_set_eq_bass_gain failed\n");
        }
    } else if (sliber_label == treble_slider_label) {
        if (audio_set_eq_treble_gain(value) < 0) {
            printf("audio_set_eq_treble_gain failed\n");
        }
    }
    
    return 0;
}

static void sound_slider_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t *key = lv_event_get_param(e);
    struct _lv_obj_t *cur_obj = lv_event_get_target(e);

    if (code == LV_EVENT_KEY) {
        if (*key == LV_KEY_BACK || !is_global_key_go_new_channel(*key)) {
            if (cur_obj == bass_slider_label) {
                lv_obj_del(sub_bass_page);
                sub_bass_page = NULL;
                bass_slider_label = NULL;
                bass_slider_bar = NULL;
            } else if (cur_obj == treble_slider_label) {
                lv_obj_del(sub_treble_page);
                sub_treble_page = NULL;
                treble_slider_label = NULL;
                treble_slider_bar = NULL;
            }

            if (*key == LV_KEY_BACK) {
                lv_menu_back_event_cb_ex(cur_obj, setting_menu);
                set_current_ui_group(Setting_sound_group);
                set_setting_show_for_video();
            } else {
                lv_obj_add_flag(Setting_activity, LV_OBJ_FLAG_HIDDEN);
            }
        } else if (*key == LV_KEY_UP || *key == LV_KEY_DOWN) {
            if (cur_obj == bass_slider_label || cur_obj == treble_slider_label) {
                set_sound_slider_value(cur_obj, *key);
            }
        }
    }
}

static void lv_sound_bass_init(int32_t val)
{
    if (!sub_bass_page)
        sub_bass_page = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(sub_bass_page);
    lv_obj_set_style_bg_color(sub_bass_page, COLOR_BLUE, 0);
    lv_obj_set_size(sub_bass_page, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(sub_bass_page, LV_OPA_30, 0);
    set_setting_hidden_for_video(sub_bass_page);

    sound_create_slider_value(sub_bass_page, &bass_slider_bar, &bass_slider_label, NULL,
            -10, 10, val);

    lv_obj_add_event_cb(bass_slider_label, sound_slider_event_cb, LV_EVENT_KEY, NULL);
}

static void lv_sound_treble_init(int32_t val)
{
    if (!sub_treble_page)
        sub_treble_page = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(sub_treble_page);
    lv_obj_set_style_bg_color(sub_treble_page, COLOR_BLUE, 0);
    lv_obj_set_size(sub_treble_page, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(sub_treble_page, LV_OPA_30, 0);
    set_setting_hidden_for_video(sub_treble_page);

    create_slider_value(sub_treble_page, &treble_slider_bar, &treble_slider_label, NULL,
            -10, 10, val);

    lv_obj_add_event_cb(treble_slider_label, sound_slider_event_cb, LV_EVENT_KEY, NULL);
}

static void sound_output_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t *key = lv_event_get_param(e);
    lv_obj_t *target_obj = lv_event_get_target(e);
    int value;
    printf("%s code = %d key = %d", __func__, code, *key);

    if (code == LV_EVENT_KEY) {
        if (*key == LV_KEY_ENTER) {
            value = lv_group_get_obj_id(sound_output_item_group, target_obj);
            switch (value) {
                case SOUND_OUTPUT_SPEAKER:
                    lv_set_sys_param(P_SOUND_OUT_MODE, value);
                    lv_label_set_text(sound_output_label, lv_get_string(STR_SOUND_SPEAKER));
                    break;
                case SOUND_OUTPUT_ARC:
                    lv_set_sys_param(P_SOUND_OUT_MODE, value);
                    lv_label_set_text(sound_output_label, lv_get_string(STR_SOUND_ARC));
#if ENABLE_ARC
#if ENABLE_HDMIRX
                    hdmirx_app_cec_set_arc_enable(1);
#endif
#endif
                    break;
                case SOUND_OUTPUT_BT:
                    lv_set_sys_param(P_SOUND_OUT_MODE, value);
                    lv_label_set_text(sound_output_label, lv_get_string(STR_BT_NAME));
                    break;
                case SOUND_OUTPUT_HEADPHONE:
                    lv_set_sys_param(P_SOUND_OUT_MODE, value);
                    lv_label_set_text(sound_output_label, lv_get_string(STR_SOUND_HEADPHONE));
                    break;
                case SOUND_OUTPUT_OWA:
                    lv_set_sys_param(P_SOUND_OUT_MODE, value);
                    lv_label_set_text(sound_output_label, lv_get_string(STR_SOUND_OWA));
#if ENABLE_ARC
#if ENABLE_HDMIRX
		    hdmirx_app_cec_set_arc_enable(0);
#endif
#endif
                    break;
                default:
                    break;
            }
            if (audio_set_output_device(value) < 0) {
                printf("audio_set_output_device %d failed\n", value);
            } else {
                printf("audio_set_output_device succeed!\n");
            }
            lv_obj_del(sub_sound_output_page);
            sub_sound_output_page = NULL;
            lv_menu_back_event_cb_ex(target_obj, setting_menu);
            set_current_ui_group(Setting_sound_group);
            set_setting_show_for_video();
        } else if (*key == LV_KEY_BACK || !is_global_key_go_new_channel(*key)) {
            lv_obj_del(sub_sound_output_page);
            sub_sound_output_page = NULL;

            if (*key == LV_KEY_BACK) {
                lv_menu_back_event_cb_ex(target_obj, setting_menu);
                set_current_ui_group(Setting_sound_group);
                set_setting_show_for_video();
            } else {
                lv_obj_add_flag(Setting_activity, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

static void lv_sound_output_init(void)
{
    lv_obj_t * item;

    if (!sub_sound_output_page)
        sub_sound_output_page = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(sub_sound_output_page);
    lv_obj_set_style_bg_color(sub_sound_output_page, COLOR_BLACK, 0);
    lv_obj_set_size(sub_sound_output_page, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(sub_sound_output_page, LV_OPA_80, 0);
    set_setting_hidden_for_video(sub_sound_output_page);

    lv_obj_t * title_cont = lv_obj_create(sub_sound_output_page);
    lv_obj_align(title_cont, LV_ALIGN_CENTER, 0, -250);
    lv_obj_set_size(title_cont, 480, 80);
    lv_obj_set_style_border_width(title_cont, 0, 0);
    lv_obj_set_style_pad_all(title_cont, 0, 0);
    lv_obj_set_style_radius(title_cont, 0, 0);
    lv_obj_set_style_shadow_width(title_cont, 0, 0);
    lv_obj_set_style_bg_color(title_cont, lv_color_make(125,125,125), 0);

    sound_output_title_lable = lv_label_create(title_cont);
    lv_obj_set_style_text_font(sound_output_title_lable, &GENERAL_FONT_MID, 0);
    lv_obj_add_style(sound_output_title_lable, &lv_pro_res.label_white, 0);
    lv_label_set_recolor(sound_output_title_lable, true);
    lv_label_set_text_fmt(sound_output_title_lable, "#ffffff %s #", lv_get_string(STR_SOUND_OUTPUT));
    lv_obj_center(sound_output_title_lable);

    lv_obj_t * main_cont = lv_obj_create(sub_sound_output_page);
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

    char *sound_output_list[6];
    sound_output_list[0] = lv_get_string(STR_SOUND_SPEAKER);
    sound_output_list[1] = lv_get_string(STR_SOUND_ARC);
    sound_output_list[2] = lv_get_string(STR_BT_NAME);
    sound_output_list[3] = lv_get_string(STR_SOUND_HEADPHONE);
    sound_output_list[4] = lv_get_string(STR_SOUND_OWA);
    for (int i = 0; i < 5; i++) {
        item = lv_pro_setting_item_create(main_cont);
        lv_obj_set_size(item, 480, 80);
        lv_pro_setting_item_set_src(item, &GENERAL_FONT_MID, &lv_pro_res.set_bg_blue, sound_output_list[i]);
        lv_group_add_obj(sound_output_item_group, lv_pro_setting_item_get_btn(item));
        lv_obj_add_event_cb(lv_pro_setting_item_get_btn(item), sound_output_event_cb, LV_EVENT_KEY, NULL);
    }
}

static void sound_mode_only_user_can_modify(int mode)
{
    if (mode != SOUND_MODE_USER) {
        lv_obj_add_state(bass_cont_obj, LV_STATE_DISABLED);
        lv_obj_set_style_text_color(lv_obj_get_child(bass_cont_obj, 0), lv_color_make(125,125,125), 0);
        lv_obj_set_style_text_color(bass_label, lv_color_make(125,125,125), 0);

        lv_obj_add_state(treble_cont_obj, LV_STATE_DISABLED);
        lv_obj_set_style_text_color(lv_obj_get_child(treble_cont_obj, 0), lv_color_make(125,125,125), 0);
        lv_obj_set_style_text_color(treble_label, lv_color_make(125,125,125), 0);
    } else {
        lv_obj_clear_state(bass_cont_obj, LV_STATE_DISABLED);
        lv_obj_set_style_text_color(lv_obj_get_child(bass_cont_obj, 0), lv_color_white(), 0);
        lv_obj_set_style_text_color(bass_label, lv_color_white(), 0);

        lv_obj_clear_state(treble_cont_obj, LV_STATE_DISABLED);
        lv_obj_set_style_text_color(lv_obj_get_child(treble_cont_obj, 0), lv_color_white(), 0);
        lv_obj_set_style_text_color(treble_label, lv_color_white(), 0);
    }
}

static void sound_mode_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t *key = lv_event_get_param(e);
    lv_obj_t *target_obj = lv_event_get_target(e);
    int value;

    if (code == LV_EVENT_KEY) {
        if (*key == LV_KEY_ENTER) {
            value = lv_group_get_obj_id(sound_mode_item_group, target_obj);
            switch (value) {
                case SOUND_MODE_STANDARD:
                    lv_set_sys_param(P_SOUND_MODE, value);
                    lv_label_set_text(sound_mode_label, lv_get_string(STR_STANDARD));
                    break;
                case SOUND_MODE_MUSIC:
                    lv_set_sys_param(P_SOUND_MODE, value);
                    lv_label_set_text(sound_mode_label, lv_get_string(STR_MUSIC));
                    break;
                case SOUND_MODE_MOVIC:
                    lv_set_sys_param(P_SOUND_MODE, value);
                    lv_label_set_text(sound_mode_label, lv_get_string(STR_MOVIE));
                    break;
                case SOUND_MODE_SPORTS:
                    lv_set_sys_param(P_SOUND_MODE, value);
                    lv_label_set_text(sound_mode_label, lv_get_string(STR_SPORTS));
                    break;
                case SOUND_MODE_USER:
                    lv_set_sys_param(P_SOUND_MODE, value);
                    lv_label_set_text(sound_mode_label, lv_get_string(STR_USER));
                    break;
                default:
                    lv_set_sys_param(P_SOUND_MODE, value);
                    lv_label_set_text(sound_mode_label, lv_get_string(STR_STANDARD));
                    value = SOUND_MODE_STANDARD;
                    break;
            }
            sound_mode_only_user_can_modify(value);
            if (audio_set_eq_mode(value) < 0) {
                printf("audio_set_eq_mode %d failed\n", value);
            }
            update_audio_param();
            update_all_obj_for_page(sub_sound_page);

            lv_obj_del(sub_sound_mode_page);
            sub_sound_mode_page = NULL;
            lv_menu_back_event_cb_ex(target_obj, setting_menu);
            set_current_ui_group(Setting_sound_group);
            set_setting_show_for_video();
        } else if (*key == LV_KEY_BACK || !is_global_key_go_new_channel(*key)) {
            lv_obj_del(sub_sound_mode_page);
            sub_sound_mode_page = NULL;

            if (*key == LV_KEY_BACK) {
                lv_menu_back_event_cb_ex(target_obj, setting_menu);
                set_current_ui_group(Setting_sound_group);
                set_setting_show_for_video();
            } else {
                lv_obj_add_flag(Setting_activity, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

static void lv_sound_mode_init(void)
{
    lv_obj_t * item;

    if (!sub_sound_mode_page)
        sub_sound_mode_page = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(sub_sound_mode_page);
    lv_obj_set_style_bg_color(sub_sound_mode_page, COLOR_BLACK, 0);
    lv_obj_set_size(sub_sound_mode_page, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(sub_sound_mode_page, LV_OPA_80, 0);
    set_setting_hidden_for_video(sub_sound_mode_page);

    lv_obj_t * title_cont = lv_obj_create(sub_sound_mode_page);
    lv_obj_align(title_cont, LV_ALIGN_CENTER, 0, -250);
    lv_obj_set_size(title_cont, 480, 80);
    lv_obj_set_style_border_width(title_cont, 0, 0);
    lv_obj_set_style_pad_all(title_cont, 0, 0);
    lv_obj_set_style_radius(title_cont, 0, 0);
    lv_obj_set_style_shadow_width(title_cont, 0, 0);
    lv_obj_set_style_bg_color(title_cont, lv_color_make(125,125,125), 0);

    sound_mode_title_lable = lv_label_create(title_cont);
    lv_obj_set_style_text_font(sound_mode_title_lable, &GENERAL_FONT_MID, 0);
    lv_obj_add_style(sound_mode_title_lable, &lv_pro_res.label_white, 0);
    lv_label_set_recolor(sound_mode_title_lable, true);
    lv_label_set_text_fmt(sound_mode_title_lable, "#ffffff %s #", lv_get_string(STR_SOUND_MODE));
    lv_obj_center(sound_mode_title_lable);

    lv_obj_t * main_cont = lv_obj_create(sub_sound_mode_page);
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

    char *sound_mode_list[5];
    sound_mode_list[0] = lv_get_string(STR_STANDARD);
    sound_mode_list[1] = lv_get_string(STR_MUSIC);
    sound_mode_list[2] = lv_get_string(STR_MOVIE);
    sound_mode_list[3] = lv_get_string(STR_SPORTS);
    sound_mode_list[4] = lv_get_string(STR_USER);
    for (int i = 0; i < 5; i++) {
        item = lv_pro_setting_item_create(main_cont);
        lv_obj_set_size(item, 480, 80);
        lv_pro_setting_item_set_src(item, &GENERAL_FONT_MID, &lv_pro_res.set_bg_blue, sound_mode_list[i]);
        lv_group_add_obj(sound_mode_item_group, lv_pro_setting_item_get_btn(item));
        lv_obj_add_event_cb(lv_pro_setting_item_get_btn(item), sound_mode_event_cb, LV_EVENT_KEY, NULL);
    }
}

static void sound_page_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t *key = lv_event_get_param(e);
    struct _lv_obj_t * target_obj = lv_event_get_target(e);
    char *user_data = (char*) lv_event_get_user_data(e);

    if (code == LV_EVENT_KEY) {
        if (*key == LV_KEY_ENTER) {
            if (target_obj == sound_mode_cont_obj) {
                lv_sound_mode_init();
                set_current_ui_group(sound_mode_item_group);
                lv_group_focus_obj_for_id(lv_group_get_head_obj_ex(sound_mode_item_group), lv_get_sys_param(P_SOUND_MODE), true);
            } else if (target_obj == bass_cont_obj) {
                lv_sound_bass_init(lv_get_sys_param(P_SOUND_BASS));
                lv_group_only_add_one_obj(key_group, bass_slider_label);
                set_current_ui_group(key_group);
            } else if (target_obj == treble_cont_obj) {
                lv_sound_treble_init(lv_get_sys_param(P_SOUND_TREBLE));
                lv_group_only_add_one_obj(key_group, treble_slider_label);
                set_current_ui_group(key_group);
            } else if (target_obj == sound_output_cont_obj) {
                lv_sound_output_init();
                set_current_ui_group(sound_output_item_group);
                lv_group_focus_obj_for_id(lv_group_get_head_obj_ex(sound_output_item_group), lv_get_sys_param(P_SOUND_OUT_MODE), true);
            }
        } else if (*key == LV_KEY_BACK || *key == LV_KEY_LEFT) {
            set_current_ui_group(Setting_group);
        }
    }
}

void lv_sound_page_init(void)
{
    lv_obj_t * section;
    Setting_sound_group = lv_group_create();
    sound_mode_item_group = lv_group_create();
    sound_output_item_group  = lv_group_create();

    /* system */
    sub_sound_page = lv_menu_page_create(setting_menu, NULL);
    lv_obj_set_scrollbar_mode(sub_sound_page, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_hor(sub_sound_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(setting_menu), 0), 0);
    lv_menu_separator_create(sub_sound_page);
    section = lv_menu_section_create(sub_sound_page);
    lv_obj_set_style_bg_color(section, lv_color_hex(0x031FFF), 0);
    lv_obj_set_style_pad_all(section, 10, 0);
    lv_obj_set_style_pad_row(section, 30, 0);
    lv_obj_set_size(section, lv_pct(100), lv_pct(95));

    sound_mode_cont_obj = create_text(section, NULL, lv_get_string(STR_SOUND_MODE), lv_get_string(STR_MOVIE));
    sound_mode_label = lv_obj_get_child(sound_mode_cont_obj, 1);
    set_setting_outline_style(sound_mode_cont_obj);
    lv_group_add_obj(Setting_sound_group, sound_mode_cont_obj);
    lv_obj_add_event_cb(sound_mode_cont_obj, sound_page_event_handler, LV_EVENT_KEY, NULL);

    bass_cont_obj = create_text(section, NULL, lv_get_string(STR_BASS), "0");
    bass_label = lv_obj_get_child(bass_cont_obj, 1);
    lv_obj_add_flag(bass_cont_obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    set_setting_outline_style(bass_cont_obj);
    lv_group_add_obj(Setting_sound_group, bass_cont_obj);
    lv_obj_add_event_cb(bass_cont_obj, sound_page_event_handler, LV_EVENT_KEY, NULL);

    treble_cont_obj = create_text(section, NULL, lv_get_string(STR_TREBLE), "0");
    treble_label = lv_obj_get_child(treble_cont_obj, 1);
    lv_obj_add_flag(treble_cont_obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    set_setting_outline_style(treble_cont_obj);
    lv_group_add_obj(Setting_sound_group, treble_cont_obj);
    lv_obj_add_event_cb(treble_cont_obj, sound_page_event_handler, LV_EVENT_KEY, NULL);

    sound_output_cont_obj = create_text(section, NULL, lv_get_string(STR_SOUND_OUTPUT), lv_get_string(STR_STANDARD));
    sound_output_label = lv_obj_get_child(sound_output_cont_obj, 1);
    set_setting_outline_style(sound_output_cont_obj);
    lv_group_add_obj(Setting_sound_group, sound_output_cont_obj);
    lv_obj_add_event_cb(sound_output_cont_obj, sound_page_event_handler, LV_EVENT_KEY, NULL);

    sound_mode_only_user_can_modify(lv_get_sys_param(P_SOUND_MODE));
}

void lv_sound_page_exit(void)
{
    if (sub_bass_page) {
        lv_obj_del(sub_bass_page);
        sub_bass_page = NULL;
    }
    if (sub_treble_page) {
        lv_obj_del(sub_treble_page);
        sub_treble_page = NULL;
    }
    if (sub_sound_output_page) {
        lv_obj_del(sub_sound_output_page);
        sub_sound_output_page = NULL;
    }
    if (sub_sound_mode_page) {
        lv_obj_del(sub_sound_mode_page);
        sub_sound_mode_page = NULL;
    }
}
