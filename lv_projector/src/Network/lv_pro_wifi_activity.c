/*
 * lv_pro_wifi_activity.c
 *
 *  Created on: 2024/09/20
 *      Author: JASON
 */

#include "lv_pro_wifi_activity.h"

#include "lv_pro_connect_wifi_activity.h"

#if ENABLE_WIFI

static void lv_pro_wifi_ui_init(void);
static void lv_pro_wifi_ui_deinit(void);
static void *wifi_control_proc(void *arg);
static void *wifi_pairstate_proc(void *arg);
static void *wifi_scan_proc(void *arg);
static void *wifi_changedstate_proc(void *arg);
static void *wifi_sta_mode_init_proc(void *arg);
static void create_wifi_scan_item(int i);
static void clear_wifi_scan_items();
static void create_wifi_pair_item(int i);
static void clear_wifi_pair_items();

/*******************************WLAN RESOURCE*******************************/
static char select_ssid[64] = {0};
static char select_bssid[18] = {0};
static char select_mgmt[64] = {0};
static char select_pwd[64] = {0};
static int select_rssi = 0;
static int select_sec = 0;

static bool wifi_reseton = false;
static volatile bool wifi_state_on = false;
static bool wifi_off_ing = false;
static bool wifi_on_ing = false;
static bool wifi_scanstate = false;
static bool wifi_modules_loaded = false;

static bool autoon_proc_run = false;
static bool control_proc_run = false;
static bool connect_proc_run = false;
static bool pairstate_proc_run = false;
static bool scan_proc_run = false;
static bool pairstate_proclock = false;

static bool show_msgbox = false;

/*******************************WALN LVGL***********************************/
lv_obj_t *lv_pro_wifi_activity;
lv_group_t *wifi_menu_group;
lv_group_t *wifi_paired_list_group;
lv_group_t *wifi_scan_list_group;

static lv_group_t *current_group;
static lv_obj_t *wifi_menu_obj;
static lv_obj_t *wifi_pair_obj;
static lv_obj_t *wifi_scan_obj;
static lv_obj_t *wifi_pairlist_obj;
static lv_obj_t *wifi_pairnum_obj;
static lv_obj_t *wifi_pair_item[SCAN_LIST_LEN];
static lv_obj_t *wifi_scanlist_obj;
static lv_obj_t *wifi_scanstate_obj;
static lv_obj_t *wifi_scannum_obj;
static lv_obj_t *wifi_scan_item[SCAN_LIST_LEN];

static lv_timer_t *scan_timer;
static lv_obj_t *htc_msg_box;

static bool wifi_lvgl_active;
static bool wifi_lvgl_exiting;

static char *wifi_oldState = WIFI_STATE_OFF;
/***********************************************************************/

// pthread msg sync
static pthread_mutex_t *wifi_mut;
static pthread_cond_t *wifi_cond;

pthread_mutex_t *lv_pro_wifi_get_flesh_ui_mutex(void) { return wifi_mut; }

pthread_cond_t *lv_pro_wifi_get_flesh_ui_cond(void) { return wifi_cond; }

void lv_pro_wifi_freshui_ack(void)
{
    if (!pairstate_proc_run) {
        return;
    }

    pthread_mutex_t *wifi_mut = lv_pro_wifi_get_flesh_ui_mutex();
    pthread_cond_t *wifi_cond = lv_pro_wifi_get_flesh_ui_cond();

    if (wifi_mut && wifi_cond) {
        pthread_mutex_lock(wifi_mut);
        pthread_cond_signal(wifi_cond);
        pthread_mutex_unlock(wifi_mut);
    }
}

static void lv_pro_wifi_freahui_wait_ack(void)
{
    pthread_mutex_t *wifi_mut = lv_pro_wifi_get_flesh_ui_mutex();
    pthread_cond_t *wifi_cond = lv_pro_wifi_get_flesh_ui_cond();

    if (wifi_mut && wifi_cond) {
        pthread_mutex_lock(wifi_mut);
        pairstate_proclock = true;
        pthread_cond_wait(wifi_cond, wifi_mut);
        pthread_mutex_unlock(wifi_mut);
        pairstate_proclock = false;
    }
}

static bool shouldExitWifi()
{
    bool isWifiInactive = !wifi_state_on;
    bool isExitingLvgl = wifi_lvgl_exiting;
    bool isWifiOffing = wifi_off_ing;
    bool isLvglActive = wifi_lvgl_active;

    bool exitCondition = isWifiInactive || isExitingLvgl || !isLvglActive || isWifiOffing;

    if (exitCondition) {
        WIFI_DBG(
            "Exiting WIFI: UI Active: %d, LVGL Exiting: %d, WIFI Offing: %d, WIFI "
            "State: %d\n",
            isLvglActive, isExitingLvgl, isWifiOffing, isWifiInactive);
    }

    return exitCondition;
}

static void wait_for_procs_to_finish()
{
    int timenum = 0;

    if (pairstate_proc_run && pairstate_proclock) lv_pro_wifi_freshui_ack();

    while (control_proc_run || pairstate_proc_run || scan_proc_run) {
        usleep(10 * 1000);
        timenum++;
        if (timenum > 50) {
            timenum = 0;
            WIFI_WRN("control_proc_run=%d, pairstate_proc_run=%d, scan_proc_run=%d\n",
                     control_proc_run, pairstate_proc_run, scan_proc_run);
        }
    }
}

static void wait_scan_proc_stop()
{
    int timenum = 0;
    while (scan_proc_run) {
        usleep(10 * 1000);
        timenum++;
        if (timenum > 50) {
            timenum = 0;
            WIFI_WRN("scan_proc_run=%d\n", scan_proc_run);
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
            WIFI_WRN("control_proc_run=%d\n", control_proc_run);
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
            WIFI_WRN("pairstate_proc_run=%d\n", pairstate_proc_run);
        }
    }
}

static void stop_wifiscan(lv_timer_t *timer)
{
    if (timer == NULL) return;
    lv_timer_del(timer);
    scan_timer = NULL;

    lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_HIDDEN_SCANSTATE_OBJ, NULL, false);
    wifi_scanstate = false;

    while (scan_proc_run) {
        usleep(10 * 1000);
    }
    WIFI_DBG("stop scan finish!\n");
}

