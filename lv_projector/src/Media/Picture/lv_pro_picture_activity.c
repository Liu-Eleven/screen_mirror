/*
 * lv_pro_picture_activity.c
 *
 */

#include <stdio.h>
#include <math.h>
#include "lv_pro_picture_activity.h"
#include "lv_pro_picture_list_activity.h"
#include "../lv_pro_media.h"
#include "../lv_pro_media_layout.h"

static lv_obj_t *state_label;
static lv_obj_t *play_imgbtn;
static lv_obj_t *ctrlbar_obj;
static lv_timer_t *ctrlbar_show_timer;
static lv_timer_t *pic_timer;

lv_obj_t *picture_activity;
lv_obj_t *picture_cont;
lv_obj_t *picture_img;
lv_group_t *picture_group;
extern lv_obj_t *launcher_activity;
static lv_obj_t *picture_label;
static int rot_angle = 0;
static int zoom_val = 3;
static bool pic_playing = false;
static bool picture_mode_single = false;
static char *zoom_str[] = {"x1/8", "x1/4", "x1/2", "", "x2", "x4", "x8"};
extern int pic_zoom_factor;
pthread_mutex_t picture_destory_mutex = PTHREAD_MUTEX_INITIALIZER;

static void lv_pro_picture_image_value_init(void) {
    rot_angle = 0;
    zoom_val = 3;
}

static void lv_pro_picture_static_value_init(void) {
    picture_mode_single = false;
    pic_playing = true;
    lv_pro_picture_image_value_init();
}

static void lv_pro_picture_show_timer_en(bool en) {
    if (en) {
        if (!picture_mode_single) {
            lv_timer_resume(pic_timer);
            lv_timer_reset(pic_timer);
        }
        pic_playing = true;
    } else {
        if (!picture_mode_single) {
            lv_timer_pause(pic_timer);
        }
        pic_playing = false;
    }
}

bool lv_pro_res_picture_get_playing() {
    return pic_playing;
}

