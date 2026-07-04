
#ifndef LV_PRO_PICTURE_LIST_ACTIVITY_H
#define LV_PRO_PICTURE_LIST_ACTIVITY_H

#include "lvgl/lvgl.h"
#include "../middle_ware/lv_pro_res_picture_player_int.h"

extern lv_obj_t *picture_info_activity;
extern lv_group_t *picture_info_group;

extern lv_obj_t *picture_list_activity;
extern lv_group_t *picture_list_group;

void lv_pro_picture_info_init(void);
void lv_pro_picture_info_exit(void);
void lv_pro_picture_list_init(void);
void lv_pro_picture_list_exit(void);
void lv_pro_picture_refresh_info_list(void);

#endif  /*LV_PRO_PICTURE_LIST_ACTIVITY_H*/
