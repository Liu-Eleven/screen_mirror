#include "lv_pro_res_media_player_int.h"

#ifdef LV_USE_LINUX_TPLAYER

#include <dirent.h>
#include "tplayer.h"
#include <video/sunxi_display2.h>
#include "../../Layer/msgbox/lv_ui_msgbox.h"
#include "../file_explorer/lv_pro_file_explorer.h"
#include "include/sys_param.h"
#include "lv_pro_res_debug.h"
#include <fcntl.h>

#define CEDARX_UNUSE(param) (void)param
#define ISNULL(x) if(!x){return -1;}

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

typedef struct lv_disp_rect {
    int x; /* xoffset */
    int y; /* yoffset */
    int w; /* width */
    int h; /* height */
} lv_disp_rect;

typedef struct movie_resolution {
    lv_disp_rect screen;
    lv_disp_rect video;
    lv_disp_rect movie;
    float screen_ratio;
} movie_resolution;

static player_context_t player_context;
static movie_resolution movie_res;
static list_head_t *media_list;
static bool media_is_open = false;
static bool media_is_playing = false;
extern int movie_ratio;
extern bool media_mode_single;
static pthread_mutex_t media_play_mutex = PTHREAD_MUTEX_INITIALIZER;
extern FileType media_filetype;
#ifdef MOVIE_SUBTITLE_SUPPORT
static lv_subtitle_t lv_subtitle={0};
static int last_buf_size = 0;
#endif

static int lv_pro_get_screen_resolution(int *width, int *height)
{
    unsigned long ioctlParam[2];
    int disp_fd = open("/dev/disp", O_RDWR);
    if (disp_fd < 0) {
        mw_err("open disp handle error!\n");
        goto Fail;
    }
    ioctlParam[0] = 0;
    ioctlParam[1] = 0;
    *width = ioctl(disp_fd, DISP_GET_SCN_WIDTH, ioctlParam);
    *height = ioctl(disp_fd, DISP_GET_SCN_HEIGHT, ioctlParam);
    if (*width == 0 || *height == 0) {
        mw_err("get screen size fail\n");
        goto Fail;
    }
    close(disp_fd);
    return 0;
Fail:
    *width = LV_HOR_RES;
    *height = LV_VER_RES;
    return -1;
}

static void lv_pro_set_movie_rect(void)
{
    float video_ratio = ((float)movie_res.video.w / (float)movie_res.video.h);
    if (video_ratio > movie_res.screen_ratio) {
        float scaleratio = ((float)movie_res.screen.w / (float)movie_res.video.w);
        int scale_h = movie_res.video.h * scaleratio;
        movie_res.movie.x = 0;
        movie_res.movie.y = movie_res.screen.h / 2 - scale_h / 2;
        movie_res.movie.w = movie_res.screen.w;
        movie_res.movie.h = scale_h;
    } else {
        float scaleratio = ((float)movie_res.screen.h / (float)movie_res.video.h);
        int scale_w = movie_res.video.w * scaleratio;
        movie_res.movie.x = movie_res.screen.w / 2 - scale_w / 2;
        movie_res.movie.y = 0;
        movie_res.movie.w = scale_w;
        movie_res.movie.h = movie_res.screen.h;
    }
}

void lv_pro_media_reset_screen_resolution(void)
{
    movie_res.screen.w = 0;
    movie_res.screen.h = 0;
    movie_res.screen_ratio = 0;
}

int lv_pro_media_msg_enqueue(lv_media_msg_type_t type, void *data, bool free_data)
{
    lv_media_msg_t *_msg = (lv_media_msg_t *)malloc(sizeof(lv_media_msg_t));
    _msg->type = type;
    _msg->data = data;
    _msg->free_data = free_data;
    system_send_msg(MSG_TYPE_MEDIA_FRESH_UI, _msg);
}

static void media_msg_callback(void)
{
    lv_pro_media_msg_enqueue(MSG_TYPE_MEDIA_PLAY_NEXT, 1, false);
}

