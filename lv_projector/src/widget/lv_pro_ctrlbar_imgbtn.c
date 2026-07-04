#include "lv_pro_ctrlbar_imgbtn.h"
#include "lvgl/lvgl.h"

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &lv_pro_ctrlbar_imgbtn_class

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_pro_ctrlbar_imgbtn_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void lv_pro_ctrlbar_imgbtn_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void lv_pro_ctrlbar_imgbtn_event(lv_event_t *e);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_pro_ctrlbar_imgbtn_class = {
    .constructor_cb = lv_pro_ctrlbar_imgbtn_constructor,
    .destructor_cb = lv_pro_ctrlbar_imgbtn_destructor,
    .width_def = LV_SIZE_CONTENT,
    .height_def = LV_SIZE_CONTENT,
    .instance_size = sizeof(lv_pro_ctrlbar_imgbtn_t),
    .base_class = &lv_obj_class
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t* lv_pro_ctrlbar_imgbtn_create(lv_obj_t *parent) {
    LV_LOG_INFO("begin");
    lv_obj_t *obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

/*=====================
 * Setter functions
 *====================*/

void lv_pro_ctrlbar_imgbtn_set_src(lv_obj_t *obj,
        const lv_img_dsc_t *src_img, const char *name,
        const lv_font_t *font_value, lv_style_t *bg_style) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    set_obj_default_outline_style(obj);

    lv_pro_ctrlbar_imgbtn_t *btn = (lv_pro_ctrlbar_imgbtn_t*) obj;

    lv_obj_update_layout(obj);
    lv_obj_set_size(btn->btn, lv_obj_get_width(obj), lv_obj_get_height(obj));
    lv_obj_add_style(btn->btn, bg_style, 0);

    lv_img_set_src(btn->img, src_img);

    lv_label_set_text_fmt(btn->name, "%s", name);
    lv_obj_set_style_text_color(btn->name, lv_color_white(), 0);
    lv_obj_set_style_text_font(btn->name, font_value, 0);

    lv_obj_align(btn->img, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_align(btn->name, LV_ALIGN_BOTTOM_MID, 0, -5);

    lv_obj_refresh_self_size(obj);

    lv_obj_invalidate(obj);
}

/*=====================
 * Getter functions
 *====================*/

lv_obj_t* lv_pro_ctrlbar_imgbtn_get_btn(lv_obj_t *obj) {
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_pro_ctrlbar_imgbtn_t *btn = (lv_pro_ctrlbar_imgbtn_t*) obj;
    return btn->btn;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_pro_ctrlbar_imgbtn_constructor(const lv_obj_class_t *class_p,
        lv_obj_t *obj) {
    LV_UNUSED(class_p);LV_TRACE_OBJ_CREATE("begin");

    lv_pro_ctrlbar_imgbtn_t *btn = (lv_pro_ctrlbar_imgbtn_t*) obj;
    btn->btn = lv_btn_create(obj);
    btn->img = lv_img_create(obj);
    btn->name = lv_label_create(obj);

    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(btn->btn, LV_OBJ_FLAG_EVENT_BUBBLE);

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_pro_ctrlbar_imgbtn_destructor(const lv_obj_class_t *class_p,
        lv_obj_t *obj) {
    LV_UNUSED(class_p);
    lv_pro_ctrlbar_imgbtn_t *btn = (lv_pro_ctrlbar_imgbtn_t*) obj;
}
