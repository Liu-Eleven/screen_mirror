
#include "lv_pro_set_btn_style5.h"

#include "lv_drivers/indev/evdev.h"
#include "lvgl/lvgl.h"

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &lv_pro_set_btn_style5_class

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_pro_set_btn_style5_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void lv_pro_set_btn_style5_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_pro_set_btn_style5_class = {
    .constructor_cb = lv_pro_set_btn_style5_constructor,
    .destructor_cb = lv_pro_set_btn_style5_destructor,
    .width_def = LV_SIZE_CONTENT,
    .height_def = LV_SIZE_CONTENT,
    .instance_size = sizeof(lv_pro_set_btn_style5_t),
    .base_class = &lv_obj_class};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t *lv_pro_set_btn_style5_create(lv_obj_t *parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t *obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

/*=====================
 * Setter functions
 *====================*/

void lv_pro_set_btn_style5_src(lv_obj_t *obj, lv_font_t *left_font, lv_color_t left_font_color,
                               const char *left_title_name, lv_font_t *right_font,
                               lv_color_t right_font_color, const char *right_content_name,
                               lv_group_t *group)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_pro_set_btn_style5_t *btn = (lv_pro_set_btn_style5_t *)obj;

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

    // 左侧title name
    lv_obj_set_style_text_font(btn->left_title, left_font, LV_PART_MAIN);
    lv_label_set_recolor(btn->left_title, true);
    lv_obj_set_style_text_color(btn->left_title, left_font_color, LV_PART_MAIN);
    lv_label_set_text(btn->left_title, left_title_name);
    lv_obj_align(btn->left_title, LV_ALIGN_LEFT_MID, 30, 0);

    // 右侧content
    lv_obj_set_style_text_font(btn->rigth_content, right_font, LV_PART_MAIN);
    lv_label_set_recolor(btn->rigth_content, true);
    lv_obj_set_style_text_color(btn->rigth_content, right_font_color, LV_PART_MAIN);
    lv_label_set_text(btn->rigth_content, right_content_name);
    lv_obj_align(btn->rigth_content, LV_ALIGN_RIGHT_MID, -30, 0);

    // 按钮
    lv_obj_set_size(btn->btn, lv_obj_get_width(obj), lv_obj_get_height(obj));
    lv_obj_set_style_border_width(btn->btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn->btn, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn->btn, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_align(btn->btn, LV_ALIGN_CENTER, 0, 0);

    lv_group_remove_obj(obj);
    lv_group_remove_obj(btn->left_title);
    lv_group_remove_obj(btn->rigth_content);
    lv_group_remove_obj(btn->btn);

    if (group != NULL) {
        lv_group_add_obj(group, obj);
    }

    lv_obj_refresh_self_size(obj);
    lv_obj_invalidate(obj);
}

/*=====================
 * Getter functions
 *====================*/
lv_obj_t *lv_pro_set_btn_style5_get_right_content_obj(lv_obj_t *obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_pro_set_btn_style5_t *btn = (lv_pro_set_btn_style5_t *)obj;
    return btn->rigth_content;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_pro_set_btn_style5_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_pro_set_btn_style5_t *btn = (lv_pro_set_btn_style5_t *)obj;

    lv_obj_remove_style_all(obj);

    btn->left_title = lv_label_create(obj);
    btn->rigth_content = lv_label_create(obj);
    btn->btn = lv_btn_create(obj);

    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(btn->btn, LV_OBJ_FLAG_EVENT_BUBBLE);

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_pro_set_btn_style5_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    LV_UNUSED(class_p);
    lv_pro_set_btn_style5_t *btn = (lv_pro_set_btn_style5_t *)obj;
}
