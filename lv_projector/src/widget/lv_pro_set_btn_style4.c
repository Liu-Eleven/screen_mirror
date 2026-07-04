
#include "lv_pro_set_btn_style4.h"

#include "lv_drivers/indev/evdev.h"
#include "lvgl/lvgl.h"

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &lv_pro_set_btn_style4_class

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_pro_set_btn_style4_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void lv_pro_set_btn_style4_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_pro_set_btn_style4_class = {
    .constructor_cb = lv_pro_set_btn_style4_constructor,
    .destructor_cb = lv_pro_set_btn_style4_destructor,
    .width_def = LV_SIZE_CONTENT,
    .height_def = LV_SIZE_CONTENT,
    .instance_size = sizeof(lv_pro_set_btn_style4_t),
    .base_class = &lv_obj_class};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t *lv_pro_set_btn_style4_create(lv_obj_t *parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t *obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

/*=====================
 * Setter functions
 *====================*/

void lv_pro_set_btn_style4_src(lv_obj_t *obj, const char *src_img, lv_font_t *name_font,
                               const char *btn_name, lv_group_t *group)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_pro_set_btn_style4_t *btn = (lv_pro_set_btn_style4_t *)obj;

    lv_obj_update_layout(obj);

    // 按钮方框
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(obj, 5, LV_PART_MAIN);
    lv_obj_set_style_radius(obj, 10, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_outline_width(obj, 4, LV_PART_MAIN);
    lv_obj_set_style_outline_color(obj, lv_color_hex(0xFFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_outline_pad(obj, 4, LV_PART_MAIN);
    lv_obj_set_style_outline_opa(obj, LV_OPA_TRANSP, LV_PART_MAIN);

    lv_obj_set_style_outline_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    // 按钮logo
    lv_img_set_src(btn->img, src_img);
    lv_obj_align(btn->img, LV_ALIGN_LEFT_MID, 30, 0);

    // 按钮
    lv_obj_set_style_border_width(btn->btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn->btn, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn->btn, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_align(btn->btn, LV_ALIGN_RIGHT_MID, -20, 0);

    // 按钮名称
    lv_obj_set_style_text_font(btn->name, name_font, LV_PART_MAIN);
    lv_obj_set_style_text_color(btn->name, lv_color_white(), 0);
    lv_label_set_text(btn->name, btn_name);
    lv_obj_align(btn->name, LV_ALIGN_RIGHT_MID, -20, 0);

    lv_group_remove_obj(obj);
    lv_group_remove_obj(btn->img);
    lv_group_remove_obj(btn->btn);
    lv_group_remove_obj(btn->name);

    if (group != NULL) {
        lv_group_add_obj(group, obj);
    }

    lv_obj_refresh_self_size(obj);
    lv_obj_invalidate(obj);
}

/*=====================
 * Getter functions
 *====================*/

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_pro_set_btn_style4_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_pro_set_btn_style4_t *btn = (lv_pro_set_btn_style4_t *)obj;

    lv_obj_remove_style_all(obj);

    btn->img = lv_img_create(obj);
    btn->btn = lv_btn_create(obj);
    btn->name = lv_label_create(obj);

    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(btn->btn, LV_OBJ_FLAG_EVENT_BUBBLE);

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_pro_set_btn_style4_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    LV_UNUSED(class_p);
    lv_pro_set_btn_style4_t *btn = (lv_pro_set_btn_style4_t *)obj;
}
