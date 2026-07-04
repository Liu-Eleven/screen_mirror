/*
 * Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
 * All rights reserved.
 *
 * File : player.cpp
 * Description : player
 * History :
 *
 */

#include "cdx_log.h"

#include <string.h>
#include "player_i.h"
#include "audioDecComponent.h"
#include "videoDecComponent.h"
#include "audioRenderComponent.h"
#include "videoRenderComponent.h"
#include "AwMessageQueue.h"
#include "avtimer.h"
#include "bitrateEstimater.h"
#include "framerateEstimater.h"
#include "iniparserapi.h"
#include <inttypes.h>
#include "wfd_player.h"
#include "player.h"

//#include "CdxUci.h" //add for get display regionScale value

#define CONFIG_DISABLE_VIDEO    (0)
#define CONFIG_DISABLE_AUDIO    (0)

//#define DEBUG_SAVE_VES
#ifdef DEBUG_SAVE_VES
static const char* dbg_wfd_ves = "/wfd_ves.dat";
static FILE* dbg_wfd_ves_fp = NULL;
#endif

#define CdxAbs(a) ((a)>0 ? (a) : -(a))

#define VIDEO_STREAM_BUFF_DURATION (5)

#define FAST_MODE_DROP_TIME (71*1000)

#define DISPLAY_UCI_CONFIG_PATH "/etc/config/system"

static inline int64_t GetCurrentTime(void)
{
    struct timeval tv;
    int64_t time;
    gettimeofday(&tv,NULL);
    time = tv.tv_sec*1000000LL + tv.tv_usec;
    return time;
}

typedef struct PlayerContext
{
    VideoStreamInfo*            pVideoStreamInfo;
    AudioStreamInfo*            pAudioStreamInfo;
    int                         nAudioStreamNum;
    int                         nAudioStreamSelected;
    LayerCtrl*                  pLayerCtrl;
    SoundCtrl*                  pAudioSink;

    VideoRenderComp*            pVideoRender;
    AudioRenderComp*            pAudioRender;
    VideoDecComp*               pVideoDecComp;
    AudioDecComp*               pAudioDecComp;
    AvTimer*                    pAvTimer;

    XAudioPlaybackRate          nPlayRate;

    enum EPLAYERSTATUS          eStatus;

    PlayerCallback              callback;
    void*                       pUserData;

    //* buffer for crashed media stream.
#if CONFIG_DISABLE_VIDEO
    void*                       pVideoStreamBuffer;
    int                         nVideoStreamBufferSize;
#endif
#if CONFIG_DISABLE_AUDIO
    void*                       pAudioStreamBuffer;
    int                         nAudioStreamBufferSize;
#endif

#if !defined(CONF_3D_ENABLE)
    void*                       pVideoSecondStreamBuffer;
    int                         nVideoSecondStreamBufferSize;
#endif
    int                         bProcessingCommand;
    int64_t                     nFirstVideoRenderPts;
    int64_t                     nFirstAudioRenderPts;
    int64_t                     nTimeBase;
    int64_t                     nTimeOffset;
    pthread_mutex_t             timerMutex;
    int64_t                     nLastTimeTimerAdjusted;
    int onResetNotSync;
    int64_t ptsBeforeReset;

    int64_t                     avs_policy; /* 0: video_pts, 1: audio_pts */
    int64_t                     nPreVideoPts;
    int64_t                     nPreAudioPts;
    int                         nUnsyncVideoFrameCnt;
    //* for pts loop back detect in subtitle pts notify process.
    int64_t                     nLastInputPts;
    int64_t                     nFirstPts;

    int                         bStreamEosSet;
    int                         bVideoRenderEosReceived;
    int                         bAudioRenderEosReceived;
    pthread_mutex_t             eosMutex;

    int                         bInFastMode;
    VConfig                     sVideoConfig;
    int                         sVideoCropWindow[4];

    //* audio decoder mutex, currently for synchronization of the switch audio
    //* operation and the demux sending job.
    pthread_mutex_t             audioDecoderMutex;

    pthread_mutex_t             playerStatusMutex;

    int                         bVideoCrash;
    int                         bAudioCrash;

    //*
    int                         bUnSurpportVideoFlag;
    void*                       pUnSurpportVideoBuffer;
    int                         nUnSurpportVideoBufferSize;

    int                         bDiscardAudio;
    int64_t                     mNeedDorpAudioTime;
    int64_t                     mAudioJumpTime[3];
    int                         nAvgByteRate;
    int                         nDefaultAvSpeed;
    int64_t                     nPtsList[5];
    int64_t                     nPtsGetTime[5];
    int                         nPtsPos;
    int64_t                     nAPtsSecs;
    int64_t                     nVPtsSecs;
    int                         nTimeDiffCount;
}PlayerContext;

static int CallbackProcess(void* pSelf, int eMessageId, void* param);
static int PlayerInitialVideo(PlayerContext* p);
static int PlayerInitialAudio(PlayerContext* p);

Player* WFD_PlayerCreate(void)
{
    PlayerContext* p;

	CDX_LOGI("WFD_PlayerCreate.");
    p = (PlayerContext*)malloc(sizeof(PlayerContext));
    if(p == NULL)
    {
        CDX_LOGE("memory alloc fail.");
        return NULL;
    }
    memset(p, 0, sizeof(*p));

    p->nPlayRate.mSpeed = 1.0f;
    p->nPlayRate.mPitch= 1.0f;

    p->eStatus = PLAYER_STATUS_STOPPED;
    p->pAvTimer = WFD_AvTimerCreate();
    p->onResetNotSync = 0;
    pthread_mutex_init(&p->eosMutex, NULL);
    pthread_mutex_init(&p->timerMutex, NULL);
    pthread_mutex_init(&p->audioDecoderMutex, NULL);
    pthread_mutex_init(&p->playerStatusMutex, NULL);

#if CONFIG_DISABLE_VIDEO
    p->nVideoStreamBufferSize = 256*1024;
    p->pVideoStreamBuffer = malloc(p->nVideoStreamBufferSize);
    if(p->pVideoStreamBuffer == NULL)
    {
        CDX_LOGE("allocate memory fail.");
        return NULL;
    }
#endif
#if CONFIG_DISABLE_AUDIO
    p->nAudioStreamBufferSize = 128*1024;
    p->pAudioStreamBuffer = malloc(p->nAudioStreamBufferSize);
    if(p->pAudioStreamBuffer == NULL)
    {
        CDX_LOGE("allocate memory fail.");
        return NULL;
    }
#endif

#if !defined(CONF_3D_ENABLE)
    p->nVideoSecondStreamBufferSize = 256*1024;
    p->pVideoSecondStreamBuffer     = malloc(p->nVideoSecondStreamBufferSize);
    if(p->pVideoSecondStreamBuffer == NULL)
    {
        CDX_LOGE("allocate memory fail.");
        return NULL;
    }
#endif

#if !defined(PLATFORM_DOLPHIN)
    p->sVideoConfig.commonConfigFlag = IS_MIRACAST_STREAM;
#endif
    p->sVideoConfig.bDispErrorFrame = 1;   // set the default value of the bDispErrorFrame is 1
    p->nDefaultAvSpeed = 1000;

#ifdef DEBUG_SAVE_VES
	dbg_wfd_ves_fp = fopen(dbg_wfd_ves, "wb");
#endif
    return (Player*)p;
}

void WFD_PlayerDestroy(Player* pl)
{
    PlayerContext* p;

    p = (PlayerContext*)pl;

    WFD_PlayerClear(pl);

    if(p->pAvTimer != NULL)
    {
        WFD_AvTimerDestroy(p->pAvTimer);
        p->pAvTimer = NULL;
    }

    pthread_mutex_destroy(&p->eosMutex);
    pthread_mutex_destroy(&p->timerMutex);
    pthread_mutex_destroy(&p->audioDecoderMutex);
    pthread_mutex_destroy(&p->playerStatusMutex);

#if CONFIG_DISABLE_VIDEO
    if(p->pVideoStreamBuffer != NULL)
    {
        free(p->pVideoStreamBuffer);
        p->pVideoStreamBuffer = NULL;
    }
#endif
#if CONFIG_DISABLE_AUDIO
    if(p->pAudioStreamBuffer != NULL)
    {
        free(p->pAudioStreamBuffer);
        p->pAudioStreamBuffer = NULL;
    }
#endif

#if !defined(CONF_3D_ENABLE)
    if(p->pVideoSecondStreamBuffer != NULL)
    {
        free(p->pVideoSecondStreamBuffer);
        p->pVideoSecondStreamBuffer = NULL;
    }
#endif

    if(p->pUnSurpportVideoBuffer != NULL)
    {
        free(p->pUnSurpportVideoBuffer);
        p->pUnSurpportVideoBuffer = NULL;
    }

    // destroy the output
    if(p->pLayerCtrl)
    {
        LayerDestroy(p->pLayerCtrl);
        p->pLayerCtrl = NULL;
    }
    if(p->pAudioSink != NULL)
    {
        SoundDeviceDestroy(p->pAudioSink);
        p->pAudioSink = NULL;
    }

    free(p);
    
#ifdef DEBUG_SAVE_VES
		if(dbg_wfd_ves_fp)
		{
			fclose(dbg_wfd_ves_fp);
			dbg_wfd_ves_fp = NULL;
		}
#endif

    return;
}

