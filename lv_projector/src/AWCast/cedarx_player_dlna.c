
#include <irender.h>
#include <pthread.h>
#include <string.h>

#ifndef LOG_TAG
#define LOG_TAG "DLNA"
#endif
#define CONFIG_LOG_LEVEL LOG_LEVEL_DEBUG
#include <cdx_log.h>
#include <CdxQueue.h>
#include <xplayer.h>
//#include <dlna_ui.h>
#include "iscrctl_draw.h"

#include "tsound_ctrl.h"

extern LayerCtrl *LayerCreate();
extern SoundCtrl* TSoundDeviceCreate(AudioFrameCallback callback, void* pUser);
extern int TSoundDeviceSetVolume(SoundCtrl* s, int vol);
extern SoundCtrl *SoundDeviceCreate();

extern SubCtrl *SubtitleCreate();
extern Deinterlace *DeinterlaceCreate();
extern LayerCtrl *WFD_LayerCreate_DE();
static int __CR_SetUrl(RenderT *render, char *url, char *title, int type);

typedef enum {
    PLAYER_CTL_NULL = 0,
    PLAYER_CTL_STOP = 1,

    PLAYER_CTL_REPEAT = 2,
    PLAYER_CTL_EXIT = 3,
    PLAYER_CTL_PLAY = 4,
    PLAYER_CTL_SEEK = 5,
    PLAYER_CTL_SET_URL = 6,
} PlayerCtlMsgE;

const char *XPLAYER_CALLBACK_NAME[] = {
    [AWPLAYER_MEDIA_NOP] = "AWPLAYER_MEDIA_NOP",
    [AWPLAYER_MEDIA_PREPARED] = "AWPLAYER_MEDIA_PREPARED",
    [AWPLAYER_MEDIA_PLAYBACK_COMPLETE] = "AWPLAYER_MEDIA_PLAYBACK_COMPLETE",
    [AWPLAYER_MEDIA_BUFFERING_UPDATE] = "AWPLAYER_MEDIA_BUFFERING_UPDATE",
    [AWPLAYER_MEDIA_SEEK_COMPLETE] = "AWPLAYER_MEDIA_SEEK_COMPLETE",
    [AWPLAYER_MEDIA_SET_VIDEO_SIZE] = "AWPLAYER_MEDIA_SET_VIDEO_SIZE",
    [AWPLAYER_MEDIA_STARTED] = "AWPLAYER_MEDIA_STARTED",
    [AWPLAYER_MEDIA_PAUSED] = "AWPLAYER_MEDIA_PAUSED",
    [AWPLAYER_MEDIA_STOPPED] = "AWPLAYER_MEDIA_STOPPED",
    [AWPLAYER_MEDIA_SKIPPED] = "AWPLAYER_MEDIA_SKIPPED",
    [AWPLAYER_MEDIA_TIMED_TEXT] = "AWPLAYER_MEDIA_TIMED_TEXT",
    [AWPLAYER_MEDIA_ERROR] = "AWPLAYER_MEDIA_ERROR",
    [AWPLAYER_MEDIA_INFO] = "AWPLAYER_MEDIA_INFO",
    [AWPLAYER_MEDIA_SUBTITLE_DATA] = "AWPLAYER_MEDIA_SUBTITLE_DATA",
    [AWPLAYER_MEDIA_LOG_RECORDER] = "AWPLAYER_MEDIA_LOG_RECORDER",
    [AWPLAYER_EXTEND_MEDIA_INFO] = "AWPLAYER_EXTEND_MEDIA_INFO",
    [AWPLAYER_MEDIA_META_DATA] = "AWPLAYER_MEDIA_META_DATA",
    [AWPLAYER_MEDIA_EVENT_MAX] = "AWPLAYER_MEDIA_EVENT_MAX",
    [AWPLAYER_MEDIA_RESET_BUFFER_FINISHED] = "AWPLAYER_MEDIA_RESET_BUFFER_FINISHED",
};

#define TITLE_STR_LEN 1024
#define URL_STR_LEN 2048
struct CedarRenderImplS {
    RenderT base;
    XPlayer *cedar_player;
    SoundCtrl *sound;

