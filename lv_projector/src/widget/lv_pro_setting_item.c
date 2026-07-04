/*
 * lv_pro_setting_item.c
 *
 */

#include "lv_pro_setting_item.h"
#include "lvgl/lvgl.h"

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &lv_pro_setting_item_class

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_pro_setting_item_constructor(const lv_obj_class_t *class_p,
        lv_obj_t *obj);
static void lv_pro_setting_item_destructor(const lv_obj_class_t *class_p,
        lv_obj_t *obj);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_pro_setting_item_class = {
    .constructor_cb = lv_pro_setting_item_constructor,
    .destructor_cb = lv_pro_setting_item_destructor,
    .width_def = LV_SIZE_CONTENT,
    .height_def = LV_SIZE_CONTENT,
    .instance_size = sizeof(lv_pro_setting_item_t),
    .base_class = &lv_obj_class
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t* lv_pro_setting_item_create(lv_obj_t *parent) {
    LV_LOG_INFO("begin");
    lv_obj_t *obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

void set_setting_item_outline_style(lv_obj_t *obj)
{
    lv_obj_set_style_bg_color(obj, lv_color_make(128, 255, 255), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_bg_opa(obj, LV_OPA_80, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_outline_width(obj, 2, 0);
    lv_obj_set_style_outline_color(obj, lv_color_hex(0xFFFFFFF), 0);
    lv_obj_set_style_outline_pad(obj, 4, 0);
    lv_obj_set_style_outline_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_outline_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
}

/*=====================
 * Setter functions
 *====================*/

void lv_pro_setting_item_set_src(lv_obj_t *obj, const lv_font_t *font_value,
        lv_style_t *bg_style, const char *name) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_pro_setting_item_t *item = (lv_pro_setting_item_t*) obj;

    lv_obj_update_layout(obj);
    lv_obj_set_size(item->btn, lv_obj_get_width(obj), lv_obj_get_height(obj));

    lv_obj_add_style(item->btn, bg_style, 0);
    lv_obj_set_style_text_font(item->btn, font_value, 0);
    set_setting_item_outline_style(item->btn);

    lv_label_set_long_mode(item->name, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(item->name, lv_obj_get_width(obj));
    lv_obj_set_style_text_align(item->name, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(item->name, font_value, 0);
    lv_label_set_text_fmt(item->name, "%s", name);
    lv_obj_set_style_text_color(item->name, lv_color_white(), 0);

    lv_obj_center(item->name);

    lv_obj_refresh_self_size(obj);

    lv_obj_invalidate(obj);
}

/*=====================
 * Getter functions
 *====================*/

lv_obj_t* lv_pro_setting_item_get_btn(lv_obj_t *obj) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_pro_setting_item_t *item = (lv_pro_setting_item_t*) obj;
    return item->btn;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_pro_setting_item_constructor(const lv_obj_class_t *class_p,
        lv_obj_t *obj) {
    LV_UNUSED(class_p);LV_TRACE_OBJ_CREATE("begin");

    lv_pro_setting_item_t *item = (lv_pro_setting_item_t*) obj;

    item->btn = lv_btn_create(obj);
    item->name = lv_label_create(obj);

    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(item->btn, LV_OBJ_FLAG_EVENT_BUBBLE);

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_pro_setting_item_destructor(const lv_obj_class_t *class_p,
        lv_obj_t *obj) {
    LV_UNUSED(class_p);
    lv_pro_setting_item_t *item = (lv_pro_setting_item_t*) obj;
}
