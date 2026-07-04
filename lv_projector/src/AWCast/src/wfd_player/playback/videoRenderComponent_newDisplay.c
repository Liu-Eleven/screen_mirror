/*
 * Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
 * All rights reserved.
 *
 * File : videoRenderComponent_newDisplay.cpp
 * Description : video render component
 * History :
 *
 */

//#define CONFIG_LOG_LEVEL    OPTION_LOG_LEVEL_DETAIL
//#define LOG_TAG "videoRenderComponent_newDisplay"
#define LOG_TAG "WFDPlayer2"
#include "cdx_log.h"

#include <pthread.h>
#include <semaphore.h>
#include <malloc.h>
#include <memory.h>
#include <time.h>
#include <sys/prctl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/ioctl.h>

#include <sys/prctl.h>

#include "baseComponent.h"
#include "videoRenderComponent.h"
#include "AwMessageQueue.h"
#include "layerControl.h"
#include "memoryAdapter.h"
#if defined(CONF_SUPPORT_G2D)
#include "g2d_driver.h"
#endif

#define USE_DEINTERLACE 1

#define SUPPORT_DENOISE_PROCESS 1
#define DEBUG_DENOISE_TIME 0
#define DENOISE_WIDTH_MAX  1920
#define DENOISE_HEIGHT_MAX 1080

static inline int64_t GetCurrentTime(void)
{
    struct timeval tv;
    int64_t time;
    gettimeofday(&tv,NULL);
    time = tv.tv_sec*1000000LL + tv.tv_usec;
    return time;
}

enum VIDEO_RENDER_RESULT
{
    VIDEO_RENDER_MESSAGE_COME      = 1,
    VIDEO_RENDER_DEINTERLACE_RESET = 2,
    VIDEO_RENDER_DROP_THE_PICTURE  = 3,
    VIDEO_RENDER_THREAD_CONTINUE   = 4,
};

struct VideoRenderComp
{
    AwMessageQueue      *mq;
    BaseCompCtx          base;

    pthread_t            sRenderThread;

    enum EPLAYERSTATUS   eStatus;

    LayerCtrl*           pLayerCtrl;
    VideoDecComp*        pDecComp;

    enum EPICTURE3DMODE  ePicture3DMode;
    enum EDISPLAY3DMODE  eDisplay3DMode;

    //* objects set by user.
    AvTimer*             pAvTimer;
    PlayerCallback       callback;
    void*                pUserData;
    int                  bEosFlag;

    //*
    int                  bResolutionChange;

    int                  bHadSetLayerInfoFlag;
    int                  bProtectedBufferFlag;//* 1: mean the video picture is secure
    int                  bHadGetBufferQueueToSF;
    int                  bBufferQueueToSF; //get videoBuffer whether QueueToSurfaceFlinger;

    //* for 3D video stream
    int                  bVideoWithTwoStream;

    FbmBufInfo           mFbmBufInfo;
    //******
    int                  bFirstPictureShowed;
    int                  bNeedResetLayerParams;
    int                  bHideVideo;
    VideoPicture*        pPicture;
    VideoPicture*        pCurrentPicture;//record the curren picturen for rotate when there is no newer picture to show
    VideoPicture*        pPrePicture;
    VideoPicture*        pPrePrePicture;
    int                  bHadGetVideoFbmBufInfoFlag;
    int                  nGpuBufferNum;
    int                  bHadSetBufferToDecoderFlag;
    VideoPicture*        pCancelPicture[10];
    int                  nCancelPictureNum;

    int                  nSetToDecoderBufferNum;

    VideoPicture*        pDiOutPicture;
    int                  bResetBufToDecoderFlag;
    int                  bHadRequestReleasePicFlag;
    int                  nNeedReleaseBufferNum;

    int                  ptsSecs;

    int                  bHoldLastPicture;
    int                  bFirstPtsNotified;
    float                fPlayRate;
    int                  bTopFieldFirstXX;
    int64_t*             nPtsList;
    int64_t*             nPtsGetTime;
    int                  bSetStatics;
    enum ROTATEDEGREE    nDegree;
#if defined(CONF_SUPPORT_G2D)
    VideoPicture*        pG2dOutPicture;//dequeuebuffer for g2d output
    int                  nG2dFd;// /dev/g2d
    g2d_lbc_rot          mG2dInfo;
    int                  G2dPrintCount;
#endif
    int                  bSetDegree;
};

static void handleStart(AwMessage *msg, void *arg);
static void handleStop(AwMessage *msg, void *arg);
static void handlePause(AwMessage *msg, void *arg);
static void handleReset(AwMessage *msg, void *arg);
static void handleSetEos(AwMessage *msg, void *arg);
static void handleQuit(AwMessage *msg, void *arg);
static void doRender(AwMessage *msg, void *arg);

static void* VideoRenderThread(void* arg);


static void NotifyVideoSizeAndSetDisplayRegion(VideoRenderComp* p);

static inline int ProcessVideoSync(VideoRenderComp* p, VideoPicture* pPicture);
static int QueueBufferToShow(VideoRenderComp* p, VideoPicture* pPicture);
static int RenderGetVideoFbmBufInfo(VideoRenderComp* p);

static int SetGpuBufferToDecoder(VideoRenderComp *p);

static int ResetBufToDecoder(VideoRenderComp *p);

VideoRenderComp *WFD_VideoRenderCompCreate(void)
{
    VideoRenderComp* p;
    int err;

    p = (VideoRenderComp*)calloc(1, sizeof(*p));
    if(p == NULL)
    {
        loge("memory alloc fail.");
        return NULL;
    }

    p->bTopFieldFirstXX = -1;
    p->bVideoWithTwoStream = -1;
    p->nDegree = ROTATE_DEGREE_UNSET;
    p->mq = AwMessageQueueCreate(4, "VideoRenderMq");
    if(p->mq == NULL)
    {
        loge("video render component create message queue fail.");
        free(p);
        return NULL;
    }

    BaseMsgHandler handler = {
        .start = handleStart,
        .stop = handleStop,
        .pause = handlePause,
        .reset = handleReset,
        .setEos = handleSetEos,
        .quit = handleQuit,
        .render = doRender,
    };

    if (BaseCompInit(&p->base, "video render", p->mq, &handler))
    {
        AwMessageQueueDestroy(p->mq);
        free(p);
        return NULL;
    }

    p->eStatus = PLAYER_STATUS_STOPPED;
    p->ptsSecs = -1;
    p->fPlayRate = 1.0f;

    err = pthread_create(&p->sRenderThread, NULL, VideoRenderThread, p);
    if(err != 0)
    {
        loge("video render component create thread fail.");
        BaseCompDestroy(&p->base);
        AwMessageQueueDestroy(p->mq);
        free(p);
        return NULL;
    }

    return p;
}

int WFD_VideoRenderCompDestroy(VideoRenderComp* p)
{
    BaseCompQuit(&p->base, NULL, NULL);
    pthread_join(p->sRenderThread, NULL);
    BaseCompDestroy(&p->base);

    AwMessageQueueDestroy(p->mq);


    free(p);

    return 0;
}

int WFD_VideoRenderCompStart(VideoRenderComp* p)
{
    return BaseCompStart(&p->base, NULL, NULL);
}

int WFD_VideoRenderCompStop(VideoRenderComp* p)
{
    return BaseCompStop(&p->base, NULL, NULL);
}

int WFD_VideoRenderCompPause(VideoRenderComp* p)
{
    return BaseCompPause(&p->base, NULL, NULL);
}

int WFD_VideoRenderCompReset(VideoRenderComp* p)
{
    return BaseCompReset(&p->base, 0, NULL, NULL);
}

