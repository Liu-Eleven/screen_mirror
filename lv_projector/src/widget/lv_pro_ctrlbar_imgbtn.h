#ifndef LV_PRO_CTRLBAR_IMGBTN_H
#define LV_PRO_CTRLBAR_IMGBTN_H

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

/*Data of box_imgbtn*/
typedef struct {
    lv_obj_t obj;

    lv_obj_t *btn;
    lv_obj_t *img;
    lv_obj_t *name;
} lv_pro_ctrlbar_imgbtn_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create a box btn objects
 * @param par pointer to an object, it will be the parent of the new box btn
 * @return pointer to the created box btn
 */
lv_obj_t* lv_pro_ctrlbar_imgbtn_create(lv_obj_t *parent);

/*=====================
 * Setter functions
 *====================*/

void lv_pro_ctrlbar_imgbtn_set_src(lv_obj_t *obj,
        const lv_img_dsc_t *src_img, const char *name,
        const lv_font_t *font_value, lv_style_t *bg_style);

/*=====================
 * Getter functions
 *====================*/

lv_obj_t* lv_pro_ctrlbar_imgbtn_get_btn(lv_obj_t *obj);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif  /*LV_PRO_CTRLBAR_IMGBTN_H*/
