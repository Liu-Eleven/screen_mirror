
/*
 * lv_pro_bt_activity.h
 *
 *  Created on: 2024/09/05
 *      Author: JASON
 */

#ifndef LV_PRO_BT_COMMON_H
#define LV_PRO_BT_COMMON_H

#include <signal.h>
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
#include "lv_pro_bt_activity.h"
#include "lv_pro_res_bt.h"
#include "lvgl/lvgl.h"
#include "page.h"

#if ENABLE_BT

#define BT_DEBUG_MODE 1
#ifdef BT_DEBUG_MODE
#define BT_DBG(fmt, ...) printf("[LVGL BT DBG] [%s]: " fmt, __func__, ##__VA_ARGS__)
#else
#define BT_DBG(fmt, ...)
#endif

#define BT_INF(fmt, ...) printf("[LVGL BT INF] [%s]: " fmt, __func__, ##__VA_ARGS__)
#define BT_WRN(fmt, ...) printf("[LVGL BT WRN] [%s]: " fmt, __func__, ##__VA_ARGS__)
#define BT_ERR(fmt, ...) printf("[LVGL BT ERR] [%s]: " fmt, __func__, ##__VA_ARGS__)

// #define LV_PRO_BT_ACTIVITY_GROUP_DEBUG 1

#define BT_STATE_ON "on"
#define BT_STATE_OFF "off"
#define BT_STATE_CONN "conn"
#define BT_STATE_DISCONN "disconn"

#define BT_STATE_PAIRLIST_CONN "pairlist_conn"
#define BT_STATE_SCANLIST_CONN "scanlist_conn"
#define BT_STATE_PAIRLIST_DISCONN "pairlist_disconn"
#define BT_STATE_PAIRLIST_UNPAIR "pairlist_unpair"

#define BT_SCAN_TIME_MS 20 * 1000

typedef enum {
    MSG_TYPE_BT_WORK_STATE_CLOSE,
    MSG_TYPE_BT_WORK_STATE_CLOSESUCCESS,
    MSG_TYPE_BT_WORK_STATE_CLOSEFAIL,
    MSG_TYPE_BT_WORK_STATE_OPEN,
    MSG_TYPE_BT_WORK_STATE_OPENSUCCESS,
    MSG_TYPE_BT_WORK_STATE_OPENFAIL,
    MSG_TYPE_BT_MODE_SWITCH,
    MSG_TYPE_BT_MODE_SWITCH_BTN_ENABLE,
    MSG_TYPE_BT_SRC_OPEN_TRY_OPEN,

    MSG_TYPE_BT_CLEARHIDDEN_SCANSTATE_OBJ,
    MSG_TYPE_BT_HIDDEN_ALL_SCANDEV_OBJ,
    MSG_TYPE_BT_HIDDEN_DEV_OBJ,
    MSG_TYPE_BT_HIDDEN_SCANSTATE_OBJ,
    MSG_TYPE_BT_UPDATE_SCANDEV_OBJ,

    MSG_TYPE_BT_DELETE_PAIR_DEV,
    MSG_TYPE_BT_HIDDEN_ALL_PAIRDEV_OBJ,
    MSG_TYPE_BT_UPDATE_PAIRDEV_OBJ,
    MSG_TYPE_BT_UPDATE_PAIRNUMBER_OBJ,

    MSG_TYPE_BT_CHANGED_DEV_STATE,
    MSG_TYPE_BT_CHANGED_HOME_CONNLOGO,
    MSG_TYPE_BT_CHANGED_HOME_DISCONNLOGO,
    MSG_TYPE_BT_CHANGED_TO_MENUGROUP,

    MSG_TYPE_BT_CHANGED_TO_RIGHTGROUP,
    MSG_TYPE_BT_TRY_GRUOP_FOCUS,

    MSG_TYPE_BT_CREARE_FRESHUI_PROC,
    MSG_TYPE_BT_CREARE_SCANDEV_PROC,
    MSG_TYPE_BT_CLICKED_PAIRDEV,
    MSG_TYPE_BT_CONN_PAIRDEV,
    MSG_TYPE_BT_DISCONN_PAIRDEV,
    MSG_TYPE_BT_DELETED_PAIRDEV,
    MSG_TYPE_BT_CONN_SCANDEV,
    MSG_TYPE_BT_LVGL_UI_DEINIT,
    MSG_TYPE_BT_LVGL_UI_INIT,
    MSG_TYPE_BT_SET_DEV_NOTSAVESTATE,
    MSG_TYPE_BT_SET_DEV_SAVEDSTATE,
    MSG_TYPE_BT_STOP_PAIRPROC,
    MSG_TYPE_BT_STOP_SCANPROC,

    MSG_TYPE_BT_A2DPSRC_CONNECTED,
    MSG_TYPE_BT_A2DPSRC_DISCONNED,

    MSG_TYPE_BT_LV_KEY_UP,
    MSG_TYPE_BT_LV_KEY_DOWN
} bt_lvgl_msg_type_t;

