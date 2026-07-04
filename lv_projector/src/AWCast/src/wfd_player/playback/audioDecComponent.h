/*
 * Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
 * All rights reserved.
 *
 * File : audioDecComponent.h
 * Description : audio decoder component
 * History :
 *
 */

#ifndef WFD_AUDIO_DECODE_COMPONENT
#define WFD_AUDIO_DECODE_COMPONENT

#include "player_i.h"
#include "avtimer.h"
#include "adecoder.h"

typedef struct AUDIOSTREAMDATAINFO
{
    char*   pData;
    int     nLength;
    int64_t nPts;
    int64_t nPcr;
    int     bIsFirstPart;
    int     bIsLastPart;
}AudioStreamDataInfo;

typedef struct AudioDecComp AudioDecComp;

AudioDecComp* WFD_AudioDecCompCreate(void);

int WFD_AudioDecCompDestroy(AudioDecComp* a);

int WFD_AudioDecCompStart(AudioDecComp* a);

int WFD_AudioDecCompStop(AudioDecComp* a);

int WFD_AudioDecCompPause(AudioDecComp* a);

int WFD_AudioDecCompReset(AudioDecComp* a, int64_t nSeekTime);

int WFD_AudioDecCompSetEOS(AudioDecComp* a);

int WFD_AudioDecCompSetCallback(AudioDecComp* a, PlayerCallback callback, void* pUserData);

int WFD_AudioDecCompSetAudioStreamInfo(AudioDecComp*    a,
                                   AudioStreamInfo* pStreamInfo,
                                   int              nStreamCount,
                                   int              nDefaultStreamIndex);

int WFD_AudioDecCompGetAudioSampleRate(AudioDecComp* a, unsigned int* pSampleRate,
        unsigned int* pChannelNum, unsigned int* pBitRate);

int WFD_AudioDecCompSetTimer(AudioDecComp* a, AvTimer* timer);

int WFD_AudioDecCompRequestStreamBuffer(AudioDecComp* a,
                                    int           nRequireSize,
                                    char**        ppBuf,
                                    int*          pBufSize,
                                    char**        ppRingBuf,
                                    int*          pRingBufSize,
                                    int           nStreamIndex);

int WFD_AudioDecCompSubmitStreamData(AudioDecComp*        a,
                                 AudioStreamDataInfo* pDataInfo,
                                 int                  nStreamIndex);

int WFD_AudioDecCompRequestPcmData(AudioDecComp*   a,
                               unsigned char** ppData,
                               unsigned int*   pSize,
                               int64_t*        pPts,
                               CdxPlaybkCfg*   cfg);

int WFD_AudioDecCompReleasePcmData(AudioDecComp* a, int nReleaseSize);

int WFD_AudioDecCompPcmDataSize(AudioDecComp* p, int nStreamIndex);
#endif
