/*
 * Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
 * All rights reserved.
 *
 * File : videoDecComponent.cpp
 * Description : video decoder component
 * History :
 *
 */

//#define CONFIG_LOG_LEVEL    OPTION_LOG_LEVEL_DETAIL
//#define LOG_TAG "videoDecComponent"
#include "cdx_log.h"

#include <pthread.h>
#include <semaphore.h>
#include <malloc.h>
#include <memory.h>
#include <time.h>
#include <sys/prctl.h>
//#include <cutils/properties.h>

#include "videoDecComponent.h"
#include "videoRenderComponent.h"
#include "AwMessageQueue.h"
#include "memoryAdapter.h"
#include "baseComponent.h"

//#include <iniparserapi.h>

enum FIRST_FRAME_STATUS {
    FIRST_FRAME_NOT_READY,
    FIRST_3D_FRAME_READY,
    SECOND_3D_FRAME_READY,
    FIRST_FRAME_READY = SECOND_3D_FRAME_READY,
};

struct VideoDecComp
{
    //* created at initialize time.
    AwMessageQueue     *mq;
    BaseCompCtx         base;

    sem_t               streamDataSem;
    sem_t               frameBufferSem;

    pthread_t           sDecodeThread;

    VideoDecoder*       pDecoder;
    enum EPLAYERSTATUS  eStatus;

    //* objects set by user.
    AvTimer*            pAvTimer;
    PlayerCallback      callback;
    void*               pUserData;
    int                 bEosFlag;

    int                 bConfigDecodeKeyFrameOnly;
    int                 bConfigDropDelayFrames;

    int                 bResolutionChange;
    int                 bCrashFlag;

    VConfig             vconfig;
    VideoStreamInfo     videoStreamInfo;
    int                 nSelectStreamIndex;

    //*for new display
    VideoRenderComp*    pVideoRenderComp;
    VideoRenderCallback videoRenderCallback;
    void*               pVideoRenderUserData;
    FbmBufInfo          mFbmBufInfo;
    int                 bFbmBufInfoValidFlag;

    char*               pSecureBuf;// for wvm video

    int                 nGpuAlignStride;

    pthread_mutex_t     videoRenderCallbackMutex;
    struct ScMemOpsS*   memOps;

    enum FIRST_FRAME_STATUS  firstFrameStatus;
};

static void handleStart(AwMessage *msg, void *arg);
static void handleStop(AwMessage *msg, void *arg);
static void handlePause(AwMessage *msg, void *arg);
static void handleReset(AwMessage *msg, void *arg);
static void handleSetEos(AwMessage *msg, void *arg);
static void handleQuit(AwMessage *msg, void *arg);
static void doDecode(AwMessage *msg, void *arg);

static void* VideoDecodeThread(void* arg);

VideoDecComp* WFD_VideoDecCompCreate(void)
{
    VideoDecComp *p;
    int err;

    p = (VideoDecComp*)calloc(1, sizeof(*p));
    if(p == NULL)
    {
        loge("memory alloc fail.");
        return NULL;
    }

    p->mq = AwMessageQueueCreate(4, "VideoDecodeMq");
    if(p->mq == NULL)
    {
        loge("video decoder component create message queue fail.");
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
        .decode = doDecode,
    };

    if (BaseCompInit(&p->base, "video decoder", p->mq, &handler))
    {
        AwMessageQueueDestroy(p->mq);
        free(p);
        return NULL;
    }

    p->pDecoder = CreateVideoDecoder();
    if(p->pDecoder == NULL)
    {
        loge("video decoder component create decoder fail.");
        BaseCompDestroy(&p->base);
        AwMessageQueueDestroy(p->mq);
        free(p);
        return NULL;
    }

    pthread_mutex_init(&p->videoRenderCallbackMutex, NULL);
    sem_init(&p->streamDataSem, 0, 0);
    sem_init(&p->frameBufferSem, 0, 0);

    p->eStatus = PLAYER_STATUS_STOPPED;

    err = pthread_create(&p->sDecodeThread, NULL, VideoDecodeThread, p);
    if(err != 0)
    {
        loge("video decode component create thread fail.");
        sem_destroy(&p->streamDataSem);
        sem_destroy(&p->frameBufferSem);
        pthread_mutex_destroy(&p->videoRenderCallbackMutex);

        DestroyVideoDecoder(p->pDecoder);
        BaseCompDestroy(&p->base);
        AwMessageQueueDestroy(p->mq);
        free(p);
        return NULL;
    }

    p->nGpuAlignStride = 32; /* force 32bit align */

    return p;
}

