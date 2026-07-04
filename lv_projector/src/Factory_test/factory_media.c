#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include "tplayer.h"
#include "../Layer/msgbox/lv_ui_msgbox.h"
#include "../widget/lv_pro_res.h"
#include "sys_param.h"
#include "lv_common.h"
#include "page.h"

static unsigned int media_total_count = 0;
static unsigned int media_fail_count = 0;
static char *factory_media_path = "/tmp/USB0/test.mp4";

lv_obj_t *m_test_time_value_lable;
lv_obj_t *m_bt_test_value_lable;
lv_obj_t *m_wifi_test_value_lable;
lv_obj_t *m_remain_time_value_lable;
lv_obj_t *m_reboot_count_value_lable;

#define CEDARX_UNUSE(param) (void)param
#define ISNULL(x) if(!x){return -1;}

#define ft_err(fmt, args...) printf("[%s, %d]error: "fmt"\r\n", __func__, __LINE__, ##args)

#define FACTORY_TEST_DEBUG 1
#if FACTORY_TEST_DEBUG
#define ft_dbg(fmt, args...) printf("[%s, %d]debug "fmt"\r\n", __func__, __LINE__, ##args)
#else
#define ft_dbg(fmt, args...)
#endif

typedef struct PLAYER_CONTEXT_T {
    TPlayer *mTPlayer;
    int mSeekable;
    int mError;
    int mVideoFrameNum;
    int mPreparedFlag;
    int mLoopFlag;
    int mSetLoop;
    int mCompleteFlag;
    bool mStartFirst;
    bool mStartFinish;
    //char mUrl[64];
    MediaInfo *mMediaInfo;
    sem_t mPreparedSem;
} player_context_t;

static player_context_t player_context;
static pthread_mutex_t video_play_mutex = PTHREAD_MUTEX_INITIALIZER;
static lv_obj_t *media_stress_activity;
static lv_group_t *media_stress_group;

void factory_media_play(void);
void factory_media_destory(void);

//* a callback for tplayer.
static int CallbackForTPlayer(void *pUserData, int msg, int param0,
        void *param1) {
    player_context_t *pPlayer = (player_context_t*) pUserData;

    CEDARX_UNUSE(param1);
    switch (msg) {
    case TPLAYER_NOTIFY_PREPARED: {
        ft_dbg("TPLAYER_NOTIFY_PREPARED,has prepared.\n");
        player_context.mPreparedFlag = 1;
        sem_post(&pPlayer->mPreparedSem);
        break;
    }
    case TPLAYER_NOTIFY_PLAYBACK_COMPLETE: {
        ft_dbg("TPLAYER_NOTIFY_PLAYBACK_COMPLETE\n");
        player_context.mCompleteFlag = 1;
        factory_media_play();
        break;
    }
    case TPLAYER_NOTIFY_MEDIA_ERROR: {
        switch (param0) {
        case TPLAYER_MEDIA_ERROR_UNKNOWN: {
            ft_err("erro type:TPLAYER_MEDIA_ERROR_UNKNOWN\n");
            player_context.mError = 1;
            break;
        }
        case TPLAYER_MEDIA_ERROR_UNSUPPORTED: {
            ft_err("erro type:TPLAYER_MEDIA_ERROR_UNSUPPORTED\n");
            player_context.mError = 1;
            break;
        }
        case TPLAYER_MEDIA_ERROR_IO: {
            ft_err("erro type:TPLAYER_MEDIA_ERROR_IO\n");
            player_context.mError = 1;
            break;
        }
        case TPLAYER_MEDIA_ERROR_AUDIO_UNSUPPORTED: {
            ft_err("erro type:TPLAYER_MEDIA_ERROR_AUDIO_UNSUPPORTED\n");
            if (player_context.mStartFirst) {
                lv_msgbox_msg_open(lv_scr_act(), lv_get_string(STR_AUDIO_USPT),
                        2000, NULL, NULL);
                player_context.mStartFirst = false;
            }
            break;
        }
        }
        ft_err("error: media player status is error!\n");
        if (player_context.mStartFinish && player_context.mError) {
            ++media_fail_count;
            lv_msgbox_msg_open(lv_scr_act(), lv_get_string(STR_VIDEO_ERROR),
                    2000, factory_media_play, NULL);
        }
        break;
    }
    case TPLAYER_NOTIFY_BUFFER_START: {
        ft_dbg("have no enough data to play\n");
        break;
    }
    case TPLAYER_NOTIFY_BUFFER_END: {
        ft_dbg("have enough data to play again\n");
        break;
    }
    case TPLAYER_NOTIFY_MEDIA_VIDEO_SIZE: {
        ft_dbg("TPLAYER_NOTIFY_MEDIA_VIDEO_SIZE: width = %d, height = %d\n",
                ((int*)param1)[0], ((int*)param1)[1]);
        break;
    }
    default: {
        break;
    }
    }
    return 0;
}