	SrcctlDrawT *sc_draw;

    int mPreStatus;
    int mStatus;
    int mError;
    pthread_mutex_t mMutex;
    int x;
    int y;
    int w;
    int h;
    int repeat_flag;
    char uri[URL_STR_LEN];
    char title[TITLE_STR_LEN];

    CdxQueueT *msg_queue;
    AwPoolT *pool;
    pthread_t msg_pid;
    int msg_thread_run; // 1:run. 0: exit
    int media_type;
    int mute;
    int volume;
    int async_play_request;
    int duration_ms;
    int position_ms;
    int seekTime;
    RenderCallback mCallback;
    char ad_url[URL_STR_LEN];
    int bAdPlayed;
    int firstRender;
};

//* a callback for awplayer.
static int __CallbackForXPlayer(void *pUserData, int msg, int ext1, void *param) {
    struct CedarRenderImplS *impl = (struct CedarRenderImplS *)pUserData;

    switch (msg) {
        case AWPLAYER_MEDIA_ERROR: {
            impl->mStatus = RENDER_STATE_STOP;
            impl->mPreStatus = RENDER_STATE_STOP;
            CDX_LOGD("error: open media source fail.\n");
            impl->mError = 1;

            CDX_LOGE(" error : how to deal with it");
            break;
        }

        case AWPLAYER_MEDIA_PREPARED: {
            CDX_LOGD("info : preared");
            impl->mPreStatus = impl->mStatus;
            impl->mStatus = RENDER_STATE_PREPARED;
            CDX_LOGD("info: prepare ok.");
            // if (impl->async_play_request)
            if (1) {
                CdxQueuePush(impl->msg_queue, (CdxQueueDataT)PLAYER_CTL_PLAY);
            }

            break;
        }

        case AWPLAYER_MEDIA_PLAYBACK_COMPLETE: {
            if (impl->repeat_flag) {
                CdxQueuePush(impl->msg_queue, (CdxQueueDataT)PLAYER_CTL_REPEAT);
                CDX_LOGD("repeat now...");
            } else {
                if (impl->ad_url[0] != 0 && impl->bAdPlayed == 0) {
                    CDX_LOGD("this advertisement has playback complete, now start to play user video");
                    impl->bAdPlayed = 1;
                    CdxQueuePush(impl->msg_queue, (CdxQueueDataT)PLAYER_CTL_SET_URL);
                    break;
                }
                CDX_LOGD("this video has playback complete.");
                impl->mPreStatus = impl->mStatus;
                impl->mStatus = RENDER_STATE_STOP;
                impl->mCallback(RENDER_MESSAGE_PLAYBACK_COMPLETE, NULL);
                // CdxQueuePush(impl->msg_queue, (CdxQueueDataT)PLAYER_CTL_EXIT);
            }
            break;
        }

        case AWPLAYER_MEDIA_INFO:
            CDX_LOGD("AWPLAYER_MEDIA_INFO: ext1=%d", ext1);

            if (ext1 == AW_MEDIA_INFO_BUFFERING_START) {
                CDX_LOGD("AW_MEDIA_INFO_BUFFERING_START");
                SrcctlDraw_BufferingStart(impl->sc_draw);
            } else if (ext1 == AW_MEDIA_INFO_BUFFERING_END) {
                CDX_LOGD("AW_MEDIA_INFO_BUFFERING_END");
                SrcctlDraw_BufferingEnd(impl->sc_draw);
            } else if (ext1 == AW_MEDIA_INFO_RENDERING_START) {
                if (impl->firstRender == 0) {
                    SrcctlDraw_VideoShow(impl->sc_draw);
                    impl->firstRender = 1;
                }
                SrcctlDraw_BufferingEnd(impl->sc_draw);
            } else {
                CDX_LOGD("AWPLAYER_MEDIA_INFO: ext1=%d", ext1);
            }
            break;
        case AWPLAYER_MEDIA_SET_VIDEO_SIZE: {
            int w, h;
            w = ((int*)param)[0];
            h = ((int*)param)[1];

            CDX_LOGD("set videosize (%d, %d)", w, h);
            break;
        }
        default: {
            if (msg != AWPLAYER_MEDIA_BUFFERING_UPDATE)
                CDX_LOGD("xplayer report event '%s' ", XPLAYER_CALLBACK_NAME[msg]);
            break;
        }
    }

    return 0;
}