static void lv_pro_wifi_toggle_state(void *obj, const char *new_state)
{
    if (obj == NULL) return;
    lv_pro_set_btn_style3_t *btn = (lv_pro_set_btn_style3_t *)obj;
    WIFI_DBG("wifi try changes state[%s->%s]\n", wifi_oldState, new_state);
    wifi_oldState = new_state;

    lv_pro_set_btn_style4_t *_btn4;
    if (strcmp(new_state, WIFI_STATE_ON) == 0) {
        _btn4 = (lv_pro_set_btn_style4_t *)lv_obj_get_child(wifi_menu_obj, 2);
        lv_obj_set_style_text_color(_btn4->name, lv_color_white(), 0);
        _btn4 = (lv_pro_set_btn_style4_t *)lv_obj_get_child(wifi_menu_obj, 3);
        lv_obj_set_style_text_color(_btn4->name, lv_color_white(), 0);
    } else if (strcmp(new_state, WIFI_STATE_OFF) == 0) {
        _btn4 = (lv_pro_set_btn_style4_t *)lv_obj_get_child(wifi_menu_obj, 2);
        lv_obj_set_style_text_color(_btn4->name, lv_color_make(0xa3, 0xa3, 0xa3), 0);
        _btn4 = (lv_pro_set_btn_style4_t *)lv_obj_get_child(wifi_menu_obj, 3);
        lv_obj_set_style_text_color(_btn4->name, lv_color_make(0xa3, 0xa3, 0xa3), 0);
    }

    // reflesh ui
    for (int i = 0; i < SCAN_LIST_LEN; i++) {
        if (wifi_pair_item[i]) lv_pro_wifi_clear_devobj(wifi_pair_item[i]);
        if (wifi_scan_item[i]) lv_pro_wifi_clear_devobj(wifi_scan_item[i]);
    }
    lv_label_set_text(wifi_pairnum_obj, " 0 / 0");
    if (strcmp(new_state, WIFI_STATE_ON) == 0) {
        lv_obj_clear_flag(wifi_pair_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(wifi_scan_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(wifi_pairlist_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(wifi_scanlist_obj, LV_OBJ_FLAG_HIDDEN);
    } else if (strcmp(new_state, WIFI_STATE_OFF) == 0) {
        lv_obj_add_flag(wifi_pair_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(wifi_scan_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(wifi_pairlist_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(wifi_scanlist_obj, LV_OBJ_FLAG_HIDDEN);
    }

    // wifi ctrl
    wifi_thread_args_btn3_t *wifictrl = malloc(sizeof(wifi_thread_args_btn3_t));
    if (wifictrl != NULL) {
        wifictrl->btn = btn;
        wifictrl->state = new_state;

        wait_control_proc_stop();
        pthread_t wifi_control;
        pthread_create(&wifi_control, NULL, wifi_control_proc, wifictrl);
        pthread_detach(wifi_control);
    } else {
        WIFI_ERR("wifictrl malloc failed\n");
    }
}

static void lv_pro_wifi_conn_status_changed(void *obj, void *dev, int taskstate, void *user_data)
{
    lv_pro_set_btn_style6_t *btn = (lv_pro_set_btn_style6_t *)obj;
    wifi_thread_args_btn6_t *arg_dev = (wifi_thread_args_btn6_t *)dev;

    if (btn != NULL) {
        char *new_state_str = NULL;
        if (taskstate == LVGL_WIFI_STATE_CONNECTING) {
            new_state_str = lv_get_string(STR_WIFI_CONNECTING);
        } else if (taskstate == LVGL_WIFI_STATE_DISCONNECTING) {
            new_state_str = lv_get_string(STR_WIFI_DISCONNECT);
        } else if (taskstate == LVGL_WIFI_STATE_UNPAIRING) {
            new_state_str = lv_get_string(STR_WIFI_CANCEL);
        }
        lv_pro_set_btn_style6_set_state_str(btn, new_state_str);
    }

    lv_pro_set_wifi_state(taskstate);

    bool set_dev = false;
    wifi_thread_args_btn6_t *msg = malloc(sizeof(wifi_thread_args_btn6_t));
    if (msg != NULL) {
        if (btn) {
            msg->btn = btn;
            msg->state = user_data;
            msg->sec = lv_pro_set_btn_style6_get_sec_value(msg->btn);
            msg->rssi = lv_pro_set_btn_style6_get_rssi_value(msg->btn);
            char *ssid = lv_pro_set_btn_style6_get_name_str(msg->btn);
            char *bssid = lv_pro_set_btn_style6_get_mac_addr_str(msg->btn);
            char *pwd = lv_pro_set_btn_style6_get_pwd(msg->btn);
            strncpy(msg->ssid, ssid, sizeof(msg->ssid) - 1);
            msg->ssid[sizeof(msg->ssid) - 1] = '\0';
            strncpy(msg->bssid, bssid, sizeof(msg->bssid) - 1);
            msg->bssid[sizeof(msg->bssid) - 1] = '\0';
            strncpy(msg->password, pwd, sizeof(msg->bssid) - 1);
            msg->password[sizeof(msg->password) - 1] = '\0';
            set_dev = true;
        }
        if (arg_dev) {
            msg->btn = arg_dev->btn;
            msg->state = user_data;
            msg->sec = arg_dev->sec;
            msg->rssi = arg_dev->rssi;
            snprintf(msg->ssid, sizeof(msg->ssid), "%s", arg_dev->ssid);
            snprintf(msg->bssid, sizeof(msg->bssid), "%s", arg_dev->bssid);
            snprintf(msg->mgmt, sizeof(msg->mgmt), "%s", arg_dev->mgmt);
            snprintf(msg->password, sizeof(msg->password), "%s", arg_dev->password);
            set_dev = true;
        }

        if (set_dev != true) {
            free(msg);
            WIFI_ERR("set_dev failed\n");
            return;
        }

        pthread_t wifi_connect;
        pthread_create(&wifi_connect, NULL, wifi_changedstate_proc, msg);
        pthread_detach(wifi_connect);
    } else {
        WIFI_ERR("msg malloc failed\n");
    }
}

static bool is_canprintf_message(lvgl_wifi_msg_t *msg)
{
    return (msg->type != MSG_TYPE_WIFI_LV_KEY_DOWN && msg->type != MSG_TYPE_WIFI_LV_KEY_UP &&
            msg->type != MSG_TYPE_WIFI_HIDDEN_ALL_SCANDEV_OBJ &&
            msg->type != MSG_TYPE_WIFI_HIDDEN_ALL_PAIRDEV_OBJ &&
            msg->type != MSG_TYPE_WIFI_UPDATE_SCANDEV_OBJ &&
            msg->type != MSG_TYPE_WIFI_UPDATE_PAIRDEV_OBJ &&
            msg->type != MSG_TYPE_WIFI_HIDDEN_DEV_OBJ &&
            msg->type != MSG_TYPE_WIFI_HIDDEN_SCANSTATE_OBJ &&
            msg->type != MSG_TYPE_WIFI_CLEARHIDDEN_SCANSTATE_OBJ &&
            msg->type != MSG_TYPE_WIFI_UPDATE_PAIRNUMBER_OBJ &&
            msg->type != MSG_TYPE_WIFI_SET_DEV_NOTSAVESTATE &&
            msg->type != MSG_TYPE_WIFI_UPDATE_SCANDEVNUM_OBJ);
}

static bool lv_pro_wifi_msg_need_ignore(lvgl_wifi_msg_t *msg)
{
    if (wifi_lvgl_exiting) {
        // msg->type != MSG_TYPE_WIFI_STOP_SCANPROC  优化退出
        return (msg->type != MSG_TYPE_WIFI_STOP_PAIRPROC &&
                msg->type != MSG_TYPE_WIFI_LVGL_UI_DEINIT);
    }
    return false;
}

void wifi_message_process(system_msg_t *sys_msg)
{
    lv_group_t *__key_group;
    WIFIDeviceMsg *device;
    lvgl_wifi_msg_t *msg;
    void *btn;

    if (sys_msg == NULL) {
        WIFI_ERR("sys msg null!\n");
        return;
    }

    if (sys_msg->type != MSG_TYPE_WIFI_FRESH_UI) {
        if (sys_msg->type == MSG_TYPE_WIFI_STA_CONNECT ||
            sys_msg->type == MSG_TYPE_WIFI_STA_DISCONNECT) {
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
        WIFI_DBG("type=%d (%s)\n", msg->type, lv_pro_wifi_msg_type_to_string(msg->type));
    }

    if (!wifi_lvgl_active || lv_pro_wifi_msg_need_ignore(msg)) {
        WIFI_WRN("msg:[%s] ignore process!\n", lv_pro_wifi_msg_type_to_string(msg->type));
        goto MSG_END;
    }

    switch (msg->type) {
    case MSG_TYPE_WIFI_HIDDEN_ALL_SCANDEV_OBJ:
        lv_pro_wifi_hidden_all_devobj(wifi_scan_item, SCAN_LIST_LEN);
        break;

    case MSG_TYPE_WIFI_HIDDEN_ALL_PAIRDEV_OBJ:
        lv_pro_wifi_hidden_all_devobj(wifi_pair_item, SCAN_LIST_LEN);
        break;

    case MSG_TYPE_WIFI_UPDATE_SCANDEV_OBJ:
        device = (WIFIDeviceMsg *)msg->data;
        if (device) {
            if (!wifi_scan_item[device->id]) {
                create_wifi_scan_item(device->id);
            } else {
                lv_pro_wifi_clear_devobj(wifi_scan_item[device->id]);
            }
            lv_pro_wifi_update_devobj(wifi_scan_item[device->id], device);
        }

        break;
    case MSG_TYPE_WIFI_UPDATE_SCANDEVNUM_OBJ:
        if (wifi_scannum_obj) {
            lv_obj_clear_flag(wifi_scannum_obj, LV_OBJ_FLAG_HIDDEN);
            char scanNum[10];
            int allNum = lv_pro_wifi_group_num_all(wifi_scan_list_group);
            int devId = lv_pro_wifi_group_devid(wifi_scan_list_group,
                                                lv_group_get_focused(wifi_scan_list_group));
            snprintf(scanNum, 10, "%d / %d", devId, allNum);
            lv_label_set_text(wifi_scannum_obj, scanNum);
        }

    case MSG_TYPE_WIFI_UPDATE_PAIRDEV_OBJ:
        device = (WIFIDeviceMsg *)msg->data;
        if (device) {
            if (!wifi_pair_item[device->id]) {
                create_wifi_pair_item(device->id);
            } else {
                lv_pro_wifi_clear_devobj(wifi_pair_item[device->id]);
            }
            lv_pro_wifi_update_devobj(wifi_pair_item[device->id], device);
        }

        break;

    case MSG_TYPE_WIFI_HIDDEN_DEV_OBJ:
        btn = msg->data;
        if (btn) lv_pro_wifi_clear_devobj(btn);
        break;

    // 扫描状态和扫描结果互斥
    case MSG_TYPE_WIFI_HIDDEN_SCANSTATE_OBJ:
        if (wifi_scanstate_obj) lv_obj_add_flag(wifi_scanstate_obj, LV_OBJ_FLAG_HIDDEN);
        if (wifi_scannum_obj) lv_obj_clear_flag(wifi_scannum_obj, LV_OBJ_FLAG_HIDDEN);
        break;

    case MSG_TYPE_WIFI_CLEARHIDDEN_SCANSTATE_OBJ:
        if (wifi_scanstate_obj) lv_obj_clear_flag(wifi_scanstate_obj, LV_OBJ_FLAG_HIDDEN);
        if (wifi_scannum_obj) {
            lv_label_set_text(wifi_scannum_obj, " ");
            lv_obj_add_flag(wifi_scannum_obj, LV_OBJ_FLAG_HIDDEN);
        }
        break;

    case MSG_TYPE_WIFI_UPDATE_PAIRNUMBER_OBJ: {
        if (!wifi_pairnum_obj) break;
        lv_label_set_text(wifi_pairnum_obj, (char *)msg->data);
        lv_obj_clear_flag(wifi_pairnum_obj, LV_OBJ_FLAG_HIDDEN);
        break;
    }

    case MSG_TYPE_WIFI_LV_KEY_UP: {
        __key_group = (lv_group_t *)msg->data;
        if (!__key_group) break;
        if (__key_group == wifi_scan_list_group && lv_group_get_obj_is_head(__key_group, NULL)) {
            if (lv_pro_wifi_group_change(wifi_paired_list_group, true, NULL) == 0) {
                current_group = wifi_paired_list_group;
                lv_pro_wifi_group_obj_clear_focus(__key_group);
            }
            if (current_group == wifi_paired_list_group && wifi_scannum_obj) {
                lv_label_set_text(wifi_scannum_obj, " ");
                lv_obj_add_flag(wifi_scannum_obj, LV_OBJ_FLAG_HIDDEN);
            }
        } else {
            lv_group_focus_obj(lv_group_get_prev(__key_group, NULL));
            if (__key_group == wifi_scan_list_group) {
                lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_UPDATE_SCANDEVNUM_OBJ, NULL, false);
            }
        }
        break;
    }

    case MSG_TYPE_WIFI_LV_KEY_DOWN: {
        __key_group = (lv_group_t *)msg->data;
        if (!__key_group) break;
        if (__key_group == wifi_paired_list_group && lv_group_get_obj_is_end(__key_group, NULL)) {
            if (lv_pro_wifi_group_change(wifi_scan_list_group, false, NULL) == 0) {
                current_group = wifi_scan_list_group;
                lv_pro_wifi_group_obj_clear_focus(__key_group);
            }
            if (current_group == wifi_scan_list_group)
                lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_UPDATE_SCANDEVNUM_OBJ, NULL, false);
        } else {
            lv_group_focus_obj(lv_group_get_next(__key_group, NULL));
            if (__key_group == wifi_scan_list_group)
                lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_UPDATE_SCANDEVNUM_OBJ, NULL, false);
        }
        break;
    }

    case MSG_TYPE_WIFI_CHANGED_TO_MENUGROUP: {
        __key_group = (lv_group_t *)msg->data;
        if (!__key_group) break;
        lv_pro_wifi_group_obj_clear_focus(__key_group);
        lv_pro_wifi_group_change(wifi_menu_group, false, NULL);
        current_group = wifi_menu_group;
        if (wifi_scannum_obj) lv_obj_add_flag(wifi_scannum_obj, LV_OBJ_FLAG_HIDDEN);
        break;
    }

    case MSG_TYPE_WIFI_CHANGED_TO_RIGHTGROUP: {
        __key_group = (lv_group_t *)msg->data;
        if (!__key_group) break;
        if (lv_pro_wifi_group_change(wifi_paired_list_group, false, NULL) == 0) {
            current_group = wifi_paired_list_group;
        } else if (lv_pro_wifi_group_change(wifi_scan_list_group, false, NULL) == 0) {
            current_group = wifi_scan_list_group;
        } else {
            current_group = __key_group;
        }
        if (current_group == wifi_scan_list_group) {
            lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_UPDATE_SCANDEVNUM_OBJ, NULL, false);
        }
        break;
    }

    case MSG_TYPE_WIFI_TRY_GRUOP_FOCUS: {
        __key_group = (lv_group_t *)msg->data;
        if (!__key_group || !lv_pro_is_wifi_state_idle()) break;
        lv_pro_wifi_group_obj_clear_focus(current_group);
        lv_pro_wifi_group_obj_clear_focus(__key_group);
        if (lv_pro_wifi_group_change(__key_group, false, NULL) == 0) {
            current_group = __key_group;
        } else {
            lv_pro_wifi_group_change(wifi_menu_group, false, NULL);
            current_group = wifi_menu_group;
        }

        if (current_group != wifi_scan_list_group && wifi_scannum_obj) {
            lv_label_set_text(wifi_scannum_obj, " ");
            lv_obj_add_flag(wifi_scannum_obj, LV_OBJ_FLAG_HIDDEN);
        }
        break;
    }

    case MSG_TYPE_WIFI_CHANGED_HOME_CONNLOGO:
    case MSG_TYPE_WIFI_CHANGED_HOME_DISCONNLOGO: {
        lv_pro_res_wifi_logo_show(msg->type == MSG_TYPE_WIFI_CHANGED_HOME_CONNLOGO);
        break;
    }

    case MSG_TYPE_WIFI_MODE_SWITCH: {
        btn = msg->data;
        if (btn) lv_pro_wifi_set_logostate(btn, 2);
        break;
    }

    case MSG_TYPE_WIFI_MODE_SWITCH_BTN_ENABLE: {
        btn = msg->data;
        if (btn) lv_pro_wifi_set_logostate(btn, -1);
        break;
    }

    case MSG_TYPE_WIFI_WORK_STATE_OPEN: {
        btn = msg->data;
        lv_pro_create_message2_box((char *)lv_get_string(STR_WIFI_OPENING));
        show_msgbox = true;
        if (btn) lv_pro_wifi_toggle_state(btn, WIFI_STATE_ON);
        break;
    }

    case MSG_TYPE_WIFI_WORK_STATE_OPENSUCCESS: {
        if (show_msgbox) {
            lv_pro_del_message2_box();
            show_msgbox = false;
        }
        if (msg->data && !wifi_lvgl_exiting && wifi_lvgl_active)
            lv_pro_wifi_group_change(msg->data, false, NULL);
        break;
    }

    case MSG_TYPE_WIFI_WORK_STATE_OPENFAIL: {
        btn = msg->data;
        if (show_msgbox) {
            lv_pro_del_message2_box();
            show_msgbox = false;
        }
        if (btn) lv_pro_wifi_set_logostate(btn, 0);
        break;
    }

    case MSG_TYPE_WIFI_WORK_STATE_CLOSE: {
        btn = msg->data;
        lv_pro_create_message2_box((char *)lv_get_string(STR_WIFI_CLOSEING));
        show_msgbox = true;
        if (btn) lv_pro_wifi_toggle_state(btn, WIFI_STATE_OFF);
        break;
    }

    case MSG_TYPE_WIFI_WORK_STATE_CLOSESUCCESS: {
        if (show_msgbox) {
            lv_pro_del_message2_box();
            show_msgbox = false;
        }
        if (msg->data && !wifi_lvgl_exiting && wifi_lvgl_active)
            lv_pro_wifi_group_change(msg->data, false, NULL);
        break;
    }

    case MSG_TYPE_WIFI_WORK_STATE_CLOSEFAIL: {
        btn = msg->data;
        if (show_msgbox) {
            lv_pro_del_message2_box();
            show_msgbox = false;
        }
        if (btn) lv_pro_wifi_set_logostate(btn, 1);
        break;
    }

    case MSG_TYPE_WIFI_CREARE_PAIRSTATE_PROC: {
        wait_pair_proc_stop();
        pthread_t wifi_pairstate;
        pthread_create(&wifi_pairstate, NULL, wifi_pairstate_proc, NULL);
        pthread_detach(wifi_pairstate);
        break;
    }

    case MSG_TYPE_WIFI_CREARE_SCANDEV_PROC: {
        for (int i = 0; i < SCAN_LIST_LEN; i++) {
            if (wifi_scan_item[i]) lv_pro_wifi_clear_devobj(wifi_scan_item[i]);
        }
        if (wifi_scanstate_obj) lv_obj_clear_flag(wifi_scanstate_obj, LV_OBJ_FLAG_HIDDEN);
        if (wifi_scannum_obj) {
            lv_label_set_text(wifi_scannum_obj, " ");
            lv_obj_add_flag(wifi_scannum_obj, LV_OBJ_FLAG_HIDDEN);
        }
        lv_obj_scroll_to_y(wifi_scanlist_obj, 0, LV_ANIM_ON);
        wait_scan_proc_stop();
        pthread_t wifi_scan;
        pthread_create(&wifi_scan, NULL, wifi_scan_proc, NULL);
        pthread_detach(wifi_scan);
        break;
    }

    case MSG_TYPE_WIFI_MANUALLY_ADD_DEV: {
        if (scan_proc_run) stop_wifiscan(scan_timer);
        lv_pro_selectbox_wifi_init(NULL, LVGL_WIFI_STATE_PAIRING);
        break;
    }

    case MSG_TYPE_WIFI_CLICKED_PAIRDEV: {
        if (scan_proc_run) stop_wifiscan(scan_timer);
        btn = (lv_pro_set_btn_style6_t *)msg->data;
        if (!btn) break;
        char *btn_state = lv_pro_set_btn_style6_get_state_str(btn);
        if (!strcmp(btn_state, lv_get_string(STR_WIFI_SAVED))) {
            lv_pro_selectbox_wifi_init(btn, LVGL_WIFI_STATE_CONNECTING);
        } else if (!strcmp(btn_state, lv_get_string(STR_WIFI_CONN))) {
            lv_pro_selectbox_wifi_init(btn, LVGL_WIFI_STATE_DISCONNECTING);
        }
        break;
    }

    case MSG_TYPE_WIFI_CONN_PAIRDEV: {
        if (scan_proc_run) stop_wifiscan(scan_timer);
        if (msg->data) {
            btn = msg->data;
            lv_pro_wifi_conn_status_changed(btn, NULL, LVGL_WIFI_STATE_CONNECTING,
                                            WIFI_STATE_PAIRLIST_CONN);
        }

        break;
    }

    case MSG_TYPE_WIFI_NEWCONN_PAIRDEV: {
        if (scan_proc_run) stop_wifiscan(scan_timer);
        if (msg->data) {
            wifi_thread_args_btn6_t *dev = msg->data;
            lv_pro_wifi_conn_status_changed(dev->btn, dev, LVGL_WIFI_STATE_CONNECTING,
                                            WIFI_STATE_ADDNEWDEV_CONN);
        }
        break;
    }

    case MSG_TYPE_WIFI_DISCONN_PAIRDEV: {
        if (scan_proc_run) stop_wifiscan(scan_timer);
        if (msg->data) {
            btn = msg->data;
            lv_pro_wifi_conn_status_changed(btn, NULL, LVGL_WIFI_STATE_DISCONNECTING,
                                            WIFI_STATE_PAIRLIST_DISCONN);
        }
        // 客户要求断开后重新触发扫描
        lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_CREARE_SCANDEV_PROC, NULL, false);
        break;
    }

    case MSG_TYPE_WIFI_DELETED_PAIRDEV: {
        if (scan_proc_run) stop_wifiscan(scan_timer);
        if (msg->data) {
            btn = msg->data;
            lv_pro_wifi_conn_status_changed(btn, NULL, LVGL_WIFI_STATE_UNPAIRING,
                                            WIFI_STATE_PAIRLIST_UNPAIR);
        }
        // 客户要求断开后重新触发扫描
        lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_CREARE_SCANDEV_PROC, NULL, false);
        break;
    }

    case MSG_TYPE_WIFI_CLICKER_SCANDEV: {
        if (scan_proc_run) stop_wifiscan(scan_timer);
        if (!msg->data) break;
        lv_pro_set_btn_style6_t *btn = (lv_pro_set_btn_style6_t *)msg->data;
        if (btn->sec == WIFI_SEC_NONE) {
            lv_pro_wifi_conn_status_changed(btn, NULL, LVGL_WIFI_STATE_CONNECTING,
                                            WIFI_STATE_SCANLIST_CONN);
        } else {
            lv_pro_connect_wifi_init(btn);
        }
        break;
    }

    case MSG_TYPE_WIFI_CONN_SCANDEV: {
        if (msg->data) {
            btn = msg->data;
            lv_pro_wifi_conn_status_changed(btn, NULL, LVGL_WIFI_STATE_CONNECTING,
                                            WIFI_STATE_SCANLIST_CONN);
        }
        break;
    }

    case MSG_TYPE_WIFI_CONN_SCANDEV_FAIL: {
        btn = msg->data;
        if (!btn) break;
        lv_pro_set_btn_style6_t *btn6 = (lv_pro_set_btn_style6_t *)btn;
        memset(select_mgmt, '\0', sizeof(select_mgmt));
        key_mgmt_to_char(select_mgmt, btn6->sec);
        lv_pro_set_btn_style6_set_state_str(btn6, select_mgmt);
        break;
    }

    case MSG_TYPE_WIFI_CONN_PAIRDEV_FAIL:
    case MSG_TYPE_WIFI_DISCONN_PAIRDEV_FAIL: {
        btn = msg->data;
        if (btn) lv_pro_set_btn_style6_set_state_str(btn, lv_get_string(STR_WIFI_SAVED));
        break;
    }

    case MSG_TYPE_WIFI_SET_DEV_NOTSAVESTATE:
        btn = msg->data;
        if (btn) lv_pro_set_btn_style6_set_state_str(btn, "");
        break;

    case MSG_TYPE_WIFI_STOP_SCANPROC:
        stop_wifiscan(scan_timer);
        break;

    case MSG_TYPE_WIFI_STOP_PAIRPROC:
        if (pairstate_proc_run && pairstate_proclock) lv_pro_wifi_freshui_ack();
        break;

        // case MSG_TYPE_WIFI_LVGL_UI_INIT:
        //     if (!wifi_lvgl_active) lv_pro_wifi_ui_init();
        //     break;

    case MSG_TYPE_WIFI_LVGL_UI_DEINIT:
        if (wifi_lvgl_active) lv_pro_wifi_ui_deinit();
        break;

    default:
        WIFI_ERR("error msg\n");
        break;
    }

MSG_END:

    if (is_canprintf_message(msg)) {
        WIFI_DBG("complete\n");
    }

    if (msg->free_data) {
        free(msg->data);
    }
    if (msg) {
        free(msg);
    }
}

static void *wifi_pairstate_proc(void *arg)
{
    (void)arg;

    if (pairstate_proc_run) {
        pthread_exit((void *)1);
        return NULL;
    }

    WIFI_DBG("start\n");
    pairstate_proc_run = true;

    wifi_mut = malloc(sizeof(pthread_mutex_t));
    wifi_cond = malloc(sizeof(pthread_cond_t));
    pthread_mutex_init(wifi_mut, NULL);
    pthread_cond_init(wifi_cond, NULL);

    bool proc_first_fresh = true;

    wifi_paired_result_t **devices = NULL;
    devices = malloc(SCAN_LIST_LEN * sizeof(wifi_paired_result_t *));
    memset(devices, 0, SCAN_LIST_LEN * sizeof(wifi_paired_result_t *));
    int pair_count = 0, connect_count = 0, found_dev = 0;

    while (wifi_state_on) {
        if (shouldExitWifi()) break;

        connect_count = 0, pair_count = 0, found_dev = 0;
        memset(devices, 0, SCAN_LIST_LEN * sizeof(wifi_paired_result_t *));
        lv_pro_res_wifi_paired_devices(devices, &pair_count);

        if (shouldExitWifi()) break;

        for (int j = 0; j < pair_count; j++) {
            devices[j]->dev_found = false;
            for (int i = 0; i < lv_pro_res_wifi_get_scan_results_num(); i++) {
                if (!strcmp(devices[j]->ssid, wifi_scan_res[i].ssid)) {
                    devices[j]->rssi = wifi_scan_res[i].rssi;
                    devices[j]->dev_found = true;
                    break;
                }
            }
            if (devices[j]->dev_found) {
                WIFI_DBG("Paired devices discoverable, ssid:%s\n", devices[j]->ssid);
            }
        }

        if (shouldExitWifi()) break;

        lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_HIDDEN_ALL_PAIRDEV_OBJ, NULL, false);

        if (pair_count > 0) {
            for (int i = 0; i < pair_count; i++) {
                WIFIDeviceMsg *devicemsg = (WIFIDeviceMsg *)malloc(sizeof(WIFIDeviceMsg));
                devicemsg->id = found_dev;
                devicemsg->status = WIFI_STATE_SAVED;
                devicemsg->rssi = devices[i]->rssi;
                if (devices[i]->conn_state) {
                    devicemsg->status = WIFI_STATE_CONNECTED;
                    connect_count++;
                }
                snprintf(devicemsg->ssid, sizeof(devicemsg->ssid), "%s", devices[i]->ssid);
                snprintf(devicemsg->bssid, sizeof(devicemsg->bssid), "%s", devices[i]->bssid);

                if (devices[i]->dev_found || devices[i]->conn_state) {
                    found_dev++;
                    lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_UPDATE_PAIRDEV_OBJ, devicemsg, true);
                } else {
                    free(devicemsg);
                    devicemsg = NULL;
                }

                free(devices[i]);
                devices[i] = NULL;

                if (shouldExitWifi()) {
                    break;
                }
            }

            /* 尝试聚焦的条件约束：
                1. 非线程创建首次刷新
                2. 不处于扫描进行状态
                3. 发现设备
                或：
                当且组正聚焦在paired list上。

            */
            if (found_dev && ((!lv_pro_res_wifi_get_scan_status_flag() && !proc_first_fresh) ||
                              current_group == wifi_paired_list_group)) {
                lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_TRY_GRUOP_FOCUS, wifi_paired_list_group,
                                        false);
            }
        } else {
            WIFI_DBG("paired device is empty\n");
            if (current_group && current_group == wifi_paired_list_group) {
                lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_CHANGED_TO_MENUGROUP, wifi_paired_list_group,
                                        false);
            }
        }

        char *state_result = malloc(10 * sizeof(char));
        WIFI_DBG("connect_count=%d, pair_count=%d found_dev=%d\n", connect_count, pair_count,
                 found_dev);
        snprintf(state_result, 10, "%d / %d", connect_count, found_dev);
        lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_UPDATE_PAIRNUMBER_OBJ, state_result, true);

        if (found_dev == 0 && current_group == wifi_paired_list_group) {
            lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_CHANGED_TO_MENUGROUP, wifi_paired_list_group,
                                    false);
        }
        proc_first_fresh = false;
        if (lv_pro_res_wifi_get_scan_status_flag()) {
            lv_pro_res_wifi_set_scan_status_flag(0);
        }
        lv_pro_wifi_freahui_wait_ack();  // 先刷新一遍，在等在阻塞
    }

    if (devices) {
        for (int i = 0; i < SCAN_LIST_LEN; i++) {
            if (devices[i]) {
                free(devices[i]);
                devices[i] = NULL;
            }
        }
        free(devices);
        devices = NULL;
    }

    pthread_mutex_destroy(wifi_mut);
    pthread_cond_destroy(wifi_cond);
    free(wifi_mut);
    free(wifi_cond);
    wifi_mut = NULL;
    wifi_cond = NULL;

    pairstate_proc_run = false;
    WIFI_DBG("exit\n");
    pthread_exit((void *)1);
    return NULL;
}

