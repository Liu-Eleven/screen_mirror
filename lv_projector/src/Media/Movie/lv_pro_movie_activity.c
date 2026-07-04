/*
 * lv_pro_movie_activity.c
 *
 */

#include "lv_pro_movie_activity.h"
#include "lv_pro_movie_list_activity.h"
#include "../lv_pro_media.h"
#ifdef MOVIE_SUBTITLE_SUPPORT
#include "../middle_ware/lv_pro_res_media_player_int.h"
#endif

static lv_obj_t *play_imgbtn;
static lv_obj_t *movie_time_bar;
static lv_obj_t *movie_label;
static lv_obj_t *state_label;
static lv_obj_t *curtime_label;
static lv_obj_t *totaltime_label;
static lv_obj_t *movie_img;
static lv_obj_t *ctrlbar_obj;
static lv_timer_t *playbar_timer;
static lv_timer_t *ctrlbar_show_timer;
static pthread_mutex_t movie_speed_mutex = PTHREAD_MUTEX_INITIALIZER;
static int fb_speed = 4;
static int ff_speed = 4;
int movie_ratio = 0;
extern bool media_mode_single;
static char *ff_str[] = {">> x16", ">> x8", ">> x4", ">> x2", ""};
static char *fb_str[] = {"", "<< x2", "<< x4", "<< x8", "<< x16"};
static int ratio_str[] = {STR_ASPECT_AUTO, STR_ASPECT_16_9, STR_ASPECT_4_3};

lv_obj_t *movie_activity;
lv_group_t *movie_group;
lv_group_t *movie_slider_group;
extern lv_obj_t *launcher_activity;

static void lv_pro_movie_speed_value_init(void) {
    fb_speed = 4;
    ff_speed = 4;
}

static void lv_pro_movie_static_value_init(void) {
    movie_ratio = 0;
    media_mode_single = false;
    lv_pro_movie_speed_value_init();
}

static void show_player_ctrlbar(bool show) {
    if (show) {
        lv_obj_clear_flag(ctrlbar_obj, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(ctrlbar_obj, LV_OBJ_FLAG_HIDDEN);
        lv_memset_00(lv_disp_get_default()->driver->draw_buf->buf_act,
                lv_disp_get_default()->driver->draw_buf->size * sizeof(lv_color32_t));
    }
}

static void ctrlbar_show_timer_cb(lv_timer_t *timer) {
    show_player_ctrlbar(false);
}

static void playbar_anim_timer(lv_timer_t *timer) {
    double percent;
    char totaltime[8], curtime[8];

    lv_pro_res_media_get_time(curtime, totaltime, &percent);

    lv_slider_set_value(movie_time_bar, percent * 100, LV_ANIM_OFF);

    lv_label_set_text_fmt(curtime_label, "#ffffff %s #", curtime);
    lv_label_set_text_fmt(totaltime_label, "#ffffff %s #", totaltime);
}

static void movie_slider_event_cb(lv_event_t *e) {
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
            set_current_ui_group(movie_group);
        } else if (*key == LV_KEY_LEFT) {
            lv_pro_res_media_set_seek(true);
            playbar_anim_timer(playbar_timer);
        } else if (*key == LV_KEY_RIGHT) {
            lv_pro_res_media_set_seek(false);
            playbar_anim_timer(playbar_timer);
        }
    }
}

