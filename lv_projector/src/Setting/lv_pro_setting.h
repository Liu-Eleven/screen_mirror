#ifndef LV_PRO_SETTING_H
#define LV_PRO_SETTING_H
#include "lvgl/lvgl.h"

extern lv_obj_t *Setting_activity;
extern lv_group_t *Setting_group;

/*
*	root setting
*/
extern lv_obj_t *setting_menu;
extern lv_obj_t * root_page;


void lv_pro_setting_init(void);

#endif  /*LV_PRO_SETTING_H*/
