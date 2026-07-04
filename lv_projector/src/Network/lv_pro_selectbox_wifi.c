/*
 * lv_pro_selectbox_wifi.c
 *
 *  Created on: 2024/10/17
 *      Author: JASON
 */

#include "lv_pro_selectbox_wifi.h"

#include "lv_pro_res_wifi.h"
#include "lv_pro_wifi_activity.h"

#if ENABLE_WIFI

static lv_pro_set_btn_style6_t *select_btn;

static lv_group_t *wifi_activity_g;
static lv_group_t *wifi_selectbox_default_g;
static lv_group_t *kb_g;

static lv_obj_t *lv_pro_wifi_selectbox_activity;
static lv_obj_t *wifi_selectbox_obj;
static lv_obj_t *kb;
static lv_obj_t *ta;

static char select_ssid[64] = {0};
static char select_geteway[18] = {0};
static char select_bssid[18] = {0};
static char select_mgmt[32] = {0};
static char select_pwd[64] = {0};
static char select_dns[40] = {0};
static char select_devip[18] = {0};
static int select_rssi = 0;
static int select_sec = 0;
static bool select_dev_nonesec = false;

static bool pwd_is_hidden = false;
static bool keypad_exit = false;

static void selectbox_wifi_ui_quit(void);
static void lv_pro_selectbox_wifi_deinit(void);
static void kb_delete_and_cutgroup(void);
static void kb_g_onoff(bool en);
static void handle_virtual_keyboard_event(lv_event_t *e);
static void create_repassword_ui(void);
static void create_request_new_password_ui(void);

static int group_user_focus_check_cb(lv_group_t *group) { return 1; }

static void group_user_focus_edge_cb(lv_group_t *group, bool next)
{
    if (kb_g == group && !next && keypad_exit == true) {
        kb_delete_and_cutgroup();
        keypad_exit = false;
    }
    return;
}

static void initialize_selected_device_parameters()
{
    select_sec = 0;
    select_rssi = 0;
    select_dev_nonesec = false;
    memset(select_ssid, '\0', sizeof(select_ssid));
    memset(select_bssid, '\0', sizeof(select_bssid));
    memset(select_mgmt, '\0', sizeof(select_mgmt));
    memset(select_pwd, '\0', sizeof(select_pwd));
    memset(select_dns, '\0', sizeof(select_dns));
    memset(select_devip, '\0', sizeof(select_devip));

    if (select_btn) {
        select_sec = lv_pro_set_btn_style6_get_sec_value(select_btn);
        select_rssi = lv_pro_set_btn_style6_get_rssi_value(select_btn);
        memcpy(select_ssid, lv_pro_set_btn_style6_get_name_str(select_btn), sizeof(select_ssid));
        memcpy(select_bssid, lv_pro_set_btn_style6_get_mac_addr_str(select_btn),
               sizeof(select_bssid));
        memcpy(select_mgmt, lv_pro_set_btn_style6_get_state_str(select_btn), sizeof(select_mgmt));
    }
}

static wifi_thread_args_btn6_t *wifi_thread_args_btn6_create()
{
    wifi_thread_args_btn6_t *device = malloc(sizeof(wifi_thread_args_btn6_t));
    if (device == NULL) {
        WIFI_ERR("device malloc failed!\n");
        return NULL;
    }

    device->btn = select_btn;
    device->sec = select_sec;
    snprintf(device->ssid, sizeof(device->ssid), "%s", select_ssid);
    snprintf(device->bssid, sizeof(device->bssid), "%s", select_bssid);
    snprintf(device->mgmt, sizeof(device->mgmt), "%s", select_mgmt);
    snprintf(device->password, sizeof(device->password), "%s", select_pwd);

    return device;
}

static void kb_delete_and_cutgroup()
{
    set_current_ui_group(wifi_selectbox_default_g);

    kb_g_onoff(false);

    if (kb) {
        lv_obj_del(kb);
        kb = NULL;
    }

    lv_obj_center(wifi_selectbox_obj);

    int child_count = lv_obj_get_child_cnt(wifi_selectbox_obj);
    if (child_count > 0) {
        lv_obj_t *last_child = lv_obj_get_child(wifi_selectbox_obj, child_count - 1);
        lv_group_focus_obj(last_child);
    }
}

static void kb_g_onoff(bool en)
{
    if (en) {
        if (!kb_g) {
            kb_g = lv_group_create();
            lv_group_set_user_focus_check_cb(kb_g, group_user_focus_check_cb);
            lv_group_set_edge_cb(kb_g, group_user_focus_edge_cb);
        }
        set_current_ui_group(kb_g);
    } else {
        if (kb_g) {
            lv_group_remove_all_objs(kb_g);
            lv_group_del(kb_g);
            kb_g = NULL;
        }
    }
}