static int playerInit(struct CedarRenderImplS *impl) {
    //* create a player.
    impl->cedar_player = XPlayerCreate();
    if (impl->cedar_player == NULL) {
        CDX_LOGD("can not create AwPlayer, quit.\n");
        return -1;
    }

    //* set callback to player.
    XPlayerSetNotifyCallback(impl->cedar_player, __CallbackForXPlayer, (void *)impl);

    //* check if the player work.
    if (XPlayerInitCheck(impl->cedar_player) != 0) {
        CDX_LOGD("initCheck of the player fail, quit.\n");
        XPlayerDestroy(impl->cedar_player);
        impl->cedar_player = NULL;
        return -1;
    }

    // LayerCtrl* layer = LayerCreate();
    LayerCtrl *layer = WFD_LayerCreate_DE();
    // if ((impl->w > 0) && (impl->h > 0))
    //{
    //	LayerSetDisplayRegion(layer, impl->x, impl->y, impl->w, impl->h);
    // }

    impl->sound = TSoundDeviceCreate(NULL, NULL);
    // SoundCtrl* sound = SoundDeviceCreate();
    SubCtrl *sub = SubtitleCreate();

    XPlayerSetAudioSink(impl->cedar_player, impl->sound);
    XPlayerSetVideoSurfaceTexture(impl->cedar_player, (void *)layer);
    XPlayerSetSubCtrl(impl->cedar_player, sub);

    impl->mStatus = RENDER_STATE_IDEL;
    impl->async_play_request = 0;
    return 0;
}

static int playerPrepare(struct CedarRenderImplS *impl) {
    if (impl->cedar_player == NULL) {
        CDX_LOGE("playerPrepare error, impl->cedar_player = NULL!\n");
        return -1;
    }
    impl->mStatus = RENDER_STATE_PREPAING;

    /* set url */
    // CDX_LOGD("uri: '%s'", impl->uri);
    char *url;
    if (impl->ad_url[0] != 0 && impl->bAdPlayed == 0) {
        CDX_LOGW("play ad url");
        url = &impl->ad_url[0];
    } else {
        CDX_LOGW("play user url");
        url = &impl->uri[0];
    }

    if (XPlayerSetDataSourceUrl(impl->cedar_player, url, NULL, NULL) != 0) {
        CDX_LOGD("error:\n");
        CDX_LOGD("    AwPlayer::setDataSource() return fail.\n");
        return -1;
    }
    CDX_LOGD("setDataSource end\n");

    //* start preparing.
    if (XPlayerPrepareAsync(impl->cedar_player) != 0) {
        CDX_LOGD("error:\n");
        CDX_LOGD("    AwPlayer::prepareAsync() return fail.\n");
        return -1;
    }

    //    impl->mPreStatus = RENDER_STATE_IDEL;
    CDX_LOGD("preparing...\n");
    //SrcctlDraw_BufferingStart(impl->sc_draw);

    return 0;
}

static int playerStart(struct CedarRenderImplS *impl) {
    pthread_mutex_lock(&impl->mMutex);
    if (impl->cedar_player == NULL) {
        CDX_LOGE("playerStart error, impl->cedar_player = NULL!\n");
        pthread_mutex_unlock(&impl->mMutex);
        return -1;
    }
    SrcctlDraw_EntryHide(impl->sc_draw);

    /* start */
    if (XPlayerStart(impl->cedar_player) != 0) {
        CDX_LOGD("XPlayerStart() return fail.\n");
        pthread_mutex_unlock(&impl->mMutex);
        return -1;
    }
    impl->mPreStatus = impl->mStatus;
    impl->mStatus = RENDER_STATE_PLAYING;
    CDX_LOGD("playing.\n");
    pthread_mutex_unlock(&impl->mMutex);

    return 0;
}

