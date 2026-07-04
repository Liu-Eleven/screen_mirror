/*
 * lv_pro_wifi_activity.h
 *
 *  Created on: 2024/09/20
 *      Author: JASON
 */

#ifndef LV_PRO_WIFI_ACTIVITY_H
#define LV_PRO_WIFI_ACTIVITY_H

#include <signal.h>
#include <stdio.h>
#include <time.h>

#include "../Layer/wifi_bt/lv_ui_wifi.h"
#include "lv_pro_wifi_common.h"

#if ENABLE_WIFI
extern lv_obj_t *lv_pro_wifi_activity;
extern lv_group_t *wifi_menu_group;

int lv_pro_wifi_init(void);
bool lv_pro_network_state(void);
void lv_pro_refresh_wifi_state(bool flag);
void lv_pro_wifi_freshui_ack(void);
extern void wifi_message_process(system_msg_t *msg);
#else
bool lv_pro_network_state(void);
#endif /* ENABLE_WIFI */

#endif /* LV_PRO_WIFI_ACTIVITY_H */
