#include "lv_pro_setting.h"
#include <stdlib.h>
#include "../widget/lv_pro_res.h"
#include "../widget/lv_pro_setting_item.h"
#include "../lv_pro_launcher.h"
#include "../Common/setting/picture_setting.h"
#include "../Common/language/string/lv_string_id.h"
#include "ksc/lv_ksc.h"
#include "lv_common.h"
#include "sys_param.h"

/*
*	image setting
*/
lv_obj_t *sub_image_page;
lv_obj_t *image_obj;
lv_group_t *Setting_image_group;
lv_group_t *image_item_group;
lv_group_t *image_slider_group;

lv_obj_t *image_mode_cont_obj;
lv_obj_t *image_mode_label;
lv_obj_t *image_mode_standard_item;
lv_obj_t *image_mode_dynamic_item;
lv_obj_t *image_mode_mild_item;
lv_obj_t *image_mode_user_item;

lv_obj_t *contrast_cont_obj;
lv_obj_t *contrast_label;
lv_obj_t *contrast_slider_label;
lv_obj_t *contrast_slider_bar;

lv_obj_t *brightness_cont_obj;
lv_obj_t *brightness_label;
lv_obj_t *brightness_slider_label;
lv_obj_t *brightness_slider_bar;

lv_obj_t *color_cont_obj;
lv_obj_t *color_label;
lv_obj_t *color_slider_label;
lv_obj_t *color_slider_bar;

lv_obj_t *sharpness_cont_obj;
lv_obj_t *sharpness_label;
lv_obj_t *sharpness_slider_label;
lv_obj_t *sharpness_slider_bar;

lv_obj_t *color_temp_cont_obj;
lv_obj_t *color_temp_label;
lv_obj_t *color_temp_standard_item;
lv_obj_t *color_temp_cold_item;
lv_obj_t *color_temp_warm_item;
lv_group_t *color_temp_item_group;

lv_obj_t *aspect_ratio_cont_obj;
lv_obj_t *aspect_ratio_label;
lv_obj_t *aspect_ratio_16_9_item;
lv_obj_t *aspect_ratio_4_3_item;
lv_group_t *aspect_ratio_item_group;

lv_obj_t *zoom_cont_obj;
lv_obj_t *zoom_label;
lv_obj_t *zoom_slider_label;
lv_obj_t *zoom_slider_bar;

lv_obj_t * sub_image_mode_page;
lv_obj_t * sub_contrast_page;
lv_obj_t * sub_brightness_page;
lv_obj_t * sub_color_page;
lv_obj_t * sub_sharpness_page;
lv_obj_t * sub_color_temp_page;
lv_obj_t * sub_aspect_ratio_page;
lv_obj_t * sub_zoom_page;

void lv_set_zoom(void)
{
#if ENABLE_KSC
    float w_ratio = 0;
    float h_ratio = 0;

    uint8_t mode = lv_get_sys_param(P_ASPECT_RATIO);
    uint8_t zoom_ratio = lv_get_sys_param(P_SYS_ZOOM_DIS_MODE);

    if (zoom_ratio > PQ_ZOOM_UI_MAX_VALUE || zoom_ratio < PQ_ZOOM_UI_MIN_VALUE)
        zoom_ratio = PQ_ZOOM_UI_MAX_VALUE;

    if (mode == ASPECT_RATIO_16_9_ID) {
        w_ratio = zoom_ratio;
        h_ratio = zoom_ratio;
    } else if (mode == ASPECT_RATIO_4_3_ID) {
        w_ratio = (float)zoom_ratio * 3 / 4;
        h_ratio = zoom_ratio;
    }

    //printf("set zoom: w_ratio %.2f, h_ratio %.2f\n", w_ratio, h_ratio);
    set_keystone_zoom(w_ratio, h_ratio);
#endif
}

