
#ifndef LV_PRO_IMG_TEXT_H
#define LV_PRO_IMG_TEXT_H

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

/*Data of pro_img_text*/
typedef struct {
    lv_obj_t obj;

    lv_obj_t *cont;
    lv_obj_t *img;
    lv_obj_t *top_label;
    lv_obj_t *bottom_label;
} lv_pro_img_text_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create a pro img_text objects
 * @param par pointer to an object, it will be the parent of the new pro img_text
 * @return pointer to the created pro img_text
 */
lv_obj_t* lv_pro_img_text_create(lv_obj_t *parent);

/*=====================
 * Setter functions
 *====================*/

void lv_pro_set_img_text_src(lv_obj_t *obj, const char *src_img, const char *top_title,
        const char *bottom_text, const uint32_t text_len);

/*=====================
 * Getter functions
 *====================*/

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif  /*LV_PRO_IMG_TEXT_H*/
