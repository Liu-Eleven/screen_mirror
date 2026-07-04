
#ifndef LV_PRO_SET_BTN_STYLE5_H
#define LV_PRO_SET_BTN_STYLE5_H

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

/*Data of pro_btn_style5*/
typedef struct {
    lv_obj_t obj;

    lv_obj_t *left_title;
    lv_obj_t *rigth_content;
    lv_obj_t *btn;

} lv_pro_set_btn_style5_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create a pro btn_style5 objects
 * @param par pointer to an object, it will be the parent of the new pro btn_style5
 * @return pointer to the created pro btn_style5
 */
lv_obj_t *lv_pro_set_btn_style5_create(lv_obj_t *parent);

/*=====================
 * Setter functions
 *====================*/

void lv_pro_set_btn_style5_src(lv_obj_t *obj, lv_font_t *left_font, lv_color_t left_font_color,
                               const char *left_title_name, lv_font_t *right_font,
                               lv_color_t right_font_color, const char *right_content_name,
                               lv_group_t *group);
/*=====================
 * Getter functions
 *====================*/

lv_obj_t *lv_pro_set_btn_style5_get_right_content_obj(lv_obj_t *obj);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_PRO_SET_BTN_STYLE5_H*/
