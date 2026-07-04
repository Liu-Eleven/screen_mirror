
#ifndef LV_PRO_PICTURE_ACTIVITY_H
#define LV_PRO_PICTURE_ACTIVITY_H

#include "lvgl/lvgl.h"
#include "../lv_pro_media_layout.h"

extern lv_obj_t *picture_activity;
extern lv_obj_t *picture_cont;
extern lv_obj_t *picture_img;
extern lv_group_t *picture_group;

void lv_pro_picture_ui_init(void);
void lv_pro_picture_exit(void);

#endif  /*LV_PRO_PICTURE_ACTIVITY_H*/