static void movie_player_event_cb(lv_event_t *e) {
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
                    if (!lv_pro_res_media_pause()) {
                        lv_timer_pause(playbar_timer);
                        lv_label_set_text_fmt(btn->name, "%s", lv_get_string(STR_PAUSE));
                        lv_label_set_text_fmt(state_label, "%s", lv_get_string(STR_PAUSE));
                        lv_img_set_src(btn->img, &player_pause);
                    }
                } else {
                    if (!lv_pro_res_media_play()) {
                        lv_timer_resume(playbar_timer);
                        lv_label_set_text_fmt(btn->name, "%s", lv_get_string(STR_PLAY));
                        lv_label_set_text_fmt(state_label, "%s", lv_get_string(STR_PLAY));
                        lv_img_set_src(btn->img, &player_playing);
                        lv_pro_movie_speed_value_init();
                    }
                }
            } else if (!strcmp(user_data, "backward")) {
                if (lv_pro_res_media_get_playing()) {
                    pthread_mutex_lock(&movie_speed_mutex);
                    fb_speed = fb_speed == 8 ? 4 : fb_speed + 1;
                    ff_speed = 4;
                    if (!lv_pro_res_media_set_speed(fb_speed)) {
                        lv_label_set_text_fmt(state_label, "%s   %s", lv_get_string(STR_FB),
                                fb_str[fb_speed - 4]);
                    } else {
                        lv_label_set_text_fmt(state_label, "%s", lv_get_string(STR_PLAY));
                        fb_speed = 4;
                    }
                    pthread_mutex_unlock(&movie_speed_mutex);
                }
            } else if (!strcmp(user_data, "forward")) {
                if (lv_pro_res_media_get_playing()) {
                    pthread_mutex_lock(&movie_speed_mutex);
                    ff_speed = ff_speed == 0 ? 4 : ff_speed - 1;
                    fb_speed = 4;
                    if (!lv_pro_res_media_set_speed(ff_speed)) {
                        lv_label_set_text_fmt(state_label, "%s   %s", lv_get_string(STR_FF),
                                ff_str[ff_speed]);
                    } else {
                        lv_label_set_text_fmt(state_label, "%s", lv_get_string(STR_PLAY));
                        ff_speed = 4;
                    }
                    pthread_mutex_unlock(&movie_speed_mutex);
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
                lv_pro_movie_ctrlbar_timer_en(false);
                lv_pro_movie_info_init();
                set_current_ui_group(movie_info_group);
            } else if (!strcmp(user_data, "list")) {
                lv_pro_movie_ctrlbar_timer_en(false);
                lv_pro_movie_list_init();
                set_current_ui_group(movie_list_group);
            } else if (!strcmp(user_data, "Auto")) {
                movie_ratio = movie_ratio == 2 ? 0 : movie_ratio + 1;
                if (movie_ratio == 2) {
                    lv_pro_res_movie_set_ratio(Movie_Ratio_4_3);
                } else if (movie_ratio == 1) {
                    lv_pro_res_movie_set_ratio(Movie_Ratio_16_9);
                } else {
                    lv_pro_res_movie_set_ratio(Movie_Ratio_Auto);
                }
                lv_label_set_text_fmt(btn->name, "%s", lv_get_string(ratio_str[movie_ratio]));
            }
        } else if (*key == LV_KEY_UP) {
            lv_group_add_obj(movie_slider_group, movie_time_bar);
            set_current_ui_group(movie_slider_group);
        }
    }
}

