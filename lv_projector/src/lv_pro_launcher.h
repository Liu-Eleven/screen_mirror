
#ifndef LV_PRO_LAUNCHER_H
#define LV_PRO_LAUNCHER_H

#include "lvgl/lvgl.h"
#include "lv_drivers/indev/evdev.h"

extern lv_obj_t *launcher_activity;
extern lv_group_t *launcher_group;
extern lv_group_t *key_group;
extern lv_obj_flag_t signalIn_flag;

extern lv_obj_t *WiredSP_activity;
extern lv_obj_t *Media_activity;
extern lv_obj_t *SignalIn_activity;
extern lv_obj_t *WiredSP_activity;
extern lv_obj_t *WirelessSP_activity;
extern lv_obj_t *Setting_activity;
extern lv_obj_t *lv_pro_wifi_activity;
extern lv_obj_t *lv_pro_bt_activity;

extern bool wireless_autoon_proc_run;
extern bool wireless_modules_loaded;

void lv_pro_launcher_init(void);

void lv_projector_ui_init(void);

void update_launcher_label(void);

void update_launcher_fresh_layout(void);

extern lv_timer_t *motor_timer;
void motor_timer_init(void);
static void motor_callback(lv_timer_t *timer);
#endif  /*LV_PRO_LAUNCHER_H*/
