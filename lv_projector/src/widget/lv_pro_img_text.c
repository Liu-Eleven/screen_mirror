
#include "lv_pro_img_text.h"
#include "lvgl/lvgl.h"
#include "lv_pro_res.h"
#include "lv_common.h"

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &lv_pro_img_text_class

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_pro_img_text_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void lv_pro_img_text_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void lv_pro_img_text_event(lv_event_t *e);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_pro_img_text_class = {
    .constructor_cb = lv_pro_img_text_constructor,
    .destructor_cb = lv_pro_img_text_destructor,
    .width_def = LV_SIZE_CONTENT,
    .height_def = LV_SIZE_CONTENT,
    .instance_size = sizeof(lv_pro_img_text_t),
    .base_class = &lv_obj_class
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t* lv_pro_img_text_create(lv_obj_t *parent) {
    LV_LOG_INFO("begin");
    lv_obj_t *obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

/*=====================
 * Setter functions
 *====================*/

void lv_pro_set_img_text_src(lv_obj_t *obj, const char *src_img, const char *top_title,
        const char *bottom_text, const uint32_t text_len) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_obj_align(obj, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_pro_img_text_t *btn = (lv_pro_img_text_t*) obj;
    lv_obj_update_layout(obj);

    lv_obj_set_size(btn->cont, lv_obj_get_width(obj), lv_obj_get_height(obj));
    lv_obj_set_style_bg_opa(btn->cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn->cont, 0, 0);
    lv_obj_clear_flag(btn->cont, LV_OBJ_FLAG_SCROLLABLE);

    lv_img_set_src(btn->img, src_img);
    lv_obj_align(btn->img, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_set_style_text_font(btn->top_label, &GENERAL_FONT_MID, 0);
    lv_label_set_recolor(btn->top_label, true);
    lv_label_set_text_fmt(btn->top_label, "#ffffff %s #", top_title);
    lv_obj_align_to(btn->top_label, btn->img, LV_ALIGN_OUT_RIGHT_TOP, 10, 0);

    lv_obj_set_style_text_font(btn->bottom_label, &GENERAL_FONT_MID, 0);
    lv_label_set_long_mode(btn->bottom_label, LV_LABEL_LONG_WRAP);
    lv_label_set_recolor(btn->bottom_label, true);
    lv_label_set_text(btn->bottom_label, bottom_text);
    lv_obj_add_style(btn->bottom_label, &lv_pro_res.label_white, 0);
    lv_obj_set_width(btn->bottom_label, text_len);
    lv_obj_align_to(btn->bottom_label, btn->top_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 20);

    lv_obj_refresh_self_size(obj);
    lv_obj_invalidate(obj);
}

/*=====================
 * Getter functions
 *====================*/

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_pro_img_text_constructor(const lv_obj_class_t *class_p,
        lv_obj_t *obj) {
    LV_UNUSED(class_p);LV_TRACE_OBJ_CREATE("begin");

    lv_pro_img_text_t *btn = (lv_pro_img_text_t*) obj;

    btn->cont = lv_obj_create(obj);
    btn->img = lv_img_create(btn->cont);
    btn->top_label = lv_label_create(btn->cont);
    btn->bottom_label = lv_label_create(btn->cont);

    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_pro_img_text_destructor(const lv_obj_class_t *class_p,
        lv_obj_t *obj) {
    LV_UNUSED(class_p);
    lv_pro_img_text_t *btn = (lv_pro_img_text_t*) obj;
}
