/*
 * lv_pro_bt_activity.c
 *
 *  Created on: 2024/09/05
 *      Author: JASON
 */

#include "lv_pro_bt_activity.h"

#include "lv_pro_bt_common.h"
#include "lv_pro_bt_conn_activity.h"

#if ENABLE_BT

static void bt_activity_event_handler(lv_event_t *e);
static void lv_pro_bt_ui_init(void);
static void lv_pro_bt_ui_deinit(void);
static void *bt_autoinit_proc(void *arg);
static void *bt_reconnct_proc(void *arg);
static void *bt_control_proc(void *arg);
static void *bt_pairstate_proc(void *arg);
static void *bt_scan_proc(void *arg);
static void *bt_changedstate_proc(void *arg);
static void create_bt_scan_item(int i);
static void clear_bt_scan_items();
static void create_bt_pair_item(int i);
static void clear_bt_pair_items();

/*******************************BT RESOURCE*******************************/
static volatile bool bt_state_on = false;
static bool bt_off_ing = false;
static bool bt_on_ing = false;
static bool bt_reconn_ing = false;
static bool bt_scanstate = false;
static bool bt_modules_loaded = false;

static bool autoon_proc_run = false;
static bool control_proc_run = false;
static bool connect_proc_run = false;
static bool pairstate_proc_run = false;
static bool scan_proc_run = false;
static bool pairstate_proclock = false;
static bool pairstate_needexit = false;

static bool show_msgbox = false;
static bool firstrefresh = false;
static bool ignoreSingleRefresh = false;

/*******************************BT LVGL***********************************/
lv_obj_t *lv_pro_bt_activity;
lv_group_t *bt_menu_group;
lv_group_t *bt_paired_list_group;
lv_group_t *bt_scan_list_group;

static lv_group_t *current_group;
static lv_obj_t *bt_menu_obj;
static lv_obj_t *bt_pair_obj;
static lv_obj_t *bt_scan_obj;
static lv_obj_t *bt_pairlist_obj;
static lv_obj_t *bt_pairnum_obj;
static lv_obj_t *bt_pair_item[SCAN_LIST_LEN];
static lv_obj_t *bt_scanlist_obj;
static lv_obj_t *bt_scanstate_obj;
static lv_obj_t *bt_scan_item[SCAN_LIST_LEN];

static lv_timer_t *scan_timer;
static lv_timer_t *backg_scan_timer;
static lv_obj_t *htc_msg_box;

static bool bt_lvgl_active;
static bool bt_lvgl_exiting;

static char *bt_oldState = BT_STATE_OFF;
/***********************************************************************/

// pthread msg sync
static pthread_mutex_t *bt_mut;
static pthread_cond_t *bt_cond;

void lv_pro_bt_freshui_ack(void)
{
    if (!pairstate_proc_run || ignoreSingleRefresh) {
        BT_WRN("proc run:%d  ignoreSingleRefresh:%d\n", pairstate_proc_run, ignoreSingleRefresh);
        return;
    }

    pthread_mutex_t *bt_mut = lv_pro_bt_get_flesh_ui_mutex();
    pthread_cond_t *bt_cond = lv_pro_bt_get_flesh_ui_cond();

    if (bt_mut && bt_cond && pairstate_proclock) {
        pthread_mutex_lock(bt_mut);
        pthread_cond_signal(bt_cond);
        pthread_mutex_unlock(bt_mut);
    }
}

static void lv_pro_bt_freahui_wait_ack(void)
{
    pthread_mutex_t *bt_mut = lv_pro_bt_get_flesh_ui_mutex();
    pthread_cond_t *bt_cond = lv_pro_bt_get_flesh_ui_cond();

    if (bt_mut && bt_cond) {
        pthread_mutex_lock(bt_mut);
        pairstate_proclock = true;
        pthread_cond_wait(bt_cond, bt_mut);
        pthread_mutex_unlock(bt_mut);
        pairstate_proclock = false;
    }
}

pthread_mutex_t *lv_pro_bt_get_flesh_ui_mutex(void) { return bt_mut; }

pthread_cond_t *lv_pro_bt_get_flesh_ui_cond(void) { return bt_cond; }

static bool shouldExitbt()
{
    bool isBluetoothInactive = !bt_state_on;
    bool isExitingLvgl = bt_lvgl_exiting;
    bool isBluetoothOffing = bt_off_ing;
    bool isLvglActive = bt_lvgl_active;

    bool exitCondition = isBluetoothInactive || isExitingLvgl || !isLvglActive || isBluetoothOffing;

    if (exitCondition) {
        BT_DBG(
            "Exiting Bluetooth: UI Active: %d, LVGL Exiting: %d, Bluetooth Offing: %d, Bluetooth "
            "State: %d\n",
            isLvglActive, isExitingLvgl, isBluetoothOffing, isBluetoothInactive);
    }

    return exitCondition;
}

static void wait_for_procs_to_finish()
{
    int timenum = 0;

    if (pairstate_proc_run && pairstate_proclock) lv_pro_bt_freshui_ack();

    while (control_proc_run || pairstate_proc_run || scan_proc_run) {
        usleep(10 * 1000);
        timenum++;
        if (timenum > 50) {
            timenum = 0;
            BT_WRN("control_proc_run=%d, pairstate_proc_run=%d, scan_proc_run=%d\n",
                   control_proc_run, pairstate_proc_run, scan_proc_run);
        }
    }
}

static void stop_btscan(lv_timer_t *timer)
{
    if (timer) {
        lv_timer_del(timer);
        scan_timer = NULL;
    }

    if (!bt_off_ing && bt_state_on) lv_pro_res_bt_scan(0);

    bt_scanstate = false;

    while (scan_proc_run) {
        usleep(10 * 1000);
    }
    BT_DBG("stop scan finish!\n");
}

static void wait_scan_proc_stop()
{
    int timenum = 0;
    while (scan_proc_run) {
        usleep(10 * 1000);
        timenum++;
        if (timenum > 50) {
            timenum = 0;
            BT_WRN("scan_proc_run=%d\n", scan_proc_run);
            stop_btscan(NULL);
        }
    }
}

static void wait_control_proc_stop()
{
    int timenum = 0;
    while (control_proc_run) {
        usleep(10 * 1000);
        timenum++;
        if (timenum > 50) {
            timenum = 0;
            BT_WRN("control_proc_run=%d\n", control_proc_run);
        }
    }
}

static void wait_pair_proc_stop()
{
    int timenum = 0;
    while (pairstate_proc_run) {
        usleep(10 * 1000);
        timenum++;
        if (timenum > 50) {
            timenum = 0;
            BT_WRN("pairstate_proc_run=%d\n", pairstate_proc_run);
        }
    }
}

