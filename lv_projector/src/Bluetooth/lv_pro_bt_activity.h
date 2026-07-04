/*
 * lv_pro_bt_activity.h
 *
 *  Created on: 2024/09/05
 *      Author: JASON
 */

#ifndef LV_PRO_BT_ACTIVITY_H
#define LV_PRO_BT_ACTIVITY_H

#include <signal.h>
#include <stdio.h>
#include <time.h>

#include "lv_pro_bt_common.h"
#include "lvgl/lvgl.h"

#if ENABLE_BT

extern lv_obj_t *lv_pro_bt_activity;
extern lv_group_t *bt_menu_group;

int lv_pro_bt_init(void);
int lv_pro_bt_lanucher_auto_on(void);

void lv_pro_bt_freshui_ack(void);
extern void bt_message_process(system_msg_t *sys_msg);

#endif /* ENABLE_BT */

#endif /* LV_PRO_BT_ACTIVITY_H */
