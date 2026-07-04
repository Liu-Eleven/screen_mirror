/*
 * lv_pro_bt_conn_activity.c
 *
 *  Created on: 2024/09/12
 *      Author: JASON
 */

#include "lv_pro_bt_conn_activity.h"

#include "lv_pro_bt_activity.h"
#include "lv_pro_bt_common.h"

#if ENABLE_BT

#if defined(LV_USE_RESOLUTION_1080P)
#define BT_DISCONN_WIDGET_SET_H 1

#define BT_DISCONN_WIGGET_WIDHT_PCT 25
#define BT_DISCONN_WIGGET_HEIHTH_PCT 35

#define BT_DISCONN_WIGGET_ALIGN_TOP1 0
#define BT_DISCONN_WIGGET_ALIGN_TOP2 0
#define BT_DISCONN_WIGGET_ALIGN_TOP3 0
#define BT_DISCONN_WIGGET_ALIGN_TOP4 10
#else
#define BT_DISCONN_WIDGET_SET_H 1

#define BT_DISCONN_WIGGET_WIDHT_PCT 25
#define BT_DISCONN_WIGGET_HEIHTH_PCT 40

#define BT_DISCONN_WIGGET_ALIGN_TOP1 0
#define BT_DISCONN_WIGGET_ALIGN_TOP2 0
#define BT_DISCONN_WIGGET_ALIGN_TOP3 0
#define BT_DISCONN_WIGGET_ALIGN_TOP4 0
#endif

static lv_obj_t *lv_pro_bt_selectbox_activity;
static lv_obj_t *bt_selectbox_obj;

static lv_group_t *bt_activity_g;
static lv_group_t *bt_selectbox_default_g;

static int group_user_focus_check_cb(lv_group_t *group) { return 1; }

void lv_pro_bt_disconn_deinit(void)
{
    BT_DBG("deinit\n");
    if (bt_selectbox_default_g) {
        lv_group_remove_all_objs(bt_selectbox_default_g);
        lv_group_del(bt_selectbox_default_g);
        bt_selectbox_default_g = NULL;
    }
    if (lv_pro_bt_selectbox_activity) {
        lv_obj_del(lv_pro_bt_selectbox_activity);
        lv_pro_bt_selectbox_activity = NULL;
    }
    lv_group_set_default(NULL);
}

static void btns_event_handle(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    const uint32_t *key_param = lv_event_get_param(e);
    uint32_t key = (key_param != NULL) ? *key_param : 0;
    lv_btnmatrix_t *btnms = (lv_btnmatrix_t *)lv_event_get_target(e);

    lv_pro_set_btn_style6_t *btn6 = (lv_pro_set_btn_style6_t *)lv_event_get_user_data(e);

    if (code == LV_EVENT_KEY) {
        if (key == LV_KEY_LEFT) {
            lv_btnmatrix_set_selected_btn(btnms, 0);
            lv_btnmatrix_clear_btn_ctrl_all(btnms, LV_BTNMATRIX_CTRL_CHECKED);
            lv_btnmatrix_set_btn_ctrl(btnms, 0, LV_BTNMATRIX_CTRL_CHECKED);
        } else if (key == LV_KEY_RIGHT) {
            lv_btnmatrix_set_selected_btn(btnms, 1);
            lv_btnmatrix_clear_btn_ctrl_all(btnms, LV_BTNMATRIX_CTRL_CHECKED);
            lv_btnmatrix_set_btn_ctrl(btnms, 1, LV_BTNMATRIX_CTRL_CHECKED);
        } else if (!is_global_key_go_new_channel(key) || key == LV_KEY_BACK) {
            lv_pro_bt_disconn_deinit();
            lv_pro_set_bt_state(LVGL_BT_STATE_IDLE);
            set_current_ui_group(bt_activity_g);
            if (key != LV_KEY_BACK) switch_page(PAGE_HOME);
            return;
        } else if (key == LV_KEY_ENTER) {
            const char *txt = lv_btnmatrix_get_btn_text(btnms, btnms->btn_id_sel);
            bool is_disconnecting = !strcmp(txt, (char *)lv_get_string(STR_BT_MAKE_DISCONN));
            // bool is_connecting = !strcmp(txt, (char *)lv_get_string(STR_BT_MAKE_CONN));

            if ((btnms->btn_id_sel == 0 || btnms->btn_id_sel == 1)) {
                if (btnms->btn_id_sel == 0) {
                    lv_pro_bt_msg_enqueue(MSG_TYPE_BT_DELETED_PAIRDEV, btn6, false);
                } else {
                    lv_pro_bt_msg_enqueue(
                        is_disconnecting ? MSG_TYPE_BT_DISCONN_PAIRDEV : MSG_TYPE_BT_CONN_PAIRDEV,
                        btn6, false);
                }
                lv_btnmatrix_clear_btn_ctrl_all(btnms, LV_BTNMATRIX_CTRL_CHECKED);
                lv_pro_bt_disconn_deinit();
                lv_pro_set_bt_state(LVGL_BT_STATE_IDLE);
                set_current_ui_group(bt_activity_g);
            }
        }
    }
}