static void show_player_ctrlbar(bool show) {
    if (show) {
        lv_obj_clear_flag(ctrlbar_obj, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(ctrlbar_obj, LV_OBJ_FLAG_HIDDEN);
    }
}

static void ctrlbar_show_timer_cb(lv_timer_t *timer) {
    show_player_ctrlbar(false);
}

void lv_pro_picture_ctrlbar_timer_en(bool en) {
    if (en) {
        lv_timer_resume(ctrlbar_show_timer);
        show_player_ctrlbar(true);
        lv_timer_reset(ctrlbar_show_timer);
    } else {
        lv_timer_pause(ctrlbar_show_timer);
    }
}

static void lv_pro_picture_show_anim_timer(lv_timer_t *timer) {
    lv_pro_picture_msg_enqueue(MSG_TYPE_PICTURE_PLAY_NEXT, 1, false);
}

void lv_pro_picture_play_status(bool en) {
    if (en) {
        lv_pro_picture_image_value_init();
        lv_pro_picture_refresh_info_list();
        if (lv_pro_res_picture_get_media_list()) {
            lv_label_set_text_fmt(picture_label, "%s",
                    lv_pro_res_picture_get_media_list()->current_node->name);
        }
        if (!lv_pro_res_picture_get_playing()) {
            lv_pro_picture_show_timer_en(true);
            lv_pro_ctrlbar_imgbtn_t *play_obj = (lv_pro_ctrlbar_imgbtn_t*) lv_obj_get_parent(
                    play_imgbtn);
            lv_img_set_src(play_obj->img, &player_playing);
            lv_label_set_text_fmt(play_obj->name, "%s", lv_get_string(STR_PLAY));
            lv_label_set_text_fmt(state_label, "%s", lv_get_string(STR_PLAY));
        } else {
            lv_timer_reset(pic_timer);
        }
    } else {
        if (lv_pro_res_picture_get_playing()) {
            lv_pro_picture_show_timer_en(false);
            lv_pro_ctrlbar_imgbtn_t *play_obj = (lv_pro_ctrlbar_imgbtn_t*) lv_obj_get_parent(
                    play_imgbtn);
            lv_img_set_src(play_obj->img, &player_pause);
            lv_label_set_text_fmt(play_obj->name, "%s", lv_get_string(STR_PAUSE));
            lv_label_set_text_fmt(state_label, "%s", lv_get_string(STR_PAUSE));
        }
    }
}

static void zoom_for_picture(lv_obj_t *picture_img, int rot_angle)
{
    lv_coord_t pic_width = lv_obj_get_width(picture_img);
    lv_coord_t pic_height = lv_obj_get_height(picture_img);

    int new_width = pic_width;
    int new_height = pic_height;

    rot_angle = rot_angle / 10;

    if (rot_angle == 90 || rot_angle == 270) {
        new_width = pic_height;
        new_height = pic_width;
    }

    pic_zoom_factor = lv_pro_set_picture_rect(new_width, new_height);
}

static void picture_player_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    char *user_data = (char*) lv_event_get_user_data(e);
    uint32_t *key = lv_event_get_param(e);
    char *zoom_state = NULL;
    int zoom = 256;
    lv_pro_ctrlbar_imgbtn_t *btn = (lv_pro_ctrlbar_imgbtn_t*) lv_obj_get_parent(
            lv_event_get_target(e));

    if (code == LV_EVENT_KEY) {
        if (lv_obj_has_flag(ctrlbar_obj, LV_OBJ_FLAG_HIDDEN)) {
            if (*key == LV_KEY_BACK) {
                switch_page_show_destory(PAGE_FILE);
                return;
            } else {
                show_player_ctrlbar(true);
                lv_timer_reset(ctrlbar_show_timer);
                return;
            }
        } else {
            if (*key == LV_KEY_BACK) {
                show_player_ctrlbar(false);
                return;
            } else {
                lv_timer_reset(ctrlbar_show_timer);
            }
        }
        if (*key == LV_KEY_ENTER) {
            if (!strcmp(user_data, "play")) {
                if (lv_pro_res_picture_get_playing()) {
                    lv_pro_picture_show_timer_en(false);
                    lv_label_set_text_fmt(btn->name, "%s", lv_get_string(STR_PAUSE));
                    lv_label_set_text_fmt(state_label, "%s", lv_get_string(STR_PAUSE));
                    lv_img_set_src(btn->img, &player_pause);
                } else {
                    lv_pro_picture_show_timer_en(true);
                    lv_label_set_text_fmt(btn->name, "%s", lv_get_string(STR_PLAY));
                    lv_label_set_text_fmt(state_label, "%s", lv_get_string(STR_PLAY));
                    lv_img_set_src(btn->img, &player_playing);
                }
            } else if (!strcmp(user_data, "pre")) {
                lv_pro_picture_msg_enqueue(MSG_TYPE_PICTURE_PLAY_NEXT, 0, false);
            } else if (!strcmp(user_data, "next")) {
                lv_pro_picture_msg_enqueue(MSG_TYPE_PICTURE_PLAY_NEXT, 1, false);
            } else if (!strcmp(user_data, "stop")) {
                switch_page_show_destory(PAGE_FILE);
            } else if (!strcmp(user_data, "anticlockwise")) {
                lv_pro_picture_play_status(false);
                if (rot_angle == 0)
                    rot_angle = 3600;
                rot_angle = rot_angle - 900;
                zoom_for_picture(picture_img, rot_angle);
                lv_img_set_angle(picture_img, rot_angle);
            } else if (!strcmp(user_data, "clockwise")) {
                lv_pro_picture_play_status(false);
                rot_angle = rot_angle >= 2700 ? 0: rot_angle + 900;
                zoom_for_picture(picture_img, rot_angle);
                lv_img_set_angle(picture_img, rot_angle);
            } else if (!strcmp(user_data, "mode")) {
                if (!picture_mode_single) {
                    if (lv_pro_res_picture_get_playing()) {
                        lv_timer_pause(pic_timer);
                    }
                    lv_label_set_text_fmt(btn->name, "%s", lv_get_string(STR_SINGLEROUND));
                    lv_img_set_src(btn->img, &player_repeat);
                    picture_mode_single = true;
                } else {
                    if (lv_pro_res_picture_get_playing()) {
                        lv_timer_resume(pic_timer);
                        lv_timer_reset(pic_timer);
                    }
                    lv_label_set_text_fmt(btn->name, "%s", lv_get_string(STR_LISTROUND));
                    lv_img_set_src(btn->img, &player_sequential);
                    picture_mode_single = false;
                }
            } else if (!strcmp(user_data, "zoomout")) {
                lv_pro_picture_play_status(false);
                zoom_val = zoom_val == 0 ? 3 : zoom_val - 1;
                zoom = pic_zoom_factor * pow(2, zoom_val - 3);
                if (zoom_val <= 3) {
                    zoom_state = lv_get_string(STR_ZOOMOUT_STATE);
                } else {
                    zoom_state = lv_get_string(STR_ZOOMIN_STATE);
                }
                lv_img_set_zoom(picture_img, zoom);
                lv_label_set_text_fmt(state_label, "%s   %s", zoom_state, zoom_str[zoom_val]);
            } else if (!strcmp(user_data, "zoomin")) {
                lv_pro_picture_play_status(false);
                zoom_val = zoom_val == 6 ? 3 : zoom_val + 1;
                zoom = pic_zoom_factor * pow(2, zoom_val - 3);
                if (zoom_val < 3) {
                    zoom_state = lv_get_string(STR_ZOOMOUT_STATE);
                } else {
                    zoom_state = lv_get_string(STR_ZOOMIN_STATE);
                }
                lv_img_set_zoom(picture_img, zoom);
                lv_label_set_text_fmt(state_label, "%s   %s", zoom_state, zoom_str[zoom_val]);
            } else if (!strcmp(user_data, "info")) {
                lv_pro_picture_ctrlbar_timer_en(false);
                lv_pro_picture_info_init();
                set_current_ui_group(picture_info_group);
            } else if (!strcmp(user_data, "list")) {
                lv_pro_picture_ctrlbar_timer_en(false);
                lv_pro_picture_list_init();
                set_current_ui_group(picture_list_group);
            }
        }
    }
}