static int semTimedWait(sem_t *sem, int64_t time_ms) {
    int err;

    if (time_ms == -1) {
        err = sem_wait(sem);
    } else {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_nsec += time_ms % 1000 * 1000 * 1000;
        ts.tv_sec += time_ms / 1000 + ts.tv_nsec / (1000 * 1000 * 1000);
        ts.tv_nsec = ts.tv_nsec % (1000 * 1000 * 1000);

        err = sem_timedwait(sem, &ts);
    }

    return err;
}

static int tplayer_init() {
    if (player_context.mTPlayer) {
        ft_err("player already init.\n");
        return -1;
    }
    //* create a player.
    player_context.mTPlayer = TPlayerCreate(CEDARX_PLAYER);
    if (player_context.mTPlayer == NULL) {
        ft_err("can not create tplayer, quit.\n");
        return -1;
    }
    //* set callback to player.
    TPlayerSetNotifyCallback(player_context.mTPlayer, CallbackForTPlayer,
            (void*) &player_context);

    //set player start status
    player_context.mError = 0;
    player_context.mSeekable = 1;
    player_context.mPreparedFlag = 0;
    player_context.mLoopFlag = 0;
    player_context.mSetLoop = 0;
    player_context.mMediaInfo = NULL;
    player_context.mCompleteFlag = 0;
    player_context.mStartFirst = true;
    sem_init(&player_context.mPreparedSem, 0, 0);

    TPlayerReset(player_context.mTPlayer);
    TPlayerSetDebugFlag(player_context.mTPlayer, 0);

    return 0;
}

static int tplayer_exit(void) {
    if (!player_context.mTPlayer) {
        ft_err("player not init.\n");
        return -1;
    }
    TPlayerReset(player_context.mTPlayer);
    TPlayerDestroy(player_context.mTPlayer);
    player_context.mTPlayer = NULL;
    sem_destroy(&player_context.mPreparedSem);
    return 0;
}

static void tplayer_setrect(unsigned int x, unsigned int y,
        unsigned int width, unsigned int height) {
    ISNULL(player_context.mTPlayer);
    if (!player_context.mPreparedFlag) {
        ft_err("not prepared!\n");
        return;
    }
    TPlayerSetDisplayRect(player_context.mTPlayer, x, y, width, height);
}