void media_player_message_process(system_msg_t *sys_msg)
{
    lv_group_t *__key_group;
    lv_media_msg_t *msg;
    char *err_str = NULL;

    if (sys_msg == NULL) {
        mw_err("sys msg null!\n");
        return;
    }

    if (sys_msg->type != MSG_TYPE_MEDIA_FRESH_UI) {
        return;
    }

    msg = sys_msg->data;

    switch (msg->type) {
    case MSG_TYPE_MEDIA_PLAY:
        media_is_open = true;
        if (movie_res.screen.w == 0 || movie_res.screen.h == 0) {
            lv_pro_get_screen_resolution(&movie_res.screen.w, &movie_res.screen.h);
            movie_res.screen_ratio = ((float)movie_res.screen.w / (float)movie_res.screen.h);
            mw_dbg("media get screen size: width = %d, height = %d.",
                    movie_res.screen.w, movie_res.screen.h);
        }
        lv_pro_res_media_play_mode(3, 0);
        break;

    case MSG_TYPE_MEDIA_DESTORY:
        media_is_open = false;
        lv_pro_res_media_list_deinit();
        lv_pro_res_media_destory();
        break;

    case MSG_TYPE_MEDIA_PLAY_NEXT:
        if (!player_context.mStartFinish) {
            break;
        }

        if (media_is_open) {
            lv_pro_res_media_stop();
            lv_pro_res_media_play_mode((int)msg->data, 0);
        }
        break;

    case MSG_TYPE_MEDIA_PLAY_LIST:
        if (media_is_open) {
            lv_pro_res_media_stop();
            lv_pro_res_media_play_mode(2, (int)msg->data);
        }
        break;

    case MSG_TYPE_MEDIA_ERROR:
        if (!media_is_open) {
            break;
        }
        if (media_filetype == FileType_Movie) {
            err_str = lv_get_string(STR_VIDEO_ERROR);
        } else if (media_filetype == FileType_Music) {
            err_str = lv_get_string(STR_AUDIO_ERROR);
        }
        lv_msgbox_msg_open(lv_scr_act(), err_str,
                2000, media_msg_callback, NULL);
        break;

    case MSG_TYPE_MEDIA_AUDIO_UNSUPPORTED:
        if (!media_is_open) {
            break;
        }
        lv_msgbox_msg_open(lv_scr_act(), lv_get_string(STR_AUDIO_USPT),
                2000, NULL, NULL);
        break;

    case MSG_TYPE_MEDIA_REFRESH_INFO:
        if (!media_is_open) {
            break;
        }
        if (media_filetype == FileType_Movie) {
            lv_pro_movie_refresh_info();
        } else if (media_filetype == FileType_Music) {
            lv_pro_music_refresh_info();
        }
        break;

    case MSG_TYPE_MEDIA_VIDEO_SIZE_CHANGE:
        if ((media_filetype == FileType_Movie) && media_is_open) {
            lv_pro_set_movie_rect();
            if (movie_ratio == 2) {
                lv_pro_res_movie_set_ratio(Movie_Ratio_4_3);
            } else if (movie_ratio == 1) {
                lv_pro_res_movie_set_ratio(Movie_Ratio_16_9);
            } else {
                lv_pro_res_movie_set_ratio(Movie_Ratio_Auto);
            }
        }
        break;
    }

MSG_END:
    if (msg->free_data) {
        free(msg->data);
    }
    if (msg) {
        free(msg);
    }
}


