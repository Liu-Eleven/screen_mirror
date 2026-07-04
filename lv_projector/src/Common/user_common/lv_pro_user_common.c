/*
 * lv_pro_user_common.c
 *
 */

#include "lv_pro_user_common.h"

/**************************消息窗口*********************************/
static lv_timer_t *msgbox_timer;

void lv_pro_msgbox_del_handle(lv_timer_t *timer)
{
    lv_timer_t *active_timer = timer ? timer : msgbox_timer;
    if (!active_timer) return;

    lv_obj_t *message_box = (lv_obj_t *)active_timer->user_data;
    if (lv_obj_is_valid(message_box)) {
        lv_obj_del(message_box);
    }

    lv_timer_del(active_timer);
    msgbox_timer = NULL;
}

lv_obj_t *lv_pro_create_message_box(char *str, uint32_t period)
{
    lv_obj_t *con = lv_obj_create(lv_layer_top());
    lv_obj_set_style_text_color(con, lv_color_white(), 0);
    lv_obj_set_size(con, LV_SIZE_CONTENT, LV_PCT(10));
    lv_obj_center(con);
    lv_obj_set_style_bg_color(con, lv_palette_darken(LV_PALETTE_GREY, 1), 0);
    lv_obj_set_scrollbar_mode(con, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(con, LV_OPA_70, 0);
    lv_obj_set_style_border_width(con, 1, 0);
    lv_obj_set_style_border_color(con, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_style_outline_width(con, 0, 0);
    lv_obj_set_style_radius(con, 10, 0);

    lv_obj_t *label = lv_label_create(con);
    lv_obj_center(label);
    lv_label_set_text(label, str);
    lv_obj_set_style_text_font(label, &GENERAL_FONT_BIG, 0);

    msgbox_timer = lv_timer_create(lv_pro_msgbox_del_handle, period, con);
    lv_timer_set_repeat_count(msgbox_timer, 1);
    lv_timer_reset(msgbox_timer);
    return con;
}
/**************************消息窗口*********************************/

static lv_obj_t *msgActivity;
static lv_group_t *currentGroup;

int lv_pro_create_message2_box(char *str)
{
    if (msgActivity) {
        return -1;
    }

    currentGroup = lv_group_get_default();
    lv_group_focus_freeze(currentGroup, true);

    msgActivity = lv_obj_create(lv_layer_top());
    lv_obj_set_size(msgActivity, lv_pct(100), lv_pct(100));
    lv_obj_set_style_border_width(msgActivity, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(msgActivity, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(msgActivity, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(msgActivity, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *msg_obj = lv_obj_create(msgActivity);
    lv_obj_set_size(msg_obj, LV_PCT(20), LV_PCT(15));
    lv_obj_set_align(msg_obj, LV_ALIGN_CENTER);
    lv_obj_set_style_radius(msg_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(msg_obj, lv_color_hex(0x4D72E0), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(msg_obj, 255, LV_PART_MAIN);

    lv_obj_t *ui_lab = lv_label_create(msg_obj);
    lv_obj_set_width(ui_lab, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_lab, LV_SIZE_CONTENT);
    lv_obj_set_align(ui_lab, LV_ALIGN_CENTER);
    lv_label_set_long_mode(ui_lab, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_color(ui_lab, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_opa(ui_lab, 255, LV_PART_MAIN);
    lv_obj_set_style_text_font(ui_lab, &GENERAL_FONT_BIG, LV_PART_MAIN);
    lv_label_set_text(ui_lab, str);

    lv_obj_update_layout(msgActivity);

    return 0;
}

void lv_pro_del_message2_box()
{
    if (currentGroup) {
        lv_group_focus_freeze(currentGroup, false);
        currentGroup = NULL;
    }

    if (msgActivity) {
        lv_obj_del(msgActivity);
        msgActivity = NULL;
    }
}

/**************************消息窗口*********************************/

static lv_obj_t *m_obj_msg = NULL;
static lv_timer_t *msgbox_timer = NULL;

void lv_pro_msgbox_msg_clear()
{
    if (msgbox_timer) {
        lv_timer_pause(msgbox_timer);
        lv_timer_del(msgbox_timer);
    }
    msgbox_timer = NULL;

    if (m_obj_msg && lv_obj_is_valid(m_obj_msg)) {
        lv_obj_del(m_obj_msg);
    }
    m_obj_msg = NULL;
}

static void msgbox_timer_cb(lv_timer_t *t) { lv_pro_msgbox_msg_clear(); }

void lv_pro_msgbox_msg_open_on_top(void *icon, char *str_msg, uint32_t timeout)
{
    if (lv_obj_is_valid(m_obj_msg)) return;

    lv_obj_t *ui_tip_img;
    lv_obj_t *ui_tip_lab;

    if (!m_obj_msg || !lv_obj_is_valid(m_obj_msg)) {
        m_obj_msg = lv_obj_create(lv_layer_top());
        lv_obj_set_size(m_obj_msg, LV_PCT(44), LV_PCT(35));
        lv_obj_set_align(m_obj_msg, LV_ALIGN_CENTER);
        lv_obj_clear_flag(m_obj_msg, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(m_obj_msg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(m_obj_msg, lv_color_hex(0x4D72E0),
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(m_obj_msg, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

        // ui_tip_img
        ui_tip_img = lv_img_create(m_obj_msg);
        lv_obj_set_width(ui_tip_img, LV_SIZE_CONTENT);
        lv_obj_set_height(ui_tip_img, LV_SIZE_CONTENT);
        lv_obj_set_x(ui_tip_img, 0);
        lv_obj_set_y(ui_tip_img, 0);
        lv_img_set_src(ui_tip_img, icon);
        lv_obj_set_align(ui_tip_img, LV_ALIGN_CENTER);
        lv_obj_add_flag(ui_tip_img, LV_OBJ_FLAG_ADV_HITTEST);
        lv_obj_clear_flag(ui_tip_img, LV_OBJ_FLAG_SCROLLABLE);

        // ui_tip_lab
        ui_tip_lab = lv_label_create(m_obj_msg);
        lv_obj_set_width(ui_tip_lab, LV_SIZE_CONTENT);
        lv_obj_set_height(ui_tip_lab, LV_SIZE_CONTENT);
        lv_obj_set_x(ui_tip_lab, 0);
        lv_obj_set_y(ui_tip_lab, 0);
        lv_obj_set_align(ui_tip_lab, LV_ALIGN_BOTTOM_MID);
        lv_label_set_long_mode(ui_tip_lab, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_style_text_color(ui_tip_lab, lv_color_hex(0xFFFFFF),
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(ui_tip_lab, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(ui_tip_lab, &GENERAL_FONT_BIG, LV_PART_MAIN | LV_STATE_DEFAULT);

        if (0 != timeout && 0xFFFFFFFF != timeout) {
            msgbox_timer = lv_timer_create(msgbox_timer_cb, timeout, NULL);
        }
    }
    if (msgbox_timer) {
        lv_timer_reset(msgbox_timer);
        lv_timer_set_period(msgbox_timer, timeout);
    }

    lv_label_set_text(ui_tip_lab, str_msg);
    lv_obj_set_style_text_font(ui_tip_lab, &GENERAL_FONT_MID, 0);
}