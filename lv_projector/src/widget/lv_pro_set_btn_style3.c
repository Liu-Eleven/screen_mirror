
#include "lv_pro_set_btn_style3.h"

#include "lv_drivers/indev/evdev.h"
#include "lvgl/lvgl.h"

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &lv_pro_set_btn_style3_class

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_pro_set_btn_style3_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void lv_pro_set_btn_style3_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_pro_set_btn_style3_class = {
    .constructor_cb = lv_pro_set_btn_style3_constructor,
    .destructor_cb = lv_pro_set_btn_style3_destructor,
    .width_def = LV_SIZE_CONTENT,
    .height_def = LV_SIZE_CONTENT,
    .instance_size = sizeof(lv_pro_set_btn_style3_t),
    .base_class = &lv_obj_class};

/**********************
 *      MACROS
 **********************/

static void lv_pro_btn_style3_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t *key = lv_event_get_param(e);
    lv_pro_set_btn_style3_t *btn = (lv_pro_set_btn_style3_t *)lv_event_get_target(e);

    if (code == LV_EVENT_KEY) {
        if (*key == LV_KEY_ENTER) {
            // Check if the button is currently checked
            if (lv_obj_has_state(btn->btn, LV_STATE_CHECKED)) {
                // If checked, uncheck and set the off image
                lv_obj_clear_state(btn->btn, LV_STATE_CHECKED);
                lv_img_set_src(btn->img, btn->src_off_img);
            } else {
                // If not checked, check and set the on image
                lv_obj_add_state(btn->btn, LV_STATE_CHECKED);
                lv_img_set_src(btn->img, btn->src_on_img);
            }
        }
    }
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t *lv_pro_set_btn_style3_create(lv_obj_t *parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t *obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

/*=====================
 * Setter functions
 *====================*/

void lv_pro_set_btn_style3_src(lv_obj_t *obj, lv_color_t bg_color, lv_color_t btn_color,
                               const char *src_on_img, const char *src_off_img, bool state,
                               lv_group_t *group)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_pro_set_btn_style3_t *btn = (lv_pro_set_btn_style3_t *)obj;

    lv_obj_update_layout(obj);

    // 开关方框
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(obj, 5, LV_PART_MAIN);
    lv_obj_set_style_radius(obj, 10, LV_PART_MAIN);
    lv_obj_set_style_bg_color(obj, bg_color, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_outline_width(obj, 4, LV_PART_MAIN);
    lv_obj_set_style_outline_color(obj, lv_color_hex(0xFFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_outline_pad(obj, 4, LV_PART_MAIN);
    lv_obj_set_style_outline_opa(obj, LV_OPA_TRANSP, LV_PART_MAIN);

    lv_obj_set_style_bg_color(obj, bg_color, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    btn->src_on_img = src_on_img;
    btn->src_off_img = src_off_img;

    // 开关logo
    if (state) {
        lv_img_set_src(btn->img, btn->src_on_img);
    } else {
        lv_img_set_src(btn->img, btn->src_off_img);
    }
    lv_obj_align(btn->img, LV_ALIGN_LEFT_MID, 30, 0);

    // 开关按钮
    lv_obj_set_style_bg_color(btn->btn, btn_color, LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(btn->btn, lv_color_hex(0x808080),
                              LV_PART_INDICATOR | LV_STATE_DEFAULT);

    if (state) {
        lv_obj_add_state(btn->btn, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(btn->btn, LV_STATE_CHECKED);
    }
    lv_obj_align(btn->btn, LV_ALIGN_RIGHT_MID, -20, 0);
    lv_obj_refresh_style(btn->btn, LV_PART_ANY, LV_STYLE_PROP_ANY);

    lv_obj_add_event_cb(obj, lv_pro_btn_style3_event_cb, LV_EVENT_KEY, NULL);
    lv_group_remove_obj(obj);
    lv_group_remove_obj(btn->img);
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

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_pro_set_btn_style3_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_pro_set_btn_style3_t *btn = (lv_pro_set_btn_style3_t *)obj;

    lv_obj_remove_style_all(obj);

    btn->img = lv_img_create(obj);
    btn->btn = lv_switch_create(obj);

    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(btn->btn, LV_OBJ_FLAG_EVENT_BUBBLE);

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_pro_set_btn_style3_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    LV_UNUSED(class_p);
    lv_pro_set_btn_style3_t *btn = (lv_pro_set_btn_style3_t *)obj;
}
