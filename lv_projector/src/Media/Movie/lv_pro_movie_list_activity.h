
#ifndef LV_PRO_MOVIE_LIST_ACTIVITY_H
#define LV_PRO_MOVIE_LIST_ACTIVITY_H

#include "lvgl/lvgl.h"

extern lv_obj_t *movie_info_activity;
extern lv_group_t *movie_info_group;

extern lv_obj_t *movie_list_activity;
extern lv_group_t *movie_list_group;

void lv_pro_movie_info_init(void);
void lv_pro_movie_info_exit(void);
void lv_pro_movie_list_init(void);
void lv_pro_movie_list_exit(void);
void lv_pro_movie_refresh_list(void);

#endif  /*LV_PRO_MOVIE_LIST_ACTIVITY_H*/