static void WakeUpThread(void *arg)
{
    VideoDecComp *p = arg;
    sem_post(&p->streamDataSem);
    sem_post(&p->frameBufferSem);
}

int WFD_VideoDecCompDestroy(VideoDecComp* p)
{
    BaseCompQuit(&p->base, WakeUpThread, p);
    pthread_join(p->sDecodeThread, NULL);
    BaseCompDestroy(&p->base);

    if(p->memOps != NULL)
    {
        if(p->pSecureBuf != NULL)
        {
#if 1
            loge("currently do not support pfree without decoder, please use vdecoder's api to pfree \n");
#else
            CdcMemPfree(p->memOps, p->pSecureBuf);
#endif
        }
        CdcMemClose(p->memOps);
    }

    free(p->videoStreamInfo.pCodecSpecificData);

    sem_destroy(&p->streamDataSem);
    sem_destroy(&p->frameBufferSem);

    DestroyVideoDecoder(p->pDecoder);
    AwMessageQueueDestroy(p->mq);

    pthread_mutex_destroy(&p->videoRenderCallbackMutex);

    free(p);

    return 0;
}

int WFD_VideoDecCompStart(VideoDecComp* p)
{
    return BaseCompStart(&p->base, WakeUpThread, p);
}

int WFD_VideoDecCompStop(VideoDecComp* p)
{
    return BaseCompStop(&p->base, WakeUpThread, p);
}

int WFD_VideoDecCompPause(VideoDecComp* p)
{
    return BaseCompPause(&p->base, WakeUpThread, p);
}

int WFD_VideoDecCompReset(VideoDecComp* p)
{
    return BaseCompReset(&p->base, 0, WakeUpThread, p);
}

int WFD_VideoDecCompSetCallback(VideoDecComp* p, PlayerCallback callback, void* pUserData)
{
    p->callback  = callback;
    p->pUserData = pUserData;
    return 0;
}

int WFD_VideoDecCompSetDecodeKeyFrameOnly(VideoDecComp* p, int bDecodeKeyFrameOnly)
{
    p->bConfigDecodeKeyFrameOnly  = bDecodeKeyFrameOnly;
    return 0;
}

