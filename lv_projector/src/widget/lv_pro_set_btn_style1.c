
#include "lv_pro_set_btn_style1.h"
#include "lvgl/lvgl.h"
#include "lv_drivers/indev/evdev.h"
#include "lv_pro_res.h"
#include "lv_common.h"

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &lv_pro_set_btn_style1_class

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_pro_set_btn_style1_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void lv_pro_set_btn_style1_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_pro_set_btn_style1_class = {
    .constructor_cb = lv_pro_set_btn_style1_constructor,
    .destructor_cb = lv_pro_set_btn_style1_destructor,
    .width_def = LV_SIZE_CONTENT,
    .height_def = LV_SIZE_CONTENT,
    .instance_size = sizeof(lv_pro_set_btn_style1_t),
    .base_class = &lv_obj_class
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t* lv_pro_set_btn_style1_create(lv_obj_t *parent) {
    LV_LOG_INFO("begin");
    lv_obj_t *obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

/*=====================
 * Setter functions
 *====================*/

void lv_pro_set_btn_style1_src(lv_obj_t *obj, const lv_font_t *font_value,
        lv_color_t bg_color, lv_opa_t bg_opa, const char *src_img, const char *name) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    int radius_val = 10;
    lv_obj_set_style_radius(obj, radius_val, 0);
    set_obj_default_outline_style(obj);

    lv_pro_set_btn_style1_t *btn = (lv_pro_set_btn_style1_t*) obj;
    lv_obj_update_layout(obj);

    lv_obj_set_size(btn->btn, lv_obj_get_width(obj), lv_obj_get_height(obj));
    lv_obj_set_style_bg_color(btn->btn, bg_color, 0);
    lv_obj_set_style_bg_opa(btn->btn, bg_opa, 0);
    lv_obj_set_style_border_width(btn->btn, 0, 0);
    lv_obj_set_style_shadow_width(btn->btn, 0, 0);
    lv_obj_set_style_radius(btn->btn, radius_val, 0);

    lv_img_set_src(btn->img, src_img);
    lv_obj_align(btn->img, LV_ALIGN_CENTER, 0, -20);

    lv_label_set_text(btn->name, name);
    lv_obj_add_style(btn->name, &lv_pro_res.label_white, 0);
    lv_label_set_long_mode(btn->name, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(btn->name, font_value, 0);
    lv_obj_set_width(btn->name, LV_PCT(90));
    lv_obj_set_style_text_align(btn->name, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align_to(btn->name, btn->img, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    lv_obj_refresh_self_size(obj);
    lv_obj_invalidate(obj);
}

void lv_pro_set_btn_style1_set_name(lv_obj_t *obj, const char *name) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_pro_set_btn_style1_t *btn = (lv_pro_set_btn_style1_t*) obj;
    lv_label_set_text_fmt(btn->name, "#ffffff %s #", name);
}

/*=====================
 * Getter functions
 *====================*/

char* lv_pro_set_btn_style1_get_name(lv_obj_t *obj) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_pro_set_btn_style1_t *btn = (lv_pro_set_btn_style1_t*) obj;
    return lv_label_get_text(btn->name);
}

lv_obj_t* lv_pro_set_btn_style1_get_btn(lv_obj_t *obj) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_pro_set_btn_style1_t *btn = (lv_pro_set_btn_style1_t*) obj;
    return btn->btn;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_pro_set_btn_style1_constructor(const lv_obj_class_t *class_p,
        lv_obj_t *obj) {
    LV_UNUSED(class_p);LV_TRACE_OBJ_CREATE("begin");

    lv_pro_set_btn_style1_t *btn = (lv_pro_set_btn_style1_t*) obj;

    btn->btn = lv_btn_create(obj);
    btn->name = lv_label_create(obj);
    btn->img = lv_img_create(obj);

    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(btn->btn, LV_OBJ_FLAG_EVENT_BUBBLE);

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_pro_set_btn_style1_destructor(const lv_obj_class_t *class_p,
        lv_obj_t *obj) {
    LV_UNUSED(class_p);
    lv_pro_set_btn_style1_t *btn = (lv_pro_set_btn_style1_t*) obj;
}