//* a callback for tplayer.
static int CallbackForTPlayer(void *pUserData, int msg, int param0,
        void *param1) {
    player_context_t *pPlayer = (player_context_t*) pUserData;

    CEDARX_UNUSE(param1);
    switch (msg) {
    case TPLAYER_NOTIFY_PREPARED: {
        mw_dbg("TPLAYER_NOTIFY_PREPARED,has prepared.\n");
        player_context.mPreparedFlag = 1;
        sem_post(&pPlayer->mPreparedSem);
        break;
    }
    case TPLAYER_NOTIFY_PLAYBACK_COMPLETE: {
        mw_dbg("TPLAYER_NOTIFY_PLAYBACK_COMPLETE\n");
        player_context.mCompleteFlag = 1;
        lv_pro_media_msg_enqueue(MSG_TYPE_MEDIA_PLAY_NEXT, 1, false);
        break;
    }
    case TPLAYER_NOTIFY_SEEK_COMPLETE: {
        mw_dbg("TPLAYER_NOTIFY_SEEK_COMPLETE>>>>info: seek ok.\n");
        break;
    }
    case TPLAYER_NOTIFY_SEEK_START_POINT: {
        mw_dbg("TPLAYER_NOTIFY_SEEK_START_POINT\n");
        lv_pro_media_msg_enqueue(MSG_TYPE_MEDIA_PLAY, NULL, false);
        break;
    }
    case TPLAYER_NOTIFY_MEDIA_ERROR: {
        switch (param0) {
        case TPLAYER_MEDIA_ERROR_UNKNOWN: {
            mw_err("erro type:TPLAYER_MEDIA_ERROR_UNKNOWN\n");
            player_context.mError = 1;
            break;
        }
        case TPLAYER_MEDIA_ERROR_UNSUPPORTED: {
            mw_err("erro type:TPLAYER_MEDIA_ERROR_UNSUPPORTED\n");
            player_context.mError = 1;
            break;
        }
        case TPLAYER_MEDIA_ERROR_IO: {
            mw_err("erro type:TPLAYER_MEDIA_ERROR_IO\n");
            player_context.mError = 1;
            break;
        }
        case TPLAYER_MEDIA_ERROR_AUDIO_UNSUPPORTED: {
            mw_err("erro type:TPLAYER_MEDIA_ERROR_AUDIO_UNSUPPORTED\n");
            if (player_context.mStartFirst) {
                lv_pro_media_msg_enqueue(MSG_TYPE_MEDIA_AUDIO_UNSUPPORTED, NULL, false);
                player_context.mStartFirst = false;
            }
            break;
        }
        }
        mw_err("error: media player status is error!\n");
        if (player_context.mStartFinish && player_context.mError) {
            lv_pro_media_msg_enqueue(MSG_TYPE_MEDIA_ERROR, NULL, false);
        }
        break;
    }
    case TPLAYER_NOTIFY_NOT_SEEKABLE: {
        pPlayer->mSeekable = 0;
        mw_err("info: media source is unseekable.\n");
        break;
    }
    case TPLAYER_NOTIFY_BUFFER_START: {
        mw_dbg("have no enough data to play\n");
        break;
    }
    case TPLAYER_NOTIFY_BUFFER_END: {
        mw_dbg("have enough data to play again\n");
        break;
    }
    case TPLAYER_NOTIFY_VIDEO_FRAME: {
        //mw_dbg("get the decoded video frame\n");
        break;
    }
    case TPLAYER_NOTIFY_AUDIO_FRAME: {
        //mw_dbg("get the decoded audio frame\n");
        break;
    }
    case TPLAYER_NOTIFY_MEDIA_VIDEO_SIZE: {
        movie_res.video.w = ((int*)param1)[0];   //real decoded video width
        movie_res.video.h = ((int*)param1)[1];   //real decoded video height
        lv_pro_media_msg_enqueue(MSG_TYPE_MEDIA_VIDEO_SIZE_CHANGE, NULL, false);
        mw_dbg("TPLAYER_NOTIFY_MEDIA_VIDEO_SIZE: width = %d, height = %d\n",
                movie_res.video.w, movie_res.video.h);
        break;
    }
#ifdef MOVIE_SUBTITLE_SUPPORT
    case TPLAYER_NOTIFY_SUBTITLE_FRAME: {
        mw_dbg("TPLAYER_NOTIFY_SUBTITLE_FRAME: get the decoded subtitle frame\n");
        uintptr_t *p = (uintptr_t*)param1;
        unsigned long buf_size = 0;
        lv_subtitle.type = ((SubtitleItem*)(p[1]))->bText;
        if (lv_subtitle.type == 0) {
            char *data = (char *)((SubtitleItem*)(p[1]))->pBitmapData;
            if(data != NULL) {
                lv_subtitle.w = ((SubtitleItem*)(p[1]))->nBitmapWidth;
                lv_subtitle.h = ((SubtitleItem*)(p[1]))->nBitmapHeight;
                buf_size = lv_subtitle.w * lv_subtitle.h * 4; //ARGB
                if (last_buf_size < buf_size) {
                    last_buf_size = buf_size;
                    if (!lv_subtitle.data) {
                        lv_subtitle.data = malloc(buf_size);
                    } else {
                        lv_subtitle.data = realloc(lv_subtitle.data, buf_size);
                    }
                    if (!lv_subtitle.data) {
                        mw_err("Error: no memory %s:%d\n", __func__ ,__LINE__);
                    }
                    memset(lv_subtitle.data, 0, buf_size);
                } else{
                    memset(lv_subtitle.data, 0, last_buf_size);
                }
                memcpy(lv_subtitle.data, data, buf_size);
                subtitles_event_send(SUBTITLES_EVENT_SHOW, &lv_subtitle);
                mw_dbg("id:%d, pItem:%lu, pic size:%lu, WxH:%dx%d\n", (int)p[0], p[1], buf_size,
                        lv_subtitle.w, lv_subtitle.h);
            } else {
                mw_err("TPLAYER_NOTIFY_SUBTITLE_FRAME: pic subtitle data is NULL!\n");
            }
        } else if (lv_subtitle.type == 1) {
            char *text = (char *)((SubtitleItem*)(p[1]))->pText;
            if(text != NULL) {
                buf_size = strlen(text);
                if (last_buf_size < buf_size) {
                    last_buf_size = buf_size;
                    if (!lv_subtitle.data) {
                        lv_subtitle.data = malloc(buf_size);
                    } else {
                        lv_subtitle.data = realloc(lv_subtitle.data, buf_size);
                    }
                    if (!lv_subtitle.data) {
                        mw_err("Error: no memory %s:%d\n", __func__ ,__LINE__);
                        break;
                    }
                    memset(lv_subtitle.data, 0, buf_size);
                } else {
                    memset(lv_subtitle.data, 0, last_buf_size);
                }
                memcpy(lv_subtitle.data, text, buf_size);
                subtitles_event_send(SUBTITLES_EVENT_SHOW, &lv_subtitle);
                mw_dbg("id:%d, pItem:%lu, text size:%lu data:%s", (int)p[0], p[1], buf_size, text);
            } else {
                mw_err("TPLAYER_NOTIFY_SUBTITLE_FRAME: text subtitle data is NULL!\n");
            }
        } else {
            mw_err("TPLAYER_NOTIFY_SUBTITLE_FRAME: Unrecognized subtitle format %d\n", lv_subtitle.type);
        }
        break;
    }
    case TPLAYER_NOTIFY_HIDE_SUBTITLE_FRAME: {
        mw_dbg("TPLAYER_NOTIFY_HIDE_SUBTITLE_FRAME\n");
        subtitles_event_send(SUBTITLES_EVENT_HIDDEN, NULL);
        break;
    }
#endif
    default: {
        mw_dbg("warning: unknown callback from Tinaplayer.\n");
        break;
    }
    }
    return 0;
}