static void create_virtual_keyboard(lv_obj_t *_ta)
{
    kb_g_onoff(true);
    kb = lv_keyboard_create(lv_pro_wifi_selectbox_activity);
    lv_obj_set_size(kb, lv_pct(100), lv_pct(WIFI_CONN_KEYPAD_HEIGHT));
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(kb, lv_color_white(), LV_PART_MAIN);

    lv_keyboard_t *keyb = (lv_keyboard_t *)kb;
    lv_obj_t *btns = (lv_obj_t *)(&keyb->btnm);
    lv_obj_set_style_bg_opa(btns, LV_OPA_10, LV_PART_ITEMS);
    // 设置默认状态样式（白底黑字）
    lv_obj_set_style_bg_color(btns, lv_color_hex(0xFFFFFF), LV_PART_ITEMS);
    lv_obj_set_style_text_color(btns, lv_color_hex(0x000000), LV_PART_ITEMS);

    // 设置功能按钮状态样式（蓝底白字）
    lv_obj_set_style_bg_color(btns, lv_color_hex(0x66b9ff), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_text_color(btns, lv_color_hex(0xFFFFFF), LV_PART_ITEMS | LV_STATE_CHECKED);

    // 设置聚焦状态样式（绿底黑字）
    lv_obj_set_style_bg_color(btns, lv_color_hex(0x28fa00), LV_PART_ITEMS | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_text_color(btns, lv_color_hex(0x000000), LV_PART_ITEMS | LV_STATE_FOCUS_KEY);

    // 设置按下状态样式（黄底白字）
    lv_obj_set_style_bg_color(btns, lv_color_hex(0xeea87c), LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_text_color(btns, lv_color_white(), LV_PART_ITEMS | LV_STATE_PRESSED);

    lv_keyboard_set_textarea(kb, _ta);
    lv_obj_add_event_cb(kb, handle_virtual_keyboard_event, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(kb, handle_virtual_keyboard_event, LV_EVENT_CLICKED, NULL);
}

static void handle_manual_wifi_connection()
{
    static lv_obj_t *htc_msg_box = NULL;
    const char *ssid;
    const char *pwd;

    ssid = lv_textarea_get_text(lv_obj_get_child(wifi_selectbox_obj, 1));

    if (strlen(ssid) == 0) {
        lv_pro_create_message_box(lv_get_string(STR_WIFI_SSID_NULL), 1000);
        return;
    }
    strncpy(select_ssid, ssid, strlen(ssid));

    if (!select_dev_nonesec) {
        pwd = lv_textarea_get_text(lv_obj_get_child(wifi_selectbox_obj, 2));

        if (strlen(pwd) == 0) {
            if (!lv_obj_is_valid(htc_msg_box)) {
                htc_msg_box =
                    lv_pro_create_message_box((char *)lv_get_string(STR_WIFI_PWD_NULL), 1000);
            }
            return;
        } else if (strlen(pwd) < 8 || strlen(pwd) > 63) {
            if (!lv_obj_is_valid(htc_msg_box)) {
                htc_msg_box =
                    lv_pro_create_message_box((char *)lv_get_string(STR_ENTER_WORD), 1000);
            }
            return;
        }
        strncpy(select_pwd, pwd, strlen(pwd));
    }

    int drop_sec =
        (int)lv_dropdown_get_selected(lv_obj_get_child(lv_obj_get_child(wifi_selectbox_obj, 3), 0));
    if (drop_sec == 0) {
        select_sec = WIFI_SEC_NONE;
    } else if (drop_sec == 1) {
        select_sec = WIFI_SEC_WEP;
    } else if (drop_sec == 2) {
        select_sec = WIFI_SEC_WPA2_PSK;
    } else if (drop_sec == 3) {
        select_sec = WIFI_SEC_WPA3_PSK;
    }

    memset(select_mgmt, '\0', sizeof(select_mgmt));
    key_mgmt_to_char(select_mgmt, select_sec);

    selectbox_wifi_ui_quit();

    if (!lv_obj_is_valid(htc_msg_box)) {
        htc_msg_box = lv_pro_create_message_box((char *)lv_get_string(STR_WIFI_CONNECTING), 1000);
    }

    wifi_thread_args_btn6_t *device = wifi_thread_args_btn6_create();
    lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_NEWCONN_PAIRDEV, device, true);

    return true;
}

static void connect_to_wifi()
{
    static lv_obj_t *htc_msg_box = NULL;
    const char *pwd =
        lv_textarea_get_text(lv_obj_get_child(lv_obj_get_child(wifi_selectbox_obj, 0), 1));

    if (strlen(pwd) == 0) {  // ta
        if (!lv_obj_is_valid(htc_msg_box)) {
            htc_msg_box = lv_pro_create_message_box((char *)lv_get_string(STR_WIFI_PWD_NULL), 1000);
        }
        return;
    } else if (strlen(pwd) < 8 || strlen(pwd) > 63) {
        if (!lv_obj_is_valid(htc_msg_box)) {
            htc_msg_box = lv_pro_create_message_box((char *)lv_get_string(STR_ENTER_WORD), 1000);
        }
        return;
    }

    strncpy(select_pwd, pwd, strlen(pwd));

    selectbox_wifi_ui_quit();

    if (!lv_obj_is_valid(htc_msg_box)) {
        htc_msg_box = lv_pro_create_message_box((char *)lv_get_string(STR_WIFI_CONNECTING), 1000);
    }

    wifi_thread_args_btn6_t *device = wifi_thread_args_btn6_create();
    if (device) lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_NEWCONN_PAIRDEV, device, true);

    return true;
}

static void handle_virtual_keyboard_event(lv_event_t *e)
{
    static int sel_id = 0, abc_btn_flag = 0;
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    const uint32_t *key_param = lv_event_get_param(e);
    uint32_t key = (key_param != NULL) ? *key_param : 0;
    const char *txt = lv_btnmatrix_get_btn_text(obj, lv_btnmatrix_get_selected_btn(obj));

    if (code == LV_EVENT_KEY) {
        if (key == LV_KEY_BACK) {
            kb_delete_and_cutgroup();
            return;
        } else if (!is_global_key_go_new_channel(key)) {
            selectbox_wifi_ui_quit();
            switch_page(PAGE_HOME);
            return;
        } else if (key == LV_KEY_ENTER) {
            if (strcmp(txt, LV_SYMBOL_OK) == 0) {
                // connect_to_wifi();
                kb_delete_and_cutgroup();
                return;
            } else if (strcmp(txt, "abc") == 0) {
                abc_btn_flag = 1;
            }
        } else if (key == LV_KEY_UP) {
            lv_btnmatrix_t *btnm = &((lv_keyboard_t *)obj)->btnm;
            if (btnm->button_areas[sel_id].y1 == btnm->button_areas[0].y1) {
                keypad_exit = true;  // 由于up事件特殊，需要等到edge_cb里面退出
                return;
            }
        }
    } else if (code == LV_EVENT_CLICKED) {
        if ((lv_btnmatrix_get_selected_btn(obj) == 11) && abc_btn_flag) {
            lv_btnmatrix_set_selected_btn(obj, 12);  // ABC
            lv_btnmatrix_set_btn_ctrl(obj, 12, LV_BTNMATRIX_CTRL_CHECKED);
        }
        abc_btn_flag = 0;
    }

    sel_id = ((lv_keyboard_t *)obj)->btnm.btn_id_sel;
}

static void handle_manual_network_password_event(lv_event_t *e)
{
    static lv_obj_t *htc_msg_box = NULL;
    lv_event_code_t code = lv_event_get_code(e);
    const uint32_t *key_param = lv_event_get_param(e);
    uint32_t key = (key_param != NULL) ? *key_param : 0;
    lv_obj_t *obj = lv_event_get_target(e);
    lv_btnmatrix_t *btnms = (lv_btnmatrix_t *)obj;
    char *user_data = (char *)lv_event_get_user_data(e);
    int btn_id = lv_btnmatrix_get_selected_btn(btnms);

    if (code == LV_EVENT_KEY) {
        if (key == LV_KEY_UP) {
            if (!strcmp(user_data, "reEnterPwd")) {
                lv_group_focus_prev(lv_group_get_default());
            } else if (!strcmp(user_data, "addNW")) {
                lv_group_focus_obj(lv_obj_get_child(obj->parent, 3));
            }
        }
        if (key == LV_KEY_DOWN) {
            lv_group_focus_obj(obj);
        } else if (key == LV_KEY_LEFT) {
            lv_btnmatrix_set_selected_btn(obj, 0);
            lv_btnmatrix_clear_btn_ctrl_all(obj, LV_BTNMATRIX_CTRL_CHECKED);
            lv_btnmatrix_set_btn_ctrl(obj, 0, LV_BTNMATRIX_CTRL_CHECKED);
        } else if (key == LV_KEY_RIGHT) {
            lv_btnmatrix_set_selected_btn(obj, 1);
            lv_btnmatrix_clear_btn_ctrl_all(obj, LV_BTNMATRIX_CTRL_CHECKED);
            lv_btnmatrix_set_btn_ctrl(obj, 1, LV_BTNMATRIX_CTRL_CHECKED);
        } else if (!is_global_key_go_new_channel(key) || key == LV_KEY_BACK) {
            selectbox_wifi_ui_quit();
            if (key != LV_KEY_BACK) switch_page(PAGE_HOME);
        } else if (key == LV_KEY_ENTER) {
            if (btn_id == 1) {
                if (!strcmp(user_data, "reEnterPwd")) {
                    connect_to_wifi();
                    return;
                } else if (!strcmp(user_data, "addNW")) {
                    handle_manual_wifi_connection();
                    return;
                }
            } else {
                selectbox_wifi_ui_quit();
                return;
            }
        }
    } else if (code == LV_EVENT_FOCUSED) {
        lv_btnmatrix_clear_btn_ctrl_all(obj, LV_BTNMATRIX_CTRL_CHECKED);
        if (btnms->btn_id_sel == 1) {
            lv_btnmatrix_set_btn_ctrl(obj, 1, LV_BTNMATRIX_CTRL_CHECKED);
        } else {
            lv_btnmatrix_set_selected_btn(obj, 0);
            lv_btnmatrix_set_btn_ctrl(obj, 0, LV_BTNMATRIX_CTRL_CHECKED);
        }
    } else if (code == LV_EVENT_DEFOCUSED) {
        lv_btnmatrix_clear_btn_ctrl_all(obj, LV_BTNMATRIX_CTRL_CHECKED);
    }
}

static void network_reconnect_pwd_event_handle(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    const uint32_t *key_param = lv_event_get_param(e);
    uint32_t key = (key_param != NULL) ? *key_param : 0;

    if (code == LV_EVENT_KEY) {
        if (key == LV_KEY_UP) {
            lv_group_focus_obj(obj);
        } else if (key == LV_KEY_DOWN) {
            lv_group_focus_obj(lv_obj_get_child(obj->parent->parent, 1));
        } else if (!is_global_key_go_new_channel(key) || key == LV_KEY_BACK) {
            selectbox_wifi_ui_quit();
            if (key != LV_KEY_BACK) switch_page(PAGE_HOME);
            return;
        } else if (key == LV_KEY_ENTER) {
            if (kb != NULL) {
                kb_g_onoff(true);
                lv_group_focus_obj(obj);
            } else {
                lv_obj_align(wifi_selectbox_obj, LV_ALIGN_TOP_MID, 0, lv_pct(10));
                create_virtual_keyboard(obj);
                lv_group_focus_obj(obj);
            }
        }
    }
}

static void password_toggle_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    lv_obj_t *ta = (lv_obj_t *)lv_event_get_user_data(e);
    const uint32_t *key_param = lv_event_get_param(e);
    uint32_t key = (key_param != NULL) ? *key_param : 0;

    if (code == LV_EVENT_KEY) {
        if (key == LV_KEY_UP) {
            lv_group_focus_prev(lv_group_get_default());
            if (pwd_is_hidden) lv_obj_clear_state(obj, LV_STATE_CHECKED);
        } else if (key == LV_KEY_DOWN) {
            lv_group_focus_next(lv_group_get_default());
            if (!pwd_is_hidden) lv_obj_add_state(obj, LV_STATE_CHECKED);
        } else if (!is_global_key_go_new_channel(key) || key == LV_KEY_BACK) {
            selectbox_wifi_ui_quit();
            if (key != LV_KEY_BACK) switch_page(PAGE_HOME);
            return;
        } else if (key == LV_KEY_ENTER) {
            if (pwd_is_hidden) {
                lv_textarea_set_password_mode(ta, false);
                pwd_is_hidden = false;
            } else {
                lv_textarea_set_password_mode(ta, true);
                pwd_is_hidden = true;
            }
        }
    }
}

static void connection_buttons_event_handler(lv_event_t *e)
{
    static lv_obj_t *htc_msg_box = NULL;
    lv_event_code_t code = lv_event_get_code(e);
    const uint32_t *key_param = lv_event_get_param(e);
    uint32_t key = (key_param != NULL) ? *key_param : 0;
    lv_obj_t *obj = lv_event_get_target(e);
    lv_btnmatrix_t *btnms = (lv_btnmatrix_t *)obj;
    char *user_data = (char *)lv_event_get_user_data(e);
    int btn_id = lv_btnmatrix_get_selected_btn(btnms);

    if (code == LV_EVENT_KEY) {
        if (key == LV_KEY_LEFT) {
            lv_btnmatrix_set_selected_btn(obj, 0);
            lv_btnmatrix_clear_btn_ctrl_all(obj, LV_BTNMATRIX_CTRL_CHECKED);
            lv_btnmatrix_set_btn_ctrl(obj, 0, LV_BTNMATRIX_CTRL_CHECKED);
        } else if (key == LV_KEY_RIGHT) {
            lv_btnmatrix_set_selected_btn(obj, 1);
            lv_btnmatrix_clear_btn_ctrl_all(obj, LV_BTNMATRIX_CTRL_CHECKED);
            lv_btnmatrix_set_btn_ctrl(obj, 1, LV_BTNMATRIX_CTRL_CHECKED);
        } else if (!is_global_key_go_new_channel(key) || key == LV_KEY_BACK) {
            selectbox_wifi_ui_quit();
            if (key != LV_KEY_BACK) switch_page(PAGE_HOME);
            return;
        } else if (key == LV_KEY_ENTER) {
            if (!strcmp(user_data, "AskConn")) {
                selectbox_wifi_ui_quit();
                if (btn_id) {
                    create_request_new_password_ui();
                } else {
                    lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_DELETED_PAIRDEV, select_btn, false);
                }
                return;
            } else if (!strcmp(user_data, "setPWD")) {
                selectbox_wifi_ui_quit();
                if (btn_id) {
                    create_repassword_ui();
                } else {
                    lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_CONN_PAIRDEV, select_btn, false);
                }
                return;
            }
        }
    }
}

