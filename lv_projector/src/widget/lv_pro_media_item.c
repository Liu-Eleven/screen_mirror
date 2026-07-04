/*
 * lv_pro_media_item.c
 *
 */

#include "lv_pro_media_item.h"
#include "lvgl/lvgl.h"
#include "lv_pro_res.h"

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &lv_pro_media_item_class

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_pro_media_item_constructor(const lv_obj_class_t *class_p,
        lv_obj_t *obj);
static void lv_pro_media_item_destructor(const lv_obj_class_t *class_p,
        lv_obj_t *obj);
static void lv_pro_media_item_anim_timer(lv_timer_t *timer);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_pro_media_item_class = {
    .constructor_cb = lv_pro_media_item_constructor,
    .destructor_cb = lv_pro_media_item_destructor,
    .width_def = LV_SIZE_CONTENT,
    .height_def = LV_SIZE_CONTENT,
    .instance_size = sizeof(lv_pro_media_item_t),
    .base_class = &lv_obj_class
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t* lv_pro_media_item_create(lv_obj_t *parent) {
    LV_LOG_INFO("begin");
    lv_obj_t *obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

/*=====================
 * Setter functions
 *====================*/

void lv_pro_media_item_set_src(lv_obj_t *obj, const lv_font_t *font_value,
        lv_style_t *bg_style, const char *playing_img, const char *name) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_pro_media_item_t *item = (lv_pro_media_item_t*) obj;

    lv_obj_update_layout(obj);
    lv_obj_set_size(item->btn, lv_obj_get_width(obj), lv_obj_get_height(obj));

    lv_obj_set_style_bg_color(item->btn, lv_color_hex(0x0000ff), 0);
    lv_obj_add_style(item->btn, bg_style, 0);
    lv_obj_set_style_bg_opa(item->btn, LV_OPA_50, LV_STATE_FOCUS_KEY);

    lv_obj_set_style_text_font(item->name, font_value, 0);
    lv_label_set_long_mode(item->name, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(item->name, lv_obj_get_width(obj) - 75);
    lv_label_set_text_fmt(item->name, "%s", name);
    lv_obj_set_style_text_color(item->name, lv_color_white(), 0);

    lv_obj_align(item->name, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_align(item->playing_img, LV_ALIGN_RIGHT_MID, -10, 0);

    lv_img_set_src(item->playing_img, playing_img);
    lv_obj_add_flag(item->playing_img, LV_OBJ_FLAG_HIDDEN);

    lv_obj_refresh_self_size(obj);

    lv_obj_invalidate(obj);
}

void lv_pro_media_item_set_play(lv_obj_t *obj, bool mode) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_pro_media_item_t *item = (lv_pro_media_item_t*) obj;

    if (item->mode == mode)
        return;

    item->mode = mode;

    if (mode) {
        lv_obj_clear_flag(item->playing_img, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_text_color(item->name, lv_color_make(0xff, 0x8a, 0x00), 0);
    } else {
        lv_obj_add_flag(item->playing_img, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_text_color(item->name, lv_color_white(), 0);
    }
}

lv_obj_t * lv_pro_media_info_create_text(lv_obj_t * parent, const lv_font_t *font_value,
        char * txt, char * value)
{
    lv_obj_t * obj = lv_btn_create(parent);
    lv_obj_set_size(obj, LV_PCT(100), LV_PCT(25));
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x0000ff), 0);
    lv_obj_add_style(obj, &lv_pro_res.set_bg_transp, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_50, LV_STATE_FOCUS_KEY);

    lv_obj_t * text_label = NULL;
    lv_obj_t * value_label = NULL;

    if(txt) {
        text_label = lv_label_create(obj);
        lv_obj_set_style_text_font(text_label, font_value, 0);
        lv_label_set_text(text_label, txt);
        lv_obj_set_style_text_color(text_label, lv_color_white(), 0);
        lv_label_set_long_mode(text_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_align(text_label, LV_ALIGN_LEFT_MID, LV_PCT(7), 0);
        lv_obj_set_width(text_label, LV_PCT(34));
    }

    if(value) {
        value_label = lv_label_create(obj);
        lv_obj_set_style_text_font(value_label, font_value, 0);
        lv_label_set_text(value_label, value);
        lv_obj_set_style_text_color(value_label, lv_color_white(), 0);
        lv_label_set_long_mode(value_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_align(value_label, LV_ALIGN_LEFT_MID, LV_PCT(48), 0);
        lv_obj_set_width(value_label, LV_PCT(45));
    }

    return obj;
}

/*=====================
 * Getter functions
 *====================*/

bool lv_pro_media_item_get_play(lv_obj_t *obj) {
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_pro_media_item_t *item = (lv_pro_media_item_t*) obj;
    return item->mode;
}

lv_obj_t* lv_pro_media_item_get_btn(lv_obj_t *obj) {
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_pro_media_item_t *item = (lv_pro_media_item_t*) obj;
    return item->btn;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_pro_media_item_constructor(const lv_obj_class_t *class_p,
        lv_obj_t *obj) {
    LV_UNUSED(class_p);LV_TRACE_OBJ_CREATE("begin");

    lv_pro_media_item_t *item = (lv_pro_media_item_t*) obj;

    item->btn = lv_btn_create(obj);
    item->name = lv_label_create(obj);
    item->playing_img = lv_img_create(obj);

    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(item->btn, LV_OBJ_FLAG_EVENT_BUBBLE);

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_pro_media_item_destructor(const lv_obj_class_t *class_p,
        lv_obj_t *obj) {
    LV_UNUSED(class_p);
    lv_pro_media_item_t *item = (lv_pro_media_item_t*) obj;
}