static int tplayer_init() {
    if (player_context.mTPlayer && !media_is_open) {
        mw_err("player already init.\n");
        return -1;
    }
    //* create a player.
    player_context.mTPlayer = TPlayerCreate(CEDARX_PLAYER);
    if (player_context.mTPlayer == NULL) {
        mw_err("can not create tplayer, quit.\n");
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
        mw_err("player not init.\n");
        return -1;
    }
    TPlayerReset(player_context.mTPlayer);
    TPlayerDestroy(player_context.mTPlayer);
    player_context.mTPlayer = NULL;
    sem_destroy(&player_context.mPreparedSem);
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

static int tplayer_play_url(const char *path) {
    int waitErr = 0;

    if (player_context.mTPlayer) {
        lv_pro_res_media_deinit();
    }
    if (lv_pro_res_media_init()) {
        mw_err("Media Player Init Fail!\n");
        return -1;
    }

    //* set url to the tinaplayer.
    if (TPlayerSetDataSource(player_context.mTPlayer, path, NULL) != 0) {
        mw_err("TPlayerSetDataSource return fail.\n");
        return -1;
    } else {
        mw_dbg("setDataSource end\n");
    }
    if (player_context.mError) {
        mw_err("error: open media source fail.\n");
        TPlayerReset(player_context.mTPlayer);
        return -1;
    }

    if (TPlayerPrepareAsync(player_context.mTPlayer) != 0) {
        mw_err("TPlayerPrepareAsync() return fail.\n");
    } else {
        mw_dbg("preparing...\n");
    }
    waitErr = semTimedWait(&player_context.mPreparedSem, 10 * 1000);
    if (waitErr == -1) {
        mw_err("prepare fail\n");
        return -1;
    }
    mw_dbg("prepared ok\n");
    /* mw_dbg("TPlayerSetHoldLastPicture()\n"); */
    /* TPlayerSetHoldLastPicture(player_context.mTPlayer, 1); */

    if (media_mode_single) {
        lv_pro_res_media_set_looping(true);
    }
#ifdef MOVIE_SUBTITLE_SUPPORT
    if (media_filetype == FileType_Movie) {
        last_buf_size = 0;
        ext_subtitles_init(); //scan subtitle file form filelist
        ext_subtitle_t *m_subtitle = ext_subtitle_data_get();
        if (m_subtitle->ext_subs_count != 0) {
            TPlayerSetExternalSubUrl(player_context.mTPlayer, m_subtitle->uris[0]);
        }
    }
#endif
    return TPlayerStart(player_context.mTPlayer);
}

static int tplayer_play(void) {
    ISNULL(player_context.mTPlayer);
    if (!player_context.mPreparedFlag) {
        mw_err("not prepared!\n");
        return -1;
    }
    if (!player_context.mStartFinish || TPlayerIsPlaying(player_context.mTPlayer)) {
        mw_err("already playing!\n");
        return 1;
    }
#ifdef MOVIE_SUBTITLE_SUPPORT
    if (media_filetype == FileType_Movie) {
        subtitles_event_send(SUBTITLES_EVENT_RESUME, NULL);
    }
#endif
    return TPlayerStart(player_context.mTPlayer);
}

static int tplayer_pause(void) {
    ISNULL(player_context.mTPlayer);
    if (!player_context.mPreparedFlag) {
        mw_err("not prepared!\n");
        return -1;
    }
    if (!player_context.mStartFinish || !TPlayerIsPlaying(player_context.mTPlayer)) {
        mw_err("not playing!\n");
        return 1;
    }
#ifdef MOVIE_SUBTITLE_SUPPORT
    if (media_filetype == FileType_Movie) {
        subtitles_event_send(SUBTITLES_EVENT_PAUSE, NULL);
    }
#endif
    return TPlayerPause(player_context.mTPlayer);
}

static int tplayer_stop(void) {
    ISNULL(player_context.mTPlayer);
    if (!player_context.mPreparedFlag) {
        mw_err("not prepared!\n");
        return -1;
    }
#ifdef MOVIE_SUBTITLE_SUPPORT
    if (media_filetype == FileType_Movie) {
        if (lv_subtitle.data) {
            free(lv_subtitle.data);
            lv_subtitle.data = NULL;
        }
        subtitles_event_send(SUBTITLES_EVENT_STOP, NULL);
        ext_subtitle_deinit();
    }
#endif

    TPlayerStop(player_context.mTPlayer);
    return TPlayerReset(player_context.mTPlayer);
}

static int tplayer_seekto(int nSeekTimeMs) {
    ISNULL(player_context.mTPlayer);
    if (!player_context.mPreparedFlag) {
        mw_err("not prepared!\n");
        return -1;
    }
    /*
     if(TPlayerIsPlaying(player_context.mTPlayer)){
     mw_err("seekto can not at playing state!\n");
     return -1;
     }
     */
    return TPlayerSeekTo(player_context.mTPlayer, nSeekTimeMs);
}

static int tplayer_setvolumn(int volumn) {
    ISNULL(player_context.mTPlayer);
    if (!player_context.mPreparedFlag) {
        mw_err("not prepared!\n");
        return -1;
    }
    return TPlayerSetVolume(player_context.mTPlayer, volumn);
}

static int tplayer_getvolumn(void) {
    ISNULL(player_context.mTPlayer);
    if (!player_context.mPreparedFlag) {
        mw_err("not prepared!\n");
        return -1;
    }
    return TPlayerGetVolume(player_context.mTPlayer);
}

static int tplayer_setlooping(bool bLoop) {
    ISNULL(player_context.mTPlayer);
    if (!player_context.mPreparedFlag) {
        mw_err("not prepared!\n");
        return -1;
    }
    return TPlayerSetLooping(player_context.mTPlayer, bLoop);
}

static int tplayer_setspeed(TplayerPlaySpeedType nSpeed) {
    ISNULL(player_context.mTPlayer);
    if (!player_context.mPreparedFlag) {
        mw_err("not prepared!\n");
        return -1;
    }
    if (!player_context.mStartFinish || !TPlayerIsPlaying(player_context.mTPlayer)) {
        mw_err("not playing!\n");
        return 1;
    }
    return TPlayerSetSpeed(player_context.mTPlayer, nSpeed);
}

static void tplayer_setratio(unsigned int x, unsigned int y,
        unsigned int width, unsigned int height) {
    ISNULL(player_context.mTPlayer);
    if (!player_context.mPreparedFlag) {
        mw_err("not prepared!\n");
        return;
    }
    TPlayerSetDisplayRect(player_context.mTPlayer, x, y, width, height);
}

static MediaInfo* tplayer_getmediainfo(void) {
    ISNULL(player_context.mTPlayer);
    if (!player_context.mPreparedFlag) {
        mw_err("not prepared!\n");
        return NULL;
    }
    return TPlayerGetMediaInfo(player_context.mTPlayer);
}

static int tplayer_switchaudio(int index) {
    ISNULL(player_context.mTPlayer);
    if (!player_context.mPreparedFlag) {
        mw_err("not prepared!\n");
        return -1;
    }
    return TPlayerSwitchAudio(player_context.mTPlayer, index);
}

static int tplayer_switchsubtitle(int index) {
    ISNULL(player_context.mTPlayer);
    if (!player_context.mPreparedFlag) {
        mw_err("not prepared!\n");
        return -1;
    }
    return TPlayerSwitchSubtitle(player_context.mTPlayer, index);
}

static int tplayer_getduration(int *msec) {
    ISNULL(player_context.mTPlayer);
    if (!player_context.mPreparedFlag) {
        //mw_err("not prepared!\n");
        return -1;
    }
    return TPlayerGetDuration(player_context.mTPlayer, msec);
}

static int tplayer_getcurrentpos(int *msec) {
    ISNULL(player_context.mTPlayer);
    if (!player_context.mPreparedFlag) {
       // mw_err("not prepared!\n");
        return -1;
    }
    return TPlayerGetCurrentPosition(player_context.mTPlayer, msec);
}

static int tplayer_getcompletestate(void) {
    return player_context.mCompleteFlag;
}

static int tplayer_videodisplayenable(int enable) {
    TPlayerSetVideoDisplay(player_context.mTPlayer, enable);
    return 0;
}

static int tplayer_setsrcrect(int x, int y, unsigned int width, unsigned int height) {
    TPlayerSetSrcRect(player_context.mTPlayer, x, y, width, height);
    return 0;
}

static int tplayer_setbrightness(unsigned int grade) {
    TPlayerSetBrightness(player_context.mTPlayer, grade);
    return 0;
}

static int tplayer_setcontrast(unsigned int grade) {
    TPlayerSetContrast(player_context.mTPlayer, grade);
    return 0;
}

static int tplayer_sethue(unsigned int grade) {
    TPlayerSetHue(player_context.mTPlayer, grade);
    return 0;
}

static int tplayer_setsaturation(unsigned int grade) {
    TPlayerSetSaturation(player_context.mTPlayer, grade);
    return 0;
}

static int tplayer_getplaying(void) {
    ISNULL(player_context.mTPlayer);
    return TPlayerIsPlaying(player_context.mTPlayer);
}

static void* MediaPlayProc(void *arg) {
    pthread_mutex_lock(&media_play_mutex);
    player_context.mStartFinish = false;

    if(media_list == NULL || media_list->current_node == NULL || media_list->current_node->path == NULL) {
        printf("invaild media path\n");
        pthread_mutex_unlock(&media_play_mutex);
        return NULL;
    }

    if (tplayer_play_url(media_list->current_node->path)) {
        lv_pro_media_msg_enqueue(MSG_TYPE_MEDIA_ERROR, NULL, false);
    }
    lv_pro_media_msg_enqueue(MSG_TYPE_MEDIA_REFRESH_INFO, NULL, false);
    player_context.mStartFinish = true;
    pthread_mutex_unlock(&media_play_mutex);

    return NULL;
}

int lv_pro_res_media_init(void) {
    return tplayer_init();
}

int lv_pro_res_media_deinit(void) {
    return tplayer_exit();
}

int lv_pro_res_media_list_init(char *cur_path, char *fn) {
    DIR *dp;
    struct dirent *dirp;
    char compete_path[FILE_PATH_MAXT_LEN];
    char compete_name[FILE_NAME_MAXT_LEN];
    int index = 0;
    int cur_index = 1;

    media_list = create_list();

    dp = opendir(cur_path);
    if (!dp) {
        mw_err("open directory %s error\n", cur_path);
        return -1;
    }

    /*Get media file name */
    while ((dirp = readdir(dp))) {
        if (lv_pro_check_file_type(dirp->d_name, media_filetype) == true) {
            //mw_dbg("media filename: %s/%s\n", cur_path, dirp->d_name);
            memset(compete_path, 0, sizeof(compete_path));
            memset(compete_name, 0, sizeof(compete_name));
            snprintf(compete_path, FILE_PATH_MAXT_LEN, "%s/%s", cur_path,
                    dirp->d_name);
            snprintf(compete_name, FILE_NAME_MAXT_LEN, "%s", dirp->d_name);
            list_append(media_list, compete_path, compete_name,
                    MEDIA_F_TYPE_VIDEO);
            ++index;
            if (!strcmp(dirp->d_name, fn)) {
                cur_index = index;
            }
#ifdef MOVIE_SUBTITLE_SUPPORT
        } else if (media_filetype == FileType_Movie) {
            lv_pro_file_subtitile_list_create(dirp->d_name);
#endif
        }
    }

    closedir(dp);

    /* Initialize the default file, the media play activity can be played immediately */
    list_skip_to_index(media_list, cur_index);

    return 0;
}

int lv_pro_res_media_list_deinit(void) {
    if (media_list != NULL) {
        destroy_list(media_list);
        media_list = NULL;
    }
#ifdef MOVIE_SUBTITLE_SUPPORT
    if (media_filetype == FileType_Movie) {
        lv_pro_file_subtitle_list_free();
    }
#endif
    return 0;
}

static void* MediaDeinitProc(void *arg) {
    pthread_mutex_lock(&media_play_mutex);
    lv_pro_res_media_stop();
    lv_pro_res_media_deinit();
    pthread_mutex_unlock(&media_play_mutex);

    return NULL;
}

void lv_pro_res_media_destory(void) {
    pthread_t thread_id;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&thread_id, &attr, MediaDeinitProc, NULL);
    pthread_attr_destroy(&attr);
}

