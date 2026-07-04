/*
 * lv_pro_movie_list_activity.c
 *
 */

#include "lv_pro_movie_list_activity.h"
#include "lv_pro_movie_activity.h"
#include "../../widget/lv_pro_res.h"
#include "../../widget/lv_pro_media_item.h"
#include "../middle_ware/lv_pro_res_media_player_int.h"
#include "lv_drivers/indev/evdev.h"

lv_obj_t *movie_info_activity;
lv_group_t *movie_info_group;

static lv_obj_t **movie_item = NULL;
lv_obj_t *movie_list_activity;
lv_group_t *movie_list_group;

struct MovieAudio {
    unsigned int cur_audio;
    unsigned int total_audio;
};
static struct MovieAudio movie_audio;

struct MovieSubtitle {
    unsigned int cur_subtitle;
    unsigned int total_subtitle;
};
static struct MovieSubtitle movie_subtitle;


static void movie_info_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t *key = lv_event_get_param(e);
    char *user_data = (char*) lv_event_get_user_data(e);
    struct _lv_obj_t * cur_obj = lv_event_get_target(e);

    if (code == LV_EVENT_KEY) {
        if (*key == LV_KEY_ENTER) {
            if (!strcmp(user_data, "exit")) {
                lv_pro_movie_ctrlbar_timer_en(true);
                lv_pro_movie_info_exit();
                set_current_ui_group(movie_group);
            }
        } else if (*key == LV_KEY_RIGHT) {
            if (!strcmp(user_data, "audio")) {
                if (movie_audio.total_audio) {
                    ++movie_audio.cur_audio;
                    if (movie_audio.cur_audio > movie_audio.total_audio) {
                        movie_audio.cur_audio = 1;
                    }
                    lv_label_set_text_fmt(lv_obj_get_child(cur_obj, 1), "< %u/%u >",
                            movie_audio.cur_audio, movie_audio.total_audio);
                    lv_pro_res_movie_switch_audio(movie_audio.cur_audio);
                }
            } else if (!strcmp(user_data, "subtitle")) {
                if (movie_subtitle.total_subtitle) {
                    ++movie_subtitle.cur_subtitle;
                    if (movie_subtitle.cur_subtitle > movie_subtitle.total_subtitle) {
                        movie_subtitle.cur_subtitle = 1;
                    }
                    lv_label_set_text_fmt(lv_obj_get_child(cur_obj, 1), "< %u/%u >",
                            movie_subtitle.cur_subtitle, movie_subtitle.total_subtitle);
                    lv_pro_res_movie_switch_subtitle(movie_subtitle.cur_subtitle);
                }
            }
        } else if (*key == LV_KEY_LEFT) {
            if (!strcmp(user_data, "audio")) {
                if (movie_audio.total_audio) {
                    --movie_audio.cur_audio;
                    if (movie_audio.cur_audio < 1) {
                        movie_audio.cur_audio = movie_audio.total_audio;
                    }
                    lv_label_set_text_fmt(lv_obj_get_child(cur_obj, 1), "< %u/%u >",
                            movie_audio.cur_audio, movie_audio.total_audio);
                    lv_pro_res_movie_switch_audio(movie_audio.cur_audio);
                }
            } else if (!strcmp(user_data, "subtitle")) {
                if (movie_subtitle.total_subtitle) {
                    --movie_subtitle.cur_subtitle;
                    if (movie_subtitle.cur_subtitle < 1) {
                        movie_subtitle.cur_subtitle = movie_subtitle.total_subtitle;
                    }
                    lv_label_set_text_fmt(lv_obj_get_child(cur_obj, 1), "< %u/%u >",
                            movie_subtitle.cur_subtitle, movie_subtitle.total_subtitle);
                    lv_pro_res_movie_switch_subtitle(movie_subtitle.cur_subtitle);
                }
            }
        } else if (*key == LV_KEY_BACK) {
            lv_pro_movie_ctrlbar_timer_en(true);
            lv_pro_movie_info_exit();
            set_current_ui_group(movie_group);
        }
    }
}

