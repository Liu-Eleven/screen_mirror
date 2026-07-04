/*
 * lv_pro_music_activity.c
 *
 */

#include "lv_pro_music_activity.h"
#include "lv_pro_music_list_activity.h"
#include "../lv_pro_media.h"
#include "../lv_pro_media_layout.h"

static lv_timer_t *playbar_timer;
static lv_obj_t *anim_img;
static lv_obj_t *play_imgbtn;
static lv_obj_t *music_time_bar;
static lv_obj_t *music_name;
static lv_obj_t *music_label;
static lv_obj_t *state_label;
static lv_obj_t *curtime_label;
static lv_obj_t *totaltime_label;
static lv_obj_t *music_img;
static lv_obj_t *ctrlbar_obj;
static lv_timer_t *ctrlbar_show_timer;
extern bool media_mode_single;

lv_obj_t *music_activity;
lv_group_t *music_group;
lv_group_t *music_slider_group;
extern lv_obj_t *launcher_activity;


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

static char *anim_png_path[] = { "A:/usr/share/lv_projector/media_playing01.png",
        "A:/usr/share/lv_projector/media_playing02.png",
        "A:/usr/share/lv_projector/media_playing03.png",
        "A:/usr/share/lv_projector/media_playing04.png" };

static void playbar_anim_timer(lv_timer_t *timer) {
    static int index;
    static int16_t angle;
    double percent;
    char totaltime[8], curtime[8];

    index = index >= 3 ? 0 : ++index;
    angle = angle >= 3600 ? 0: 100 + angle;
    lv_img_set_src(anim_img, anim_png_path[index]);
    lv_img_set_angle(music_img, angle);

    lv_pro_res_media_get_time(curtime, totaltime, &percent);

    lv_slider_set_value(music_time_bar, percent * 100, LV_ANIM_OFF);

    lv_label_set_text_fmt(curtime_label, "#ffffff %s #", curtime);
    lv_label_set_text_fmt(totaltime_label, "#ffffff %s #", totaltime);
}

static void music_slider_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t *key = lv_event_get_param(e);

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
        if (*key == LV_KEY_DOWN) {
            set_current_ui_group(music_group);
        } else if (*key == LV_KEY_LEFT) {
            lv_pro_res_media_set_seek(true);
            playbar_anim_timer(playbar_timer);
        } else if (*key == LV_KEY_RIGHT) {
            lv_pro_res_media_set_seek(false);
            playbar_anim_timer(playbar_timer);
        }
    }
}

static void music_player_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    char *user_data = (char*) lv_event_get_user_data(e);
    uint32_t *key = lv_event_get_param(e);
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
                if (lv_pro_res_media_get_playing()) {
                    lv_pro_res_media_pause();
                    lv_timer_pause(playbar_timer);
                    lv_label_set_text_fmt(btn->name, "%s", lv_get_string(STR_PAUSE));
                    lv_label_set_text_fmt(state_label, "%s", lv_get_string(STR_PAUSE));
                    lv_img_set_src(btn->img, &player_pause);
                } else {
                    lv_pro_res_media_play();
                    lv_timer_resume(playbar_timer);
                    lv_label_set_text_fmt(btn->name, "%s", lv_get_string(STR_PLAY));
                    lv_label_set_text_fmt(state_label, "%s", lv_get_string(STR_PLAY));
                    lv_img_set_src(btn->img, &player_playing);
                }
            } else if (!strcmp(user_data, "pre")) {
                lv_pro_media_msg_enqueue(MSG_TYPE_MEDIA_PLAY_NEXT, 0, false);
            } else if (!strcmp(user_data, "next")) {
                lv_pro_media_msg_enqueue(MSG_TYPE_MEDIA_PLAY_NEXT, 1, false);
            } else if (!strcmp(user_data, "stop")) {
                switch_page_show_destory(PAGE_FILE);
            } else if (!strcmp(user_data, "mode")) {
                if (!media_mode_single) {
                    lv_pro_res_media_set_looping(true);
                    lv_label_set_text_fmt(btn->name, "%s", lv_get_string(STR_SINGLEROUND));
                    lv_img_set_src(btn->img, &player_repeat);
                    media_mode_single = true;
                } else {
                    lv_pro_res_media_set_looping(false);
                    lv_label_set_text_fmt(btn->name, "%s", lv_get_string(STR_LISTROUND));
                    lv_img_set_src(btn->img, &player_sequential);
                    media_mode_single = false;
                }
            } else if (!strcmp(user_data, "info")) {
                lv_pro_music_ctrlbar_timer_en(false);
                lv_pro_music_info_init();
                set_current_ui_group(music_info_group);
            } else if (!strcmp(user_data, "list")) {
                lv_pro_music_ctrlbar_timer_en(false);
                lv_pro_music_list_init();
                set_current_ui_group(music_list_group);
            }
        } else if (*key == LV_KEY_UP) {
            lv_group_add_obj(music_slider_group, music_time_bar);
            set_current_ui_group(music_slider_group);
        }
    }
}

