
#include "lv_pro_set_btn_style6.h"

#include "lv_drivers/indev/evdev.h"
#include "lvgl/lvgl.h"

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &lv_pro_set_btn_style6_class

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_pro_set_btn_style6_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void lv_pro_set_btn_style6_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_pro_set_btn_style6_class = {
    .constructor_cb = lv_pro_set_btn_style6_constructor,
    .destructor_cb = lv_pro_set_btn_style6_destructor,
    .width_def = LV_SIZE_CONTENT,
    .height_def = LV_SIZE_CONTENT,
    .instance_size = sizeof(lv_pro_set_btn_style6_t),
    .base_class = &lv_obj_class};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t *lv_pro_set_btn_style6_create(lv_obj_t *parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t *obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

/*=====================
 * Setter functions
 *====================*/

void lv_pro_set_btn_style6_src(lv_obj_t *obj, const char *src_img, lv_font_t *devname_font,
                               lv_color_t devname_font_color, const char *devname,
                               lv_font_t *mac_font, lv_color_t mac_font_color, const char *mac,
                               lv_font_t *devstate_font, lv_color_t devstate_font_color,
                               const char *devstate, lv_group_t *group)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_pro_set_btn_style6_t *btn = (lv_pro_set_btn_style6_t *)obj;

    lv_obj_update_layout(obj);

    // 主框
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(obj, 5, LV_PART_MAIN);
    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_outline_width(obj, 4, LV_PART_MAIN);
    lv_obj_set_style_outline_color(obj, lv_color_hex(0xFFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_outline_pad(obj, 4, LV_PART_MAIN);
    lv_obj_set_style_outline_opa(obj, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_outline_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

    // 左侧图标
    btn->src_img = src_img;
    lv_img_set_src(btn->img, btn->src_img);
    lv_obj_align(btn->img, LV_ALIGN_LEFT_MID, 30, 0);

    // 左侧devname
    lv_label_set_long_mode(btn->name, LV_LABEL_LONG_SCROLL);
    lv_obj_set_style_text_font(btn->name, devname_font, LV_PART_MAIN);
    lv_label_set_recolor(btn->name, true);
    lv_obj_set_style_text_color(btn->name, devname_font_color, LV_PART_MAIN);
    lv_label_set_text(btn->name, devname);
    lv_obj_align_to(btn->name, btn->img, LV_ALIGN_OUT_RIGHT_MID, 25, 0);

    // 右侧state
    lv_obj_set_style_text_font(btn->state, devstate_font, LV_PART_MAIN);
    lv_label_set_recolor(btn->state, true);
    lv_obj_set_style_text_color(btn->state, devstate_font_color, LV_PART_MAIN);
    lv_label_set_text(btn->state, devstate);
    lv_obj_align(btn->state, LV_ALIGN_RIGHT_MID, -20, 0);

    // 中间devmac
    lv_obj_set_style_text_font(btn->mac_addr, mac_font, LV_PART_MAIN);
    lv_label_set_recolor(btn->mac_addr, true);
    lv_obj_set_style_text_color(btn->mac_addr, mac_font_color, LV_PART_MAIN);
    lv_label_set_text(btn->mac_addr, mac);
    lv_obj_align_to(btn->mac_addr, btn->state, LV_ALIGN_OUT_LEFT_MID, -5, 0);
    lv_obj_add_flag(btn->mac_addr, LV_OBJ_FLAG_HIDDEN);

    // 按钮
    lv_obj_set_style_border_width(btn->btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn->btn, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn->btn, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_align(btn->btn, LV_ALIGN_RIGHT_MID, -20, 0);

    memset(btn->pwd, '\0', sizeof(btn->pwd));

    lv_group_remove_obj(obj);
    lv_group_remove_obj(btn->img);
    lv_group_remove_obj(btn->name);
    lv_group_remove_obj(btn->mac_addr);
    lv_group_remove_obj(btn->state);
    lv_group_remove_obj(btn->btn);

    if (group != NULL) {
        lv_group_add_obj(group, obj);
        // 添加组先取消focuse属性
        lv_group_set_editing(group, false);
        lv_res_t res = lv_event_send(btn, LV_EVENT_DEFOCUSED, NULL);
        if (res == LV_RES_OK) lv_obj_invalidate(obj);
    }

    lv_obj_refresh_self_size(obj);
    lv_obj_invalidate(obj);
}

void lv_pro_set_btn_style6_set_btn_icon(lv_obj_t *obj, const char *img)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_pro_set_btn_style6_t *btn = (lv_pro_set_btn_style6_t *)obj;
    btn->src_img = img;
    lv_img_set_src(btn->img, btn->src_img);
}

void lv_pro_set_btn_style6_set_name_str(lv_obj_t *obj, const char *name)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_pro_set_btn_style6_t *btn = (lv_pro_set_btn_style6_t *)obj;
    lv_label_set_text(btn->name, name);
}

void lv_pro_set_btn_style6_set_mac_addr_str(lv_obj_t *obj, const char *name)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_pro_set_btn_style6_t *btn = (lv_pro_set_btn_style6_t *)obj;
    lv_label_set_text(btn->mac_addr, name);
}

void lv_pro_set_btn_style6_set_state_str(lv_obj_t *obj, const char *name)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_pro_set_btn_style6_t *btn = (lv_pro_set_btn_style6_t *)obj;
    lv_label_set_text(btn->state, name);
}