int WFD_VideoDecCompSetVideoStreamInfo(VideoDecComp* p, VideoStreamInfo* pStreamInfo, VConfig* pVconfig)
{
    int ret;
    pVconfig->nAlignStride = p->nGpuAlignStride;
    pVconfig->bGpuBufValid = 1;

    logd("++++++++ pVconfig->bGpuBufValid = %d,nGpuAlignStride = %d ",
         pVconfig->bGpuBufValid,p->nGpuAlignStride);
    //* save the config and video stream info
    if(p->videoStreamInfo.pCodecSpecificData)
    {
        free(p->videoStreamInfo.pCodecSpecificData);
        p->videoStreamInfo.pCodecSpecificData = NULL;
        p->videoStreamInfo.nCodecSpecificDataLen = 0;
    }

    p->vconfig = *pVconfig;
    p->videoStreamInfo = *pStreamInfo;

    if(pStreamInfo->nCodecSpecificDataLen > 0)
    {
        p->videoStreamInfo.pCodecSpecificData = (char*)malloc(pStreamInfo->nCodecSpecificDataLen);
        if(p->videoStreamInfo.pCodecSpecificData == NULL)
        {
            loge("malloc video codec specific data fail!");
            return -1;
        }
        memcpy(p->videoStreamInfo.pCodecSpecificData,
               pStreamInfo->pCodecSpecificData,
               pStreamInfo->nCodecSpecificDataLen);
        p->videoStreamInfo.nCodecSpecificDataLen = pStreamInfo->nCodecSpecificDataLen;
    }

    logv("pStreamInfo->bSecureStreamFlagLevel1 = %d",pStreamInfo->bSecureStreamFlagLevel1);
    if(pStreamInfo->bSecureStreamFlagLevel1 == 1)
    {
        pVconfig->memops = SecureMemAdapterGetOpsS();
    }
    else
    {
        pVconfig->memops = MemAdapterGetOpsS();
    }

    if(pStreamInfo->eCodecFormat == VIDEO_CODEC_FORMAT_H265)
    {
        logd("enable afbc mode for H265 big size(judge by decoder).");
        pVconfig->eCtlAfbcMode = ENABLE_AFBC_JUST_BIG_SIZE;
    }
    else
    {
        logd("disable afbc mode");
		pVconfig->eCtlAfbcMode = 0;
    }

    ret = InitializeVideoDecoder(p->pDecoder, pStreamInfo, pVconfig);

    //* set secure buffer to wvm parser
    if(pStreamInfo->bSecureStreamFlagLevel1 == 1)
    {
        char* pBuf;
        int nBufferCount = 1;
        int nBufferSize  = 256*1024;
        uintptr_t param[2];

        //* set buffer count
        int nMessageId = PLAYER_VIDEO_DECODER_NOTIFY_SET_SECURE_BUFFER_COUNT;
        p->callback(p->pUserData,nMessageId,(void*)&nBufferCount);

        if(p->memOps != NULL)
        {
            if(p->pSecureBuf != NULL)
            {
#if 1
                loge("currently do not support pfree without decoder, please use vdecoder's api to pfree \n");
#else
                CdcMemPfree(p->memOps, p->pSecureBuf);
#endif
            }

            CdcMemClose(p->memOps);
        }

        p->memOps = SecureMemAdapterGetOpsS();
        CdcMemOpen(p->memOps);

#if 1
        loge("currently do not support palloc without decoder, please use vdecoder's api to palloc \n");
        goto _exit;
#else
        pBuf = (char*)CdcMemPalloc(p->memOps, nBufferSize);
#endif
        if(pBuf == NULL)
        {
            loge("video request secure buffer failed!");
            ret = -1;
            goto _exit;
        }

        //* set buffer address
        param[0]   = nBufferSize;
        param[1]   = (uintptr_t)pBuf;
        nMessageId = PLAYER_VIDEO_DECODER_NOTIFY_SET_SECURE_BUFFERS;

        p->callback(p->pUserData,nMessageId,(void*)param);

        p->pSecureBuf = pBuf;
    }
    else
    {
        p->memOps = MemAdapterGetOpsS();
        CdcMemOpen(p->memOps);
    }

_exit:
    if(ret < 0)
    {
        p->bCrashFlag = 1;
        loge("WFD_VideoDecCompSetVideoStreamInfo fail");
    }

    return ret;
}

int WFD_VideoDecCompGetVideoStreamInfo(VideoDecComp* p, VideoStreamInfo* pStreamInfo)
{
    return GetVideoStreamInfo(p->pDecoder, pStreamInfo);
}

int WFD_VideoDecCompSetTimer(VideoDecComp* p, AvTimer* timer)
{
    p->pAvTimer  = timer;
    return 0;
}

int WFD_VideoDecCompRequestStreamBuffer(VideoDecComp* p,
                                    int           nRequireSize,
                                    char**        ppBuf,
                                    int*          pBufSize,
                                    char**        ppRingBuf,
                                    int*          pRingBufSize,
                                    int           nStreamIndex)
{
    int ret;

    //* we can use the same sbm interface to process secure video

    ret = RequestVideoStreamBuffer(p->pDecoder,
                                    nRequireSize,
                                    ppBuf,
                                    pBufSize,
                                    ppRingBuf,
                                    pRingBufSize,
                                    nStreamIndex);

    if(p->bCrashFlag && (ret < 0 || ((*pBufSize + *pRingBufSize) < nRequireSize)))
    {
        //* decoder crashed.
        ResetVideoDecoder(p->pDecoder); //* flush streams.
    }

    return ret;
}