static void handle_disconnect_button_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    bool is_delete = (bool)lv_event_get_user_data(e);
    lv_obj_t *obj = lv_event_get_target(e);
    const uint32_t *key_param = lv_event_get_param(e);
    uint32_t key = (key_param != NULL) ? *key_param : 0;

    if (code == LV_EVENT_KEY) {
        if (key == LV_KEY_ENTER) {
            selectbox_wifi_ui_quit();
            lv_pro_wifi_msg_enqueue(
                is_delete ? MSG_TYPE_WIFI_DELETED_PAIRDEV : MSG_TYPE_WIFI_DISCONN_PAIRDEV,
                select_btn, false);
            return;
        } else if (!is_global_key_go_new_channel(key) || key == LV_KEY_BACK) {
            selectbox_wifi_ui_quit();
            if (key != LV_KEY_BACK) switch_page(PAGE_HOME);
            return;
        } else if (key == LV_KEY_LEFT) {
            lv_group_focus_obj(lv_obj_get_child(obj->parent, 0));
        } else if (key == LV_KEY_RIGHT) {
            lv_group_focus_obj(lv_obj_get_child(obj->parent, 1));
        }
    }
}

static void wifi_add_hidden_net_name_event_handle(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    const uint32_t *key_param = lv_event_get_param(e);
    uint32_t key = (key_param != NULL) ? *key_param : 0;

    if (code == LV_EVENT_KEY) {
        if (key == LV_KEY_DOWN) {
            lv_group_focus_obj(lv_obj_get_child(obj->parent, 2));
        } else if (!is_global_key_go_new_channel(key) || key == LV_KEY_BACK) {
            selectbox_wifi_ui_quit();
            if (key != LV_KEY_BACK) switch_page(PAGE_HOME);
            return;
        } else if (key == LV_KEY_ENTER) {
            if (kb != NULL) {
                kb_g_onoff(true);
                lv_group_focus_obj(obj);
            } else {
                lv_obj_align(wifi_selectbox_obj, LV_ALIGN_TOP_MID, 0, lv_pct(10));
                create_virtual_keyboard(obj);
                lv_group_focus_obj(obj);
            }
        }
    }
}

static void wifi_add_hidden_net_pwd_event_handle(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    const uint32_t *key_param = lv_event_get_param(e);
    uint32_t key = (key_param != NULL) ? *key_param : 0;

    if (code == LV_EVENT_KEY) {
        if (key == LV_KEY_DOWN) {
            lv_group_focus_obj(lv_obj_get_child(obj->parent, 3));
        } else if (key == LV_KEY_UP) {
            lv_group_focus_obj(lv_obj_get_child(obj->parent, 1));
        } else if (!is_global_key_go_new_channel(key) || key == LV_KEY_BACK) {
            selectbox_wifi_ui_quit();
            if (key != LV_KEY_BACK) switch_page(PAGE_HOME);
            return;
        } else if (key == LV_KEY_ENTER) {
            if (kb != NULL) {
                kb_g_onoff(true);
                lv_group_focus_obj(obj);
            } else {
                lv_obj_align(wifi_selectbox_obj, LV_ALIGN_TOP_MID, 0, lv_pct(10));
                create_virtual_keyboard(obj);
                lv_group_focus_obj(obj);
            }
        }
    }
}