int WFD_VideoRenderCompSetEOS(VideoRenderComp* p)
{
    return BaseCompSetEos(&p->base, NULL, NULL);
}

int WFD_VideoRenderCompSetCallback(VideoRenderComp* p, PlayerCallback callback, void* pUserData)
{
    p->callback  = callback;
    p->pUserData = pUserData;

    return 0;
}

int WFD_VideoRenderCompSetTimer(VideoRenderComp* p, AvTimer* timer)
{
    p->pAvTimer = timer;
    return 0;
}

static inline void ContinueRender(VideoRenderComp* p)
{
    if (p->eStatus == PLAYER_STATUS_STARTED ||
            (p->eStatus == PLAYER_STATUS_PAUSED &&
             p->bFirstPictureShowed == 0))
        BaseCompContinue(&p->base);
}

static void returnOldPic(VideoRenderComp *p)
{
    p->pCurrentPicture = NULL;
    if(p->pPicture != NULL)
    {
        WFD_VideoDecCompReturnPicture(p->pDecComp, p->pPicture);
        p->pPicture = NULL;
    }

    if(p->pPrePicture != NULL)
    {
        WFD_VideoDecCompReturnPicture(p->pDecComp, p->pPrePicture);
        p->pPrePicture = NULL;
    }

#if defined(CONF_DI_V2X_SUPPORT)
    if(p->pPrePrePicture != NULL)
    {
        WFD_VideoDecCompReturnPicture(p->pDecComp, p->pPrePrePicture);
        p->pPrePrePicture = NULL;
    }
#endif

    if(p->pDiOutPicture != NULL)
    {
        if(p->pLayerCtrl != NULL)
            LayerQueueBuffer(p->pLayerCtrl, p->pDiOutPicture, 0);
        p->pDiOutPicture = NULL;
    }

#if defined(CONF_SUPPORT_G2D)
    if(p->pG2dOutPicture != NULL)
    {
        if(p->pLayerCtrl != NULL)
            LayerQueueBuffer(p->pLayerCtrl, p->pG2dOutPicture, 0);
        p->pG2dOutPicture = NULL;
    }
#endif
}

static void setLayerInfo(VideoRenderComp *p)
{
    enum EPIXELFORMAT eDisplayPixelFormat = PIXEL_FORMAT_DEFAULT;
    FbmBufInfo* pFbmBufInfo = &p->mFbmBufInfo;

    eDisplayPixelFormat = (enum EPIXELFORMAT)pFbmBufInfo->ePixelFormat;

    LayerSetDisplayPixelFormat(p->pLayerCtrl, eDisplayPixelFormat);
    LayerSetVideoWithTwoStreamFlag(p->pLayerCtrl, p->bVideoWithTwoStream);
    LayerSetIsSoftDecoderFlag(p->pLayerCtrl, pFbmBufInfo->bIsSoftDecoderFlag);
    LayerSetDisplayBufferSize(p->pLayerCtrl, pFbmBufInfo->nBufWidth, pFbmBufInfo->nBufHeight);
    p->nGpuBufferNum = LayerSetDisplayBufferCount(p->pLayerCtrl, pFbmBufInfo->nBufNum);

    p->bHadSetLayerInfoFlag = 1;
}

static void handleSetWindow(AwMessage *msg, void *arg)
{
    VideoRenderComp *p = arg;
    CDX_LOGD("process MESSAGE_ID_SET_WINDOW message, p->pPicture(%p)",p->pPicture);

    returnOldPic(p);
    LayerCtrl *lc = msg->opaque;

    // if the video Window change
    if(p->pLayerCtrl != NULL) //old nativewindow
    {
        //* 1. get the buffers in display and return to decoder
        VideoPicture* pPicture = NULL;
        int nWhileNum = 0;
        while(1)
        {
            nWhileNum++;
            if(nWhileNum >= 100)
            {
                loge("get pic node time more than 100, it is wrong");
                break;
            }

            pPicture = LayerGetBufferOwnedByGpu(p->pLayerCtrl);
            logv("test pPicture=%p, nWhileNum=%d", pPicture, nWhileNum);
            if(pPicture == NULL)
            {
                break;
            }
            WFD_VideoDecCompReturnPicture(p->pDecComp, pPicture);
        }

        LayerResetNativeWindow(p->pLayerCtrl, NULL);

        //2. set video decoder release flag
        WFD_VideoDecCompSetVideoFbmBufRelease(p->pDecComp);
        p->bResetBufToDecoderFlag = 1;
        p->nNeedReleaseBufferNum  = p->nGpuBufferNum;

        goto set_nativeWindow_exit;
    }
    else
    {
        p->pLayerCtrl = lc;
        LayerResetNativeWindow(p->pLayerCtrl, NULL);
    }

    if(p->pLayerCtrl != NULL)
        LayerSetSecureFlag(p->pLayerCtrl, p->bProtectedBufferFlag);

    p->bNeedResetLayerParams = 1;

    //* we should set layer info here if it hadn't set it
    if(p->bHadSetLayerInfoFlag == 0 && p->mFbmBufInfo.nBufNum != 0)
        setLayerInfo(p);

set_nativeWindow_exit:
    sem_post(msg->replySem);
    ContinueRender(p);
}

int WFD_VideoRenderCompSetWindow(VideoRenderComp* p, LayerCtrl* lc)
{
	CDX_LOGI("step...");

    if(lc == NULL)
        return -1;

    sem_t replySem;
    sem_init(&replySem, 0, 0);

    AwMessage msg = {
        .messageId = MESSAGE_ID_SET_WINDOW,
        .replySem = &replySem,
        .opaque = lc,
        .execute = handleSetWindow,
    };

    CDX_LOGD("video render component setting window: %p", lc);

    if(AwMessageQueuePostMessage(p->mq, &msg) != 0)
    {
        loge("fatal error, video render component post message fail.");
        abort();
    }

    if(SemTimedWait(&replySem, -1) < 0)
    {
        loge("video render component wait for setting window finish failed.");
        sem_destroy(&replySem);
        return -1;
    }

    sem_destroy(&replySem);
    return 0;
}

int WFD_VideoRenderCompSetDecodeComp(VideoRenderComp* p, VideoDecComp* d)
{
    p->pDecComp  = d;
    return 0;
}

int WFD_VideoRenderSet3DMode(VideoRenderComp* p,
                         enum EPICTURE3DMODE ePicture3DMode,
                         enum EDISPLAY3DMODE eDisplay3DMode)
{
    logv("video render component setting 3d mode.");

    // These two variables are useless now.
    p->ePicture3DMode = ePicture3DMode;
    p->eDisplay3DMode = eDisplay3DMode;
    return 0;
}

static void handleVideoHide(AwMessage *msg, void *arg)
{
    VideoRenderComp *p = arg;

    p->bHideVideo = msg->int64Value;

    if (p->pLayerCtrl != NULL)
    {
        if (p->bHideVideo)
            LayerCtrlHideVideo(p->pLayerCtrl);
        else if (p->bFirstPictureShowed == 1)
            LayerCtrlShowVideo(p->pLayerCtrl);
    }

    sem_post(msg->replySem);
    ContinueRender(p);
}

static void handleSetHoldLastPicture(AwMessage *msg, void *arg)
{
    VideoRenderComp *p = arg;
    p->bHoldLastPicture = msg->int64Value;

    if (p->pLayerCtrl && p->eStatus == PLAYER_STATUS_STOPPED)
    {
        LayerCtrlHoldLastPicture(p->pLayerCtrl, p->bHoldLastPicture);

        if (!p->bHoldLastPicture)
            LayerCtrlHideVideo(p->pLayerCtrl);
    }

    ContinueRender(p);
}