typedef enum {
    BT_STATE_UNSAVE,
    BT_STATE_SAVED,
    BT_STATE_DISCONNECTED,
    BT_STATE_CONNECTING,
    BT_STATE_CONNECTED,
    BT_STATE_DISCONNECTING,
    BT_STATE_DISCONNECT
} ConnectionStatus;

typedef enum {
    BT_OPEN_LOGO,
    BT_CLOSE_LOGO,
    BT_SEARCH_LOGO,
    BT_BACKHOME_LOGO,
    BT_SCAN_LOGO_DF,
    BT_SCAN_LOGO_PHONE,
    BT_SCAN_LOGO_HEADSET,
    BT_SCAN_LOGO_COMPUTER,
    RESOURCE_COUNT
} ResourceType;

typedef struct {
    int id;
    char name[249];
    char macAddress[18];
    ConnectionStatus status;
    int cod;
} BluetoothDeviceMsg;

typedef struct {
    lv_pro_set_btn_style3_t *btn;
    const char *state;
} thread_args_btn3_t;

typedef struct {
    lv_pro_set_btn_style6_t *btn;
    char name[249];
    char addr[18];
    const char *state;
} thread_args_btn6_t;

typedef struct {
    bt_lvgl_msg_type_t type;
    void *data;
    bool free_data;
} lvgl_bt_msg_t;

typedef struct {
    char selectMac[18];
    char selectName[249];
    uint32_t cod;
    lv_pro_bt_state_t bt_state;
    bool isselected;
} SelectDeviceInfo;

// resource
extern const char *resource_paths[];
char *lv_pro_bt_get_resource_path(ResourceType type);
char *lv_pro_bt_msg_type_to_string(bt_lvgl_msg_type_t msg_type);

// group
bool lv_pro_bt_group_is_active(void);
int lv_pro_bt_group_change(lv_group_t *group, bool back_found, lv_obj_t *obj);
void lv_pro_bt_group_printf_debug(lv_obj_t *group, int direction);
int lv_pro_bt_group_user_focus_check_cb(lv_group_t *group);
void lv_pro_bt_group_obj_clear_focus(lv_group_t *group);

// update obj
void lv_pro_bt_hidden_all_devobj(lv_obj_t **items, int length);
void lv_pro_bt_clear_devobj(void *obj);
void lv_pro_bt_update_devobj(lv_obj_t *item, BluetoothDeviceMsg *device);
void lv_pro_bt_set_logostate(void *obj, int state);
int lv_pro_bt_msg_enqueue(bt_lvgl_msg_type_t type, void *data, bool free_data);

#if ENABLE_WIFI == 0
char *get_wireless_ko_path(void);
#endif
void update_file_bt_state(const char *state);
int get_file_bt_state(void);
void update_file_bt_connectstate(const char *state, const char *mac, const char *name);
int get_file_bt_connectstate(char *mac, size_t mac_len, char *name, size_t name_len);

lv_obj_t *message_box_ui(lv_obj_t *obj, char *str, uint32_t period);

void setSelectDeviceInfo(const char *mac, const char *name, uint32_t cod, lv_pro_bt_state_t state,
                         bool selected);
void clearSelectDeviceInfo(void);
SelectDeviceInfo *getSelectDeviceInfo(void);

#endif /* ENABLE_BT */

#endif /* LV_PRO_BT_COMMON_H */