void lv_pro_music_play_status(bool en) {
    if (en) {
        lv_pro_music_refresh_list();
        if (lv_pro_res_media_get_media_list()) {
            lv_label_set_text_fmt(music_name, "#ffffff %s #",
                    lv_pro_res_media_get_media_list()->current_node->name);
            lv_label_set_text_fmt(music_label, "%s",
                    lv_pro_res_media_get_media_list()->current_node->name);
        }
        if (lv_pro_res_media_get_playing()) {
            lv_timer_resume(playbar_timer);
            lv_pro_ctrlbar_imgbtn_t *play_obj = (lv_pro_ctrlbar_imgbtn_t*) lv_obj_get_parent(
                    play_imgbtn);
            lv_img_set_src(play_obj->img, &player_playing);
            lv_label_set_text_fmt(play_obj->name, "%s", lv_get_string(STR_PLAY));
            lv_label_set_text_fmt(state_label, "%s", lv_get_string(STR_PLAY));
        }
    } else {
        lv_timer_pause(playbar_timer);
        lv_pro_ctrlbar_imgbtn_t *play_obj = (lv_pro_ctrlbar_imgbtn_t*) lv_obj_get_parent(
                play_imgbtn);
        lv_img_set_src(play_obj->img, &player_pause);
        lv_label_set_text_fmt(play_obj->name, "%s", lv_get_string(STR_PAUSE));
        lv_label_set_text_fmt(state_label, "%s", lv_get_string(STR_PAUSE));
    }
}

void lv_pro_music_ctrlbar_timer_en(bool en) {
    if (en) {
        lv_timer_resume(ctrlbar_show_timer);
        show_player_ctrlbar(true);
        lv_timer_reset(ctrlbar_show_timer);
    } else {
        lv_timer_pause(ctrlbar_show_timer);
    }
}

void lv_pro_music_exit(void) {
    lv_pro_res_media_stop();
    lv_timer_del(playbar_timer);
    lv_timer_del(ctrlbar_show_timer);
    lv_pro_media_msg_enqueue(MSG_TYPE_MEDIA_DESTORY, NULL, false);

    if (music_slider_group) {
        lv_group_remove_all_objs(music_slider_group);
        lv_group_del(music_slider_group);
        music_slider_group = NULL;
    }
    if (music_group) {
        lv_group_remove_all_objs(music_group);
        lv_group_del(music_group);
        music_group = NULL;
    }
    if (music_activity) {
        lv_obj_del_async(music_activity);
        music_activity = NULL;
    }
}