int VideoRenderSetHoldLastPicture(VideoRenderComp* p, int bHold)
{
    AwMessage msg = {
        .messageId = MESSAGE_ID_SET_HOLD_LAST_PICTURE,
        .replySem = NULL,
        .int64Value = bHold,
        .execute = handleSetHoldLastPicture,
    };

    logv("video render component setting hold last picture(bHold=%d).", bHold);

    if(AwMessageQueuePostMessage(p->mq, &msg) != 0)
    {
        loge("fatal error, video render component post message fail.");
        abort();
    }

    return 0;
}

void WFD_VideoRenderCompSetProtecedFlag(VideoRenderComp* p, int bProtectedFlag)
{
    p->bProtectedBufferFlag = bProtectedFlag;
    return;
}

int WFD_VideoRenderCompSetVideoStreamInfo(VideoRenderComp* v, VideoStreamInfo* pStreamInfo)
{
    CEDARX_UNUSE(v);
    CEDARX_UNUSE(pStreamInfo);
    return 0;
}

int WFD_VideoRenderCompSetSyncFirstPictureFlag(VideoRenderComp* v, int bSyncFirstPictureFlag)
{
    //*TODO
    (void)v;
    (void)bSyncFirstPictureFlag;
    return 0;
}

int WFD_VideoRenderCompSetPtsStatics(VideoRenderComp* p, int64_t* nPtsList, int64_t* nPtsGetTime)
{
    p->nPtsList = nPtsList;
    p->nPtsGetTime = nPtsGetTime;
    p->bSetStatics = 1;
    return 0;
}
static void* VideoRenderThread(void* arg)
{
    VideoRenderComp *p = arg;
    AwMessage msg;

    prctl(PR_SET_NAME, (unsigned long)"AWVideoRenderThread", 0, 0, 0);

#if defined(CONF_MEDIA_BOOST_CPU)
#if (CONF_MEDIA_BOOST_CPU_MODE == 2) // h6
    //AWVideoRenderThread prio = 105
    char cmd[256];
    sprintf(cmd, "mode%d:0:p:105", gettid());
    property_set("media.boost.pref", cmd);
    CDX_LOGD("setprop media.boost.pref %s", cmd);
#endif
#endif

    while (AwMessageQueueGetMessage(p->mq, &msg) == 0)
    {
        if (msg.execute != NULL)
            msg.execute(&msg, p);
        else
            loge("msg with msg_id %d doesn't have a handler", msg.messageId);
    }

    return NULL;
}

static void handleStart(AwMessage *msg, void *arg)
{
    VideoRenderComp *p = arg;

    if(p->eStatus == PLAYER_STATUS_STARTED)
    {
        CDX_LOGW("already in started status.");
        *msg->result = -1;
        sem_post(msg->replySem);
        BaseCompContinue(&p->base);
        return;
    }

    if(p->eStatus == PLAYER_STATUS_STOPPED)
    {
        p->bFirstPictureShowed = 0;
        p->bFirstPtsNotified = 0;
        p->bEosFlag = 0;
    }

    p->eStatus = PLAYER_STATUS_STARTED;
    *msg->result = 0;
    sem_post(msg->replySem);

    BaseCompContinue(&p->base);
}

static void handleStop(AwMessage *msg, void *arg)
{
    VideoRenderComp *p = arg;

    if(p->eStatus == PLAYER_STATUS_STOPPED)
    {
        CDX_LOGW("already in stopped status.");
        *msg->result = -1;
        sem_post(msg->replySem);
        return;
    }

    returnOldPic(p);

    if(p->pLayerCtrl != NULL)
    {
        LayerCtrlHoldLastPicture(p->pLayerCtrl, p->bHoldLastPicture);

        if (!p->bHoldLastPicture)
            LayerCtrlHideVideo(p->pLayerCtrl);
    }

    //* set status to stopped.
    p->eStatus = PLAYER_STATUS_STOPPED;
    *msg->result = 0;
    sem_post(msg->replySem);
}

static void handlePause(AwMessage *msg, void *arg)
{
    VideoRenderComp *p = arg;

    if(p->eStatus != PLAYER_STATUS_STARTED  &&
       !(p->eStatus == PLAYER_STATUS_PAUSED && p->bFirstPictureShowed == 0))
    {
        CDX_LOGW("not in started status, pause operation invalid.");
        *msg->result = -1;
        sem_post(msg->replySem);
        return;
    }

    p->eStatus = PLAYER_STATUS_PAUSED;

    *msg->result = 0;
    sem_post(msg->replySem);

    if(p->bFirstPictureShowed == 0)
        BaseCompContinue(&p->base);
}

static void handleQuit(AwMessage *msg, void *arg)
{
    VideoRenderComp *p = arg;

    returnOldPic(p);

    sem_post(msg->replySem);
    p->eStatus = PLAYER_STATUS_STOPPED;

    if (p->pLayerCtrl != NULL)
    {
        LayerRelease(p->pLayerCtrl);
        p->pLayerCtrl = NULL;
    }

    pthread_exit(NULL);
}

static void handleReset(AwMessage *msg, void *arg)
{
    VideoRenderComp *p = arg;

    returnOldPic(p);

    p->bEosFlag = 0;
    p->bFirstPictureShowed = 0;
    p->bFirstPtsNotified = 0;
    if(p->bBufferQueueToSF == 1)
    {
#if defined(CONF_PTS_TOSF)
        if(p->pLayerCtrl)
            LayerControl(p->pLayerCtrl, CDX_LAYER_CMD_RESTART_SCHEDULER, NULL);
#endif
    }
    *msg->result = 0;
    sem_post(msg->replySem);

    ContinueRender(p);
}

static void handleSetEos(AwMessage *msg, void *arg)
{
    VideoRenderComp *p = arg;

    p->bEosFlag = 1;
    sem_post(msg->replySem);

    ContinueRender(p);
}



static int requestPicture(VideoRenderComp *p)
{
    while(p->pPicture == NULL)
    {
        p->pPicture = WFD_VideoDecCompRequestPicture(p->pDecComp, 0, &p->bResolutionChange);
        logv("get picture, picture %p", p->pPicture);
        if(p->pPicture != NULL || (p->pPicture == NULL && p->bEosFlag))
        {
            p->pCurrentPicture = p->pPicture;
            break;
        }

        if(p->bResolutionChange)
        {
            LayerControl(p->pLayerCtrl, CDX_LAYER_CMD_SET_VIDEO_RESOLUTION_CHANGED, &p->bResolutionChange);
            //* reopen the video engine.
            WFD_VideoDecCompReopenVideoEngine(p->pDecComp);
            //* reopen the layer.
            if(p->pLayerCtrl != NULL)
            {
                LayerReset(p->pLayerCtrl);
                p->bNeedResetLayerParams = 1;
            }
            p->bResolutionChange          = 0;
            LayerControl(p->pLayerCtrl, CDX_LAYER_CMD_SET_VIDEO_RESOLUTION_CHANGED, &p->bResolutionChange);
            p->bHadSetLayerInfoFlag       = 0;
            p->bHadGetVideoFbmBufInfoFlag = 0;
            p->bHadSetBufferToDecoderFlag = 0;

            BaseCompContinue(&p->base);
            return -1;
        }

        if (AwMessageQueueWaitMessage(p->mq, 2) > 0)
            return -1;
    }

    if(p->pPicture == NULL && p->bEosFlag == 1)
    {
        p->callback(p->pUserData, PLAYER_VIDEO_RENDER_NOTIFY_EOS, NULL);
        return -1;
    }

    return 0;
}