int WFD_PlayerSetVideoStreamInfo(Player* pl, VideoStreamInfo* pStreamInfo)
{
    PlayerContext* p;
	
	CDX_LOGI("step...");

    p = (PlayerContext*)pl;
    pthread_mutex_lock(&p->playerStatusMutex);

#if CONFIG_DISABLE_VIDEO
    pthread_mutex_unlock(&p->playerStatusMutex);
    return 0;
#else
    if(p->pVideoStreamInfo != NULL)
    {
        if(p->pVideoStreamInfo->pCodecSpecificData != NULL &&
           p->pVideoStreamInfo->nCodecSpecificDataLen > 0)
        {
            free(p->pVideoStreamInfo->pCodecSpecificData);
            p->pVideoStreamInfo->pCodecSpecificData = NULL;
            p->pVideoStreamInfo->nCodecSpecificDataLen = 0;
        }

        free(p->pVideoStreamInfo);
        p->pVideoStreamInfo = NULL;
    }

    if(pStreamInfo)
    {
        p->pVideoStreamInfo = (VideoStreamInfo*)malloc(sizeof(VideoStreamInfo));
        if(p->pVideoStreamInfo == NULL)
        {
            CDX_LOGE("malloc memory fail.");
            pthread_mutex_unlock(&p->playerStatusMutex);
            return -1;
        }

        memcpy(p->pVideoStreamInfo, pStreamInfo, sizeof(VideoStreamInfo));
        p->pVideoStreamInfo->bIsFramePackage = 1;
        if(pStreamInfo->pCodecSpecificData != NULL &&
           pStreamInfo->nCodecSpecificDataLen > 0)
        {
            p->pVideoStreamInfo->pCodecSpecificData =
                (char*)malloc(pStreamInfo->nCodecSpecificDataLen);
            if(p->pVideoStreamInfo->pCodecSpecificData == NULL)
            {
                CDX_LOGE("malloc memory fail.");
                free(p->pVideoStreamInfo);
                p->pVideoStreamInfo = NULL;
                pthread_mutex_unlock(&p->playerStatusMutex);
                return -1;
            }
            memcpy(p->pVideoStreamInfo->pCodecSpecificData,
                   pStreamInfo->pCodecSpecificData,
                   pStreamInfo->nCodecSpecificDataLen);
        }
    }

    if(PlayerInitialVideo(p) == 0)
    {
        if(p->eStatus != PLAYER_STATUS_STOPPED)
        {
            WFD_VideoRenderCompStart(p->pVideoRender);
            if(p->eStatus == PLAYER_STATUS_PAUSED)
                WFD_VideoRenderCompPause(p->pVideoRender);
            WFD_VideoDecCompStart(p->pVideoDecComp);
            if(p->eStatus == PLAYER_STATUS_PAUSED)
                WFD_VideoDecCompPause(p->pVideoDecComp);
        }
        pthread_mutex_unlock(&p->playerStatusMutex);
        return 0;
    }
    else
    {
    	
        pthread_mutex_unlock(&p->playerStatusMutex);
        return -1;
    }
#endif
}

int WFD_PlayerSetAudioStreamInfo(Player* pl, AudioStreamInfo* pStreamInfo,
        int nStreamNum, int nDefaultStream)
{
    PlayerContext* p;
    int            i;

    p = (PlayerContext*)pl;

    if(nStreamNum == 0)
    {
        CDX_LOGE("have no audio stream, wouldn't init audio player");
        return 0;
    }

#if CONFIG_DISABLE_AUDIO
    return 0;
#else
    if(p->pAudioStreamInfo != NULL && p->nAudioStreamNum > 0)
    {
        for(i=0; i<p->nAudioStreamNum; i++)
        {
            if(p->pAudioStreamInfo[i].pCodecSpecificData != NULL &&
               p->pAudioStreamInfo[i].nCodecSpecificDataLen > 0)
            {
                free(p->pAudioStreamInfo[i].pCodecSpecificData);
                p->pAudioStreamInfo[i].pCodecSpecificData = NULL;
                p->pAudioStreamInfo[i].nCodecSpecificDataLen = 0;
            }
        }

        free(p->pAudioStreamInfo);
        p->pAudioStreamInfo = NULL;
    }

    p->nAudioStreamNum = 0;
    p->nAudioStreamSelected = 0;

    if(pStreamInfo != NULL && nStreamNum > 0)
    {
        p->pAudioStreamInfo = (AudioStreamInfo*)malloc(sizeof(AudioStreamInfo) * nStreamNum);
        if(p->pAudioStreamInfo == NULL)
        {
            CDX_LOGE("malloc memory fail.");
            return -1;
        }

        memcpy(p->pAudioStreamInfo, pStreamInfo, sizeof(AudioStreamInfo)*nStreamNum);

        for(i=0; i<nStreamNum; i++)
        {
            if(pStreamInfo[i].pCodecSpecificData != NULL &&
               pStreamInfo[i].nCodecSpecificDataLen > 0)
            {
                p->pAudioStreamInfo[i].pCodecSpecificData =
                    (char*)malloc(pStreamInfo[i].nCodecSpecificDataLen);
                if(p->pAudioStreamInfo[i].pCodecSpecificData == NULL)
                {
                    CDX_LOGE("malloc memory fail.");
                    break;
                }
                memcpy(p->pAudioStreamInfo[i].pCodecSpecificData,
                       pStreamInfo[i].pCodecSpecificData,
                       pStreamInfo[i].nCodecSpecificDataLen);
            }
        }

        if(i != nStreamNum)
        {
            i--;
            for(; i>=0; i--)
            {
                if(p->pAudioStreamInfo[i].pCodecSpecificData != NULL &&
                   p->pAudioStreamInfo[i].nCodecSpecificDataLen > 0)
                {
                    free(p->pAudioStreamInfo[i].pCodecSpecificData);
                    p->pAudioStreamInfo[i].pCodecSpecificData = NULL;
                    p->pAudioStreamInfo[i].nCodecSpecificDataLen = 0;
                }
            }

            free(p->pAudioStreamInfo);
            p->pAudioStreamInfo = NULL;
            return -1;
        }

        p->nAudioStreamSelected = nDefaultStream;
        p->nAudioStreamNum = nStreamNum;
    }

    if(PlayerInitialAudio(p) == 0)
    {
        if(p->eStatus != PLAYER_STATUS_STOPPED)
        {
            WFD_AudioRenderCompStart(p->pAudioRender);
            if(p->eStatus == PLAYER_STATUS_PAUSED)
                WFD_AudioRenderCompPause(p->pAudioRender);
            WFD_AudioDecCompStart(p->pAudioDecComp);
            if(p->eStatus == PLAYER_STATUS_PAUSED)
                WFD_AudioDecCompPause(p->pAudioDecComp);
        }
        return 0;
    }
    else
    {
        return -1;
    }
#endif
}


int WFD_PlayerSetWindow(Player* pl, LayerCtrl* pLc)
{
    PlayerContext* p;

	CDX_LOGI("step...");
    p = (PlayerContext*)pl;

    if(p->pVideoRender != NULL)
    {
        WFD_VideoRenderCompSetWindow(p->pVideoRender, pLc);
    }

    p->pLayerCtrl = pLc;

    return 0;
}

int WFD_PlayerSetAudioSink(Player* pl, SoundCtrl* pAudioSink)
{
    PlayerContext* p;

    p = (PlayerContext*)pl;

    p->pAudioSink = pAudioSink;
    if(p->pAudioRender != NULL)
    {
        return WFD_AudioRenderCompSetAudioSink(p->pAudioRender, pAudioSink);
    }

    return 0;
}

int WFD_PlayerSetCallback(Player* pl, PlayerCallback callback, void* pUserData)
{
    PlayerContext* p;

    p = (PlayerContext*)pl;
    p->callback  = callback;
    p->pUserData = pUserData;
    return 0;
}

int WFD_PlayerHasVideo(Player* pl)
{
    PlayerContext* p;

    p = (PlayerContext*)pl;
    if(p->pVideoRender != NULL)
        return 1;
    else
        return 0;
}

int WFD_PlayerHasAudio(Player* pl)
{
    PlayerContext* p;

    p = (PlayerContext*)pl;
    if(p->pAudioRender != NULL)
        return 1;
    else
        return 0;
}