static void *wifi_scan_proc(void *arg)
{
    (void)arg;

    if (shouldExitWifi() || scan_proc_run || !wifi_state_on || !wifi_lvgl_active) {
        pthread_exit((void *)1);
        return NULL;
    }

    lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_CLEARHIDDEN_SCANSTATE_OBJ, NULL, false);
    scan_timer = lv_timer_create(stop_wifiscan, WIFI_SCAN_TIME_MS, NULL);
    wifi_scanstate = true;

    WIFI_DBG("start\n");
    scan_proc_run = true;

    while (wifi_state_on && wifi_scanstate) {
        if (shouldExitWifi()) {
            WIFI_DBG("break\n");
            break;
        }

        if (lv_pro_res_wifi_scan() != 0) break;

        if (shouldExitWifi()) {
            WIFI_DBG("break\n");
            break;
        }

        if (lv_pro_res_wifi_get_scan_results_num() == 0) {
            sleep(1);
            continue;
        }

        lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_HIDDEN_ALL_SCANDEV_OBJ, NULL, false);

        wifi_paired_result_t **_pairDevices = NULL;
        int pair_count = 0, add_count = 0;
        _pairDevices = malloc(SCAN_LIST_LEN * sizeof(wifi_paired_result_t *));
        memset(_pairDevices, 0, SCAN_LIST_LEN * sizeof(wifi_paired_result_t *));
        lv_pro_res_wifi_paired_devices(_pairDevices, &pair_count);

        for (int i = 0; i < lv_pro_res_wifi_get_scan_results_num(); i++) {
            if (!is_ssid_valid(wifi_scan_res[i].ssid)) {
                bool pdev_found = false;
                for (int j = 0; j < pair_count; j++) {
                    if (!strcmp(_pairDevices[j]->ssid, wifi_scan_res[i].ssid)) {
                        pdev_found = true;
                        break;
                    }
                }
                if (!pdev_found) {
                    WIFIDeviceMsg *device = lv_pro_wifi_scandev_init(&wifi_scan_res[i], add_count);
                    if (device) {
                        lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_UPDATE_SCANDEV_OBJ, device, true);
                        add_count++;
                    }
                }
            }
            if (shouldExitWifi()) {
                WIFI_DBG("break\n");
                break;
            }
        }

        if (_pairDevices) {
            for (int i = 0; i < SCAN_LIST_LEN; i++) {
                if (_pairDevices[i]) {
                    free(_pairDevices[i]);
                    _pairDevices[i] = NULL;
                }
            }
            free(_pairDevices);
            _pairDevices = NULL;
        }
        break;
    }

    scan_proc_run = false;
    stop_wifiscan(scan_timer);
    WIFI_DBG("exit\n");
    pthread_exit((void *)1);
    return NULL;
}