static int notifyFirstPts(VideoRenderComp *p)
{
    /* this callback may block because the player need wait
     * audio first frame to sync.
     */
    int ret = p->callback(p->pUserData, PLAYER_VIDEO_RENDER_NOTIFY_FIRST_PICTURE,
            (void*)&p->pPicture->nPts);
    if(ret == TIMER_DROP_VIDEO_DATA)
    {
        //* video first frame pts small (too much) than the audio,
        //* discard this frame to catch up the audio.
        WFD_VideoDecCompReturnPicture(p->pDecComp, p->pPicture);
        p->pPicture = NULL;
        BaseCompContinue(&p->base);
        return -1;
    }
    else if(ret == TIMER_NEED_NOTIFY_AGAIN)
    {
        /* waiting process for first frame sync with audio is
         * broken by a new message to player, so the player tell
         * us to notify again later.
         */
        if (AwMessageQueueWaitMessage(p->mq, 10) <= 0)
            BaseCompContinue(&p->base);
        return -1;
    }

    p->bFirstPtsNotified = 1;

    if(ret == TIMER_SMOOTH_VIDEO_DATA)
    {
        p->bFirstPtsNotified = 0;
        p->bFirstPictureShowed = 0;
    }

    return 0;
}

static int showProgressivePicture(VideoRenderComp *p)
{
    //* the first picture is showed unsychronized.
    if(p->bFirstPictureShowed == 0)
    {
        VideoPicture* pReturnPicture = NULL;

        QueueBufferToShow(p, p->pPicture);

        if(p->nSetToDecoderBufferNum < p->nGpuBufferNum)
        {
            // set all of the nativeBuffer to decoder
            int ret;
            int i;
            VideoPicture mTmpVideoPicture;
            VideoPicture* pTmpVideoPicture = &mTmpVideoPicture;
            ret = LayerDequeueBuffer(p->pLayerCtrl, &pTmpVideoPicture, 1);
            if(ret == 0)
            {
                p->nSetToDecoderBufferNum ++;
                WFD_VideoDecCompSetVideoFbmBufAddress(p->pDecComp, pTmpVideoPicture, 0);
            }
            else
            {
                CDX_LOGW("LayerDequeueBuffer failed");
                BaseCompContinue(&p->base);
                return -1;
            }

            if(p->nSetToDecoderBufferNum == p->nGpuBufferNum)
            {
                for (i = 0; i < p->nCancelPictureNum; ++i)
                {
                    LayerQueueBuffer(p->pLayerCtrl, p->pCancelPicture[i], 0);
                }
            }
        }
        else
        {
            LayerDequeueBuffer(p->pLayerCtrl, &pReturnPicture, 0);
            WFD_VideoDecCompReturnPicture(p->pDecComp, pReturnPicture);
        }
        p->pPicture = NULL;

        return 0;
    }

    //* wait according to the presentation time stamp.
    if(p->bFirstPictureShowed == 0)
        return 0;

    ProcessVideoSync(p, p->pPicture);
    {

        VideoPicture* pReturnPicture = NULL;
        QueueBufferToShow(p, p->pPicture);
#ifdef DEBU_PTS_DIFF
        if(p->bSetStatics)
        {
        int k = 0;
        int64_t timeDiff = 0;
            for(k == 0; k < 5; k++)
            {
                if(p->nPtsList[k] == p->pPicture->nPts)
                {
                    timeDiff = GetCurrentTime() - p->nPtsGetTime[k];
                    if(timeDiff > 20000)
                        CDX_LOGD("pts %lld. --> timeDiff %lld ms.", p->pPicture->nPts, timeDiff/1000);
                }
            }
        }
#endif 
        if(p->nSetToDecoderBufferNum < p->nGpuBufferNum)
        {
            // set all of the nativeBuffer to decoder
            int ret;
            int i;
            VideoPicture mTmpVideoPicture;
            VideoPicture* pTmpVideoPicture = &mTmpVideoPicture;
            ret = LayerDequeueBuffer(p->pLayerCtrl, &pTmpVideoPicture, 1);
            if(ret == 0)
            {
                p->nSetToDecoderBufferNum ++;
                WFD_VideoDecCompSetVideoFbmBufAddress(p->pDecComp, pTmpVideoPicture, 0);
            }
            else
            {
                CDX_LOGW("LayerDequeueBuffer failed");
                BaseCompContinue(&p->base);
                return -1;
            }

            if(p->nSetToDecoderBufferNum == p->nGpuBufferNum)
            {
                for (i = 0; i < p->nCancelPictureNum; ++i)
                {
                    LayerQueueBuffer(p->pLayerCtrl, p->pCancelPicture[i], 0);
                }
            }
        }
        else
        {
            LayerDequeueBuffer(p->pLayerCtrl, &pReturnPicture, 0);
            WFD_VideoDecCompReturnPicture(p->pDecComp, pReturnPicture);
        }

        p->pPicture = NULL;
    }

    return 0;
}

static int checkFlags(VideoRenderComp *p)
{
    // when nativeWindow change, we should reset buffer to decoder
 //   CDX_LOGD("checkFlags begin");
	
    while(p->bResetBufToDecoderFlag == 1)
    {
        int ret = ResetBufToDecoder(p);
        logv("bResetBufToDecoderFlag=1, ret=%d, bHadGetVideoFbmBufInfoFlag=%d",
            ret, p->bHadGetVideoFbmBufInfoFlag);
        if(ret == VIDEO_RENDER_THREAD_CONTINUE)
        {
            if (AwMessageQueueWaitMessage(p->mq, 5) <= 0)
                BaseCompContinue(&p->base);
            return -1;
        }
        else if (AwMessageQueueWaitMessage(p->mq, 5) > 0)
        {
            return -1;
        }
    }
//    CDX_LOGD("bHadGetVideoFbmBufInfoFlag %d", p->bHadGetVideoFbmBufInfoFlag);

    //* get video fbm buf info
    while(p->bHadGetVideoFbmBufInfoFlag == 0)
    {
        if(p->bEosFlag == 1)
        {
            p->callback(p->pUserData, PLAYER_VIDEO_RENDER_NOTIFY_EOS, NULL);
			CDX_LOGD("checkFlags ");
            return -1;
        }

        if(RenderGetVideoFbmBufInfo(p) == 0)
            p->bHadGetVideoFbmBufInfoFlag = 1;
        else if (AwMessageQueueWaitMessage(p->mq, 5) > 0)
    	{
			CDX_LOGD("checkFlags ");
            return -1;
    	}
    }

 //   CDX_LOGD("bHadSetBufferToDecoderFlag %d",p->bHadSetBufferToDecoderFlag);
    //* set buffer to decoder
    while(p->bHadSetBufferToDecoderFlag == 0)
    {
        if(p->bHadSetLayerInfoFlag)
        {
            SetGpuBufferToDecoder(p);
            p->bHadSetBufferToDecoderFlag = 1;
        }
        else if (AwMessageQueueWaitMessage(p->mq, 5) > 0)
        {
			CDX_LOGD("checkFlags ");
            return -1;
        }
    }
//    CDX_LOGD("checkFlags finish");

    return 0;
}