int WFD_PlayerStart(Player* pl)
{
    PlayerContext* p;

    p = (PlayerContext*)pl;
    pthread_mutex_lock(&p->playerStatusMutex);

    CDX_LOGD("player start");

    if(p->eStatus == PLAYER_STATUS_STARTED)
    {
        pthread_mutex_unlock(&p->playerStatusMutex);
        CDX_LOGE("invalid start operation, player already in started status.");
        return -1;
    }

#if 0
    // 1. get degree from uci
    char degree[4]  = "";
    if (uci_get_config("degree", degree, sizeof(degree), DISPLAY_UCI_CONFIG_PATH, "setting") != -1)
        WFD_PlayerSetRotateDegree((Player*)p, atoi(degree));
    char fullScreen[4] = "";
    if (uci_get_config("full_screen", fullScreen, sizeof(fullScreen), DISPLAY_UCI_CONFIG_PATH, "display") != -1)
        WFD_PlayerSetFullScreen((Player*)p, atoi(fullScreen));
#endif
    p->bProcessingCommand = 1;

    if(p->eStatus == PLAYER_STATUS_PAUSED)
    {
        if(p->bInFastMode)
        {
            //* set timer to the max value to make the audio render to discard all data.
            p->pAvTimer->SetTime(p->pAvTimer, 0x7000000000000000LL);
        }
        else
        {
            if(p->nTimeBase != -1)
                p->pAvTimer->Start(p->pAvTimer);
        }

        //* resume components.
        if(p->pVideoDecComp != NULL)
            WFD_VideoDecCompStart(p->pVideoDecComp);
        if(p->pAudioDecComp != NULL)
            WFD_AudioDecCompStart(p->pAudioDecComp);
        if(p->pVideoRender != NULL)
            WFD_VideoRenderCompStart(p->pVideoRender);
        if(p->pAudioRender != NULL)
            WFD_AudioRenderCompStart(p->pAudioRender);
    }
    else
    {
        if((p->pVideoDecComp == NULL || p->pVideoRender == NULL) &&
           (p->pAudioDecComp == NULL || p->pAudioRender == NULL))
        {
            pthread_mutex_unlock(&p->playerStatusMutex);
            CDX_LOGE("neither video nor audio stream exits.");
            return -1;
        }

        p->nFirstVideoRenderPts    = -1;
        p->nFirstAudioRenderPts    = -1;
        p->bStreamEosSet           = 0;
        p->bVideoRenderEosReceived = 0;
        p->bAudioRenderEosReceived = 0;
        p->nLastTimeTimerAdjusted  = 0;
        p->nPreVideoPts            = -1;
        p->nPreAudioPts            = -1;
        p->nUnsyncVideoFrameCnt    = 0;
        p->nLastInputPts           = -1;

        //* nTimeBase==-1 means timer will be started at the first pts callback.
        p->nTimeBase               = -1;
        p->nTimeOffset             = 0;

        p->pAvTimer->Stop(p->pAvTimer);
        if(p->bInFastMode)
        {
            /* set timer to a big value(but not easy to loop back) to make the
             * audio render to discard all data.
             */
            p->pAvTimer->SetTime(p->pAvTimer, 0x7000000000000000LL);
        }
        else
            p->pAvTimer->SetTime(p->pAvTimer, 0);
        p->pAvTimer->SetSpeed(p->pAvTimer, p->nDefaultAvSpeed);

        /* start components.
         * must start the decoder components first, because render component
         * thread will call methods like WFD_AudioDecCompRequestPcmData() which
         * need access audio or subtitle decoders inside decoder components, if
         * we start the render components first, decoders are not created yet
         * and this will cause the render thread crash when calling methods
         * like WFD_AudioDecCompRequestPcmData().
         */
        if(p->pVideoDecComp != NULL)
            WFD_VideoDecCompStart(p->pVideoDecComp);
        if(p->pAudioDecComp != NULL)
            WFD_AudioDecCompStart(p->pAudioDecComp);
        if(p->pVideoRender != NULL)
            WFD_VideoRenderCompStart(p->pVideoRender);
        if(p->pAudioRender != NULL)
            WFD_AudioRenderCompStart(p->pAudioRender);
    }
    p->mNeedDorpAudioTime = 0;
    p->mAudioJumpTime[0]  = 0;
    p->mAudioJumpTime[1]  = 0;
    p->mAudioJumpTime[2]  = 0;
    p->bProcessingCommand = 0;
    p->eStatus = PLAYER_STATUS_STARTED;
    pthread_mutex_unlock(&p->playerStatusMutex);

    return 0;
}

int WFD_PlayerStop(Player* pl)      //* media stream information is still kept by the player.
{
    PlayerContext* p;
    CDX_LOGD("****** WFD_PlayerStop");
    p = (PlayerContext*)pl;

    if(p->eStatus == PLAYER_STATUS_STOPPED)
    {
        CDX_LOGE("invalid stop operation, player already in stopped status.");
        return -1;
    }

    p->bProcessingCommand = 1;

    /* stop components.
     * must stop the render components first, because render component thread
     * will call methods like WFD_AudioDecCompRequestPcmData() which need access
     * audio or subtitle decoders inside decoder components, if we stop the
     * decoder components first, decoders are destroyed and this will cause the
     * render thread crash when calling methods like
     * WFD_AudioDecCompRequestPcmData().
     */
    if(p->pAudioRender != NULL)
        WFD_AudioRenderCompStop(p->pAudioRender);
    if(p->pVideoRender != NULL)
        WFD_VideoRenderCompStop(p->pVideoRender);
    if(p->pAudioDecComp != NULL)
        WFD_AudioDecCompStop(p->pAudioDecComp);
    if(p->pVideoDecComp != NULL)
        WFD_VideoDecCompStop(p->pVideoDecComp);

    CDX_LOGD("****** WFD_PlayerStop end");

    p->pAvTimer->Stop(p->pAvTimer);
    p->pAvTimer->SetTime(p->pAvTimer, 0);

    p->bProcessingCommand = 0;
    p->bInFastMode        = 0;
    p->eStatus = PLAYER_STATUS_STOPPED;
    p->bVideoCrash = 0;
    p->bAudioCrash = 0;
    p->bDiscardAudio  = 0;

    return 0;
}

int WFD_PlayerPause(Player* pl)
{
    PlayerContext* p;

    p = (PlayerContext*)pl;

    if(p->eStatus != PLAYER_STATUS_STARTED)
    {
        CDX_LOGE("invalid pause operation, player not in started status.");
        return -1;
    }

    p->bProcessingCommand = 1;

    //* pause components.
    if(p->pVideoRender != NULL)
        WFD_VideoRenderCompPause(p->pVideoRender);
    if(p->pAudioRender != NULL)
        WFD_AudioRenderCompPause(p->pAudioRender);
    if(p->pVideoDecComp != NULL)
        WFD_VideoDecCompPause(p->pVideoDecComp);
    if(p->pAudioDecComp != NULL)
        WFD_AudioDecCompPause(p->pAudioDecComp);

    p->pAvTimer->Stop(p->pAvTimer);

    p->bProcessingCommand = 0;
    p->eStatus = PLAYER_STATUS_PAUSED;

    return 0;
}

enum EPLAYERSTATUS WFD_PlayerGetStatus(Player* pl)
{
    PlayerContext* p;
    int              ret;
    p = (PlayerContext*)pl;
    return p->eStatus;
}

//* for seek operation, mute be called under paused status.
int WFD_PlayerReset(Player* pl, int64_t nSeekTimeUs)
{
    PlayerContext* p;

    p = (PlayerContext*)pl;

    if(p->eStatus == PLAYER_STATUS_STOPPED)
    {
        CDX_LOGE("invalid reset operation, should be called under pause status.");
        return -1;
    }

    p->bProcessingCommand = 1;

    if(p->eStatus == PLAYER_STATUS_STARTED)
    {
        //* pause components.
        if(p->pVideoRender != NULL)
            WFD_VideoRenderCompPause(p->pVideoRender);
        if(p->pAudioRender != NULL)
            WFD_AudioRenderCompPause(p->pAudioRender);
        if(p->pVideoDecComp != NULL)
            WFD_VideoDecCompPause(p->pVideoDecComp);
        if(p->pAudioDecComp != NULL)
            WFD_AudioDecCompPause(p->pAudioDecComp);
    }

    //* reset components.
    if(p->pVideoDecComp != NULL)
        WFD_VideoDecCompReset(p->pVideoDecComp);
    if(p->pAudioDecComp != NULL)
        WFD_AudioDecCompReset(p->pAudioDecComp, nSeekTimeUs);
    if(p->pVideoRender != NULL)
        WFD_VideoRenderCompReset(p->pVideoRender);
    if(p->pAudioRender != NULL)
        WFD_AudioRenderCompReset(p->pAudioRender);

    p->pAvTimer->Stop(p->pAvTimer);
    p->pAvTimer->SetTime(p->pAvTimer, 0);

    p->nFirstVideoRenderPts    = -1;
    p->nFirstAudioRenderPts    = -1;
    p->bStreamEosSet           = 0;
    p->bVideoRenderEosReceived = 0;
    p->bAudioRenderEosReceived = 0;
    p->nLastTimeTimerAdjusted  = 0;
    p->nPreVideoPts            = -1;
    p->nPreAudioPts            = -1;
    p->nUnsyncVideoFrameCnt    = 0;
    p->nLastInputPts           = -1;
    p->bVideoCrash             = 0;
    p->bAudioCrash             = 0;
    p->mNeedDorpAudioTime      = 0;
    p->mAudioJumpTime[0]       = 0;
    p->mAudioJumpTime[1]       = 0;
    p->mAudioJumpTime[2]       = 0;

    //* nTimeBase==-1 means timer will be started at the first pts callback.
    p->nTimeBase               = -1;
    p->nTimeOffset             = nSeekTimeUs;


    p->bProcessingCommand = 0;

    if(p->eStatus == PLAYER_STATUS_STARTED)
    {
        /* start components.
         * must start the decoder components first, because render component
         * thread will call methods like WFD_AudioDecCompRequestPcmData() which
         * need access audio or subtitle decoders inside decoder components, if
         * we start the render components first, decoders are not created yet
         * and this will cause the render thread crash when calling methods
         * like WFD_AudioDecCompRequestPcmData().
         */
        if(p->pVideoRender != NULL)
            WFD_VideoRenderCompStart(p->pVideoRender);
        if(p->pAudioRender != NULL)
            WFD_AudioRenderCompStart(p->pAudioRender);
        if(p->pVideoDecComp != NULL)
            WFD_VideoDecCompStart(p->pVideoDecComp);
        if(p->pAudioDecComp != NULL)
            WFD_AudioDecCompStart(p->pAudioDecComp);
    }

    return 0;
}