int WFD_VideoDecCompSubmitStreamData(VideoDecComp*        p,
                                 VideoStreamDataInfo* pDataInfo,
                                 int                  nStreamIndex)
{
    int                  ret;
    int                  nSemCnt;

    //* don't receive input stream when decoder crashed.
    //* so the stream buffer always has empty buffer for the demux.
    //* otherwise the demux thread will blocked when video stream buffer is full.
    if(p->bCrashFlag)
    {
        return 0;
    }

    //* we can use the same sbm interface to process secure video

    ret = SubmitVideoStreamData(p->pDecoder, pDataInfo, nStreamIndex);
    if(sem_getvalue(&p->streamDataSem, &nSemCnt) == 0)
    {
        if(nSemCnt == 0)
            sem_post(&p->streamDataSem);
    }
    p->nSelectStreamIndex = nStreamIndex;

    return ret;
}

int WFD_VideoDecCompStreamDataSize(VideoDecComp* p, int nStreamIndex)
{
    if(p == NULL)
        return 0;

    return VideoStreamDataSize(p->pDecoder, nStreamIndex);
}

int WFD_VideoDecCompStreamFrameNum(VideoDecComp* p, int nStreamIndex)
{
    if (p == NULL)
        return 0;

    return VideoStreamFrameNum(p->pDecoder, nStreamIndex);
}

VideoPicture* WFD_VideoDecCompRequestPicture(VideoDecComp* p, int nStreamIndex, int* bResolutionChanged)
{
    VideoPicture *pPicture;

    pPicture = RequestPicture(p->pDecoder, nStreamIndex);
    if(bResolutionChanged != NULL)
    {
        if(pPicture != NULL || p->bResolutionChange == 0)
        {
            *bResolutionChanged = 0;
        }
        else
        {
            logd("set resolution changed.");
            *bResolutionChanged = 1;
        }
    }

    return pPicture;
}

int WFD_ValidPictureNum(VideoDecComp* p, int nStreamIndex)
{
    int num;
    num = ValidPictureNum(p->pDecoder, nStreamIndex);
    return num;
}

int WFD_VideoDecCompReturnPicture(VideoDecComp* p, VideoPicture* pPicture)
{
    int ret;
    int nSemCnt;

    ret = ReturnPicture(p->pDecoder, pPicture);
    if(sem_getvalue(&p->frameBufferSem, &nSemCnt) == 0)
    {
        if(nSemCnt == 0)
            sem_post(&p->frameBufferSem);
    }

    return ret;
}

int WFD_VideoDecCompReopenVideoEngine(VideoDecComp* p)
{
    int ret;

    VideoStreamInfo* pVideoInfo =
        (VideoStreamInfo*)VideoStreamDataInfoPointer(p->pDecoder,p->nSelectStreamIndex);
    if(pVideoInfo != NULL)
    {
        if(p->videoStreamInfo.pCodecSpecificData)
            free(p->videoStreamInfo.pCodecSpecificData);
        memcpy(&p->videoStreamInfo,pVideoInfo,sizeof(VideoStreamInfo));
        if(pVideoInfo->pCodecSpecificData)
        {
            p->videoStreamInfo.pCodecSpecificData =
                (char*)malloc(pVideoInfo->nCodecSpecificDataLen);
            if(p->videoStreamInfo.pCodecSpecificData == NULL)
            {
                loge("malloc video codec specific data failed!");
                return -1;
            }
            memcpy(p->videoStreamInfo.pCodecSpecificData,
                   pVideoInfo->pCodecSpecificData,
                   pVideoInfo->nCodecSpecificDataLen);
            p->videoStreamInfo.nCodecSpecificDataLen = pVideoInfo->nCodecSpecificDataLen;

            free(pVideoInfo->pCodecSpecificData);
        }
        else
        {
            p->videoStreamInfo.pCodecSpecificData    = NULL;
            p->videoStreamInfo.nCodecSpecificDataLen = 0;
        }

        free(pVideoInfo);
    }
    else
    {
        //*if resolustionChange was detected by decoder, we should not send the
        //* specific data to decoder. or decoder will appear error.
        if(p->videoStreamInfo.pCodecSpecificData)
            free(p->videoStreamInfo.pCodecSpecificData);

        p->videoStreamInfo.pCodecSpecificData = NULL;
        p->videoStreamInfo.nCodecSpecificDataLen = 0;
    }
    if(p->videoStreamInfo.bSecureStreamFlagLevel1 == 1)
    {
        p->vconfig.memops = SecureMemAdapterGetOpsS();
    }
    else
    {
        p->vconfig.memops = MemAdapterGetOpsS();
    }

    ret = ReopenVideoEngine(p->pDecoder,&p->vconfig,&p->videoStreamInfo);

    p->bResolutionChange = 0;
    BaseCompContinue(&p->base);

    return ret;
}