static void wifi_add_hidden_net_security_event_handle(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    lv_obj_t *drop_obj = lv_obj_get_child(obj, 0);
    const uint32_t *key_param = lv_event_get_param(e);
    uint32_t key = (key_param != NULL) ? *key_param : 0;
    static char parem;

    if (code == LV_EVENT_KEY) {
        if (key == LV_KEY_DOWN) {
            if (lv_dropdown_is_open(drop_obj)) {
                parem = LV_KEY_DOWN;
                lv_event_send(drop_obj, LV_EVENT_KEY, &parem);
            } else {
                lv_group_focus_obj(lv_obj_get_child(obj->parent, 4));
            }
        } else if (key == LV_KEY_UP) {
            if (lv_dropdown_is_open(drop_obj)) {
                parem = LV_KEY_UP;
                lv_event_send(drop_obj, LV_EVENT_KEY, &parem);
            } else {
                lv_group_focus_obj(lv_obj_get_child(obj->parent, 2));
            }
        } else if (key == LV_KEY_ENTER) {
            if (lv_dropdown_is_open(drop_obj)) {
                parem = LV_KEY_ENTER;
                lv_event_send(drop_obj, LV_EVENT_KEY, &parem);
                lv_dropdown_close(drop_obj);
            } else {
                lv_dropdown_open(drop_obj);
            }
        } else if (!is_global_key_go_new_channel(key) || key == LV_KEY_BACK) {
            if (lv_dropdown_is_open(drop_obj)) {
                parem = key;
                lv_event_send(drop_obj, LV_EVENT_KEY, &parem);
                lv_dropdown_close(drop_obj);
                selectbox_wifi_ui_quit();
                if (key != LV_KEY_BACK) switch_page(PAGE_HOME);
                return;
            } else {
                selectbox_wifi_ui_quit();
                if (key != LV_KEY_BACK) switch_page(PAGE_HOME);
                return;
            }
        }
        int btn_num = lv_dropdown_get_selected(drop_obj);
        if (btn_num == 0) {
            select_dev_nonesec = true;
        } else {
            select_dev_nonesec = false;
        }
    }
}

static lv_obj_t *create_wifi_disconnect_list(lv_obj_t *parent, int w, int h)
{
    lv_obj_t *obj = lv_list_create(parent);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_outline_width(obj, 0, 0);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(obj, LV_OPA_0, 0);
    lv_obj_set_size(obj, LV_PCT(w), LV_PCT(h));
    lv_obj_align(obj, LV_ALIGN_CENTER, 0, 0);
    lv_group_add_obj(lv_group_get_default(), obj);
    return obj;
}