void lv_pro_set_btn_style6_set_sec_value(lv_obj_t *obj, int sec)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_pro_set_btn_style6_t *btn = (lv_pro_set_btn_style6_t *)obj;
    btn->sec = sec;
}

void lv_pro_set_btn_style6_set_rssi_value(lv_obj_t *obj, int rssi)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_pro_set_btn_style6_t *btn = (lv_pro_set_btn_style6_t *)obj;
    btn->rssi = rssi;
}

void lv_pro_set_btn_style6_set_pwd(lv_obj_t *obj, char *pwd)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_pro_set_btn_style6_t *btn = (lv_pro_set_btn_style6_t *)obj;
    snprintf(btn->pwd, sizeof(btn->pwd), "%s", pwd);
}

/*=====================
 * Getter functions
 *====================*/

char *lv_pro_set_btn_style6_get_name_str(lv_obj_t *obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_pro_set_btn_style6_t *btn = (lv_pro_set_btn_style6_t *)obj;
    return lv_label_get_text(btn->name);
}

char *lv_pro_set_btn_style6_get_mac_addr_str(lv_obj_t *obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_pro_set_btn_style6_t *btn = (lv_pro_set_btn_style6_t *)obj;
    return lv_label_get_text(btn->mac_addr);
}

char *lv_pro_set_btn_style6_get_state_str(lv_obj_t *obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_pro_set_btn_style6_t *btn = (lv_pro_set_btn_style6_t *)obj;
    return lv_label_get_text(btn->state);
}

int lv_pro_set_btn_style6_get_sec_value(lv_obj_t *obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_pro_set_btn_style6_t *btn = (lv_pro_set_btn_style6_t *)obj;
    return btn->sec;
}

int lv_pro_set_btn_style6_get_rssi_value(lv_obj_t *obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_pro_set_btn_style6_t *btn = (lv_pro_set_btn_style6_t *)obj;
    return btn->rssi;
}

char *lv_pro_set_btn_style6_get_pwd(lv_obj_t *obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_pro_set_btn_style6_t *btn = (lv_pro_set_btn_style6_t *)obj;
    return btn->pwd;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_pro_set_btn_style6_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_pro_set_btn_style6_t *btn = (lv_pro_set_btn_style6_t *)obj;

    btn->img = lv_img_create(obj);
    btn->name = lv_label_create(obj);
    btn->mac_addr = lv_label_create(obj);
    btn->state = lv_label_create(obj);
    btn->btn = lv_btn_create(obj);

    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_add_flag(btn->btn, LV_OBJ_FLAG_EVENT_BUBBLE);

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_pro_set_btn_style6_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    LV_UNUSED(class_p);
    lv_pro_set_btn_style6_t *btn = (lv_pro_set_btn_style6_t *)obj;
}