void lv_pro_picture_exit(void) {
    lv_timer_del(ctrlbar_show_timer);
    lv_timer_del(pic_timer);
    lv_pro_res_picture_list_deinit();

    if (picture_group) {
        lv_group_remove_all_objs(picture_group);
        lv_group_del(picture_group);
        picture_group = NULL;
    }
    if (picture_activity) {
        lv_obj_del_async(picture_activity);
        picture_activity = NULL;
        picture_img = NULL;
    }
}

void lv_pro_picture_ui_init(void) {
    picture_activity = lv_obj_create(NULL);
    picture_group = lv_group_create();

    lv_pro_picture_static_value_init();
    lv_obj_set_size(picture_activity, lv_pct(100), lv_pct(100));
    lv_obj_add_style(picture_activity, &lv_pro_res.set_bg_black, 0);

    picture_cont = lv_obj_create(picture_activity);
    lv_obj_set_size(picture_cont, lv_pct(100), lv_pct(100));
    lv_obj_add_style(picture_cont, &lv_pro_res.set_bg_transp, 0);
    lv_obj_set_scrollbar_mode(picture_cont, LV_SCROLLBAR_MODE_OFF);

    ctrlbar_obj = lv_obj_create(picture_activity);
    lv_obj_set_size(ctrlbar_obj, lv_pct(100), lv_pct(26));
    lv_obj_align(ctrlbar_obj, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_style(ctrlbar_obj, &lv_pro_res.set_bg_black, 0);
    lv_obj_set_style_bg_opa(ctrlbar_obj, LV_OPA_60, 0);

    lv_obj_t *btn_cont = lv_obj_create(ctrlbar_obj);
    lv_obj_set_size(btn_cont, lv_pct(90), lv_pct(62));
    lv_obj_align(btn_cont, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_style(btn_cont, &lv_pro_res.set_bg_transp, 0);

    lv_obj_set_style_pad_column(btn_cont, CTRLBAR_BTN_COL, 0);
    lv_obj_set_style_pad_all(btn_cont, 5, 0);
    lv_obj_set_scrollbar_mode(btn_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(btn_cont, LV_FLEX_FLOW_ROW);

    const lv_img_dsc_t *icon_path[] = {
            &player_playing,
            &picture_anticlockwise,
            &picture_clockwise,
            &player_back,
            &player_next,
            &player_stop,
            &player_sequential,
            &picture_zoomout,
            &picture_zoomin,
            &player_info,
            &player_list
    };

    int imgbtn_label[] = { STR_PLAY, STR_ROTATE_LEFT, STR_ROTATE_RIGHT_K,
            STR_PHOTO_PREV, STR_PHOTO_NEXT, STR_STOP, STR_ROUND,
            STR_ZOOMOUT_STATE, STR_ZOOMIN_STATE, STR_INFO, STR_LIST };

    char *imgbtn_name[] = { "play", "anticlockwise", "clockwise", "pre", "next", "stop", "mode", "zoomout", "zoomin", "info", "list" };

    /*Create image button*/
    for (int i = 0; i < 11; i++) {
        lv_obj_t *ctrlbar_imgbtn = lv_pro_ctrlbar_imgbtn_create(btn_cont);
        lv_obj_set_size(ctrlbar_imgbtn, CTRLBAR_BTN_WIDTH, CTRLBAR_BTN_HEIGHT);
        lv_pro_ctrlbar_imgbtn_set_src(ctrlbar_imgbtn, icon_path[i],
                lv_get_string(imgbtn_label[i]), &GENERAL_FONT_MID,
                &lv_pro_res.set_bg_transp);
        lv_obj_t *ctrlbar_imgbtn_btn = lv_pro_ctrlbar_imgbtn_get_btn(ctrlbar_imgbtn);
        if (i == 0) {
            play_imgbtn = ctrlbar_imgbtn_btn;
        }
        lv_obj_add_event_cb(ctrlbar_imgbtn_btn, picture_player_event_cb, LV_EVENT_KEY, imgbtn_name[i]);
        lv_group_add_obj(picture_group, ctrlbar_imgbtn_btn);
    }

    lv_obj_t *label_cont = lv_obj_create(ctrlbar_obj);
    lv_obj_set_size(label_cont, lv_pct(90), lv_pct(38));
    lv_obj_add_style(label_cont, &lv_pro_res.set_bg_transp, 0);
    lv_obj_align_to(label_cont, btn_cont, LV_ALIGN_OUT_TOP_MID, 0, 0);

    picture_label = lv_label_create(label_cont);
    lv_label_set_long_mode(picture_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(picture_label, lv_pct(40));
    lv_label_set_recolor(picture_label, true);
    lv_obj_set_style_text_font(picture_label, &GENERAL_FONT_MID, 0);
    if (lv_pro_res_picture_get_media_list()) {
        lv_label_set_text_fmt(picture_label, "%s",
                lv_pro_res_picture_get_media_list()->current_node->name);
    } else {
        lv_label_set_text_fmt(picture_label, "%s", "Not Picture Play");
    }
    lv_obj_set_style_text_color(picture_label, lv_color_white(), 0);
    lv_obj_align(picture_label, LV_ALIGN_TOP_LEFT, lv_pct(10), STATE_ALIGN);

    state_label = lv_label_create(label_cont);
    lv_label_set_long_mode(state_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_font(state_label, &GENERAL_FONT_MID, 0);
    lv_label_set_text_fmt(state_label, "%s", lv_get_string(STR_PLAY));
    lv_obj_set_style_text_color(state_label, lv_color_white(), 0);
    lv_obj_align(state_label, LV_ALIGN_TOP_MID, lv_pct(30), STATE_ALIGN);

    show_player_ctrlbar(true);
    ctrlbar_show_timer = lv_timer_create(ctrlbar_show_timer_cb, 5000, NULL);

    pic_timer = lv_timer_create(lv_pro_picture_show_anim_timer, 3000, NULL);

    lv_pro_picture_msg_enqueue(MSG_TYPE_PICTURE_PLAY, NULL, false);
}

static int create_picture_ui(void)
{
    lv_pro_picture_ui_init();
    load_current_channel(picture_activity, picture_group);

    return 0;
}

static int destory_picture_ui(void)
{
    pthread_mutex_lock(&picture_destory_mutex);
    lv_pro_picture_info_exit();
    lv_pro_picture_list_exit();
    lv_pro_picture_exit();
    lv_img_cache_invalidate_src(NULL);
    page_id_t next_page_id = get_current_page_id();
    if ((next_page_id != PAGE_FILE) && (next_page_id != PAGE_SETTING)) {
        lv_pro_file_ui_exit();
    }
    pthread_mutex_unlock(&picture_destory_mutex);
    return 0;
}

static int show_picture_ui(void)
{
    if (picture_info_activity) {
        lv_obj_clear_flag(picture_info_activity, LV_OBJ_FLAG_HIDDEN);
        load_current_channel(picture_activity, picture_info_group);
    } else if (picture_list_activity) {
        lv_obj_clear_flag(picture_list_activity, LV_OBJ_FLAG_HIDDEN);
        load_current_channel(picture_activity, picture_list_group);
    } else {
        load_current_channel(picture_activity, picture_group);
    }
    return 0;
}

static int hide_picture_ui(void)
{
    if (picture_info_activity) {
        lv_obj_add_flag(picture_info_activity, LV_OBJ_FLAG_HIDDEN);
    } else if (picture_list_activity) {
        lv_obj_add_flag(picture_list_activity, LV_OBJ_FLAG_HIDDEN);
    }
    return 0;
}

static page_interface_t page_picture_ui =
{
    .ops =
    {
        create_picture_ui,
        destory_picture_ui,
        show_picture_ui,
        hide_picture_ui,
    },
    .info =
    {
        .id = PAGE_PICTURE,
        .user_data = NULL
    }
};

void REGISTER_PAGE_PICTURE(void)
{
    reg_page(&page_picture_ui);
}
