/*
 * lv_pro_connect_wifi_activity.c
 *
 *  Created on: 2024/09/12
 *      Author: JASON
 */

#include "lv_pro_connect_wifi_activity.h"

#include "lv_pro_res_wifi.h"
#include "lv_pro_wifi_activity.h"

#if ENABLE_WIFI
static lv_obj_t *htc_msg_box = NULL;
static lv_pro_set_btn_style6_t *select_btn;

static lv_obj_t *lv_pro_connect_wifi_activity;
static lv_obj_t *wifi_conn_obj;
static lv_obj_t *kb;
static lv_obj_t *ta;

static lv_group_t *wifi_activity_g;
static lv_group_t *wifi_connect_g;
static lv_group_t *kb_g;

static char select_ssid[64] = {0};
static char select_bssid[18] = {0};
static char select_mgmt[64] = {0};
static int select_rssi = 0;
static int select_sec = 0;

static bool pwd_is_hidden = false;
static bool keypad_exit = false;

static void kb_btn_wifi_conn_run_cb(void);
static void create_keypad_widget(void);
static void pwd_event_handle(lv_event_t *e);
static void btns_event_handle(lv_event_t *e);
static void checkout_event_handle(lv_event_t *e);
static void wifi_kbbtns_event_handle(lv_event_t *e);
static void kb_delete_and_cutgroup(void);

static int group_user_focus_check_cb(lv_group_t *group)
{
    return 1;  // 返回1，交给2级处理
}

