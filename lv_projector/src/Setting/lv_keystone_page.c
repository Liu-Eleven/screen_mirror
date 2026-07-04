#include "lv_pro_setting.h"
#include <stdlib.h>
#include "../widget/lv_pro_res.h"
#include "../widget/lv_pro_setting_item.h"
#include "../lv_pro_launcher.h"
#include "../Common/language/string/lv_string_id.h"
#include "lv_common.h"
#include "sys_param.h"

#if ENABLE_KSC
#include "ksc/lv_ksc.h"
#endif

#define KEYSTONE_TL_ITEM    (0) //KEYSTONE_TOP_LEFT_ID
#define KEYSTONE_TR_ITEM    (1) //KEYSTONE_TOP_RIGHT_ID
#define KEYSTONE_BR_ITEM    (2) //KEYSTONE_BOTTOM_RIGHT_ID
#define KEYSTONE_BL_ITEM    (3) //KEYSTONE_BOTTOM_RIGHT_ID

#define STYLE_POS_X     	(0)
#define STYLE_POS_Y     	(1)
#define KEYSTONE_STEP_ADD	(2)
#define KEYSTONE_STEP_DEL	(-2)

/*
*	keystone setting
*/
lv_obj_t *sub_keystone_page;
lv_obj_t *keystone_obj;
lv_group_t *Setting_keystone_group;

lv_obj_t *auto_keystone_obj;
lv_obj_t *manual_keystone_obj;
lv_obj_t *manual_keystone_reset_obj;
lv_obj_t *keystone_sw;
lv_obj_t *keystone_sw_label;

static uint8_t keystone_item_id;

void update_keystone_label(void)
{
    lv_label_set_text(lv_obj_get_child(auto_keystone_obj, 0), lv_get_string(STR_AUTO_KEYSTONE));
    lv_label_set_text(lv_obj_get_child(manual_keystone_obj, 0), lv_get_string(STR_MANUAL_KEYSTONE));
    lv_label_set_text(lv_obj_get_child(manual_keystone_reset_obj, 0), lv_get_string(STR_MANUAL_KEYSTONE));
    lv_label_set_text(lv_obj_get_child(manual_keystone_reset_obj, 1), lv_get_string(STR_RESET));

    if (lv_get_sys_param(P_AUTO_KEYSTONE))
        lv_label_set_text(keystone_sw_label, lv_get_string(STR_ON));
    else
        lv_label_set_text(keystone_sw_label, lv_get_string(STR_OFF));
}

void update_auto_keystone(void)
{

}

static void reset_keystone_pos(void)
{
#if ENABLE_KSC
    lv_set_sys_param(P_KEYSTONE_TOP_LEFT_X, 0);
    lv_set_sys_param(P_KEYSTONE_TOP_LEFT_Y, 0);
    set_keystone_pos(KEYSTONE_TL_ITEM, 0, 0);

    lv_set_sys_param(P_KEYSTONE_TOP_RIGHT_X, 0);
    lv_set_sys_param(P_KEYSTONE_TOP_RIGHT_Y, 0);
    set_keystone_pos(KEYSTONE_TR_ITEM, 0, 0);

    lv_set_sys_param(P_KEYSTONE_BOTTOM_LEFT_X, 0);
    lv_set_sys_param(P_KEYSTONE_BOTTOM_LEFT_Y, 0);
    set_keystone_pos(KEYSTONE_BL_ITEM, 0, 0);

    lv_set_sys_param(P_KEYSTONE_BOTTOM_RIGHT_X, 0);
    lv_set_sys_param(P_KEYSTONE_BOTTOM_RIGHT_Y, 0);
    set_keystone_pos(KEYSTONE_BR_ITEM, 0, 0);
#endif
}