static void movie_list_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    char *user_data = (char*) lv_event_get_user_data(e);
    uint32_t *key = lv_event_get_param(e);

    if (code == LV_EVENT_KEY) {
        if (*key == LV_KEY_ENTER) {
            if (!strcmp(user_data, "Item")) {
                list_head_t *my_media_list = lv_pro_res_media_get_media_list();
                for (int i = 0; i < list_get_total_num(my_media_list); i++) {
                    if (lv_pro_media_item_get_play(movie_item[i])) {
                        lv_pro_media_item_set_play(movie_item[i], false);
                    }
                    if (lv_obj_get_parent(obj) == movie_item[i]) {
                        lv_pro_media_item_set_play(lv_obj_get_parent(obj), true);
                        lv_pro_media_msg_enqueue(MSG_TYPE_MEDIA_PLAY_LIST, i + 1, false);
                    }
                }
            }
        } else if (*key == LV_KEY_BACK) {
            lv_pro_movie_ctrlbar_timer_en(true);
            lv_pro_movie_list_exit();
            set_current_ui_group(movie_group);
        }
    }
}

#ifdef MOVIE_SUBTITLE_SUPPORT
#include "../file_explorer/lv_pro_file_explorer.h"
#include "../middle_ware/glist.h"
static glist* ext_subtitle_format_handle(glist* subs_glist)
{
    glist* dst_glist = NULL;
    // scan subglist ,if subsglist has a .idx
    if (subs_glist != NULL) {
        dst_glist = subs_glist;
        for(int i = 0; i < glist_length(subs_glist); i++) {
            char *file_str = (char *)glist_nth_data(subs_glist,i);
            if (strstr(file_str, ".idx")) {
                char file_without_ext[1024] = {0};
                lv_pro_file_rm_extension(file_without_ext, file_str);
                for(int j = 0; j < glist_length(subs_glist); j++) {
                    if (!strncmp(file_without_ext, (char *)glist_nth_data(subs_glist, j),
                                strlen(file_without_ext))) {
                        // finded samename in glist
                        if (lv_pro_file_optional_filter((char *)glist_nth_data(subs_glist, j), "sub")) {
                            dst_glist = glist_delete_link(subs_glist, glist_nth(subs_glist, j));
                        }
                    }
                }
            }
        }
    }
    return dst_glist;
}
static ext_subtitle_t ext_subtitle;
char *m_uris[MAX_EXT_SUBTITLE_NUM] = {NULL};
#define SUB_MAX_FILE_NAME 1024

int ext_subtitles_init(void)
{
    int j = 0;
    list_head_t *movie_list = lv_pro_res_media_get_media_list();
    if (movie_list == NULL) {
        return 0;
    }
    glist* subs_list = lv_pro_file_subtitile_list_get();
    subs_list=ext_subtitle_format_handle(subs_list);
    if (subs_list != NULL) {
        int len=glist_length(subs_list);
        list_node_t * file_node = movie_list->current_node;
        if (file_node->name != NULL) {
            char file_without_ext[SUB_MAX_FILE_NAME] = {0};
            lv_pro_file_rm_extension(file_without_ext, file_node->name);
            char* sub_without_ext = (char*)malloc(SUB_MAX_FILE_NAME);
            if (!sub_without_ext) {
                return -1;
            }
            for(int i = 0; i < len; i++) {
                memset(sub_without_ext, 0, SUB_MAX_FILE_NAME);
                lv_pro_file_rm_extension(sub_without_ext, (char*)glist_nth_data(subs_list, i));
                if (!strcmp(sub_without_ext, file_without_ext)) {
                    ext_subtitle.ext_subs_count++;
                    char url_single[SUB_MAX_FILE_NAME] = {0};
                    lv_pro_file_get_fullname(url_single, movie_list->current_node->path,
                            glist_nth_data(subs_list, i));
                    m_uris[j] = strdup(url_single);
                    j++;
                    ext_subtitle.uris = m_uris;
                }
            }
            free(sub_without_ext);
        }
    }
    return 0;
}
int ext_subtitle_deinit(void)
{
    glist* subs_list = lv_pro_file_subtitile_list_get();
    if (subs_list != NULL && ext_subtitle.ext_subs_count != 0) {
            for(int i = 0; i < ext_subtitle.ext_subs_count; i++) {
                free(m_uris[i]);
                m_uris[i] = NULL;
            }
        memset(&ext_subtitle, 0, sizeof(ext_subtitle_t));
    }
    return 0;
}
ext_subtitle_t * ext_subtitle_data_get(void)
{
    return &ext_subtitle;
}
#endif