//* must be called under stopped status, all stream information cleared.
int WFD_PlayerClear(Player* pl)
{
    PlayerContext* p;

    p = (PlayerContext*)pl;

    if(p->eStatus != PLAYER_STATUS_STOPPED)
    {
        CDX_LOGE("invalid clear operation, player is not in stopped status.");
        return -1;
    }

    p->bProcessingCommand      = 1;
    p->nFirstVideoRenderPts    = -1;
    p->nFirstAudioRenderPts    = -1;
    p->bStreamEosSet           = 0;
    p->bVideoRenderEosReceived = 0;
    p->bAudioRenderEosReceived = 0;
    p->bVideoCrash             = 0;
    p->bAudioCrash             = 0;
    p->sVideoCropWindow[0]     = 0;
    p->sVideoCropWindow[1]     = 0;
    p->sVideoCropWindow[2]     = 0;
    p->sVideoCropWindow[3]     = 0;

    if(p->pVideoRender != NULL)
    {
        WFD_VideoRenderCompDestroy(p->pVideoRender);
        p->pVideoRender = NULL;
    }

    if(p->pAudioRender != NULL)
    {
        WFD_AudioRenderCompDestroy(p->pAudioRender);
        p->pAudioRender = NULL;
    }

    if(p->pVideoDecComp != NULL)
    {
        WFD_VideoDecCompDestroy(p->pVideoDecComp);
        p->pVideoDecComp = NULL;
    }

    if(p->pAudioDecComp != NULL)
    {
        WFD_AudioDecCompDestroy(p->pAudioDecComp);
        p->pAudioDecComp = NULL;
    }

    p->bProcessingCommand = 0;


    if(p->pVideoStreamInfo != NULL)
    {
        if(p->pVideoStreamInfo->pCodecSpecificData != NULL &&
           p->pVideoStreamInfo->nCodecSpecificDataLen > 0)
        {
            free(p->pVideoStreamInfo->pCodecSpecificData);
            p->pVideoStreamInfo->pCodecSpecificData = NULL;
            p->pVideoStreamInfo->nCodecSpecificDataLen = 0;
        }

        free(p->pVideoStreamInfo);
        p->pVideoStreamInfo = NULL;
    }

    if(p->pAudioStreamInfo != NULL && p->nAudioStreamNum > 0)
    {
        int i;
        for(i=0; i<p->nAudioStreamNum; i++)
        {
            if(p->pAudioStreamInfo[i].pCodecSpecificData != NULL &&
               p->pAudioStreamInfo[i].nCodecSpecificDataLen > 0)
            {
                free(p->pAudioStreamInfo[i].pCodecSpecificData);
                p->pAudioStreamInfo[i].pCodecSpecificData = NULL;
                p->pAudioStreamInfo[i].nCodecSpecificDataLen = 0;
            }
        }

        free(p->pAudioStreamInfo);
        p->pAudioStreamInfo = NULL;
    }

    return 0;
}

int WFD_PlayerRequestStreamBuffer(Player*         pl,
                              int             nRequireSize,
                              void**          ppBuf,
                              int*            pBufSize,
                              void**          ppRingBuf,
                              int*            pRingBufSize,
                              enum EMEDIATYPE eMediaType,
                              int             nStreamIndex)
{
    PlayerContext* p;
    int            ret;

    p = (PlayerContext*)pl;

    //CDX_LOGD("request stream buffer, eMediaType = %d, require size = %d", eMediaType, nRequireSize);

    *ppBuf          = NULL;
    *pBufSize       = 0;
    *ppRingBuf      = NULL;
    *pRingBufSize   = 0;

    if(p->eStatus == PLAYER_STATUS_STOPPED)
    {
        CDX_LOGE("can not request stream buffer at stopped status.");
        return -1;
    }

    if(eMediaType == MEDIA_TYPE_VIDEO)
    {
        if(p->bUnSurpportVideoFlag == 1)
        {
            if(nRequireSize > p->nUnSurpportVideoBufferSize)
                nRequireSize = p->nUnSurpportVideoBufferSize;
            *pBufSize = nRequireSize;
            *ppBuf = p->pUnSurpportVideoBuffer;
            return 0;
        }

#if CONFIG_DISABLE_VIDEO
        if(nRequireSize > p->nVideoStreamBufferSize)
            nRequireSize = p->nVideoStreamBufferSize;
        *pBufSize = nRequireSize;
        *ppBuf = p->pVideoStreamBuffer;
        return 0;
#else

#if !defined(CONF_3D_ENABLE)
        if(nStreamIndex == 1)
        {
            if(nRequireSize > p->nVideoSecondStreamBufferSize)
                nRequireSize = p->nVideoSecondStreamBufferSize;
            *pBufSize = nRequireSize;
            *ppBuf    = p->pVideoSecondStreamBuffer;
            return 0;
        }
#endif

        if(p->pVideoDecComp == NULL)
        {
            CDX_LOGE("no video decode component, can not request video stream buffer.");
            return -1;
        }

        ret = WFD_VideoDecCompRequestStreamBuffer(p->pVideoDecComp,
                                              nRequireSize,
                                              (char**)ppBuf,
                                              pBufSize,
                                              (char**)ppRingBuf,
                                              pRingBufSize,
                                              nStreamIndex);

        return ret;
#endif
    }
    else if(eMediaType == MEDIA_TYPE_AUDIO)
    {
#if CONFIG_DISABLE_AUDIO
        if(nRequireSize > p->nAudioStreamBufferSize)
            nRequireSize = p->nAudioStreamBufferSize;
        *pBufSize = nRequireSize;
        *ppBuf = p->pAudioStreamBuffer;
        return 0;
#else
        pthread_mutex_lock(&p->audioDecoderMutex);

        if(p->pAudioDecComp == NULL)
        {
            CDX_LOGE("no audio decode component, can not request audio stream buffer.");
            pthread_mutex_unlock(&p->audioDecoderMutex);
            return -1;
        }

        ret = WFD_AudioDecCompRequestStreamBuffer(p->pAudioDecComp,
                                              nRequireSize,
                                              (char**)ppBuf,
                                              pBufSize,
                                              (char**)ppRingBuf,
                                              pRingBufSize,
                                              nStreamIndex);

        pthread_mutex_unlock(&p->audioDecoderMutex);
        return ret;
#endif
    }
    else
    {
        return 0;
    }
}

