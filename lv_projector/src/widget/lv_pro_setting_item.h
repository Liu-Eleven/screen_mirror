
#ifndef LV_PRO_SETTING_ITEM_H
#define LV_PRO_SETTING_ITEM_H

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

/*Data of pro_btn*/
typedef struct {
    lv_obj_t obj;

    lv_obj_t *btn;
    lv_obj_t *name;
} lv_pro_setting_item_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create a pro_setting item objects
 * @param par pointer to an object, it will be the parent of the new pro btn
 * @return pointer to the created pro btn
 */
lv_obj_t* lv_pro_setting_item_create(lv_obj_t *parent);

/*=====================
 * Setter functions
 *====================*/

void lv_pro_setting_item_set_src(lv_obj_t *obj, const lv_font_t *font_value,
        lv_style_t *bg_style, const char *name);

/*=====================
 * Getter functions
 *====================*/

lv_obj_t* lv_pro_setting_item_get_btn(lv_obj_t *obj);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif  /*LV_PRO_SETTING_ITEM_H*/