static void *wifi_control_proc(void *arg)
{
    wifi_thread_args_btn3_t *args = (wifi_thread_args_btn3_t *)arg;
    lv_pro_set_btn_style3_t *_btn = (lv_pro_set_btn_style3_t *)args->btn;

    if (control_proc_run) {
        free(args);
        WIFI_ERR("exit\n");
        pthread_exit((void *)1);
        return NULL;
    }

    WIFI_DBG("start changestate:%s\n", args->state);
    control_proc_run = true;

    if (!strcmp(args->state, WIFI_STATE_ON)) {
        if (!wifi_off_ing) {
            wifi_on_ing = true;
            if (lv_pro_res_wifi_init()) {
                WIFI_ERR("WIFI INIT ERR!\n");
                goto END;
            }
            if (lv_pro_res_wifi_on()) {
                WIFI_ERR("STA MODE INIT ERR\n");
                wifi_state_on = false;
                wifi_on_ing = false;
                // 需要隐藏界面
                update_file_wifi_state(WIFI_STATE_OFF);
                lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_WORK_STATE_OPENFAIL, args->btn, false);
                goto END;
            }
            update_file_wifi_state(WIFI_STATE_ON);
            lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_WORK_STATE_OPENSUCCESS, wifi_menu_group, false);
            wifi_state_on = true;
            wifi_on_ing = false;
        } else {
            lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_WORK_STATE_OPENFAIL, args->btn, false);
            goto END;
        }
        if (wifi_state_on) {
            if (!pairstate_proc_run) {
                lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_CREARE_PAIRSTATE_PROC, NULL, false);
            }
            if (!scan_proc_run) {
                lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_CREARE_SCANDEV_PROC, NULL, false);
            }
        }
    } else if (!strcmp(args->state, WIFI_STATE_OFF)) {
        if (!wifi_on_ing) {
            wifi_off_ing = true;
            if (scan_proc_run) stop_wifiscan(scan_timer);
            lv_pro_res_wifi_off();
            lv_pro_res_wifi_deinit();
            wifi_state_on = false;
            wifi_off_ing = false;
            update_file_wifi_state(WIFI_STATE_OFF);
            lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_WORK_STATE_CLOSESUCCESS, wifi_menu_group, false);
        } else {
            lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_WORK_STATE_CLOSEFAIL, args->btn, false);
            goto END;
        }
    }