void lv_pro_music_ui_init(void) {
    music_activity = lv_obj_create(NULL);
    music_group = lv_group_create();
    music_slider_group = lv_group_create();

    media_mode_single = false;
    lv_obj_set_size(music_activity, lv_pct(100), lv_pct(100));
    lv_obj_add_style(music_activity, &lv_pro_res.set_bg_blue, 0);

    lv_obj_t *main_cont = lv_obj_create(music_activity);
    lv_obj_set_size(main_cont, lv_pct(80), lv_pct(50));
    lv_obj_center(main_cont);
    lv_obj_add_style(main_cont, &lv_pro_res.set_bg_blue, 0);

    lv_obj_t *title_label = lv_label_create(main_cont);
    lv_obj_set_pos(title_label, 20, 20);
    lv_obj_set_style_text_font(title_label, &GENERAL_FONT_BIG, 0);
    lv_label_set_recolor(title_label, true);
    lv_label_set_text_fmt(title_label, "#ffffff %s #", "Music");

    anim_img = lv_img_create(main_cont);
    lv_img_set_src(anim_img, anim_png_path[0]);
    lv_obj_align(anim_img, LV_ALIGN_TOP_RIGHT, -20, 20);

    music_name = lv_label_create(main_cont);
    lv_label_set_long_mode(music_name, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(music_name, 340);
    lv_label_set_recolor(music_name, true);
    lv_obj_set_style_text_font(music_name, &GENERAL_FONT_BIG, 0);
    if (lv_pro_res_media_get_media_list()) {
        lv_label_set_text_fmt(music_name, "#ffffff %s #",
                lv_pro_res_media_get_media_list()->current_node->name);
    } else {
        lv_label_set_text_fmt(music_name, "#ffffff %s #", "Not Music Play");
    }
    lv_obj_align(music_name, LV_ALIGN_CENTER, 40, -40);

    music_img = lv_img_create(main_cont);
    lv_img_set_src(music_img, "A:/usr/share/lv_projector/music_record.png");
    lv_obj_align_to(music_img, music_name, LV_ALIGN_OUT_LEFT_MID, -20, 0);

    ctrlbar_obj = lv_obj_create(music_activity);
    lv_obj_set_size(ctrlbar_obj, lv_pct(100), lv_pct(CTRLBAR_PCT));
    lv_obj_align(ctrlbar_obj, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_style(ctrlbar_obj, &lv_pro_res.set_bg_black, 0);
    lv_obj_set_style_bg_opa(ctrlbar_obj, LV_OPA_60, 0);

    lv_obj_t *btn_cont = lv_obj_create(ctrlbar_obj);
    lv_obj_set_size(btn_cont, lv_pct(90), lv_pct(52));
    lv_obj_align(btn_cont, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_style(btn_cont, &lv_pro_res.set_bg_transp, 0);

    lv_obj_set_style_pad_column(btn_cont, CTRLBAR_BTN_COL, 0);
    lv_obj_set_style_pad_all(btn_cont, 5, 0);
    lv_obj_set_scrollbar_mode(btn_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(btn_cont, LV_FLEX_FLOW_ROW);

    const lv_img_dsc_t *icon_path[] = {
            &player_playing,
            &player_back,
            &player_next,
            &player_stop,
            &player_sequential,
            &player_info,
            &player_list
    };

    int imgbtn_label[] = { STR_PLAY, STR_PREV, STR_NEXT, STR_STOP,
            STR_ROUND, STR_INFO, STR_LIST };

    char *imgbtn_name[] = { "play", "pre", "next", "stop", "mode", "info", "list" };

    /*Create image button*/
    for (int i = 0; i < 7; i++) {
        lv_obj_t *ctrlbar_imgbtn = lv_pro_ctrlbar_imgbtn_create(btn_cont);
        lv_obj_set_size(ctrlbar_imgbtn, CTRLBAR_BTN_WIDTH, CTRLBAR_BTN_HEIGHT);
        lv_pro_ctrlbar_imgbtn_set_src(ctrlbar_imgbtn, icon_path[i],
                lv_get_string(imgbtn_label[i]), &GENERAL_FONT_MID,
                &lv_pro_res.set_bg_transp);
        lv_obj_t *ctrlbar_imgbtn_btn = lv_pro_ctrlbar_imgbtn_get_btn(ctrlbar_imgbtn);
        if (i == 0) {
            play_imgbtn = ctrlbar_imgbtn_btn;
        }
        lv_obj_add_event_cb(ctrlbar_imgbtn_btn, music_player_event_cb, LV_EVENT_KEY, imgbtn_name[i]);
        lv_group_add_obj(music_group, ctrlbar_imgbtn_btn);
    }


    lv_obj_t *time_cont = lv_obj_create(ctrlbar_obj);
    lv_obj_set_size(time_cont, lv_pct(90), lv_pct(48));
    lv_obj_add_style(time_cont, &lv_pro_res.set_bg_transp, 0);
    lv_obj_align_to(time_cont, btn_cont, LV_ALIGN_OUT_TOP_MID, 0, 0);

    music_label = lv_label_create(time_cont);
    lv_label_set_long_mode(music_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(music_label, lv_pct(40));
    lv_label_set_recolor(music_label, true);
    lv_obj_set_style_text_font(music_label, &GENERAL_FONT_MID, 0);
    if (lv_pro_res_media_get_media_list()) {
        lv_label_set_text_fmt(music_label, "%s",
                lv_pro_res_media_get_media_list()->current_node->name);
    } else {
        lv_label_set_text_fmt(music_label, "%s", "Not Music Play");
    }
    lv_obj_set_style_text_color(music_label, lv_color_white(), 0);
    lv_obj_align(music_label, LV_ALIGN_TOP_LEFT, lv_pct(10), STATE_ALIGN);

    state_label = lv_label_create(time_cont);
    lv_label_set_long_mode(state_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_font(state_label, &GENERAL_FONT_MID, 0);
    lv_label_set_text_fmt(state_label, "%s", lv_get_string(STR_PLAY));
    lv_obj_set_style_text_color(state_label, lv_color_white(), 0);
    lv_obj_align(state_label, LV_ALIGN_TOP_MID, lv_pct(30), STATE_ALIGN);

    music_time_bar = lv_slider_create(time_cont);
    lv_obj_set_size(music_time_bar, lv_pct(80), TIMEBAR_HEIGHT);
    lv_obj_align(music_time_bar, LV_ALIGN_BOTTOM_MID, 0, TIMEBAR_ALIGN);
    lv_slider_set_value(music_time_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(music_time_bar, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_bg_color(music_time_bar, lv_color_hex(0xff8000), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(music_time_bar, lv_color_hex(0xc0c0c0), LV_PART_KNOB);
    lv_obj_add_event_cb(music_time_bar, music_slider_event_cb, LV_EVENT_KEY, NULL);

    curtime_label = lv_label_create(time_cont);
    lv_label_set_recolor(curtime_label, true);
    lv_obj_set_style_text_font(curtime_label, &GENERAL_FONT_MID, 0);
    lv_label_set_text_fmt(curtime_label, "#ffffff %s #", "00:00");
    lv_obj_align_to(curtime_label, music_time_bar, LV_ALIGN_OUT_LEFT_MID, 0, 0);

    totaltime_label = lv_label_create(time_cont);
    lv_label_set_recolor(totaltime_label, true);
    lv_obj_set_style_text_font(totaltime_label, &GENERAL_FONT_MID, 0);
    lv_label_set_text_fmt(totaltime_label, "#ffffff %s #", "00:00");
    lv_obj_align_to(totaltime_label, music_time_bar, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

    show_player_ctrlbar(true);
    ctrlbar_show_timer = lv_timer_create(ctrlbar_show_timer_cb, 5000, NULL);

    playbar_timer = lv_timer_create(playbar_anim_timer, 500, NULL);

    lv_pro_media_msg_enqueue(MSG_TYPE_MEDIA_PLAY, NULL, false);
}

static int create_music_ui(void)
{
    lv_pro_music_ui_init();
    load_current_channel(music_activity, music_group);

    return 0;
}

static int destory_music_ui(void)
{
    lv_pro_music_info_exit();
    lv_pro_music_list_exit();
    lv_pro_music_exit();
    page_id_t next_page_id = get_current_page_id();
    if ((next_page_id != PAGE_FILE) && (next_page_id != PAGE_SETTING)) {
        lv_pro_file_ui_exit();
    }
    return 0;
}

static int show_music_ui(void)
{
    if (music_info_activity) {
        lv_obj_clear_flag(music_info_activity, LV_OBJ_FLAG_HIDDEN);
        load_current_channel(music_activity, music_info_group);
    } else if (music_list_activity) {
        lv_obj_clear_flag(music_list_activity, LV_OBJ_FLAG_HIDDEN);
        load_current_channel(music_activity, music_list_group);
    } else {
        load_current_channel(music_activity, music_group);
    }
    return 0;
}

static int hide_music_ui(void)
{
    if (music_info_activity) {
        lv_obj_add_flag(music_info_activity, LV_OBJ_FLAG_HIDDEN);
    } else if (music_list_activity) {
        lv_obj_add_flag(music_list_activity, LV_OBJ_FLAG_HIDDEN);
    }
    return 0;
}

static page_interface_t page_music_ui =
{
    .ops =
    {
        create_music_ui,
        destory_music_ui,
        show_music_ui,
        hide_music_ui,
    },
    .info =
    {
        .id = PAGE_MUSIC,
        .user_data = NULL
    }
};

void REGISTER_PAGE_MUSIC(void)
{
    reg_page(&page_music_ui);
}
