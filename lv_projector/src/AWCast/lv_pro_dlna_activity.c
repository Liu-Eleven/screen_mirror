/*
 * lv_pro_dlna_activity.c
 *
 */

#include "../Media/lv_pro_media_layout.h"
#include "../WirelessSP/lv_pro_wirelesssp.h"
#include "../lv_pro_launcher.h"
#include "../widget/lv_pro_ctrlbar_imgbtn.h"
#include "iscrctl_draw.h"
#include "widget/lv_pro_res.h"
#define LOG_TAG "DLNA_ACT"
#define CONFIG_LOG_LEVEL LOG_LEVEL_DEBUG
#include <cdx_log.h>
#include <irender.h>

static lv_timer_t *timestamp_timer;
static lv_timer_t *ctrlbar_show_timer;

static lv_obj_t *dlna_activity;
static lv_obj_t *play_imgbtn;
static lv_obj_t *time_bar;
static lv_obj_t *movie_lable;
static lv_obj_t *curtime_lable;
static lv_obj_t *totaltime_lable;
static lv_obj_t *ctrlbar_obj;
static lv_obj_t *buff_bar;

static lv_group_t *dlna_group;
static RenderT *dlna_render;
static lv_anim_t buff_anim;
static bool dlna_activity_exit = true;
static int SeekTime = 0;
static int64_t last_seek_op_time = 0;

void lv_pro_dlna_set_seek(bool ward)
{
    uint32_t total_time = 0;
    uint32_t cur_play_time = 0;
    uint32_t jump_interval = 0;
    uint32_t seek_time = 0;
    int64_t cur_op_time = 0;
    struct timeval time_t;

    total_time = RenderGetDuration(dlna_render);
    cur_play_time = RenderGetPosition(dlna_render);

    if (total_time < jump_interval)
        return ;
    if (cur_play_time >= total_time)
        return;

    gettimeofday(&time_t, NULL);
    cur_op_time = time_t.tv_sec * 1000 + time_t.tv_usec / 1000;

    if ((cur_op_time - last_seek_op_time) < 800)
        SeekTime += 10000;
    else
        SeekTime = 10000;

    last_seek_op_time = cur_op_time;

    if (SeekTime < 300000)
        jump_interval = SeekTime;
    else
        jump_interval = 300000;

    if (ward) { // set forward
        if ((cur_play_time + jump_interval) > total_time)
            seek_time = total_time;
        else
            seek_time = cur_play_time + jump_interval;
    } else { //set backward
        if (cur_play_time > jump_interval)
            seek_time = cur_play_time - jump_interval;
        else
            seek_time = 0;
    }
    RenderSeekto(dlna_render,seek_time);
}

