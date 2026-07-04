/*
 * lv_pro_res_bt.h
 *
 *  Created on: 2024/09/13
 *      Author: hongjiasen
 */

#ifndef LV_PRO_RES_BT_H_
#define LV_PRO_RES_BT_H_

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../Layer/wifi_bt/lv_ui_wifi.h"
#include "lv_common.h"

#if ENABLE_BT

#ifdef LV_USE_LINUX_BT4_MODE

#include <bt_dev_list.h>
#include <bt_log.h>
#include <bt_manager.h>

#define BTMG_A2DP_SINK_ENABLE (1 << 0)
#define BTMG_A2DP_SOUCE_ENABLE (1 << 1)
#define BTMG_AVRCP_ENABLE (1 << 2)
#define BTMG_HFP_HF_ENABLE (1 << 3)
#define BTMG_HFP_AG_ENABLE (1 << 4)
#define BTMG_GATT_SERVER_ENABLE (1 << 5)
#define BTMG_GATT_CLIENT_ENABLE (1 << 6)
#define BTMG_SPP_ENABLE (1 << 7)
#define BTMG_SMP_ENABLE (1 << 8)

#endif

#define SCAN_LIST_LEN 40

typedef enum {
    LVGL_BT_STATE_IDLE,
    LVGL_BT_STATE_BUSY,
    LVGL_BT_STATE_OPENING,
    LVGL_BT_STATE_CONNECTING,
    LVGL_BT_STATE_DISCONNECTING,
    LVGL_BT_STATE_UNPAIRING,
    LVGL_BT_STATE_CLOSING
} lv_pro_bt_state_t;

typedef struct {
    btmg_bt_device_t *devices;
    int count;
} FreeDevicesArgs;

extern btmg_callback_t *factory_bt_callback;
extern dev_list_t *lv_pro_bt_discovered_devices;
extern btmg_profile_info_t *bt_pro_info;
extern pthread_mutex_t lv_pro_bt_discovered_devices_mutex;
extern int bt_mode;
extern void clear_bluealsa_card();

int lv_pro_res_bt_init(void);
int lv_pro_res_bt_deinit(void);
int lv_pro_res_bt_on(void);
int lv_pro_res_bt_off(void);
int lv_pro_res_bt_scan(int scan);
int lv_pro_res_bt_connect(char *mac_addr);
int lv_pro_res_bt_disconnect(char *mac_addr);
int lv_pro_res_bt_unpair(char *mac_addr);
void lv_pro_res_bt_paired_devices(btmg_bt_device_t **devices, int *count);
void lv_pro_res_bt_free_devices(btmg_bt_device_t *devices, int count);
void lv_pro_res_bt_logo_show(bool conn);

void lv_pro_set_bt_state(lv_pro_bt_state_t new_state);
lv_pro_bt_state_t lv_pro_get_bt_state(void);
bool lv_pro_is_bt_state_idle(void);
int lv_pro_res_bt_disconnect_old_connection(void);

pthread_mutex_t *lv_pro_bt_get_flesh_ui_mutex(void);
pthread_cond_t *lv_pro_bt_get_flesh_ui_cond(void);

#endif /* ENABLE_BT */

#endif /* LV_PRO_RES_BT_H_ */