static int playerStop(struct CedarRenderImplS *impl) {
    int ret = 0;
    CDX_LOGD("playerStop");
    if (impl->cedar_player == NULL) {
        return -1;
    }

    if (impl->cedar_player != NULL) {
        CDX_LOGD("XPlayerStop ........");
        ret = XPlayerStop(impl->cedar_player);
        //impl->cedar_player = NULL;
    }
    impl->mPreStatus = impl->mStatus;
    impl->mStatus = RENDER_STATE_STOP;

    return ret;
}

static int playerPause(struct CedarRenderImplS *impl) {
    pthread_mutex_lock(&impl->mMutex);
    int ret = 0;
    if (impl->cedar_player == NULL) {
        CDX_LOGE("playerPause error, impl->cedar_player = NULL!\n");
        pthread_mutex_unlock(&impl->mMutex);
        return -1;
    }
    ret = XPlayerPause(impl->cedar_player);
    impl->mPreStatus = impl->mStatus;
    impl->mStatus = RENDER_STATE_PAUSE;
    pthread_mutex_unlock(&impl->mMutex);
    return ret;
}

static int playerResume(struct CedarRenderImplS *impl) {
    pthread_mutex_lock(&impl->mMutex);
    int ret = 0;
    if (impl->cedar_player == NULL) {
        CDX_LOGE("playerResume error, impl->cedar_player = NULL!\n");
        pthread_mutex_unlock(&impl->mMutex);
        return -1;
    }
    ret = XPlayerStart(impl->cedar_player);
    impl->mPreStatus = impl->mStatus;
    impl->mStatus = RENDER_STATE_PLAYING;
    pthread_mutex_unlock(&impl->mMutex);
    return ret;
}

static int isMusic(struct CedarRenderImplS *impl) {
    pthread_mutex_lock(&impl->mMutex);
    if (impl->cedar_player == NULL) {
        CDX_LOGE("isMusic error, impl->cedar_player = NULL!\n");
        pthread_mutex_unlock(&impl->mMutex);
        return 0;
    }

    MediaInfo *mi = XPlayerGetMediaInfo(impl->cedar_player);
    pthread_mutex_unlock(&impl->mMutex);
    if (mi == NULL) {
        CDX_LOGE("get MediaInfo failed");
        return 0;
    }

    CDX_LOGD("video num: %d, audio num: %d\n", mi->nVideoStreamNum, mi->nAudioStreamNum);
    if (mi->nVideoStreamNum == 0 && mi->nAudioStreamNum > 0) {
        return 1;
    }
    return 0;
}

static void printLongString(char *comment, char *str) {
    int len = 0, parts = 0, i = 0;

    len = strlen(str);
    parts = len / 256 + 1;
    char buf[260] = {0};

    for (i = 0; i < parts; i++) {
        snprintf(buf, 256 + 1, "%s", str + (256 * i));
        CDX_LOGD("'%s': %s", comment, buf);
    }
    return;
}

static void *__MessageProc(void *param) {
    struct CedarRenderImplS *impl = (struct CedarRenderImplS *)param;
    int ret;

    /* msg main process */
    while (impl->msg_thread_run) {
        PlayerCtlMsgE msg = (PlayerCtlMsgE)CdxQueuePop(impl->msg_queue);
        if (msg == 0) {
            usleep(20000);
            continue;
        }
        switch (msg) {
            case PLAYER_CTL_STOP: {
                ret = playerStop(impl);
                if (ret != 0) {
                    CDX_LOGE("stop error....");
                }
                break;
            }
            case PLAYER_CTL_PLAY: {
                CDX_LOGD("receive Message: PLAYER_CTL_PLAY\n");
                ret = playerStart(impl);
                if (ret != 0) {
                    CDX_LOGE("stop error....");
                }
                if (isMusic(impl)) {
                    //SrcctlDraw_EntryShow(impl->sc_draw, "测试音乐");
                }
                break;
            }
            case PLAYER_CTL_SEEK: {
                pthread_mutex_lock(&impl->mMutex);
                CDX_LOGD("receive Message: PLAYER_CTL_SEEK\n");
                if (impl->cedar_player != NULL) {
                    XPlayerSeekTo(impl->cedar_player, impl->seekTime, AW_SEEK_PREVIOUS_SYNC);
                }
                pthread_mutex_unlock(&impl->mMutex);
                break;
            }
            case PLAYER_CTL_EXIT: {
                // goto out;
                break;
            }
            case PLAYER_CTL_SET_URL: {
                CDX_LOGW("PLAYER_CTL_SET_URL");
                __CR_SetUrl(&impl->base, impl->uri, impl->title, 0);
                break;
            }

            default:
                CDX_LOGE("what happen!!! shold not be here... '%d' ", msg);
                break;
        }
    }

out:
    CDX_LOGD("cedar player exit....");
    return NULL;
}