void lv_pro_movie_play_status(bool en) {
    if (en) {
        lv_pro_movie_speed_value_init();
        lv_pro_movie_refresh_list();
        if (lv_pro_res_media_get_media_list()) {
            lv_label_set_text_fmt(movie_label, "%s",
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

void lv_pro_movie_ctrlbar_timer_en(bool en) {
    if (en) {
        lv_timer_resume(ctrlbar_show_timer);
        show_player_ctrlbar(true);
        lv_timer_reset(ctrlbar_show_timer);
    } else {
        lv_timer_pause(ctrlbar_show_timer);
    }
}

#ifdef MOVIE_SUBTITLE_SUPPORT
lv_obj_t *subtitle_label;
lv_obj_t *subtitle_pic;
subtitles_event_t subtitles_e = -1;
char *subtitles_str = NULL;
lv_timer_t *subtitles_timer = NULL;
static lv_img_dsc_t subtitles_img_dsc = {0};
int subtitles_type = -1;

static void subtitles_event_cb() {
    switch (subtitles_e) {
    case SUBTITLES_EVENT_SHOW:
        if(subtitles_type == 1) {
            show_subtitles(subtitles_str);
        } else{
            show_subtitles_pic(&subtitles_img_dsc);
        }
        break;
    case SUBTITLES_EVENT_HIDDEN:
        if(subtitles_type == 1) {
            show_subtitles("");
        } else{
            show_subtitles_pic(NULL);
        }
        break;
    case SUBTITLES_EVENT_PAUSE:
        if (subtitles_timer) {
            lv_timer_pause(subtitles_timer);
        }
        break;
    case SUBTITLES_EVENT_RESUME:
        if (subtitles_timer) {
            lv_timer_resume(subtitles_timer);
            lv_timer_reset(subtitles_timer);
        }
        break;
    default:
        break;
    }
    subtitles_e = -1;
}

void subtitles_event_send(int e, lv_subtitle_t *subtitle) {
    if (e == SUBTITLES_EVENT_STOP) {
        subtitles_e = -1;
        if(subtitles_timer) {
            show_subtitles("");
            show_subtitles_pic(NULL);
        }
    } else {
        subtitles_e = e;
        if (subtitle == NULL) {
            return;
        }
        if (subtitle->type == 1){
            subtitles_str = (char *)subtitle->data;
        } else if (subtitle->type == 0){
            subtitles_img_dsc.header.w = subtitle->w;
            subtitles_img_dsc.header.h = subtitle->h;
            subtitles_img_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
            subtitles_img_dsc.data_size = subtitle->w*subtitle->h*LV_IMG_PX_SIZE_ALPHA_BYTE;
            subtitles_img_dsc.data = subtitle->data;
        }
        subtitles_type = subtitle->type;
    }
}

static void subtitles_timer_handle(lv_timer_t *e) {
    subtitles_event_cb();
}

void create_subtitles_rect(void) {
    lv_obj_t *subtitles_obj = lv_obj_create(movie_activity);
    lv_obj_set_size(subtitles_obj, LV_PCT(80), LV_SIZE_CONTENT);
    lv_obj_align(subtitles_obj, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_style(subtitles_obj, &lv_pro_res.set_bg_transp, 0);
    lv_obj_set_scrollbar_mode(subtitles_obj, LV_SCROLLBAR_MODE_OFF);

    subtitle_label = lv_label_create(subtitles_obj);
    lv_label_set_long_mode(subtitle_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_size(subtitle_label, lv_pct(100), LV_SIZE_CONTENT);
    lv_label_set_recolor(subtitle_label, true);
    lv_obj_set_style_text_color(subtitle_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(subtitle_label, &GENERAL_FONT_MID, 0);
    lv_obj_set_style_text_align(subtitle_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(subtitle_label, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_label_set_text(subtitle_label, "");

    subtitle_pic = lv_img_create(subtitles_obj);
    lv_img_set_src(subtitle_pic, NULL);
    lv_obj_align(subtitle_pic, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_size(subtitle_pic, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

    if (!subtitles_timer) {
        subtitles_e = -1;
        subtitles_timer = lv_timer_create(subtitles_timer_handle, 500, 0);
        lv_timer_reset(subtitles_timer);
    }
}

void del_subtitles_rect(void) {
    if (subtitles_timer) {
        show_subtitles("");
        show_subtitles_pic(NULL);
        lv_timer_pause(subtitles_timer);
        lv_timer_del(subtitles_timer);
        subtitles_timer = NULL;
        subtitle_label = NULL;
        subtitle_pic = NULL;
    }
}

#define SUB_STR_MAXSIZE 512
void show_subtitles(char *str) {
    if (!subtitle_label) {
        return;
    }
    if (!str) {
        lv_label_set_text(subtitle_label, "");
        return;
    }
    char sub_str[SUB_STR_MAXSIZE] = {0};
    string_fmt_conv_to_utf8(str, &sub_str, SUB_STR_MAXSIZE/2);
    lv_label_set_text(subtitle_label, sub_str);
    lv_obj_set_size(subtitle_label->parent, LV_PCT(80), LV_SIZE_CONTENT);
}

void show_subtitles_pic(lv_img_dsc_t *dsc) {
    if (dsc == NULL){
        if(!lv_obj_has_flag(subtitle_pic, LV_OBJ_FLAG_HIDDEN)){
            lv_obj_add_flag(subtitle_pic, LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        lv_obj_clear_flag(subtitle_pic, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_size(subtitle_pic->parent, dsc->header.w, dsc->header.h);
        lv_img_set_src(subtitle_pic, dsc);
        lv_img_cache_invalidate_src(dsc);
    }
}
#endif

void lv_pro_movie_exit(void) {
    lv_timer_del(playbar_timer);
    lv_timer_del(ctrlbar_show_timer);
#ifdef MOVIE_SUBTITLE_SUPPORT
    del_subtitles_rect();
#endif
    lv_pro_media_msg_enqueue(MSG_TYPE_MEDIA_DESTORY, NULL, false);

    if (movie_slider_group) {
        lv_group_remove_all_objs(movie_slider_group);
        lv_group_del(movie_slider_group);
        movie_slider_group = NULL;
    }
    if (movie_group) {
        lv_group_remove_all_objs(movie_group);
        lv_group_del(movie_group);
        movie_group = NULL;
    }
    if (movie_activity) {
        lv_obj_del_async(movie_activity);
        movie_activity = NULL;
    }
}

static void movie_load_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_SCREEN_LOADED) {
        lv_disp_get_default()->driver->screen_transp = 1;
        lv_disp_set_bg_opa(lv_disp_get_default(), LV_OPA_TRANSP);
        /* Empty the buffer, not emptying will cause the UI to be opaque */
        lv_memset_00(lv_disp_get_default()->driver->draw_buf->buf_act,
                lv_disp_get_default()->driver->draw_buf->size * sizeof(lv_color32_t));
        lv_obj_set_style_bg_opa(movie_activity, LV_OPA_TRANSP, 0);
    } else if (code == LV_EVENT_SCREEN_UNLOADED) {
        lv_disp_get_default()->driver->screen_transp = 0;
        lv_disp_set_bg_opa(lv_disp_get_default(), LV_OPA_COVER);
        lv_obj_set_style_bg_opa(movie_activity, LV_OPA_COVER, 0);
    }
}

void lv_pro_movie_ui_init(void) {
    movie_activity = lv_obj_create(NULL);
    movie_group = lv_group_create();
    movie_slider_group = lv_group_create();

    lv_pro_movie_static_value_init();
    lv_obj_set_size(movie_activity, lv_pct(100), lv_pct(100));
    lv_obj_add_style(movie_activity, &lv_pro_res.set_bg_black, 0);

    lv_obj_add_event_cb(movie_activity, movie_load_event_handler, LV_EVENT_SCREEN_LOADED, NULL);
    lv_obj_add_event_cb(movie_activity, movie_load_event_handler, LV_EVENT_SCREEN_UNLOADED, NULL);

#ifdef MOVIE_SUBTITLE_SUPPORT
    create_subtitles_rect();
#endif
    ctrlbar_obj = lv_obj_create(movie_activity);
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
            &player_backward,
            &player_forward,
            &player_back,
            &player_next,
            &player_stop,
            &player_sequential,
            &player_info,
            &player_list,
            &movie_setup
    };

    int imgbtn_label[] = { STR_PLAY, STR_FB, STR_FF, STR_PREV, STR_NEXT, STR_STOP,
            STR_ROUND, STR_INFO, STR_LIST, STR_SETUP };

    char *imgbtn_name[] = { "play", "backward", "forward", "pre", "next", "stop",
            "mode", "info", "list", "Auto" };

    /*Create image button*/
    for (int i = 0; i < 10; i++) {
        lv_obj_t *ctrlbar_imgbtn = lv_pro_ctrlbar_imgbtn_create(btn_cont);
        lv_obj_set_size(ctrlbar_imgbtn, CTRLBAR_BTN_WIDTH, CTRLBAR_BTN_HEIGHT);
        lv_pro_ctrlbar_imgbtn_set_src(ctrlbar_imgbtn, icon_path[i],
                lv_get_string(imgbtn_label[i]), &GENERAL_FONT_MID,
                &lv_pro_res.set_bg_transp);
        lv_obj_t *ctrlbar_imgbtn_btn = lv_pro_ctrlbar_imgbtn_get_btn(ctrlbar_imgbtn);
        if (i == 0) {
            play_imgbtn = ctrlbar_imgbtn_btn;
        }
        lv_obj_add_event_cb(ctrlbar_imgbtn_btn, movie_player_event_cb, LV_EVENT_KEY, imgbtn_name[i]);
        lv_group_add_obj(movie_group, ctrlbar_imgbtn_btn);
    }

    lv_obj_t *time_cont = lv_obj_create(ctrlbar_obj);
    lv_obj_set_size(time_cont, lv_pct(90), lv_pct(48));
    lv_obj_add_style(time_cont, &lv_pro_res.set_bg_transp, 0);
    lv_obj_align_to(time_cont, btn_cont, LV_ALIGN_OUT_TOP_MID, 0, 0);

    movie_label = lv_label_create(time_cont);
    lv_label_set_long_mode(movie_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(movie_label, lv_pct(40));
    lv_label_set_recolor(movie_label, true);
    lv_obj_set_style_text_font(movie_label, &GENERAL_FONT_MID, 0);
    if (lv_pro_res_media_get_media_list()) {
        lv_label_set_text_fmt(movie_label, "%s",
                lv_pro_res_media_get_media_list()->current_node->name);
    } else {
        lv_label_set_text_fmt(movie_label, "%s", "Not Movie Play");
    }
    lv_obj_set_style_text_color(movie_label, lv_color_white(), 0);
    lv_obj_align(movie_label, LV_ALIGN_TOP_LEFT, lv_pct(10), STATE_ALIGN);

    state_label = lv_label_create(time_cont);
    lv_label_set_long_mode(state_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_font(state_label, &GENERAL_FONT_MID, 0);
    lv_label_set_text_fmt(state_label, "%s", lv_get_string(STR_PLAY));
    lv_obj_set_style_text_color(state_label, lv_color_white(), 0);
    lv_obj_align(state_label, LV_ALIGN_TOP_MID, lv_pct(30), STATE_ALIGN);

    movie_time_bar = lv_slider_create(time_cont);
    lv_obj_set_size(movie_time_bar, lv_pct(80), TIMEBAR_HEIGHT);
    lv_obj_align(movie_time_bar, LV_ALIGN_BOTTOM_MID, 0, TIMEBAR_ALIGN);
    lv_slider_set_value(movie_time_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(movie_time_bar, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_bg_color(movie_time_bar, lv_color_hex(0xff8000), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(movie_time_bar, lv_color_hex(0xc0c0c0), LV_PART_KNOB);
    lv_obj_add_event_cb(movie_time_bar, movie_slider_event_cb, LV_EVENT_KEY, NULL);

    curtime_label = lv_label_create(time_cont);
    lv_label_set_recolor(curtime_label, true);
    lv_obj_set_style_text_font(curtime_label, &GENERAL_FONT_MID, 0);
    lv_label_set_text_fmt(curtime_label, "#ffffff %s #", "00:00");
    lv_obj_align_to(curtime_label, movie_time_bar, LV_ALIGN_OUT_LEFT_MID, 0, 0);

    totaltime_label = lv_label_create(time_cont);
    lv_label_set_recolor(totaltime_label, true);
    lv_obj_set_style_text_font(totaltime_label, &GENERAL_FONT_MID, 0);
    lv_label_set_text_fmt(totaltime_label, "#ffffff %s #", "00:00");
    lv_obj_align_to(totaltime_label, movie_time_bar, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

    show_player_ctrlbar(true);
    ctrlbar_show_timer = lv_timer_create(ctrlbar_show_timer_cb, 5000, NULL);

    playbar_timer = lv_timer_create(playbar_anim_timer, 500, NULL);

    lv_pro_media_msg_enqueue(MSG_TYPE_MEDIA_PLAY, NULL, false);
}

static int create_movie_ui(void)
{
    lv_pro_movie_ui_init();
    load_current_channel(movie_activity, movie_group);

    return 0;
}

static int destory_movie_ui(void)
{
    lv_pro_movie_info_exit();
    lv_pro_movie_list_exit();
    lv_pro_movie_exit();
    page_id_t next_page_id = get_current_page_id();
    if ((next_page_id != PAGE_FILE) && (next_page_id != PAGE_SETTING)) {
        lv_pro_file_ui_exit();
    }
    return 0;
}

static int show_movie_ui(void)
{
    if (movie_ratio == 2) {
        lv_pro_res_movie_set_ratio(Movie_Ratio_4_3);
    } else if (movie_ratio == 1) {
        lv_pro_res_movie_set_ratio(Movie_Ratio_16_9);
    }
    if (movie_info_activity) {
        lv_obj_clear_flag(movie_info_activity, LV_OBJ_FLAG_HIDDEN);
        set_current_ui_group(movie_info_group);
    } else if (movie_list_activity) {
        lv_obj_clear_flag(movie_list_activity, LV_OBJ_FLAG_HIDDEN);
        set_current_ui_group(movie_list_group);
    } else {
        set_current_ui_group(movie_group);
    }
    return 0;
}

static int hide_movie_ui(void)
{
    if (movie_info_activity) {
        lv_obj_add_flag(movie_info_activity, LV_OBJ_FLAG_HIDDEN);
    } else if (movie_list_activity) {
        lv_obj_add_flag(movie_list_activity, LV_OBJ_FLAG_HIDDEN);
    }
    return 0;
}

static page_interface_t page_movie_ui =
{
    .ops =
    {
        create_movie_ui,
        destory_movie_ui,
        show_movie_ui,
        hide_movie_ui,
    },
    .info =
    {
        .id = PAGE_MOVIE,
        .user_data = NULL
    }
};

void REGISTER_PAGE_MOVIE(void)
{
    reg_page(&page_movie_ui);
}