#if 0 /* v2.7 only support new display, do not need rotate pic in decoder */
int VideoDecCompRotatePicture(VideoDecComp* p,
                              VideoPicture* pPictureIn,
                              VideoPicture* pPictureOut,
                              int           nRotateDegree,
                              int           nGpuYAlign,
                              int           nGpuCAlign)
{
    //* on chip-1673,we use hardware to do video rotation
#if(ROTATE_PIC_HW == 1)
    CDX_PLAYER_UNUSE(nGpuYAlign);
    CDX_PLAYER_UNUSE(nGpuCAlign);
    return RotatePictureHw(p->pDecoder,pPictureIn, pPictureOut, nRotateDegree);
#else
    //* rotatePicture() should known the gpuAlign
    return RotatePicture(p->memOps, pPictureIn, pPictureOut, nRotateDegree,nGpuYAlign,nGpuCAlign);
#endif

}
#endif
/*
VideoPicture* VideoDecCompAllocatePictureBuffer(int nWidth, int nHeight,
            int nLineStride, int ePixelFormat)
{
    return AllocatePictureBuffer(nWidth, nHeight, nLineStride, ePixelFormat);
}

int VideoDecCompFreePictureBuffer(VideoPicture* pPicture)
{
    return FreePictureBuffer(pPicture);
}
*/

#if 0
//* for new display
int VideoDecCompSetVideoRenderCallback(VideoDecComp* v,
        VideoRenderCallback callback, void* pUserData)
{
    VideoDecComp* p;

    p = (VideoDecComp*)v;

    pthread_mutex_lock(&p->videoRenderCallbackMutex);
    p->videoRenderCallback  = callback;
    pthread_mutex_unlock(&p->videoRenderCallbackMutex);

    p->pVideoRenderUserData = pUserData;
    return 0;
}
#endif

//*******************************  START  **********************************//
//** for new display structure interface.
//**
FbmBufInfo* WFD_VideoDecCompGetVideoFbmBufInfo(VideoDecComp* p)
{
    if(p->pDecoder != NULL)
        return GetVideoFbmBufInfo(p->pDecoder);

    CDX_LOGE("the pDecoder is null when call WFD_VideoDecCompGetVideoFbmBufInfo()");
    return NULL;
}

VideoPicture* WFD_VideoDecCompSetVideoFbmBufAddress(VideoDecComp* p,
        VideoPicture* pVideoPicture, int bForbidUseFlag)
{
    if(p->pDecoder != NULL)
        return SetVideoFbmBufAddress(p->pDecoder,pVideoPicture,bForbidUseFlag);

    loge("the pDecoder is null when call WFD_VideoDecCompSetVideoFbmBufAddress()");
    return NULL;
}

int WFD_VideoDecCompSetVideoFbmBufRelease(VideoDecComp* p)
{
    if(p->pDecoder != NULL)
        return SetVideoFbmBufRelease(p->pDecoder);

    loge("the pDecoder is null when call WFD_VideoDecCompSetVideoFbmBufRelease()");
    return -1;
}

VideoPicture* WFD_VideoDecCompRequestReleasePicture(VideoDecComp* p)
{
    if(p->pDecoder != NULL)
        return RequestReleasePicture(p->pDecoder);

    loge("the pDecoder is null when call WFD_VideoDecCompRequestReleasePicture()");
    return NULL;
}

