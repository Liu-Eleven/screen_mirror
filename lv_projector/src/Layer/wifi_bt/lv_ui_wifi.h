#ifndef _LV_UI_WIFI_H
#define _LV_UI_WIFI_H

#include "lvgl/lvgl.h"
#include "lv_common.h"

#if ENABLE_BT
extern lv_obj_t *bt_show;
#endif

#if ENABLE_WIFI
extern lv_obj_t *wifi_show;
#endif

void create_ui_wifi_bt_icon(lv_obj_t *parent);
void del_ui_wifi_bt_icon(void);
void hide_ui_wifi_bt_icon(void);
void display_ui_wifi_bt_icon(void);

#endif