void lv_pro_movie_info_init(void) {
    movie_info_activity = lv_obj_create(lv_layer_top());
    lv_obj_set_size(movie_info_activity, LV_PCT(INFO_WIDTH_PCT), LV_PCT(INFO_HEIGHT_PCT));
    lv_obj_align(movie_info_activity, LV_ALIGN_TOP_RIGHT, 0, LV_PCT(INFO_ALIGN_PCT));
    lv_obj_set_style_bg_opa(movie_info_activity, LV_OPA_50, 0);
    lv_obj_add_style(movie_info_activity, &lv_pro_res.set_bg_black, 0);
    lv_obj_add_flag(movie_info_activity, LV_OBJ_FLAG_USER_2);
    movie_info_group = lv_group_create();

    lv_obj_t *title_cont = lv_obj_create(movie_info_activity);
    lv_obj_set_size(title_cont, LV_PCT(100), LV_PCT(17));
    lv_obj_add_style(title_cont, &lv_pro_res.set_bg_transp, 0);
    lv_obj_align(title_cont, LV_ALIGN_TOP_MID, 0, 0);

    list_head_t *my_media_list = lv_pro_res_media_get_media_list();
    lv_obj_t *title_label = lv_label_create(title_cont);
    lv_obj_set_style_text_font(title_label, &GENERAL_FONT_MID, 0);
    lv_label_set_recolor(title_label, true);
    lv_label_set_text_fmt(title_label, "%s", my_media_list->current_node->name);
    lv_obj_set_style_text_color(title_label, lv_color_white(), 0);
    lv_label_set_long_mode(title_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_align(title_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(title_label, LV_PCT(90));
    lv_obj_center(title_label);

    lv_obj_t *main_cont = lv_obj_create(movie_info_activity);
    lv_obj_align_to(main_cont, title_cont, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_set_size(main_cont, LV_PCT(100), LV_PCT(66));
    lv_obj_set_style_border_width(main_cont, 0, 0);
    lv_obj_set_style_pad_all(main_cont, 0, 0);
    lv_obj_set_style_pad_row(main_cont, 2, 0);
    lv_obj_set_style_pad_top(main_cont, 5, 0);
    lv_obj_set_style_pad_bottom(main_cont, 5, 0);
    lv_obj_set_style_radius(main_cont, 0, 0);
    lv_obj_set_style_shadow_width(main_cont, 0, 0);
    lv_obj_set_scrollbar_mode(main_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_style(main_cont, &lv_pro_res.set_bg_transp, 0);
    lv_obj_set_flex_flow(main_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_add_flag(main_cont, LV_OBJ_FLAG_USER_2);

    struct MovieInfo mi;
    memset(&mi, 0, sizeof(struct MovieInfo));
    char audio[10];
    char subtitle[10];
    lv_pro_res_movie_get_info(&mi);
    if(mi.audio > 0) {
        if (!movie_audio.total_audio) {
            movie_audio.cur_audio = 1;
            movie_audio.total_audio = mi.audio;
        }
        snprintf(audio, sizeof(audio), "< %u/%u >", movie_audio.cur_audio, movie_audio.total_audio);
    } else {
        movie_audio.cur_audio = 0;
        movie_audio.total_audio = 0;
        strcpy(audio, "< --/-- >");
    }

    if(mi.subtitle > 0) {
        if (!movie_subtitle.total_subtitle) {
            movie_subtitle.cur_subtitle = 1;
            movie_subtitle.total_subtitle = mi.subtitle;
        }
        snprintf(subtitle, sizeof(subtitle), "< %u/%u >", movie_subtitle.cur_subtitle,
                movie_subtitle.total_subtitle);
    } else {
        movie_subtitle.cur_subtitle = 0;
        movie_subtitle.total_subtitle = 0;
        strcpy(subtitle, "< --/-- >");
    }

    lv_pro_media_info_create_text(main_cont, &GENERAL_FONT_MID, lv_get_string(STR_INFO_RES), mi.resolution);
    lv_obj_t *audio_cont = lv_pro_media_info_create_text(main_cont, &GENERAL_FONT_MID,
            lv_get_string(STR_INFO_AUDIO), audio);
    lv_obj_t *subtitle_cont = lv_pro_media_info_create_text(main_cont, &GENERAL_FONT_MID,
            lv_get_string(STR_INFO_SUBTITLE), subtitle);
    lv_pro_media_info_create_text(main_cont, &GENERAL_FONT_MID, lv_get_string(STR_INFO_SIZE), mi.filesize);

    lv_group_add_obj(movie_info_group, audio_cont);
    lv_group_add_obj(movie_info_group, subtitle_cont);
    lv_obj_add_event_cb(audio_cont, movie_info_event_cb, LV_EVENT_KEY, "audio");
    lv_obj_add_event_cb(subtitle_cont, movie_info_event_cb, LV_EVENT_KEY, "subtitle");

    lv_obj_t *exit_cont = lv_btn_create(movie_info_activity);
    lv_obj_set_size(exit_cont, LV_PCT(100), LV_PCT(17));
    lv_obj_set_style_border_width(exit_cont, 0, 0);
    lv_obj_set_style_pad_all(exit_cont, 0, 0);
    lv_obj_set_style_radius(exit_cont, 0, 0);
    lv_obj_set_style_shadow_width(exit_cont, 0, 0);
    lv_obj_set_style_bg_color(exit_cont, lv_color_hex(0x0000ff), 0);
    lv_obj_set_style_bg_opa(exit_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_opa(exit_cont, LV_OPA_50, LV_STATE_FOCUS_KEY);
    lv_obj_align(exit_cont, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_obj_t *exit_label = lv_label_create(exit_cont);
    lv_obj_set_style_text_font(exit_label, &GENERAL_FONT_MID, 0);
    lv_label_set_recolor(exit_label, true);
    lv_label_set_text_fmt(exit_label, "#ffffff %s #", lv_get_string(STR_INFO_EXIT));
    lv_obj_center(exit_label);

    lv_group_add_obj(movie_info_group, exit_cont);
    lv_obj_add_event_cb(exit_cont, movie_info_event_cb, LV_EVENT_KEY, "exit");
    lv_group_focus_obj(exit_cont);
}

void lv_pro_movie_info_exit(void) {
    if (movie_info_group) {
        lv_group_remove_all_objs(movie_info_group);
        lv_group_del(movie_info_group);
        movie_info_group = NULL;
        lv_group_set_default(NULL);
    }
    if (movie_info_activity) {
        lv_obj_del(movie_info_activity);
        movie_info_activity = NULL;
    }
}

void lv_pro_movie_refresh_info(void) {
    if (movie_audio.total_audio || movie_subtitle.total_subtitle) {
        memset(&movie_audio, 0, sizeof(struct MovieAudio));
        memset(&movie_subtitle, 0, sizeof(struct MovieSubtitle));
    }
    if (movie_info_activity) {
        lv_obj_t *title_label = lv_obj_get_child(lv_obj_get_child(movie_info_activity, 0), 0);
        list_head_t *my_media_list = lv_pro_res_media_get_media_list();
        lv_label_set_text_fmt(title_label, "%s", my_media_list->current_node->name);
        lv_obj_t *cont0 = lv_obj_get_child(lv_obj_get_child(movie_info_activity, 1), 0);
        lv_obj_t *cont1 = lv_obj_get_child(lv_obj_get_child(movie_info_activity, 1), 1);
        lv_obj_t *cont2 = lv_obj_get_child(lv_obj_get_child(movie_info_activity, 1), 2);
        lv_obj_t *cont3 = lv_obj_get_child(lv_obj_get_child(movie_info_activity, 1), 3);
        struct MovieInfo mi;
        memset(&mi, 0, sizeof(struct MovieInfo));
        char audio[10];
        char subtitle[10];
        lv_pro_res_movie_get_info(&mi);
        if(mi.audio > 0) {
            movie_audio.cur_audio = 1;
            movie_audio.total_audio = mi.audio;
            snprintf(audio, sizeof(audio), "< 1/%u >", movie_audio.total_audio);
        } else {
            movie_audio.cur_audio = 0;
            movie_audio.total_audio = 0;
            strcpy(audio, "< --/-- >");
        }

        if(mi.subtitle > 0) {
            movie_subtitle.cur_subtitle = 1;
            movie_subtitle.total_subtitle = mi.subtitle;
            snprintf(subtitle, sizeof(subtitle), "< 1/%u >", movie_subtitle.total_subtitle);
        } else {
            movie_subtitle.cur_subtitle = 0;
            movie_subtitle.total_subtitle = 0;
            strcpy(subtitle, "< --/-- >");
        }

        lv_label_set_text(lv_obj_get_child(cont0, 1), mi.resolution);
        lv_label_set_text(lv_obj_get_child(cont1, 1), audio);
        lv_label_set_text(lv_obj_get_child(cont2, 1), subtitle);
        lv_label_set_text(lv_obj_get_child(cont3, 1), mi.filesize);
    }
}


void lv_pro_movie_list_init(void) {
    movie_list_activity = lv_obj_create(lv_layer_top());
    lv_obj_set_size(movie_list_activity, LV_PCT(LIST_WIDTH_PCT), LV_PCT(LIST_HEIGHT_PCT));
    lv_obj_align(movie_list_activity,  LV_ALIGN_TOP_RIGHT, 0, LV_PCT(LIST_ALIGN_PCT));
    lv_obj_set_style_bg_opa(movie_list_activity, LV_OPA_50, 0);
    lv_obj_add_style(movie_list_activity, &lv_pro_res.set_bg_black, 0);
    movie_list_group = lv_group_create();

    lv_obj_t *main_cont = lv_obj_create(movie_list_activity);
    lv_obj_align(main_cont, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_size(main_cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_border_width(main_cont, 0, 0);
    lv_obj_set_style_pad_all(main_cont, 5, 0);
    lv_obj_set_style_pad_row(main_cont, 2, 0);
    lv_obj_set_style_pad_top(main_cont, 5, 0);
    lv_obj_set_style_pad_bottom(main_cont, 5, 0);
    lv_obj_set_style_radius(main_cont, 0, 0);
    lv_obj_set_style_shadow_width(main_cont, 0, 0);
    lv_obj_set_scrollbar_mode(main_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_style(main_cont, &lv_pro_res.set_bg_transp, 0);
    lv_obj_set_flex_flow(main_cont, LV_FLEX_FLOW_COLUMN);

    list_head_t *my_media_list = lv_pro_res_media_get_media_list();
    int cur_index = list_get_cur_index(my_media_list);
    int total_items = list_get_total_num(my_media_list);
    movie_item = (lv_obj_t **)malloc(total_items * sizeof(lv_obj_t *));
    if (movie_item == NULL) {
        return;
    }
    list_skip_to_head(my_media_list);

    const char *playing_img = "A:/usr/share/lv_projector/media_playing01.png";

    for (int i = 0; i < total_items; i++) {
        movie_item[i] = lv_pro_media_item_create(main_cont);
        lv_obj_set_size(movie_item[i], LV_PCT(100), LV_PCT(15));
        lv_pro_media_item_set_src(movie_item[i], &GENERAL_FONT_MID,
                &lv_pro_res.set_bg_transp, playing_img,
                my_media_list->current_node->name);
        lv_obj_add_event_cb(movie_item[i], movie_list_event_cb, LV_EVENT_KEY, "Item");
        lv_obj_t *movie_item_btn = lv_pro_media_item_get_btn(movie_item[i]);
        lv_group_add_obj(movie_list_group, movie_item_btn);
        if (i == cur_index - 1) {
            lv_group_focus_obj(movie_item_btn);
        }
        list_get_next_node(my_media_list);
    }

    lv_pro_media_item_set_play(movie_item[cur_index - 1], true);

    list_skip_to_index(my_media_list, cur_index);
}

void lv_pro_movie_list_exit(void) {
    if (movie_list_group) {
        lv_group_remove_all_objs(movie_list_group);
        lv_group_del(movie_list_group);
        movie_list_group = NULL;
        lv_group_set_default(NULL);
    }
    if (movie_list_activity) {
        lv_obj_del(movie_list_activity);
        movie_list_activity = NULL;
    }
    if (movie_item)
    {
        free(movie_item);
        movie_item = NULL;
    }
}

void lv_pro_movie_refresh_list(void) {
    if (movie_list_activity) {
        list_head_t *my_media_list = lv_pro_res_media_get_media_list();
        int cur_index = list_get_cur_index(my_media_list);
        int pre_index = cur_index == 1 ? list_get_total_num(my_media_list) : cur_index-1;
        lv_pro_media_item_set_play(movie_item[pre_index - 1], false);
        lv_pro_media_item_set_play(movie_item[cur_index - 1], true);
    }
}
