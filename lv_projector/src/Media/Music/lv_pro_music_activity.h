
#ifndef LV_PRO_MUSIC_ACTIVITY_H
#define LV_PRO_MUSIC_ACTIVITY_H

#include "lvgl/lvgl.h"
#include "../lv_pro_media_layout.h"

extern lv_obj_t *music_activity;
extern lv_group_t *music_group;
extern bool music_activity_is_open;

void lv_pro_music_ui_init(void);
void lv_pro_music_play_status(bool en);
void lv_pro_music_ctrlbar_timer_en(bool en);
void lv_pro_music_exit(void);

#endif  /*LV_PRO_MUSIC_ACTIVITY_H*/