void lv_pro_res_media_play_mode(int mode, int index) {
    media_is_playing = true;
    if (mode == 1) {
        list_get_next_node(media_list);
    } else if (mode == 0) {
        list_get_pre_node(media_list);
    } else if (mode == 2) {
        list_skip_to_index(media_list, index);
    }
    if (media_filetype == FileType_Movie) {
        lv_pro_movie_play_status(true);
    } else if (media_filetype == FileType_Music) {
        lv_pro_music_play_status(true);
    }
    pthread_t thread_id;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&thread_id, &attr, MediaPlayProc, NULL);
    pthread_attr_destroy(&attr);
}

int lv_pro_res_media_play(void) {
    media_is_playing = true;
    return tplayer_play();
}

int lv_pro_res_media_pause(void) {
    media_is_playing = false;
    return tplayer_pause();
}

int lv_pro_res_media_stop(void) {
    media_is_playing = false;
    return tplayer_stop();
}

int lv_pro_res_media_get_volumn(void) {
    return tplayer_getvolumn();
}

int lv_pro_res_media_set_volumn(int volume) {
    return tplayer_setvolumn(volume);
}

int lv_pro_res_media_set_looping(bool bLoop) {
    return tplayer_setlooping(bLoop);
}

int lv_pro_res_media_set_forward(int nSeekTimeMs) {
    int curDuration = 0;
    int totalDuration = 0;
    int seekDuration = 0;
    tplayer_getduration(&totalDuration);
    tplayer_getcurrentpos(&curDuration);
    seekDuration = curDuration + nSeekTimeMs;
    if (seekDuration > totalDuration)
        seekDuration = totalDuration;
    return tplayer_seekto(seekDuration);
}

