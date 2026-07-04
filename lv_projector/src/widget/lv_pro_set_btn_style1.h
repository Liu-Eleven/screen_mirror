
#ifndef LV_PRO_SET_BTN_STYLE1_H
#define LV_PRO_SET_BTN_STYLE1_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lvgl/src/lv_conf_internal.h"
#include "lvgl/src/core/lv_obj.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/*Data of pro_btn_style1*/
typedef struct {
    lv_obj_t obj;

    lv_obj_t *btn;
    lv_obj_t *name;
    lv_obj_t *img;
} lv_pro_set_btn_style1_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create a pro btn_style1 objects
 * @param par pointer to an object, it will be the parent of the new pro btn_style1
 * @return pointer to the created pro btn_style1
 */
lv_obj_t* lv_pro_set_btn_style1_create(lv_obj_t *parent);

/*=====================
 * Setter functions
 *====================*/

void lv_pro_set_btn_style1_src(lv_obj_t *obj, const lv_font_t *font_value,
        lv_color_t bg_color, lv_opa_t bg_opa, const char *src_img, const char *name);

void lv_pro_set_btn_style1_set_name(lv_obj_t *obj, const char *name);

/*=====================
 * Getter functions
 *====================*/

char* lv_pro_set_btn_style1_get_name(lv_obj_t *obj);
lv_obj_t* lv_pro_set_btn_style1_get_btn(lv_obj_t *obj);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif  /*LV_PRO_SET_BTN_STYLE1_H*/