END:
    free(args);
    lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_MODE_SWITCH_BTN_ENABLE, _btn, false);
    lv_pro_set_wifi_state(LVGL_WIFI_STATE_IDLE);
    control_proc_run = false;
    WIFI_DBG("exit\n");
    pthread_exit((void *)1);
    return NULL;
}

static void *wifi_changedstate_proc(void *arg)
{
    if (connect_proc_run) {
        pthread_exit((void *)1);
        return NULL;
    }

    WIFI_DBG("start\n");
    connect_proc_run = true;

    wifi_thread_args_btn6_t *args = (wifi_thread_args_btn6_t *)arg;

    if (!strcmp(args->state, WIFI_STATE_PAIRLIST_CONN)) {
        int pairdev_conn_result = lv_pro_res_wifi_sta_auto_conn(args->ssid);
        WIFI_DBG("pairdev_conn_result = %d\n", pairdev_conn_result);
        if (pairdev_conn_result) {
            if (!lv_obj_is_valid(htc_msg_box)) {
                htc_msg_box =
                    lv_pro_create_message_box((char *)lv_get_string(STR_WIFI_CONN_FAILED), 3000);
            }
            if (args->btn)
                lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_CONN_PAIRDEV_FAIL, args->btn, false);
        } else {
            if (!lv_obj_is_valid(htc_msg_box)) {
                htc_msg_box =
                    lv_pro_create_message_box((char *)lv_get_string(STR_WIFI_CONN_SUCCESS), 1000);
            }
        }
    } else if (!strcmp(args->state, WIFI_STATE_ADDNEWDEV_CONN)) {
        int newconn_result = lv_pro_res_wifi_connect(args->ssid, args->password, args->sec);
        WIFI_DBG("newconn_result = %d\n", newconn_result);
        if (newconn_result) {
            if (!lv_obj_is_valid(htc_msg_box)) {
                htc_msg_box =
                    lv_pro_create_message_box((char *)lv_get_string(STR_WIFI_CONN_FAILED), 3000);
            }
            if (args->btn)
                lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_CONN_PAIRDEV_FAIL, args->btn, false);
        } else {
            if (!lv_obj_is_valid(htc_msg_box)) {
                htc_msg_box =
                    lv_pro_create_message_box((char *)lv_get_string(STR_WIFI_CONN_SUCCESS), 1000);
            }
        }
    } else if (!strcmp(args->state, WIFI_STATE_SCANLIST_CONN)) {
        int scandev_conn_result = lv_pro_res_wifi_connect(args->ssid, args->password, args->sec);
        WIFI_DBG("scandev_conn_result = %d\n", scandev_conn_result);
        if (scandev_conn_result) {
            if (!lv_obj_is_valid(htc_msg_box)) {
                htc_msg_box =
                    lv_pro_create_message_box((char *)lv_get_string(STR_WIFI_CONN_FAILED), 3000);
            }
            if (args->btn)
                lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_CONN_SCANDEV_FAIL, args->btn, false);
        } else {
            if (!lv_obj_is_valid(htc_msg_box)) {
                htc_msg_box =
                    lv_pro_create_message_box((char *)lv_get_string(STR_WIFI_CONN_SUCCESS), 1000);
            }
            if (args->btn) lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_HIDDEN_DEV_OBJ, args->btn, false);
        }
    } else if (!strcmp(args->state, WIFI_STATE_PAIRLIST_DISCONN)) {
        int disconn_result = lv_pro_res_wifi_disconnect();
        WIFI_DBG("disconn_result = %d\n", disconn_result);
        if (disconn_result) {
            if (!lv_obj_is_valid(htc_msg_box)) {
                htc_msg_box =
                    lv_pro_create_message_box((char *)lv_get_string(STR_NETWORK_ERR), 3000);
            }
            if (args->btn)
                lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_DISCONN_PAIRDEV_FAIL, args->btn, false);
        } else {
            if (!lv_obj_is_valid(htc_msg_box)) {
                htc_msg_box =
                    lv_pro_create_message_box((char *)lv_get_string(STR_WIFI_DISCONNECT), 1000);
            }
            wifi_sta_auto_reconnect(false);
        }
    } else if (!strcmp(args->state, WIFI_STATE_PAIRLIST_UNPAIR)) {
        int unpair_result = lv_pro_res_wifi_sta_unpair_networks(args->ssid);
        WIFI_DBG("unpair_result = %d\n", unpair_result);
        if (unpair_result) {
            if (!lv_obj_is_valid(htc_msg_box)) {
                htc_msg_box =
                    lv_pro_create_message_box((char *)lv_get_string(STR_NETWORK_ERR), 3000);
            }
            lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_DISCONN_PAIRDEV_FAIL, args->btn, false);
        } else {
            wifi_sta_auto_reconnect(false);
            if (args->btn) lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_HIDDEN_DEV_OBJ, args->btn, false);
            lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_TRY_GRUOP_FOCUS, wifi_paired_list_group, false);
        }
    }

    lv_pro_set_wifi_state(LVGL_WIFI_STATE_IDLE);

    free(args);

    connect_proc_run = false;
    WIFI_DBG("exit\n");
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
    bool is_idle = lv_pro_is_wifi_state_idle();

    if (code == LV_EVENT_KEY) {
        lv_pro_wifi_group_printf_debug(current_group, 1);

        if (key == LV_KEY_ENTER) {
            if (!strcmp(user_data, "Switch")) {
                lv_pro_set_btn_style3_t *btn = (lv_pro_set_btn_style3_t *)obj;
                bool is_checked = lv_obj_has_state(btn->btn, LV_STATE_CHECKED);

                if (is_checked) {
                    if (!wifi_state_on && is_idle) {
                        lv_pro_set_wifi_state(LVGL_WIFI_STATE_OPENING);
                        lv_obj_add_state(btn->btn, LV_STATE_DISABLED);
                        lv_obj_add_state(btn, LV_STATE_DISABLED);
                        lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_WORK_STATE_OPEN, btn, false);
                    } else {
                        if (!is_idle) {
                            if (!lv_obj_is_valid(htc_msg_box)) {
                                htc_msg_box = lv_pro_create_message_box(
                                    (char *)lv_get_string(STR_DONT_OPERATE), 1000);
                            }
                        }
                        lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_WORK_STATE_OPENFAIL, btn, false);
                    }
                } else {  // !is_checked
                    if (wifi_state_on && is_idle) {
                        lv_pro_set_wifi_state(LVGL_WIFI_STATE_CLOSING);
                        lv_obj_add_state(btn->btn, LV_STATE_DISABLED);
                        lv_obj_add_state(btn, LV_STATE_DISABLED);
                        lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_WORK_STATE_CLOSE, btn, false);
                    } else {
                        if (!is_idle) {
                            if (!lv_obj_is_valid(htc_msg_box)) {
                                htc_msg_box = lv_pro_create_message_box(
                                    (char *)lv_get_string(STR_DONT_OPERATE), 1000);
                            }
                        }
                        lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_WORK_STATE_CLOSEFAIL, btn, false);
                    }
                }

            } else if (!strcmp(user_data, "Search")) {
                if (wifi_state_on && is_idle && !scan_proc_run) {
                    lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_CREARE_SCANDEV_PROC, NULL, false);
                } else if (wifi_state_on && !is_idle) {
                    if (!lv_obj_is_valid(htc_msg_box)) {
                        htc_msg_box = lv_pro_create_message_box(
                            (char *)lv_get_string(STR_DONT_OPERATE), 1000);
                    }
                }

            } else if (!strcmp(user_data, "Addnetwork")) {
                if (wifi_state_on && is_idle) {
                    lv_pro_set_wifi_state(LVGL_WIFI_STATE_CONNECTING);
                    lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_MANUALLY_ADD_DEV, NULL, false);
                } else if (wifi_state_on && !is_idle) {
                    if (!lv_obj_is_valid(htc_msg_box)) {
                        htc_msg_box = lv_pro_create_message_box(
                            (char *)lv_get_string(STR_DONT_OPERATE), 1000);
                    }
                }
            } else if (!strcmp(user_data, "BackHome")) {
                if (is_idle) {
                    switch_page(PAGE_HOME);
                    // load_current_channel(launcher_activity, launcher_group);
                } else if (!lv_obj_is_valid(htc_msg_box)) {
                    htc_msg_box =
                        lv_pro_create_message_box((char *)lv_get_string(STR_DONT_OPERATE), 1000);
                }
                return;
            } else if (!strcmp(user_data, "Paired_dev")) {
                lv_pro_set_btn_style6_t *btn = (lv_pro_set_btn_style6_t *)obj;
                if (wifi_state_on && is_idle) {
                    lv_pro_set_wifi_state(LVGL_WIFI_STATE_BUSY);
                    lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_CLICKED_PAIRDEV, btn, false);
                } else if (!is_idle && !lv_obj_is_valid(htc_msg_box)) {
                    htc_msg_box =
                        lv_pro_create_message_box((char *)lv_get_string(STR_DONT_OPERATE), 1000);
                }
            } else if (!strcmp(user_data, "Scan_dev")) {
                lv_pro_set_btn_style6_t *btn = (lv_pro_set_btn_style6_t *)obj;
                if (wifi_state_on && is_idle) {
                    lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_CLICKER_SCANDEV, btn, false);
                    lv_pro_set_wifi_state(LVGL_WIFI_STATE_BUSY);
                } else if (!is_idle) {
                    if (!lv_obj_is_valid(htc_msg_box)) {
                        htc_msg_box = lv_pro_create_message_box(
                            (char *)lv_get_string(STR_DONT_OPERATE), 1000);
                    }
                }
            }
        } else if (!is_global_key_go_new_channel(key) || key == LV_KEY_BACK) {
            switch_page(PAGE_HOME);
            // load_current_channel(launcher_activity, launcher_group);
            return;
        } else if (key == LV_KEY_RIGHT) {
            if (current_group && current_group == wifi_menu_group) {
                if (wifi_scanstate) {
                    // 防止再扫描的时候切到扫描或者配对页面操作，逻辑混乱
                    if (!lv_obj_is_valid(htc_msg_box)) {
                        htc_msg_box = lv_pro_create_message_box(
                            (char *)lv_get_string(STR_WIFI_SCANNING), 1000);
                    }
                } else if (wifi_state_on) {
                    lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_CHANGED_TO_RIGHTGROUP, wifi_menu_group,
                                            false);
                }
            }
        } else if (key == LV_KEY_LEFT) {
            if (current_group && current_group != wifi_menu_group) {
                lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_CHANGED_TO_MENUGROUP, current_group, false);
            }
        } else if (key == LV_KEY_UP) {
            if (current_group)
                lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_LV_KEY_UP, current_group, false);
        } else if (key == LV_KEY_DOWN) {
            if (current_group)
                lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_LV_KEY_DOWN, current_group, false);
        } else if (key == LV_KEY_HOME) {
            return;
        }
        lv_pro_wifi_group_printf_debug(current_group, 0);
    }
}