static void lv_pro_bt_toggle_state(void *obj, const char *new_state)
{
    if (obj == NULL) return;
    lv_pro_set_btn_style3_t *btn = (lv_pro_set_btn_style3_t *)obj;
    BT_DBG("bt try changes state[%s->%s]\n", bt_oldState, new_state);
    bt_oldState = new_state;

    lv_pro_set_btn_style4_t *_btn4;
    if (strcmp(new_state, BT_STATE_ON) == 0) {
        _btn4 = (lv_pro_set_btn_style4_t *)lv_obj_get_child(bt_menu_obj, 2);
        lv_obj_set_style_text_color(_btn4->name, lv_color_white(), 0);
    } else if (strcmp(new_state, BT_STATE_OFF) == 0) {
        _btn4 = (lv_pro_set_btn_style4_t *)lv_obj_get_child(bt_menu_obj, 2);
        lv_obj_set_style_text_color(_btn4->name, lv_color_make(0xa3, 0xa3, 0xa3), 0);
    }

    // reflesh ui
    for (int i = 0; i < SCAN_LIST_LEN; i++) {
        if (bt_pair_item[i]) lv_pro_bt_clear_devobj(bt_pair_item[i]);
        if (bt_scan_item[i]) lv_pro_bt_clear_devobj(bt_scan_item[i]);
    }
    lv_label_set_text(bt_pairnum_obj, " 0 / 0");
    if (strcmp(new_state, BT_STATE_ON) == 0) {
        lv_obj_clear_flag(bt_pair_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(bt_scan_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(bt_pairlist_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(bt_scanlist_obj, LV_OBJ_FLAG_HIDDEN);
    } else if (strcmp(new_state, BT_STATE_OFF) == 0) {
        lv_obj_add_flag(bt_pair_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(bt_scan_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(bt_pairlist_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(bt_scanlist_obj, LV_OBJ_FLAG_HIDDEN);
    }

    // bt ctrl
    thread_args_btn3_t *btctrl = malloc(sizeof(thread_args_btn3_t));
    if (btctrl != NULL) {
        btctrl->btn = btn;
        btctrl->state = new_state;

        wait_control_proc_stop();
        pthread_t bt_control;
        pthread_create(&bt_control, NULL, bt_control_proc, btctrl);
        pthread_detach(bt_control);
    } else {
        BT_ERR("btctrl malloc failed\n");
    }
}

static void lv_pro_bt_conn_status_changed(void *obj, int taskstate, void *user_data)
{
    if (obj == NULL) return;
    lv_pro_set_btn_style6_t *btn = (lv_pro_set_btn_style6_t *)obj;

    char *new_state_str = NULL;
    if (taskstate == LVGL_BT_STATE_CONNECTING) {
        new_state_str = lv_get_string(STR_BT_CONNECTING);
    } else if (taskstate == LVGL_BT_STATE_DISCONNECTING) {
        new_state_str = lv_get_string(STR_BT_DISCONNECTING);
    } else if (taskstate == LVGL_BT_STATE_UNPAIRING) {
        new_state_str = lv_get_string(STR_BT_UNPAIR);
    }
    lv_pro_set_btn_style6_set_state_str(btn, new_state_str);
    lv_pro_set_bt_state(taskstate);

    thread_args_btn6_t *state_changed = malloc(sizeof(thread_args_btn6_t));
    if (state_changed != NULL) {
        state_changed->btn = btn;
        state_changed->state = user_data;
        char *name = lv_pro_set_btn_style6_get_name_str(btn);
        char *mac = lv_pro_set_btn_style6_get_mac_addr_str(btn);
        strncpy(state_changed->name, name, sizeof(state_changed->name) - 1);
        state_changed->name[sizeof(state_changed->name) - 1] = '\0';
        strncpy(state_changed->addr, mac, sizeof(state_changed->addr) - 1);
        state_changed->addr[sizeof(state_changed->addr) - 1] = '\0';

        setSelectDeviceInfo(mac, name, 0, taskstate, 1);
        pthread_t bt_connect;
        pthread_create(&bt_connect, NULL, bt_changedstate_proc, state_changed);
        pthread_detach(bt_connect);
    } else {
        BT_ERR("state_changed malloc failed\n");
    }
}

static bool is_canprintf_message(lvgl_bt_msg_t *msg)
{
    return (msg->type != MSG_TYPE_BT_LV_KEY_DOWN && msg->type != MSG_TYPE_BT_LV_KEY_UP &&
            msg->type != MSG_TYPE_BT_HIDDEN_ALL_SCANDEV_OBJ &&
            msg->type != MSG_TYPE_BT_HIDDEN_ALL_PAIRDEV_OBJ &&
            msg->type != MSG_TYPE_BT_UPDATE_SCANDEV_OBJ &&
            msg->type != MSG_TYPE_BT_UPDATE_PAIRDEV_OBJ &&
            msg->type != MSG_TYPE_BT_HIDDEN_DEV_OBJ &&
            msg->type != MSG_TYPE_BT_HIDDEN_SCANSTATE_OBJ &&
            msg->type != MSG_TYPE_BT_CLEARHIDDEN_SCANSTATE_OBJ &&
            msg->type != MSG_TYPE_BT_UPDATE_PAIRNUMBER_OBJ &&
            msg->type != MSG_TYPE_BT_SET_DEV_NOTSAVESTATE);
}

static bool lv_pro_bt_msg_need_ignore(lvgl_bt_msg_t *msg)
{
    if (bt_lvgl_exiting) {
        // msg->type != MSG_TYPE_BT_STOP_SCANPROC  优化退出
        return (msg->type != MSG_TYPE_BT_STOP_PAIRPROC && msg->type != MSG_TYPE_BT_LVGL_UI_DEINIT);
    }
    return false;
}

void bt_message_process(system_msg_t *sys_msg)
{
    lv_group_t *__key_group;
    BluetoothDeviceMsg *device;
    lvgl_bt_msg_t *msg;
    void *btn;

    if (sys_msg == NULL) {
        BT_ERR("sys msg null!\n");
        return;
    }

    if (sys_msg->type != MSG_TYPE_BT_FRESH_UI) {
        if (sys_msg->type == MSG_TYPE_BT_A2DPSRC_CONNECT ||
            sys_msg->type == MSG_TYPE_BT_A2DPSRC_DISCONNECT) {
            msg = sys_msg->data;
            if (msg && msg->free_data && msg->data) {
                free(msg->data);
            }
            if (sys_msg->data) free(sys_msg->data);
        }

        return;
    }

    msg = sys_msg->data;

    if (is_canprintf_message(msg)) {
        BT_DBG("typeid=%d [%s]\n", msg->type, lv_pro_bt_msg_type_to_string(msg->type));
    }

    if (!bt_lvgl_active || lv_pro_bt_msg_need_ignore(msg)) {
        BT_WRN("msg:[%s] ignore process!\n", lv_pro_bt_msg_type_to_string(msg->type));
        goto MSG_END;
    }

    switch (msg->type) {
    case MSG_TYPE_BT_HIDDEN_ALL_SCANDEV_OBJ:
        if (bt_scan_item) lv_pro_bt_hidden_all_devobj(bt_scan_item, SCAN_LIST_LEN);
        break;

    case MSG_TYPE_BT_HIDDEN_ALL_PAIRDEV_OBJ:
        if (bt_pair_item) lv_pro_bt_hidden_all_devobj(bt_pair_item, SCAN_LIST_LEN);
        break;

    case MSG_TYPE_BT_UPDATE_SCANDEV_OBJ:
        device = (BluetoothDeviceMsg *)msg->data;
        if (device) {
            if (!bt_scan_item[device->id]) {
                create_bt_scan_item(device->id);
            } else {
                lv_pro_bt_clear_devobj(bt_scan_item[device->id]);
            }
            lv_pro_bt_update_devobj(bt_scan_item[device->id], device);
        }
        break;

    case MSG_TYPE_BT_UPDATE_PAIRDEV_OBJ:
        device = (BluetoothDeviceMsg *)msg->data;
        if (device) {
            if (!bt_pair_item[device->id]) {
                create_bt_pair_item(device->id);
            } else {
                lv_pro_bt_clear_devobj(bt_pair_item[device->id]);
            }
            lv_pro_bt_update_devobj(bt_pair_item[device->id], device);
        }
        break;

    case MSG_TYPE_BT_HIDDEN_DEV_OBJ:
        btn = msg->data;
        if (btn) lv_pro_bt_clear_devobj(btn);
        break;

    case MSG_TYPE_BT_HIDDEN_SCANSTATE_OBJ:
        if (bt_scanstate_obj) lv_obj_add_flag(bt_scanstate_obj, LV_OBJ_FLAG_HIDDEN);
        break;

    case MSG_TYPE_BT_CLEARHIDDEN_SCANSTATE_OBJ:
        if (bt_scanstate_obj) lv_obj_clear_flag(bt_scanstate_obj, LV_OBJ_FLAG_HIDDEN);
        break;

    case MSG_TYPE_BT_UPDATE_PAIRNUMBER_OBJ: {
        if (!bt_pairnum_obj) break;
        lv_label_set_text(bt_pairnum_obj, (char *)msg->data);
        lv_obj_clear_flag(bt_pairnum_obj, LV_OBJ_FLAG_HIDDEN);
        break;
    }

    case MSG_TYPE_BT_LV_KEY_UP: {
        if (!lv_pro_bt_group_is_active()) break;
        __key_group = (lv_group_t *)msg->data;
        if (!__key_group) break;
        if (__key_group == bt_scan_list_group && lv_group_get_obj_is_head(__key_group, NULL)) {
            if (lv_pro_bt_group_change(bt_paired_list_group, true, NULL) == 0) {
                current_group = bt_paired_list_group;
                lv_pro_bt_group_obj_clear_focus(__key_group);
            }
        } else {
            lv_group_focus_obj(lv_group_get_prev(__key_group, NULL));
        }
        break;
    }

    case MSG_TYPE_BT_LV_KEY_DOWN: {
        if (!lv_pro_bt_group_is_active()) break;
        __key_group = (lv_group_t *)msg->data;
        if (!__key_group) break;
        if (__key_group == bt_paired_list_group && lv_group_get_obj_is_end(__key_group, NULL)) {
            if (lv_pro_bt_group_change(bt_scan_list_group, false, NULL) == 0) {
                current_group = bt_scan_list_group;
                lv_pro_bt_group_obj_clear_focus(__key_group);
            }
        } else {
            lv_group_focus_obj(lv_group_get_next(__key_group, NULL));
        }
        break;
    }

    case MSG_TYPE_BT_CHANGED_TO_MENUGROUP: {
        if (!lv_pro_bt_group_is_active()) break;
        __key_group = (lv_group_t *)msg->data;
        if (!__key_group) break;
        lv_pro_bt_group_obj_clear_focus(__key_group);
        lv_pro_bt_group_change(bt_menu_group, false, NULL);
        current_group = bt_menu_group;
        break;
    }

    case MSG_TYPE_BT_CHANGED_TO_RIGHTGROUP: {
        if (!lv_pro_bt_group_is_active()) break;
        __key_group = (lv_group_t *)msg->data;
        if (!__key_group) break;
        if (lv_pro_bt_group_change(bt_paired_list_group, false, NULL) == 0) {
            current_group = bt_paired_list_group;
        } else if (lv_pro_bt_group_change(bt_scan_list_group, false, NULL) == 0) {
            current_group = bt_scan_list_group;
        } else {
            current_group = __key_group;
        }
        break;
    }

    case MSG_TYPE_BT_TRY_GRUOP_FOCUS: {
        if (!lv_pro_bt_group_is_active()) break;
        __key_group = (lv_group_t *)msg->data;
        if (!__key_group) break;
        lv_pro_bt_group_obj_clear_focus(current_group);
        lv_pro_bt_group_obj_clear_focus(__key_group);
        if (lv_pro_bt_group_change(__key_group, false, NULL) == 0) {
            current_group = __key_group;
        } else {
            lv_pro_bt_group_change(bt_menu_group, false, NULL);
            current_group = bt_menu_group;
        }

        break;
    }

    case MSG_TYPE_BT_CHANGED_HOME_CONNLOGO:
    case MSG_TYPE_BT_CHANGED_HOME_DISCONNLOGO: {
        lv_pro_res_bt_logo_show(msg->type == MSG_TYPE_BT_CHANGED_HOME_CONNLOGO);
        break;
    }

    case MSG_TYPE_BT_SRC_OPEN_TRY_OPEN: {
        int connId = msg->data;
        btn = bt_pair_item[connId];
        if (btn) {
            htc_msg_box =
                message_box_ui(htc_msg_box, (char *)lv_get_string(STR_BT_TRY_RECONNECT), 1000);
            lv_pro_bt_conn_status_changed(btn, LVGL_BT_STATE_CONNECTING, BT_STATE_PAIRLIST_CONN);
        } else {
            BT_ERR("try connecting item is NULL!\n");
        }

        break;
    }

    case MSG_TYPE_BT_MODE_SWITCH: {
        btn = msg->data;
        if (btn) lv_pro_bt_set_logostate(btn, 2);
        break;
    }

    case MSG_TYPE_BT_MODE_SWITCH_BTN_ENABLE: {
        btn = msg->data;
        if (btn) lv_pro_bt_set_logostate(btn, -1);
        break;
    }

    case MSG_TYPE_BT_WORK_STATE_OPEN: {
        btn = msg->data;
        lv_pro_create_message2_box((char *)lv_get_string(STR_BT_OPENING));
        show_msgbox = true;
        if (btn) lv_pro_bt_toggle_state(btn, BT_STATE_ON);
        break;
    }

    case MSG_TYPE_BT_WORK_STATE_OPENSUCCESS: {
        if (show_msgbox) {
            lv_pro_del_message2_box();
            show_msgbox = false;
        }
        if (msg->data && !bt_lvgl_exiting && bt_lvgl_active)
            lv_pro_bt_group_change(msg->data, false, NULL);
        break;
    }

    case MSG_TYPE_BT_WORK_STATE_OPENFAIL: {
        btn = msg->data;
        if (show_msgbox) {
            lv_pro_del_message2_box();
            show_msgbox = false;
        }
        if (btn) lv_pro_bt_set_logostate(btn, 0);
        break;
    }

    case MSG_TYPE_BT_WORK_STATE_CLOSE: {
        lv_pro_create_message2_box((char *)lv_get_string(STR_BT_CLOSEING));
        show_msgbox = true;
        if (pairstate_proc_run && pairstate_proclock) lv_pro_bt_freshui_ack();
        btn = msg->data;
        if (btn) lv_pro_bt_toggle_state(btn, BT_STATE_OFF);
        break;
    }

    case MSG_TYPE_BT_WORK_STATE_CLOSESUCCESS: {
        if (show_msgbox) {
            lv_pro_del_message2_box();
            show_msgbox = false;
        }
        if (msg->data && !bt_lvgl_exiting && bt_lvgl_active)
            lv_pro_bt_group_change(msg->data, false, NULL);
        break;
    }

    case MSG_TYPE_BT_WORK_STATE_CLOSEFAIL: {
        btn = msg->data;
        if (show_msgbox) {
            lv_pro_del_message2_box();
            show_msgbox = false;
        }
        if (btn) lv_pro_bt_set_logostate(btn, 1);
        break;
    }

    case MSG_TYPE_BT_CREARE_FRESHUI_PROC: {
        wait_pair_proc_stop();
        pthread_t bt_pairstate;
        pthread_create(&bt_pairstate, NULL, bt_pairstate_proc, NULL);
        pthread_detach(bt_pairstate);
        break;
    }

    case MSG_TYPE_BT_CREARE_SCANDEV_PROC: {
        for (int i = 0; i < SCAN_LIST_LEN; i++) {
            if (bt_scan_item[i]) lv_pro_bt_clear_devobj(bt_scan_item[i]);
        }
        if (bt_scanstate_obj) lv_obj_clear_flag(bt_scanstate_obj, LV_OBJ_FLAG_HIDDEN);
        wait_scan_proc_stop();
        pthread_t bt_scan;
        pthread_create(&bt_scan, NULL, bt_scan_proc, NULL);
        pthread_detach(bt_scan);
        break;
    }

    case MSG_TYPE_BT_CLICKED_PAIRDEV: {
        if (bt_scanstate) stop_btscan(scan_timer);
        btn = (lv_pro_set_btn_style6_t *)msg->data;
        if (!btn) break;
        char *btn_state = lv_pro_set_btn_style6_get_state_str(btn);
        if (!strcmp(btn_state, lv_get_string(STR_BT_SAVED))) {
            lv_pro_selectbox_bt(btn, LVGL_BT_STATE_CONNECTING);
        } else if (!strcmp(btn_state, lv_get_string(STR_BT_CONN))) {
            lv_pro_selectbox_bt(btn, LVGL_BT_STATE_DISCONNECTING);
        }
        break;
    }

    case MSG_TYPE_BT_CONN_PAIRDEV: {
        if (bt_scanstate) stop_btscan(scan_timer);
        btn = msg->data;
        if (btn)
            lv_pro_bt_conn_status_changed(btn, LVGL_BT_STATE_CONNECTING, BT_STATE_PAIRLIST_CONN);
        break;
    }

    case MSG_TYPE_BT_DISCONN_PAIRDEV: {
        if (bt_scanstate) stop_btscan(scan_timer);
        btn = msg->data;
        if (btn)
            lv_pro_bt_conn_status_changed(btn, LVGL_BT_STATE_DISCONNECTING,
                                          BT_STATE_PAIRLIST_DISCONN);
        break;
    }

    case MSG_TYPE_BT_DELETED_PAIRDEV: {
        if (bt_scanstate) stop_btscan(scan_timer);
        btn = msg->data;
        if (btn)
            lv_pro_bt_conn_status_changed(btn, LVGL_BT_STATE_UNPAIRING, BT_STATE_PAIRLIST_UNPAIR);
        break;
    }

    case MSG_TYPE_BT_CONN_SCANDEV: {
        if (bt_scanstate) stop_btscan(scan_timer);
        btn = msg->data;
        if (btn)
            lv_pro_bt_conn_status_changed(btn, LVGL_BT_STATE_CONNECTING, BT_STATE_SCANLIST_CONN);
        break;
    }

    case MSG_TYPE_BT_SET_DEV_SAVEDSTATE:
        btn = msg->data;
        if (btn) lv_pro_set_btn_style6_set_state_str(btn, lv_get_string(STR_BT_MY_DEV));
        break;

    case MSG_TYPE_BT_SET_DEV_NOTSAVESTATE:
        btn = msg->data;
        if (btn) lv_pro_set_btn_style6_set_state_str(btn, "");
        break;

    case MSG_TYPE_BT_STOP_SCANPROC:
        if (bt_scanstate) stop_btscan(scan_timer);
        break;

    case MSG_TYPE_BT_STOP_PAIRPROC:
        /*Add a needexit variable to prevent the thread from exiting the page when it is in a
         * blocked state, and the thread cannot perceive the normal exit*/
        pairstate_needexit = true;
        if (pairstate_proc_run && pairstate_proclock) lv_pro_bt_freshui_ack();
        break;

        // case MSG_TYPE_BT_LVGL_UI_INIT:
        //     if (!bt_lvgl_active) lv_pro_bt_ui_init();
        //     break;

    case MSG_TYPE_BT_LVGL_UI_DEINIT:
        if (bt_lvgl_active) lv_pro_bt_ui_deinit();
        break;

    default:
        BT_DBG("error msg\n");
        break;
    }

MSG_END:

    if (is_canprintf_message(msg)) {
        BT_DBG("complete\n");
    }

    if (msg->free_data) {
        free(msg->data);
    }
    if (msg) {
        free(msg);
    }
}

static void *bt_pairstate_proc(void *arg)
{
    (void)arg;

    if (pairstate_proc_run) {
        pthread_exit((void *)1);
        return NULL;
    }

    BT_DBG("start\n");
    pairstate_proc_run = true;
    pairstate_needexit = false;

    bt_mut = malloc(sizeof(pthread_mutex_t));
    bt_cond = malloc(sizeof(pthread_cond_t));
    pthread_mutex_init(bt_mut, NULL);
    pthread_cond_init(bt_cond, NULL);

    bool proc_first_fresh = true;

    btmg_bt_device_t *devices = NULL;

    char reconnect_mac[18];
    char reconnect_name[256];
    int found_dev = 0;
    int connect_count = 0;

    /*The query is only done when Bluetooth is turned on for the first time or when the Bluetooth
     * connection is back, and the device is read directly from the configuration file to refresh
     * the UI, without calling lv_pro_res_bt_paired_devices to avoid blocking*/

    if (bt_mode == 0 && (firstrefresh || bt_reconn_ing)) {
        firstrefresh = false;
        memset(reconnect_mac, '\0', sizeof(reconnect_mac));
        memset(reconnect_name, '\0', sizeof(reconnect_name));
        int ret = get_file_bt_connectstate(reconnect_mac, sizeof(reconnect_mac), reconnect_name,
                                           sizeof(reconnect_name));
        BT_DBG("isneedReconn=%d mac=%s name=%s\n", ret, reconnect_mac, reconnect_name);
        if (ret == 1) {
            // 找到需要回连的设备，flag置0 就绪
            int connId = -1;
            BluetoothDeviceMsg *devicemsg =
                (BluetoothDeviceMsg *)malloc(sizeof(BluetoothDeviceMsg));
            connId = found_dev++;
            devicemsg->id = connId;
            devicemsg->status = BT_STATE_CONNECTING;
            devicemsg->cod = 0;
            snprintf(devicemsg->name, sizeof(devicemsg->name), "%s", reconnect_name);
            snprintf(devicemsg->macAddress, sizeof(devicemsg->macAddress), "%s", reconnect_mac);
            lv_pro_bt_msg_enqueue(MSG_TYPE_BT_UPDATE_PAIRDEV_OBJ, devicemsg, true);
            if (!bt_reconn_ing) {
                // It is not that the reconn thread needs to actively trigger the connection
                lv_pro_bt_msg_enqueue(MSG_TYPE_BT_SRC_OPEN_TRY_OPEN, connId, false);
            }
            goto FRESHDEV_END;
        }
    }

    while (bt_state_on) {
        BT_DBG("fresh pair list!\n");
        /*When you initiate a connection back, selectdev will be recorded. When you reopen the page,
         * the last execution status will be displayed first.*/
        SelectDeviceInfo *info = getSelectDeviceInfo();
        if (info->isselected) {
            BT_DBG("selectDev Found\n");
            BT_DBG("selectDev Name:%s Mac:%s \n", info->selectName, info->selectMac);
            BluetoothDeviceMsg *devicemsg =
                (BluetoothDeviceMsg *)malloc(sizeof(BluetoothDeviceMsg));
            devicemsg->id = 0;
            devicemsg->status = info->bt_state;
            devicemsg->cod = info->cod;
            snprintf(devicemsg->name, sizeof(devicemsg->name), "%s", info->selectName);
            snprintf(devicemsg->macAddress, sizeof(devicemsg->macAddress), "%s", info->selectMac);
            lv_pro_bt_msg_enqueue(MSG_TYPE_BT_UPDATE_PAIRDEV_OBJ, devicemsg, true);
            lv_pro_bt_msg_enqueue(MSG_TYPE_BT_UPDATE_PAIRNUMBER_OBJ, "0 / 1", false);
        }

        connect_count = 0, found_dev = 0;
        lv_pro_res_bt_paired_devices(&devices, &found_dev);

        if (shouldExitbt() || pairstate_needexit) {
            break;
        }

        lv_pro_bt_msg_enqueue(MSG_TYPE_BT_HIDDEN_ALL_PAIRDEV_OBJ, NULL, false);

        if (found_dev > 0) {
            for (int i = 0; i < found_dev; i++) {
                BluetoothDeviceMsg *devicemsg =
                    (BluetoothDeviceMsg *)malloc(sizeof(BluetoothDeviceMsg));
                devicemsg->id = i;
                devicemsg->status = BT_STATE_SAVED;
                devicemsg->cod = devices[i].r_class;
                if (devices[i].connected) {
                    devicemsg->status = BT_STATE_CONNECTED;
                    connect_count++;
                }
                snprintf(devicemsg->name, sizeof(devicemsg->name), "%s", devices[i].remote_name);
                snprintf(devicemsg->macAddress, sizeof(devicemsg->macAddress), "%s",
                         devices[i].remote_address);
                lv_pro_bt_msg_enqueue(MSG_TYPE_BT_UPDATE_PAIRDEV_OBJ, devicemsg, true);

                if (shouldExitbt() || pairstate_needexit) {
                    break;
                }
            }
            // 防止pair列表刷新状态的时候丢失聚焦
            if (lv_pro_is_bt_state_idle() && connect_count > 0 && !proc_first_fresh) {
                lv_pro_bt_msg_enqueue(MSG_TYPE_BT_TRY_GRUOP_FOCUS, bt_paired_list_group, false);
            }

        } else {
            BT_DBG("paired device is empty\n");
            if (current_group && current_group == bt_paired_list_group) {
                lv_pro_bt_msg_enqueue(MSG_TYPE_BT_CHANGED_TO_MENUGROUP, bt_paired_list_group,
                                      false);
            }
        }
    FRESHDEV_END: {
        char *state_result = malloc(10 * sizeof(char));
        BT_DBG("connect_count=%d found_dev=%d\n", connect_count, found_dev);
        snprintf(state_result, 10, "%d / %d", connect_count, found_dev);
        lv_pro_bt_msg_enqueue(MSG_TYPE_BT_UPDATE_PAIRNUMBER_OBJ, state_result, true);

        proc_first_fresh = false;
        lv_pro_bt_freahui_wait_ack();

        if (shouldExitbt() || pairstate_needexit) {
            break;
        }
    }
    }

    if (devices) {
        lv_pro_res_bt_free_devices(devices, found_dev);
        devices = NULL;
    }

    pthread_mutex_destroy(bt_mut);
    pthread_cond_destroy(bt_cond);
    free(bt_mut);
    free(bt_cond);
    bt_mut = NULL;
    bt_cond = NULL;

    pairstate_proc_run = false;
    BT_DBG("exit\n");
    pthread_exit((void *)1);
    return NULL;
}

static void *bt_scan_proc(void *arg)
{
    (void)arg;

    if (shouldExitbt() || scan_proc_run || !bt_state_on || !bt_lvgl_active || bt_reconn_ing) {
        pthread_exit((void *)1);
        return NULL;
    }

    BT_DBG("start\n");
    scan_proc_run = true;

    // 优化体验,每次扫描前确保扫描已经停止
    lv_pro_res_bt_scan(0);

    int scan_result = lv_pro_res_bt_scan(1);
    if (scan_result) {
        BT_ERR("scan failed\n");
        scan_proc_run = false;
        pthread_exit((void *)1);
        return NULL;
    } else {
        lv_pro_bt_msg_enqueue(MSG_TYPE_BT_CLEARHIDDEN_SCANSTATE_OBJ, NULL, false);
        lv_pro_bt_msg_enqueue(MSG_TYPE_BT_HIDDEN_ALL_SCANDEV_OBJ, NULL, false);
        scan_timer = lv_timer_create(stop_btscan, BT_SCAN_TIME_MS, NULL);
        bt_scanstate = true;
    }

    dev_node_t *dev_node = NULL;
    int scan_count = 0;
    int add_count = 0;
    bool should_exit = false;

    while (bt_state_on && bt_scanstate) {
        if (shouldExitbt()) {
            should_exit = true;
            break;
        }

        pthread_mutex_lock(&lv_pro_bt_discovered_devices_mutex);
        dev_node = lv_pro_bt_discovered_devices->head;
        if (!dev_node) {
            pthread_mutex_unlock(&lv_pro_bt_discovered_devices_mutex);
            usleep(100 * 1000);
            continue;
        }
        scan_count = 0;
        while (dev_node) {
            if (shouldExitbt() || !bt_scanstate) {
                should_exit = true;
                break;
            }
            if (scan_count == add_count) {
                BluetoothDeviceMsg *device = malloc(sizeof(BluetoothDeviceMsg));
                device->id = add_count;
                device->status = BT_STATE_UNSAVE;
                device->cod = dev_node->profile;

                const char *name =
                    strcmp(dev_node->dev_name, " ") == 0 ? dev_node->dev_addr : dev_node->dev_name;
                snprintf(device->name, sizeof(device->name), "%s", name);
                snprintf(device->macAddress, sizeof(device->macAddress), "%s", dev_node->dev_addr);
                lv_pro_bt_msg_enqueue(MSG_TYPE_BT_UPDATE_SCANDEV_OBJ, device, true);
                add_count++;
            }
            dev_node = dev_node->next;
            scan_count++;
        }
        pthread_mutex_unlock(&lv_pro_bt_discovered_devices_mutex);
    }

    if (scan_timer) {
        lv_timer_del(scan_timer);
        scan_timer = NULL;
    }

    lv_pro_bt_msg_enqueue(MSG_TYPE_BT_HIDDEN_SCANSTATE_OBJ, NULL, false);

    scan_proc_run = false;
    BT_DBG("exit\n");
    pthread_exit((void *)1);
    return NULL;
}

static void *bt_control_proc(void *arg)
{
    thread_args_btn3_t *args = (thread_args_btn3_t *)arg;
    lv_pro_set_btn_style3_t *_btn = (lv_pro_set_btn_style3_t *)args->btn;

    if (control_proc_run) {
        free(args);
        BT_DBG("exit\n");
        pthread_exit((void *)1);
        return NULL;
    }

    BT_DBG("start changestate:%s\n", args->state);
    control_proc_run = true;

    if (!strcmp(args->state, BT_STATE_ON)) {
        if (!bt_off_ing) {
            bt_on_ing = true;
            lv_pro_res_bt_init();
            if (lv_pro_res_bt_on()) {
                bt_state_on = false;
                bt_on_ing = false;
                // 需要隐藏界面
                update_file_bt_state(BT_STATE_OFF);
                lv_pro_bt_msg_enqueue(MSG_TYPE_BT_WORK_STATE_OPENFAIL, args->btn, false);
                goto END;
            }
            update_file_bt_state(BT_STATE_ON);
            lv_pro_bt_msg_enqueue(MSG_TYPE_BT_WORK_STATE_OPENSUCCESS, bt_menu_group, false);
            firstrefresh = true;
            bt_state_on = true;
            bt_on_ing = false;
        } else {
            lv_pro_bt_msg_enqueue(MSG_TYPE_BT_WORK_STATE_OPENFAIL, args->btn, false);
            goto END;
        }
        if (bt_state_on) {
            if (!pairstate_proc_run) {
                lv_pro_bt_msg_enqueue(MSG_TYPE_BT_CREARE_FRESHUI_PROC, NULL, false);
            }
            if (!scan_proc_run && !bt_reconn_ing) {
                lv_pro_bt_msg_enqueue(MSG_TYPE_BT_CREARE_SCANDEV_PROC, NULL, false);
            }
        }
    } else if (!strcmp(args->state, BT_STATE_OFF)) {
        if (!bt_on_ing) {
            bt_off_ing = true;
            if (bt_scanstate) stop_btscan(scan_timer);
            lv_pro_res_bt_off();
            lv_pro_res_bt_deinit();
            bt_state_on = false;
            bt_off_ing = false;
            update_file_bt_state(BT_STATE_OFF);
            clearSelectDeviceInfo();
            lv_pro_bt_msg_enqueue(MSG_TYPE_BT_WORK_STATE_CLOSESUCCESS, bt_menu_group, false);
        } else {
            lv_pro_bt_msg_enqueue(MSG_TYPE_BT_WORK_STATE_CLOSEFAIL, args->btn, false);
            goto END;
        }
    }

END:
    free(args);
    lv_pro_bt_msg_enqueue(MSG_TYPE_BT_MODE_SWITCH_BTN_ENABLE, _btn, false);
    lv_pro_set_bt_state(LVGL_BT_STATE_IDLE);
    control_proc_run = false;
    BT_DBG("exit\n");
    pthread_exit((void *)1);
    return NULL;
}

static void *bt_changedstate_proc(void *arg)
{
    if (connect_proc_run) {
        pthread_exit((void *)1);
        return NULL;
    }

    BT_DBG("start\n");
    connect_proc_run = true;

    thread_args_btn6_t *args = (thread_args_btn6_t *)arg;

    if (bt_reconn_ing) {
        goto CHANGEDSTATE_END;
    }

    if (!strcmp(args->state, BT_STATE_PAIRLIST_CONN) ||
        !strcmp(args->state, BT_STATE_SCANLIST_CONN)) {
        int ret = lv_pro_res_bt_disconnect_old_connection();
        if (ret == 1) {
            /*When connecting to a Bluetooth device, if there is already a device connected, the
             * original device will be disconnected first. At this time, the page will be refreshed
             * due to the disconnection event. Connecting a new device will refresh it again. In
             * this optimization, the refresh action triggered by the disconnection will be
             * ignored.*/
            ignoreSingleRefresh = true;
            usleep(200 * 1000);
            /*The delay is 200ms. The device may have been disconnected and the callback may not be
             * received. The marker is actively erased and ignored.*/
            ignoreSingleRefresh = false;
        }
    }

    if (!strcmp(args->state, BT_STATE_PAIRLIST_CONN)) {
        int conn_result = lv_pro_res_bt_connect(args->addr);
        if (conn_result) {
            htc_msg_box =
                message_box_ui(htc_msg_box, (char *)lv_get_string(STR_BT_CONN_FAILED), 1000);
        } else {
            htc_msg_box =
                message_box_ui(htc_msg_box, (char *)lv_get_string(STR_BT_CONN_SUCCESS), 1000);
        }
    } else if (!strcmp(args->state, BT_STATE_SCANLIST_CONN)) {
        int conn_result = lv_pro_res_bt_connect(args->addr);
        if (conn_result) {
            htc_msg_box =
                message_box_ui(htc_msg_box, (char *)lv_get_string(STR_BT_CONN_FAILED), 1000);
            lv_pro_bt_msg_enqueue(MSG_TYPE_BT_SET_DEV_NOTSAVESTATE, args->btn, false);
        } else {
            htc_msg_box =
                message_box_ui(htc_msg_box, (char *)lv_get_string(STR_BT_CONN_SUCCESS), 1000);
            lv_pro_bt_msg_enqueue(MSG_TYPE_BT_HIDDEN_DEV_OBJ, args->btn, false);
        }
    } else if (!strcmp(args->state, BT_STATE_PAIRLIST_DISCONN)) {
        lv_pro_res_bt_disconnect(args->addr);
        usleep(300 * 1000);
        BT_DBG("disconnected:%s\n", args->addr);
        htc_msg_box = message_box_ui(htc_msg_box, (char *)lv_get_string(STR_BT_DISCONNECTED), 1000);
        lv_pro_bt_msg_enqueue(MSG_TYPE_BT_CREARE_SCANDEV_PROC, NULL, false);
    } else if (!strcmp(args->state, BT_STATE_PAIRLIST_UNPAIR)) {
        lv_pro_res_bt_unpair(args->addr);
        lv_pro_bt_msg_enqueue(MSG_TYPE_BT_HIDDEN_DEV_OBJ, args->btn, false);
        lv_pro_bt_msg_enqueue(MSG_TYPE_BT_TRY_GRUOP_FOCUS, bt_paired_list_group, false);
        lv_pro_bt_msg_enqueue(MSG_TYPE_BT_CREARE_SCANDEV_PROC, NULL, false);
    }

    lv_pro_set_bt_state(LVGL_BT_STATE_IDLE);

CHANGEDSTATE_END:
    clearSelectDeviceInfo();
    free(args);

    connect_proc_run = false;
    BT_DBG("exit\n");
    pthread_exit((void *)1);
    return NULL;
}

static void key_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    const uint32_t *key_param = lv_event_get_param(e);
    uint32_t key = (key_param != NULL) ? *key_param : 0;
    lv_obj_t *obj = lv_event_get_target(e);
    char *user_data = (char *)lv_event_get_user_data(e);

    if (code == LV_EVENT_KEY) {
        lv_pro_bt_group_printf_debug(current_group, 1);
        bool is_idle = lv_pro_is_bt_state_idle();

        if (key == LV_KEY_ENTER) {
            if (!strcmp(user_data, "Switch")) {
                lv_pro_set_btn_style3_t *btn = (lv_pro_set_btn_style3_t *)obj;
                bool is_checked = lv_obj_has_state(btn->btn, LV_STATE_CHECKED);

                if (is_checked) {
                    if (!bt_state_on && is_idle) {
                        lv_pro_set_bt_state(LVGL_BT_STATE_OPENING);
                        lv_obj_add_state(btn->btn, LV_STATE_DISABLED);
                        lv_obj_add_state(btn, LV_STATE_DISABLED);
                        lv_pro_bt_msg_enqueue(MSG_TYPE_BT_WORK_STATE_OPEN, btn, false);
                    } else {
                        if (!is_idle) {
                            htc_msg_box = message_box_ui(
                                htc_msg_box, (char *)lv_get_string(STR_DONT_OPERATE), 1500);
                        }
                        lv_pro_bt_msg_enqueue(MSG_TYPE_BT_WORK_STATE_OPENFAIL, btn, false);
                    }
                } else {  // !is_checked
                    if (bt_state_on && is_idle) {
                        lv_pro_set_bt_state(LVGL_BT_STATE_CLOSING);
                        lv_obj_add_state(btn->btn, LV_STATE_DISABLED);
                        lv_obj_add_state(btn, LV_STATE_DISABLED);
                        lv_pro_bt_msg_enqueue(MSG_TYPE_BT_WORK_STATE_CLOSE, btn, false);
                    } else {
                        if (!is_idle) {
                            htc_msg_box = message_box_ui(
                                htc_msg_box, (char *)lv_get_string(STR_DONT_OPERATE), 1500);
                        }
                        lv_pro_bt_msg_enqueue(MSG_TYPE_BT_WORK_STATE_CLOSEFAIL, btn, false);
                    }
                }

            } else if (!strcmp(user_data, "Search")) {
                if (bt_state_on && is_idle && !scan_proc_run && !bt_reconn_ing) {
                    lv_pro_bt_msg_enqueue(MSG_TYPE_BT_CREARE_SCANDEV_PROC, NULL, false);
                } else if (bt_state_on && !is_idle) {
                    htc_msg_box =
                        message_box_ui(htc_msg_box, (char *)lv_get_string(STR_DONT_OPERATE), 1500);
                }

            } else if (!strcmp(user_data, "BackHome")) {
                if (is_idle) {
                    switch_page(PAGE_HOME);
                    // load_current_channel(launcher_activity, launcher_group);
                } else {
                    htc_msg_box =
                        message_box_ui(htc_msg_box, (char *)lv_get_string(STR_DONT_OPERATE), 1500);
                }
                return;
            } else if (!strcmp(user_data, "SwitchMode")) {
                lv_pro_set_btn_style3_t *btn = (lv_pro_set_btn_style3_t *)obj;
                if (!bt_state_on) {
                    if (lv_obj_has_state(btn->btn, LV_STATE_CHECKED)) {
                        bt_mode = 1;
                    } else {
                        bt_mode = 0;
                    }
                } else {
                    htc_msg_box =
                        message_box_ui(htc_msg_box, (char *)lv_get_string(STR_DONT_OPERATE), 1500);
                    lv_pro_bt_msg_enqueue(MSG_TYPE_BT_MODE_SWITCH, btn, false);
                }
            } else if (!strcmp(user_data, "Paired_dev")) {
                lv_pro_set_btn_style6_t *btn = (lv_pro_set_btn_style6_t *)obj;

                if (bt_state_on && is_idle) {
                    lv_pro_bt_msg_enqueue(MSG_TYPE_BT_CLICKED_PAIRDEV, btn, false);
                    lv_pro_set_bt_state(LVGL_BT_STATE_BUSY);
                } else if (!is_idle) {
                    htc_msg_box =
                        message_box_ui(htc_msg_box, (char *)lv_get_string(STR_DONT_OPERATE), 1500);
                }
            } else if (!strcmp(user_data, "Scan_dev")) {
                lv_pro_set_btn_style6_t *btn = (lv_pro_set_btn_style6_t *)obj;

                if (bt_state_on && is_idle) {
                    lv_pro_bt_msg_enqueue(MSG_TYPE_BT_CONN_SCANDEV, btn, false);
                    lv_pro_set_bt_state(LVGL_BT_STATE_BUSY);
                } else if (!is_idle) {
                    htc_msg_box =
                        message_box_ui(htc_msg_box, (char *)lv_get_string(STR_DONT_OPERATE), 1500);
                }
            }
        } else if (!is_global_key_go_new_channel(key) || key == LV_KEY_BACK) {
            switch_page(PAGE_HOME);
            // load_current_channel(launcher_activity, launcher_group);
            return;
        } else if (key == LV_KEY_RIGHT) {
            if (bt_mode == 0 && current_group == bt_menu_group) {
                lv_pro_bt_msg_enqueue(MSG_TYPE_BT_CHANGED_TO_RIGHTGROUP, bt_menu_group, false);
            }
        } else if (key == LV_KEY_LEFT) {
            if (current_group != bt_menu_group) {
                lv_pro_bt_msg_enqueue(MSG_TYPE_BT_CHANGED_TO_MENUGROUP, current_group, false);
            }
        } else if (key == LV_KEY_UP) {
            lv_pro_bt_msg_enqueue(MSG_TYPE_BT_LV_KEY_UP, current_group, false);

        } else if (key == LV_KEY_DOWN) {
            lv_pro_bt_msg_enqueue(MSG_TYPE_BT_LV_KEY_DOWN, current_group, false);
        } else if (key == LV_KEY_HOME) {
            return;
        }

        lv_pro_bt_group_printf_debug(current_group, 0);
    }
}

static void create_bt_scan_item(int i)
{
    char name_with_id[10];
    snprintf(name_with_id, sizeof(name_with_id), "Name %d", i);
    bt_scan_item[i] = lv_pro_set_btn_style6_create(bt_scanlist_obj);
    lv_obj_set_size(bt_scan_item[i], lv_pct(100), LV_SIZE_CONTENT);
    lv_pro_set_btn_style6_src(bt_scan_item[i], lv_pro_bt_get_resource_path(BT_SCAN_LOGO_DF),
                              &GENERAL_FONT_NORMAL, lv_color_white(), name_with_id,
                              &GENERAL_FONT_SMALL, lv_color_hex(0XC0C0C0), "Mac",
                              &GENERAL_FONT_NORMAL, lv_color_white(), "", bt_scan_list_group);
    lv_obj_add_event_cb(bt_scan_item[i], key_event_cb, LV_EVENT_KEY, "Scan_dev");
    lv_obj_add_flag(bt_scan_item[i], LV_OBJ_FLAG_HIDDEN);
}

static void clear_bt_scan_items()
{
    for (int i = 0; i < SCAN_LIST_LEN; i++) {
        bt_scan_item[i] = NULL;
    }
}

static void create_bt_pair_item(int i)
{
    char name_with_id[10];
    snprintf(name_with_id, sizeof(name_with_id), "Name %d", i);
    bt_pair_item[i] = lv_pro_set_btn_style6_create(bt_pairlist_obj);
    lv_obj_set_size(bt_pair_item[i], lv_pct(100), LV_SIZE_CONTENT);
    lv_pro_set_btn_style6_src(
        bt_pair_item[i], lv_pro_bt_get_resource_path(BT_SCAN_LOGO_DF), &GENERAL_FONT_NORMAL,
        lv_color_white(), name_with_id, &GENERAL_FONT_SMALL, lv_color_hex(0XC0C0C0), "Mac",
        &GENERAL_FONT_NORMAL, lv_color_white(), lv_get_string(STR_BT_SAVED), bt_paired_list_group);
    lv_obj_add_event_cb(bt_pair_item[i], key_event_cb, LV_EVENT_KEY, "Paired_dev");
    lv_obj_add_flag(bt_pair_item[i], LV_OBJ_FLAG_HIDDEN);
}

static void clear_bt_pair_items()
{
    for (int i = 0; i < SCAN_LIST_LEN; i++) {
        bt_pair_item[i] = NULL;
    }
}

static void lv_pro_bt_ui_init(void)
{
    BT_DBG("start\n");

    /*******左侧菜单*******/
    // 左侧菜单
    bt_menu_group = lv_group_create();
    lv_group_set_user_focus_check_cb(bt_menu_group, lv_pro_bt_group_user_focus_check_cb);

    bt_menu_obj = lv_obj_create(lv_pro_bt_activity);
    lv_obj_set_size(bt_menu_obj, lv_pct(30), lv_pct(100));
    lv_obj_align(bt_menu_obj, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_border_width(bt_menu_obj, 3, LV_PART_MAIN);
    lv_obj_set_style_border_side(bt_menu_obj, LV_BORDER_SIDE_RIGHT, LV_PART_MAIN);
    lv_obj_set_style_pad_all(bt_menu_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(bt_menu_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bt_menu_obj, COLOR_BLUE, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(bt_menu_obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(bt_menu_obj, LV_OBJ_FLAG_USER_2);

    // 菜单名称
    lv_obj_t *bt_menu_name = lv_label_create(bt_menu_obj);
    lv_obj_set_width(bt_menu_name, lv_pct(95));
    lv_obj_align(bt_menu_name, LV_ALIGN_TOP_LEFT, 10, lv_pct(20));
    lv_obj_set_style_border_width(bt_menu_name, 1, LV_PART_MAIN);
    lv_obj_set_style_border_side(bt_menu_name, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
    lv_obj_set_style_border_color(bt_menu_name, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(bt_menu_name, 5, LV_PART_MAIN);
    lv_obj_set_style_pad_left(bt_menu_name, 10, LV_PART_MAIN);
    lv_obj_set_style_radius(bt_menu_name, 0, LV_PART_MAIN);
    lv_obj_set_style_text_font(bt_menu_name, &GENERAL_FONT_BIG, LV_PART_MAIN);
    lv_label_set_recolor(bt_menu_name, true);
    lv_label_set_text_fmt(bt_menu_name, "#ffffff %s #", lv_get_string(STR_BT_NAME));
    lv_obj_add_flag(bt_menu_name, LV_OBJ_FLAG_HIDDEN);  // 客户要求不显示

    // 蓝牙开关
    lv_obj_t *bt_state_btn = lv_pro_set_btn_style3_create(bt_menu_obj);
    lv_obj_set_size(bt_state_btn, lv_pct(95), lv_pct(10));
    lv_obj_align_to(bt_state_btn, bt_menu_name, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 30);
    lv_pro_set_btn_style3_src(
        bt_state_btn, COLOR_BLUE, lv_color_hex(0xffa523), lv_pro_bt_get_resource_path(BT_OPEN_LOGO),
        lv_pro_bt_get_resource_path(BT_CLOSE_LOGO), bt_state_on, bt_menu_group);
    lv_obj_add_event_cb(bt_state_btn, key_event_cb, LV_EVENT_KEY, "Switch");
    lv_pro_set_btn_style3_t *_btn = (lv_pro_set_btn_style3_t *)bt_state_btn;
    if (bt_state_on) {
        bt_oldState = BT_STATE_ON;
        lv_obj_add_state(_btn->btn, LV_STATE_CHECKED);
    } else {
        bt_oldState = BT_STATE_OFF;
        lv_obj_clear_state(_btn->btn, LV_STATE_CHECKED);
    }

    // 蓝牙扫描按钮
    lv_obj_t *bt_search_btn = lv_pro_set_btn_style4_create(bt_menu_obj);
    lv_obj_set_size(bt_search_btn, lv_pct(95), lv_pct(10));
    lv_obj_align_to(bt_search_btn, bt_state_btn, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_pro_set_btn_style4_src(bt_search_btn, lv_pro_bt_get_resource_path(BT_SEARCH_LOGO),
                              &GENERAL_FONT_NORMAL, lv_get_string(STR_SEARCH_BT), bt_menu_group);
    lv_obj_add_event_cb(bt_search_btn, key_event_cb, LV_EVENT_KEY, "Search");
    lv_pro_set_btn_style4_t *_btn4 = (lv_pro_set_btn_style4_t *)bt_search_btn;
    if (!bt_state_on) {
        lv_obj_set_style_text_color(_btn4->name, lv_color_make(0xa3, 0xa3, 0xa3), 0);
    }

    // 蓝牙返回HOME按钮
    lv_obj_t *bt_backhome_btn = lv_pro_set_btn_style4_create(bt_menu_obj);
    lv_obj_set_size(bt_backhome_btn, lv_pct(95), lv_pct(10));
    lv_obj_align_to(bt_backhome_btn, bt_search_btn, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_pro_set_btn_style4_src(bt_backhome_btn, lv_pro_bt_get_resource_path(BT_BACKHOME_LOGO),
                              &GENERAL_FONT_NORMAL, lv_get_string(STR_HOME_TITLE), bt_menu_group);
    lv_obj_add_event_cb(bt_backhome_btn, key_event_cb, LV_EVENT_KEY, "BackHome");

    // 蓝牙模式切换
    lv_obj_t *bt_mode_btn = lv_pro_set_btn_style3_create(bt_menu_obj);
    lv_obj_set_size(bt_mode_btn, lv_pct(95), lv_pct(10));
    lv_obj_align_to(bt_mode_btn, bt_backhome_btn, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_pro_set_btn_style3_src(bt_mode_btn, COLOR_BLUE, lv_color_hex(0xffa523),
                              lv_pro_bt_get_resource_path(BT_OPEN_LOGO),
                              lv_pro_bt_get_resource_path(BT_CLOSE_LOGO), bt_mode, bt_menu_group);
    lv_obj_add_event_cb(bt_mode_btn, key_event_cb, LV_EVENT_KEY, "SwitchMode");
    _btn = (lv_pro_set_btn_style3_t *)bt_mode_btn;
    lv_obj_t *mode_name = lv_label_create(bt_mode_btn);
    lv_obj_align(mode_name, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(mode_name, &GENERAL_FONT_MID, LV_PART_MAIN);
    lv_label_set_recolor(mode_name, true);
    lv_label_set_text_fmt(mode_name, "#ffffff %s #", lv_get_string(STR_BT_SINK_SPK_MODE));

    if (bt_mode) {
        lv_obj_add_state(_btn->btn, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(_btn->btn, LV_STATE_CHECKED);
    }

    // 暂屏蔽按钮，不支持音箱模式
    lv_obj_add_flag(bt_mode_btn, LV_OBJ_FLAG_HIDDEN);

    /*******右侧配对列表*******/
    // 配对列表透明框

    bt_pair_obj = lv_obj_create(lv_pro_bt_activity);
    lv_obj_set_size(bt_pair_obj, lv_pct(68), lv_pct(30));
    lv_obj_align_to(bt_pair_obj, bt_menu_obj, LV_ALIGN_OUT_RIGHT_TOP, 10, 10);
    lv_obj_set_style_border_width(bt_pair_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(bt_pair_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(bt_pair_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bt_pair_obj, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(bt_pair_obj, LV_SCROLLBAR_MODE_OFF);

    // 配对标题-配对连接数量
    lv_obj_t *bt_pairtitle_obj = lv_pro_set_btn_style5_create(bt_pair_obj);
    lv_obj_set_width(bt_pairtitle_obj, lv_pct(100));
    lv_obj_align(bt_pairtitle_obj, LV_ALIGN_TOP_LEFT, 0, lv_pct(10));
    lv_pro_set_btn_style5_src(bt_pairtitle_obj, &GENERAL_FONT_BIG, lv_color_white(),
                              lv_get_string(STR_BT_MY_DEV), &GENERAL_FONT_BIG, lv_color_white(),
                              "0 / 0", NULL);
    bt_pairnum_obj = lv_pro_set_btn_style5_get_right_content_obj(bt_pairtitle_obj);

    // 已配对列表 透明框
    bt_pairlist_obj = lv_obj_create(bt_pair_obj);
    lv_obj_set_size(bt_pairlist_obj, lv_pct(100), lv_pct(78));
    lv_obj_align_to(bt_pairlist_obj, bt_pairtitle_obj, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_obj_set_style_border_width(bt_pairlist_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(bt_pairlist_obj, 20, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bt_pairlist_obj, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_add_flag(bt_pairlist_obj, LV_OBJ_FLAG_USER_2);  // user2 为纵向
    lv_obj_set_flex_flow(bt_pairlist_obj, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(bt_pairlist_obj, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END,
                          LV_FLEX_ALIGN_START);

    bt_paired_list_group = lv_group_create();
    lv_group_set_user_focus_check_cb(bt_paired_list_group, lv_pro_bt_group_user_focus_check_cb);
#if 0
    for (int i = 0; i < SCAN_LIST_LEN; i++) {
        char name_with_id[10];
        snprintf(name_with_id, sizeof(name_with_id), "Name %d", i);
        bt_pair_item[i] = lv_pro_set_btn_style6_create(bt_pairlist_obj);
        lv_obj_set_size(bt_pair_item[i], lv_pct(100), LV_SIZE_CONTENT);
        lv_pro_set_btn_style6_src(bt_pair_item[i], lv_pro_bt_get_resource_path(BT_SCAN_LOGO_DF),
                                  &GENERAL_FONT_NORMAL, lv_color_white(), name_with_id,
                                  &GENERAL_FONT_SMALL, lv_color_hex(0XC0C0C0), "Mac",
                                  &GENERAL_FONT_NORMAL, lv_color_white(),
                                  lv_get_string(STR_BT_SAVED), bt_paired_list_group);
        lv_obj_add_event_cb(bt_pair_item[i], key_event_cb, LV_EVENT_KEY, "Paired_dev");
        lv_obj_add_flag(bt_pair_item[i], LV_OBJ_FLAG_HIDDEN);
    }
#endif

    lv_pro_bt_group_obj_clear_focus(bt_paired_list_group);
    if (!bt_state_on) {
        lv_obj_add_flag(bt_pair_obj, LV_OBJ_FLAG_HIDDEN);
    }

    /*******右侧扫描列表*******/
    // 右侧扫描框

    bt_scan_obj = lv_obj_create(lv_pro_bt_activity);
    lv_obj_set_size(bt_scan_obj, lv_pct(68), lv_pct(68));
    lv_obj_align_to(bt_scan_obj, bt_pair_obj, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
    lv_obj_set_style_border_width(bt_scan_obj, 3, LV_PART_MAIN);
    lv_obj_set_style_border_side(bt_scan_obj, LV_BORDER_SIDE_TOP, LV_PART_MAIN);
    lv_obj_set_style_pad_all(bt_scan_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(bt_scan_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bt_scan_obj, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(bt_scan_obj, LV_SCROLLBAR_MODE_OFF);

    // 扫描标题-扫描状态
    lv_obj_t *bt_scantitle_obj = lv_pro_set_btn_style5_create(bt_scan_obj);
    lv_obj_set_width(bt_scantitle_obj, lv_pct(100));
    lv_obj_align(bt_scantitle_obj, LV_ALIGN_TOP_LEFT, 0, 15);
    lv_pro_set_btn_style5_src(bt_scantitle_obj, &GENERAL_FONT_BIG, lv_color_white(),
                              lv_get_string(STR_BT_OTHER_DEV), &GENERAL_FONT_BIG, lv_color_white(),
                              lv_get_string(STR_BT_SEARCHING), NULL);
    bt_scanstate_obj = lv_pro_set_btn_style5_get_right_content_obj(bt_scantitle_obj);
    lv_obj_add_flag(bt_scanstate_obj, LV_OBJ_FLAG_HIDDEN);

    // 扫描列表 透明框
    bt_scanlist_obj = lv_obj_create(bt_scan_obj);
    lv_obj_set_size(bt_scanlist_obj, lv_pct(100), lv_pct(83));
    lv_obj_align_to(bt_scanlist_obj, bt_scantitle_obj, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_obj_set_style_border_width(bt_scanlist_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(bt_scanlist_obj, 20, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bt_scanlist_obj, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_flex_flow(bt_scanlist_obj, LV_FLEX_FLOW_COLUMN);
    lv_obj_add_flag(bt_scanlist_obj, LV_OBJ_FLAG_USER_2);  // user2 为纵向
    lv_obj_set_flex_align(bt_scanlist_obj, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END,
                          LV_FLEX_ALIGN_START);

    bt_scan_list_group = lv_group_create();
    lv_group_set_user_focus_check_cb(bt_scan_list_group, lv_pro_bt_group_user_focus_check_cb);
#if 0
    for (int i = 0; i < SCAN_LIST_LEN; i++) {
        char name_with_id[10];
        snprintf(name_with_id, sizeof(name_with_id), "Name %d", i);
        bt_scan_item[i] = lv_pro_set_btn_style6_create(bt_scanlist_obj);
        lv_obj_set_size(bt_scan_item[i], lv_pct(100), LV_SIZE_CONTENT);
        lv_pro_set_btn_style6_src(bt_scan_item[i], lv_pro_bt_get_resource_path(BT_SCAN_LOGO_DF),
                                  &GENERAL_FONT_NORMAL, lv_color_white(), name_with_id,
                                  &GENERAL_FONT_SMALL, lv_color_hex(0XC0C0C0), "Mac",
                                  &GENERAL_FONT_NORMAL, lv_color_white(), "", bt_scan_list_group);
        lv_obj_add_event_cb(bt_scan_item[i], key_event_cb, LV_EVENT_KEY, "Scan_dev");
        lv_obj_add_flag(bt_scan_item[i], LV_OBJ_FLAG_HIDDEN);
    }
#endif
    lv_pro_bt_group_obj_clear_focus(bt_scan_list_group);

    if (!bt_state_on) {
        lv_obj_add_flag(bt_scan_obj, LV_OBJ_FLAG_HIDDEN);
    }

    current_group = bt_menu_group;
    lv_obj_update_layout(lv_pro_bt_activity);

    BT_DBG("finish\n");
}

static void lv_pro_bt_ui_deinit(void)
{
    BT_DBG("start\n");

    if (htc_msg_box) {
        lv_pro_msgbox_del_handle(NULL);
        htc_msg_box = NULL;
    }

    if (bt_scan_list_group) {
        lv_group_remove_all_objs(bt_scan_list_group);
        lv_group_del(bt_scan_list_group);
        bt_scan_list_group = NULL;
    }
    if (bt_scan_obj) {
        lv_obj_del(bt_scan_obj);
        bt_scan_obj = NULL;
    }
    if (bt_paired_list_group) {
        lv_group_remove_all_objs(bt_paired_list_group);
        lv_group_del(bt_paired_list_group);
        bt_paired_list_group = NULL;
    }
    if (bt_pair_obj) {
        lv_obj_del(bt_pair_obj);
        bt_pair_obj = NULL;
    }
    if (bt_menu_group) {
        lv_group_remove_all_objs(bt_menu_group);
        lv_group_del(bt_menu_group);
        bt_menu_group = NULL;
    }
    if (bt_menu_obj) {
        lv_obj_del(bt_menu_obj);
        bt_menu_obj = NULL;
    }
    if (lv_pro_bt_activity) {
        lv_obj_del_async(lv_pro_bt_activity);
        lv_pro_bt_activity = NULL;
    }

    clear_bt_scan_items();
    clear_bt_pair_items();

    bt_lvgl_active = false;
    bt_lvgl_exiting = false;
    BT_DBG("finish\n");
}

LV_IMG_DECLARE(IDB_Icon_unsupported);

int lv_pro_bt_init(void)
{
    if (lv_pro_bt_activity || bt_lvgl_active || bt_lvgl_exiting) {
        BT_ERR("bt activity already init!\n");
        return -1;
    }

    if (autoon_proc_run) {
        lv_pro_msgbox_msg_open_on_top(&IDB_Icon_unsupported, lv_get_string(STR_BT_READY), 2000);
        return -1;
    }

    if (!bt_modules_loaded) {
        lv_pro_msgbox_msg_open_on_top(&IDB_Icon_unsupported, lv_get_string(STR_BT_DEV_DETECT_FAIL),
                                      2000);
        return -1;
    }

    // bt背景图
    lv_pro_bt_activity = lv_obj_create(NULL);
    lv_obj_set_size(lv_pro_bt_activity, lv_pct(100), lv_pct(100));
    lv_obj_set_style_border_width(lv_pro_bt_activity, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(lv_pro_bt_activity, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(lv_pro_bt_activity, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(lv_pro_bt_activity, COLOR_BLUE, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(lv_pro_bt_activity, LV_SCROLLBAR_MODE_OFF);

    lv_pro_bt_ui_init();

    return 0;
}

int lv_pro_bt_lanucher_auto_on(void)
{
    pthread_t bt_thread;
    if (pthread_create(&bt_thread, NULL, bt_autoinit_proc, NULL) != 0) {
        BT_ERR("failed to create bt autoinit thread\n");
        return -1;
    }
    pthread_detach(bt_thread);

    return 0;
}

static void *bt_autoinit_proc(void *arg)
{
    int ret;
    autoon_proc_run = true;

    while (wireless_autoon_proc_run) {
        usleep(10 * 1000);
    }

    if (wireless_modules_loaded) {
        bt_modules_loaded = true;
    } else {
        bt_modules_loaded = false;
        goto AUTOON_END;
    }

    // Check BT state
    if ((ret = get_file_bt_state()) != 1) {
        goto AUTOON_END;
    }

    // Initialize BT
    if ((ret = lv_pro_res_bt_init()) != 0) {
        BT_ERR("bt init err!\n");
        goto AUTOON_END;
    }

    // Enable
    if ((ret = lv_pro_res_bt_on()) != 0) {
        BT_ERR("bt open err\n");
        lv_pro_res_bt_deinit();
        goto AUTOON_END;
    }

    lv_pro_msgbox_msg_clear();
    bt_state_on = true;

    usleep(1200 * 1000);
    pthread_t bt_reconnect;
    pthread_create(&bt_reconnect, NULL, bt_reconnct_proc, NULL);
    pthread_detach(bt_reconnect);

AUTOON_END:
    autoon_proc_run = false;
    pthread_exit((void *)1);
    return NULL;
}

static void *bt_reconnct_proc(void *arg)
{
    int ret;
    bt_reconn_ing = true;

    char devMac[18];
    char devName[256];
    ret = get_file_bt_connectstate(devMac, sizeof(devMac), devName, sizeof(devName));
    BT_DBG("ret=%d devMac=%s devName=%s\n", ret, devMac, devName);
    if (ret == 1) {
        lv_pro_set_bt_state(LVGL_BT_STATE_CONNECTING);
        ret = lv_pro_res_bt_connect(devMac);
        lv_pro_set_bt_state(LVGL_BT_STATE_IDLE);
    }

    if (ret != 0) {
        if (pairstate_proc_run && pairstate_proclock) lv_pro_bt_freshui_ack();
    }

    bt_reconn_ing = false;
    pthread_exit((void *)1);
    return NULL;
}

static void backg_scan_timeout(lv_timer_t *timer)
{
    // 超时后关闭蓝牙扫描
    BT_DBG("ing\n");
    if (backg_scan_timer) {
        lv_timer_del(backg_scan_timer);
        backg_scan_timer = NULL;
    }

    if (bt_scanstate) stop_btscan(NULL);
}

static int create_lvgl_bt(void)
{
    BT_DBG("start\n");

    lv_obj_set_parent(lv_pro_bt_activity, launcher_activity);
    load_current_channel(lv_pro_bt_activity, bt_menu_group);

    bt_lvgl_active = true;

    if (backg_scan_timer) {
        lv_timer_del(backg_scan_timer);
        backg_scan_timer = NULL;
    }

    if (bt_state_on) {
        lv_pro_bt_msg_enqueue(MSG_TYPE_BT_CREARE_FRESHUI_PROC, NULL, false);
        if (!bt_reconn_ing) lv_pro_bt_msg_enqueue(MSG_TYPE_BT_CREARE_SCANDEV_PROC, NULL, false);
    }

    BT_DBG("finish\n");
    return 0;
}

static int destory_lvgl_bt(void)
{
    BT_DBG("start\n");
    bt_lvgl_exiting = true;
    if (bt_state_on) {
        // lv_pro_bt_msg_enqueue(MSG_TYPE_BT_STOP_SCANPROC, NULL, false);
        if (!backg_scan_timer && scan_proc_run)
            backg_scan_timer = lv_timer_create(backg_scan_timeout, 2000, NULL);
        lv_pro_bt_msg_enqueue(MSG_TYPE_BT_STOP_PAIRPROC, NULL, false);
    }
    // 适配快速退出，忽略等待pair scan线程
#if 0
    wait_for_procs_to_finish();
#else
    if (pairstate_proc_run && pairstate_proclock) lv_pro_bt_freshui_ack();
    wait_control_proc_stop();
#endif

    if (show_msgbox) {
        lv_pro_del_message2_box();
        show_msgbox = false;
    }

    lv_pro_bt_msg_enqueue(MSG_TYPE_BT_LVGL_UI_DEINIT, NULL, false);
    BT_DBG("finish\n");
    return 0;
}

static int show_lvgl_bt(void)
{
    BT_DBG("\n");
    return 0;
}

static int hide_lvgl_bt(void)
{
    BT_DBG("\n");
    return 0;
}

static page_interface_t page_lvgl_bt = {.ops =
                                            {
                                                create_lvgl_bt,
                                                destory_lvgl_bt,
                                                show_lvgl_bt,
                                                hide_lvgl_bt,
                                            },
                                        .info = {.id = PAGE_BT, .user_data = NULL}};

void REGISTER_PAGE_BT(void) { reg_page(&page_lvgl_bt); }

#endif /* ENABLE_BT */