void lv_pro_selectbox_bt(void *obj, int state_enum)
{
    BT_DBG("init\n");
    char select_name[249];
    char select_mac[19];

    lv_pro_set_bt_state(LVGL_BT_STATE_BUSY);

    lv_pro_set_btn_style6_t *btn = (lv_pro_set_btn_style6_t *)obj;

    // 创建BT半透明顶页
    lv_pro_bt_selectbox_activity = lv_obj_create(lv_layer_top());
    lv_obj_set_size(lv_pro_bt_selectbox_activity, lv_pct(100), lv_pct(100));
    lv_obj_set_style_border_width(lv_pro_bt_selectbox_activity, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(lv_pro_bt_selectbox_activity, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(lv_pro_bt_selectbox_activity, LV_OPA_10, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(lv_pro_bt_selectbox_activity, LV_SCROLLBAR_MODE_OFF);

    // 创建连接框
    bt_selectbox_obj = lv_obj_create(lv_pro_bt_selectbox_activity);
    lv_obj_set_size(bt_selectbox_obj, lv_pct(BT_DISCONN_WIGGET_WIDHT_PCT),
                    lv_pct(BT_DISCONN_WIGGET_HEIHTH_PCT));
    lv_obj_align(bt_selectbox_obj, LV_ALIGN_CENTER, 0, -10);
    lv_obj_set_style_border_width(bt_selectbox_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(bt_selectbox_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(bt_selectbox_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(bt_selectbox_obj, 3, LV_PART_MAIN);
    lv_obj_set_style_radius(bt_selectbox_obj, 5, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bt_selectbox_obj, lv_color_make(32, 32, 32), LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(bt_selectbox_obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(bt_selectbox_obj, LV_OBJ_FLAG_USER_2);  // user2 为纵向

    bt_activity_g = lv_group_get_default();
    bt_selectbox_default_g = lv_group_create();
    set_current_ui_group(bt_selectbox_default_g);
    lv_group_set_user_focus_check_cb(bt_selectbox_default_g, group_user_focus_check_cb);

    // 1 蓝牙名称标题
    lv_obj_t *btname_title_obj = lv_label_create(bt_selectbox_obj);
    lv_obj_align(btname_title_obj, LV_ALIGN_TOP_LEFT, 10, 10);
    if (BT_DISCONN_WIDGET_SET_H) lv_obj_set_height(btname_title_obj, lv_pct(20));
    lv_obj_set_style_pad_ver(btname_title_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_text_font(btname_title_obj, &GENERAL_FONT_BIG, LV_PART_MAIN);
    lv_label_set_recolor(btname_title_obj, true);
    lv_label_set_text_fmt(btname_title_obj, "#ffffff %s #", lv_get_string(STR_BT_DEVICE));

    // 2 蓝牙名称
    lv_obj_t *btname_obj = lv_label_create(bt_selectbox_obj);
    lv_obj_align_to(btname_obj, btname_title_obj, LV_ALIGN_OUT_BOTTOM_LEFT, 10,
                    BT_DISCONN_WIGGET_ALIGN_TOP1);
    if (BT_DISCONN_WIDGET_SET_H) lv_obj_set_height(btname_obj, lv_pct(15));
    lv_obj_set_style_pad_ver(btname_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_text_font(btname_obj, &GENERAL_FONT_BIG, LV_PART_MAIN);
    lv_obj_set_style_text_color(btname_obj, lv_color_hex(0xDCDCDC), LV_PART_MAIN);
    lv_label_set_text(btname_obj, lv_pro_set_btn_style6_get_name_str(btn));

    // 3 蓝牙MAC地址标题
    lv_obj_t *btmac_title_obj = lv_label_create(bt_selectbox_obj);
    lv_obj_align_to(btmac_title_obj, btname_obj, LV_ALIGN_OUT_BOTTOM_LEFT, -10,
                    BT_DISCONN_WIGGET_ALIGN_TOP2);
    if (BT_DISCONN_WIDGET_SET_H) lv_obj_set_height(btmac_title_obj, lv_pct(20));
    lv_obj_set_style_pad_ver(btmac_title_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_text_font(btmac_title_obj, &GENERAL_FONT_BIG, LV_PART_MAIN);
    lv_label_set_recolor(btmac_title_obj, true);
    lv_label_set_text_fmt(btmac_title_obj, "#ffffff %s #", lv_get_string(STR_BT_MAC));

    // 4 蓝牙MAC地址
    lv_obj_t *btmac_obj = lv_label_create(bt_selectbox_obj);
    lv_obj_align_to(btmac_obj, btmac_title_obj, LV_ALIGN_OUT_BOTTOM_LEFT, 10,
                    BT_DISCONN_WIGGET_ALIGN_TOP3);
    if (BT_DISCONN_WIDGET_SET_H) lv_obj_set_height(btmac_obj, lv_pct(15));
    lv_obj_set_style_pad_ver(btmac_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_text_font(btmac_obj, &GENERAL_FONT_BIG, LV_PART_MAIN);
    lv_obj_set_style_text_color(btmac_obj, lv_color_hex(0xDCDCDC), LV_PART_MAIN);
    lv_label_set_text(btmac_obj, lv_pro_set_btn_style6_get_mac_addr_str(btn));

    // 9 连接取消按钮
    static const char *strs[3];
    strs[0] = (char *)lv_get_string(STR_BT_DELETE);
    if (state_enum == LVGL_BT_STATE_CONNECTING) {
        strs[1] = (char *)lv_get_string(STR_BT_MAKE_CONN);
    } else if (state_enum == LVGL_BT_STATE_DISCONNECTING) {
        strs[1] = (char *)lv_get_string(STR_BT_MAKE_DISCONN);
    }
    strs[2] = "";

    lv_obj_t *choose_obj = lv_btnmatrix_create(bt_selectbox_obj);
    lv_obj_set_size(choose_obj, lv_pct(100), lv_pct(25));
    lv_obj_align(choose_obj, LV_ALIGN_BOTTOM_MID, 0, -(BT_DISCONN_WIGGET_ALIGN_TOP4));
    lv_btnmatrix_set_map(choose_obj, strs);
    lv_obj_set_style_text_font(choose_obj, &GENERAL_FONT_BIG, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(choose_obj, lv_color_make(32, 32, 32), LV_PART_MAIN);
    lv_obj_set_style_bg_color(choose_obj, lv_color_make(26, 26, 26), LV_PART_ITEMS);
    lv_obj_set_style_bg_color(choose_obj, lv_color_hex(0x3dcf07), LV_PART_ITEMS | LV_STATE_CHECKED);

    lv_obj_set_style_text_color(choose_obj, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_color(choose_obj, lv_color_white(), LV_PART_ITEMS);

    lv_obj_set_style_border_width(choose_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(choose_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(choose_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(choose_obj, 0, LV_PART_ITEMS);
    lv_obj_set_style_pad_all(choose_obj, 0, LV_PART_ITEMS);
    lv_obj_set_style_radius(choose_obj, 0, LV_PART_ITEMS);
    lv_obj_set_style_pad_row(choose_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_width(choose_obj, 4, LV_PART_ITEMS);
    lv_obj_set_style_outline_color(choose_obj, lv_color_hex(0xFFFFFFF), LV_PART_ITEMS);
    lv_obj_set_style_outline_pad(choose_obj, 0, LV_PART_ITEMS);
    lv_obj_set_style_outline_opa(choose_obj, LV_OPA_0, LV_PART_ITEMS);
    lv_obj_set_style_outline_opa(choose_obj, LV_OPA_100, LV_PART_ITEMS | LV_STATE_FOCUS_KEY);

    lv_obj_set_style_bg_opa(choose_obj, LV_OPA_0, LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(choose_obj, LV_OPA_100, LV_PART_ITEMS | LV_STATE_CHECKED);

    lv_btnmatrix_set_btn_ctrl_all(choose_obj, LV_BTNMATRIX_CTRL_CHECKABLE);
    lv_btnmatrix_set_one_checked(choose_obj, true);
    lv_btnmatrix_set_selected_btn(choose_obj, 1);
    lv_btnmatrix_set_btn_ctrl(choose_obj, 1, LV_BTNMATRIX_CTRL_CHECKED);

    lv_obj_add_event_cb(choose_obj, btns_event_handle, LV_EVENT_KEY, btn);

    lv_obj_update_layout(lv_pro_bt_selectbox_activity);
}
#endif /* ENABLE_BT */