#if defined(CONF_SUPPORT_G2D)
static int G2dProcessRotate(VideoRenderComp* p, VideoPicture* pPicture, enum ROTATEDEGREE degree, int bDegreeChanged) {
    int ret = -1;
    int out_pic_len;

    cdx_int64 t0 = CdxGetNowUs();

    if (p->nG2dFd == 0) {
        if((p->nG2dFd = open("/dev/g2d",O_RDWR)) == -1) {
            loge("open g2d device fail!\n");
            close(p->nG2dFd);
            return -1;
        } else {
            logd("pG2dFd = %d", p->nG2dFd);
        }
    }

    cdx_int64 t2 = CdxGetNowUs();
    ret = LayerDequeueBuffer(p->pLayerCtrl, &p->pG2dOutPicture, 0);
    cdx_int64 t3 = CdxGetNowUs();
    if(ret != 0) {
        loge("dequeue buffer failed when process g2d rotate");
        return ret;
    }

    memset(&p->mG2dInfo, 0x00, sizeof(g2d_lbc_rot));

    // resolve Huawei mobile phone screen edge distortion
    out_pic_len = p->pG2dOutPicture->nWidth*p->pG2dOutPicture->nHeight;
    memset(p->pG2dOutPicture->pData0, 0x10, out_pic_len);
    memset(p->pG2dOutPicture->pData0 + out_pic_len, 0x80, out_pic_len/2);

    int rect_w = pPicture->nRightOffset - pPicture->nLeftOffset;
    int rect_h = pPicture->nBottomOffset - pPicture->nTopOffset;

    p->mG2dInfo.blt.src_image_h.width = pPicture->nWidth;
    p->mG2dInfo.blt.src_image_h.height = pPicture->nHeight;
    p->mG2dInfo.blt.src_image_h.clip_rect.x = pPicture->nLeftOffset;
    p->mG2dInfo.blt.src_image_h.clip_rect.y = pPicture->nTopOffset;
    p->mG2dInfo.blt.src_image_h.clip_rect.w = rect_w;
    p->mG2dInfo.blt.src_image_h.clip_rect.h = rect_h;

    switch (degree) {
    case ROTATE_DEGREE_0:
        p->mG2dInfo.blt.flag_h = G2D_ROT_0;
        p->mG2dInfo.blt.dst_image_h.width = pPicture->nWidth;
        p->mG2dInfo.blt.dst_image_h.height = pPicture->nHeight;
        p->mG2dInfo.blt.dst_image_h.clip_rect.x = pPicture->nLeftOffset;
        p->mG2dInfo.blt.dst_image_h.clip_rect.y = pPicture->nTopOffset;
        p->mG2dInfo.blt.dst_image_h.clip_rect.w = rect_w;
        p->mG2dInfo.blt.dst_image_h.clip_rect.h = rect_h;

        p->pG2dOutPicture->nWidth = pPicture->nWidth;
        p->pG2dOutPicture->nHeight = pPicture->nHeight;
        p->pG2dOutPicture->nLeftOffset = pPicture->nLeftOffset;
        p->pG2dOutPicture->nRightOffset = pPicture->nRightOffset;
        p->pG2dOutPicture->nTopOffset = pPicture->nTopOffset;
        p->pG2dOutPicture->nBottomOffset = pPicture->nBottomOffset;
        break;
    case ROTATE_DEGREE_90:
        p->mG2dInfo.blt.flag_h = G2D_ROT_90;
        p->mG2dInfo.blt.dst_image_h.width = pPicture->nHeight;
        p->mG2dInfo.blt.dst_image_h.height = pPicture->nWidth;
        p->mG2dInfo.blt.dst_image_h.clip_rect.x = pPicture->nTopOffset;
        p->mG2dInfo.blt.dst_image_h.clip_rect.y = pPicture->nLeftOffset;
        p->mG2dInfo.blt.dst_image_h.clip_rect.w = rect_h;
        p->mG2dInfo.blt.dst_image_h.clip_rect.h = rect_w;

        p->pG2dOutPicture->nWidth = pPicture->nHeight;
        p->pG2dOutPicture->nHeight = pPicture->nWidth;
        p->pG2dOutPicture->nLeftOffset = pPicture->nTopOffset;
        p->pG2dOutPicture->nRightOffset = pPicture->nBottomOffset;
        p->pG2dOutPicture->nTopOffset = pPicture->nLeftOffset;
        p->pG2dOutPicture->nBottomOffset = pPicture->nRightOffset;
        break;
    case ROTATE_DEGREE_180:
        p->mG2dInfo.blt.flag_h = G2D_ROT_180;
        p->mG2dInfo.blt.dst_image_h.width = pPicture->nWidth;
        p->mG2dInfo.blt.dst_image_h.height = pPicture->nHeight;
        p->mG2dInfo.blt.dst_image_h.clip_rect.x = pPicture->nLeftOffset;
        p->mG2dInfo.blt.dst_image_h.clip_rect.y = pPicture->nTopOffset;
        p->mG2dInfo.blt.dst_image_h.clip_rect.w = rect_w;
        p->mG2dInfo.blt.dst_image_h.clip_rect.h = rect_h;

        p->pG2dOutPicture->nWidth = pPicture->nWidth;
        p->pG2dOutPicture->nHeight = pPicture->nHeight;
        p->pG2dOutPicture->nLeftOffset = pPicture->nLeftOffset;
        p->pG2dOutPicture->nRightOffset = pPicture->nRightOffset;
        p->pG2dOutPicture->nTopOffset = pPicture->nTopOffset;
        p->pG2dOutPicture->nBottomOffset = pPicture->nBottomOffset;
        break;
    case ROTATE_DEGREE_270:
        p->mG2dInfo.blt.flag_h = G2D_ROT_270;
        p->mG2dInfo.blt.dst_image_h.width = pPicture->nHeight;
        p->mG2dInfo.blt.dst_image_h.height = pPicture->nWidth;
        p->mG2dInfo.blt.dst_image_h.clip_rect.x = pPicture->nTopOffset;
        p->mG2dInfo.blt.dst_image_h.clip_rect.y = pPicture->nLeftOffset;
        p->mG2dInfo.blt.dst_image_h.clip_rect.w = rect_h;
        p->mG2dInfo.blt.dst_image_h.clip_rect.h = rect_w;

        p->pG2dOutPicture->nWidth = pPicture->nHeight;
        p->pG2dOutPicture->nHeight = pPicture->nWidth;
        p->pG2dOutPicture->nLeftOffset = pPicture->nTopOffset;
        p->pG2dOutPicture->nRightOffset = pPicture->nBottomOffset;
        p->pG2dOutPicture->nTopOffset = pPicture->nLeftOffset;
        p->pG2dOutPicture->nBottomOffset = pPicture->nRightOffset;
        break;
    default:
        p->mG2dInfo.blt.flag_h = G2D_ROT_0;
        p->mG2dInfo.blt.dst_image_h.width = pPicture->nWidth;
        p->mG2dInfo.blt.dst_image_h.height = pPicture->nHeight;
        p->mG2dInfo.blt.dst_image_h.clip_rect.x = pPicture->nLeftOffset;
        p->mG2dInfo.blt.dst_image_h.clip_rect.y = pPicture->nTopOffset;
        p->mG2dInfo.blt.dst_image_h.clip_rect.w = rect_w;
        p->mG2dInfo.blt.dst_image_h.clip_rect.h = rect_h;

        p->pG2dOutPicture->nWidth = pPicture->nWidth;
        p->pG2dOutPicture->nHeight = pPicture->nHeight;
        p->pG2dOutPicture->nLeftOffset = pPicture->nLeftOffset;
        p->pG2dOutPicture->nRightOffset = pPicture->nRightOffset;
        p->pG2dOutPicture->nTopOffset = pPicture->nTopOffset;
        p->pG2dOutPicture->nBottomOffset = pPicture->nBottomOffset;
        break;
    }

    switch (p->mFbmBufInfo.ePixelFormat) {
        case PIXEL_FORMAT_YUV_PLANER_420:
            p->mG2dInfo.blt.dst_image_h.format = G2D_FORMAT_YUV420_PLANAR;
            p->mG2dInfo.blt.src_image_h.format = G2D_FORMAT_YUV420_PLANAR;
            break;

        case PIXEL_FORMAT_YV12:
            p->mG2dInfo.blt.dst_image_h.format = G2D_FORMAT_YUV420_PLANAR;
            p->mG2dInfo.blt.src_image_h.format = G2D_FORMAT_YUV420_PLANAR;
            break;

        case PIXEL_FORMAT_NV12:
            p->mG2dInfo.blt.dst_image_h.format = G2D_FORMAT_YUV420UVC_U1V1U0V0;
            p->mG2dInfo.blt.src_image_h.format = G2D_FORMAT_YUV420UVC_U1V1U0V0;
            break;

        case PIXEL_FORMAT_NV21:
            p->mG2dInfo.blt.dst_image_h.format = G2D_FORMAT_YUV420UVC_V1U1V0U0;
            p->mG2dInfo.blt.src_image_h.format = G2D_FORMAT_YUV420UVC_V1U1V0U0;
            break;

        default:
        {
            loge("unsupported pixel format.");
            return -1;
        }
    }

    p->mG2dInfo.blt.src_image_h.fd = pPicture->nBufFd;
    p->mG2dInfo.blt.dst_image_h.fd = p->pG2dOutPicture->nBufFd;

    p->mG2dInfo.blt.src_image_h.align[0] = 0;
    p->mG2dInfo.blt.src_image_h.align[1] = 0;
    p->mG2dInfo.blt.src_image_h.align[2] = 0;
    p->mG2dInfo.blt.dst_image_h.align[0] = 0;
    p->mG2dInfo.blt.dst_image_h.align[1] = 0;
    p->mG2dInfo.blt.dst_image_h.align[2] = 0;

    cdx_int64 t4 = CdxGetNowUs();
    if(ioctl(p->nG2dFd , G2D_CMD_BITBLT_H ,(unsigned long)(&p->mG2dInfo.blt)) < 0)
    {
        loge("G2D_CMD_BITBLT_H failure!");
        ret = -1;
        if(p->pG2dOutPicture != NULL) {
            LayerQueueBuffer(p->pLayerCtrl, p->pG2dOutPicture, 0);
            p->pG2dOutPicture = NULL;
        }
        return ret;
    }
    cdx_int64 t5 = CdxGetNowUs();

    p->pG2dOutPicture->nPts = pPicture->nPts;
    if (bDegreeChanged) {  // set crop info to layercontrol when set rotation degree
        p->bSetDegree = 0;
        int size[4];
        int width = p->pG2dOutPicture->nRightOffset - p->pG2dOutPicture->nLeftOffset;
        int height = p->pG2dOutPicture->nBottomOffset - p->pG2dOutPicture->nTopOffset;

        size[0] = p->pG2dOutPicture->nLeftOffset;
        size[1] = p->pG2dOutPicture->nTopOffset;
        size[2] = width;
        size[3] = height;

        if (p->pLayerCtrl != NULL) {
            LayerSetDisplayRegion(p->pLayerCtrl, size[0], size[1], size[2], size[3]);
            LayerSetDisplayBufferSize(p->pLayerCtrl, p->pG2dOutPicture->nWidth, p->pG2dOutPicture->nHeight);
            switch (p->nDegree) {
                case ROTATE_DEGREE_0:
                    ret = LayerSetRotateDegree(p->pLayerCtrl, 0);
                    break;
                case ROTATE_DEGREE_90:
                    ret = LayerSetRotateDegree(p->pLayerCtrl, 90);
                    break;
                case ROTATE_DEGREE_180:
                    ret = LayerSetRotateDegree(p->pLayerCtrl, 180);
                    break;
                case ROTATE_DEGREE_270:
                    ret = LayerSetRotateDegree(p->pLayerCtrl, 270);
                    break;
                default:
                    ret = -1;
            }
        }
    }

    cdx_int64 t1 = CdxGetNowUs();
    p->G2dPrintCount++;
    if (p->G2dPrintCount > 30*5) {
        logw("G2dProcessRotate time: %lld ms", (t1-t0)/1000);
        logw("LayerDequeueBuffer time: %lld ms", (t3-t2)/1000);
        logw("G2d ioctl time: %lld ms", (t5-t4)/1000);
        p->G2dPrintCount = 0;
    }

    return 0;
}
#endif

