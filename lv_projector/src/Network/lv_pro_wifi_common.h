
/*
 * lv_pro_bt_activity.h
 *
 *  Created on: 2024/09/05
 *      Author: JASON
 */

#ifndef LV_PRO_WIFI_COMMON_H
#define LV_PRO_WIFI_COMMON_H

#include <ctype.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../Common/user_common/lv_pro_user_common.h"
#include "../Layer/wifi_bt/lv_ui_wifi.h"
#include "../lv_pro_launcher.h"
#include "../widget/lv_pro_res.h"
#include "../widget/lv_pro_set_btn_style1.h"
#include "../widget/lv_pro_set_btn_style3.h"
#include "../widget/lv_pro_set_btn_style4.h"
#include "../widget/lv_pro_set_btn_style5.h"
#include "../widget/lv_pro_set_btn_style6.h"
#include "Common/language/string/lv_string_id.h"
#include "Common/message/lv_pro_msg.h"
#include "Layer/prompt/lv_ui_prompt.h"
#include "include/sys_msg.h"
#include "lv_common.h"
#include "lv_pro_res_wifi.h"
#include "lvgl/lvgl.h"
#include "page.h"

#if ENABLE_WIFI

#define WIFI_DEBUG_MODE 1

#ifdef WIFI_DEBUG_MODE
#define WIFI_DBG(fmt, ...) printf("[LVGL WIFI DBG] [%s]: " fmt, __func__, ##__VA_ARGS__)
#else
#define WIFI_DBG(fmt, ...)
#endif

#define WIFI_INF(fmt, ...) printf("[LVGL WIFI INF] [%s]: " fmt, __func__, ##__VA_ARGS__)
#define WIFI_WRN(fmt, ...) printf("[LVGL WIFI WRN] [%s]: " fmt, __func__, ##__VA_ARGS__)
#define WIFI_ERR(fmt, ...) printf("[LVGL WIFI ERR] [%s]: " fmt, __func__, ##__VA_ARGS__)

// #define LV_PRO_WIFI_ACTIVITY_GROUP_DEBUG 1

// 状态常量
#define WIFI_STATE_ON "on"
#define WIFI_STATE_OFF "off"
#define WIFI_STATE_CONN "conn"
#define WIFI_STATE_DISCONN "disconn"

#define WIFI_SEC_NONE 0

// 表示不同状态
#define WIFI_STATE_ADDNEWDEV_CONN "addnewdev_conn"
#define WIFI_STATE_PAIRLIST_CONN "pairlist_conn"
#define WIFI_STATE_SCANLIST_CONN "scanlist_conn"
#define WIFI_STATE_PAIRLIST_DISCONN "pairlist_disconn"
#define WIFI_STATE_PAIRLIST_UNPAIR "pairlist_unpair"

#define WIFI_SCAN_TIME_MS 20 * 1000

typedef enum {
    MSG_TYPE_WIFI_WORK_STATE_CLOSE,
    MSG_TYPE_WIFI_WORK_STATE_CLOSESUCCESS,
    MSG_TYPE_WIFI_WORK_STATE_CLOSEFAIL,
    MSG_TYPE_WIFI_WORK_STATE_OPEN,
    MSG_TYPE_WIFI_WORK_STATE_OPENSUCCESS,
    MSG_TYPE_WIFI_WORK_STATE_OPENFAIL,
    MSG_TYPE_WIFI_MODE_SWITCH,
    MSG_TYPE_WIFI_MODE_SWITCH_BTN_ENABLE,

    MSG_TYPE_WIFI_CLEARHIDDEN_SCANSTATE_OBJ,
    MSG_TYPE_WIFI_HIDDEN_ALL_SCANDEV_OBJ,
    MSG_TYPE_WIFI_FOCUSID_SCANDEV_OBJ,
    MSG_TYPE_WIFI_HIDDEN_DEV_OBJ,
    MSG_TYPE_WIFI_HIDDEN_SCANSTATE_OBJ,
    MSG_TYPE_WIFI_UPDATE_SCANDEV_OBJ,
    MSG_TYPE_WIFI_UPDATE_SCANDEVNUM_OBJ,

    MSG_TYPE_WIFI_DELETE_PAIR_DEV,
    MSG_TYPE_WIFI_HIDDEN_ALL_PAIRDEV_OBJ,
    MSG_TYPE_WIFI_UPDATE_PAIRDEV_OBJ,
    MSG_TYPE_WIFI_UPDATE_PAIRNUMBER_OBJ,

    MSG_TYPE_WIFI_CHANGED_DEV_STATE,
    MSG_TYPE_WIFI_CHANGED_HOME_CONNLOGO,
    MSG_TYPE_WIFI_CHANGED_HOME_DISCONNLOGO,
    MSG_TYPE_WIFI_CHANGED_TO_MENUGROUP,

    MSG_TYPE_WIFI_CHANGED_TO_RIGHTGROUP,
    MSG_TYPE_WIFI_TRY_GRUOP_FOCUS,

    MSG_TYPE_WIFI_CREARE_PAIRSTATE_PROC,
    MSG_TYPE_WIFI_CREARE_SCANDEV_PROC,
    MSG_TYPE_WIFI_MANUALLY_ADD_DEV,
    MSG_TYPE_WIFI_CLICKED_PAIRDEV,
    MSG_TYPE_WIFI_CONN_PAIRDEV,
    MSG_TYPE_WIFI_NEWCONN_PAIRDEV,
    MSG_TYPE_WIFI_DISCONN_PAIRDEV,
    MSG_TYPE_WIFI_DELETED_PAIRDEV,
    MSG_TYPE_WIFI_CLICKER_SCANDEV,
    MSG_TYPE_WIFI_CONN_SCANDEV,
    MSG_TYPE_WIFI_CONN_SCANDEV_FAIL,
    MSG_TYPE_WIFI_LVGL_UI_DEINIT,
    MSG_TYPE_WIFI_LVGL_UI_INIT,
    MSG_TYPE_WIFI_SET_DEV_NOTSAVESTATE,
    MSG_TYPE_WIFI_CONN_PAIRDEV_FAIL,
    MSG_TYPE_WIFI_DISCONN_PAIRDEV_FAIL,
    MSG_TYPE_WIFI_STOP_PAIRPROC,
    MSG_TYPE_WIFI_STOP_SCANPROC,

    MSG_TYPE_WIFI_STATE_CONNECTED,
    MSG_TYPE_WIFI_STATE_DISCONNED,

    MSG_TYPE_WIFI_LV_KEY_UP,
    MSG_TYPE_WIFI_LV_KEY_DOWN

} wifi_lvgl_msg_type_t;