static void create_wifi_scan_item(int i)
{
    char name_with_id[10];
    snprintf(name_with_id, sizeof(name_with_id), "Name %d", i);
    wifi_scan_item[i] = lv_pro_set_btn_style6_create(wifi_scanlist_obj);
    lv_obj_set_size(wifi_scan_item[i], lv_pct(100), LV_SIZE_CONTENT);
    lv_pro_set_btn_style6_src(wifi_scan_item[i], lv_pro_wifi_get_resource_path(WIFI_SCAN0_LOGO),
                              &GENERAL_FONT_NORMAL, lv_color_white(), name_with_id,
                              &GENERAL_FONT_SMALL, lv_color_hex(0XC0C0C0), "Mac",
                              &GENERAL_FONT_NORMAL, lv_color_white(), "", wifi_scan_list_group);
    lv_obj_add_event_cb(wifi_scan_item[i], key_event_cb, LV_EVENT_KEY, "Scan_dev");
    lv_obj_add_flag(wifi_scan_item[i], LV_OBJ_FLAG_HIDDEN);
}

static void clear_wifi_scan_items()
{
    for (int i = 0; i < SCAN_LIST_LEN; i++) {
        wifi_scan_item[i] = NULL;
    }
}

static void create_wifi_pair_item(int i)
{
    char name_with_id[10];
    snprintf(name_with_id, sizeof(name_with_id), "Name %d", i);
    wifi_pair_item[i] = lv_pro_set_btn_style6_create(wifi_pairlist_obj);
    lv_obj_set_size(wifi_pair_item[i], lv_pct(100), LV_SIZE_CONTENT);
    lv_pro_set_btn_style6_src(wifi_pair_item[i], lv_pro_wifi_get_resource_path(WIFI_SCAN0_LOGO),
                              &GENERAL_FONT_NORMAL, lv_color_white(), name_with_id,
                              &GENERAL_FONT_SMALL, lv_color_hex(0XC0C0C0), "Mac",
                              &GENERAL_FONT_NORMAL, lv_color_white(), lv_get_string(STR_WIFI_SAVED),
                              wifi_paired_list_group);
    lv_obj_add_event_cb(wifi_pair_item[i], key_event_cb, LV_EVENT_KEY, "Paired_dev");
    lv_obj_add_flag(wifi_pair_item[i], LV_OBJ_FLAG_HIDDEN);
}

static void clear_wifi_pair_items()
{
    for (int i = 0; i < SCAN_LIST_LEN; i++) {
        wifi_pair_item[i] = NULL;
    }
}