#if defined(CONF_SUPPORT_G2D)
static int showRotatePicture_G2d(VideoRenderComp *p, enum ROTATEDEGREE degree)
{
    int ret = 0;

    ret = G2dProcessRotate(p, p->pPicture, degree, p->bSetDegree);
    if(ret == -1)
    {
        loge("g2d process picture rorate fail!");
        returnOldPic(p);
        BaseCompContinue(&p->base);
        return -1;
    }

    //* display pG2dOutPicture
    if(p->bFirstPictureShowed != 0)
    {
        ret = ProcessVideoSync(p, p->pG2dOutPicture);
        if(ret == VIDEO_RENDER_MESSAGE_COME)
        {
            returnOldPic(p);
            return -1;
        }
        else if(ret == VIDEO_RENDER_DROP_THE_PICTURE)
        {
            returnOldPic(p);
            BaseCompContinue(&p->base);
            return -1;
        }
    }

    QueueBufferToShow(p, p->pG2dOutPicture);

    WFD_VideoDecCompReturnPicture(p->pDecComp, p->pPicture);
    p->pPicture = NULL;

    return 0;
}
#endif

static void doRender(AwMessage *msg, void *arg)
{
    VideoRenderComp *p = arg;
    (void)msg;

    if(p->eStatus != PLAYER_STATUS_STARTED &&
      !(p->eStatus == PLAYER_STATUS_PAUSED && p->bFirstPictureShowed == 0))
    {
        CDX_LOGW("not in started status, render message ignored.");
        return;
    }

    if (checkFlags(p) != 0)
	{
        CDX_LOGE("checkFlags error.");
        return;
	}
	
    if (p->pPicture == NULL)
    {
        if (requestPicture(p) != 0)
            return;
        if(p->bFirstPictureShowed == 0 || p->bNeedResetLayerParams == 1)
        {
            NotifyVideoSizeAndSetDisplayRegion(p);
            p->bNeedResetLayerParams = 0;
        }
    }


    /************************************************************
     * notify the first sync frame to set timer. the first sync
     * frame is the second picture, the first picture need to be
     * showed as soon as we can.(unsynchroized)
     ***********************************************************/
    if(p->bFirstPictureShowed && p->bFirstPtsNotified == 0)
    {
        if (notifyFirstPts(p) != 0)
            return;
    }

    //******************************************************
    //* sync and show the picture
    //******************************************************
#if defined(CONF_SUPPORT_G2D)
    if (p->nDegree == ROTATE_DEGREE_UNSET) {
        if (showProgressivePicture(p) != 0)
            return;
    } else {
        if (showRotatePicture_G2d(p, p->nDegree) != 0) {
            CDX_LOGW("showRotatePicture_G2d err");
        }
    }
#else
    if (showProgressivePicture(p) != 0)
        return;
#endif

    if(p->bFirstPictureShowed == 0)
        p->bFirstPictureShowed = 1;

    if(p->eStatus == PLAYER_STATUS_STARTED)
    {
        BaseCompContinue(&p->base);
    }
    else
    {
        logi("first picture showed at paused status.");
    }
}