typedef enum {
    WIFI_STATE_UNSAVE,
    WIFI_STATE_SAVED,
    WIFI_STATE_DISCONNECTED,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_DISCONNECTING,
    WIFI_STATE_DISCONNECT
} WIFIConnectionStatus;

typedef enum {
    WIFI_OPEN_LOGO,
    WIFI_CLOSE_LOGO,
    WIFI_SEARCH_LOGO,
    WIFI_ADDNETWORK_LOGO,
    WIFI_BACKHOME_LOGO,
    WIFI_SCAN0_LOGO,
    WIFI_SCAN1_LOGO,
    WIFI_SCAN2_LOGO,
    WIFI_SCAN3_LOGO,
    WIFI_RESOURCE_COUNT
} WIFIResourceType;

typedef struct {
    int id;
    int sec;
    int rssi;
    uint32_t freq;
    bool scan_action;
    char ssid[33];
    char bssid[18];
    char mgmt[33];
    WIFIConnectionStatus status;
} WIFIDeviceMsg;

typedef struct {
    const char *state;
    lv_pro_set_btn_style3_t *btn;
} wifi_thread_args_btn3_t;

typedef struct {
    int sec;
    char mgmt[32];
    char ssid[64];
    char bssid[18];
    char password[64];
    int rssi;
    const char *state;
    lv_pro_set_btn_style6_t *btn;
} wifi_thread_args_btn6_t;

typedef struct {
    wifi_lvgl_msg_type_t type;
    void *data;
    bool free_data;
} lvgl_wifi_msg_t;

// group
int lv_pro_wifi_group_change(lv_group_t *group, bool back_found, lv_obj_t *obj);
void lv_pro_wifi_group_printf_debug(lv_obj_t *group, int direction);
int lv_pro_wifi_group_user_focus_check_cb(lv_group_t *group);
void lv_pro_wifi_group_obj_clear_focus(lv_group_t *group);
int lv_pro_wifi_group_devid(lv_group_t *group, lv_obj_t *obj);
int lv_pro_wifi_group_num_all(lv_group_t *group);

// update obj
void lv_pro_wifi_hidden_all_devobj(lv_obj_t **items, int length);
void lv_pro_wifi_clear_devobj(void *obj);
void lv_pro_wifi_update_icon(void *obj, int rssi);
void lv_pro_wifi_update_devobj(lv_obj_t *item, WIFIDeviceMsg *device);
void lv_pro_wifi_set_logostate(void *obj, int state);
WIFIDeviceMsg *lv_pro_wifi_scandev_init(wifi_scan_result_t *dev, int dev_id);
int lv_pro_wifi_msg_enqueue(wifi_lvgl_msg_type_t type, void *data, bool free_data);

char *get_wireless_ko_path(void);
void update_file_wifi_state(const char *state);
int get_file_wifi_state();
int lv_pro_res_wifi_get_dev(const char *ssid, char *pwd, char *key_mgnt);

#endif /* ENABLE_WIFI */

#endif /* LV_PRO_WIFI_COMMON_H */