VideoPicture*  WFD_VideoDecCompReturnRelasePicture(VideoDecComp* p,
        VideoPicture* pVpicture, int bForbidUseFlag)
{
    if(p->pDecoder != NULL)
        return ReturnReleasePicture(p->pDecoder,pVpicture, bForbidUseFlag);

    loge("the pDecoder is null when call WFD_VideoDecCompReturnRelasePicture()");
    return NULL;
}

static void* VideoDecodeThread(void* arg)
{
    VideoDecComp *p = arg;
    AwMessage msg;

    prctl(PR_SET_NAME, (unsigned long)"AWDecoderThread", 0, 0, 0);

#if defined(CONF_MEDIA_BOOST_CPU)
#if (CONF_MEDIA_BOOST_CPU_MODE == 2) // h6
    //AWDecoderThread prio = 105
    char cmd[256];
    sprintf(cmd, "mode%d:0:p:105", gettid());
    property_set("media.boost.pref", cmd);
    logd("setprop media.boost.pref %s", cmd);
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
    VideoDecComp *p = arg;

    if(p->eStatus == PLAYER_STATUS_STARTED)
    {
        loge("invalid start operation, already in started status.");
        if(msg->result != NULL)
            *msg->result = -1;
        sem_post(msg->replySem);
        return;
    }

    if(p->eStatus == PLAYER_STATUS_PAUSED)
    {
        if(p->bCrashFlag == 0)
            BaseCompContinue(&p->base);
        p->eStatus = PLAYER_STATUS_STARTED;
        if (msg->result)
            *msg->result = 0;
        sem_post(msg->replySem);
        return;
    }

    BaseCompContinue(&p->base);
    ResetVideoDecoder(p->pDecoder);
    p->bEosFlag = 0;
    p->eStatus = PLAYER_STATUS_STARTED;

    if (msg->result)
        *msg->result = 0;
    sem_post(msg->replySem);
}

static void handleStop(AwMessage *msg, void *arg)
{
    VideoDecComp *p = arg;

    if(p->eStatus == PLAYER_STATUS_STOPPED)
    {
        loge("invalid stop operation, already in stopped status.");
        if (msg->result)
            *msg->result = -1;
        sem_post(msg->replySem);
    }

    p->firstFrameStatus = FIRST_FRAME_NOT_READY;
    ResetVideoDecoder(p->pDecoder);
    p->eStatus = PLAYER_STATUS_STOPPED;

    if (msg->result)
        *msg->result = 0;
    sem_post(msg->replySem);
}

static void handlePause(AwMessage *msg, void *arg)
{
    VideoDecComp *p = arg;

    if(p->eStatus != PLAYER_STATUS_STARTED  &&
       !(p->eStatus == PLAYER_STATUS_PAUSED && p->firstFrameStatus != FIRST_FRAME_READY))
    {
        loge("invalid pause operation, component not in started status.");
        if (msg->result)
            *msg->result = -1;
        sem_post(msg->replySem);
        return;
    }

    p->eStatus = PLAYER_STATUS_PAUSED;
    if(p->firstFrameStatus != FIRST_FRAME_READY)
        BaseCompContinue(&p->base);

    if (msg->result)
        *msg->result = 0;
    sem_post(msg->replySem);
}

static void handleQuit(AwMessage *msg, void *arg)
{
    VideoDecComp *p = arg;

    ResetVideoDecoder(p->pDecoder);
    p->eStatus = PLAYER_STATUS_STOPPED;

    if (msg->result)
        *msg->result = 0;
    sem_post(msg->replySem);
    pthread_exit(NULL);
}

static void handleReset(AwMessage *msg, void *arg)
{
    VideoDecComp *p = arg;

    p->bEosFlag = 0;
    p->firstFrameStatus = FIRST_FRAME_NOT_READY;
    ResetVideoDecoder(p->pDecoder);

    if (msg->result)
        *msg->result = 0;
    sem_post(msg->replySem);

    //* send a message to continue the thread.
    if(p->eStatus != PLAYER_STATUS_STOPPED)
        BaseCompContinue(&p->base);
}