int WFD_PlayerSubmitStreamData(Player*              pl,
                           MediaStreamDataInfo* pDataInfo,
                           enum EMEDIATYPE      eMediaType,
                           int                  nStreamIndex)
{
    PlayerContext* p;
    int            ret;

    p = (PlayerContext*)pl;

    if(eMediaType==0){
        int64_t ptsSecs = (int)(pDataInfo->nPts/1000000);
        if (p->nVPtsSecs <= ptsSecs-10)//print every 10s
        {
            p->nVPtsSecs = ptsSecs;
            CDX_LOGW("submit %s stream data , pts is %.3f ms",eMediaType==0?"+++video+++":"---audio---",pDataInfo->nPts/1000.0);
        }
    }else{
        int64_t ptsSecs = (int)(pDataInfo->nPts/1000000);
        if (p->nAPtsSecs <= ptsSecs-10)//print every 10s
        {
            p->nAPtsSecs = ptsSecs;
            CDX_LOGW("submit %s stream data , pts is %.3f ms",eMediaType==0?"+++video+++":"---audio---",pDataInfo->nPts/1000.0);
        }
    }


//    CDX_LOGD("submit stream data, eMediaType = %d", eMediaType);

    if(p->eStatus == PLAYER_STATUS_STOPPED)
    {
        CDX_LOGE("can not submit stream data at stopped status.");
        return -1;
    }

    if(pDataInfo->nPts != -1)
        p->nLastInputPts = pDataInfo->nPts; //* save the last input pts.

    if(eMediaType == MEDIA_TYPE_VIDEO)
    {
        if(p->bUnSurpportVideoFlag == 1)
            return 0;


#if CONFIG_DISABLE_VIDEO
        return 0;
#else

#if !defined(CONF_3D_ENABLE)
            if(nStreamIndex == 1)
                return 0;
#endif
        VideoStreamDataInfo dataInfo;
        memset(&dataInfo, 0, sizeof(VideoStreamDataInfo));

        if(p->pVideoDecComp == NULL)
        {
            CDX_LOGE("no video decode component, can not submit video stream data.");
            return -1;
        }
        p->nPtsList[p->nPtsPos] = pDataInfo->nPts;
        p->nPtsGetTime[p->nPtsPos] = GetCurrentTime();
        p->nPtsPos++;
        if(p->nPtsPos >= 5)
            p->nPtsPos = 0;
//        CDX_LOGD("submit video data, pts = %lld ms, curTime = %lld ms", pDataInfo->nPts/1000,
//            p->pAvTimer->GetTime(p->pAvTimer)/1000);


#ifdef DEBUG_SAVE_VES
	   fwrite(pDataInfo->pData, 1, pDataInfo->nLength, dbg_wfd_ves_fp);
#endif //DEBUG_SAVE_BITSTREAM

        //* process the video resolution change
        if(pDataInfo->nStreamChangeFlag == 1)
        {
            if(pDataInfo->nStreamChangeNum != 1)
            {
                CDX_LOGE("the videoNum is not 1,");
                abort();
            }

            VideoStreamInfo* pVideoChangeInfo = (VideoStreamInfo*)pDataInfo->pStreamInfo;
            VideoStreamInfo* pVideoStreamInfo   = (VideoStreamInfo*)malloc(sizeof(VideoStreamInfo));
            memset(pVideoStreamInfo,0,sizeof(VideoStreamInfo));

            pVideoStreamInfo->eCodecFormat    = pVideoChangeInfo->eCodecFormat;
            pVideoStreamInfo->nWidth          = pVideoChangeInfo->nWidth;
            pVideoStreamInfo->nHeight         = pVideoChangeInfo->nHeight;
            pVideoStreamInfo->nFrameRate      = pVideoChangeInfo->nFrameRate;
            pVideoStreamInfo->nFrameDuration  = pVideoChangeInfo->nFrameDuration;
            pVideoStreamInfo->nAspectRatio    = pVideoChangeInfo->nAspectRatio;
            pVideoStreamInfo->bIs3DStream     = pVideoChangeInfo->bIs3DStream;
            pVideoStreamInfo->bIsFramePackage = pVideoChangeInfo->bIsFramePackage;

            if(pVideoChangeInfo->nCodecSpecificDataLen > 0)
            {
                pVideoStreamInfo->pCodecSpecificData =
                    (char*)malloc(pVideoChangeInfo->nCodecSpecificDataLen);
                if(pVideoStreamInfo->pCodecSpecificData == NULL)
                {
                    CDX_LOGE("malloc pCodecSpecificData failed");
                    free(pVideoChangeInfo->pCodecSpecificData);
                    free(pVideoChangeInfo);
                    return -1;
                }

                memcpy(pVideoStreamInfo->pCodecSpecificData,pVideoChangeInfo->pCodecSpecificData,
                       pVideoChangeInfo->nCodecSpecificDataLen);
                pVideoStreamInfo->nCodecSpecificDataLen = pVideoChangeInfo->nCodecSpecificDataLen;

                free(pVideoChangeInfo->pCodecSpecificData);
                pVideoChangeInfo->pCodecSpecificData = NULL;
            }
            else
            {
                pVideoStreamInfo->pCodecSpecificData    = NULL;
                pVideoStreamInfo->nCodecSpecificDataLen = 0;
            }

            free(pVideoChangeInfo);
            pDataInfo->pStreamInfo = NULL;

            dataInfo.bVideoInfoFlag = 1;
            dataInfo.pVideoInfo     = pVideoStreamInfo;
        }
        else
        {
            dataInfo.pVideoInfo     = NULL;
            dataInfo.bVideoInfoFlag = 0;
        }

        dataInfo.pData        = pDataInfo->pData;
        dataInfo.nLength      = pDataInfo->nLength;
        dataInfo.nPts         = pDataInfo->nPts;
        dataInfo.nPcr         = pDataInfo->nPcr;
        dataInfo.bIsFirstPart = pDataInfo->bIsFirstPart;
        dataInfo.bIsLastPart  = pDataInfo->bIsLastPart;
        dataInfo.nStreamIndex = nStreamIndex;
        dataInfo.bValid = 1;

        ret = WFD_VideoDecCompSubmitStreamData(p->pVideoDecComp, &dataInfo, nStreamIndex);
        return ret;
#endif
    }
    else if(eMediaType == MEDIA_TYPE_AUDIO)
    {
#if CONFIG_DISABLE_AUDIO
        return 0;
#else
        AudioStreamDataInfo dataInfo;

        pthread_mutex_lock(&p->audioDecoderMutex);

        if(p->pAudioDecComp == NULL)
        {
            CDX_LOGE("no audio decode component, can not submit audio stream data.");
            pthread_mutex_unlock(&p->audioDecoderMutex);
            return -1;
        }

        CDX_LOGV("submit audio data, pts = %lld ms, curTime = %lld ms", pDataInfo->nPts/1000,
            p->pAvTimer->GetTime(p->pAvTimer)/1000);

        //* process the video resolution change.
        //* now, we just free the buffer , not to process it really
        if(pDataInfo->nStreamChangeFlag == 1)
        {
            int i;
            AudioStreamInfo* pAudioChangeInfo = (AudioStreamInfo*)pDataInfo->pStreamInfo;
            for(i = 0; i < pDataInfo->nStreamChangeNum; i++)
            {
                if(pAudioChangeInfo[i].pCodecSpecificData != NULL)
                    free(pAudioChangeInfo[i].pCodecSpecificData);
            }
            free(pAudioChangeInfo);
        }

        dataInfo.pData        = pDataInfo->pData;
        dataInfo.nLength      = pDataInfo->nLength;
        dataInfo.nPts         = pDataInfo->nPts;
        dataInfo.nPcr         = pDataInfo->nPcr;
        dataInfo.nPcr         = pDataInfo->nPcr;
        dataInfo.bIsFirstPart = pDataInfo->bIsFirstPart;
        dataInfo.bIsLastPart  = pDataInfo->bIsLastPart;

        ret = WFD_AudioDecCompSubmitStreamData(p->pAudioDecComp, &dataInfo, nStreamIndex);
        pthread_mutex_unlock(&p->audioDecoderMutex);

        return ret;
#endif
	}
    else
    {
		return 0;
    }
}

//* how much video stream data has been buffered.
int WFD_PlayerGetVideoStreamDataSize(Player* pl)
{
    PlayerContext* p;

    p = (PlayerContext*)pl;
    if(p->pVideoDecComp != NULL)
        return WFD_VideoDecCompStreamDataSize(p->pVideoDecComp, 0);
    else
        return 0;
}

//* how many stream frame in buffer.
int WFD_PlayerGetVideoStreamFrameNum(Player* pl)
{
    PlayerContext* p;

    p = (PlayerContext*)pl;
    if(p->pVideoDecComp != NULL)
        return WFD_VideoDecCompStreamFrameNum(p->pVideoDecComp, 0);
    else
        return 0;
}

//* how much audio pcm data has been decoded to the pcm buffer.
int WFD_PlayerGetAudioPcmDataSize(Player* pl)
{
    PlayerContext* p;

    p = (PlayerContext*)pl;
    if(p->pAudioDecComp != NULL)
        return WFD_AudioDecCompPcmDataSize(p->pAudioDecComp, p->nAudioStreamSelected);
    else
        return 0;
}

int WFD_PlayerStopAudio(Player* pl)
{
    PlayerContext* p;

    p = (PlayerContext*)pl;
    if(p->pAudioRender == NULL || p->pAudioDecComp == NULL)
    {
        CDX_LOGW("no audio decoder or render, switch audio fail.");
        return -1;
    }

    pthread_mutex_lock(&p->audioDecoderMutex);

    p->bAudioCrash = 0;

    //* 1. stop the audio render.
    WFD_AudioRenderCompStop(p->pAudioRender);

    //* 2. stop the audio decoder.
    WFD_AudioDecCompStop(p->pAudioDecComp);
    pthread_mutex_unlock(&p->audioDecoderMutex);

    return 0;
}
int WFD_PlayerStartAudio(Player* pl)
{
    PlayerContext* p;

    p = (PlayerContext*)pl;
    if(p->pAudioRender == NULL || p->pAudioDecComp == NULL)
    {
        CDX_LOGW("no audio decoder or render, switch audio fail.");
        return -1;
    }

    pthread_mutex_lock(&p->audioDecoderMutex);

    if(p->eStatus != PLAYER_STATUS_STOPPED)
    {
        //* 4. start the audio decoder and render.
        WFD_AudioDecCompStart(p->pAudioDecComp);
        WFD_AudioRenderCompStart(p->pAudioRender);

        if(p->eStatus == PLAYER_STATUS_PAUSED)
        {
            //* 5. pause the audio decoder and render.
            WFD_AudioRenderCompPause(p->pAudioRender);
            WFD_AudioDecCompPause(p->pAudioDecComp);
        }
    }

    if(p->bStreamEosSet)
    {
        p->bAudioRenderEosReceived = 0;
        WFD_AudioDecCompSetEOS(p->pAudioDecComp);
    }

    pthread_mutex_unlock(&p->audioDecoderMutex);

    return 0;
}