static int set_slider_value(struct _lv_obj_t *sliber_label, int key)
{
    struct _lv_obj_t *text;
    struct _lv_obj_t *sliber_bar;
    sys_param_id id;
    int min, max;
    int value;
    char value_str[32] = {0};

    if (sliber_label == contrast_slider_label)
    {
       text = contrast_label;
       sliber_bar = contrast_slider_bar;
       id = P_CONTRAST;
    } else if (sliber_label == brightness_slider_label) {
       text = brightness_label;
       sliber_bar = brightness_slider_bar;
       id = P_BRIGHTNESS;
    } else if (sliber_label == color_slider_label) {
       text = color_label;
       sliber_bar = color_slider_bar;
       id = P_COLOR;
    } else if (sliber_label == sharpness_slider_label) {
       text = sharpness_label;
       sliber_bar = sharpness_slider_bar;
       id = P_SHARPNESS;
    } else if (sliber_label == zoom_slider_label) {
       text = zoom_label;
       sliber_bar = zoom_slider_bar;
       id = P_SYS_ZOOM_DIS_MODE;
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

    if (id == P_SYS_ZOOM_DIS_MODE)
        lv_set_zoom();

    return 0;
}

static void image_slider_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t *key = lv_event_get_param(e);
    struct _lv_obj_t * cur_obj = lv_event_get_target(e);
    char *user_data = (char*) lv_event_get_user_data(e);

    if (code == LV_EVENT_KEY) {
        if (*key == LV_KEY_BACK || !is_global_key_go_new_channel(*key)) {
            if (!strcmp(user_data, "Contrast")) {
                lv_obj_del(sub_contrast_page);
                sub_contrast_page = NULL;
                contrast_slider_label = NULL;
                contrast_slider_bar = NULL;
            } else if (!strcmp(user_data, "Brightness")) {
                lv_obj_del(sub_brightness_page);
                sub_brightness_page = NULL;
                brightness_slider_label = NULL;
                brightness_slider_bar = NULL;
            } else if (!strcmp(user_data, "Color")) {
                lv_obj_del(sub_color_page);
                sub_color_page = NULL;
                color_slider_label = NULL;
                color_slider_bar = NULL;
            } else if (!strcmp(user_data, "Sharpness")) {
                lv_obj_del(sub_sharpness_page);
                sub_sharpness_page = NULL;
                sharpness_slider_label = NULL;
                sharpness_slider_bar = NULL;
            } else if (!strcmp(user_data, "Zoom")) {
                lv_obj_del(sub_zoom_page);
                sub_zoom_page = NULL;
                zoom_slider_label = NULL;
                zoom_slider_bar = NULL;
            }
            if (*key == LV_KEY_BACK) {
                lv_menu_back_event_cb_ex(cur_obj, setting_menu);
                set_current_ui_group(Setting_image_group);
                set_setting_show_for_video();
            }
        } else if (*key == LV_KEY_UP || *key == LV_KEY_DOWN) {
            if (cur_obj == contrast_slider_label || cur_obj == brightness_slider_label || cur_obj == color_slider_label ||
                    cur_obj == sharpness_slider_label || cur_obj == zoom_slider_label) {
                set_slider_value(cur_obj, *key);
            }
        }
    }
}