static void group_user_focus_edge_cb(lv_group_t *group, bool next)
{
    if (kb_g == group && !next && keypad_exit == true) {
        kb_delete_and_cutgroup();
        keypad_exit = false;
    }
    return;
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

static void create_keypad_widget(void)
{
    int height = 100 - WIFI_CONNE_WIDGET_HEIGHT_PCT - WIFI_CONNE_WIDGET_ALIGN_TOP - 5;

    kb_g_onoff(true);
    kb = lv_keyboard_create(lv_pro_connect_wifi_activity);
    lv_obj_set_size(kb, lv_pct(100), lv_pct(height));
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

    lv_keyboard_set_textarea(kb, ta);
    lv_obj_add_event_cb(kb, wifi_kbbtns_event_handle, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(kb, wifi_kbbtns_event_handle, LV_EVENT_CLICKED, NULL);
}

static void kb_delete_and_cutgroup(void)
{
    set_current_ui_group(wifi_connect_g);

    kb_g_onoff(false);

    if (kb) {
        lv_obj_del(kb);
        kb = NULL;
    }

    lv_obj_center(wifi_conn_obj);
    lv_group_focus_obj(lv_obj_get_child(wifi_conn_obj, 8));
}

static void kb_btn_wifi_conn_run_cb(void)
{
    const char *pwd = lv_textarea_get_text(lv_obj_get_child(wifi_conn_obj, 6));

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

    lv_pro_set_btn_style6_set_pwd(select_btn, pwd);
    lv_pro_connect_wifi_deinit();
    set_current_ui_group(wifi_activity_g);
    lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_CONN_SCANDEV, select_btn, false);

    return true;
}

static void pwd_event_handle(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    const uint32_t *key_param = lv_event_get_param(e);
    uint32_t key = (key_param != NULL) ? *key_param : 0;

    if (code == LV_EVENT_KEY) {
        if (key == LV_KEY_UP) {
            lv_group_focus_obj(obj);
        } else if (key == LV_KEY_ENTER) {
            if (kb != NULL) {
                lv_group_focus_obj(ta);
            } else {
                lv_obj_align(wifi_conn_obj, LV_ALIGN_TOP_MID, 0,
                             lv_pct(WIFI_CONNE_WIDGET_ALIGN_TOP));
                create_keypad_widget();
                lv_group_focus_obj(ta);
            }
            return;
        } else if (key == LV_KEY_DOWN) {
            lv_group_focus_obj(lv_obj_get_child(obj->parent, 7));
        } else if (!is_global_key_go_new_channel(key) || key == LV_KEY_BACK) {
            lv_pro_connect_wifi_deinit();
            set_current_ui_group(wifi_activity_g);
            lv_pro_set_wifi_state(LVGL_WIFI_STATE_IDLE);
            if (key != LV_KEY_BACK) switch_page(PAGE_HOME);
            return;
        }
    }
}

static void checkout_event_handle(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    lv_obj_t *ta = (lv_obj_t *)lv_event_get_user_data(e);
    const uint32_t *key_param = lv_event_get_param(e);
    uint32_t key = (key_param != NULL) ? *key_param : 0;

    if (code == LV_EVENT_KEY) {
        if (key == LV_KEY_UP) {
            lv_group_focus_obj(lv_obj_get_child(obj->parent, 6));
            if (pwd_is_hidden) lv_obj_clear_state(obj, LV_STATE_CHECKED);
        } else if (key == LV_KEY_DOWN) {
            lv_group_focus_obj(lv_obj_get_child(obj->parent, 8));
            if (!pwd_is_hidden) lv_obj_add_state(obj, LV_STATE_CHECKED);
        } else if (!is_global_key_go_new_channel(key) || key == LV_KEY_BACK) {
            lv_pro_connect_wifi_deinit();
            set_current_ui_group(wifi_activity_g);
            lv_pro_set_wifi_state(LVGL_WIFI_STATE_IDLE);
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

static void btns_event_handle(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    const uint32_t *key_param = lv_event_get_param(e);
    uint32_t key = (key_param != NULL) ? *key_param : 0;
    lv_obj_t *obj = lv_event_get_target(e);
    lv_btnmatrix_t *btnms = (lv_btnmatrix_t *)obj;

    if (code == LV_EVENT_KEY) {
        if (key == LV_KEY_UP) {
            lv_group_focus_obj(lv_obj_get_child(obj->parent, 7));
        }
        if (key == LV_KEY_DOWN) {
            lv_group_focus_obj(obj);
        } else if (key == LV_KEY_LEFT) {
            btnms->btn_id_sel = 0;
            lv_btnmatrix_clear_btn_ctrl_all(obj, LV_BTNMATRIX_CTRL_CHECKED);
            lv_btnmatrix_set_btn_ctrl(obj, 0, LV_BTNMATRIX_CTRL_CHECKED);
            lv_btnmatrix_set_selected_btn(obj, 0);
        } else if (key == LV_KEY_RIGHT) {
            btnms->btn_id_sel = 1;
            lv_btnmatrix_clear_btn_ctrl_all(obj, LV_BTNMATRIX_CTRL_CHECKED);
            lv_btnmatrix_set_btn_ctrl(obj, 1, LV_BTNMATRIX_CTRL_CHECKED);
            lv_btnmatrix_set_selected_btn(obj, 1);
        } else if (!is_global_key_go_new_channel(key) || key == LV_KEY_BACK) {
            lv_pro_connect_wifi_deinit();
            set_current_ui_group(wifi_activity_g);
            lv_pro_set_wifi_state(LVGL_WIFI_STATE_IDLE);
            if (key != LV_KEY_BACK) switch_page(PAGE_HOME);
            return;
        } else if (key == LV_KEY_ENTER) {
            int id = btnms->btn_id_sel;
            if (id == 1) {
                kb_btn_wifi_conn_run_cb();
            } else if (id == 0) {
                lv_pro_connect_wifi_deinit();
                set_current_ui_group(wifi_activity_g);
                lv_pro_set_wifi_state(LVGL_WIFI_STATE_IDLE);
            }
            return;
        }
    } else if (code == LV_EVENT_FOCUSED) {
        lv_btnmatrix_clear_btn_ctrl_all(obj, LV_BTNMATRIX_CTRL_CHECKED);
        if (btnms->btn_id_sel == 1) {
            lv_btnmatrix_set_btn_ctrl(obj, 1, LV_BTNMATRIX_CTRL_CHECKED);
        } else {
            btnms->btn_id_sel = 0;
            lv_btnmatrix_set_btn_ctrl(obj, 0, LV_BTNMATRIX_CTRL_CHECKED);
        }
    } else if (code == LV_EVENT_DEFOCUSED) {
        lv_btnmatrix_clear_btn_ctrl_all(obj, LV_BTNMATRIX_CTRL_CHECKED);
    }
}

static void wifi_kbbtns_event_handle(lv_event_t *e)
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
        } else if (!is_global_key_go_new_channel(key) || key == LV_KEY_BACK) {
            lv_pro_connect_wifi_deinit();
            set_current_ui_group(wifi_activity_g);
            lv_pro_set_wifi_state(LVGL_WIFI_STATE_IDLE);
            if (key != LV_KEY_BACK) switch_page(PAGE_HOME);
            return;
        } else if (key == LV_KEY_ENTER) {
            if (strcmp(txt, LV_SYMBOL_OK) == 0) {
                kb_btn_wifi_conn_run_cb();
                return;
            } else if (strcmp(txt, "abc") == 0) {
                abc_btn_flag = 1;
            }
        } else if (key == LV_KEY_UP) {
            lv_btnmatrix_t *btnm = &((lv_keyboard_t *)obj)->btnm;
            if (btnm->button_areas[sel_id].y1 == btnm->button_areas[0].y1) {
                // 由于up事件特殊，会触发group查询，需要等到查询结束在edge_cb里面退出
                keypad_exit = true;
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

void lv_pro_connect_wifi_deinit(void)
{
    WIFI_DBG("%s start\n", __func__);
    if (kb_g) {
        lv_group_remove_all_objs(kb_g);
        lv_group_del(kb_g);
        kb_g = NULL;
    }
    if (kb) {
        lv_obj_del(kb);
        kb = NULL;
    }
    if (wifi_connect_g) {
        lv_group_remove_all_objs(wifi_connect_g);
        lv_group_del(wifi_connect_g);
        wifi_connect_g = NULL;
    }

    if (lv_pro_connect_wifi_activity) {
        lv_obj_del(lv_pro_connect_wifi_activity);
        lv_pro_connect_wifi_activity = NULL;
    }
    lv_group_set_default(NULL);
    WIFI_DBG("%s end\n", __func__);
}

void lv_pro_connect_wifi_init(void *obj)
{
    WIFI_DBG("%s start\n", __func__);

    wifi_activity_g = lv_group_get_default();

    select_btn = (lv_pro_set_btn_style6_t *)obj;
    memset(select_ssid, '\0', sizeof(select_ssid));
    memset(select_bssid, '\0', sizeof(select_bssid));
    memset(select_mgmt, '\0', sizeof(select_mgmt));
    select_sec = lv_pro_set_btn_style6_get_sec_value(select_btn);
    select_rssi = lv_pro_set_btn_style6_get_rssi_value(select_btn);
    memcpy(select_ssid, lv_pro_set_btn_style6_get_name_str(select_btn), sizeof(select_ssid));
    memcpy(select_bssid, lv_pro_set_btn_style6_get_mac_addr_str(select_btn), sizeof(select_bssid));
    memcpy(select_mgmt, lv_pro_set_btn_style6_get_state_str(select_btn), sizeof(select_mgmt));

    wifi_connect_g = lv_group_create();
    lv_group_set_user_focus_check_cb(wifi_connect_g, group_user_focus_check_cb);
    set_current_ui_group(wifi_connect_g);

    // 创建WIFI半透明顶页
    lv_pro_connect_wifi_activity = lv_obj_create(lv_layer_top());
    lv_obj_set_size(lv_pro_connect_wifi_activity, lv_pct(100), lv_pct(100));
    lv_obj_set_style_border_width(lv_pro_connect_wifi_activity, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(lv_pro_connect_wifi_activity, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(lv_pro_connect_wifi_activity, LV_OPA_50, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(lv_pro_connect_wifi_activity, LV_SCROLLBAR_MODE_OFF);

    // 创建连接框
    wifi_conn_obj = lv_obj_create(lv_pro_connect_wifi_activity);
    lv_obj_set_size(wifi_conn_obj, lv_pct(WIFI_CONNE_WIDGET_WIDTH_PCT),
                    lv_pct(WIFI_CONNE_WIDGET_HEIGHT_PCT));
    lv_obj_align(wifi_conn_obj, LV_ALIGN_TOP_MID, 0, lv_pct(WIFI_CONNE_WIDGET_ALIGN_TOP));
    lv_obj_set_style_border_width(wifi_conn_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(wifi_conn_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(wifi_conn_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(wifi_conn_obj, 3, LV_PART_MAIN);
    lv_obj_set_style_radius(wifi_conn_obj, 5, LV_PART_MAIN);
    lv_obj_set_style_bg_color(wifi_conn_obj, lv_color_make(32, 32, 32), LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(wifi_conn_obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(wifi_conn_obj, LV_OBJ_FLAG_USER_1);

    // 1 WIFI名称
    lv_obj_t *ssid_obj = lv_label_create(wifi_conn_obj);
    lv_obj_set_width(ssid_obj, lv_pct(100));
    lv_obj_align(ssid_obj, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_border_side(ssid_obj, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
    lv_obj_set_style_pad_all(ssid_obj, 2, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ssid_obj, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_style_text_font(ssid_obj, &GENERAL_FONT_BIG, LV_PART_MAIN);
    lv_obj_set_style_text_color(ssid_obj, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_align(ssid_obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_label_set_text(ssid_obj, select_ssid);
    lv_label_set_long_mode(ssid_obj, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_anim_speed(ssid_obj, 30, 0);

    // 2 信号强度标题
    lv_obj_t *strength_title_obj = lv_label_create(wifi_conn_obj);
    lv_obj_align_to(strength_title_obj, ssid_obj, LV_ALIGN_OUT_BOTTOM_LEFT, 9,
                    WIFI_CONNE_WIDGET_ALIGN_TOP0);
    if (WIFI_CONN_WIDGET_SET_H) lv_obj_set_height(strength_title_obj, lv_pct(9));
    lv_obj_set_style_pad_all(strength_title_obj, 2, LV_PART_MAIN);
    lv_obj_set_style_text_font(strength_title_obj, &GENERAL_FONT_BIG, LV_PART_MAIN);
    lv_label_set_recolor(strength_title_obj, true);
    lv_label_set_text_fmt(strength_title_obj, "#ffffff %s #", lv_get_string(STR_WIFI_STRENGTH));

    // 3 信号强度值
    lv_obj_t *strength_value_obj = lv_label_create(wifi_conn_obj);
    lv_obj_align_to(strength_value_obj, strength_title_obj, LV_ALIGN_OUT_BOTTOM_LEFT, 0,
                    WIFI_CONNE_WIDGET_ALIGN_TOP1);
    if (WIFI_CONN_WIDGET_SET_H) lv_obj_set_height(strength_value_obj, lv_pct(9));
    lv_obj_set_style_pad_all(strength_value_obj, 2, LV_PART_MAIN);
    lv_obj_set_style_text_font(strength_value_obj, &GENERAL_FONT_MID, LV_PART_MAIN);
    lv_obj_set_style_text_color(strength_value_obj, lv_color_hex(0xDCDCDC), LV_PART_MAIN);
    lv_label_set_text(strength_value_obj, get_rssi_signal_strength(select_rssi));

    // 4 WIFI安全等级标题
    lv_obj_t *security_title_obj = lv_label_create(wifi_conn_obj);
    lv_obj_align_to(security_title_obj, strength_value_obj, LV_ALIGN_OUT_BOTTOM_LEFT, 0,
                    WIFI_CONNE_WIDGET_ALIGN_TOP2);
    if (WIFI_CONN_WIDGET_SET_H) lv_obj_set_height(security_title_obj, lv_pct(9));
    lv_obj_set_style_pad_all(security_title_obj, 2, LV_PART_MAIN);
    lv_obj_set_style_text_font(security_title_obj, &GENERAL_FONT_BIG, LV_PART_MAIN);
    lv_label_set_recolor(security_title_obj, true);
    lv_label_set_text_fmt(security_title_obj, "#ffffff %s #", lv_get_string(STR_WIFI_SECURITY));

    // 5 WIFI安全等级
    lv_obj_t *security_value_obj = lv_label_create(wifi_conn_obj);
    lv_obj_align_to(security_value_obj, security_title_obj, LV_ALIGN_OUT_BOTTOM_LEFT, 0,
                    WIFI_CONNE_WIDGET_ALIGN_TOP3);
    if (WIFI_CONN_WIDGET_SET_H) lv_obj_set_height(security_value_obj, lv_pct(9));
    lv_obj_set_style_pad_all(security_value_obj, 2, LV_PART_MAIN);
    lv_obj_set_style_text_font(security_value_obj, &GENERAL_FONT_MID, LV_PART_MAIN);
    lv_obj_set_style_text_color(security_value_obj, lv_color_hex(0xDCDCDC), LV_PART_MAIN);
    lv_label_set_text(security_value_obj, select_mgmt);

    // 6 密码标题
    lv_obj_t *password_title_obj = lv_label_create(wifi_conn_obj);
    lv_obj_align_to(password_title_obj, security_value_obj, LV_ALIGN_OUT_BOTTOM_LEFT, 0,
                    WIFI_CONNE_WIDGET_ALIGN_TOP4);
    if (WIFI_CONN_WIDGET_SET_H) lv_obj_set_height(password_title_obj, lv_pct(9));
    lv_obj_set_style_pad_all(password_title_obj, 2, LV_PART_MAIN);
    lv_obj_set_style_text_font(password_title_obj, &GENERAL_FONT_BIG, LV_PART_MAIN);
    lv_label_set_recolor(password_title_obj, true);
    lv_label_set_text_fmt(password_title_obj, "#ffffff %s #", lv_get_string(STR_WIFI_PWD));

    // 7 密码
    ta = lv_textarea_create(wifi_conn_obj);
    lv_obj_set_width(ta, lv_pct(95));
    lv_obj_align_to(ta, password_title_obj, LV_ALIGN_OUT_BOTTOM_LEFT, 0,
                    WIFI_CONNE_WIDGET_ALIGN_TOP5);
    lv_obj_set_style_pad_all(ta, 0, LV_PART_MAIN);
    lv_obj_set_style_border_side(ta, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
    lv_obj_set_style_border_color(ta, lv_color_make(0, 0, 255), LV_PART_MAIN | LV_STATE_FOCUSED);

    lv_textarea_set_max_length(ta, WIFI_MAX_PWD_LEN - 1);
    lv_textarea_set_one_line(ta, true);
    lv_obj_set_style_text_font(ta, &GENERAL_FONT_MID, LV_PART_MAIN);
    lv_obj_set_style_text_color(ta, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_textarea_set_placeholder_text(ta, lv_get_string(STR_ENTER_WORD));
    if (pwd_is_hidden)
        lv_textarea_set_password_mode(ta, true);
    else
        lv_textarea_set_password_mode(ta, false);

    lv_obj_add_event_cb(ta, pwd_event_handle, LV_EVENT_KEY, "Password");

    // 8 密码明文按钮
    lv_obj_t *password_plain_obj = lv_checkbox_create(wifi_conn_obj);
    lv_obj_align_to(password_plain_obj, ta, LV_ALIGN_OUT_BOTTOM_LEFT, 0,
                    WIFI_CONNE_WIDGET_ALIGN_TOP6);
    if (WIFI_CONN_WIDGET_SET_H) lv_obj_set_height(password_plain_obj, lv_pct(8));
    lv_obj_set_style_pad_all(password_plain_obj, 2, LV_PART_MAIN);
    lv_obj_set_style_text_font(password_plain_obj, &GENERAL_FONT_NORMAL, LV_PART_MAIN);
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
    lv_checkbox_set_text(password_plain_obj, lv_get_string(STR_WIFI_SHOW_PWD));
    lv_obj_add_event_cb(password_plain_obj, checkout_event_handle, LV_EVENT_KEY, ta);
    if (!pwd_is_hidden) {
        lv_obj_add_state(password_plain_obj, LV_STATE_CHECKED);
    }

    // 9 连接取消按钮
    static const char *strs[3];
    strs[0] = (char *)lv_get_string(STR_WIFI_CANCEL);
    strs[1] = (char *)lv_get_string(STR_WIFI_CONN);
    strs[2] = "";

    lv_obj_t *choose_obj = lv_btnmatrix_create(wifi_conn_obj);
    lv_obj_set_size(choose_obj, lv_pct(100), lv_pct(15));
    lv_obj_align(choose_obj, LV_ALIGN_BOTTOM_MID, 0, -(WIFI_CONNE_WIDGET_ALIGN_TOP7));
    lv_btnmatrix_set_map(choose_obj, strs);
    lv_obj_set_style_text_font(choose_obj, &GENERAL_FONT_MID, LV_PART_MAIN);
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
    lv_obj_add_event_cb(choose_obj, btns_event_handle, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(choose_obj, btns_event_handle, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(choose_obj, btns_event_handle, LV_EVENT_DEFOCUSED, NULL);

    // 10 键盘
    create_keypad_widget();
    lv_group_focus_obj(ta);

    WIFI_DBG("%s end\n", __func__);

    lv_obj_update_layout(lv_pro_connect_wifi_activity);
}
#endif /* ENABLE_WIFI */