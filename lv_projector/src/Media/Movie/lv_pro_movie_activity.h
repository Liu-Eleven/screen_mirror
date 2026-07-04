
#ifndef LV_PRO_MOVIE_ACTIVITY_H
#define LV_PRO_MOVIE_ACTIVITY_H

#include "lvgl/lvgl.h"
#include "../lv_pro_media_layout.h"

extern lv_obj_t *movie_activity;
extern lv_group_t *movie_group;
extern bool movie_activity_is_open;

void lv_pro_movie_ui_init(void);
void lv_pro_movie_play_status(bool en);
void lv_pro_movie_ctrlbar_timer_en(bool en);
void lv_pro_movie_exit(void);

#endif  /*LV_PRO_MOVIE_ACTIVITY_H*/