int lv_pro_res_media_set_backward(int nSeekTimeMs) {
    int curDuration = 0;
    int totalDuration = 0;
    int seekDuration = 0;
    tplayer_getduration(&totalDuration);
    tplayer_getcurrentpos(&curDuration);
    seekDuration = curDuration - nSeekTimeMs;
    if (seekDuration < 0)
        seekDuration = 0;
    return tplayer_seekto(seekDuration);
}

static int SeekTime = 0;
static int64_t last_seek_op_time = 0;
void lv_pro_res_media_set_seek(bool backward) {
    uint32_t total_time = 0;
    uint32_t play_time_initial = 0;
    uint32_t play_time = 0;
    uint32_t jump_interval = 0;
    uint32_t seek_time = 0;
    int64_t cur_op_time = 0;
    struct timeval time_t;

    tplayer_getduration(&total_time);
    tplayer_getcurrentpos(&play_time_initial);
    play_time = play_time_initial;

    if (total_time < jump_interval)
        return;
    if(play_time_initial >= total_time)
        return;

    gettimeofday(&time_t, NULL);
    cur_op_time = time_t.tv_sec * 1000 + time_t.tv_usec / 1000;
    if ((cur_op_time - last_seek_op_time) < 800) {
        SeekTime += 10000;
    } else {
        SeekTime = 10000;
    }

    last_seek_op_time = cur_op_time;

    if (SeekTime < 300000)
        jump_interval = SeekTime;
    else
        jump_interval = 300000;

    if (backward) { //seek backward
        if (play_time > jump_interval)
            seek_time = play_time - jump_interval;
        else
            seek_time = 0;
    } else { //seek forward
        if ((play_time + jump_interval) > total_time)
            seek_time = total_time;
        else
            seek_time = play_time + jump_interval;
    }
    tplayer_seekto(seek_time);
}