static int __CR_SetUrl(RenderT *render, char *url, char *title, int type) {
    CDX_LOGD("'%s' in", __FUNCTION__);
    struct CedarRenderImplS *impl = CdxContainerOf(render, struct CedarRenderImplS, base);
    pthread_mutex_lock(&impl->mMutex);
    impl->mStatus = RENDER_STATE_IDEL;

    int ret = 0;

    SrcctlDraw_EntryShow(impl->sc_draw, "set url");

    if (impl->cedar_player != NULL) {
        // ret = playerStop(impl);
        CDX_LOGD("player is in the playing status, first stop xplayer, then set url");
        XPlayerDestroy(impl->cedar_player);
        impl->cedar_player = NULL;
        impl->duration_ms = 0;
    }

    if (url == NULL) {
        CDX_LOGE("url is NULL");
        pthread_mutex_unlock(&impl->mMutex);
        return -1;
    }

    playerInit(impl);
    printLongString("url", url);

    CDX_LOG_CHECK(strlen(url) < URL_STR_LEN, "url too long '%d'", strlen(url));

    strcpy(impl->uri, url);
    strncpy(impl->title, title, TITLE_STR_LEN - 1);
    impl->media_type = type;

    playerPrepare(impl);
    pthread_mutex_unlock(&impl->mMutex);

    return ret;
}

static int __CR_Play(RenderT *render) {
    struct CedarRenderImplS *impl = CdxContainerOf(render, struct CedarRenderImplS, base);

    int ret = 0;
    CDX_LOGD("'%s' in", __FUNCTION__);

    // if (impl->mStatus == RENDER_STATE_PREPARED)
    //{
    //	playerStart(impl);
    // }
    // else
    //{
    //	impl->async_play_request = 1;
    // }

    return ret;
}

static int __CR_Stop(RenderT *render) {
    struct CedarRenderImplS *impl = CdxContainerOf(render, struct CedarRenderImplS, base);
    pthread_mutex_lock(&impl->mMutex);
    int ret = 0;
    CDX_LOGD("'%s' in", __FUNCTION__);

    SrcctlDraw_BufferingEnd(impl->sc_draw);

    if (impl->mStatus != RENDER_STATE_STOP) {
        ret = playerStop(impl);

        if (impl->cedar_player != NULL) {
            XPlayerDestroy(impl->cedar_player);
            impl->cedar_player = NULL;
            impl->duration_ms = 0;
        }
    } else {
        CDX_LOGW("player is already in stopped status! ignore this call");
    }
    pthread_mutex_unlock(&impl->mMutex);

    return ret;
}

static int __CR_Rotate(RenderT *render, enum ROTATEDEGREE eRotateDegree) {
    struct CedarRenderImplS *impl = CdxContainerOf(render, struct CedarRenderImplS, base);
    pthread_mutex_lock(&impl->mMutex);
    int ret = 0;
    //CDX_LOGD("'%s' TODO, unsupport now in", __FUNCTION__);

    if (impl->cedar_player) ret = XPlayerSetRotateDegree(impl->cedar_player, eRotateDegree);
    pthread_mutex_unlock(&impl->mMutex);
    return ret;
}