static lv_obj_t * create_keystone_item(lv_obj_t * obj, const char * src)
{
    lv_obj_t *img_obj;

    lv_obj_t * item_obj = lv_obj_create(obj);
    lv_obj_set_size(item_obj, 200, 200);
    lv_obj_set_style_bg_opa(item_obj, LV_OPA_0, 0);
    lv_obj_set_style_border_width(item_obj, 0, 0);
    //lv_obj_set_style_outline_width(item_obj, 1, 0);
    lv_obj_set_style_outline_width(item_obj, 0, 0);
    lv_obj_clear_flag(item_obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_outline_color(item_obj, lv_color_hex(0xFFFFFFF), 0);

    img_obj = lv_img_create(item_obj);
    lv_obj_align(img_obj, LV_ALIGN_CENTER, 0, 0);
    lv_img_set_src(img_obj, src);

    char *png_path[] = {"A:/usr/share/lv_projector/keystone/dir_top.png",
                        "A:/usr/share/lv_projector/keystone/dir_bottom.png",
                        "A:/usr/share/lv_projector/keystone/dir_left.png",
                        "A:/usr/share/lv_projector/keystone/dir_right.png",
                        "A:/usr/share/lv_projector/keystone/checkbox_select.png"};

    img_obj = lv_img_create(item_obj);
    lv_obj_align(img_obj, LV_ALIGN_TOP_MID, 0, -20);
    lv_img_set_zoom(img_obj, 150);
    lv_img_set_src(img_obj, png_path[0]);
    lv_obj_add_flag(img_obj, LV_OBJ_FLAG_HIDDEN);

    img_obj = lv_img_create(item_obj);
    lv_obj_align(img_obj, LV_ALIGN_BOTTOM_MID, 0, 20);
    lv_img_set_zoom(img_obj, 150);
    lv_img_set_src(img_obj, png_path[1]);
    lv_obj_add_flag(img_obj, LV_OBJ_FLAG_HIDDEN);

    img_obj = lv_img_create(item_obj);
    lv_obj_align(img_obj, LV_ALIGN_LEFT_MID, -20, 0);
    lv_img_set_zoom(img_obj, 150);
    lv_img_set_src(img_obj, png_path[2]);
    lv_obj_add_flag(img_obj, LV_OBJ_FLAG_HIDDEN);

    img_obj = lv_img_create(item_obj);
    lv_obj_align(img_obj, LV_ALIGN_RIGHT_MID, 20, 0);
    lv_img_set_zoom(img_obj, 150);
    lv_img_set_src(img_obj, png_path[3]);
    lv_obj_add_flag(img_obj, LV_OBJ_FLAG_HIDDEN);

    img_obj = lv_img_create(item_obj);
    lv_obj_align(img_obj, LV_ALIGN_CENTER, 0, 0);
    lv_img_set_src(img_obj, png_path[4]);
    lv_obj_add_flag(img_obj, LV_OBJ_FLAG_HIDDEN);

    return item_obj;
}

static void update_keystone_all_pos(lv_obj_t *obj)
{
    lv_obj_t *text_lable;
    char str_pos[32] = {0};

    text_lable = lv_obj_get_child(lv_obj_get_child(obj, 2), 0);
    sprintf(str_pos, "%d,%d", lv_get_sys_param(P_KEYSTONE_TOP_LEFT_X), lv_get_sys_param(P_KEYSTONE_TOP_LEFT_Y));
    lv_label_set_text(text_lable, str_pos);

    text_lable = lv_obj_get_child(lv_obj_get_child(obj, 2), 1);
    sprintf(str_pos, "%d,%d", lv_get_sys_param(P_KEYSTONE_TOP_RIGHT_X), lv_get_sys_param(P_KEYSTONE_TOP_RIGHT_Y));
    lv_label_set_text(text_lable, str_pos);

    text_lable = lv_obj_get_child(lv_obj_get_child(obj, 2), 2);
    sprintf(str_pos, "%d,%d", lv_get_sys_param(P_KEYSTONE_BOTTOM_RIGHT_X), lv_get_sys_param(P_KEYSTONE_BOTTOM_RIGHT_Y));
    lv_label_set_text(text_lable, str_pos);

    text_lable = lv_obj_get_child(lv_obj_get_child(obj, 2), 3);
    sprintf(str_pos, "%d,%d", lv_get_sys_param(P_KEYSTONE_BOTTOM_LEFT_X), lv_get_sys_param(P_KEYSTONE_BOTTOM_LEFT_Y));
    lv_label_set_text(text_lable, str_pos);
}

static void update_keystone_pos(lv_obj_t *obj, uint8_t style, int value)
{
#if ENABLE_KSC
    lv_obj_t *text_lable;
    uint32_t pos_x_id;
    uint32_t pos_y_id;
    uint16_t pos_x;
    uint16_t pos_y;
    char str_pos[32] = {0};

    switch (keystone_item_id) {
        case KEYSTONE_TL_ITEM:
            text_lable = lv_obj_get_child(lv_obj_get_child(obj, 2), 0);
            pos_x_id = P_KEYSTONE_TOP_LEFT_X;
            pos_y_id = P_KEYSTONE_TOP_LEFT_Y;
            break;
        case KEYSTONE_TR_ITEM:
            text_lable = lv_obj_get_child(lv_obj_get_child(obj, 2), 1);
            pos_x_id = P_KEYSTONE_TOP_RIGHT_X;
            pos_y_id = P_KEYSTONE_TOP_RIGHT_Y;
            break;
        case KEYSTONE_BR_ITEM:
            text_lable = lv_obj_get_child(lv_obj_get_child(obj, 2), 2);
            pos_x_id = P_KEYSTONE_BOTTOM_RIGHT_X;
            pos_y_id = P_KEYSTONE_BOTTOM_RIGHT_Y;
            break;
        case KEYSTONE_BL_ITEM:
            text_lable = lv_obj_get_child(lv_obj_get_child(obj, 2), 3);
            pos_x_id = P_KEYSTONE_BOTTOM_LEFT_X;
            pos_y_id = P_KEYSTONE_BOTTOM_LEFT_Y;
            break;
        default:
            text_lable = lv_obj_get_child(lv_obj_get_child(obj, 2), 0);
            pos_x_id = P_KEYSTONE_TOP_LEFT_X;
            pos_y_id = P_KEYSTONE_TOP_LEFT_Y;
    }

    pos_x = lv_get_sys_param(pos_x_id);
    pos_y = lv_get_sys_param(pos_y_id);
    /* x */
    if (style == STYLE_POS_X) {
        if ((pos_x + value) < 0 || (pos_x + value) > LIMIT_KSC_X_MAX_VALUE)
            return;
        pos_x = pos_x + value;
        lv_set_sys_param(pos_x_id, pos_x);
    } else {
        if (pos_y + value < 0 || (pos_y + value) > LIMIT_KSC_Y_MAX_VALUE)
            return;
        pos_y = pos_y + value;
        lv_set_sys_param(pos_y_id, pos_y);
    }

    sprintf(str_pos, "%d,%d", pos_x, pos_y);
    lv_label_set_text(text_lable, str_pos);
    set_keystone_pos(keystone_item_id, pos_x, pos_y);
#endif
}

static void display_keystone_pos_icon(lv_obj_t *obj)
{

    lv_obj_clear_flag(lv_obj_get_child(obj, 1), LV_OBJ_FLAG_HIDDEN);    // up
    lv_obj_clear_flag(lv_obj_get_child(obj, 2), LV_OBJ_FLAG_HIDDEN);    // down
    lv_obj_clear_flag(lv_obj_get_child(obj, 3), LV_OBJ_FLAG_HIDDEN);    // left
    lv_obj_clear_flag(lv_obj_get_child(obj, 4), LV_OBJ_FLAG_HIDDEN);    // rigth
    lv_obj_clear_flag(lv_obj_get_child(obj, 5), LV_OBJ_FLAG_HIDDEN);    // center
    switch (keystone_item_id) {
        case KEYSTONE_TL_ITEM:
            if (lv_get_sys_param(P_KEYSTONE_TOP_LEFT_X) == 0)
                lv_obj_add_flag(lv_obj_get_child(obj, 3), LV_OBJ_FLAG_HIDDEN);

            if (lv_get_sys_param(P_KEYSTONE_TOP_LEFT_Y) == 0)
                lv_obj_add_flag(lv_obj_get_child(obj, 1), LV_OBJ_FLAG_HIDDEN);
            break;
        case KEYSTONE_TR_ITEM:
            if (lv_get_sys_param(P_KEYSTONE_TOP_RIGHT_X) == 0)
                lv_obj_add_flag(lv_obj_get_child(obj, 4), LV_OBJ_FLAG_HIDDEN);

            if (lv_get_sys_param(P_KEYSTONE_TOP_RIGHT_Y) == 0)
                lv_obj_add_flag(lv_obj_get_child(obj, 1), LV_OBJ_FLAG_HIDDEN);
            break;
        case KEYSTONE_BR_ITEM:
            if (lv_get_sys_param(P_KEYSTONE_BOTTOM_RIGHT_X) == 0)
                lv_obj_add_flag(lv_obj_get_child(obj, 4), LV_OBJ_FLAG_HIDDEN);

            if (lv_get_sys_param(P_KEYSTONE_BOTTOM_RIGHT_Y) == 0)
                lv_obj_add_flag(lv_obj_get_child(obj, 2), LV_OBJ_FLAG_HIDDEN);
            break;
        case KEYSTONE_BL_ITEM:
            if (lv_get_sys_param(P_KEYSTONE_BOTTOM_LEFT_X) == 0)
                lv_obj_add_flag(lv_obj_get_child(obj, 3), LV_OBJ_FLAG_HIDDEN);

            if (lv_get_sys_param(P_KEYSTONE_BOTTOM_LEFT_Y) == 0)
                lv_obj_add_flag(lv_obj_get_child(obj, 2), LV_OBJ_FLAG_HIDDEN);
            break;
        default:
            break;
    }
}

static void keystone_item_event(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t *key = lv_event_get_param(e);
    lv_obj_t * cur_obj = lv_event_get_target(e);
    lv_obj_t * next_obj;

    if (code == LV_EVENT_KEY) {
        if (*key == LV_KEY_ENTER) {
            lv_obj_add_flag(lv_obj_get_child(cur_obj, 1), LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(lv_obj_get_child(cur_obj, 2), LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(lv_obj_get_child(cur_obj, 3), LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(lv_obj_get_child(cur_obj, 4), LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(lv_obj_get_child(cur_obj, 5), LV_OBJ_FLAG_HIDDEN);

            keystone_item_id = keystone_item_id + 1;
            if (keystone_item_id > 3)
                keystone_item_id = 0;

            next_obj = lv_obj_get_child(lv_obj_get_parent(cur_obj), 3 + keystone_item_id);
            lv_group_only_add_one_obj(lv_obj_get_group(cur_obj), next_obj);
            display_keystone_pos_icon(next_obj);
        } else if (*key == LV_KEY_BACK || !is_global_key_go_new_channel(*key)) {
            lv_group_t *g = lv_obj_get_group(cur_obj);
            lv_group_remove_all_objs(g);
            lv_obj_del(lv_obj_get_parent(cur_obj));
            lv_group_del(g);

            if (*key == LV_KEY_BACK)
                set_current_ui_group(Setting_keystone_group);
            else
                lv_obj_add_flag(Setting_activity, LV_OBJ_FLAG_HIDDEN);
        } else if (*key == LV_KEY_UP) {
            if (keystone_item_id == KEYSTONE_TL_ITEM || keystone_item_id == KEYSTONE_TR_ITEM)
                update_keystone_pos(lv_obj_get_parent(cur_obj), STYLE_POS_Y, KEYSTONE_STEP_DEL);
            else if (keystone_item_id == KEYSTONE_BL_ITEM || keystone_item_id == KEYSTONE_BR_ITEM)
                update_keystone_pos(lv_obj_get_parent(cur_obj), STYLE_POS_Y, KEYSTONE_STEP_ADD);

            display_keystone_pos_icon(cur_obj);
        } else if (*key == LV_KEY_DOWN) {
            if (keystone_item_id == KEYSTONE_TL_ITEM || keystone_item_id == KEYSTONE_TR_ITEM)
                update_keystone_pos(lv_obj_get_parent(cur_obj), STYLE_POS_Y, KEYSTONE_STEP_ADD);
            else if (keystone_item_id == KEYSTONE_BL_ITEM || keystone_item_id == KEYSTONE_BR_ITEM)
                update_keystone_pos(lv_obj_get_parent(cur_obj), STYLE_POS_Y, KEYSTONE_STEP_DEL);

            display_keystone_pos_icon(cur_obj);
        } else if (*key == LV_KEY_LEFT) {
            if (keystone_item_id == KEYSTONE_TL_ITEM || keystone_item_id == KEYSTONE_BL_ITEM)
                update_keystone_pos(lv_obj_get_parent(cur_obj), STYLE_POS_X, KEYSTONE_STEP_DEL);
            else if (keystone_item_id == KEYSTONE_TR_ITEM || keystone_item_id == KEYSTONE_BR_ITEM)
                update_keystone_pos(lv_obj_get_parent(cur_obj), STYLE_POS_X, KEYSTONE_STEP_ADD);

            display_keystone_pos_icon(cur_obj);
        } else if (*key == LV_KEY_RIGHT) {
            if (keystone_item_id == KEYSTONE_TL_ITEM || keystone_item_id == KEYSTONE_BL_ITEM)
                update_keystone_pos(lv_obj_get_parent(cur_obj), STYLE_POS_X, KEYSTONE_STEP_ADD);
            else if (keystone_item_id == KEYSTONE_TR_ITEM || keystone_item_id == KEYSTONE_BR_ITEM)
                update_keystone_pos(lv_obj_get_parent(cur_obj), STYLE_POS_X, KEYSTONE_STEP_DEL);

            display_keystone_pos_icon(cur_obj);
        }
    }
}

static void create_keystone_ui(void)
{
    lv_obj_t * img_obj;
    lv_obj_t * item_obj;
    lv_obj_t * text_lable;
    lv_group_t * keystone_item_group = lv_group_create();

    lv_obj_t * manual_keystone_page = lv_obj_create(lv_layer_top());
    lv_obj_set_size(manual_keystone_page, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(manual_keystone_page, COLOR_BLUE, 0);
    lv_obj_set_style_bg_opa(manual_keystone_page, LV_OPA_100, 0);
    lv_obj_clear_flag(manual_keystone_page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(manual_keystone_page, 0, 0);
    lv_obj_set_style_outline_width(manual_keystone_page, 0, 0);

    lv_obj_t * title_lable = lv_label_create(manual_keystone_page);
    lv_obj_set_style_text_font(title_lable, &GENERAL_FONT_BIG, 0);
    lv_obj_add_style(title_lable, &lv_pro_res.label_white, 0);
    lv_label_set_recolor(title_lable, true);
    lv_label_set_text_fmt(title_lable, "#ffffff %s #", lv_get_string(STR_KEYSTONE_INFO));
    lv_obj_align(title_lable, LV_ALIGN_BOTTOM_MID, 0, 0);

    char *png_path[] = {"A:/usr/share/lv_projector/keystone/correction.png",
                        "A:/usr/share/lv_projector/keystone/checkbox_normal.png"};

    img_obj = lv_img_create(manual_keystone_page);
    lv_obj_align(img_obj, LV_ALIGN_CENTER, 0, 0);
    lv_img_set_src(img_obj, png_path[0]);

    lv_obj_t * frame_obj = lv_obj_create(manual_keystone_page);
    lv_obj_align(frame_obj, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(frame_obj, LV_PCT(85), LV_PCT(75));
    lv_obj_set_style_bg_opa(frame_obj, LV_OPA_0, 0);
    lv_obj_set_style_border_width(frame_obj, 0, 0);
    lv_obj_set_style_outline_width(frame_obj, 4, 0);
    lv_obj_set_style_outline_color(frame_obj, lv_color_hex(0xFFFFFFF), 0);

    text_lable = lv_label_create(frame_obj);
    lv_obj_set_style_text_font(text_lable, &GENERAL_FONT_MID, 0);
    lv_obj_add_style(text_lable, &lv_pro_res.label_white, 0);
    lv_label_set_recolor(text_lable, true);
    lv_label_set_text_fmt(text_lable, "#ffffff %s #", "0,0");
    lv_obj_align(text_lable, LV_ALIGN_TOP_LEFT, 10, 10);

    text_lable = lv_label_create(frame_obj);
    lv_obj_set_style_text_font(text_lable, &GENERAL_FONT_MID, 0);
    lv_obj_add_style(text_lable, &lv_pro_res.label_white, 0);
    lv_label_set_recolor(text_lable, true);
    lv_label_set_text_fmt(text_lable, "#ffffff %s #", "0,0");
    lv_obj_align(text_lable, LV_ALIGN_TOP_RIGHT, -10, 10);

    text_lable = lv_label_create(frame_obj);
    lv_obj_set_style_text_font(text_lable, &GENERAL_FONT_MID, 0);
    lv_obj_add_style(text_lable, &lv_pro_res.label_white, 0);
    lv_label_set_recolor(text_lable, true);
    lv_label_set_text_fmt(text_lable, "#ffffff %s #", "0,0");
    lv_obj_align(text_lable, LV_ALIGN_BOTTOM_RIGHT, -10, -10);

    text_lable = lv_label_create(frame_obj);
    lv_obj_set_style_text_font(text_lable, &GENERAL_FONT_MID, 0);
    lv_obj_add_style(text_lable, &lv_pro_res.label_white, 0);
    lv_label_set_recolor(text_lable, true);
    lv_label_set_text_fmt(text_lable, "#ffffff %s #", "0,0");
    lv_obj_align(text_lable, LV_ALIGN_BOTTOM_LEFT, 10, -10);

    item_obj = create_keystone_item(manual_keystone_page, png_path[1]);
    lv_obj_add_event_cb(item_obj, keystone_item_event, LV_EVENT_KEY, NULL);

    item_obj = create_keystone_item(manual_keystone_page, png_path[1]);
    lv_obj_add_event_cb(item_obj, keystone_item_event, LV_EVENT_KEY, NULL);

    item_obj = create_keystone_item(manual_keystone_page, png_path[1]);
    lv_obj_add_event_cb(item_obj, keystone_item_event, LV_EVENT_KEY, NULL);

    item_obj = create_keystone_item(manual_keystone_page, png_path[1]);
    lv_obj_add_event_cb(item_obj, keystone_item_event, LV_EVENT_KEY, NULL);

    int keystone_page_h = lv_obj_get_height(manual_keystone_page);
    int keystone_page_w = lv_obj_get_width(manual_keystone_page);
    int frame_h = lv_obj_get_height(frame_obj);
    int frame_w = lv_obj_get_width(frame_obj);

    item_obj = lv_obj_get_child(manual_keystone_page, 3);
    int item_h = lv_obj_get_height(item_obj);
    int item_w = lv_obj_get_width(item_obj);
    lv_obj_align(item_obj, LV_ALIGN_TOP_MID, -(frame_w / 2), (keystone_page_h - frame_h - item_h) / 2 - 20);

    item_obj = lv_obj_get_child(manual_keystone_page, 4);
    lv_obj_align(item_obj, LV_ALIGN_TOP_MID, (frame_w / 2), (keystone_page_h - frame_h - item_h) / 2 - 20);

    item_obj = lv_obj_get_child(manual_keystone_page, 5);
    lv_obj_align(item_obj, LV_ALIGN_BOTTOM_MID, (frame_w / 2), -((keystone_page_h - frame_h - item_h) / 2) + 20);

    item_obj = lv_obj_get_child(manual_keystone_page, 6);
    lv_obj_align(item_obj, LV_ALIGN_BOTTOM_MID, -(frame_w / 2), -((keystone_page_h - frame_h - item_h) / 2) + 20);

    keystone_item_id = 0;
    lv_group_only_add_one_obj(keystone_item_group, lv_obj_get_child(manual_keystone_page, 3));
    display_keystone_pos_icon(lv_obj_get_child(manual_keystone_page, 3));

    update_keystone_all_pos(manual_keystone_page);
    set_current_ui_group(keystone_item_group);
}

static void enable_manual_keystone_mode(int mode)
{
    if (mode) {
        lv_obj_add_state(manual_keystone_obj, LV_STATE_DISABLED);
        lv_obj_set_style_text_color(lv_obj_get_child(manual_keystone_obj, 0), lv_color_make(125,125,125), 0);

        lv_obj_add_state(manual_keystone_reset_obj, LV_STATE_DISABLED);
        lv_obj_set_style_text_color(lv_obj_get_child(manual_keystone_reset_obj, 0), lv_color_make(125,125,125), 0);
        lv_obj_set_style_text_color(lv_obj_get_child(manual_keystone_reset_obj, 1), lv_color_make(125,125,125), 0);
    } else {
        lv_obj_clear_state(manual_keystone_obj, LV_STATE_DISABLED);
        lv_obj_set_style_text_color(lv_obj_get_child(manual_keystone_obj, 0), lv_color_white(), 0);

        lv_obj_clear_state(manual_keystone_reset_obj, LV_STATE_DISABLED);
        lv_obj_set_style_text_color(lv_obj_get_child(manual_keystone_reset_obj, 0), lv_color_white(), 0);
        lv_obj_set_style_text_color(lv_obj_get_child(manual_keystone_reset_obj, 1), lv_color_white(), 0);
    }
    lv_obj_clear_state(auto_keystone_obj, LV_STATE_DISABLED);
}

static void keystone_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t *key = lv_event_get_param(e);
    lv_obj_t *target_obj = lv_event_get_target(e);
    char *user_data = lv_event_get_user_data(e);

    if (code == LV_EVENT_KEY) {
        if (*key == LV_KEY_ENTER) {
            if (target_obj == auto_keystone_obj) {
                if (lv_obj_has_state(keystone_sw, LV_STATE_CHECKED)) {
                    lv_label_set_text(keystone_sw_label, lv_get_string(STR_OFF));
                    lv_obj_clear_state(keystone_sw, LV_STATE_CHECKED);
                    lv_set_sys_param(P_AUTO_KEYSTONE, 0);
                } else {
                    lv_label_set_text(keystone_sw_label, lv_get_string(STR_ON));
                    lv_obj_add_state(keystone_sw, LV_STATE_CHECKED);
                    lv_set_sys_param(P_AUTO_KEYSTONE, 1);
                }
                enable_manual_keystone_mode(lv_get_sys_param(P_AUTO_KEYSTONE));
            } else if (target_obj == manual_keystone_obj) {
                create_keystone_ui();
            } else if (target_obj == manual_keystone_reset_obj) {
                reset_keystone_pos();
            }
        } else if (*key == LV_KEY_BACK || *key == LV_KEY_LEFT) {
            set_current_ui_group(Setting_group);
            update_osd_timer(1);
        }
    }
}

void lv_keystone_page_init(void)
{
    lv_obj_t * cont;
    lv_obj_t * section;
    lv_obj_t * menu_cont_obj;
    lv_obj_t * title_cont;
    lv_obj_t * title_lable;
    lv_obj_t * main_cont;

    /* keystone */
    sub_keystone_page = lv_menu_page_create(setting_menu, NULL);
    lv_obj_set_scrollbar_mode(sub_keystone_page, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_hor(sub_keystone_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(setting_menu), 0), 0);
    lv_menu_separator_create(sub_keystone_page);
    section = lv_menu_section_create(sub_keystone_page);
    lv_obj_set_style_bg_color(section, COLOR_BLUE, 0);
    lv_obj_set_style_pad_all(section, 10, 0);
    lv_obj_set_style_pad_row(section, 30, 0);
    lv_obj_set_size(section, lv_pct(100), lv_pct(95));

    auto_keystone_obj = create_text(section, NULL, lv_get_string(STR_AUTO_KEYSTONE), NULL);
    keystone_sw_label = lv_label_create(auto_keystone_obj);
    keystone_sw = lv_switch_create(auto_keystone_obj);
    lv_obj_set_style_text_font(keystone_sw_label, &GENERAL_FONT_MID, 0);
    lv_obj_add_style(keystone_sw_label, &lv_pro_res.label_white, 0);
    lv_group_add_obj(Setting_keystone_group, auto_keystone_obj);
    lv_obj_add_event_cb(auto_keystone_obj, keystone_event_cb, LV_EVENT_KEY, NULL);

    manual_keystone_obj = create_text(section, NULL, lv_get_string(STR_MANUAL_KEYSTONE), NULL);
    lv_group_add_obj(Setting_keystone_group, manual_keystone_obj);
    lv_obj_add_event_cb(manual_keystone_obj, keystone_event_cb, LV_EVENT_KEY, NULL);

    manual_keystone_reset_obj = create_text(section, NULL, lv_get_string(STR_MANUAL_KEYSTONE), lv_get_string(STR_RESET));
    lv_group_add_obj(Setting_keystone_group, manual_keystone_reset_obj);
    lv_obj_add_event_cb(manual_keystone_reset_obj, keystone_event_cb, LV_EVENT_KEY, NULL);

    if (lv_get_sys_param(P_AUTO_KEYSTONE)) {
        lv_label_set_text(keystone_sw_label, lv_get_string(STR_ON));
        lv_obj_add_state(keystone_sw, LV_STATE_CHECKED);
    } else {
        lv_label_set_text(keystone_sw_label, lv_get_string(STR_OFF));
        lv_obj_clear_state(keystone_sw, LV_STATE_CHECKED);
    }
    enable_manual_keystone_mode(lv_get_sys_param(P_AUTO_KEYSTONE));
}

void lv_keystone_page_exit(void)
{
}