int lv_pro_res_media_set_speed(int speed) {
    return tplayer_setspeed(speed);
}

int lv_pro_res_movie_set_ratio(MovieRatio ratio) {
    int ret = 0;
    if (!player_context.mStartFinish) {
        mw_err("not ready!\n");
        return ret;
    }
    int sys_width = movie_res.screen.w;
    int sys_height = movie_res.screen.h;
    int value = lv_get_sys_param(P_ASPECT_RATIO);
    switch (ratio) {
        case Movie_Ratio_16_9:
        {
            if (value == 0) { /* ASPECT_RATIO_16_9_ID */
                tplayer_setratio(0, 0, sys_width, sys_height);
            } else { /* ASPECT_RATIO_4_3_ID */
                int ratio_h = sys_height/4*3;
                tplayer_setratio(0, (sys_height - ratio_h)/2, sys_width, ratio_h);
            }
            break;
        }
        case Movie_Ratio_4_3:
        {
            if (value == 0) { /* ASPECT_RATIO_16_9_ID */
                int ratio_w = sys_height/3*4;
                tplayer_setratio((sys_width - ratio_w)/2, 0, ratio_w, sys_height);
            } else { /* ASPECT_RATIO_4_3_ID */
                tplayer_setratio(0, 0, sys_width, sys_height);
            }
            break;
        }
        case Movie_Ratio_Auto:
        {
            tplayer_setratio(movie_res.movie.x, movie_res.movie.y,
                    movie_res.movie.w, movie_res.movie.h);
            break;
        }
        default:
        {
            ret = -1;
            break;
        }
    }
    return ret;
}

void lv_pro_res_movie_get_info(struct MovieInfo *mi) {
    int width = 0, height = 0;
    float filesize = 0;
    MediaInfo* MediaInfo = NULL;
    MediaInfo = tplayer_getmediainfo();
    if(MediaInfo != NULL) {
        if (MediaInfo->nFileSize > 0) {
            if (MediaInfo->nFileSize < 1024) {
                filesize = MediaInfo->nFileSize;
                snprintf(mi->filesize, sizeof(mi->filesize), "%.2fB", filesize);
            } else if ((MediaInfo->nFileSize >= 1024) && (MediaInfo->nFileSize < 1048576)) {
                filesize = MediaInfo->nFileSize / 1024.0;
                snprintf(mi->filesize, sizeof(mi->filesize), "%.2fKB", filesize);
            } else if ((MediaInfo->nFileSize >= 1048576) && (MediaInfo->nFileSize < 1073741824)) {
                filesize = MediaInfo->nFileSize / 1048576.0;
                snprintf(mi->filesize, sizeof(mi->filesize), "%.2fMB", filesize);
            } else {
                filesize = MediaInfo->nFileSize / 1073741824.0;
                snprintf(mi->filesize, sizeof(mi->filesize), "%.2fGB", filesize);
            }
        } else {
            strcpy(mi->filesize, "--");
        }

        if(MediaInfo->pVideoStreamInfo != NULL) {
            width = MediaInfo->pVideoStreamInfo->nWidth;
            height = MediaInfo->pVideoStreamInfo->nHeight;
            if (width || height) {
                snprintf(mi->resolution, sizeof(mi->resolution), "%dx%d", width, height);
            } else {
                strcpy(mi->resolution, "--x--");
            }
        } else {
            strcpy(mi->resolution, "--x--");
        }

        mi->audio = MediaInfo->nAudioStreamNum;
        mi->subtitle = MediaInfo->nSubtitleStreamNum;
    } else {
        strcpy(mi->filesize, "--");
        strcpy(mi->resolution, "--x--");
        mi->audio = 0;
        mi->subtitle = 0;
    }
}

