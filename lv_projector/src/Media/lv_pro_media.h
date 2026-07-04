
#ifndef LV_PRO_MEDIA_H
#define LV_PRO_MEDIA_H

#include "lvgl/lvgl.h"
#include "middle_ware/lv_pro_res_media_player_int.h"
#include "lv_drivers/indev/evdev.h"
#include "../../widget/lv_pro_res.h"
#include "../../widget/lv_pro_ctrlbar_imgbtn.h"

extern lv_obj_t *Media_activity;
extern lv_group_t *Media_group;
extern lv_obj_t *Disk_activity;
extern lv_group_t *Disk_group;
extern lv_obj_t *File_activity;
extern lv_group_t *File_right_group;
extern bool media_mode_single;

void lv_pro_file_ui_exit(void);
void lv_pro_media_ui_init(void);

#endif  /*LV_PRO_MEDIA_H*/