static lv_obj_t *create_list_sub_wifi_ip_msg_btn_obj(lv_obj_t *parent, int key, bool prompt1,
                                                     char *value, bool prompt2, bool is_disable)
{
    lv_obj_t *list_btn = lv_list_add_btn(parent, NULL, NULL);
    lv_obj_set_style_bg_color(list_btn, lv_palette_main(LV_PALETTE_BLUE), LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(list_btn, LV_OPA_0, 0);
    lv_obj_set_style_bg_opa(list_btn, LV_OPA_100, LV_STATE_CHECKED);
    lv_obj_set_style_pad_hor(list_btn, 0, 0);
    if (WIFI_IP_MSG_SUB_LIST_PAD_VER >= 0) {
        lv_obj_set_style_pad_ver(list_btn, WIFI_IP_MSG_SUB_LIST_PAD_VER, 0);
    }
    lv_group_remove_obj(list_btn);

    if (is_disable) {
        lv_obj_set_style_text_color(list_btn, lv_color_make(175, 175, 175), 0);
    } else {
        lv_obj_set_style_text_color(list_btn, lv_color_white(), 0);
    }
    lv_obj_t *obj;

    obj = lv_label_create(list_btn);
    lv_label_set_text(obj, (char *)lv_get_string(key));
    lv_obj_set_style_text_font(obj, &GENERAL_FONT_BIG, LV_PART_MAIN);
    lv_obj_set_flex_grow(obj, 11);
    lv_obj_set_height(obj, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_hor(obj, 0, 0);

    if (prompt1) {
        obj = lv_label_create(list_btn);
        lv_label_set_text(obj, LV_SYMBOL_LEFT);
        lv_obj_set_flex_grow(obj, 1);
    }

    obj = lv_label_create(list_btn);
    lv_obj_set_style_text_font(obj, &GENERAL_FONT_BIG, LV_PART_MAIN);
    lv_label_set_text(obj, value);
    lv_obj_set_flex_grow(obj, 17);
    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_height(obj, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_hor(obj, 0, 0);

    if (prompt2) {
        obj = lv_label_create(list_btn);
        lv_label_set_text(obj, LV_SYMBOL_RIGHT);
        lv_obj_set_flex_grow(obj, 1);
    }
    return list_btn;
}

static void create_repassword_ui(void)  // 修改密码UI
{
    WIFI_DBG("start\n");
    lv_pro_set_wifi_state(LVGL_WIFI_STATE_CONNECTING);
    wifi_selectbox_default_g = lv_group_create();
    lv_group_set_user_focus_check_cb(wifi_selectbox_default_g, group_user_focus_check_cb);
    set_current_ui_group(wifi_selectbox_default_g);

    lv_pro_wifi_selectbox_activity = lv_obj_create(lv_layer_top());
    lv_obj_set_size(lv_pro_wifi_selectbox_activity, lv_pct(100), lv_pct(100));
    lv_obj_set_style_border_width(lv_pro_wifi_selectbox_activity, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(lv_pro_wifi_selectbox_activity, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(lv_pro_wifi_selectbox_activity, LV_OPA_50, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(lv_pro_wifi_selectbox_activity, LV_SCROLLBAR_MODE_OFF);

    wifi_selectbox_obj = lv_obj_create(lv_pro_wifi_selectbox_activity);
    lv_obj_set_style_text_color(wifi_selectbox_obj, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(wifi_selectbox_obj, &GENERAL_FONT_BIG, LV_PART_MAIN);
    lv_obj_align(wifi_selectbox_obj, LV_ALIGN_TOP_MID, 0, LV_PCT(50 - WIFI_NEW_PWD_WIDGET_H));
    lv_obj_set_size(wifi_selectbox_obj, LV_PCT(WIFI_NEW_PWD_WIDGET_W),
                    LV_PCT(WIFI_NEW_PWD_WIDGET_H));
    lv_obj_set_style_pad_hor(wifi_selectbox_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(wifi_selectbox_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(wifi_selectbox_obj, lv_color_make(32, 32, 32), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(wifi_selectbox_obj, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(wifi_selectbox_obj, 2, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(wifi_selectbox_obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(wifi_selectbox_obj, 10, LV_PART_MAIN);
    lv_obj_set_flex_flow(wifi_selectbox_obj, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(wifi_selectbox_obj, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER);

    lv_obj_t *panel_obj = lv_obj_create(wifi_selectbox_obj);
    lv_obj_set_flex_grow(panel_obj, 9);
    lv_obj_set_style_text_color(panel_obj, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_width(panel_obj, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(panel_obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(panel_obj, 0, LV_PART_MAIN);
    lv_obj_set_width(panel_obj, lv_pct(100));
    lv_obj_set_flex_flow(panel_obj, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_bg_opa(panel_obj, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(panel_obj, 0, LV_PART_MAIN);
    lv_obj_set_flex_align(panel_obj, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    // 0 密码标题
    lv_obj_t *repassword_title_obj = lv_label_create(panel_obj);
    lv_obj_set_style_text_font(repassword_title_obj, &GENERAL_FONT_BIG, LV_PART_MAIN);
    lv_obj_align(repassword_title_obj, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_label_set_recolor(repassword_title_obj, true);
    lv_label_set_text_fmt(repassword_title_obj, "#ffffff %s #", lv_get_string(STR_WIFI_PWD));
    lv_obj_set_flex_grow(repassword_title_obj, 4);

    // 1 密码框
    ta = lv_textarea_create(panel_obj);
    lv_obj_set_flex_grow(ta, 9);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_max_length(ta, WIFI_MAX_PWD_LEN);
    lv_textarea_set_placeholder_text(ta, lv_get_string(STR_ENTER_WORD));
    lv_textarea_set_password_bullet(ta, ".");
    lv_obj_set_style_border_side(ta, LV_BORDER_SIDE_NONE, LV_PART_MAIN);
    lv_obj_set_style_radius(ta, 0, LV_PART_MAIN);
    lv_obj_set_style_text_color(ta, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_radius(ta, 5, LV_PART_MAIN);
    lv_obj_set_style_bg_color(ta, lv_palette_main(LV_PALETTE_BLUE), LV_PART_CURSOR);
    if (pwd_is_hidden)
        lv_textarea_set_password_mode(ta, true);
    else
        lv_textarea_set_password_mode(ta, false);
    lv_obj_add_event_cb(ta, network_reconnect_pwd_event_handle, LV_EVENT_KEY, "Password");

    // 2 密码明文按钮
    lv_obj_t *password_plain_obj = lv_checkbox_create(wifi_selectbox_obj);
    lv_obj_set_flex_grow(password_plain_obj, 5);
    lv_obj_set_style_text_font(password_plain_obj, &GENERAL_FONT_MID, LV_PART_MAIN);
    lv_checkbox_set_text(password_plain_obj, lv_get_string(STR_WIFI_SHOW_PWD));
    lv_obj_set_style_pad_ver(password_plain_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_left(password_plain_obj, 10, LV_PART_MAIN);
    lv_obj_set_style_text_color(password_plain_obj, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_align(password_plain_obj, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_set_style_outline_width(password_plain_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(password_plain_obj, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_bg_color(password_plain_obj, lv_color_make(32, 32, 32), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(password_plain_obj, lv_color_make(32, 32, 32),
                              LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(password_plain_obj, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(password_plain_obj, lv_color_hex(0x00BFFF),
                              LV_PART_INDICATOR | LV_STATE_FOCUSED);
    lv_obj_set_style_bg_color(password_plain_obj, lv_color_hex(0x00BFFF),
                              LV_PART_INDICATOR | LV_STATE_CHECKED | LV_STATE_FOCUSED);
    lv_obj_set_style_bg_opa(password_plain_obj, LV_OPA_COVER, LV_PART_INDICATOR | LV_STATE_FOCUSED);
    if (!pwd_is_hidden) {
        lv_obj_add_state(password_plain_obj, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(password_plain_obj, password_toggle_event_handler, LV_EVENT_KEY, ta);

    // 3 取消连接按钮
    static const char *strs[3];
    strs[0] = (char *)lv_get_string(STR_WIFI_CANCEL);
    strs[1] = (char *)lv_get_string(STR_BT_MAKE_CONN);
    strs[2] = "";

    lv_obj_t *choose_obj = lv_btnmatrix_create(wifi_selectbox_obj);
    lv_obj_set_width(choose_obj, LV_PCT(100));
    lv_obj_set_flex_grow(choose_obj, 10);
    lv_btnmatrix_set_map(choose_obj, strs);
    lv_obj_set_style_text_font(choose_obj, &GENERAL_FONT_MID, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(choose_obj, -6, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(choose_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(choose_obj, 0, LV_PART_ITEMS);
    lv_obj_set_style_border_width(choose_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(choose_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(choose_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(choose_obj, lv_color_make(32, 32, 32), LV_PART_MAIN);
    lv_obj_set_style_radius(choose_obj, 0, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(choose_obj, lv_color_make(26, 26, 26), LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(choose_obj, LV_OPA_0, LV_PART_ITEMS);
    lv_obj_set_style_text_color(choose_obj, lv_color_white(), LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(choose_obj, LV_OPA_100, LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_text_color(choose_obj, lv_color_black(), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_btnmatrix_set_btn_ctrl_all(choose_obj, LV_BTNMATRIX_CTRL_CHECKABLE);
    lv_btnmatrix_set_selected_btn(choose_obj, 0);
    lv_btnmatrix_set_one_checked(choose_obj, true);
    lv_obj_add_event_cb(choose_obj, handle_manual_network_password_event, LV_EVENT_KEY,
                        "reEnterPwd");
    lv_obj_add_event_cb(choose_obj, handle_manual_network_password_event, LV_EVENT_FOCUSED,
                        "reEnterPwd");
    lv_obj_add_event_cb(choose_obj, handle_manual_network_password_event, LV_EVENT_DEFOCUSED,
                        "reEnterPwd");

    // 4 键盘
    create_virtual_keyboard(ta);
    lv_group_focus_obj(ta);

    lv_obj_update_layout(lv_pro_wifi_selectbox_activity);

    WIFI_DBG("end\n");
}

static void create_request_new_password_ui(void)  // 创建请求新密码UI
{
    WIFI_DBG("start\n");
    lv_pro_set_wifi_state(LVGL_WIFI_STATE_CONNECTING);
    wifi_selectbox_default_g = lv_group_create();
    set_current_ui_group(wifi_selectbox_default_g);
    lv_group_set_user_focus_check_cb(wifi_selectbox_default_g, group_user_focus_check_cb);

    lv_pro_wifi_selectbox_activity = lv_obj_create(lv_layer_top());
    lv_obj_set_size(lv_pro_wifi_selectbox_activity, lv_pct(100), lv_pct(100));
    lv_obj_set_style_border_width(lv_pro_wifi_selectbox_activity, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(lv_pro_wifi_selectbox_activity, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(lv_pro_wifi_selectbox_activity, LV_OPA_30, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(lv_pro_wifi_selectbox_activity, LV_SCROLLBAR_MODE_OFF);

    wifi_selectbox_obj = lv_obj_create(lv_pro_wifi_selectbox_activity);
    lv_obj_set_size(wifi_selectbox_obj, lv_pct(WIFI_CONN_WIGGET_WIDHT_PCT),
                    lv_pct(WIFI_CONN_WIGGET_HEIHTH_PCT));
    lv_obj_align(wifi_selectbox_obj, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(wifi_selectbox_obj, 20, LV_PART_MAIN);
    lv_obj_set_style_border_width(wifi_selectbox_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(wifi_selectbox_obj, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_pad_gap(wifi_selectbox_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(wifi_selectbox_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(wifi_selectbox_obj, 3, LV_PART_MAIN);

    // 0 创建标题标签
    lv_obj_t *repassword_title_obj = lv_label_create(wifi_selectbox_obj);
    // lv_obj_set_height(repassword_title_obj, lv_pct(20));
    lv_obj_align(repassword_title_obj, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_text_font(repassword_title_obj, &GENERAL_FONT_BIG, LV_PART_MAIN);
    lv_label_set_text(repassword_title_obj, lv_get_string(STR_WIFI_RE_ENTER_PWD));

    // 1 创建连接与取消按钮矩阵
    static const char *strs[3];
    strs[0] = lv_get_string(STR_PROMPT_NO);
    strs[1] = lv_get_string(STR_PROMPT_YES);
    strs[2] = "";
    lv_obj_t *choose_obj = lv_btnmatrix_create(wifi_selectbox_obj);
    lv_obj_align(choose_obj, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_size(choose_obj, lv_pct(90), lv_pct(50));
    lv_btnmatrix_set_map(choose_obj, strs);
    lv_obj_set_style_bg_opa(choose_obj, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_style_border_width(choose_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(choose_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(choose_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_text_font(choose_obj, &GENERAL_FONT_BIG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(choose_obj, LV_OPA_0, LV_PART_ITEMS);
    lv_obj_set_style_border_width(choose_obj, 2, LV_PART_ITEMS);
    lv_obj_set_style_border_color(choose_obj, lv_color_hex(0x122656), LV_PART_ITEMS);
    lv_obj_set_style_text_color(choose_obj, lv_color_black(), LV_PART_ITEMS);
    lv_obj_set_style_text_color(choose_obj, lv_color_white(), LV_STATE_CHECKED | LV_PART_ITEMS);
    lv_obj_set_style_text_font(choose_obj, &GENERAL_FONT_BIG, LV_PART_ITEMS);
    lv_btnmatrix_set_btn_ctrl_all(choose_obj, LV_BTNMATRIX_CTRL_CHECKABLE);
    lv_btnmatrix_set_one_checked(choose_obj, true);
    lv_btnmatrix_set_selected_btn(choose_obj, 0);
    lv_btnmatrix_set_btn_ctrl(choose_obj, 0, LV_BTNMATRIX_CTRL_CHECKED);
    lv_obj_add_event_cb(choose_obj, connection_buttons_event_handler, LV_EVENT_KEY, "setPWD");

    lv_obj_align_to(repassword_title_obj, choose_obj, LV_ALIGN_OUT_TOP_LEFT, 0, -10);
    lv_group_focus_obj(lv_obj_get_child(wifi_selectbox_obj, 1));

    lv_obj_update_layout(lv_pro_wifi_selectbox_activity);

    WIFI_DBG("end\n");
}

static void create_toggle_conn_selection_ui()  // 连接取消选择框UI
{
    WIFI_DBG("start\n");

    int found = lv_pro_res_wifi_get_dev(select_ssid, select_pwd, select_mgmt);
    if (found) {
        WIFI_DBG("dev password found:\n");
    } else {
        WIFI_DBG("dev password not found!\n");
    }
    select_sec = (int)char_to_key_mgmt(select_mgmt);
    WIFI_DBG("select_ssid: %s\n", select_ssid);
    WIFI_DBG("select_bssid: %s\n", select_bssid);
    WIFI_DBG("select_mgmt: %s\n", select_mgmt);
    WIFI_DBG("select_pwd: %s\n", select_pwd);
    WIFI_DBG("select_sec: %d\n", select_sec);

    lv_pro_wifi_selectbox_activity = lv_obj_create(lv_layer_top());
    lv_obj_set_size(lv_pro_wifi_selectbox_activity, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(lv_pro_wifi_selectbox_activity, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(lv_pro_wifi_selectbox_activity, LV_OPA_30, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(lv_pro_wifi_selectbox_activity, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_border_width(lv_pro_wifi_selectbox_activity, 0, LV_PART_MAIN);

    wifi_selectbox_obj = lv_obj_create(lv_pro_wifi_selectbox_activity);
    lv_obj_set_size(wifi_selectbox_obj, lv_pct(WIFI_CONN_WIGGET_WIDHT_PCT),
                    lv_pct(WIFI_CONN_WIGGET_HEIHTH_PCT));
    lv_obj_align(wifi_selectbox_obj, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(wifi_selectbox_obj, 20, LV_PART_MAIN);
    lv_obj_set_style_border_width(wifi_selectbox_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(wifi_selectbox_obj, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_pad_gap(wifi_selectbox_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(wifi_selectbox_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(wifi_selectbox_obj, 3, LV_PART_MAIN);

    // 0 创建标题标签
    lv_obj_t *reqconn_title_obj = lv_label_create(wifi_selectbox_obj);
    // lv_obj_set_height(reqconn_title_obj, lv_pct(20));
    lv_obj_align(reqconn_title_obj, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_text_font(reqconn_title_obj, &GENERAL_FONT_BIG, LV_PART_MAIN);
    lv_label_set_text(reqconn_title_obj, lv_get_string(STR_WIFI_CONN));

    // 1 创建连接与取消按钮矩阵
    static const char *strs[3];
    strs[0] = lv_get_string(STR_BT_DELETE);
    strs[1] = lv_get_string(STR_WIFI_CONN);
    strs[2] = "";
    lv_obj_t *choose_obj = lv_btnmatrix_create(wifi_selectbox_obj);
    lv_obj_align(choose_obj, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_size(choose_obj, lv_pct(90), lv_pct(50));
    lv_btnmatrix_set_map(choose_obj, strs);
    lv_obj_set_style_bg_opa(choose_obj, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_style_border_width(choose_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(choose_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(choose_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_text_font(choose_obj, &GENERAL_FONT_BIG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(choose_obj, LV_OPA_0, LV_PART_ITEMS);
    lv_obj_set_style_border_width(choose_obj, 2, LV_PART_ITEMS);
    lv_obj_set_style_border_color(choose_obj, lv_color_hex(0x122656), LV_PART_ITEMS);
    lv_obj_set_style_text_color(choose_obj, lv_color_black(), LV_PART_ITEMS);
    lv_obj_set_style_text_color(choose_obj, lv_color_white(), LV_STATE_CHECKED | LV_PART_ITEMS);
    lv_obj_set_style_text_font(choose_obj, &GENERAL_FONT_BIG, LV_PART_ITEMS);
    lv_btnmatrix_set_btn_ctrl_all(choose_obj, LV_BTNMATRIX_CTRL_CHECKABLE);
    lv_btnmatrix_set_one_checked(choose_obj, true);
    lv_btnmatrix_set_selected_btn(choose_obj, 1);
    lv_btnmatrix_set_btn_ctrl(choose_obj, 1, LV_BTNMATRIX_CTRL_CHECKED);
    lv_obj_add_event_cb(choose_obj, connection_buttons_event_handler, LV_EVENT_KEY, "AskConn");

    lv_obj_align_to(reqconn_title_obj, choose_obj, LV_ALIGN_OUT_TOP_LEFT, 0, -10);
    lv_group_focus_obj(lv_obj_get_child(wifi_selectbox_obj, 1));

    lv_obj_update_layout(lv_pro_wifi_selectbox_activity);

    WIFI_DBG("end\n");
}

static void show_wifi_details_and_disconnect_ui(void)  // Wi-Fi详情消息与断开UI页面
{
    WIFI_DBG("start\n");

    lv_pro_wifi_selectbox_activity = lv_obj_create(lv_layer_top());
    lv_obj_set_size(lv_pro_wifi_selectbox_activity, lv_pct(100), lv_pct(100));
    lv_obj_set_style_border_width(lv_pro_wifi_selectbox_activity, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(lv_pro_wifi_selectbox_activity, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(lv_pro_wifi_selectbox_activity, LV_OPA_50, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(lv_pro_wifi_selectbox_activity, LV_SCROLLBAR_MODE_OFF);

    wifi_selectbox_obj = lv_obj_create(lv_pro_wifi_selectbox_activity);
    lv_obj_set_size(wifi_selectbox_obj, lv_pct(WIFI_DISCONN_WIGGET_WIDHT_PCT),
                    lv_pct(WIFI_DISCONN_WIGGET_HEIHTH_PCT));
    lv_obj_align(wifi_selectbox_obj, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_gap(wifi_selectbox_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(wifi_selectbox_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(wifi_selectbox_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(wifi_selectbox_obj, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(wifi_selectbox_obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(wifi_selectbox_obj, lv_color_make(32, 32, 32), LV_PART_MAIN);
    lv_obj_set_style_text_color(wifi_selectbox_obj, lv_color_white(), LV_PART_MAIN);

    // 0 SSID标题
    lv_obj_t *ssid_title_obj = lv_label_create(wifi_selectbox_obj);
    lv_obj_set_size(ssid_title_obj, LV_PCT(100), LV_PCT(14));
    lv_obj_set_style_text_align(ssid_title_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_top(ssid_title_obj, 10, 0);
    lv_obj_align(ssid_title_obj, LV_ALIGN_TOP_MID, 0, 0);
    lv_label_set_text(ssid_title_obj, select_ssid);
    lv_obj_set_style_text_font(ssid_title_obj, &GENERAL_FONT_BIG, LV_PART_MAIN);

    wifi_sta_info_t wifi_sta_info;
    memset(&wifi_sta_info, 0, sizeof(wifi_sta_info_t));
    int ret = wifi_sta_get_info(&wifi_sta_info);
    if (ret == 0) {
        WIFI_DBG("sta ip_addr: %d.%d.%d.%d\n", wifi_sta_info.ip_addr[0], wifi_sta_info.ip_addr[1],
                 wifi_sta_info.ip_addr[2], wifi_sta_info.ip_addr[3]);

        snprintf(select_devip, 18, "%d.%d.%d.%d", wifi_sta_info.ip_addr[0],
                 wifi_sta_info.ip_addr[1], wifi_sta_info.ip_addr[2], wifi_sta_info.ip_addr[3]);
        snprintf(select_geteway, 18, "%d.%d.%d.%d", wifi_sta_info.gw_addr[0],
                 wifi_sta_info.gw_addr[1], wifi_sta_info.gw_addr[2], wifi_sta_info.gw_addr[3]);
        snprintf(select_bssid, 18, "%X:%X:%X:%X:%X:%X", wifi_sta_info.mac_addr[0],
                 wifi_sta_info.mac_addr[1], wifi_sta_info.mac_addr[2], wifi_sta_info.mac_addr[3],
                 wifi_sta_info.mac_addr[4], wifi_sta_info.mac_addr[5]);
    } else {
        WIFI_DBG("Failed to get Wi-Fi STA info, ret: %d\n", ret);
        snprintf(select_devip, 18, "null");
    }

    char *net_mode = "DHCP";
    char *ip_addr = select_devip;
    char *gateway = select_geteway;
    char *mac = select_bssid;
    // gateway 、 net_wsk 、 htc_dns 暂无接口获取
    //  char *net_mask = "null";
    //  char *htc_dns = "null";

    lv_obj_t *list = create_wifi_disconnect_list(wifi_selectbox_obj, 100, 70);
    lv_obj_align_to(list, ssid_title_obj, LV_ALIGN_OUT_BOTTOM_MID, 0, LV_PART_MAIN);
    if (WIFI_IP_MSG_PAD_HOR >= 0) {
        lv_obj_set_style_pad_hor(list, WIFI_IP_MSG_PAD_HOR, 0);
    }
    create_list_sub_wifi_ip_msg_btn_obj(list, STR_WIFI_NET_MODE, false, net_mode, false, false);
    create_list_sub_wifi_ip_msg_btn_obj(list, STR_WIFI_IP_ADDR, false, ip_addr, false, false);
    create_list_sub_wifi_ip_msg_btn_obj(list, STR_WIFI_MAC, false, mac, false, false);
    create_list_sub_wifi_ip_msg_btn_obj(list, STR_WIFI_GATE_WAY, false, gateway, false, false);
    // create_list_sub_wifi_ip_msg_btn_obj(list, STR_WIFI_NET_MASK, false, net_mask, false, false);
    // create_list_sub_wifi_ip_msg_btn_obj(list, STR_WIFI_DNS, false, htc_dns, false, false);

    list->user_data = (void *)true;

    // 2 底部下划线
    lv_obj_t *foot = lv_obj_create(wifi_selectbox_obj);
    lv_obj_set_size(foot, LV_PCT(100), LV_PCT(16));
    lv_obj_set_scrollbar_mode(foot, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(foot, 0, 0);
    lv_obj_set_style_border_width(foot, 0, 0);
    lv_obj_set_style_pad_gap(foot, 0, 0);
    lv_obj_set_style_bg_opa(foot, LV_OPA_0, 0);
    lv_obj_set_flex_flow(foot, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(foot, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_align(foot, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_obj_t *btn = lv_btn_create(lv_obj_get_child(wifi_selectbox_obj, 2));
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, lv_get_string(STR_BT_MAKE_DISCONN));
    lv_obj_set_style_text_font(label, &GENERAL_FONT_BIG, LV_PART_MAIN);
    lv_obj_add_event_cb(btn, handle_disconnect_button_event, LV_EVENT_KEY, (void *)false);
    lv_obj_set_style_radius(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_100, LV_STATE_FOCUSED);
    lv_group_focus_obj(btn);

    btn = lv_btn_create(lv_obj_get_child(wifi_selectbox_obj, 2));
    label = lv_label_create(btn);
    lv_label_set_text(label, lv_get_string(STR_BT_DELETE));
    lv_obj_set_style_text_font(label, &GENERAL_FONT_BIG, LV_PART_MAIN);
    lv_obj_add_event_cb(btn, handle_disconnect_button_event, LV_EVENT_KEY, (void *)true);
    lv_obj_set_style_radius(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_100, LV_STATE_FOCUSED);

    lv_obj_update_layout(lv_pro_wifi_selectbox_activity);

    WIFI_DBG("end\n");
}

static void add_hidden_wifi_ui(void)  // 添加隐藏wifi ui
{
    WIFI_DBG("start\n");

    lv_pro_wifi_selectbox_activity = lv_obj_create(lv_layer_top());
    lv_obj_set_size(lv_pro_wifi_selectbox_activity, lv_pct(100), lv_pct(100));
    lv_obj_set_style_border_width(lv_pro_wifi_selectbox_activity, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(lv_pro_wifi_selectbox_activity, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(lv_pro_wifi_selectbox_activity, LV_OPA_50, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(lv_pro_wifi_selectbox_activity, LV_SCROLLBAR_MODE_OFF);

    wifi_selectbox_obj = lv_obj_create(lv_pro_wifi_selectbox_activity);
    lv_obj_set_style_text_font(wifi_selectbox_obj, &GENERAL_FONT_BIG, 0);
    lv_obj_set_style_text_color(wifi_selectbox_obj, lv_color_white(), 0);
    lv_obj_set_style_text_opa(wifi_selectbox_obj, LV_OPA_COVER, 0);
    lv_obj_align(wifi_selectbox_obj, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(wifi_selectbox_obj, lv_pct(WIFI_ADD_HIDDEN_NET_WIDTH),
                    lv_pct(WIFI_ADD_HIDDEN_NET_HEIGHT));
    lv_obj_set_style_pad_gap(wifi_selectbox_obj, 0, 0);
    lv_obj_set_style_pad_top(wifi_selectbox_obj, 3, 0);
    lv_obj_set_style_pad_bottom(wifi_selectbox_obj, 0, 0);
    lv_obj_set_style_pad_hor(wifi_selectbox_obj, 0, 0);
    lv_obj_set_style_bg_color(wifi_selectbox_obj, lv_color_make(32, 32, 32), 0);
    lv_obj_set_style_border_width(wifi_selectbox_obj, 0, 0);
    lv_obj_set_style_radius(wifi_selectbox_obj, 10, 0);
    lv_obj_set_scrollbar_mode(wifi_selectbox_obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(wifi_selectbox_obj, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(wifi_selectbox_obj, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    // 0 标题
    lv_obj_t *obj;
    obj = lv_label_create(wifi_selectbox_obj);
    lv_label_set_text(obj, lv_get_string(STR_WIFI_ADD));
    lv_obj_set_style_pad_top(obj, 10, 0);
    lv_obj_set_style_pad_bottom(obj, 10, 0);
    lv_obj_set_flex_grow(obj, 2);
    lv_obj_set_width(obj, lv_pct(98));
    lv_obj_set_style_text_font(obj, &GENERAL_FONT_BIG, 0);
    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, 0);

    // 1 输入SSID
    obj = lv_textarea_create(wifi_selectbox_obj);
    lv_obj_set_flex_grow(obj, 2);
    lv_obj_set_width(obj, lv_pct(100));
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_pad_left(obj, 5, 0);
    lv_obj_set_style_text_color(obj, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_textarea_set_placeholder_text(obj, lv_get_string(STR_BT_DEVICE));
    lv_textarea_set_max_length(obj, SSID_MAX_LEN);
    lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_textarea_set_one_line(obj, true);
    lv_obj_set_style_border_color(obj, lv_color_make(30, 80, 160), LV_STATE_FOCUSED);
    lv_obj_add_event_cb(obj, wifi_add_hidden_net_name_event_handle, LV_EVENT_KEY, 0);
    lv_textarea_set_align(obj, LV_TEXT_ALIGN_LEFT);

    // 2 输入密码
    obj = lv_textarea_create(wifi_selectbox_obj);
    lv_obj_set_flex_grow(obj, 2);
    lv_obj_set_width(obj, lv_pct(100));
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_pad_left(obj, 5, 0);
    lv_textarea_set_placeholder_text(obj, lv_get_string(STR_ENTER_WORD));
    lv_textarea_set_max_length(obj, WIFI_MAX_PWD_LEN);
    lv_obj_set_style_text_color(obj, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_textarea_set_one_line(obj, true);
    lv_obj_set_style_border_color(obj, lv_color_make(30, 80, 160), LV_STATE_FOCUSED);
    lv_obj_add_event_cb(obj, wifi_add_hidden_net_pwd_event_handle, LV_EVENT_KEY, 0);
    lv_textarea_set_align(obj, LV_TEXT_ALIGN_LEFT);

    // 3 选择加密方式
    {
        obj = lv_obj_create(wifi_selectbox_obj);
        lv_group_add_obj(wifi_selectbox_default_g, obj);
        lv_obj_set_flex_grow(obj, 2);
        lv_obj_set_width(obj, lv_pct(100));
        lv_obj_set_style_pad_all(obj, 1, 0);
        lv_obj_set_style_pad_left(obj, 5, 0);
        lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_style_radius(obj, 3, 0);
        lv_obj_set_style_bg_opa(obj, LV_OPA_0, 0);
        lv_obj_set_style_border_width(obj, 2, 0);
        lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_BOTTOM, 0);
        lv_obj_add_event_cb(obj, wifi_add_hidden_net_security_event_handle, LV_EVENT_KEY, NULL);

        lv_obj_t *drop_obj = lv_dropdown_create(obj);
        lv_textarea_set_align(drop_obj, LV_TEXT_ALIGN_LEFT);
        lv_dropdown_set_options(drop_obj,
                                "NONE\n"
                                "WEP\n"
                                "WPA/WPA2\n"
                                "WPA3"

        );
        lv_obj_set_width(drop_obj, lv_pct(50));
        lv_obj_align(drop_obj, LV_ALIGN_TOP_LEFT, 10, 0);
        lv_obj_set_scrollbar_mode(drop_obj, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_style_border_width(drop_obj, 1, 0);
        lv_obj_set_style_border_color(drop_obj, lv_color_make(30, 80, 160), LV_STATE_FOCUSED);
        lv_dropdown_set_dir(drop_obj, LV_DIR_RIGHT);
        lv_dropdown_set_symbol(drop_obj, "\xE2\x86\x92");
        lv_obj_set_style_text_color(drop_obj, lv_color_white(), 0);
        lv_obj_set_style_text_font(drop_obj, &GENERAL_FONT_MID, 0);
        lv_obj_set_style_bg_opa(drop_obj, LV_OPA_0, 0);
        int sel = lv_dropdown_get_option_cnt(drop_obj) - 2;
        lv_dropdown_set_selected(drop_obj, sel);

        lv_obj_t *list = lv_dropdown_get_list(drop_obj);
        lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_style_text_font(list, &GENERAL_FONT_MID, 0);
        if (WIFI_ADD_HIDDEN_DROP_OBJ_LIST_LINE_SPACE >= 0) {
            lv_obj_set_style_text_line_space(list, WIFI_ADD_HIDDEN_DROP_OBJ_LIST_LINE_SPACE, 0);
        }
        if (WIFI_ADD_HIDDEN_DROP_OBJ_LIST_PAD >= 0) {
            lv_obj_set_style_pad_all(list, WIFI_ADD_HIDDEN_DROP_OBJ_LIST_PAD, 0);
        }
    }

    // 按钮
    static const char *strs[3];
    strs[0] = lv_get_string(STR_WIFI_CANCEL);
    strs[1] = lv_get_string(STR_BT_MAKE_CONN);
    strs[2] = "";
    obj = lv_btnmatrix_create(wifi_selectbox_obj);
    lv_obj_set_width(obj, lv_pct(100));
    lv_obj_set_flex_grow(obj, 3);
    lv_btnmatrix_set_map(obj, strs);
    lv_obj_set_style_text_font(obj, &GENERAL_FONT_MID, LV_PART_MAIN);
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(obj, lv_color_make(32, 32, 32), LV_PART_MAIN);
    lv_obj_set_style_radius(obj, 0, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(obj, lv_color_make(26, 26, 26), LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(obj, LV_OPA_0, LV_PART_ITEMS);
    lv_obj_set_style_text_color(obj, lv_color_white(), LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(obj, LV_OPA_100, LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_text_color(obj, lv_color_black(), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_btnmatrix_set_btn_ctrl_all(obj, LV_BTNMATRIX_CTRL_CHECKABLE);
    lv_btnmatrix_set_selected_btn(obj, 0);
    lv_btnmatrix_set_one_checked(obj, true);
    lv_obj_add_event_cb(obj, handle_manual_network_password_event, LV_EVENT_KEY, "addNW");
    lv_obj_add_event_cb(obj, handle_manual_network_password_event, LV_EVENT_FOCUSED, "addNW");
    lv_obj_add_event_cb(obj, handle_manual_network_password_event, LV_EVENT_DEFOCUSED, "addNW");

    lv_group_focus_obj(lv_obj_get_child(wifi_selectbox_obj, 1));

    lv_obj_update_layout(lv_pro_wifi_selectbox_activity);

    WIFI_DBG("end\n");
}

void lv_pro_selectbox_wifi_init(void *obj, int state_enum)
{
    select_btn = (lv_pro_set_btn_style6_t *)obj;

    initialize_selected_device_parameters();

    wifi_activity_g = lv_group_get_default();
    wifi_selectbox_default_g = lv_group_create();
    set_current_ui_group(wifi_selectbox_default_g);
    lv_group_set_user_focus_check_cb(wifi_selectbox_default_g, group_user_focus_check_cb);

    if (state_enum == LVGL_WIFI_STATE_CONNECTING) {
        create_toggle_conn_selection_ui();
    } else if (state_enum == LVGL_WIFI_STATE_DISCONNECTING) {
        show_wifi_details_and_disconnect_ui();
    } else if (state_enum == LVGL_WIFI_STATE_PAIRING) {
        add_hidden_wifi_ui();
    }
}

static void selectbox_wifi_ui_quit()
{
    lv_pro_selectbox_wifi_deinit();
    set_current_ui_group(wifi_activity_g);
    lv_pro_set_wifi_state(LVGL_WIFI_STATE_IDLE);
}

static void lv_pro_selectbox_wifi_deinit(void)
{
    WIFI_DBG("start\n");
    if (kb_g) {
        lv_group_remove_all_objs(kb_g);
        lv_group_del(kb_g);
        kb_g = NULL;
    }
    if (kb) {
        lv_obj_del(kb);
        kb = NULL;
    }
    if (wifi_selectbox_default_g) {
        lv_group_remove_all_objs(wifi_selectbox_default_g);
        lv_group_del(wifi_selectbox_default_g);
        wifi_selectbox_default_g = NULL;
    }
    if (wifi_selectbox_obj) {
        lv_obj_del(wifi_selectbox_obj);
        wifi_selectbox_obj = NULL;
    }
    if (lv_pro_wifi_selectbox_activity) {
        lv_obj_del(lv_pro_wifi_selectbox_activity);
        lv_pro_wifi_selectbox_activity = NULL;
    }
    lv_group_set_default(NULL);
    WIFI_DBG("end\n");
}

#endif /* ENABLE_WIFI */