static void color_temp_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t *key = lv_event_get_param(e);
    struct _lv_obj_t * cur_obj = lv_event_get_target(e);

    if (code == LV_EVENT_KEY) {
        if (*key == LV_KEY_ENTER) {
            if (cur_obj == lv_pro_setting_item_get_btn(color_temp_standard_item)) {
                lv_label_set_text(color_temp_label, lv_get_string(STR_STANDARD));
                lv_set_sys_param(P_COLOR_TEMP, COLOR_TEMP_STANDARD);
            } else if (cur_obj == lv_pro_setting_item_get_btn(color_temp_cold_item)) {
                lv_label_set_text(color_temp_label, lv_get_string(STR_COLD));
                lv_set_sys_param(P_COLOR_TEMP, COLOR_TEMP_COLD);
            } else if (cur_obj == lv_pro_setting_item_get_btn(color_temp_warm_item)) {
                lv_label_set_text(color_temp_label, lv_get_string(STR_WARM));
                lv_set_sys_param(P_COLOR_TEMP, COLOR_TEMP_WARM);
            }
            update_picture_param();
            update_all_obj_for_page(sub_image_page);

            lv_obj_del(sub_color_temp_page);
            sub_color_temp_page = NULL;
            lv_menu_back_event_cb_ex(cur_obj, setting_menu);
            set_current_ui_group(Setting_image_group);
            set_setting_show_for_video();
        } else if (*key == LV_KEY_BACK || !is_global_key_go_new_channel(*key)) {
            lv_obj_del(sub_color_temp_page);
            sub_color_temp_page = NULL;

            if (*key == LV_KEY_BACK) {
                lv_menu_back_event_cb_ex(cur_obj, setting_menu);
                set_current_ui_group(Setting_image_group);
                set_setting_show_for_video();
            } else {
                lv_obj_add_flag(Setting_activity, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

static void aspect_ratio_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t *key = lv_event_get_param(e);
    struct _lv_obj_t * cur_obj = lv_event_get_target(e);

    if (code == LV_EVENT_KEY) {
        if (*key == LV_KEY_ENTER) {
            if (cur_obj == lv_pro_setting_item_get_btn(aspect_ratio_16_9_item)) {
                lv_label_set_text(aspect_ratio_label, "16:9");
                lv_set_sys_param(P_ASPECT_RATIO, ASPECT_RATIO_16_9_ID);
            } else if (cur_obj == lv_pro_setting_item_get_btn(aspect_ratio_4_3_item)) {
                lv_label_set_text(aspect_ratio_label, "4:3");
                lv_set_sys_param(P_ASPECT_RATIO, ASPECT_RATIO_4_3_ID);
            }

            lv_set_zoom();
            update_picture_param();
            update_all_obj_for_page(sub_image_page);

            lv_obj_del(sub_aspect_ratio_page);
            sub_aspect_ratio_page = NULL;
            lv_menu_back_event_cb_ex(cur_obj, setting_menu);
            set_current_ui_group(Setting_image_group);
            set_setting_show_for_video();
        } else if (*key == LV_KEY_BACK || !is_global_key_go_new_channel(*key)) {
            lv_obj_del(sub_aspect_ratio_page);
            sub_aspect_ratio_page = NULL;

            if (*key == LV_KEY_BACK) {
                lv_menu_back_event_cb_ex(cur_obj, setting_menu);
                set_current_ui_group(Setting_image_group);
                set_setting_show_for_video();
            } else {
                lv_obj_add_flag(Setting_activity, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

static void image_mode_only_user_can_modify(int mode)
{
    if (mode == PICTURE_MODE_STANDARD_ID || mode == PICTURE_MODE_DYNAMIC_ID ||
        mode == PICTURE_MODE_MILD_ID) {
        lv_obj_add_state(contrast_cont_obj, LV_STATE_DISABLED);
        lv_obj_set_style_text_color(lv_obj_get_child(contrast_cont_obj, 0), lv_color_make(125,125,125), 0);
        lv_obj_set_style_text_color(contrast_label, lv_color_make(125,125,125), 0);

        lv_obj_add_state(brightness_cont_obj, LV_STATE_DISABLED);
        lv_obj_set_style_text_color(lv_obj_get_child(brightness_cont_obj, 0), lv_color_make(125,125,125), 0);
        lv_obj_set_style_text_color(brightness_label, lv_color_make(125,125,125), 0);

        lv_obj_add_state(color_cont_obj, LV_STATE_DISABLED);
        lv_obj_set_style_text_color(lv_obj_get_child(color_cont_obj, 0), lv_color_make(125,125,125), 0);
        lv_obj_set_style_text_color(color_label, lv_color_make(125,125,125), 0);

        lv_obj_add_state(sharpness_cont_obj, LV_STATE_DISABLED);
        lv_obj_set_style_text_color(lv_obj_get_child(sharpness_cont_obj, 0), lv_color_make(125,125,125), 0);
        lv_obj_set_style_text_color(sharpness_label, lv_color_make(125,125,125), 0);
    } else if (PICTURE_MODE_USER_ID) {
        lv_obj_clear_state(contrast_cont_obj, LV_STATE_DISABLED);
        lv_obj_set_style_text_color(lv_obj_get_child(contrast_cont_obj, 0), lv_color_white(), 0);
        lv_obj_set_style_text_color(contrast_label, lv_color_white(), 0);

        lv_obj_clear_state(brightness_cont_obj, LV_STATE_DISABLED);
        lv_obj_set_style_text_color(lv_obj_get_child(brightness_cont_obj, 0), lv_color_white(), 0);
        lv_obj_set_style_text_color(brightness_label, lv_color_white(), 0);

        lv_obj_clear_state(color_cont_obj, LV_STATE_DISABLED);
        lv_obj_set_style_text_color(lv_obj_get_child(color_cont_obj, 0), lv_color_white(), 0);
        lv_obj_set_style_text_color(color_label, lv_color_white(), 0);

        lv_obj_clear_state(sharpness_cont_obj, LV_STATE_DISABLED);
        lv_obj_set_style_text_color(lv_obj_get_child(sharpness_cont_obj, 0), lv_color_white(), 0);
        lv_obj_set_style_text_color(sharpness_label, lv_color_white(), 0);
    }
}

static void image_mode_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t *key = lv_event_get_param(e);
    struct _lv_obj_t * cur_obj = lv_event_get_target(e);

    if (code == LV_EVENT_KEY) {
        if (*key == LV_KEY_ENTER) {
            if (cur_obj == lv_pro_setting_item_get_btn(image_mode_standard_item)) {
                lv_label_set_text(image_mode_label, lv_get_string(STR_STANDARD));
                lv_set_sys_param(P_PICTURE_MODE, PICTURE_MODE_STANDARD_ID);
                image_mode_only_user_can_modify(PICTURE_MODE_STANDARD_ID);
            } else if (cur_obj == lv_pro_setting_item_get_btn(image_mode_dynamic_item)) {
                lv_label_set_text(image_mode_label, lv_get_string(STR_DYNAMIC));
                lv_set_sys_param(P_PICTURE_MODE, PICTURE_MODE_DYNAMIC_ID);
                image_mode_only_user_can_modify(PICTURE_MODE_DYNAMIC_ID);
            } else if (cur_obj == lv_pro_setting_item_get_btn(image_mode_mild_item)) {
                lv_label_set_text(image_mode_label, lv_get_string(STR_MILD));
                lv_set_sys_param(P_PICTURE_MODE, PICTURE_MODE_MILD_ID);
                image_mode_only_user_can_modify(PICTURE_MODE_MILD_ID);
            } else if (cur_obj == lv_pro_setting_item_get_btn(image_mode_user_item)) {
                lv_label_set_text(image_mode_label, lv_get_string(STR_USER));
                lv_set_sys_param(P_PICTURE_MODE, PICTURE_MODE_USER_ID);
                image_mode_only_user_can_modify(PICTURE_MODE_USER_ID);
            }
            update_picture_param();
            update_all_obj_for_page(sub_image_page);

            lv_obj_del(sub_image_mode_page);
            sub_image_mode_page = NULL;
            lv_menu_back_event_cb_ex(cur_obj, setting_menu);
            set_current_ui_group(Setting_image_group);
            set_setting_show_for_video();
        } else if (*key == LV_KEY_BACK || !is_global_key_go_new_channel(*key)) {
            lv_obj_del(sub_image_mode_page);
            sub_image_mode_page = NULL;
            if (*key == LV_KEY_BACK) {
                lv_menu_back_event_cb_ex(cur_obj, setting_menu);
                set_current_ui_group(Setting_image_group);
                set_setting_show_for_video();
            } else {
                lv_obj_add_flag(Setting_activity, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

static void sub_page_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t *key = lv_event_get_param(e);
    struct _lv_obj_t * cur_obj = lv_event_get_target(e);
    char *user_data = (char*) lv_event_get_user_data(e);

    if (code == LV_EVENT_KEY) {
        if (*key == LV_KEY_ENTER) {
            if (cur_obj == image_mode_cont_obj) {
                lv_image_mode_init();
                set_current_ui_group(image_item_group);
                lv_group_focus_obj_for_id(lv_group_get_head_obj_ex(image_item_group), lv_get_sys_param(P_PICTURE_MODE), true);
            } else if (cur_obj == contrast_cont_obj) {
                uint32_t val = lv_get_sys_param(P_CONTRAST);
                lv_contrast_init(val);
                lv_group_only_add_one_obj(image_slider_group, contrast_slider_label);
                set_current_ui_group(image_slider_group);
                lv_group_focus_obj_ex(lv_group_get_head_obj_ex(image_slider_group));
            } else if (cur_obj == brightness_cont_obj) {
                uint32_t val = lv_get_sys_param(P_BRIGHTNESS);
                lv_brightness_init(val);
                lv_group_only_add_one_obj(image_slider_group, brightness_slider_label);
                set_current_ui_group(image_slider_group);
                lv_group_focus_obj_ex(lv_group_get_head_obj_ex(image_slider_group));
            } else if (cur_obj == color_cont_obj) {
                uint32_t val = lv_get_sys_param(P_COLOR);
                lv_color_init(val);
                lv_group_only_add_one_obj(image_slider_group, color_slider_label);
                set_current_ui_group(image_slider_group);
                lv_group_focus_obj_ex(lv_group_get_head_obj_ex(image_slider_group));
            } else if (cur_obj == sharpness_cont_obj) {
                uint32_t val = lv_get_sys_param(P_SHARPNESS);
                lv_sharpness_init(val);
                lv_group_only_add_one_obj(image_slider_group, sharpness_slider_label);
                set_current_ui_group(image_slider_group);
                lv_group_focus_obj_ex(lv_group_get_head_obj_ex(image_slider_group));
            } else if (cur_obj == zoom_cont_obj) {
                uint32_t val = lv_get_sys_param(P_SYS_ZOOM_DIS_MODE);
                lv_zoom_init(val);
                lv_group_only_add_one_obj(image_slider_group, zoom_slider_label);
                set_current_ui_group(image_slider_group);
                lv_group_focus_obj_ex(lv_group_get_head_obj_ex(image_slider_group));
            } else if (cur_obj == color_temp_cont_obj) {
                lv_color_temp_init();
                set_current_ui_group(color_temp_item_group);
                lv_group_focus_obj_for_id(lv_group_get_head_obj_ex(color_temp_item_group), lv_get_sys_param(P_COLOR_TEMP), true);
            } else if (cur_obj == aspect_ratio_cont_obj) {
                lv_aspect_ratio_init();
                set_current_ui_group(aspect_ratio_item_group);
                lv_group_focus_obj_for_id(lv_group_get_head_obj_ex(aspect_ratio_item_group), lv_get_sys_param(P_ASPECT_RATIO), true);
            }
        } else if (*key == LV_KEY_BACK || *key == LV_KEY_LEFT) {
            set_current_ui_group(Setting_group);
        }
    }
}

static void slider_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * slider = lv_event_get_target(e);
    char *user_data = (char*) lv_event_get_user_data(e);
    char buf[10];
    lv_snprintf(buf, sizeof(buf), "%d", (int)lv_slider_get_value(slider));

    if (code == LV_EVENT_VALUE_CHANGED) {
        if (!strcmp(user_data, "Contrast")) {
            lv_label_set_text(contrast_slider_label, buf);
            lv_label_set_text(contrast_label, buf);
            /* call contrast function interface */

        } else if (!strcmp(user_data, "Brightness")) {
            lv_label_set_text(brightness_slider_label, buf);
            lv_label_set_text(brightness_label, buf);
            /* call brightness function interface */

        } else if (!strcmp(user_data, "Color")) {
            lv_label_set_text(color_slider_label, buf);
            lv_label_set_text(color_label, buf);
            /* call color function interface */

        } else if (!strcmp(user_data, "Sharpness")) {
            lv_label_set_text(sharpness_slider_label, buf);
            lv_label_set_text(sharpness_label, buf);
            /* call sharpness function interface */

        } else if (!strcmp(user_data, "Zoom")) {
            lv_label_set_text(zoom_slider_label, buf);
            lv_label_set_text(zoom_label, buf);
            /* call zoom function interface */
        }
    }
}

void create_slider_value(lv_obj_t * obj, lv_obj_t ** slider_obj, lv_obj_t ** value_obj,
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

    lv_obj_add_event_cb(*slider_obj, slider_event_cb, LV_EVENT_VALUE_CHANGED, txt);
}


void lv_image_mode_init(void)
{
    if (!sub_image_mode_page)
        sub_image_mode_page = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(sub_image_mode_page);
    lv_obj_set_style_bg_color(sub_image_mode_page, COLOR_BLACK, 0);
    lv_obj_set_size(sub_image_mode_page, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(sub_image_mode_page, LV_OPA_80, 0);
    set_setting_hidden_for_video(sub_image_mode_page);

    lv_obj_t * title_cont = lv_obj_create(sub_image_mode_page);
    lv_obj_align(title_cont, LV_ALIGN_CENTER, 0, -250);
    lv_obj_set_size(title_cont, 480, 80);
    lv_obj_set_style_border_width(title_cont, 0, 0);
    lv_obj_set_style_pad_all(title_cont, 0, 0);
    lv_obj_set_style_radius(title_cont, 0, 0);
    lv_obj_set_style_shadow_width(title_cont, 0, 0);
    lv_obj_set_style_bg_color(title_cont, lv_color_make(125,125,125), 0);

    lv_obj_t * title_lable = lv_label_create(title_cont);
    lv_obj_set_style_text_font(title_lable, &GENERAL_FONT_MID, 0);
    lv_label_set_recolor(title_lable, true);
    lv_label_set_text_fmt(title_lable, "#ffffff %s #", lv_get_string(STR_PICTURE_MODE));
    lv_obj_center(title_lable);

    lv_obj_t * main_cont = lv_obj_create(sub_image_mode_page);
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

    image_mode_standard_item = lv_pro_setting_item_create(main_cont);
    lv_obj_set_size(image_mode_standard_item, 480, 80);
    lv_pro_setting_item_set_src(image_mode_standard_item, &GENERAL_FONT_MID, &lv_pro_res.set_bg_blue, lv_get_string(STR_STANDARD));
    lv_group_add_obj(image_item_group, lv_pro_setting_item_get_btn(image_mode_standard_item));
    lv_obj_add_event_cb(lv_pro_setting_item_get_btn(image_mode_standard_item), image_mode_event_cb, LV_EVENT_KEY, "Image_Mode");

    image_mode_dynamic_item = lv_pro_setting_item_create(main_cont);
    lv_obj_set_size(image_mode_dynamic_item, 480, 80);
    lv_pro_setting_item_set_src(image_mode_dynamic_item, &GENERAL_FONT_MID, &lv_pro_res.set_bg_blue, lv_get_string(STR_DYNAMIC));
    lv_group_add_obj(image_item_group, lv_pro_setting_item_get_btn(image_mode_dynamic_item));
    lv_obj_add_event_cb(lv_pro_setting_item_get_btn(image_mode_dynamic_item), image_mode_event_cb, LV_EVENT_KEY, "Image_Mode");

    image_mode_mild_item = lv_pro_setting_item_create(main_cont);
    lv_obj_set_size(image_mode_mild_item, 480, 80);
    lv_pro_setting_item_set_src(image_mode_mild_item, &GENERAL_FONT_MID, &lv_pro_res.set_bg_blue, lv_get_string(STR_MILD));
    lv_group_add_obj(image_item_group, lv_pro_setting_item_get_btn(image_mode_mild_item));
    lv_obj_add_event_cb(lv_pro_setting_item_get_btn(image_mode_mild_item), image_mode_event_cb, LV_EVENT_KEY, "Image_Mode");

    image_mode_user_item = lv_pro_setting_item_create(main_cont);
    lv_obj_set_size(image_mode_user_item, 480, 80);
    lv_pro_setting_item_set_src(image_mode_user_item, &GENERAL_FONT_MID, &lv_pro_res.set_bg_blue, lv_get_string(STR_USER));
    lv_group_add_obj(image_item_group, lv_pro_setting_item_get_btn(image_mode_user_item));
    lv_obj_add_event_cb(lv_pro_setting_item_get_btn(image_mode_user_item), image_mode_event_cb, LV_EVENT_KEY, "Image_Mode");
}

void lv_contrast_init(int32_t val)
{
    if (!sub_contrast_page)
        sub_contrast_page = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(sub_contrast_page);
    lv_obj_set_style_bg_color(sub_contrast_page, COLOR_BLUE, 0);
    lv_obj_set_size(sub_contrast_page, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(sub_contrast_page, LV_OPA_30, 0);
    set_setting_hidden_for_video(sub_contrast_page);

    create_slider_value(sub_contrast_page, &contrast_slider_bar, &contrast_slider_label, "Contrast",
            PQ_CONTRAST_UI_MIN_VALUE, PQ_CONTRAST_UI_MAX_VALUE, val);

    lv_obj_add_event_cb(contrast_slider_label, image_slider_event_cb, LV_EVENT_KEY, "Contrast");
}

void lv_brightness_init(int32_t val)
{
    if (!sub_brightness_page)
        sub_brightness_page = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(sub_brightness_page);
    lv_obj_set_style_bg_color(sub_brightness_page, COLOR_BLUE, 0);
    lv_obj_set_size(sub_brightness_page, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(sub_brightness_page, LV_OPA_30, 0);
    set_setting_hidden_for_video(sub_brightness_page);

    create_slider_value(sub_brightness_page, &brightness_slider_bar, &brightness_slider_label, "Brightness", PQ_BRIGHTNESS_UI_MIN_VALUE, PQ_BRIGHTNESS_UI_MAX_VALUE, val);

    lv_obj_add_event_cb(brightness_slider_label, image_slider_event_cb, LV_EVENT_KEY, "Brightness");
}

void lv_color_init(int32_t val)
{
    if (!sub_color_page)
        sub_color_page = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(sub_color_page);
    lv_obj_set_style_bg_color(sub_color_page, COLOR_BLUE, 0);
    lv_obj_set_size(sub_color_page, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(sub_color_page, LV_OPA_30, 0);
    set_setting_hidden_for_video(sub_color_page);

    create_slider_value(sub_color_page, &color_slider_bar, &color_slider_label, "Color", PQ_COLOR_UI_MIN_VALUE, PQ_COLOR_UI_MAX_VALUE, val);

    lv_obj_add_event_cb(color_slider_label, image_slider_event_cb, LV_EVENT_KEY, "Color");
}

void lv_sharpness_init(int32_t val)
{
    if (!sub_sharpness_page)
        sub_sharpness_page = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(sub_sharpness_page);
    lv_obj_set_style_bg_color(sub_sharpness_page, COLOR_BLUE, 0);
    lv_obj_set_size(sub_sharpness_page, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(sub_sharpness_page, LV_OPA_30, 0);
    set_setting_hidden_for_video(sub_sharpness_page);

    create_slider_value(sub_sharpness_page, &sharpness_slider_bar, &sharpness_slider_label, "Sharpness", PQ_SHARPNESS_UI_MIN_VALUE, PQ_SHARPNESS_UI_MAX_VALUE, val);

    lv_obj_add_event_cb(sharpness_slider_label, image_slider_event_cb, LV_EVENT_KEY, "Sharpness");
}


void lv_color_temp_init(void)
{
    if (!sub_color_temp_page)
        sub_color_temp_page = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(sub_color_temp_page);
    lv_obj_set_style_bg_color(sub_color_temp_page, COLOR_BLACK, 0);
    lv_obj_set_size(sub_color_temp_page, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(sub_color_temp_page, LV_OPA_80, 0);
    set_setting_hidden_for_video(sub_color_temp_page);

    lv_obj_t *title_cont = lv_obj_create(sub_color_temp_page);
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
    lv_label_set_text_fmt(title_lable, "#ffffff %s #", lv_get_string(STR_COLOR_TEMP));
    lv_obj_center(title_lable);

    lv_obj_t *main_cont = lv_obj_create(sub_color_temp_page);
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

    color_temp_standard_item = lv_pro_setting_item_create(main_cont);
    lv_obj_set_size(color_temp_standard_item, 480, 80);
    lv_pro_setting_item_set_src(color_temp_standard_item, &GENERAL_FONT_MID, &lv_pro_res.set_bg_blue, lv_get_string(STR_STANDARD));
    lv_group_add_obj(color_temp_item_group, lv_pro_setting_item_get_btn(color_temp_standard_item));
    lv_obj_add_event_cb(lv_pro_setting_item_get_btn(color_temp_standard_item), color_temp_event_cb, LV_EVENT_KEY, "Color_Temperature");

    color_temp_cold_item = lv_pro_setting_item_create(main_cont);
    lv_obj_set_size(color_temp_cold_item, 480, 80);
    lv_pro_setting_item_set_src(color_temp_cold_item, &GENERAL_FONT_MID, &lv_pro_res.set_bg_blue, lv_get_string(STR_COLD));
    lv_group_add_obj(color_temp_item_group, lv_pro_setting_item_get_btn(color_temp_cold_item));
    lv_obj_add_event_cb(lv_pro_setting_item_get_btn(color_temp_cold_item), color_temp_event_cb, LV_EVENT_KEY, "Color_Temperature");

    color_temp_warm_item = lv_pro_setting_item_create(main_cont);
    lv_obj_set_size(color_temp_warm_item, 480, 80);
    lv_pro_setting_item_set_src(color_temp_warm_item, &GENERAL_FONT_MID, &lv_pro_res.set_bg_blue, lv_get_string(STR_WARM));
    lv_group_add_obj(color_temp_item_group, lv_pro_setting_item_get_btn(color_temp_warm_item));
    lv_obj_add_event_cb(lv_pro_setting_item_get_btn(color_temp_warm_item), color_temp_event_cb, LV_EVENT_KEY, "Color_Temperature");
}

void lv_aspect_ratio_init(void)
{
    if (!sub_aspect_ratio_page)
        sub_aspect_ratio_page = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(sub_aspect_ratio_page);
    lv_obj_set_style_bg_color(sub_aspect_ratio_page, COLOR_BLACK, 0);
    lv_obj_set_size(sub_aspect_ratio_page, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(sub_aspect_ratio_page, LV_OPA_80, 0);
    set_setting_hidden_for_video(sub_aspect_ratio_page);

    lv_obj_t *title_cont = lv_obj_create(sub_aspect_ratio_page);
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
    lv_label_set_text_fmt(title_lable, "#ffffff %s #", lv_get_string(STR_ASPECT_RATIO));
    lv_obj_center(title_lable);

    lv_obj_t *main_cont = lv_obj_create(sub_aspect_ratio_page);
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

    aspect_ratio_16_9_item = lv_pro_setting_item_create(main_cont);
    lv_obj_set_size(aspect_ratio_16_9_item, 480, 80);
    lv_pro_setting_item_set_src(aspect_ratio_16_9_item, &GENERAL_FONT_MID, &lv_pro_res.set_bg_blue, lv_get_string(STR_ASPECT_16_9));
    lv_group_add_obj(aspect_ratio_item_group, lv_pro_setting_item_get_btn(aspect_ratio_16_9_item));
    lv_obj_add_event_cb(lv_pro_setting_item_get_btn(aspect_ratio_16_9_item), aspect_ratio_event_cb, LV_EVENT_KEY, "Aspect_Ratio");

    aspect_ratio_4_3_item = lv_pro_setting_item_create(main_cont);
    lv_obj_set_size(aspect_ratio_4_3_item, 480, 80);
    lv_pro_setting_item_set_src(aspect_ratio_4_3_item, &GENERAL_FONT_MID, &lv_pro_res.set_bg_blue, lv_get_string(STR_ASPECT_4_3));
    lv_group_add_obj(aspect_ratio_item_group, lv_pro_setting_item_get_btn(aspect_ratio_4_3_item));
    lv_obj_add_event_cb(lv_pro_setting_item_get_btn(aspect_ratio_4_3_item), aspect_ratio_event_cb, LV_EVENT_KEY, "Aspect_Ratio");
}


void lv_zoom_init(int32_t val)
{
    if (!sub_zoom_page)
        sub_zoom_page = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(sub_zoom_page);
    lv_obj_set_style_bg_color(sub_zoom_page, COLOR_BLUE, 0);
    lv_obj_set_size(sub_zoom_page, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(sub_zoom_page, LV_OPA_30, 0);
    set_setting_hidden_for_video(sub_zoom_page);

    create_slider_value(sub_zoom_page, &zoom_slider_bar, &zoom_slider_label, "Zoom",
            PQ_ZOOM_UI_MIN_VALUE, PQ_ZOOM_UI_MAX_VALUE, val);

    lv_obj_add_event_cb(zoom_slider_label, image_slider_event_cb, LV_EVENT_KEY, "Zoom");
}


void lv_image_page_init(void)
{
    lv_obj_t * section;

    /* image */
    sub_image_page = lv_menu_page_create(setting_menu, NULL);
    lv_obj_set_scrollbar_mode(sub_image_page, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_hor(sub_image_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(setting_menu), 0), 0);
    lv_menu_separator_create(sub_image_page);
    section = lv_menu_section_create(sub_image_page);
    lv_obj_set_style_bg_color(section, COLOR_BLUE, 0);
    lv_obj_set_style_pad_all(section, 10, 0);
    lv_obj_set_style_pad_row(section, 30, 0);
    lv_obj_set_size(section, lv_pct(100), lv_pct(95));


    image_mode_cont_obj = create_text(section, NULL, lv_get_string(STR_PICTURE_MODE), "standard");
    image_mode_label = lv_obj_get_child(image_mode_cont_obj, 1);
    lv_obj_add_flag(image_mode_cont_obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    set_setting_outline_style(image_mode_cont_obj);
    lv_group_add_obj(Setting_image_group, image_mode_cont_obj);
    lv_obj_add_event_cb(image_mode_cont_obj, sub_page_event_handler, LV_EVENT_KEY, "Image_Mode");

    contrast_cont_obj = create_text(section, NULL, lv_get_string(STR_CONSTRAST), "10");
    contrast_label = lv_obj_get_child(contrast_cont_obj, 1);
    lv_obj_add_flag(contrast_cont_obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    set_setting_outline_style(contrast_cont_obj);
    lv_group_add_obj(Setting_image_group, contrast_cont_obj);
    lv_obj_add_event_cb(contrast_cont_obj, sub_page_event_handler, LV_EVENT_KEY, "Contrast");

    brightness_cont_obj = create_text(section, NULL, lv_get_string(STR_BRIGHTNESS), "20");
    brightness_label = lv_obj_get_child(brightness_cont_obj, 1);
    lv_obj_add_flag(brightness_cont_obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    set_setting_outline_style(brightness_cont_obj);
    lv_group_add_obj(Setting_image_group, brightness_cont_obj);
    lv_obj_add_event_cb(brightness_cont_obj, sub_page_event_handler, LV_EVENT_KEY, "Brightness");

    color_cont_obj = create_text(section, NULL, lv_get_string(STR_COLOR), "30");
    color_label = lv_obj_get_child(color_cont_obj, 1);
    lv_obj_add_flag(color_cont_obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    set_setting_outline_style(color_cont_obj);
    lv_group_add_obj(Setting_image_group, color_cont_obj);
    lv_obj_add_event_cb(color_cont_obj, sub_page_event_handler, LV_EVENT_KEY, "Color");

    sharpness_cont_obj = create_text(section, NULL, lv_get_string(STR_SHARPNESS), "0");
    sharpness_label = lv_obj_get_child(sharpness_cont_obj, 1);
    lv_obj_add_flag(sharpness_cont_obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    set_setting_outline_style(sharpness_cont_obj);
    lv_group_add_obj(Setting_image_group, sharpness_cont_obj);
    lv_obj_add_event_cb(sharpness_cont_obj, sub_page_event_handler, LV_EVENT_KEY, "Sharpness");

    color_temp_cont_obj = create_text(section, NULL, lv_get_string(STR_COLOR_TEMP), "standard");
    color_temp_label = lv_obj_get_child(color_temp_cont_obj, 1);
    lv_obj_add_flag(color_temp_cont_obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    set_setting_outline_style(color_temp_cont_obj);
    lv_group_add_obj(Setting_image_group, color_temp_cont_obj);
    lv_obj_add_event_cb(color_temp_cont_obj, sub_page_event_handler, LV_EVENT_KEY, "Color_Temperature");

    aspect_ratio_cont_obj = create_text(section, NULL, lv_get_string(STR_ASPECT_RATIO), "16:9");
    aspect_ratio_label = lv_obj_get_child(aspect_ratio_cont_obj, 1);
    set_setting_outline_style(aspect_ratio_cont_obj);
    lv_obj_add_flag(aspect_ratio_cont_obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_group_add_obj(Setting_image_group, aspect_ratio_cont_obj);
    lv_obj_add_event_cb(aspect_ratio_cont_obj, sub_page_event_handler, LV_EVENT_KEY, "Aspect_Ratio");

    zoom_cont_obj = create_text(section, NULL, lv_get_string(STR_WINDOW_SCALE), "0");
    zoom_label = lv_obj_get_child(zoom_cont_obj, 1);
    lv_obj_add_flag(zoom_cont_obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    set_setting_outline_style(zoom_cont_obj);
    lv_group_add_obj(Setting_image_group, zoom_cont_obj);
    lv_obj_add_event_cb(zoom_cont_obj, sub_page_event_handler, LV_EVENT_KEY, "Zoom");

    image_mode_only_user_can_modify(lv_get_sys_param(P_PICTURE_MODE));
}

void lv_image_page_exit(void)
{
    if (sub_contrast_page) {
        lv_obj_del(sub_contrast_page);
        sub_contrast_page = NULL;
    }
    if (sub_brightness_page) {
        lv_obj_del(sub_brightness_page);
        sub_brightness_page = NULL;
    }
    if (sub_color_page) {
        lv_obj_del(sub_color_page);
        sub_color_page = NULL;
    }
    if (sub_sharpness_page) {
        lv_obj_del(sub_sharpness_page);
        sub_sharpness_page = NULL;
    }
    if (sub_zoom_page) {
        lv_obj_del(sub_zoom_page);
        sub_zoom_page = NULL;
    }
    if (sub_color_temp_page) {
        lv_obj_del(sub_color_temp_page);
        sub_color_temp_page = NULL;
    }
    if (sub_aspect_ratio_page) {
        lv_obj_del(sub_aspect_ratio_page);
        sub_aspect_ratio_page = NULL;
    }
    if (sub_image_mode_page) {
        lv_obj_del(sub_image_mode_page);
        sub_image_mode_page = NULL;
    }
}
