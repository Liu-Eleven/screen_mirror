/*
 * Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
 * All rights reserved.
 *
 * File : audioDecComponent.cpp
 * Description : audio decoder component
 * History :
 *
 */

#include "cdx_log.h"

#include <pthread.h>
#include <semaphore.h>
#include <malloc.h>
#include <memory.h>
#include <time.h>

#include "audioDecComponent.h"
#include "adecoder.h"

#include "AwMessageQueue.h"
#include "streamManager.h"
#include "baseComponent.h"

#define MAX_AUDIO_STREAM_BUFFER_SIZE (1024*1024)
#define MAX_AUDIO_STREAM_FRAME_COUNT (1024)

struct AudioDecComp
{
    //* created at initialize time.
    AwMessageQueue*        mq;

    BaseCompCtx             base;

    sem_t                   streamDataSem;
    sem_t                   frameBufferSem;

    pthread_t               sDecodeThread;

    enum EPLAYERSTATUS      eStatus;

    //* objects set by user.
    AvTimer*                pAvTimer;
    PlayerCallback          callback;
    void*                   pUserData;
    int                     bEosFlag;

    int                     nOffset;
    BsInFor                 bsInfo;
    AudioDecoder*           pDecoder;
    int                     nStreamCount;
    int                     nStreamSelected;

    AudioStreamInfo*        pStreamInfoArr;
    StreamManager**         pStreamManagerArr;
    pthread_mutex_t         streamManagerMutex;

    pthread_mutex_t         decoderDestroyMutex;    //* to protect decoder from destroyed.

    int                     bCrashFlag;

    int                     afterSwitchStream;
    int                     bFirstFramePtsValid;
};

static void handleStart(AwMessage *msg, void *arg);
static void handleStop(AwMessage *msg, void *arg);
static void handlePause(AwMessage *msg, void *arg);
static void handleReset(AwMessage *msg, void *arg);
static void handleSetEos(AwMessage *msg, void *arg);
static void handleQuit(AwMessage *msg, void *arg);
static void doDecode(AwMessage *msg, void *arg);

static void* AudioDecodeThread(void* arg);
#if 0
static void FlushStreamManagerBuffers(AudioDecComp* p, int64_t curTime,
                                    int bIncludeSeletedStream);
#endif