static void handleSetEos(AwMessage *msg, void *arg)
{
    VideoDecComp *p = arg;

    p->bEosFlag = 1;
    sem_post(msg->replySem);

    if(p->bCrashFlag == 0)
    {
        //* send a message to continue the thread.
        if(p->eStatus == PLAYER_STATUS_STARTED ||
           (p->eStatus == PLAYER_STATUS_PAUSED && p->firstFrameStatus != FIRST_FRAME_READY))
            BaseCompContinue(&p->base);
    }
    else
    {
        logv("video decoder notify eos.");
        p->callback(p->pUserData, PLAYER_VIDEO_DECODER_NOTIFY_EOS, NULL);
    }
}

static void doDecode(AwMessage *msg, void *arg)
{
    VideoDecComp *p = arg;
    (void)msg;

    if(p->eStatus != PLAYER_STATUS_STARTED &&
       !(p->eStatus == PLAYER_STATUS_PAUSED && p->firstFrameStatus != FIRST_FRAME_READY))
    {
        loge("not in started status, ignore decode message.");
        return;
    }

    if(p->bCrashFlag)
    {
        logw("video decoder has already crashed,  MESSAGE_ID_DECODE will not be processe.");
        return;
    }

    int64_t nCurTime = p->pAvTimer->GetTime(p->pAvTimer);

    int ret = DecodeVideoStream(p->pDecoder,
                                p->bEosFlag,
                                p->bConfigDecodeKeyFrameOnly,
                                p->bConfigDropDelayFrames,
                                nCurTime);

    CDX_LOGV("DecodeVideoStream return = %d, p->bCrashFlag(%d)", ret, p->bCrashFlag);

    if(ret == VDECODE_RESULT_NO_BITSTREAM)
    {
        if(p->bEosFlag)
        {
            logv("video decoder notify eos.");
            p->callback(p->pUserData, PLAYER_VIDEO_DECODER_NOTIFY_EOS, NULL);
            return;
        }
        else
        {
            SemTimedWait(&p->streamDataSem, 1);    //* wait for stream data.
            BaseCompContinue(&p->base);
            return;
        }
    }
    else if(ret == VDECODE_RESULT_NO_FRAME_BUFFER)
    {
        SemTimedWait(&p->frameBufferSem, 20);    //* wait for frame buffer.
        BaseCompContinue(&p->base);
        return;
    }
    else if(ret == VDECODE_RESULT_RESOLUTION_CHANGE)
    {
        logv("decode thread detect resolution change.");
        p->bResolutionChange = 1;
        //*for new display, we should callback the message to videoRender
        if(p->videoRenderCallback != NULL)
        {
            int msg = VIDEO_RENDER_RESOLUTION_CHANGE;
            p->videoRenderCallback(p->pVideoRenderUserData, msg,
                    (void*)(&p->bResolutionChange));
        }
        return;
    }
    else if(ret < 0)    //* ret == VDECODE_RESULT_UNSUPPORTED
    {
        logw("video decoder notify crash.");
        p->bCrashFlag = 1;
        p->callback(p->pUserData, PLAYER_VIDEO_DECODER_NOTIFY_CRASH, NULL);
        return;
    }

    if(p->firstFrameStatus != FIRST_FRAME_READY)
    {
        if(ret == VDECODE_RESULT_FRAME_DECODED ||
                ret == VDECODE_RESULT_KEYFRAME_DECODED)
        {
            VideoStreamInfo videoStreamInfo;
            GetVideoStreamInfo(p->pDecoder, &videoStreamInfo);
            if(videoStreamInfo.bIs3DStream)
            {
                if(p->firstFrameStatus == FIRST_3D_FRAME_READY)
                    p->firstFrameStatus = SECOND_3D_FRAME_READY;
                else
                    //* decode one picture of a 3d frame(with two picture).
                    p->firstFrameStatus = FIRST_3D_FRAME_READY;
            }
            else
                p->firstFrameStatus = FIRST_FRAME_READY;
        }
    }

    if(p->eStatus == PLAYER_STATUS_STARTED || p->firstFrameStatus != FIRST_FRAME_READY)
    {
        BaseCompContinue(&p->base);
        return;
    }
}