static int CallbackProcess(void* pSelf, int eMessageId, void* param)
{
    PlayerContext* p;

    p = (PlayerContext*)pSelf;

    switch(eMessageId)
    {
        case PLAYER_VIDEO_DECODER_NOTIFY_EOS:
        {
            if(p->pVideoRender != NULL)
                WFD_VideoRenderCompSetEOS(p->pVideoRender);
            return 0;
        }

        case PLAYER_AUDIO_DECODER_NOTIFY_EOS:
        {
            if(p->pAudioRender != NULL)
                WFD_AudioRenderCompSetEOS(p->pAudioRender);
            return 0;
        }

        case PLAYER_VIDEO_RENDER_NOTIFY_EOS:
        {
            pthread_mutex_lock(&p->eosMutex);
            p->bVideoRenderEosReceived = 1;
            if(p->bAudioRenderEosReceived == 1 || WFD_PlayerHasAudio((Player*)p) == 0
                    || p->bAudioCrash == 1)
            {
                //*When recive eos, we should stop the avTimer,
                //*or the PlayerGetPts() function will return wrong values.
                if(p->pAvTimer != NULL)
                    p->pAvTimer->Stop(p->pAvTimer);

                if(p->callback != NULL)
                    p->callback(p->pUserData, PLAYBACK_NOTIFY_EOS, NULL);
            }
            pthread_mutex_unlock(&p->eosMutex);
            return 0;
        }

        case PLAYER_AUDIO_RENDER_NOTIFY_EOS:
        {
            pthread_mutex_lock(&p->eosMutex);
            p->bAudioRenderEosReceived = 1;
            if(p->bVideoRenderEosReceived == 1 || WFD_PlayerHasVideo((Player*)p) == 0
                    || p->bVideoCrash == 1)
            {
                //*When recive eos, we should stop the avTimer,
                //*or the PlayerGetPts() function will return wrong values.
                if(p->pAvTimer != NULL)
                    p->pAvTimer->Stop(p->pAvTimer);

                if(p->callback != NULL)
                    p->callback(p->pUserData, PLAYBACK_NOTIFY_EOS, NULL);
            }
            pthread_mutex_unlock(&p->eosMutex);
            return 0;
        }

        case PLAYER_VIDEO_RENDER_NOTIFY_CRASH:
        {
            p->bVideoRenderEosReceived = 1;
            CDX_LOGD("==== video render crash === ");
            return 0;
        }

        case PLAYER_VIDEO_RENDER_NOTIFY_VIDEO_SIZE:
        {
            if(p->callback != NULL)
            {
                p->callback(p->pUserData, PLAYBACK_NOTIFY_VIDEO_SIZE, param);
                p->callback(p->pUserData, PLAYBACK_NOTIFY_FIRST_PICTURE, NULL);
            }
            return 0;
        }

        case PLAYER_VIDEO_RENDER_NOTIFY_VIDEO_CROP:
        {
            p->sVideoCropWindow[0] = ((int*)param)[0];
            p->sVideoCropWindow[1] = ((int*)param)[1];
            p->sVideoCropWindow[2] = ((int*)param)[2];
            p->sVideoCropWindow[3] = ((int*)param)[3];

            if(p->callback != NULL) {
                p->callback(p->pUserData, PLAYBACK_NOTIFY_VIDEO_CROP, param);
            }

            return 0;
        }
        case PLAYER_VIDEO_RENDER_NOTIFY_FIRST_PICTURE:
        {
            p->nFirstVideoRenderPts = *((int64_t*)param);

            CDX_LOGD("first video pts = %lld", p->nFirstVideoRenderPts);

            pthread_mutex_lock(&p->timerMutex);
            p->nTimeBase = p->nFirstVideoRenderPts;
            p->pAvTimer->SetTime(p->pAvTimer, p->nTimeBase);
            p->pAvTimer->Start(p->pAvTimer);
            pthread_mutex_unlock(&p->timerMutex);
            
            p->nLastTimeTimerAdjusted = p->nFirstVideoRenderPts;
            CDX_LOGD("miracast, video master clock, set time to %" PRId64 " and start timer.", p->nTimeBase);
            p->nTimeDiffCount = 0;
            return 0;
        }

        case PLAYER_AUDIO_RENDER_NOTIFY_FIRST_FRAME:
        {
            
            p->nFirstAudioRenderPts = *((int64_t*)param);
            CDX_LOGD("first audio pts = %.3f", p->nFirstAudioRenderPts/1000000.0);

#if 0
            pthread_mutex_lock(&p->timerMutex);
            p->nTimeBase = p->nFirstAudioRenderPts;
            p->pAvTimer->SetTime(p->pAvTimer, p->nTimeBase);
            p->pAvTimer->Start(p->pAvTimer);
            pthread_mutex_unlock(&p->timerMutex);

            p->nPreAudioPts = p->nFirstAudioRenderPts;
            p->nLastTimeTimerAdjusted = p->nFirstAudioRenderPts;
#endif
            return 0;
        }

        case PLAYER_VIDEO_RENDER_NOTIFY_PICTURE_PTS:
        {
            int64_t nVideoPts;
            int64_t nCurTime;
            int64_t nTimeDiff;
			int nResetDiff = 70000; /* 70ms */
            int     nWaitTimeMs;

            p->nPreVideoPts = *((int64_t*)param);
            nVideoPts = *((int64_t*)param);
            nCurTime  = p->pAvTimer->GetTime(p->pAvTimer);
            nTimeDiff = nVideoPts - nCurTime;

            nWaitTimeMs = (int)(nTimeDiff/1000);

			if (p->avs_policy == 1)
			{
				p->avs_policy = 0;
				CDX_LOGW("now avtime follow ==================> video_pts. ");
			}

			if (CdxAbs(nTimeDiff) > nResetDiff)	 //* difference is more than nResetDiff ms.
			{
                p->nTimeDiffCount++;
                if (p->nTimeDiffCount > 3) {
                    //* time difference is too big, we can not adjust the timer speed to make it
                    //* become synchronize in a short time,
                    //* so, just reset the timer, this will make the video display flush or stuff.
                    CDX_LOGE("video reset the timer to %.3f, time difference is %.3f",
                             nVideoPts/1000000.0, nTimeDiff/1000000.0);
                    p->pAvTimer->SetTime(p->pAvTimer, nVideoPts);
                    p->pAvTimer->SetSpeed(p->pAvTimer, 1000);
                    p->nLastTimeTimerAdjusted = nVideoPts;
                    nWaitTimeMs = 0;
                    p->nTimeDiffCount = 0;
                }
			}
			else
			{
                p->nTimeDiffCount = 0;
				if(nTimeDiff > 50000)
					p->pAvTimer->SetSpeed(p->pAvTimer, 1006);
				else if(nTimeDiff > 40000)
					p->pAvTimer->SetSpeed(p->pAvTimer, 1005);
				else if(nTimeDiff > 30000)
					p->pAvTimer->SetSpeed(p->pAvTimer, 1004);
				else if(nTimeDiff > 20000)
					p->pAvTimer->SetSpeed(p->pAvTimer, 1003);
				else if(nTimeDiff > 10000)
					p->pAvTimer->SetSpeed(p->pAvTimer, 1002);
				else if(nTimeDiff > 5000)
					p->pAvTimer->SetSpeed(p->pAvTimer, 1001);
				else if(nTimeDiff < -50000)
					p->pAvTimer->SetSpeed(p->pAvTimer, 994);
				else if(nTimeDiff < -40000)
					p->pAvTimer->SetSpeed(p->pAvTimer, 995);
				else if(nTimeDiff < -30000)
					p->pAvTimer->SetSpeed(p->pAvTimer, 996);
				else if(nTimeDiff < -20000)
					p->pAvTimer->SetSpeed(p->pAvTimer, 997);
				else if(nTimeDiff < -10000)
					p->pAvTimer->SetSpeed(p->pAvTimer, 998);
				else if(nTimeDiff < -5000)
					p->pAvTimer->SetSpeed(p->pAvTimer, 999);
				else
				{
				//	p->pAvTimer->SetSpeed(p->pAvTimer, 1000);
				}
			
            }
            return (nWaitTimeMs > 0 ? nWaitTimeMs : 0);
        }

        case PLAYER_AUDIO_RENDER_NOTIFY_PTS_AND_CACHETIME:
        {
            int64_t nAudioPts;
            int64_t nCachedTimeInSoundDevice;
            int64_t nCachedTimeInBufQueue = 33000; // 2 periods 2 * 16.7ms
            int64_t nTimeDiff;
            int64_t nCurTime;
            int64_t nCurAudioTime;
            int ret = 0;
            int nResetDiff = 100000; /* 100ms */
			
            nAudioPts = ((int64_t*)param)[0];
            nCachedTimeInSoundDevice = ((int64_t*)param)[1];
            nCurTime = p->pAvTimer->GetTime(p->pAvTimer);
            nCurAudioTime = nAudioPts - nCachedTimeInSoundDevice;

			/* first 1s audio, force render */
            if (CdxAbs(nAudioPts - p->nLastTimeTimerAdjusted) < 1000000)
            {
            	return 0;
            }

            
   			nTimeDiff = (nCurTime - nCachedTimeInBufQueue) - nCurAudioTime;

			if (nCurTime - p->nPreVideoPts > 1000000)
			{
				if (p->avs_policy == 0)
				{
					p->avs_policy = 1;
					CDX_LOGW("now avtime follow ==================> audio_pts. ");
				}
				if (CdxAbs(nTimeDiff) > nResetDiff)  //* difference is more than nResetDiff ms.
				{
					 //* time difference is too big, we can not adjust the timer speed to make it
					 //* become synchronize in a short time,
					 //* so, just reset the timer, this will make the video display flush or stuff.
					CDX_LOGE("audio reset the timer to %.3f, time difference is %.3f",
						 nCurAudioTime/1000000.0, nTimeDiff/1000000.0);
					p->pAvTimer->SetTime(p->pAvTimer, nCurAudioTime);
					p->pAvTimer->SetSpeed(p->pAvTimer, 1000);
					p->nLastTimeTimerAdjusted = nCurAudioTime;
				}
				else
				{
				
					if(nTimeDiff > 50000)
						p->pAvTimer->SetSpeed(p->pAvTimer, 1006);
					else if(nTimeDiff > 40000)
						p->pAvTimer->SetSpeed(p->pAvTimer, 1005);
					else if(nTimeDiff > 30000)
						p->pAvTimer->SetSpeed(p->pAvTimer, 1004);
					else if(nTimeDiff > 20000)
						p->pAvTimer->SetSpeed(p->pAvTimer, 1003);
					else if(nTimeDiff > 10000)
						p->pAvTimer->SetSpeed(p->pAvTimer, 1002);
					else if(nTimeDiff > 5000)
						p->pAvTimer->SetSpeed(p->pAvTimer, 1001);
					else if(nTimeDiff < -50000)
						p->pAvTimer->SetSpeed(p->pAvTimer, 994);
					else if(nTimeDiff < -40000)
						p->pAvTimer->SetSpeed(p->pAvTimer, 995);
					else if(nTimeDiff < -30000)
						p->pAvTimer->SetSpeed(p->pAvTimer, 996);
					else if(nTimeDiff < -20000)
						p->pAvTimer->SetSpeed(p->pAvTimer, 997);
					else if(nTimeDiff < -10000)
						p->pAvTimer->SetSpeed(p->pAvTimer, 998);
					else if(nTimeDiff < -5000)
						p->pAvTimer->SetSpeed(p->pAvTimer, 999);
					else
					{
					//	p->pAvTimer->SetSpeed(p->pAvTimer, 1000);
					}
				
				}
				ret = 0; 
			}
			else
			{
                if ((nCurTime - nCachedTimeInBufQueue) - nCurAudioTime > nResetDiff) {//audio delay larger than nResetDiff
                    //audio late & there is enough buffer
                    if (nCachedTimeInSoundDevice > 50000 || WFD_PlayerGetAudioPcmDataSize(p) > 30000) {
                        CDX_LOGW("audio timeout drop, pts: '%.3f', cache '%.3f', avtime '%.3f'",
                             nAudioPts/1000000.0, nCachedTimeInSoundDevice/1000000.0, nCurTime/1000000.0);
                        ret = TIMER_DROP_AUDIO_DATA;
                    }
                }
   			}
			return ret;
			
        }

        case PLAYER_VIDEO_DECODER_NOTIFY_STREAM_UNDERFLOW:
        case PLAYER_AUDIO_DECODER_NOTIFY_STREAM_UNDERFLOW:
            break;

        case PLAYER_VIDEO_DECODER_NOTIFY_CRASH:

            pthread_mutex_lock(&p->eosMutex);

            p->bVideoCrash = 1;
            if(p->callback != NULL)
                p->callback(p->pUserData, PLAYBACK_NOTIFY_VIDEO_UNSUPPORTED, NULL);

            if(p->bAudioRenderEosReceived == 1 ||
                    WFD_PlayerHasAudio((Player*)p) == 0 ||
                    p->bAudioCrash == 1)
            {
                if(p->callback != NULL)
                    p->callback(p->pUserData, PLAYBACK_NOTIFY_EOS, NULL);
            }
            pthread_mutex_unlock(&p->eosMutex);
            break;

        case PLAYER_VIDEO_RENDER_NOTIFY_VIDEO_FRAME:
            if(p->callback != NULL)
            {
                CDX_LOGD("===== notify render key frame in fast mode");
                p->callback(p->pUserData, PLAYBACK_NOTIFY_VIDEO_RENDER_FRAME, NULL);
            }
            break;

        case PLAYER_AUDIO_DECODER_NOTIFY_CRASH:

            pthread_mutex_lock(&p->eosMutex);

            p->bAudioCrash = 1;
            if(p->callback != NULL)
                p->callback(p->pUserData, PLAYBACK_NOTIFY_AUDIO_UNSUPPORTED, NULL);

            if(p->bVideoRenderEosReceived == 1 ||
                    WFD_PlayerHasVideo((Player*)p) == 0 ||
                    p->bVideoCrash == 1)
            {
                if(p->callback != NULL)
                    p->callback(p->pUserData, PLAYBACK_NOTIFY_EOS, NULL);
            }
            pthread_mutex_unlock(&p->eosMutex);
            break;
        case PLAYER_AUDIO_DECODER_NOTIFY_AUDIORAWPLAY:
            CDX_LOGE("%s,1",__func__);
            if(p->callback != NULL)
                p->callback(p->pUserData, PLAYBACK_NOTIFY_AUDIORAWPLAY, param);
            CDX_LOGE("%s,2",__func__);
            break;

        case PLAYER_VIDEO_DECODER_NOTIFY_SET_SECURE_BUFFER_COUNT:

            if(p->callback != NULL)
                p->callback(p->pUserData, PLAYBACK_NOTIFY_SET_SECURE_BUFFER_COUNT, param);
            break;

        case PLAYER_VIDEO_DECODER_NOTIFY_SET_SECURE_BUFFERS:

            if(p->callback != NULL)
                p->callback(p->pUserData, PLAYBACK_NOTIFY_SET_SECURE_BUFFERS, param);
            break;

        case PLAYER_AUDIO_RENDER_NOTIFY_AUDIO_INFO:

            if(p->callback != NULL)
            {
                int nAudioInfo[4];
                nAudioInfo[0] = p->nAudioStreamSelected;
                nAudioInfo[1] = ((int *)param)[0];
                nAudioInfo[2] = ((int *)param)[1];
                nAudioInfo[3] = ((int *)param)[2] > 0 ? ((int *)param)[2] : 320*1000;
                p->callback(p->pUserData, PLAYBACK_NOTIFY_AUDIO_INFO, (void *)nAudioInfo);
            }
            break;

        case PLAYER_VIDEO_RENDER_NOTIFY_PICTURE_PARAM:

            if(p->callback != NULL)
                p->callback(p->pUserData, PLAYBACK_NOTIFY_VIDEO_FRAME_PARAM, param);
            break;

        default:
            break;
    }

    return 0;
}

