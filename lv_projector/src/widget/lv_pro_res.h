
#ifndef LV_PRO_RES_H
#define LV_PRO_RES_H

#include "lvgl/lvgl.h"
#include <pthread.h>

typedef struct {
    lv_style_t set_bg_transp;
    lv_style_t set_bg_grey;
    lv_style_t set_bg_black;
    lv_style_t set_bg_blue;

    lv_style_t label_white;
} lv_pro_res_t;

extern lv_pro_res_t lv_pro_res;
extern pthread_mutex_t lvgl_mutex;

void lv_pro_res_init(void);

#endif  /*LV_PRO_RES_H*/