static int tplayer_play_url(const char *path) {
    int waitErr = 0;

    if (player_context.mTPlayer) {
        tplayer_exit();
    }
    if (tplayer_init()) {
        ft_err("Media Player Init Fail!\n");
        return -1;
    }

    //* set url to the tinaplayer.
    if (TPlayerSetDataSource(player_context.mTPlayer, path, NULL) != 0) {
        ft_err("TPlayerSetDataSource return fail.\n");
        return -1;
    } else {
        ft_dbg("setDataSource end\n");
    }
    if (player_context.mError) {
        ft_err("error: open media source fail.\n");
        TPlayerReset(player_context.mTPlayer);
        return -1;
    }

    if (TPlayerPrepareAsync(player_context.mTPlayer) != 0) {
        ft_err("TPlayerPrepareAsync() return fail.\n");
    } else {
        ft_dbg("preparing...\n");
    }
    waitErr = semTimedWait(&player_context.mPreparedSem, 10 * 1000);
    if (waitErr == -1) {
        ft_err("prepare fail\n");
        return -1;
    }
    ft_dbg("prepared ok\n");
    /* ft_dbg("TPlayerSetHoldLastPicture()\n"); */
    /* TPlayerSetHoldLastPicture(player_context.mTPlayer, 1); */

    //tplayer_setrect(0, 0, LV_HOR_RES, LV_VER_RES);
    return TPlayerStart(player_context.mTPlayer);
}

static int tplayer_stop(void) {
    ISNULL(player_context.mTPlayer);
    if (!player_context.mPreparedFlag) {
        ft_err("not prepared!\n");
        return -1;
    }
    TPlayerStop(player_context.mTPlayer);
    return TPlayerReset(player_context.mTPlayer);
}

static void* MediaPlayProc(void *arg) {
    pthread_mutex_lock(&video_play_mutex);
    player_context.mStartFinish = false;
    if (tplayer_play_url(factory_media_path)) {
        ++media_fail_count;
        lv_msgbox_msg_open(lv_scr_act(), lv_get_string(STR_VIDEO_ERROR),
                2000, factory_media_play, NULL);
    } else {
        ++media_total_count;
    }
    player_context.mStartFinish = true;
    pthread_mutex_unlock(&video_play_mutex);
    return NULL;
}

static void* MediaDeinitProc(void *arg) {
    pthread_mutex_lock(&video_play_mutex);
    tplayer_stop();
    tplayer_exit();
    pthread_mutex_unlock(&video_play_mutex);
    return NULL;
}

void factory_media_play(void) {
    pthread_t thread_id;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&thread_id, &attr, MediaPlayProc, NULL);
    pthread_attr_destroy(&attr);
}

void factory_media_destory(void) {
    pthread_t thread_id;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&thread_id, &attr, MediaDeinitProc, NULL);
    pthread_attr_destroy(&attr);
}

void factory_media_stress_ui_exit(void) {
    if (player_context.mTPlayer) {
        factory_media_destory();
    }
    if (media_stress_group) {
        lv_group_remove_all_objs(media_stress_group);
        lv_group_del(media_stress_group);
        media_stress_group = NULL;
    }
    if (media_stress_activity) {
        lv_obj_del_async(media_stress_activity);
        media_stress_activity = NULL;
    }
}

static void media_stress_load_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_SCREEN_LOADED) {
        lv_disp_get_default()->driver->screen_transp = 1;
        lv_disp_set_bg_opa(lv_disp_get_default(), LV_OPA_TRANSP);
        /* Empty the buffer, not emptying will cause the UI to be opaque */
        lv_memset_00(lv_disp_get_default()->driver->draw_buf->buf_act,
                lv_disp_get_default()->driver->draw_buf->size * sizeof(lv_color32_t));
        lv_obj_set_style_bg_opa(media_stress_activity, LV_OPA_TRANSP, 0);
    } else if (code == LV_EVENT_SCREEN_UNLOADED) {
        lv_disp_get_default()->driver->screen_transp = 0;
        lv_disp_set_bg_opa(lv_disp_get_default(), LV_OPA_COVER);
        lv_obj_set_style_bg_opa(media_stress_activity, LV_OPA_COVER, 0);
    }
}

static void factory_test_media_event(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t *key = lv_event_get_param(e);

    if (code == LV_EVENT_KEY) {
        if (*key == LV_KEY_BACK) {
            switch_page(PAGE_FACTORY_TEST);
        }
    }
}