static int PlayerInitialVideo(PlayerContext* p)
{
	CDX_LOGI("step...");
    if(p->pVideoRender != NULL)
    {
        WFD_VideoRenderCompDestroy(p->pVideoRender);
        p->pVideoRender = NULL;
    }

    if(p->pVideoDecComp != NULL)
    {
        WFD_VideoDecCompDestroy(p->pVideoDecComp);
        p->pVideoDecComp = NULL;
    }

    p->sVideoCropWindow[0] = 0;
    p->sVideoCropWindow[1] = 0;
    p->sVideoCropWindow[2] = 0;
    p->sVideoCropWindow[3] = 0;

    p->pVideoDecComp = WFD_VideoDecCompCreate();

    const char *vd_outfmt = GetConfigParamterString("vd_output_fmt", NULL);
    CDX_LOG_CHECK(vd_outfmt, "vd_output_fmt NULL, pls check your config.");

    if (strcmp(vd_outfmt, "yv12") == 0)
    {
        p->sVideoConfig.eOutputPixelFormat = PIXEL_FORMAT_YV12;
    }
    else if (strcmp(vd_outfmt, "nv21") == 0)
    {
        p->sVideoConfig.eOutputPixelFormat = PIXEL_FORMAT_NV21;
    }
    else
    {
        CDX_LOG_CHECK(0, "invalid config '%s', pls check your cedarx.conf.",
                         vd_outfmt);
    }

    /* We should set format to nv21 if it is 3D stream,  if not can't play well, */
    /* Because new-display-double-stream only support nv21 format */
#if defined(CONF_NEW_DISPLAY)
    if(p->pVideoStreamInfo->bIs3DStream == 1)
        p->sVideoConfig.eOutputPixelFormat = PIXEL_FORMAT_NV21;
#endif

#if TINA_LINUX_SUPPORT
    p->sVideoConfig.nLbcLossyComMod = GetConfigParamterInt("vd_lbc_mode", 0);
    if (p->sVideoConfig.nLbcLossyComMod > 0)
    {
        int b4kEn = GetConfigParamterInt("vd_lbc_4k_en", 0);
        int bsamllsizeEn = GetConfigParamterInt("vd_lbc_smallsize_en", 1);
        logd("video stream width:%d, height:%d, b4kEn:%d, bsamllsizeEn:%d", p->pVideoStreamInfo->nWidth,
        p->pVideoStreamInfo->nHeight, b4kEn, bsamllsizeEn);

        // only enable lbc > 500*1080
        if ((p->pVideoStreamInfo->nWidth * p->pVideoStreamInfo->nHeight ) < 540000 && (bsamllsizeEn == 0))
        {
            p->sVideoConfig.nLbcLossyComMod = 0;
            logd("disable lbc");
        }
        else
        {
            p->sVideoConfig.bScaleDownEn  = 1;
            p->sVideoConfig.nHorizonScaleDownRatio  = 1;
            p->sVideoConfig.nVerticalScaleDownRatio = 1;

            p->sVideoConfig.bIsLossy = GetConfigParamterInt("vd_lbc_is_lossy", 1);
            p->sVideoConfig.bRcEn = GetConfigParamterInt("vd_lbc_rc_en", 1);
            logd("enable lbc");
        }
    }
#endif

    p->sVideoConfig.bSecureosEn = p->pVideoStreamInfo->bSecureStreamFlagLevel1;

    p->sVideoConfig.nDisplayHoldingFrameBufferNum     = GetConfigParamterInt("pic_4list_num", 3);
    p->sVideoConfig.nDeInterlaceHoldingFrameBufferNum = GetConfigParamterInt("pic_4di_num", 2);
    p->sVideoConfig.nRotateHoldingFrameBufferNum      = GetConfigParamterInt("pic_4rotate_num", 0);
    p->sVideoConfig.nDecodeSmoothFrameBufferNum       = GetConfigParamterInt("pic_4smooth_num", 3);
    if (p->pVideoStreamInfo->nWidth >= 3840 || p->pVideoStreamInfo->nHeight >= 2160) {
        CDX_LOGD("clap cast resolution may reach 4K, reduce smooth buffer to:%d to save memory",
             p->sVideoConfig.nDecodeSmoothFrameBufferNum);
    } else {
        //p->sVideoConfig.nDisplayHoldingFrameBufferNum = 5;
        //CDX_LOGD("resolution of airplay and miracast could not exceed 1080P, set nDisplayHoldingFrameBufferNum:%d",
        //     p->sVideoConfig.nDisplayHoldingFrameBufferNum);
    }

    p->sVideoConfig.nVbvBufferSize = 3*1024*1024;
    //if(p->nAvgByteRate > 0)
    //    p->sVideoConfig.nVbvBufferSize =
    //        p->nAvgByteRate*VIDEO_STREAM_BUFF_DURATION/1024/1024*1024*1024;
    CDX_LOGD("nVbvBufferSize=%d, nAvgByteRate=%d",
        p->sVideoConfig.nVbvBufferSize, p->nAvgByteRate);

    //* Because in the function of WFD_VideoDecCompSetVideoStreamInfo, we will call
    //* the callback function to do something, so we must setCallback before it.
    WFD_VideoDecCompSetCallback(p->pVideoDecComp, CallbackProcess, (void*)p);
    WFD_VideoDecCompSetTimer(p->pVideoDecComp, p->pAvTimer);

    p->bUnSurpportVideoFlag = 0;

#if defined(CONF_SUPPORT_G2D)
    /* G2D need 2 disp buf */
    p->sVideoConfig.nDisplayHoldingFrameBufferNum = p->sVideoConfig.nDisplayHoldingFrameBufferNum + 2;
    LayerControl(p->pLayerCtrl, CDX_LAYER_CMD_SET_DISP_BUFFER_NUM, &p->sVideoConfig.nDisplayHoldingFrameBufferNum);
#endif

    if(WFD_VideoDecCompSetVideoStreamInfo(p->pVideoDecComp, p->pVideoStreamInfo, &p->sVideoConfig) != 0)
    {
        CDX_LOGE("video stream is not supported.");

        if(p->callback != NULL)
        {
            p->callback(p->pUserData, PLAYBACK_NOTIFY_VIDEO_UNSUPPORTED, NULL);
        }

        WFD_VideoDecCompDestroy(p->pVideoDecComp);
        p->pVideoDecComp = NULL;

        return -1;
    }

    p->pVideoRender = WFD_VideoRenderCompCreate();
    if(p->pVideoRender == NULL)
    {
        CDX_LOGE("create video render fail.");
        WFD_VideoDecCompDestroy(p->pVideoDecComp);
        p->pVideoDecComp = NULL;
        return -1;
    }

    //* set video stream info for videorender
    WFD_VideoRenderCompSetVideoStreamInfo(p->pVideoRender, p->pVideoStreamInfo);

    /* tell the render whether the output buffer is secure(should be protect by
     * native window) or not
     */
    WFD_VideoRenderCompSetProtecedFlag(p->pVideoRender,p->pVideoStreamInfo->bSecureStreamFlag);
    WFD_VideoRenderCompSetCallback(p->pVideoRender, CallbackProcess, (void*)p);
    WFD_VideoRenderCompSetTimer(p->pVideoRender, p->pAvTimer);
    WFD_VideoRenderCompSetDecodeComp(p->pVideoRender, p->pVideoDecComp);

    WFD_VideoRenderCompSetSyncFirstPictureFlag(p->pVideoRender, 0);

    if(p->pVideoStreamInfo->bIs3DStream)
        WFD_VideoRenderSet3DMode(p->pVideoRender, PICTURE_3D_MODE_TWO_SEPERATED_PICTURE,
                DISPLAY_3D_MODE_2D);

	CDX_LOGI("trace, pLayerCtrl:'%p'...", p->pLayerCtrl);
	if(p->pLayerCtrl != NULL)
        WFD_VideoRenderCompSetWindow(p->pVideoRender, p->pLayerCtrl);

    WFD_VideoRenderCompSetPtsStatics(p->pVideoRender, p->nPtsList, p->nPtsGetTime);
    return 0;
}