static int IsVideoWithTwoStream(VideoDecComp* pDecComp)
{
    VideoStreamInfo videoStreamInfo;
    if(WFD_VideoDecCompGetVideoStreamInfo(pDecComp, &videoStreamInfo) == 0)
        return videoStreamInfo.bIs3DStream;
    return 0;
}

static void NotifyVideoSizeAndSetDisplayRegion(VideoRenderComp* p)
{
    int size[4];

    if((p->pPicture->nRightOffset - p->pPicture->nLeftOffset) > 0 &&
       (p->pPicture->nBottomOffset - p->pPicture->nTopOffset) > 0)
    {
        int width = p->pPicture->nRightOffset - p->pPicture->nLeftOffset;
        int height = p->pPicture->nBottomOffset - p->pPicture->nTopOffset;
#if defined(CONF_SUPPORT_G2D)
        switch (p->nDegree) {
            case ROTATE_DEGREE_90:
            case ROTATE_DEGREE_270:
                size[0] = p->pPicture->nTopOffset;
                size[1] = p->pPicture->nLeftOffset;
                size[2] = height;
                size[3] = width;
                break;
            case ROTATE_DEGREE_0:
            case ROTATE_DEGREE_180:
            default:
                size[0] = p->pPicture->nLeftOffset;
                size[1] = p->pPicture->nTopOffset;
                size[2] = width;
                size[3] = height;
                break;
        }
#else
        size[0] = p->pPicture->nLeftOffset;
        size[1] = p->pPicture->nTopOffset;
        size[2] = width;
        size[3] = height;
#endif
        p->callback(p->pUserData, PLAYER_VIDEO_RENDER_NOTIFY_VIDEO_CROP, (void*)size);

        if(p->pLayerCtrl != NULL)
        {
            LayerSetDisplayRegion(p->pLayerCtrl, size[0], size[1], size[2], size[3]);
        }

        size[0] = width;
        size[1] = height;
        size[2] = 0;
        size[3] = 0;
        p->callback(p->pUserData, PLAYER_VIDEO_RENDER_NOTIFY_VIDEO_SIZE, (void*)size);
    }
    else
    {
        CDX_LOGW("the offsets of picture are not right, we set bufferWidht and \
              bufferHeight as video size, this maybe wrong, offset: %d, %d, %d, %d",
              p->pPicture->nLeftOffset,p->pPicture->nRightOffset,
              p->pPicture->nTopOffset,p->pPicture->nBottomOffset);

        if(p->pLayerCtrl != NULL)
        {
            LayerSetDisplayRegion(p->pLayerCtrl,
                                  0,
                                  0,
                                  p->pPicture->nWidth,
                                  p->pPicture->nHeight);
        }

        size[0] = p->pPicture->nWidth;
        size[1] = p->pPicture->nHeight;
        size[2] = 0;
        size[3] = 0;
        p->callback(p->pUserData, PLAYER_VIDEO_RENDER_NOTIFY_VIDEO_SIZE, (void*)size);


    }

    return ;
}

static inline int ProcessVideoSync(VideoRenderComp* p, VideoPicture* pPicture)
{
	int nWaitTime;
    nWaitTime = p->callback(p->pUserData, PLAYER_VIDEO_RENDER_NOTIFY_PICTURE_PTS,
            (void*)&pPicture->nPts);

    //VideoDecComp* pVideoDecComp = p->pDecComp;
    //int num = WFD_ValidPictureNum(pVideoDecComp, 0);
    //if(num > 0){
    //    //CDX_LOGD("pictures already decoded, but not render: validPicture number is %d, normally validPicture should be 0", num);
    //}

	//if(nWaitTime > 0 && num == 0)
	//{
    //    if(nWaitTime > 50){//when we wait time is larger than 50ms , print it
    //        //CDX_LOGD("there is no picture to render , so we wait to play smoothly: waitTime is %dms", nWaitTime);
    //    }
	//	if (AwMessageQueueWaitMessage(p->mq, nWaitTime) > 0)
	//		return VIDEO_RENDER_MESSAGE_COME;
	//}
	//else if (nWaitTime < -200)
	//{
	//	//* if it is expired, we should drop it
	//	CDX_LOGE("need to drop this frame");
	//	return VIDEO_RENDER_DROP_THE_PICTURE;
	//}

    return 0;
}

static int64_t Utils_GetTimeUS() 
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return (int64_t)tv.tv_sec * 1000000ll + tv.tv_usec;
}

static int QueueBufferToShow(VideoRenderComp* p, VideoPicture* pPicture)
{
    if(p->pLayerCtrl != NULL)
    {
        LayerQueueBuffer(p->pLayerCtrl, pPicture, 1);
    }

    int ptsSecs = (int)(pPicture->nPts/1000000);
    if (p->ptsSecs <= ptsSecs-10)//print every 10s
    {
        p->ptsSecs = ptsSecs;
        CDX_LOGD("after decoder, video pts %.3f ms(@ %.3f ms), avTimer is %.3f ms", pPicture->nPts/1000.0, Utils_GetTimeUS()/1000.0,p->pAvTimer->GetTime(p->pAvTimer)/1000.0);
    }

    if(p->pLayerCtrl != NULL && p->bHideVideo == 0)
        LayerCtrlShowVideo(p->pLayerCtrl);

    return 0;
}

static int RenderGetVideoFbmBufInfo(VideoRenderComp* p)
{
    enum EPIXELFORMAT eDisplayPixelFormat = PIXEL_FORMAT_DEFAULT;
    FbmBufInfo* pFbmBufInfo =  WFD_VideoDecCompGetVideoFbmBufInfo(p->pDecComp);


    if(pFbmBufInfo == NULL)
	{
		//CDX_LOGD("pFbmBufInfo = %p",pFbmBufInfo);
        return -1;
    }
    p->mFbmBufInfo = *pFbmBufInfo;
     //* We check whether it is a 3D stream here,
    //* because Layer must know whether it 3D stream at the beginning;
    p->bVideoWithTwoStream = IsVideoWithTwoStream(p->pDecComp);

    CDX_LOGD("video buffer info: nWidth[%d],nHeight[%d],nBufferCount[%d],ePixelFormat[%d]",
          pFbmBufInfo->nBufWidth,pFbmBufInfo->nBufHeight,
          pFbmBufInfo->nBufNum,pFbmBufInfo->ePixelFormat);
    CDX_LOGD("video buffer info: nAlignValue[%d],bProgressiveFlag[%d],bIsSoftDecoderFlag[%d]",
          pFbmBufInfo->nAlignValue,pFbmBufInfo->bProgressiveFlag,
          pFbmBufInfo->bIsSoftDecoderFlag);

    if (p->pLayerCtrl == NULL)
    {
        CDX_LOGW("lc is NULL");
        return 0;
    }


    eDisplayPixelFormat = (enum EPIXELFORMAT)pFbmBufInfo->ePixelFormat;

//    LayerSetHdrInfo(p->pLayerCtrl, pFbmBufInfo);
    LayerSetDisplayPixelFormat(p->pLayerCtrl, eDisplayPixelFormat);
    LayerSetDisplayBufferSize(p->pLayerCtrl, pFbmBufInfo->nBufWidth,
            pFbmBufInfo->nBufHeight);
    LayerSetVideoWithTwoStreamFlag(p->pLayerCtrl, p->bVideoWithTwoStream);
    LayerSetIsSoftDecoderFlag(p->pLayerCtrl, pFbmBufInfo->bIsSoftDecoderFlag);
    p->nGpuBufferNum = LayerSetDisplayBufferCount(p->pLayerCtrl,
            pFbmBufInfo->nBufNum);

    p->bHadSetLayerInfoFlag  = 1;

    return 0;
}

