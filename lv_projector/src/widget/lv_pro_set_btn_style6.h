
#ifndef LV_PRO_SET_BTN_STYLE6_H
#define LV_PRO_SET_BTN_STYLE6_H

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

/*Data of pro_btn_style6*/
typedef struct {
    lv_obj_t obj;

    lv_obj_t *img;
    lv_obj_t *name;
    lv_obj_t *mac_addr;
    lv_obj_t *state;
    lv_obj_t *btn;

    int sec;
    int rssi;
    char pwd[64];
    const void *src_img;

} lv_pro_set_btn_style6_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create a pro btn_style6 objects
 * @param par pointer to an object, it will be the parent of the new pro btn_style6
 * @return pointer to the created pro btn_style6
 */
lv_obj_t *lv_pro_set_btn_style6_create(lv_obj_t *parent);

/*=====================
 * Setter functions
 *====================*/

void lv_pro_set_btn_style6_src(lv_obj_t *obj, const char *src_img, lv_font_t *devname_font,
                               lv_color_t devname_font_color, const char *devname,
                               lv_font_t *mac_font, lv_color_t mac_font_color, const char *mac,
                               lv_font_t *devstate_font, lv_color_t devstate_font_color,
                               const char *devstate, lv_group_t *group);

void lv_pro_set_btn_style6_set_btn_icon(lv_obj_t *obj, const char *img);
void lv_pro_set_btn_style6_set_name_str(lv_obj_t *obj, const char *name);
void lv_pro_set_btn_style6_set_mac_addr_str(lv_obj_t *obj, const char *name);
void lv_pro_set_btn_style6_set_state_str(lv_obj_t *obj, const char *name);
void lv_pro_set_btn_style6_set_sec_value(lv_obj_t *obj, int sec);
void lv_pro_set_btn_style6_set_rssi_value(lv_obj_t *obj, int rssi);
void lv_pro_set_btn_style6_set_pwd(lv_obj_t *obj, char *pwd);

/*=====================
 * Getter functions
 *====================*/

char *lv_pro_set_btn_style6_get_name_str(lv_obj_t *obj);
char *lv_pro_set_btn_style6_get_mac_addr_str(lv_obj_t *obj);
char *lv_pro_set_btn_style6_get_state_str(lv_obj_t *obj);
int lv_pro_set_btn_style6_get_sec_value(lv_obj_t *obj);
int lv_pro_set_btn_style6_get_rssi_value(lv_obj_t *obj);
char *lv_pro_set_btn_style6_get_pwd(lv_obj_t *obj);
/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_PRO_SET_BTN_STYLE6_H*/
