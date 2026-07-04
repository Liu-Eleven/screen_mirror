/*
 * lv_pro_bt_conn_activity.h
 *
 *  Created on: 2024/09/12
 *      Author: JASON
 */

#ifndef LV_PRO_BT_CONN_ACTIVITY_H
#define LV_PRO_BT_CONN_ACTIVITY_H

#include "Common/message/lv_pro_msg.h"
#include "lvgl/lvgl.h"

#if ENABLE_BT

void lv_pro_selectbox_bt(void *obj, int state_enum);

#endif /* ENABLE_BT */

#endif /* LV_PRO_BT_CONN_ACTIVITY_H */