static void lv_pro_wifi_ui_init(void)
{
    WIFI_DBG("start\n");

    /*******左侧菜单*******/
    wifi_menu_group = lv_group_create();
    lv_group_set_user_focus_check_cb(wifi_menu_group, lv_pro_wifi_group_user_focus_check_cb);

    // 左侧菜单
    wifi_menu_obj = lv_obj_create(lv_pro_wifi_activity);
    lv_obj_set_size(wifi_menu_obj, lv_pct(30), lv_pct(100));
    lv_obj_align(wifi_menu_obj, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_border_width(wifi_menu_obj, 3, LV_PART_MAIN);
    lv_obj_set_style_border_side(wifi_menu_obj, LV_BORDER_SIDE_RIGHT, LV_PART_MAIN);
    lv_obj_set_style_pad_all(wifi_menu_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(wifi_menu_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(wifi_menu_obj, COLOR_BLUE, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(wifi_menu_obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(wifi_menu_obj, LV_OBJ_FLAG_USER_2);

    // 菜单名称
    lv_obj_t *wifi_menu_name = lv_label_create(wifi_menu_obj);
    lv_obj_set_width(wifi_menu_name, lv_pct(95));
    lv_obj_align(wifi_menu_name, LV_ALIGN_TOP_LEFT, 10, lv_pct(20));
    lv_obj_set_style_border_width(wifi_menu_name, 1, LV_PART_MAIN);
    lv_obj_set_style_border_side(wifi_menu_name, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
    lv_obj_set_style_border_color(wifi_menu_name, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(wifi_menu_name, 5, LV_PART_MAIN);
    lv_obj_set_style_pad_left(wifi_menu_name, 10, LV_PART_MAIN);
    lv_obj_set_style_radius(wifi_menu_name, 0, LV_PART_MAIN);
    lv_obj_set_style_text_font(wifi_menu_name, &GENERAL_FONT_BIG, LV_PART_MAIN);
    lv_label_set_recolor(wifi_menu_name, true);
    lv_label_set_text_fmt(wifi_menu_name, "#ffffff %s #", lv_get_string(STR_WIFI_NAME));
    lv_obj_add_flag(wifi_menu_name, LV_OBJ_FLAG_HIDDEN);  // 客户要求不显示

    // WLAN开关
    lv_obj_t *wifi_state_btn = lv_pro_set_btn_style3_create(wifi_menu_obj);
    lv_obj_set_size(wifi_state_btn, lv_pct(95), lv_pct(10));
    lv_obj_align_to(wifi_state_btn, wifi_menu_name, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 30);
    lv_pro_set_btn_style3_src(wifi_state_btn, COLOR_BLUE, lv_color_hex(0xffa523),
                              lv_pro_wifi_get_resource_path(WIFI_OPEN_LOGO),
                              lv_pro_wifi_get_resource_path(WIFI_CLOSE_LOGO), wifi_state_on,
                              wifi_menu_group);
    lv_obj_add_event_cb(wifi_state_btn, key_event_cb, LV_EVENT_KEY, "Switch");
    lv_pro_set_btn_style3_t *_btn3 = (lv_pro_set_btn_style3_t *)wifi_state_btn;
    if (wifi_state_on) {
        wifi_oldState = WIFI_STATE_ON;
        lv_obj_add_state(_btn3->btn, LV_STATE_CHECKED);
    } else {
        wifi_oldState = WIFI_STATE_OFF;
        lv_obj_clear_state(_btn3->btn, LV_STATE_CHECKED);
    }

    // WLAN扫描按钮
    lv_obj_t *wifi_search_btn = lv_pro_set_btn_style4_create(wifi_menu_obj);
    lv_obj_set_size(wifi_search_btn, lv_pct(95), lv_pct(10));
    lv_obj_align_to(wifi_search_btn, wifi_state_btn, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_pro_set_btn_style4_src(wifi_search_btn, lv_pro_wifi_get_resource_path(WIFI_SEARCH_LOGO),
                              &GENERAL_FONT_NORMAL, lv_get_string(STR_SEARCH_WIFI),
                              wifi_menu_group);
    lv_obj_add_event_cb(wifi_search_btn, key_event_cb, LV_EVENT_KEY, "Search");
    lv_pro_set_btn_style4_t *_btn4 = (lv_pro_set_btn_style4_t *)wifi_search_btn;
    if (!wifi_state_on) {
        lv_obj_set_style_text_color(_btn4->name, lv_color_make(0xa3, 0xa3, 0xa3), 0);
    }

    // WLAN手动添加网络按钮
    lv_obj_t *wifi_addnetwork_btn = lv_pro_set_btn_style4_create(wifi_menu_obj);
    lv_obj_set_size(wifi_addnetwork_btn, lv_pct(95), lv_pct(10));
    lv_obj_align_to(wifi_addnetwork_btn, wifi_search_btn, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_pro_set_btn_style4_src(wifi_addnetwork_btn,
                              lv_pro_wifi_get_resource_path(WIFI_ADDNETWORK_LOGO),
                              &GENERAL_FONT_NORMAL, lv_get_string(STR_WIFI_ADD), wifi_menu_group);
    lv_obj_add_event_cb(wifi_addnetwork_btn, key_event_cb, LV_EVENT_KEY, "Addnetwork");
    _btn4 = (lv_pro_set_btn_style4_t *)wifi_addnetwork_btn;
    if (!wifi_state_on) {
        lv_obj_set_style_text_color(_btn4->name, lv_color_make(0xa3, 0xa3, 0xa3), 0);
    }

    // WLAN返回HOME按钮
    lv_obj_t *wifi_backhome_btn = lv_pro_set_btn_style4_create(wifi_menu_obj);
    lv_obj_set_size(wifi_backhome_btn, lv_pct(95), lv_pct(10));
    lv_obj_align_to(wifi_backhome_btn, wifi_addnetwork_btn, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_pro_set_btn_style4_src(wifi_backhome_btn, lv_pro_wifi_get_resource_path(WIFI_BACKHOME_LOGO),
                              &GENERAL_FONT_NORMAL, lv_get_string(STR_HOME_TITLE), wifi_menu_group);
    lv_obj_add_event_cb(wifi_backhome_btn, key_event_cb, LV_EVENT_KEY, "BackHome");

    /*******右侧配对列表*******/
    // 配对列表透明框
    wifi_pair_obj = lv_obj_create(lv_pro_wifi_activity);
    lv_obj_set_size(wifi_pair_obj, lv_pct(68), lv_pct(28));
    lv_obj_align_to(wifi_pair_obj, wifi_menu_obj, LV_ALIGN_OUT_RIGHT_TOP, 10, 10);
    lv_obj_set_style_border_width(wifi_pair_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(wifi_pair_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(wifi_pair_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(wifi_pair_obj, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(wifi_pair_obj, LV_SCROLLBAR_MODE_OFF);

    // 配对标题-配对连接数量
    lv_obj_t *wifi_pairtitle_obj = lv_pro_set_btn_style5_create(wifi_pair_obj);
    lv_obj_set_width(wifi_pairtitle_obj, lv_pct(100));
    lv_obj_align(wifi_pairtitle_obj, LV_ALIGN_TOP_LEFT, 0, lv_pct(10));
    lv_pro_set_btn_style5_src(wifi_pairtitle_obj, &GENERAL_FONT_BIG, lv_color_white(),
                              lv_get_string(STR_WIFI_MY_DEV), &GENERAL_FONT_BIG, lv_color_white(),
                              "0 / 0", NULL);
    wifi_pairnum_obj = lv_pro_set_btn_style5_get_right_content_obj(wifi_pairtitle_obj);

    // 已配对列表 透明框
    wifi_pairlist_obj = lv_obj_create(wifi_pair_obj);
    lv_obj_set_size(wifi_pairlist_obj, lv_pct(100), lv_pct(78));
    lv_obj_align_to(wifi_pairlist_obj, wifi_pairtitle_obj, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_obj_set_style_border_width(wifi_pairlist_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(wifi_pairlist_obj, 20, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(wifi_pairlist_obj, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_add_flag(wifi_pairlist_obj, LV_OBJ_FLAG_USER_2);  // user2 为纵向
    lv_obj_set_flex_flow(wifi_pairlist_obj, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(wifi_pairlist_obj, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END,
                          LV_FLEX_ALIGN_START);

    wifi_paired_list_group = lv_group_create();
    lv_group_set_user_focus_check_cb(wifi_paired_list_group, lv_pro_wifi_group_user_focus_check_cb);
#if 0
    for (int i = 0; i < SCAN_LIST_LEN; i++) {
        char name_with_id[10];
        snprintf(name_with_id, sizeof(name_with_id), "Name %d", i);
        wifi_pair_item[i] = lv_pro_set_btn_style6_create(wifi_pairlist_obj);
        lv_obj_set_size(wifi_pair_item[i], lv_pct(100), LV_SIZE_CONTENT);
        lv_pro_set_btn_style6_src(wifi_pair_item[i], lv_pro_wifi_get_resource_path(WIFI_SCAN0_LOGO),
                                  &GENERAL_FONT_NORMAL, lv_color_white(), name_with_id,
                                  &GENERAL_FONT_SMALL, lv_color_hex(0XC0C0C0), "Mac",
                                  &GENERAL_FONT_NORMAL, lv_color_white(),
                                  lv_get_string(STR_WIFI_SAVED), wifi_paired_list_group);
        lv_obj_add_event_cb(wifi_pair_item[i], key_event_cb, LV_EVENT_KEY, "Paired_dev");
        lv_obj_add_flag(wifi_pair_item[i], LV_OBJ_FLAG_HIDDEN);
    }
#endif
    lv_pro_wifi_group_obj_clear_focus(wifi_paired_list_group);
    if (!wifi_state_on) {
        lv_obj_add_flag(wifi_pair_obj, LV_OBJ_FLAG_HIDDEN);
    }

    /*******右侧扫描列表*******/
    // 右侧扫描框
    wifi_scan_obj = lv_obj_create(lv_pro_wifi_activity);
    lv_obj_set_size(wifi_scan_obj, lv_pct(68), lv_pct(67));
    lv_obj_align_to(wifi_scan_obj, wifi_pair_obj, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
    lv_obj_set_style_border_width(wifi_scan_obj, 3, LV_PART_MAIN);
    lv_obj_set_style_border_side(wifi_scan_obj, LV_BORDER_SIDE_TOP, LV_PART_MAIN);
    lv_obj_set_style_pad_all(wifi_scan_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(wifi_scan_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(wifi_scan_obj, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(wifi_scan_obj, LV_SCROLLBAR_MODE_OFF);

    // 扫描标题-扫描状态
    lv_obj_t *wifi_scantitle_obj = lv_pro_set_btn_style5_create(wifi_scan_obj);
    lv_obj_set_width(wifi_scantitle_obj, lv_pct(100));
    lv_obj_align(wifi_scantitle_obj, LV_ALIGN_TOP_LEFT, 0, 15);
    lv_pro_set_btn_style5_src(wifi_scantitle_obj, &GENERAL_FONT_BIG, lv_color_white(),
                              lv_get_string(STR_WIFI_OTHER_DEV), &GENERAL_FONT_BIG,
                              lv_color_white(), lv_get_string(STR_WIFI_SEARCHING), NULL);
    wifi_scanstate_obj = lv_pro_set_btn_style5_get_right_content_obj(wifi_scantitle_obj);
    lv_obj_add_flag(wifi_scanstate_obj, LV_OBJ_FLAG_HIDDEN);

    wifi_scannum_obj = lv_label_create(wifi_scantitle_obj);
    lv_obj_set_style_text_font(wifi_scannum_obj, &GENERAL_FONT_BIG, LV_PART_MAIN);
    lv_label_set_recolor(wifi_scannum_obj, true);
    lv_obj_set_style_text_color(wifi_scannum_obj, lv_color_white(), LV_PART_MAIN);
    lv_label_set_text(wifi_scannum_obj, "0 / 0");
    lv_obj_align_to(wifi_scannum_obj, wifi_scanstate_obj, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(wifi_scannum_obj, LV_OBJ_FLAG_HIDDEN);

    // 扫描列表 透明框
    wifi_scanlist_obj = lv_obj_create(wifi_scan_obj);
    lv_obj_set_size(wifi_scanlist_obj, lv_pct(100), lv_pct(78));
    lv_obj_align_to(wifi_scanlist_obj, wifi_scantitle_obj, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_obj_set_style_border_width(wifi_scanlist_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(wifi_scanlist_obj, 20, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(wifi_scanlist_obj, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_flex_flow(wifi_scanlist_obj, LV_FLEX_FLOW_COLUMN);
    lv_obj_add_flag(wifi_scanlist_obj, LV_OBJ_FLAG_USER_2);  // user2 为纵向
    lv_obj_set_flex_align(wifi_scanlist_obj, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END,
                          LV_FLEX_ALIGN_START);

    wifi_scan_list_group = lv_group_create();
    lv_group_set_user_focus_check_cb(wifi_scan_list_group, lv_pro_wifi_group_user_focus_check_cb);
#if 0
    for (int i = 0; i < SCAN_LIST_LEN; i++) {
        char name_with_id[10];
        snprintf(name_with_id, sizeof(name_with_id), "Name %d", i);
        wifi_scan_item[i] = lv_pro_set_btn_style6_create(wifi_scanlist_obj);
        lv_obj_set_size(wifi_scan_item[i], lv_pct(100), LV_SIZE_CONTENT);
        lv_pro_set_btn_style6_src(wifi_scan_item[i], lv_pro_wifi_get_resource_path(WIFI_SCAN0_LOGO),
                                  &GENERAL_FONT_NORMAL, lv_color_white(), name_with_id,
                                  &GENERAL_FONT_SMALL, lv_color_hex(0XC0C0C0), "Mac",
                                  &GENERAL_FONT_NORMAL, lv_color_white(), "", wifi_scan_list_group);
        lv_obj_add_event_cb(wifi_scan_item[i], key_event_cb, LV_EVENT_KEY, "Scan_dev");
        lv_obj_add_flag(wifi_scan_item[i], LV_OBJ_FLAG_HIDDEN);
    }
#endif
    lv_pro_wifi_group_obj_clear_focus(wifi_scan_list_group);

    if (!wifi_state_on) {
        lv_obj_add_flag(wifi_scan_obj, LV_OBJ_FLAG_HIDDEN);
    }

    current_group = wifi_menu_group;
    lv_obj_update_layout(lv_pro_wifi_activity);

    WIFI_DBG("finish\n");
}

static void lv_pro_wifi_ui_deinit(void)
{
    WIFI_DBG("start\n");

    if (wifi_scan_list_group) {
        lv_group_remove_all_objs(wifi_scan_list_group);
        lv_group_del(wifi_scan_list_group);
        wifi_scan_list_group = NULL;
    }
    if (wifi_scannum_obj) {
        lv_obj_del(wifi_scannum_obj);
        wifi_scannum_obj = NULL;
    }
    if (wifi_scanstate_obj) {
        lv_obj_del(wifi_scanstate_obj);
        wifi_scanstate_obj = NULL;
    }
    if (wifi_scan_obj) {
        lv_obj_del(wifi_scan_obj);
        wifi_scan_obj = NULL;
    }
    if (wifi_paired_list_group) {
        lv_group_remove_all_objs(wifi_paired_list_group);
        lv_group_del(wifi_paired_list_group);
        wifi_paired_list_group = NULL;
    }
    if (wifi_pairnum_obj) {
        lv_obj_del(wifi_pairnum_obj);
        wifi_pairnum_obj = NULL;
    }
    if (wifi_pair_obj) {
        lv_obj_del(wifi_pair_obj);
        wifi_pair_obj = NULL;
    }
    if (wifi_menu_group) {
        lv_group_remove_all_objs(wifi_menu_group);
        lv_group_del(wifi_menu_group);
        wifi_menu_group = NULL;
    }
    if (wifi_menu_obj) {
        lv_obj_del(wifi_menu_obj);
        wifi_menu_obj = NULL;
    }
    if (lv_pro_wifi_activity) {
        lv_obj_del_async(lv_pro_wifi_activity);
        lv_pro_wifi_activity = NULL;
    }

    clear_wifi_scan_items();
    clear_wifi_pair_items();

    current_group = NULL;
    wifi_lvgl_active = false;
    wifi_lvgl_exiting = false;

    WIFI_DBG("finish\n");
}

LV_IMG_DECLARE(IDB_Icon_unsupported);

int lv_pro_wifi_init(void)
{
    if (lv_pro_wifi_activity || wifi_lvgl_active || wifi_lvgl_exiting) {
        WIFI_ERR("wifi activity already init!\n");
        return -1;
    }

    if (autoon_proc_run) {
        lv_pro_msgbox_msg_open_on_top(&IDB_Icon_unsupported, lv_get_string(STR_WIFI_READY), 2000);
        return -1;
    }

    if (!wifi_modules_loaded) {
        lv_pro_msgbox_msg_open_on_top(&IDB_Icon_unsupported,
                                      lv_get_string(STR_WIFI_DEV_DETECT_FAIL), 2000);
        return -1;
    }

    // wifi背景图
    lv_pro_wifi_activity = lv_obj_create(NULL);
    lv_obj_set_size(lv_pro_wifi_activity, lv_pct(100), lv_pct(100));
    lv_obj_set_style_border_width(lv_pro_wifi_activity, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(lv_pro_wifi_activity, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(lv_pro_wifi_activity, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(lv_pro_wifi_activity, COLOR_BLUE, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(lv_pro_wifi_activity, LV_SCROLLBAR_MODE_OFF);

    lv_pro_wifi_ui_init();

    return 0;
}

bool lv_pro_network_state(void) { return wifi_state_on; }

void lv_pro_refresh_wifi_state(bool flag) { wifi_reseton = flag; }

int lv_pro_wifi_lanucher_auto_on(void)
{
    pthread_t wifi_thread;
    if (pthread_create(&wifi_thread, NULL, wifi_sta_mode_init_proc, NULL) != 0) {
        WIFI_ERR("Failed to create wifi thread\n");
        return -1;
    }
    pthread_detach(wifi_thread);

    return 0;
}

static void *wifi_sta_mode_init_proc(void *arg)
{
    int ret;
    autoon_proc_run = true;

    while (wireless_autoon_proc_run) {
        usleep(10 * 1000);
    }

    if (wireless_modules_loaded) {
        wifi_modules_loaded = true;
    } else {
        wifi_modules_loaded = false;
        goto AUTOON_END;
    }

    // Check WiFi state
    if ((ret = get_file_wifi_state()) != 1) {
        goto AUTOON_END;
    }

    // Initialize WiFi
    if ((ret = lv_pro_res_wifi_init()) != 0) {
        WIFI_ERR("wifi init err!\n");
        goto AUTOON_END;
    }

    // Enable STA mode
    if ((ret = lv_pro_res_wifi_on()) != 0) {
        WIFI_ERR("sta mode init err!\n");
        lv_pro_res_wifi_deinit();
        goto AUTOON_END;
    }

    lv_pro_msgbox_msg_clear();
    wifi_state_on = true;

AUTOON_END:
    autoon_proc_run = false;
    pthread_exit((void *)1);
    return NULL;
}

static int create_lvgl_wifi(void)
{
    WIFI_DBG("start\n");

    lv_obj_set_parent(lv_pro_wifi_activity, launcher_activity);
    load_current_channel(lv_pro_wifi_activity, wifi_menu_group);

    wifi_lvgl_active = true;

    if (wifi_state_on) {
        if (!pairstate_proc_run) {
            lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_CREARE_PAIRSTATE_PROC, NULL, false);
        }
// 快速响应页面，忽略线程执行状态
#if 0
            if (!scan_proc_run) {
                lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_CREARE_SCANDEV_PROC, NULL, false);
            }
#else
        lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_CREARE_SCANDEV_PROC, NULL, false);
#endif
    }
    WIFI_DBG(" finish\n");
    return 0;
}

static int destory_lvgl_wifi(void)
{
    WIFI_DBG("start\n");
    wifi_lvgl_exiting = true;
    if (wifi_state_on) {
        lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_STOP_SCANPROC, NULL, false);
        lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_STOP_PAIRPROC, NULL, false);
    }

    // 适配快速退出，忽略等待pair scan线程
#if 0
    wait_for_procs_to_finish();
#else
    if (pairstate_proc_run && pairstate_proclock) lv_pro_wifi_freshui_ack();
    wait_control_proc_stop();
#endif

    if (show_msgbox) {
        lv_pro_del_message2_box();
        show_msgbox = false;
    }

    lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_LVGL_UI_DEINIT, NULL, false);
    WIFI_DBG("finish\n");
    return 0;
}

static int show_lvgl_wifi(void)
{
    WIFI_DBG("\n");
    return 0;
}

static int hide_lvgl_wifi(void)
{
    WIFI_DBG("\n");
    return 0;
}

static page_interface_t page_lvgl_wifi = {.ops =
                                              {
                                                  create_lvgl_wifi,
                                                  destory_lvgl_wifi,
                                                  show_lvgl_wifi,
                                                  hide_lvgl_wifi,
                                              },
                                          .info = {.id = PAGE_WIFI, .user_data = NULL}};

void REGISTER_PAGE_WIFI(void) { reg_page(&page_lvgl_wifi); }

#else
bool lv_pro_network_state(void) { return false; }

#endif /* ENABLE_WIFI */
