/*
 * lv_pro_setting.c
 *
 */
#include "lv_pro_setting.h"
#include "lv_image_page.h"
#include "lv_keystone_page.h"
#include "lv_system_page.h"
#include "lv_sound_page.h"
#include <stdlib.h>
#include "../widget/lv_pro_res.h"
#include "../widget/lv_pro_setting_item.h"
#include "../lv_pro_launcher.h"
#include "../Common/setting/picture_setting.h"
#include "../Common/setting/system_setting.h"
#include "../Common/setting/keystone_setting.h"
#include "../Common/language/string/lv_string_id.h"
#include "lv_common.h"
#include "sys_param.h"
#include "page.h"

lv_obj_t *Setting_activity;
lv_group_t *Setting_group;

/*
*	root setting
*/
lv_obj_t *setting_menu;
lv_obj_t * root_page;


/*******************************************************/
void update_setting_lable(void)
{
    lv_label_set_text(lv_obj_get_child(image_obj, 1), lv_get_string(STR_PICTURE_MODE_TITLE));
    lv_label_set_text(lv_obj_get_child(system_obj, 1), lv_get_string(STR_OPTION_MODE_TITLE));
#if ENABLE_KSC
    lv_label_set_text(lv_obj_get_child(keystone_obj, 1), lv_get_string(STR_KEYSTONE));
#endif
    lv_label_set_text(lv_obj_get_child(sound_obj, 1), lv_get_string(STR_SOUND_MODE_TITLE));

    lv_label_set_text(lv_obj_get_child(image_mode_cont_obj, 0), lv_get_string(STR_PICTURE_MODE));
    lv_label_set_text(lv_obj_get_child(contrast_cont_obj, 0), lv_get_string(STR_CONSTRAST));
    lv_label_set_text(lv_obj_get_child(brightness_cont_obj, 0), lv_get_string(STR_BRIGHTNESS));
    lv_label_set_text(lv_obj_get_child(color_cont_obj, 0), lv_get_string(STR_COLOR));
    lv_label_set_text(lv_obj_get_child(sharpness_cont_obj, 0), lv_get_string(STR_SHARPNESS));
    lv_label_set_text(lv_obj_get_child(color_temp_cont_obj, 0), lv_get_string(STR_COLOR_TEMP));
    lv_label_set_text(lv_obj_get_child(aspect_ratio_cont_obj, 0), lv_get_string(STR_ASPECT_RATIO));
    lv_label_set_text(lv_obj_get_child(zoom_cont_obj, 0), lv_get_string(STR_WINDOW_SCALE));
}

void update_all_obj_for_page(lv_obj_t *page)
{
    char value_str[32];
    int value;
    if (page == sub_image_page) {
        value = lv_get_sys_param(P_PICTURE_MODE);
        if (value == PICTURE_MODE_STANDARD_ID)
            lv_label_set_text(image_mode_label, lv_get_string(STR_STANDARD));
        else if (value == PICTURE_MODE_DYNAMIC_ID)
            lv_label_set_text(image_mode_label, lv_get_string(STR_DYNAMIC));
        else if (value == PICTURE_MODE_MILD_ID)
            lv_label_set_text(image_mode_label, lv_get_string(STR_MILD));
        else if (value == PICTURE_MODE_USER_ID)
            lv_label_set_text(image_mode_label, lv_get_string(STR_USER));

        memset(value_str, 0, sizeof(value_str));
        sprintf(value_str, "%d", lv_get_sys_param(P_CONTRAST));
        lv_label_set_text(contrast_label, value_str);

        memset(value_str, 0, sizeof(value_str));
        sprintf(value_str, "%d", lv_get_sys_param(P_BRIGHTNESS));
        lv_label_set_text(brightness_label, value_str);

        memset(value_str, 0, sizeof(value_str));
        sprintf(value_str, "%d", lv_get_sys_param(P_COLOR));
        lv_label_set_text(color_label, value_str);

        memset(value_str, 0, sizeof(value_str));
        sprintf(value_str, "%d", lv_get_sys_param(P_SHARPNESS));
        lv_label_set_text(sharpness_label, value_str);

        memset(value_str, 0, sizeof(value_str));
        sprintf(value_str, "%d", lv_get_sys_param(P_SYS_ZOOM_DIS_MODE));
        lv_label_set_text(zoom_label, value_str);

        value = lv_get_sys_param(P_COLOR_TEMP);
        if (value == PQ_COLOR_TEMP_STANDARD_ID)
            lv_label_set_text(color_temp_label, lv_get_string(STR_STANDARD));
        else if (value == PQ_COLOR_TEMP_COLD_ID)
            lv_label_set_text(color_temp_label, lv_get_string(STR_COLD));
        else if (value == PQ_COLOR_TEMP_WARM_ID)
            lv_label_set_text(color_temp_label, lv_get_string(STR_WARM));

        value = lv_get_sys_param(P_ASPECT_RATIO);
        if (value == ASPECT_RATIO_16_9_ID)
            lv_label_set_text(aspect_ratio_label, "16:9");
        else if (value == ASPECT_RATIO_4_3_ID)
            lv_label_set_text(aspect_ratio_label, "4:3");

        set_all_pq_for_current_picture_mode();
    } else if (page == sub_system_page) {
        update_projection_mode();
        update_home_background();
        update_auto_sleep(1);
        update_osd_timer(0);
    } else if (page == sub_keystone_page) {
        update_keystone_label();
    } else if (page == sub_sound_page) {
        update_sound_label();
    }
}