static int SetGpuBufferToDecoder(VideoRenderComp*p)
{
    VideoPicture mTmpVideoPicture;
    int i;
    VideoPicture* pTmpVideoPicture = &mTmpVideoPicture;
    int nLayerBufferNum = LayerGetBufferNumHoldByGpu(p->pLayerCtrl);
    memset(pTmpVideoPicture, 0, sizeof(VideoPicture));
    p->nSetToDecoderBufferNum = 0;
    p->nCancelPictureNum = 0;
	
    CDX_LOGD("SetGpuBufferToDecoder:nNumHoldByLayer = %d,p->nGpuBufferNum = %d",nLayerBufferNum,p->nGpuBufferNum);
    for(i = 0; i< p->nGpuBufferNum; i++)
    {
        int ret = LayerDequeueBuffer(p->pLayerCtrl, &pTmpVideoPicture, 1);
        if(ret == 0)
        {
            if (i >= p->nGpuBufferNum - nLayerBufferNum)
            {
                p->pCancelPicture[p->nCancelPictureNum] =
                    WFD_VideoDecCompSetVideoFbmBufAddress(p->pDecComp, pTmpVideoPicture, 1);
                p->nCancelPictureNum ++;
            }
            else
            {
                WFD_VideoDecCompSetVideoFbmBufAddress(p->pDecComp, pTmpVideoPicture, 0);
            }
            p->nSetToDecoderBufferNum ++;
        }
        else
        {
            // we cannot dequeue the last buffer form native window right now
            // in the case of  resolution change. so dequeue it after queue the
            // first picture to native window
            CDX_LOGW("*** dequeue buffer failed when set-buffer-to-decoder");
            //abort();
        }
    }

    if(p->nGpuBufferNum == p->nSetToDecoderBufferNum)
    {
        for (i = 0; i < nLayerBufferNum; ++i)
        {
            LayerQueueBuffer(p->pLayerCtrl, p->pCancelPicture[i], 0);
        }
    }

    return 0;
}

// release the buffer from the old layer;
// deuqeuebuffer from new layer;
// set the buffer to decoder
static int ResetBufToDecoder(VideoRenderComp*p)
{
    int ret = 0;
    VideoPicture* pReleasePicture = NULL;

    if(p->bHadRequestReleasePicFlag == 0)
    {
        // 1. release the buffer from the old layer;
        pReleasePicture = WFD_VideoDecCompRequestReleasePicture(p->pDecComp);

        logv("*** pReleasePicture(%p),nNeedReleaseBufferNum(%d)",
             pReleasePicture,p->nNeedReleaseBufferNum);

        if(pReleasePicture != NULL)
        {
            LayerReleaseBuffer(p->pLayerCtrl, pReleasePicture); //old
        }
        else
        {
            //* we drop the picture here, or the decoder will block and
            //* can not return all the ReleasePicture
            VideoPicture* pRequestPic = NULL;
            int nResolutionChange = 0;
            pRequestPic = WFD_VideoDecCompRequestPicture(p->pDecComp, 0, &nResolutionChange);
            logv("pRequestPic=%p", pRequestPic);
            if(pRequestPic != NULL)
                WFD_VideoDecCompReturnPicture(p->pDecComp, pRequestPic);
        }
    }

    if(p->bHadRequestReleasePicFlag == 1 || pReleasePicture != NULL)
    {
        VideoPicture mTmpReturnPicture;
        memset(&mTmpReturnPicture, 0, sizeof(VideoPicture));
        VideoPicture* pTmpReturnPicture = &mTmpReturnPicture;

        // 2. dequeuebuffer from new layer;
        ret = LayerDequeueBuffer(p->pLayerCtrl, &pTmpReturnPicture, 1);
        if(ret == 0)
        {
            if(p->nNeedReleaseBufferNum <= LayerGetBufferNumHoldByGpu(p->pLayerCtrl))
            {
                // 4. set the buffer to decoder and keep Num buffers for GPU.
                pTmpReturnPicture = WFD_VideoDecCompReturnRelasePicture(p->pDecComp,
                        pTmpReturnPicture, 1);
                LayerQueueBuffer(p->pLayerCtrl, pTmpReturnPicture, 0);
            }
            else
            {
                // 3. set the buffer to decoder
                WFD_VideoDecCompReturnRelasePicture(p->pDecComp, pTmpReturnPicture, 0);
            }

            if(p->bHadRequestReleasePicFlag == 1)
            {
                p->bHadRequestReleasePicFlag = 0;
            }

            p->nNeedReleaseBufferNum--;

            if(p->nNeedReleaseBufferNum <= 0)
            {
                p->bResetBufToDecoderFlag = 0;
            }
        }
        else
        {
            p->bHadRequestReleasePicFlag = 1;
            CDX_LOGD("LayerDequeueBuffer fail.");
            return VIDEO_RENDER_THREAD_CONTINUE;
        }
    }
    return 0;
}

int WFD_VideoRenderSetCastMode(VideoRenderComp *p, enum CASTMODE eCastMode){
    return 0;
    /*
    if(p->pLayerCtrl != NULL){
        if(eCastMode == CAST_MIRACAST_MODE)
            return LayerSetMiracastMode(p->pLayerCtrl, 1);
        else
            return LayerSetMiracastMode(p->pLayerCtrl, 0);
    }else{
        return -1;
    }*/
}

static void handleRotate(AwMessage* msg, void* arg) {
    VideoRenderComp* p = arg;

#if defined(CONF_SUPPORT_G2D)
    if (p->pCurrentPicture != NULL) {
        p->pCurrentPicture->nPts += 22*1000;//update pts to make layercontrol to see this reused picture as a new picture
        int ret = G2dProcessRotate(p, p->pCurrentPicture, p->nDegree, p->bSetDegree);
        if (ret == -1) {
            loge("g2d process picture rorate fail!");
            returnOldPic(p);
            BaseCompContinue(&p->base);
            return -1;
        }
        QueueBufferToShow(p, p->pG2dOutPicture);
    }
#endif
    if (p->pLayerCtrl != NULL && p->pCurrentPicture != NULL) {
        //LayerRotateScreen(p->pLayerCtrl, p->pCurrentPicture);
    }
    if (p->eStatus == PLAYER_STATUS_STARTED) {
        BaseCompContinue(&p->base);
    }
}

int WFD_VideoRenderSetRotateDegree(VideoRenderComp *p, enum ROTATEDEGREE eRotateDegree){

    int ret = -1;
    if (p->pPicture != NULL && p->pPicture->nWidth >= 3840)
        return ret;

    if (p->nDegree != eRotateDegree) {
        p->nDegree = eRotateDegree;
        p->bSetDegree = 1;
    } else {
        return -1;
    }

    if (ret != -1 && p->bFirstPictureShowed != 0) {
        loge("user rotate screen when player is paused");

        AwMessage msg = {
            .messageId = MESSAGE_ID_ROTATE,
            .execute = handleRotate,
        };

        if (AwMessageQueuePostMessage(p->mq, &msg) != 0) {
            loge("fatal error, video render component post message fail.");
            abort();
        }
    }
    return ret;
}

int WFD_VideoRenderSetFullScreen(VideoRenderComp *p, int fullScreen){
    if(p->pLayerCtrl != NULL){
        return LayerSetDisplayFullScreen(p->pLayerCtrl, fullScreen);
    }else{
        return -1;
    }
}
