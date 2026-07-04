
#ifndef LV_PRO_MUSIC_LIST_ACTIVITY_H
#define LV_PRO_MUSIC_LIST_ACTIVITY_H

#include "lvgl/lvgl.h"

extern lv_obj_t *music_info_activity;
extern lv_group_t *music_info_group;

extern lv_obj_t *music_list_activity;
extern lv_group_t *music_list_group;

void lv_pro_music_info_init(void);
void lv_pro_music_info_exit(void);
void lv_pro_music_list_init(void);
void lv_pro_music_list_exit(void);
void lv_pro_music_refresh_list(void);

#endif  /*LV_PRO_MUSIC_LIST_ACTIVITY_H*/