static void show_player_ctrlbar(bool show)
{
    if (!ctrlbar_obj) return;

    if (show) {
        lv_obj_clear_flag(ctrlbar_obj, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(ctrlbar_obj, LV_OBJ_FLAG_HIDDEN);
    }
    lv_memset_00(lv_disp_get_default()->driver->draw_buf->buf_act,
                 lv_disp_get_default()->driver->draw_buf->size * sizeof(lv_color32_t));
}

static void ctrlbar_show_timer_cb(lv_timer_t *timer)
{
    lv_timer_pause(timer);
    lv_timer_reset(timer);
    show_player_ctrlbar(false);
}

static int __LV_SrcctlDraw_VideoShow(SrcctlDrawT *scd)
{
    CDX_LOGI(" '%s' entry ", __FUNCTION__);
    CDX_UNUSE(scd);
    if (dlna_activity_exit) return;

    lv_pro_dlna_timer_en(true);
    show_player_ctrlbar(true);
    lv_pro_dlna_ctrlbar_timer_en(true);

    return 0;
}

void create_buffering_anim(lv_obj_t *parent)
{
    buff_bar = lv_bar_create(parent);
    lv_obj_set_size(buff_bar, 120, 120);
    lv_obj_center(buff_bar);

    lv_obj_set_style_radius(buff_bar, 50, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(buff_bar, LV_OPA_30, 0);
    lv_obj_set_style_bg_grad_dir(buff_bar, LV_GRAD_DIR_HOR, LV_PART_MAIN);
    lv_obj_set_style_bg_color(buff_bar, lv_color_hex(0xff8000), LV_PART_INDICATOR);

    lv_anim_init(&buff_anim);
    lv_anim_set_var(&buff_anim, buff_bar);
    lv_anim_set_values(&buff_anim, 0, 100);
    lv_anim_set_time(&buff_anim, 1000);
    lv_anim_set_playback_time(&buff_anim, 0);
    lv_anim_set_repeat_count(&buff_anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&buff_anim, lv_anim_path_ease_in_out);
    lv_anim_set_exec_cb(&buff_anim, (lv_anim_exec_xcb_t)lv_bar_set_value);

    lv_obj_t *label = lv_label_create(buff_bar);
    lv_label_set_text(label, "buffering");
    lv_obj_center(label);

    lv_obj_set_style_text_font(label, &GENERAL_FONT_MID, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0x000000), 0);
    lv_obj_set_size(label, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
}

static void start_buffering_anim()
{
    lv_anim_start(&buff_anim);
    if (buff_bar) lv_obj_clear_flag(buff_bar, LV_OBJ_FLAG_HIDDEN);
}
static void stop_buffering_anim()
{
    lv_anim_del(&buff_anim, NULL); /*exec_cb == NULL would delete all animations of var*/
    if (buff_bar) lv_obj_add_flag(buff_bar, LV_OBJ_FLAG_HIDDEN);
    lv_memset_00(lv_disp_get_default()->driver->draw_buf->buf_act,
                 lv_disp_get_default()->driver->draw_buf->size * sizeof(lv_color32_t));
}

static int __LV_SrcctlDraw_EntryShow(SrcctlDrawT *scd, const char *movie_name)
{
    CDX_LOGI(" '%s' entry, mn:%s ", __FUNCTION__, movie_name);
    if (dlna_activity_exit) {  // 确保资源已经退出
        pthread_mutex_lock(&lvgl_mutex);
        lv_pro_dlna_ui_init();
        hidden_wirelesssp_acitvity(true);
        dlna_activity_exit = false;
        pthread_mutex_unlock(&lvgl_mutex);
        return 0;
    } else {
        CDX_LOGE(" '%s' fail!!, mn:%s ", __FUNCTION__, movie_name);
        return -1;
    }
}

static int __LV_SrcctlDraw_EntryHide(SrcctlDrawT *scd)
{
    CDX_LOGI(" '%s' entry ", __FUNCTION__);
    return 0;
}

static int __LV_SrcctlDraw_ProcessBarShow(SrcctlDrawT *scd, int duration, int position)
{
    CDX_LOGI(" '%s' entry ", __FUNCTION__);
    lv_pro_dlna_btnimg_setplaystate(0);
    return 0;
}

static int __LV_SrcctlDraw_ProcessBarHide(SrcctlDrawT *scd)
{
    CDX_LOGI(" '%s' entry ", __FUNCTION__);
    lv_pro_dlna_btnimg_setplaystate(1);
    return 0;
}

static int __LV_SrcctlDraw_VolumeBarShow(SrcctlDrawT *scd, int vol)
{
    CDX_LOGI(" '%s' entry ", __FUNCTION__);
    return 0;
}

static int __LV_SrcctlDraw_VolumeBarHide(SrcctlDrawT *scd)
{
    CDX_LOGI(" '%s' entry ", __FUNCTION__);
    return 0;
}

static int __LV_SrcctlDraw_BufferingStart(SrcctlDrawT *scd)
{
    CDX_LOGI(" '%s' entry ", __FUNCTION__);
    if (dlna_activity_exit) return;
    start_buffering_anim();
    return 0;
}

static int __LV_SrcctlDraw_BufferingEnd(SrcctlDrawT *scd)
{
    CDX_LOGI(" '%s' entry ", __FUNCTION__);
    if (dlna_activity_exit) return;
    pthread_mutex_lock(&lvgl_mutex);
    stop_buffering_anim();
    pthread_mutex_unlock(&lvgl_mutex);
    return 0;
}

static int __LV_SrcctlDraw_Control(SrcctlDrawT *scd, int cmd, void *param)
{
    CDX_LOGI(" '%s' entry, %p, cmd:%d", __FUNCTION__, scd, cmd);
    if (dlna_activity_exit) return;

    switch (cmd) {
    case DLNA_EVENT_QUIT:  // todo, need to return to wireless UI
    {
        pthread_mutex_lock(&lvgl_mutex);
        DLNA_Quit();
        AWCast_restart_MiracastAirplay();
        pthread_mutex_unlock(&lvgl_mutex);
        break;
    }

    default: {
        break;
    }
    }

    return 0;
}

struct SrcctlDrawOpsS lv_scd_ops = {
    .video_show = __LV_SrcctlDraw_VideoShow,
    .entry_show = __LV_SrcctlDraw_EntryShow,
    .entry_hide = __LV_SrcctlDraw_EntryHide,
    .process_bar_show = __LV_SrcctlDraw_ProcessBarShow,
    .process_bar_hide = __LV_SrcctlDraw_ProcessBarHide,
    .volume_bar_show = __LV_SrcctlDraw_VolumeBarShow,
    .volume_bar_hide = __LV_SrcctlDraw_VolumeBarHide,
    .buffering_start = __LV_SrcctlDraw_BufferingStart,
    .buffering_end = __LV_SrcctlDraw_BufferingEnd,
    .control = __LV_SrcctlDraw_Control,
};

struct SrcctlDrawS lv_scd;

SrcctlDrawT *LV_SrcctlDraw_Instance()
{
    lv_scd.ops = &lv_scd_ops;
    return &lv_scd;
}

static int timems2String(char *buf, int val)
{
    int h;
    int mm;
    int secs;

    secs = val / 1000;
    mm = secs / 60;
    h = mm / 60;

    secs = secs % 60;
    mm = mm % 60;

    sprintf(buf, "%02d:%02d:%02d", h, mm, secs);

    return 0;
}

static void anim_timer(lv_timer_t *timer)
{
    char totaltime[9] = {0};
    char curtime[9] = {0};
    int duration;
    int position;

    duration = RenderGetDuration(dlna_render);
    position = RenderGetPosition(dlna_render);
    //CDX_LOGD("anim_timer, dura:%d, pos:%d", duration, position);
    lv_bar_set_value(time_bar, (position * 100) / duration, LV_ANIM_OFF);

    timems2String(totaltime, duration);
    timems2String(curtime, position);

    lv_label_set_text_fmt(movie_lable, "#ffffff DLNA #");
    lv_label_set_text_fmt(curtime_lable, "#ffffff %s #", curtime);
    lv_label_set_text_fmt(totaltime_lable, "#ffffff %s #", totaltime);
}

static void dlna_player_event_cb(lv_event_t *e)
{
    if (dlna_activity_exit) return;

    lv_event_code_t code = lv_event_get_code(e);
    uint32_t *key = lv_event_get_param(e);
    lv_obj_t *obj = lv_event_get_target(e);
    char *user_data = (char *)lv_event_get_user_data(e);

    if (code == LV_EVENT_KEY) {
        CDX_LOGD("LV_EVENT_KEY *key=%u", *key);

        // 2. check hidden
        if (lv_obj_has_flag(ctrlbar_obj, LV_OBJ_FLAG_HIDDEN)) {
            lv_pro_dlna_timer_en(true);
            show_player_ctrlbar(true);
            lv_pro_dlna_ctrlbar_timer_en(true);
            // return;
        }
        if (!is_global_key_go_new_channel(*key)) {
            lv_pro_dlna_del_render();
            DLNA_Quit();
            AWCast_exit();
            return;
        } else if (*key == LV_KEY_BACK) {
            lv_pro_dlna_del_render();
            DLNA_Quit();
            AWCast_StopService();
            AWCast_exit();
            AWCast_init();
            AWCast_StartService();
            return;
        } else if (*key == LV_KEY_LEFT) {
            lv_pro_dlna_set_seek(0);
            return;
        } else if (*key == LV_KEY_RIGHT) {
            lv_pro_dlna_set_seek(1);
            return;
        } else if (*key == LV_KEY_ENTER) {
            if (!strcmp(user_data, "play")) {
                lv_pro_dlna_playstate_changed();
            }
            return;
        }
    }
}

void lv_pro_dlna_timer_en(bool en)
{
    if (!play_imgbtn || !dlna_render) return;
    lv_pro_ctrlbar_imgbtn_t *btn = (lv_pro_ctrlbar_imgbtn_t *)lv_obj_get_parent(play_imgbtn);

    if (en) {
        if (RenderGetState(dlna_render) == RENDER_STATE_PLAYING) {
            if (timestamp_timer) lv_timer_resume(timestamp_timer);
            lv_img_set_src(btn->img, &player_playing);
            lv_label_set_text_fmt(btn->name, "%s", lv_get_string(STR_PLAY));
        }
    } else {
        if (timestamp_timer) lv_timer_pause(timestamp_timer);
        lv_img_set_src(btn->img, &player_pause);
        lv_label_set_text_fmt(btn->name, "%s", lv_get_string(STR_PAUSE));
    }
}

void lv_pro_dlna_playstate_changed()
{
    if (!play_imgbtn || !dlna_render) return;
    lv_pro_ctrlbar_imgbtn_t *btn = (lv_pro_ctrlbar_imgbtn_t *)lv_obj_get_parent(play_imgbtn);

    if (RenderGetState(dlna_render) == RENDER_STATE_PAUSE) {
        RenderResume(dlna_render);
        if (timestamp_timer) lv_timer_resume(timestamp_timer);
        lv_img_set_src(btn->img, &player_playing);
        lv_label_set_text_fmt(btn->name, "%s", lv_get_string(STR_PLAY));
    } else {
        RenderPause(dlna_render);
        if (timestamp_timer) lv_timer_pause(timestamp_timer);
        lv_img_set_src(btn->img, &player_pause);
        lv_label_set_text_fmt(btn->name, "%s", lv_get_string(STR_PAUSE));
    }
}

void lv_pro_dlna_btnimg_setplaystate(int play)
{
    if (!play_imgbtn) return;
    lv_pro_ctrlbar_imgbtn_t *btn = (lv_pro_ctrlbar_imgbtn_t *)lv_obj_get_parent(play_imgbtn);

    if (play) {
        if (timestamp_timer) lv_timer_resume(timestamp_timer);
        lv_img_set_src(btn->img, &player_playing);
        lv_label_set_text_fmt(btn->name, "%s", lv_get_string(STR_PLAY));
    } else {
        if (timestamp_timer) lv_timer_pause(timestamp_timer);
        lv_img_set_src(btn->img, &player_pause);
        lv_label_set_text_fmt(btn->name, "%s", lv_get_string(STR_PAUSE));
    }
}

void lv_pro_dlna_ctrlbar_timer_en(bool en)
{
    if (!ctrlbar_show_timer) return;

    if (en) {
        lv_timer_reset(ctrlbar_show_timer);
        lv_timer_resume(ctrlbar_show_timer);
    } else {
        lv_timer_pause(ctrlbar_show_timer);
        lv_timer_reset(ctrlbar_show_timer);
    }
}

void lv_pro_dlna_ui_init(void)
{
    CDX_LOGI(" %s: uiinit \n", __func__);
    dlna_activity = lv_obj_create(lv_layer_top());
    dlna_group = lv_group_create();
    lv_group_set_default(NULL);
    set_current_ui_group(dlna_group);

    lv_obj_set_size(dlna_activity, lv_pct(100), lv_pct(100));
    lv_obj_add_style(dlna_activity, &lv_pro_res.set_bg_black, 0);
    lv_obj_set_style_bg_opa(dlna_activity, LV_OPA_TRANSP, 0);

    ctrlbar_obj = lv_obj_create(dlna_activity);
    lv_obj_set_size(ctrlbar_obj, lv_pct(100), lv_pct(CTRLBAR_PCT));
    lv_obj_align(ctrlbar_obj, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_style(ctrlbar_obj, &lv_pro_res.set_bg_black, 0);
    lv_obj_set_style_bg_opa(ctrlbar_obj, LV_OPA_30, 0);

    lv_obj_t *btn_cont = lv_obj_create(ctrlbar_obj);
    lv_obj_set_size(btn_cont, lv_pct(90), lv_pct(52));
    lv_obj_align(btn_cont, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_style(btn_cont, &lv_pro_res.set_bg_transp, 0);

    lv_obj_set_style_pad_column(btn_cont, CTRLBAR_BTN_COL, 0);
    lv_obj_set_style_pad_all(btn_cont, 5, 0);
    lv_obj_set_scrollbar_mode(btn_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(btn_cont, LV_FLEX_FLOW_ROW);

    const lv_img_dsc_t *icon_path[] = {&player_playing};
    int imgbtn_label[] = {STR_PLAY};
    char *imgbtn_name[] = {"play"};

    lv_obj_t *ctrlbar_imgbtn = lv_pro_ctrlbar_imgbtn_create(btn_cont);
    lv_obj_set_size(ctrlbar_imgbtn, CTRLBAR_BTN_WIDTH, CTRLBAR_BTN_HEIGHT);
    lv_pro_ctrlbar_imgbtn_set_src(ctrlbar_imgbtn, icon_path[0], lv_get_string(imgbtn_label[0]),
                                  &GENERAL_FONT_MID, &lv_pro_res.set_bg_transp);

    lv_obj_t *ctrlbar_imgbtn_btn = lv_pro_ctrlbar_imgbtn_get_btn(ctrlbar_imgbtn);

    play_imgbtn = ctrlbar_imgbtn_btn;
    // 设置按下状态样式
    lv_obj_set_style_bg_color(play_imgbtn, lv_color_hex(0xeea87c), LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(play_imgbtn, LV_OPA_50, 0);
    lv_obj_add_event_cb(ctrlbar_imgbtn, dlna_player_event_cb, LV_EVENT_KEY, imgbtn_name[0]);
    lv_group_focus_obj(ctrlbar_imgbtn);

    lv_obj_t *time_cont = lv_obj_create(ctrlbar_obj);
    lv_obj_set_size(time_cont, lv_pct(90), lv_pct(48));
    lv_obj_add_style(time_cont, &lv_pro_res.set_bg_transp, 0);
    lv_obj_align_to(time_cont, btn_cont, LV_ALIGN_OUT_TOP_MID, 0, 0);

    movie_lable = lv_label_create(time_cont);
    lv_label_set_long_mode(movie_lable, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(movie_lable, lv_pct(40));
    lv_label_set_recolor(movie_lable, true);
    lv_obj_set_style_text_font(movie_lable, &GENERAL_FONT_MID, 0);
    lv_label_set_text_fmt(movie_lable, "#ffffff %s #", "No Movie Play");
    lv_obj_align(movie_lable, LV_ALIGN_TOP_LEFT, lv_pct(10), STATE_ALIGN);

    time_bar = lv_bar_create(time_cont);
    lv_obj_set_size(time_bar, lv_pct(80), 20);
    lv_obj_align(time_bar, LV_ALIGN_BOTTOM_MID, 0, -35);
    lv_bar_set_value(time_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(time_bar, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_bg_color(time_bar, lv_color_hex(0xff8000), LV_PART_INDICATOR);

    curtime_lable = lv_label_create(time_cont);
    lv_label_set_recolor(curtime_lable, true);
    lv_obj_set_style_text_font(curtime_lable, &GENERAL_FONT_MID, 0);
    lv_label_set_text_fmt(curtime_lable, "#ffffff %s #", "00:00:00");
    lv_obj_align_to(curtime_lable, time_bar, LV_ALIGN_OUT_LEFT_MID, 0, 0);

    totaltime_lable = lv_label_create(time_cont);
    lv_label_set_recolor(totaltime_lable, true);
    lv_obj_set_style_text_font(totaltime_lable, &GENERAL_FONT_MID, 0);
    lv_label_set_text_fmt(totaltime_lable, "#ffffff %s #", "00:00:00");
    lv_obj_align_to(totaltime_lable, time_bar, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

    create_buffering_anim(dlna_activity);

    show_player_ctrlbar(true);
    ctrlbar_show_timer = lv_timer_create(ctrlbar_show_timer_cb, 5000, NULL);

    timestamp_timer = lv_timer_create(anim_timer, 100, NULL);
    lv_timer_pause(timestamp_timer);

    lv_disp_get_default()->driver->screen_transp = 1;
    lv_disp_set_bg_opa(lv_disp_get_default(), LV_OPA_TRANSP);
    /* Empty the buffer, not emptying will cause the UI to be opaque */
    lv_memset_00(lv_disp_get_default()->driver->draw_buf->buf_act,
                 lv_disp_get_default()->driver->draw_buf->size * sizeof(lv_color32_t));
    lv_obj_set_style_bg_opa(dlna_activity, LV_OPA_TRANSP, 0);
}

void lv_pro_dlna_ui_deinit(void)
{
    CDX_LOGI(" %s: uideinit \n", __func__);

    stop_buffering_anim();

    if (timestamp_timer) {
        lv_timer_pause(timestamp_timer);
        lv_timer_del(timestamp_timer);
        timestamp_timer = NULL;
    }
    if (ctrlbar_show_timer) {
        lv_timer_pause(ctrlbar_show_timer);
        lv_timer_del(ctrlbar_show_timer);
        ctrlbar_show_timer = NULL;
    }
    if (dlna_group) {
        lv_group_remove_all_objs(dlna_group);
        lv_group_del(dlna_group);
        dlna_group = NULL;
    }
    if (dlna_activity) {
        lv_obj_del(dlna_activity);
        dlna_activity = NULL;

        // clear
        time_bar = NULL;
        movie_lable = NULL;
        curtime_lable = NULL;
        totaltime_lable = NULL;
        ctrlbar_obj = NULL;
    }
}

void hidden_wirelesssp_acitvity(bool hidden)
{
    if (hidden) {
        lv_obj_add_flag(WirelessSP_activity, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(WirelessSP_activity, LV_OBJ_FLAG_HIDDEN);
    }
    lv_memset_00(lv_disp_get_default()->driver->draw_buf->buf_act,
                 lv_disp_get_default()->driver->draw_buf->size * sizeof(lv_color32_t));
}

void DLNA_Quit(void)
{
    CDX_LOGI(" '%s' \n", __func__);
    if (dlna_activity_exit) {
        CDX_LOGE(" %s: already quit\n", __func__);
    }

    dlna_activity_exit = true;
    hidden_wirelesssp_acitvity(false);
    lv_pro_dlna_ui_deinit();
    lv_group_set_default(NULL);
    set_current_ui_group(WirelessSP_group);

    lv_disp_get_default()->driver->screen_transp = 0;
    lv_disp_set_bg_opa(lv_disp_get_default(), LV_OPA_COVER);
    /* Empty the buffer, not emptying will cause the UI to be opaque */
    lv_memset_00(lv_disp_get_default()->driver->draw_buf->buf_act,
                 lv_disp_get_default()->driver->draw_buf->size * sizeof(lv_color32_t));
}

void lv_pro_dlna_set_render(RenderT *render) { dlna_render = render; }
void lv_pro_dlna_del_render(void) { dlna_render = NULL; }