void factory_media_report_result(lv_obj_t *parent_obj)
{
    lv_obj_t *media_result_cont = lv_obj_create(parent_obj);
    lv_obj_set_size(media_result_cont, lv_pct(100), lv_pct(30));
    lv_obj_align(media_result_cont, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_add_style(media_result_cont, &lv_pro_res.set_bg_grey, 0);
    lv_obj_set_style_bg_opa(media_result_cont, LV_OPA_80, 0);

    lv_group_add_obj(media_stress_group, media_result_cont);
    lv_obj_add_event_cb(media_result_cont, factory_test_media_event, LV_EVENT_KEY, NULL);

    /* Test time */
    lv_obj_t *m_test_time_text_lable = lv_label_create(media_result_cont);
    lv_obj_set_size(m_test_time_text_lable, lv_pct(20), lv_pct(15));
    lv_obj_set_align(m_test_time_text_lable, LV_ALIGN_TOP_LEFT);
    lv_obj_set_style_text_font(m_test_time_text_lable, &GENERAL_FONT_BIG, 0);
    lv_label_set_text(m_test_time_text_lable, "老化时间:");

    m_test_time_value_lable = lv_label_create(media_result_cont);
    lv_obj_set_size(m_test_time_value_lable, lv_pct(20), lv_pct(15));
    lv_obj_align_to(m_test_time_value_lable, m_test_time_text_lable, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
    lv_obj_set_style_text_font(m_test_time_value_lable, &GENERAL_FONT_BIG, 0);
    lv_label_set_text(m_test_time_value_lable, "12:34:56");

    /* BT Test */
    lv_obj_t *m_bt_test_text_lable = lv_label_create(media_result_cont);
    lv_obj_set_size(m_bt_test_text_lable, lv_pct(20), lv_pct(15));
    lv_obj_align_to(m_bt_test_text_lable, m_test_time_value_lable, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
    lv_obj_set_style_text_font(m_bt_test_text_lable, &GENERAL_FONT_BIG, 0);
    lv_label_set_text(m_bt_test_text_lable, "蓝牙压力测试:");

    m_bt_test_value_lable = lv_label_create(media_result_cont);
    lv_obj_set_size(m_bt_test_value_lable, lv_pct(20), lv_pct(15));
    lv_obj_align_to(m_bt_test_value_lable, m_bt_test_text_lable, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
    lv_obj_set_style_text_font(m_bt_test_value_lable, &GENERAL_FONT_BIG, 0);

    /* Test time */
    lv_obj_t *m_remain_time_text_lable = lv_label_create(media_result_cont);
    lv_obj_set_size(m_remain_time_text_lable, lv_pct(20), lv_pct(15));
    lv_obj_align_to(m_remain_time_text_lable, m_test_time_text_lable, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_text_font(m_remain_time_text_lable, &GENERAL_FONT_BIG, 0);
    lv_label_set_text(m_remain_time_text_lable, "老化剩余时间:");

    m_remain_time_value_lable = lv_label_create(media_result_cont);
    lv_obj_set_size(m_remain_time_value_lable, lv_pct(20), lv_pct(15));
    lv_obj_align_to(m_remain_time_value_lable, m_remain_time_text_lable, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
    lv_obj_set_style_text_font(m_remain_time_value_lable, &GENERAL_FONT_BIG, 0);
    lv_label_set_text(m_remain_time_value_lable, "12:34:56");

    /* WIFI Test */
    lv_obj_t *m_wifi_test_text_lable = lv_label_create(media_result_cont);
    lv_obj_set_size(m_wifi_test_text_lable, lv_pct(20), lv_pct(15));
    lv_obj_align_to(m_wifi_test_text_lable, m_remain_time_value_lable, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
    lv_obj_set_style_text_font(m_wifi_test_text_lable, &GENERAL_FONT_BIG, 0);
    lv_label_set_text(m_wifi_test_text_lable, "wifi压力测试:");

    m_wifi_test_value_lable = lv_label_create(media_result_cont);
    lv_obj_set_size(m_wifi_test_value_lable, lv_pct(20), lv_pct(15));
    lv_obj_align_to(m_wifi_test_value_lable, m_wifi_test_text_lable, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
    lv_obj_set_style_text_font(m_wifi_test_value_lable, &GENERAL_FONT_BIG, 0);

    /* Reboot time */
    lv_obj_t *m_reboot_count_text_lable = lv_label_create(media_result_cont);
    lv_obj_set_size(m_reboot_count_text_lable, lv_pct(20), lv_pct(15));
    lv_obj_align_to(m_reboot_count_text_lable, m_remain_time_text_lable, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_text_font(m_reboot_count_text_lable, &GENERAL_FONT_BIG, 0);
    lv_label_set_text(m_reboot_count_text_lable, "重启次数:");

    m_reboot_count_value_lable = lv_label_create(media_result_cont);
    lv_obj_set_size(m_reboot_count_value_lable, lv_pct(20), lv_pct(15));
    lv_obj_align_to(m_reboot_count_value_lable, m_reboot_count_text_lable, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
    lv_obj_set_style_text_font(m_reboot_count_value_lable, &GENERAL_FONT_BIG, 0);
}

void factory_media_stress_ui_init(void) {
    media_stress_activity = lv_obj_create(NULL);
    media_stress_group = lv_group_create();

    lv_obj_set_size(media_stress_activity, lv_pct(100), lv_pct(100));
    lv_obj_add_style(media_stress_activity, &lv_pro_res.set_bg_black, 0);

    lv_obj_add_event_cb(media_stress_activity, media_stress_load_event_handler,
            LV_EVENT_SCREEN_LOADED, NULL);
    lv_obj_add_event_cb(media_stress_activity, media_stress_load_event_handler,
            LV_EVENT_SCREEN_UNLOADED, NULL);

    factory_media_report_result(media_stress_activity);

    if (access(factory_media_path, F_OK) == 0) {
        if (is_volume_mute()) {
            lv_set_mute(false);
        }
        lv_set_volume(100);
        lv_set_sys_param(P_VOLUME, 100);

        factory_media_play();
    } else {
        lv_obj_t *file_null_warn = lv_label_create(media_stress_activity);
        lv_obj_set_size(file_null_warn, LV_PCT(100), 50);
        lv_obj_align(file_null_warn, LV_ALIGN_CENTER, 0, 0);
        lv_obj_add_style(file_null_warn, &lv_pro_res.label_white, 0);
        lv_obj_set_style_text_font(file_null_warn, &GENERAL_FONT_BIG, 0);
        lv_label_set_text_fmt(file_null_warn, "视频测试文件: %s 不存在!", factory_media_path);
        lv_obj_set_style_text_align(file_null_warn, LV_TEXT_ALIGN_CENTER, 0);
    }
}

static int create_media_stress_ui(void)
{
    factory_media_stress_ui_init();
    load_current_channel(media_stress_activity, media_stress_group);
    return 0;
}

static int destory_media_stress_ui(void)
{
    factory_media_stress_ui_exit();
    return 0;
}

static int show_media_stress_ui(void)
{
    set_current_ui_group(media_stress_group);
    return 0;
}

static int hide_media_stress_ui(void)
{
    return 0;
}

static page_interface_t page_media_stress_ui =
{
    .ops =
    {
        create_media_stress_ui,
        destory_media_stress_ui,
        show_media_stress_ui,
        hide_media_stress_ui,
    },
    .info =
    {
        .id = PAGE_MEDIA_STRESS,
        .user_data = NULL
    }
};

void REGISTER_PAGE_MEDIA_STRESS(void)
{
    reg_page(&page_media_stress_ui);
}