static int PlayerInitialAudio(PlayerContext* p)
{
    int ret;

    if(p->pAudioRender != NULL)
    {
        WFD_AudioRenderCompDestroy(p->pAudioRender);
        p->pAudioRender = NULL;
    }

    if(p->pAudioDecComp != NULL)
    {
        WFD_AudioDecCompDestroy(p->pAudioDecComp);
        p->pAudioDecComp = NULL;
    }

    p->pAudioDecComp = WFD_AudioDecCompCreate();
    ret = WFD_AudioDecCompSetAudioStreamInfo(p->pAudioDecComp,
                                         p->pAudioStreamInfo,
                                         p->nAudioStreamNum,
                                         p->nAudioStreamSelected);

    if(ret != 0)
    {
        CDX_LOGE("selected audio stream is not supported, "
                "call SwitchAudio() to select another stream.");
        /* in this case the decoder did process, but video and subtitle will be work */
    }

    WFD_AudioDecCompSetCallback(p->pAudioDecComp, CallbackProcess, (void*)p);
    WFD_AudioDecCompSetTimer(p->pAudioDecComp, p->pAvTimer);

    p->pAudioRender = WFD_AudioRenderCompCreate();
    if(p->pAudioRender == NULL)
    {
        CDX_LOGE("create audio render fail.");
        WFD_AudioDecCompDestroy(p->pAudioDecComp);
        p->pAudioDecComp = NULL;
        return -1;
    }

    WFD_AudioRenderCompSetCallback(p->pAudioRender, CallbackProcess, (void*)p);
    WFD_AudioRenderCompSetTimer(p->pAudioRender, p->pAvTimer);
    WFD_AudioRenderCompSetDecodeComp(p->pAudioRender, p->pAudioDecComp);
    if(p->pAudioSink != NULL)
        WFD_AudioRenderCompSetAudioSink(p->pAudioRender, p->pAudioSink);

    return 0;
}

int WFD_PlayerSetCastMode(Player *pl, enum CASTMODE eCastMode){
    PlayerContext* p;
    p = (PlayerContext*)pl;
    if(p->pVideoRender != NULL){
        return WFD_VideoRenderSetCastMode(p->pVideoRender, eCastMode);
    }else{
        return -1;
    }
}

int WFD_PlayerSetRotateDegree(Player *pl, enum ROTATEDEGREE eRotateDegree){
    PlayerContext* p;
    p = (PlayerContext*)pl;
    CDX_LOGD("set rotate degree:%d, videorender: %p", eRotateDegree, p->pVideoRender);
    char degree[32];
    sprintf(degree, "%d", eRotateDegree);

//    if(uci_set_config("degree", degree, DISPLAY_UCI_CONFIG_PATH, "setting"))
//       CDX_LOGW("uci_set_config degree fail !");

    if (p != NULL) {
        if(p->pVideoRender != NULL) {
            if (p->pVideoStreamInfo != NULL) {
#if defined(CONF_SUPPORT_VE_ROTATE)
                CDX_LOGD("VE rotate, eRotateDegree:%d", eRotateDegree);
                if(eRotateDegree > 0)
                    p->sVideoConfig.bRotationEn  = 1;
                else {
                    p->sVideoConfig.bRotationEn  = 0;
                    eRotateDegree = 0;
                }
                p->sVideoConfig.nRotateDegree = eRotateDegree;
                return 0;
#elif defined(CONF_SUPPORT_G2D)
                if (p->pVideoStreamInfo->nWidth >= 3840)
                    return WFD_VideoRenderSetRotateDegree(p->pVideoRender, ROTATE_DEGREE_UNSET);
                else
                    return WFD_VideoRenderSetRotateDegree(p->pVideoRender, eRotateDegree);
#else
                return 0;
#endif
            }
        }
    }
    return -1;
}

int WFD_PlayerSetFullScreen(Player *pl, int fullScreen){
    PlayerContext* p;
    p = (PlayerContext*)pl;

    if(p->pVideoRender != NULL){
        return WFD_VideoRenderSetFullScreen(p->pVideoRender, fullScreen);
    }else{
        return -1;
    }
}