static void setting_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t *key = lv_event_get_param(e);
    struct _lv_obj_t * cur_obj = lv_event_get_target(e);

    if (code == LV_EVENT_FOCUSED) {
        if (image_obj == cur_obj) {
            lv_menu_load_page_event_cb_ex(cur_obj, setting_menu, sub_image_page);
        } else if (system_obj == cur_obj) {
            lv_menu_load_page_event_cb_ex(cur_obj, setting_menu, sub_system_page);
        }
#if ENABLE_KSC
        else if (keystone_obj == cur_obj) {
            del_osd_timer();
            lv_menu_load_page_event_cb_ex(cur_obj, setting_menu, sub_keystone_page);
        } 
#endif
        else if (sound_obj == cur_obj) {
            lv_menu_load_page_event_cb_ex(cur_obj, setting_menu, sub_sound_page);
        }
    } else if (code == LV_EVENT_KEY) {
        if (*key == LV_KEY_BACK) {
            page_id_t last_page_id = get_last_page_id();
            if (last_page_id == PAGE_SOURCE || ((last_page_id >= PAGE_FILE) && (last_page_id <= PAGE_EBOOK)))
                switch_page_show_destory(last_page_id);
            else
                switch_page(PAGE_HOME);
        } else if (*key == LV_KEY_RIGHT || *key == LV_KEY_ENTER) {
            if (image_obj == cur_obj) {
                set_current_ui_group(Setting_image_group);
                lv_group_focus_obj(lv_group_get_head_obj_ex(Setting_image_group));
            } else if (system_obj == cur_obj) {
                set_current_ui_group(Setting_system_group);
                lv_group_focus_obj(lv_group_get_head_obj_ex(Setting_system_group));
            } 
#if ENABLE_KSC
            else if (keystone_obj == cur_obj) {
                set_current_ui_group(Setting_keystone_group);
                lv_group_focus_obj(lv_group_get_head_obj_ex(Setting_keystone_group));
            } 
#endif
            else if (sound_obj == cur_obj) {
                set_current_ui_group(Setting_sound_group);
                lv_group_focus_obj(lv_group_get_head_obj_ex(Setting_sound_group));
            }
        } else if (!is_global_key_go_new_channel(*key)) {
            page_id_t last_page_id = get_last_page_id();
            if ((last_page_id >= PAGE_FILE) && (last_page_id <= PAGE_EBOOK)) {
                set_current_page_id(last_page_id);
                destory_page(PAGE_SETTING);
            }
        }
    }
}