int lv_pro_res_movie_switch_audio(int nStreamIndex) {
    return tplayer_switchaudio(nStreamIndex - 1);
}

int lv_pro_res_movie_switch_subtitle(int nStreamIndex) {
    return tplayer_switchsubtitle(nStreamIndex - 1);
}

void lv_pro_res_music_get_info(struct MusicInfo *mi) {
    float filesize = 0;
    char album[MUSIC_INFO_MAXSIZE] = {0};
    char artist[MUSIC_INFO_MAXSIZE] = {0};
    MediaInfo* MediaInfo = NULL;
    MediaInfo = tplayer_getmediainfo();
    if(MediaInfo != NULL) {
        if (MediaInfo->nFileSize) {
            if (MediaInfo->nFileSize < 1024) {
                filesize = MediaInfo->nFileSize;
                snprintf(mi->filesize, sizeof(mi->filesize), "%.2fB", filesize);
            } else if ((MediaInfo->nFileSize >= 1024) && (MediaInfo->nFileSize < 1048576)) {
                filesize = MediaInfo->nFileSize / 1024.0;
                snprintf(mi->filesize, sizeof(mi->filesize), "%.2fKB", filesize);
            } else if ((MediaInfo->nFileSize >= 1048576) && (MediaInfo->nFileSize < 1073741824)) {
                filesize = MediaInfo->nFileSize / 1048576.0;
                snprintf(mi->filesize, sizeof(mi->filesize), "%.2fMB", filesize);
            } else {
                filesize = MediaInfo->nFileSize / 1073741824.0;
                snprintf(mi->filesize, sizeof(mi->filesize), "%.2fGB", filesize);
            }
        } else {
            strcpy(mi->filesize, "--");
        }

        if(MediaInfo->mId3Info.albumsz > 0) {
            string_fmt_conv_to_utf8(MediaInfo->mId3Info.album, &album, MUSIC_INFO_MAXSIZE/2);
            snprintf(mi->album, sizeof(mi->album), "%s", album);
        } else {
            strcpy(mi->album, "--");
        }

        if(MediaInfo->mId3Info.artistsz > 0) {
            string_fmt_conv_to_utf8(MediaInfo->mId3Info.artist, &artist, MUSIC_INFO_MAXSIZE/2);
            snprintf(mi->artist, sizeof(mi->artist), "%s", artist);
        } else {
            strcpy(mi->artist, "--");
        }
    } else {
        strcpy(mi->filesize, "--");
        strcpy(mi->album, "--");
        strcpy(mi->artist, "--");
    }
}

void lv_pro_res_media_get_percent(double *percent) {
    int curDuration, totalDuration;
    tplayer_getduration(&totalDuration);
    tplayer_getcurrentpos(&curDuration);
    *percent = (double) curDuration / totalDuration;
}

void lv_pro_res_media_get_time(char *curTime, char *totalTime, double *percent) {
    int ret, curDuration, totalDuration, timeMin, timeSec;
    double sec;

    ret = tplayer_getduration(&totalDuration);

    if (!ret) {
        timeMin = totalDuration / 1000 / 60;
        sec = (double) totalDuration / 1000 / 60 - timeMin;
        timeSec = sec * 60;
        sprintf(totalTime, "%02d:%02d", timeMin, timeSec);
        ret = tplayer_getcurrentpos(&curDuration);

        if (!ret) {
            timeMin = curDuration / 1000 / 60;
            sec = (double) curDuration / 1000 / 60 - timeMin;
            timeSec = sec * 60;
            sprintf(curTime, "%02d:%02d", timeMin, timeSec);

            *percent = (double) curDuration / totalDuration;
        } else {
            strcpy(curTime, "00:00");
            *percent = 0;
        }
    } else {
        strcpy(totalTime, "00:00");
        strcpy(curTime, "00:00");
        *percent = 0;
    }

}

list_head_t* lv_pro_res_media_get_media_list() {
    return media_list;
}

bool lv_pro_res_media_get_playing() {
    return media_is_playing;
}

#endif