AudioDecComp* WFD_AudioDecCompCreate(void)
{
    AudioDecComp* p;
    int                  err;

    p = (AudioDecComp*)malloc(sizeof(AudioDecComp));
    if(p == NULL)
    {
        loge("memory alloc fail.");
        return NULL;
    }
    memset(p, 0, sizeof(*p));

    p->mq = AwMessageQueueCreate(4, "AudioDecodeMq");
    if(p->mq == NULL)
    {
        loge("audio decoder component create message queue fail.");
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

    if (BaseCompInit(&p->base, "audio decoder", p->mq, &handler))
    {
        AwMessageQueueDestroy(p->mq);
        free(p);
        return NULL;
    }

    sem_init(&p->streamDataSem, 0, 0);
    sem_init(&p->frameBufferSem, 0, 0);
    pthread_mutex_init(&p->streamManagerMutex, NULL);
    pthread_mutex_init(&p->decoderDestroyMutex, NULL);

    p->eStatus = PLAYER_STATUS_STOPPED;

    err = pthread_create(&p->sDecodeThread, NULL, AudioDecodeThread, p);
    if(err != 0)
    {
        loge("audio decode component create thread fail.");
        BaseCompDestroy(&p->base);
        sem_destroy(&p->streamDataSem);
        sem_destroy(&p->frameBufferSem);
        pthread_mutex_destroy(&p->streamManagerMutex);
        pthread_mutex_destroy(&p->decoderDestroyMutex);
        AwMessageQueueDestroy(p->mq);
        free(p);
        return NULL;
    }

    return p;
}

static void WakeUpThread(void *arg)
{
    AudioDecComp *p = arg;
    //* wake up the thread if it is pending for stream data or frame buffer.
    sem_post(&p->streamDataSem);
    sem_post(&p->frameBufferSem);
}

int WFD_AudioDecCompDestroy(AudioDecComp* p)
{
    void*                status;
    int                  i;

    BaseCompQuit(&p->base, WakeUpThread, p);

    pthread_join(p->sDecodeThread, &status);

    BaseCompDestroy(&p->base);

    sem_destroy(&p->streamDataSem);
    sem_destroy(&p->frameBufferSem);
    pthread_mutex_destroy(&p->streamManagerMutex);
    pthread_mutex_destroy(&p->decoderDestroyMutex);

    if(p->pStreamInfoArr != NULL)
    {
        for(i=0; i<p->nStreamCount; i++)
        {
            if(p->pStreamInfoArr[i].pCodecSpecificData != NULL &&
                    p->pStreamInfoArr[i].nCodecSpecificDataLen > 0)
            {
                free(p->pStreamInfoArr[i].pCodecSpecificData);
                p->pStreamInfoArr[i].pCodecSpecificData = NULL;
            }
        }
        free(p->pStreamInfoArr);
    }

    if(p->pStreamManagerArr != NULL)
    {
        for(i=0; i<p->nStreamCount; i++)
        {
            if(p->pStreamManagerArr[i] != NULL)
            {
                WFD_StreamManagerDestroy(p->pStreamManagerArr[i]);
                p->pStreamManagerArr[i] = NULL;
            }
        }
        free(p->pStreamManagerArr);
    }

    AwMessageQueueDestroy(p->mq);
    free(p);

    return 0;
}

int WFD_AudioDecCompStart(AudioDecComp* p)
{
    return BaseCompStart(&p->base, WakeUpThread, p);
}

int WFD_AudioDecCompStop(AudioDecComp* p)
{
    return BaseCompStop(&p->base, WakeUpThread, p);
}

int WFD_AudioDecCompPause(AudioDecComp* p)
{
    return BaseCompPause(&p->base, WakeUpThread, p);
}

int WFD_AudioDecCompReset(AudioDecComp* p, int64_t nSeekTime)
{
    return BaseCompReset(&p->base, nSeekTime, WakeUpThread, p);
}

int WFD_AudioDecCompSetEOS(AudioDecComp* p)
{
    return BaseCompSetEos(&p->base, WakeUpThread, p);
}

int WFD_AudioDecCompSetCallback(AudioDecComp* p, PlayerCallback callback, void* pUserData)
{
    p->callback  = callback;
    p->pUserData = pUserData;

    return 0;
}

int WFD_AudioDecCompSetAudioStreamInfo(AudioDecComp*    p,
                                   AudioStreamInfo* pStreamInfo,
                                   int              nStreamCount,
                                   int              nDefaultStreamIndex)
{
    int i;

    if(p->pStreamInfoArr != NULL && p->nStreamCount > 0)
    {
        for(i=0; i<p->nStreamCount; i++)
        {
            if(p->pStreamInfoArr[i].pCodecSpecificData != NULL &&
               p->pStreamInfoArr[i].nCodecSpecificDataLen > 0)
            {
                free(p->pStreamInfoArr[i].pCodecSpecificData);
                p->pStreamInfoArr[i].pCodecSpecificData = NULL;
                p->pStreamInfoArr[i].nCodecSpecificDataLen = 0;
            }
        }

        free(p->pStreamInfoArr);
        p->pStreamInfoArr = NULL;
    }

    if(p->pStreamManagerArr != NULL)
    {
        for(i=0; i<p->nStreamCount; i++)
        {
            if(p->pStreamManagerArr[i] != NULL)
            {
                WFD_StreamManagerDestroy(p->pStreamManagerArr[i]);
                p->pStreamManagerArr[i] = NULL;
            }
        }
        free(p->pStreamManagerArr);
    }

    p->nStreamSelected = 0;
    p->nStreamCount = 0;

    p->pStreamInfoArr = (AudioStreamInfo*)malloc(sizeof(AudioStreamInfo)*nStreamCount);
    if(p->pStreamInfoArr == NULL)
    {
        loge("memory malloc fail!");
        return -1;
    }
    memset(p->pStreamInfoArr, 0, sizeof(AudioStreamInfo)*nStreamCount);

    for(i=0; i<nStreamCount; i++)
    {
        memcpy(&p->pStreamInfoArr[i], &pStreamInfo[i], sizeof(AudioStreamInfo));
        if(pStreamInfo[i].pCodecSpecificData != NULL && pStreamInfo[i].nCodecSpecificDataLen > 0)
        {
            p->pStreamInfoArr[i].pCodecSpecificData =
                            (char*)malloc(pStreamInfo[i].nCodecSpecificDataLen);
            if(p->pStreamInfoArr[i].pCodecSpecificData == NULL)
            {
                loge("malloc memory fail.");
                p->pStreamInfoArr[i].nCodecSpecificDataLen = 0;
                break;
            }
            memcpy(p->pStreamInfoArr[i].pCodecSpecificData,
                   pStreamInfo[i].pCodecSpecificData,
                   pStreamInfo[i].nCodecSpecificDataLen);
        }
    }

    if(i != nStreamCount)
    {
        //* memory alloc fail break.
        i--;
        for(; i>=0; i--)
        {
            if(p->pStreamInfoArr[i].pCodecSpecificData != NULL &&
                    p->pStreamInfoArr[i].nCodecSpecificDataLen > 0)
            {
                free(p->pStreamInfoArr[i].pCodecSpecificData);
                p->pStreamInfoArr[i].pCodecSpecificData = NULL;
                p->pStreamInfoArr[i].nCodecSpecificDataLen = 0;
            }
        }
        free(p->pStreamInfoArr);
        return -1;
    }

    p->pStreamManagerArr = (StreamManager**)malloc(nStreamCount*sizeof(StreamManager*));
    if(p->pStreamManagerArr == NULL)
    {
        loge("malloc memory fail.");
        for(i=0; i<nStreamCount; i++)
        {
            if(p->pStreamInfoArr[i].pCodecSpecificData != NULL &&
                    p->pStreamInfoArr[i].nCodecSpecificDataLen > 0)
            {
                free(p->pStreamInfoArr[i].pCodecSpecificData);
                p->pStreamInfoArr[i].pCodecSpecificData = NULL;
                p->pStreamInfoArr[i].nCodecSpecificDataLen = 0;
            }
        }
        free(p->pStreamInfoArr);
        return -1;
    }

    for(i=0; i<nStreamCount; i++)
    {
        p->pStreamManagerArr[i] = WFD_StreamManagerCreate(MAX_AUDIO_STREAM_BUFFER_SIZE,
                                                      MAX_AUDIO_STREAM_FRAME_COUNT,
                                                      i);
        if(p->pStreamManagerArr[i] == NULL)
        {
            loge("create stream manager for audio stream %d fail", i);
            break;
        }
    }

    if(i != nStreamCount)
    {
        //* memory alloc fail break.
        i--;
        for(; i>=0; i--)
        {
            WFD_StreamManagerDestroy(p->pStreamManagerArr[i]);
            p->pStreamManagerArr[i] = NULL;
        }
        free(p->pStreamManagerArr);
        p->pStreamManagerArr = NULL;

        for(i=0; i<nStreamCount; i++)
        {
            if(p->pStreamInfoArr[i].pCodecSpecificData != NULL &&
                    p->pStreamInfoArr[i].nCodecSpecificDataLen > 0)
            {
                free(p->pStreamInfoArr[i].pCodecSpecificData);
                p->pStreamInfoArr[i].pCodecSpecificData = NULL;
                p->pStreamInfoArr[i].nCodecSpecificDataLen = 0;
            }
        }
        free(p->pStreamInfoArr);
        return -1;
    }

    p->nStreamSelected = nDefaultStreamIndex;
    p->nStreamCount = nStreamCount;

    return 0;
}

int WFD_AudioDecCompGetAudioSampleRate(AudioDecComp* p,
                                   unsigned int* pSampleRate,
                                   unsigned int* pChannelNum,
                                   unsigned int* pBitRate)
{
    int                  i;

    if(p->nStreamCount <= 0)
        return -1;
    i = p->nStreamSelected;
    *pSampleRate = p->pStreamInfoArr[i].nSampleRate;
    *pChannelNum = p->pStreamInfoArr[i].nChannelNum;
    *pBitRate    = p->pStreamInfoArr[i].nAvgBitrate;
    return 0;
}

int WFD_AudioDecCompSetTimer(AudioDecComp* p, AvTimer* timer)
{
    p->pAvTimer = timer;
    return 0;
}

int WFD_AudioDecCompRequestStreamBuffer(AudioDecComp* p,
                                    int           nRequireSize,
                                    char**        ppBuf,
                                    int*          pBufSize,
                                    char**        ppRingBuf,
                                    int*          pRingBufSize,
                                    int           nStreamIndex)
{
    StreamManager*       pSm;
    StreamFrame*         pTmpFrame;
    char*                pBuf0;
    char*                pBuf1;
    int                  nBufSize0;
    int                  nBufSize1;

    *ppBuf        = NULL;
    *ppRingBuf    = NULL;
    *pBufSize     = 0;
    *pRingBufSize = 0;

    pBuf0      = NULL;
    pBuf1      = NULL;
    nBufSize0  = 0;
    nBufSize1  = 0;

    pthread_mutex_lock(&p->streamManagerMutex);

    if(nStreamIndex < 0 || nStreamIndex >= p->nStreamCount)
    {
        loge("stream index invalid, stream index = %d, audio stream num = %d",
                    nStreamIndex, p->nStreamCount);
        pthread_mutex_unlock(&p->streamManagerMutex);
        return -1;
    }

    /* This is a waste of time. If decoder is crashed, p->bCrashFlag != 0,
     * we will call StreamManagerRequestStream in this function when necessary,
     * and free some space.
     *
     * 2016-05-17
     * You can remove this block of code after, say half a year.
     */
#if 0
    //* when decoder crashed, the main thread is not running,
    //* we need to flush stream buffers for demux keep going.
    if (p->bCrashFlag &&
            p->pAvTimer->GetStatus(p->pAvTimer) == TIMER_STATUS_START)
    {
        int64_t nCurTime = p->pAvTimer->GetTime(p->pAvTimer);
        for (int i = 0; i < p->nStreamCount; i++)
        {
            pSm = p->pStreamManagerArr[i];
            StreamManagerRewind(pSm, nCurTime);
        }
    }
#endif

    pSm = p->pStreamManagerArr[nStreamIndex];

    if(pSm == NULL)
    {
        loge("buffer for selected stream is not created, request buffer fail.");
        pthread_mutex_unlock(&p->streamManagerMutex);
        return -1;
    }

    if(nRequireSize > WFD_StreamManagerBufferSize(pSm))
    {
        loge("require size too big.");
        pthread_mutex_unlock(&p->streamManagerMutex);
        return -1;
    }

    if(nStreamIndex == p->nStreamSelected && p->bCrashFlag == 0)
    {
        if(WFD_StreamManagerRequestBuffer(pSm, nRequireSize, &pBuf0, &nBufSize0) < 0)
        {
            pthread_mutex_unlock(&p->streamManagerMutex);
            logv("request buffer fail.");
            return -1;
        }
    }
    else
    {
        while(WFD_StreamManagerRequestBuffer(pSm, nRequireSize, &pBuf0, &nBufSize0) < 0)
        {
            pTmpFrame = WFD_StreamManagerRequestStream(pSm);
            if(pTmpFrame != NULL)
                WFD_StreamManagerFlushStream(pSm, pTmpFrame);
            else
            {
                loge("all stream flushed but still can not allocate buffer.");
                pthread_mutex_unlock(&p->streamManagerMutex);
                return -1;
            }
        }
    }

    //* output the buffer.
    *ppBuf    = pBuf0;
    *pBufSize = nBufSize0;

    pthread_mutex_unlock(&p->streamManagerMutex);
    return 0;
}

int WFD_AudioDecCompSubmitStreamData(AudioDecComp*        p,
                                 AudioStreamDataInfo* pDataInfo,
                                 int                  nStreamIndex)
{
    int                  nSemCnt;
    StreamManager*       pSm;
    StreamFrame          streamFrame;

    //* submit data to stream manager
    pthread_mutex_lock(&p->streamManagerMutex);

    pSm = p->pStreamManagerArr[nStreamIndex];

    streamFrame.pData   = pDataInfo->pData;
    streamFrame.nLength = pDataInfo->nLength;
    if(pDataInfo->bIsFirstPart)
    {
        streamFrame.nPts = pDataInfo->nPts;
        streamFrame.nPcr = pDataInfo->nPcr;
    }
    else
    {
        streamFrame.nPts = -1;
        streamFrame.nPcr = -1;
    }

    WFD_StreamManagerAddStream(pSm, &streamFrame);

    pthread_mutex_unlock(&p->streamManagerMutex);

    if(sem_getvalue(&p->streamDataSem, &nSemCnt) == 0)
    {
        if(nSemCnt == 0)
            sem_post(&p->streamDataSem);
    }

    return 0;
}

int AudioDecCompStreamBufferSize(AudioDecComp* p, int nStreamIndex)
{
    StreamManager*       pSm;

    pSm = p->pStreamManagerArr[nStreamIndex];

    return WFD_StreamManagerBufferSize(pSm);
}

int WFD_AudioDecCompRequestPcmData(AudioDecComp*   p,
                               unsigned char** ppData,
                               unsigned int*   pSize,
                               int64_t*        pPts,
                               CdxPlaybkCfg*   cfg)
{
    AudioStreamInfo pStreamInfo = p->pStreamInfoArr[p->nStreamSelected];
    //* this method is called by the audio render thread,
    //* the audio render thread is paused or stop before the audio decoder thread,
    //* so here we do not need to lock the decoderDestroyMutex.
    if(p->pDecoder != NULL)
    {
        *pPts = PlybkRequestPcmPts(p->pDecoder);
        memcpy(cfg,&(pStreamInfo.raw_data),sizeof(CdxPlaybkCfg));
        return  PlybkRequestPcmBuffer(p->pDecoder, ppData, (int*)pSize);
    }
    else
    {
        *ppData = NULL;
        *pPts = -1;
        memset(cfg,0,sizeof(CdxPlaybkCfg));
        return -1;
    }
}

int WFD_AudioDecCompReleasePcmData(AudioDecComp* p, int nReleaseSize)
{
    int ret;
    int nSemCnt;

    //* this method is called by the audio render thread,
    //* the audio render thread is paused or stop before the audio decoder thread,
    //* so here we do not need to lock the decoderDestroyMutex.
    ret = PlybkUpdatePcmBuffer(p->pDecoder, nReleaseSize);

    if(sem_getvalue(&p->frameBufferSem, &nSemCnt) == 0)
    {
        if(nSemCnt == 0)
            sem_post(&p->frameBufferSem);
    }

    return ret;
}

int WFD_AudioDecCompPcmDataSize(AudioDecComp* p, int nStreamIndex)
{
    int                  nPcmDataSize;

    nPcmDataSize = 0;

    CEDARX_UNUSE(nStreamIndex);

    //* this method is called by the demux thread, the decoder may be destroyed when
    //* switching audio, so we should lock the decoderDestroyMutex to protect the
    //* decoder from destroyed.
    pthread_mutex_lock(&p->decoderDestroyMutex);
    if(p->pDecoder != NULL)
        nPcmDataSize = AudioPCMDataSize(p->pDecoder);
    pthread_mutex_unlock(&p->decoderDestroyMutex);

    return nPcmDataSize;
}

static void* AudioDecodeThread(void* arg)
{
    AudioDecComp *p = arg;
    AwMessage msg;

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
    AudioDecComp *p = arg;

    if (p->eStatus == PLAYER_STATUS_STARTED)
    {
        loge("invalid start operation, already in started status.");
        if (msg->result)
            *msg->result = -1;
        sem_post(msg->replySem);
        return;
    }

    if (p->eStatus == PLAYER_STATUS_PAUSED)
    {
        //* send a decode message to start decoding.
        if (p->bCrashFlag == 0)
            BaseCompContinue(&p->base);
        p->eStatus = PLAYER_STATUS_STARTED;
        if (msg->result)
            *msg->result = 0;
        sem_post(msg->replySem);
        return;
    }

    //* create a decoder.
    //* lock the decoderDestroyMutex to prevend the demux thread from getting
    //* stream data size or pcm data size when the decoder is being created.
    //* see AudioDecCompStreamDataSize() and AudioDecCompPcmDataSize().
    pthread_mutex_lock(&p->decoderDestroyMutex);

    p->pDecoder = CreateAudioDecoder();
    if (p->pDecoder == NULL)
    {
        pthread_mutex_unlock(&p->decoderDestroyMutex);
        loge("audio decoder component create decoder fail.");
        p->bCrashFlag = 1;
        p->callback(p->pUserData, PLAYER_AUDIO_DECODER_NOTIFY_CRASH, NULL);
        if (msg->result)
            *msg->result = -1;
        sem_post(msg->replySem);
        return;
    }

    memset(&p->bsInfo, 0, sizeof(BsInFor));
    if (InitializeAudioDecoder(p->pDecoder,
                              &p->pStreamInfoArr[p->nStreamSelected],
                              &p->bsInfo) != 0)
    {
        loge("initialize audio decoder fail.");
        DestroyAudioDecoder(p->pDecoder);
        p->pDecoder = NULL;
        pthread_mutex_unlock(&p->decoderDestroyMutex);
        p->bCrashFlag = 1;
        p->callback(p->pUserData, PLAYER_AUDIO_DECODER_NOTIFY_CRASH, NULL);
        if (msg->result)
            *msg->result = -1;
        sem_post(msg->replySem);
        return;
    }

#ifdef __ANDROID__
    SetRawPlayParam(p->pDecoder,(void*)p);
#else
    SetRawPlayParam(p->pDecoder,(void*)p, 0);
#endif
    //* demux thread can use the decoder now.
    pthread_mutex_unlock(&p->decoderDestroyMutex);

    //* send a decode message.
    BaseCompContinue(&p->base);
    p->bEosFlag = 0;
    p->eStatus = PLAYER_STATUS_STARTED;

    if (msg->result)
        *msg->result = 0;
    sem_post(msg->replySem);
}

static void handleStop(AwMessage *msg, void *arg)
{
    AudioDecComp *p = arg;

    if (p->eStatus == PLAYER_STATUS_STOPPED)
    {
        loge("invalid stop operation, already in stopped status.");
        if (p->bCrashFlag == 1)
            p->bCrashFlag = 0;

        if (msg->result)
            *msg->result = -1;
        sem_post(msg->replySem);
        return;
    }

    //* destroy decoder.
    //* lock the decoderDestroyMutex to prevend the demux thread from getting
    //* stream data size or pcm data size when the decoder is being created.
    //* see AudioDecCompStreamDataSize() and AudioDecCompPcmDataSize().
    pthread_mutex_lock(&p->decoderDestroyMutex);
    p->bFirstFramePtsValid = 0;
    if (p->pDecoder != NULL)
    {
        DestroyAudioDecoder(p->pDecoder);
        p->pDecoder = NULL;
    }
    pthread_mutex_unlock(&p->decoderDestroyMutex);
    memset(&p->bsInfo, 0, sizeof(BsInFor));

    p->bCrashFlag = 0;
    p->eStatus = PLAYER_STATUS_STOPPED;
    if (msg->result)
        *msg->result = 0;
    sem_post(msg->replySem);
}

static void handlePause(AwMessage *msg, void *arg)
{
    AudioDecComp *p = arg;

    if (p->eStatus != PLAYER_STATUS_STARTED)
    {
        loge("invalid pause operation, component not in started status.");
        if (msg->result)
            *msg->result = -1;
        sem_post(msg->replySem);
        return;
    }

    p->eStatus = PLAYER_STATUS_PAUSED;
    if (msg->result)
        *msg->result = 0;
    sem_post(msg->replySem);
}

static void handleReset(AwMessage *msg, void *arg)
{
    AudioDecComp *p = arg;

    pthread_mutex_lock(&p->streamManagerMutex);
    int i;
    for(i=0; i<p->nStreamCount; i++)
    {
        if(p->pStreamManagerArr[i] != NULL)
            WFD_StreamManagerReset(p->pStreamManagerArr[i]);
    }
    pthread_mutex_unlock(&p->streamManagerMutex);

    p->bFirstFramePtsValid = 0;
    p->bEosFlag = 0;
    p->bCrashFlag = 0;
    if(p->pDecoder != NULL)
       ResetAudioDecoder(p->pDecoder, msg->seekTime);

    if (msg->result)
        *msg->result = 0;
    sem_post(msg->replySem);

    //* send a message to continue the thread.
    if (p->eStatus == PLAYER_STATUS_STARTED)
        BaseCompContinue(&p->base);
}

static void handleSetEos(AwMessage *msg, void *arg)
{
    AudioDecComp *p = arg;

    p->bEosFlag = 1;
    sem_post(msg->replySem);

    //* send a message to continue the thread.
    if(p->bCrashFlag == 0 && p->eStatus == PLAYER_STATUS_STARTED)
        BaseCompContinue(&p->base);
}

static void handleQuit(AwMessage *msg, void *arg)
{
    AudioDecComp *p = arg;

    //* destroy decoder and break.
    //* lock the decoderDestroyMutex to prevend the demux thread from getting
    //* stream data size or pcm data size when the decoder is being created.
    //* see AudioDecCompStreamDataSize() and AudioDecCompPcmDataSize().
    pthread_mutex_lock(&p->decoderDestroyMutex);
    if (p->pDecoder != NULL)
    {
        DestroyAudioDecoder(p->pDecoder);
        p->pDecoder = NULL;
    }
    pthread_mutex_unlock(&p->decoderDestroyMutex);

    p->eStatus = PLAYER_STATUS_STOPPED;
    if (msg->result)
        *msg->result = 0;
    sem_post(msg->replySem);
    pthread_exit(NULL);
}

static void doDecode(AwMessage *msg, void *arg)
{
    int ret = 0;
    AudioDecComp *p = arg;
    (void)msg;

    char*          pOutputBuf  = NULL;
    int            nPcmDataLen = 0;
    StreamManager* pSm         = p->pStreamManagerArr[p->nStreamSelected];
    StreamFrame*   pFrame      = NULL;
    unsigned char* pBuf0       = NULL;
    unsigned char* pBuf1       = NULL;
    int            nBufSize0   = 0;
    int            nBufSize1   = 0;

    if (p->eStatus != PLAYER_STATUS_STARTED)
    {
        logw("not in started status, ignore decode message.");
        return;
    }

    pthread_mutex_lock(&p->streamManagerMutex);
    if (p->afterSwitchStream &&
            p->pAvTimer->GetStatus(p->pAvTimer) == TIMER_STATUS_START)
    {
        p->afterSwitchStream = 0;
        int64_t nCurTime = p->pAvTimer->GetTime(p->pAvTimer);
        WFD_StreamManagerRewind(pSm, nCurTime);
    }
    pthread_mutex_unlock(&p->streamManagerMutex);

    if (DecRequestPcmBuffer(p->pDecoder, &pOutputBuf) < 0)
    {
        //* no pcm buffer, wait for the pcm buffer semaphore.
        logv("no pcm buffer, wait.");
        SemTimedWait(&p->frameBufferSem, 20);    //* wait for frame buffer.
        BaseCompContinue(&p->base);
        return;
    }

    //* Add stream to decoder.
    pFrame = WFD_StreamManagerGetFrameInfo(pSm, 0);
    if (pFrame != NULL)
    {
        if((p->bFirstFramePtsValid == 0) && (pFrame->nPts == -1))
        {
            pFrame = WFD_StreamManagerRequestStream(pSm);
            WFD_StreamManagerFlushStream(pSm, pFrame);

            if(p->bEosFlag &&
                WFD_StreamManagerStreamFrameNum(p->pStreamManagerArr[p->nStreamSelected]) == 0)
            {
                logd("audio decoder notify eos.");
                p->callback(p->pUserData, PLAYER_AUDIO_DECODER_NOTIFY_EOS, NULL);
            }
            else
            {
                BaseCompContinue(&p->base);
            }
            return;
        }
        ret = ParserRequestBsBuffer(p->pDecoder,
                                    pFrame->nLength,
                                    &pBuf0,
                                    &nBufSize0,
                                    &pBuf1,
                                    &nBufSize1,
                                    &p->nOffset);
        if ((nBufSize0+nBufSize1) >= pFrame->nLength)
        {
            pFrame = WFD_StreamManagerRequestStream(pSm);
            if(nBufSize0 >= pFrame->nLength)
                memcpy(pBuf0, pFrame->pData, pFrame->nLength);
            else
            {
                memcpy(pBuf0, pFrame->pData, nBufSize0);
                memcpy(pBuf1, (char*)pFrame->pData + nBufSize0,
                        pFrame->nLength - nBufSize0);
            }
            ParserUpdateBsBuffer(p->pDecoder,
                                 pFrame->nLength,
                                 pFrame->nPts,
                                 p->nOffset);
            WFD_StreamManagerFlushStream(pSm, pFrame);
            p->bFirstFramePtsValid = 1;
        }
    }

    ret = DecodeAudioStream(p->pDecoder,
                            &p->pStreamInfoArr[p->nStreamSelected],
                            pOutputBuf,
                            &nPcmDataLen);
    logv("DecodeAudioStream, ret = %d",ret);
    if(ret == ERR_AUDIO_DEC_NONE)
    {
        if(p->pStreamInfoArr[p->nStreamSelected].nSampleRate != p->bsInfo.out_samplerate ||
            p->pStreamInfoArr[p->nStreamSelected].nChannelNum != p->bsInfo.out_channels)
        {
            p->pStreamInfoArr[p->nStreamSelected].nSampleRate = p->bsInfo.out_samplerate;
            p->pStreamInfoArr[p->nStreamSelected].nChannelNum = p->bsInfo.out_channels;
        }
        DecUpdatePcmBuffer(p->pDecoder, nPcmDataLen);
        BaseCompContinue(&p->base);
        return;
    }
    else if(ret == ERR_AUDIO_DEC_NO_BITSTREAM || ret == ERR_AUDIO_DEC_ABSEND)
    {
        if(p->bEosFlag &&
           WFD_StreamManagerStreamFrameNum(p->pStreamManagerArr[p->nStreamSelected]) == 0)
        {
            logv("audio decoder notify eos.");
            p->callback(p->pUserData, PLAYER_AUDIO_DECODER_NOTIFY_EOS, NULL);
            return;
        }
        else
        {
            if(WFD_StreamManagerStreamFrameNum(p->pStreamManagerArr[p->nStreamSelected]) == 0)
                SemTimedWait(&p->streamDataSem, 50);
            BaseCompContinue(&p->base);
            return;
        }
    }
    else if(ret == ERR_AUDIO_DEC_EXIT || ret == ERR_AUDIO_DEC_ENDINGCHKFAIL)
    {
        p->bCrashFlag = 1;
        p->callback(p->pUserData, PLAYER_AUDIO_DECODER_NOTIFY_CRASH, NULL);
        return;
    }
    else
    {
        logw("DecodeAudioStream() return %d, continue to decode", ret);
        BaseCompContinue(&p->base);
    }
}

#if 0
static void FlushStreamManagerBuffers(AudioDecComp* p, int64_t curTime,
                int bIncludeSeletedStream)
{
    //* to prevent from flush incorrectly when pts loop back,
    //* we find the frame who's pts is near the current timer value,
    //* and flush frames before this frame.

    int            i;
    int            nFrameIndex;
    int            nFlushPos;
    int            nFrameCount;
    StreamFrame*   pFrame;
    StreamManager* pSm;
    int64_t        nMinPtsDiff;
    int64_t        nPtsDiff;

    for(i=0; i<p->nStreamCount; i++)
    {
        if(i == p->nStreamSelected && bIncludeSeletedStream == 0)
            continue;

        pSm = p->pStreamManagerArr[i];
        nFrameCount = StreamManagerStreamFrameNum(pSm);
        nMinPtsDiff = 0x7fffffffffffffffLL; //* set it to the max value.
        nFlushPos   = nFrameCount;
        for(nFrameIndex=0; nFrameIndex<nFrameCount; nFrameIndex++)
        {
            pFrame = StreamManagerGetFrameInfo(pSm, nFrameIndex);
            if(pFrame->nPts == -1)
                continue;

            nPtsDiff = pFrame->nPts - curTime;
            if(nPtsDiff >= 0 && nPtsDiff < nMinPtsDiff)
            {
                nMinPtsDiff = nPtsDiff;
                nFlushPos   = nFrameIndex;
            }
        }

        //* flush frames before nFlushPos.
        for(nFrameIndex=0; nFrameIndex<nFlushPos; nFrameIndex++)
        {
            pFrame = StreamManagerRequestStream(pSm);
            StreamManagerFlushStream(pSm, pFrame);
        }
    }

    return;
}
#endif