lv_obj_t * create_text(lv_obj_t * parent, const char * icon, const char * txt, char * value)
{
    lv_obj_t * obj = lv_menu_cont_create(parent);

    lv_obj_t * img = NULL;
    lv_obj_t * text_label = NULL;
    lv_obj_t * value_label = NULL;

    set_setting_outline_style(obj);

    if(icon) {
        img = lv_img_create(obj);
        lv_img_set_src(img, icon);
    }

    if(txt) {
        text_label = lv_label_create(obj);
        lv_label_set_text(text_label, txt);
        lv_obj_add_style(text_label, &lv_pro_res.label_white, 0);
        lv_obj_set_style_text_font(text_label, &GENERAL_FONT_MID, 0);
        lv_label_set_long_mode(text_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_flex_grow(text_label, 1);
    }

    if(value) {
        value_label = lv_label_create(obj);
        lv_label_set_text(value_label, value);
        lv_obj_add_style(value_label, &lv_pro_res.label_white, 0);
        lv_obj_set_style_text_font(value_label, &GENERAL_FONT_MID, 0);
        lv_label_set_long_mode(value_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_align(value_label, LV_ALIGN_RIGHT_MID, 0, 0);
    }

    return obj;
}

lv_obj_t * create_text_value(lv_obj_t * obj, const char * txt, lv_obj_t * value_label, char * value)
{
    lv_obj_t * text_label = NULL;

    if(txt) {
        text_label = lv_label_create(obj);
        lv_label_set_text(text_label, txt);
        lv_label_set_long_mode(text_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_flex_grow(text_label, 1);
    }

    if(value) {
        lv_label_set_text(value_label, value);
        lv_label_set_long_mode(value_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_align(value_label, LV_ALIGN_RIGHT_MID, 0, 0);
    }

    if(value && txt) {
        lv_obj_swap(value_label, text_label);
    }

    return obj;
}

lv_obj_t * create_switch(lv_obj_t * parent, const char * icon, const char * txt, bool chk)
{
    lv_obj_t * obj = create_text(parent, icon, txt, NULL);

    lv_obj_t * sw = lv_switch_create(obj);
    lv_obj_add_state(sw, chk ? LV_STATE_CHECKED : 0);

    return obj;
}

void set_setting_outline_style(lv_obj_t *obj)
{
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x031FFF), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_bg_opa(obj, 100, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_outline_width(obj, 2, 0);
    lv_obj_set_style_outline_color(obj, lv_color_hex(0xFFFFFFF), 0);
    lv_obj_set_style_outline_pad(obj, 4, 0);
    lv_obj_set_style_outline_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_outline_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
}

void set_setting_hidden_for_video(lv_obj_t *obj)
{
    page_id_t last_page_id = get_last_page_id();
    if (last_page_id == PAGE_SOURCE || ((last_page_id >= PAGE_FILE) && (last_page_id <= PAGE_EBOOK))) {
        lv_obj_add_flag(Setting_activity, LV_OBJ_FLAG_HIDDEN);

        if (obj)
            lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    }
}

void set_setting_show_for_video(void)
{
    page_id_t last_page_id = get_last_page_id();
    if (last_page_id == PAGE_SOURCE || ((last_page_id >= PAGE_FILE) && (last_page_id <= PAGE_EBOOK))) {
        lv_obj_clear_flag(Setting_activity, LV_OBJ_FLAG_HIDDEN);
    }
}

void lv_pro_setting_init(void) {
    Setting_activity = lv_obj_create(lv_layer_top());
    Setting_group = lv_group_create();
    Setting_image_group = lv_group_create();
    Setting_system_group = lv_group_create();
#if ENABLE_KSC
    Setting_keystone_group = lv_group_create();
#endif
    image_item_group = lv_group_create();
    image_slider_group = lv_group_create();
    color_temp_item_group = lv_group_create();
    aspect_ratio_item_group = lv_group_create();
    //resolution_item_group = lv_group_create();
    mbox_item_group = lv_group_create();

    lv_obj_set_size(Setting_activity, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(Setting_activity, COLOR_BLUE, 0);
    lv_obj_clear_flag(Setting_activity, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(Setting_activity, 0, 0);
    lv_obj_set_style_outline_width(Setting_activity, 0, 0);

    setting_menu = lv_menu_create(Setting_activity);
    lv_obj_set_style_bg_color(setting_menu, COLOR_BLUE, 0);
    lv_obj_set_size(setting_menu, lv_pct(95), lv_pct(80));
    lv_obj_set_style_text_font(((lv_menu_t *)setting_menu)->main_header_title, &GENERAL_FONT_MID, 0);
    lv_obj_center(setting_menu);

    lv_image_page_init();
    lv_system_page_init();
    lv_keystone_page_init();
    lv_sound_page_init();

    char *png_path[] = {"A:/usr/share/lv_projector/setting/setting_image.png",
                        "A:/usr/share/lv_projector/setting/setting_sound.png",
                        "A:/usr/share/lv_projector/setting/setting_system.png",
                        "A:/usr/share/lv_projector/setting/setting_keystone.png"};

    lv_obj_t * section;
    /*Create a root page*/
    root_page = lv_menu_page_create(setting_menu, NULL);
    lv_obj_set_style_pad_hor(root_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(setting_menu), 0), 0);
    lv_obj_set_size(root_page, lv_pct(100), lv_pct(95));

    section = lv_menu_section_create(root_page);
    lv_obj_set_style_bg_color(section, COLOR_BLUE, 0);
    lv_obj_set_style_pad_all(section, 10, 0);
    lv_obj_set_style_pad_row(section, 30, 0);
    lv_obj_set_size(section, lv_pct(100), lv_pct(100));
    lv_obj_set_style_border_color(section, lv_color_hex(0xFFFFFFF), 0);
    lv_obj_set_style_border_width(section, 3, 0);
    lv_obj_set_style_border_side(section, LV_BORDER_SIDE_RIGHT, 0);
    lv_obj_set_style_border_opa(section, LV_OPA_COVER, 0);

    lv_obj_t * title_obj = create_text(section, NULL, lv_get_string(STR_SETTING_TITLE), NULL);
    lv_obj_set_style_border_color(title_obj, lv_color_hex(0xFFFFFFF), 0);
    lv_obj_set_style_border_width(title_obj, 3, 0);
    lv_obj_set_style_border_side(title_obj, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_opa(title_obj, LV_OPA_COVER, 0);

    image_obj = create_text(section, png_path[0], lv_get_string(STR_PICTURE_MODE_TITLE), NULL);
    lv_menu_set_load_page_event(setting_menu, image_obj, sub_image_page);
    set_setting_outline_style(image_obj);
    lv_group_add_obj(Setting_group, image_obj);
    lv_obj_add_event_cb(image_obj, setting_event_handler, LV_EVENT_ALL, "Image_Settings");

    sound_obj = create_text(section, png_path[1], lv_get_string(STR_SOUND_MODE_TITLE), NULL);
    lv_menu_set_load_page_event(setting_menu, sound_obj, sub_sound_page);
    set_setting_outline_style(sound_obj);
    lv_group_add_obj(Setting_group, sound_obj);
    lv_obj_add_event_cb(sound_obj, setting_event_handler, LV_EVENT_ALL, NULL);

    system_obj = create_text(section, png_path[2], lv_get_string(STR_OPTION_MODE_TITLE), NULL);
    lv_menu_set_load_page_event(setting_menu, system_obj, sub_system_page);
    set_setting_outline_style(system_obj);
    lv_group_add_obj(Setting_group, system_obj);
    lv_obj_add_event_cb(system_obj, setting_event_handler, LV_EVENT_ALL, "System_Settings");
#if ENABLE_KSC
    keystone_obj = create_text(section, png_path[3], lv_get_string(STR_KEYSTONE), NULL);
    lv_menu_set_load_page_event(setting_menu, keystone_obj, sub_keystone_page);
    set_setting_outline_style(keystone_obj);
    lv_group_add_obj(Setting_group, keystone_obj);
    lv_obj_add_event_cb(keystone_obj, setting_event_handler, LV_EVENT_ALL, "Keystone_Correction");
#endif

    lv_menu_set_sidebar_page(setting_menu, root_page);

    lv_event_send(lv_obj_get_child(lv_obj_get_child(lv_menu_get_cur_sidebar_page(setting_menu), 0), 0), LV_EVENT_CLICKED, NULL);

    update_all_obj_for_page(sub_image_page);
    update_all_obj_for_page(sub_system_page);
    update_all_obj_for_page(sub_sound_page);
    update_all_obj_for_page(sub_keystone_page);

    lv_obj_update_layout(Setting_activity);
    lv_obj_add_flag(Setting_activity, LV_OBJ_FLAG_HIDDEN);

}

static int create_setting(void)
{
    lv_obj_clear_flag(Setting_activity, LV_OBJ_FLAG_HIDDEN);
    set_current_ui_group(Setting_group);
    lv_group_focus_obj(lv_group_get_head_obj_ex(Setting_group));
    update_osd_timer(1);
    return 0;
}

static int destory_setting(void)
{
    lv_obj_add_flag(Setting_activity, LV_OBJ_FLAG_HIDDEN);
    del_osd_timer();
    lv_image_page_exit();
    lv_system_page_exit();
    lv_sound_page_exit();
    lv_keystone_page_exit();
    return 0;
}

static int show_setting(void)
{
    lv_obj_clear_flag(Setting_activity, LV_OBJ_FLAG_HIDDEN);
    update_osd_timer(1);
    return 0;
}

static int hide_setting(void)
{
    lv_obj_add_flag(Setting_activity, LV_OBJ_FLAG_HIDDEN);
    del_osd_timer();
    return 0;
}

static page_interface_t page_setting =
{
    .ops =
    {
        create_setting,
        destory_setting,
        show_setting,
        hide_setting,
    },
    .info =
    {
        .id = PAGE_SETTING,
        .user_data = NULL
    }
};

void REGISTER_PAGE_SETTING(void)
{
    reg_page(&page_setting);
}
