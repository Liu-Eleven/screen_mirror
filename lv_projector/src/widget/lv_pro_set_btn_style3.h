
#ifndef LV_PRO_SET_BTN_STYLE3_H
#define LV_PRO_SET_BTN_STYLE3_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lvgl/src/core/lv_obj.h"
#include "lvgl/src/lv_conf_internal.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/*Data of pro_btn_style3*/
typedef struct {
    lv_obj_t obj;

    lv_obj_t *img;
    lv_obj_t *btn;

    const void *src_on_img;
    const void *src_off_img;
} lv_pro_set_btn_style3_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create a pro btn_style3 objects
 * @param par pointer to an object, it will be the parent of the new pro btn_style3
 * @return pointer to the created pro btn_style3
 */
lv_obj_t *lv_pro_set_btn_style3_create(lv_obj_t *parent);

/*=====================
 * Setter functions
 *====================*/

void lv_pro_set_btn_style3_src(lv_obj_t *obj, lv_color_t bg_color, lv_color_t btn_color,
                               const char *src_on_img, const char *src_off_img, bool state,
                               lv_group_t *group);

/*=====================
 * Getter functions
 *====================*/

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_PRO_SET_BTN_STYLE3_H*/