static int __CR_setAdUrl(RenderT *render, char *ad_url) {
    struct CedarRenderImplS *impl = CdxContainerOf(render, struct CedarRenderImplS, base);
    pthread_mutex_lock(&impl->mMutex);
    int ret = 0;
    CDX_LOGD("'%s' in, ad_url is %s", __FUNCTION__, ad_url);
    strcpy(impl->ad_url, ad_url);
    pthread_mutex_unlock(&impl->mMutex);
    return ret;
}

static int __CR_Pause(RenderT *render) {
    struct CedarRenderImplS *impl = CdxContainerOf(render, struct CedarRenderImplS, base);
    int ret = 0;
    CDX_LOGD("'%s' in", __FUNCTION__);

    playerPause(impl);

    if (impl->duration_ms == 0) {
        XPlayerGetDuration(impl->cedar_player, &impl->duration_ms);
    }

    int position_ms = 0;
    ret = XPlayerGetCurrentPosition(impl->cedar_player, &position_ms);

    SrcctlDraw_ProcessBarShow(impl->sc_draw, impl->duration_ms, position_ms);
    return ret;
}

static int __CR_Resume(RenderT *render) {
    struct CedarRenderImplS *impl = CdxContainerOf(render, struct CedarRenderImplS, base);
    int ret = 0;
    CDX_LOGD("'%s' in", __FUNCTION__);

    SrcctlDraw_ProcessBarHide(impl->sc_draw);

    playerResume(impl);
    return ret;
}

static int __CR_Seekto(RenderT *render, int time_ms) {
    struct CedarRenderImplS *impl = CdxContainerOf(render, struct CedarRenderImplS, base);
    CDX_LOGD("%s in", __FUNCTION__);
    int ret;
    impl->seekTime = time_ms;
    ret = CdxQueuePush(impl->msg_queue, (CdxQueueDataT)PLAYER_CTL_SEEK);
    return ret;
}

static int __CR_GetPosition(RenderT *render) {
    struct CedarRenderImplS *impl = CdxContainerOf(render, struct CedarRenderImplS, base);
    int ret = 0;
    int position_ms = 0;
    // CDX_LOGD("'%s' in", __FUNCTION__);

    if (impl->ad_url[0] != 0 && impl->bAdPlayed == 0) {
        return -1; // ad playing, return NOT_IMPLEMENTED
    }
    if (impl->cedar_player != NULL) {
        ret = XPlayerGetCurrentPosition(impl->cedar_player, &position_ms);
        impl->position_ms = position_ms;
    }
    return impl->position_ms;
}

static int __CR_GetDuration(RenderT *render) {
    struct CedarRenderImplS *impl = CdxContainerOf(render, struct CedarRenderImplS, base);
    int ret = 0;

    // CDX_LOGD("'%s' in", __FUNCTION__);
    if (impl->ad_url[0] != 0 && impl->bAdPlayed == 0) {
        return -1; // ad playing, return NOT_IMPLEMENTED
    }

    if (impl->duration_ms) {
        return impl->duration_ms;
    }

    if (impl->cedar_player != NULL) {
        ret = XPlayerGetDuration(impl->cedar_player, &impl->duration_ms);
    }

    return impl->duration_ms;
}

static int __CR_GetState(RenderT *render) {
    struct CedarRenderImplS *impl = CdxContainerOf(render, struct CedarRenderImplS, base);
    int ret = 0;
    //	CDX_LOGD("CedarPlayer state '%d' in", impl->mStatus);

    return impl->mStatus;
}

static int __CR_Destroy(RenderT *render) {
    struct CedarRenderImplS *impl = CdxContainerOf(render, struct CedarRenderImplS, base);
    int ret = 0;
    CDX_LOGD("'%s' in", __FUNCTION__);

    if (impl->cedar_player != NULL) {
        XPlayerDestroy(impl->cedar_player);
        impl->cedar_player = NULL;
    }

    impl->msg_thread_run = 0;
    pthread_join(impl->msg_pid, NULL);
    CDX_LOGD("'%s' in", __FUNCTION__);

    CdxQueueDestroy(impl->msg_queue);
    impl->msg_queue = NULL;
    AwPoolDestroy(impl->pool);
    impl->pool = NULL;
    pthread_mutex_destroy(&impl->mMutex);

    free(impl);

    return ret;
}

