
#ifndef LV_PRO_SET_BTN_STYLE4_H
#define LV_PRO_SET_BTN_STYLE4_H

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

/*Data of pro_btn_style4*/
typedef struct {
    lv_obj_t obj;

    lv_obj_t *img;
    lv_obj_t *btn;
    lv_obj_t *name;

} lv_pro_set_btn_style4_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create a pro btn_style4 objects
 * @param par pointer to an object, it will be the parent of the new pro btn_style4
 * @return pointer to the created pro btn_style4
 */
lv_obj_t *lv_pro_set_btn_style4_create(lv_obj_t *parent);

/*=====================
 * Setter functions
 *====================*/

void lv_pro_set_btn_style4_src(lv_obj_t *obj, const char *src_img, lv_font_t *name_font,
                               const char *btn_name, lv_group_t *group);
/*=====================
 * Getter functions
 *====================*/

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_PRO_SET_BTN_STYLE4_H*/