int __CR_SetMute(RenderT *render, int mute) {
    struct CedarRenderImplS *impl = CdxContainerOf(render, struct CedarRenderImplS, base);
    CDX_LOGD("'%s' in", __FUNCTION__);

    impl->mute = mute;
    return 0;
}

int __CR_GetMute(RenderT *render) {
    struct CedarRenderImplS *impl = CdxContainerOf(render, struct CedarRenderImplS, base);
    CDX_LOGD("'%s' in", __FUNCTION__);

    return impl->mute;
}

int __CR_SetVolume(RenderT *render, int volume) {
    struct CedarRenderImplS *impl = CdxContainerOf(render, struct CedarRenderImplS, base);
    CDX_LOGD("SetVolume '%d' in", volume);

    int vol_ui;

    impl->volume = volume;

    TSoundDeviceSetVolume(impl->sound, volume);

    vol_ui = volume / 7 + (!!(volume % 7));

    if (vol_ui > 15) {
        vol_ui = 15;
    }

    if (vol_ui < 0) {
        vol_ui = 0;
    }

    SrcctlDraw_VolumeBarShow(impl->sc_draw, vol_ui);

    return 0;
}

int __CR_GetVolume(RenderT *render) {
    struct CedarRenderImplS *impl = CdxContainerOf(render, struct CedarRenderImplS, base);
    CDX_LOGD("%s in, volume: %d", __FUNCTION__, impl->volume);

    return impl->volume;
}

static int __CR_SetCallback(RenderT *render, RenderCallback callback) {
    struct CedarRenderImplS *impl = CdxContainerOf(render, struct CedarRenderImplS, base);
    impl->mCallback = callback;
    return 0;
}

struct IRenderOpsS cedar_render_ops = {.set_url = __CR_SetUrl,
                                       .play = __CR_Play,
                                       .stop = __CR_Stop,
                                       .rotate = __CR_Rotate,
                                       .pause = __CR_Pause,
                                       .resume = __CR_Resume,
                                       .seekto = __CR_Seekto,
                                       .get_position = __CR_GetPosition,
                                       .get_duration = __CR_GetDuration,
                                       .get_state = __CR_GetState,
                                       .set_mute = __CR_SetMute,
                                       .get_mute = __CR_GetMute,
                                       .set_volume = __CR_SetVolume,
                                       .get_volume = __CR_GetVolume,
                                       .destroy = __CR_Destroy,
                                       .setCallback = __CR_SetCallback,
                                       .setAdUrl = __CR_setAdUrl};

RenderT *CedarRender_Instance(SrcctlDrawT *sc_draw)
{
    struct CedarRenderImplS *impl;
    CDX_LOGD("'%s' in", __FUNCTION__);

    impl = calloc(1, sizeof(*impl));
    if (impl == NULL) {
        CDX_LOGE("calloc fail, %d", sizeof(*impl));
        return NULL;
    }
    // for test
    // strcpy(impl->ad_url, "https://www.w3school.com.cn/example/html5/mov_bbb.mp4");

    pthread_mutex_init(&impl->mMutex, NULL);
    impl->pool = AwPoolCreate(NULL);
    impl->msg_queue = CdxQueueCreate(impl->pool);
    impl->msg_thread_run = 1;
    pthread_create(&impl->msg_pid, NULL, __MessageProc, impl);

    impl->sc_draw = sc_draw;

    impl->base.ops = &cedar_render_ops;

    return &impl->base;
}

#if 0
int CedarRender_Destroy(RenderT *render) {
    struct CedarRenderImplS *impl = CdxContainerOf(render, struct CedarRenderImplS, base);

    // TODO: free SrcctlDraw_Instance

    impl->msg_thread_run = 0;
    pthread_join(impl->msg_pid, NULL);
    CdxQueueDestroy(impl->msg_queue);
    impl->msg_queue = NULL;
    AwPoolDestroy(impl->pool);
    impl->pool = NULL;
    pthread_mutex_destroy(&impl->mMutex);
    free(impl);
    impl = NULL;
    return 0;
}
#endif
