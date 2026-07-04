/*
 * Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
 * All rights reserved.
 *
 * File : wfd_player.h
 * Description : player
 * History :
 *
 */

#ifndef WFD_PLAYER_H
#define WFD_PLAYER_H

#include "vdecoder.h"
#include "adecoder.h"
#include "layerControl.h"
#include "soundControl.h"
#include <CdxEnumCommon.h>
#include "player.h"

CHECK_PLAYBACK_NOTIFY_MAX_VALID(PLAYBACK_NOTIFY_MAX)

enum CASTMODE
{
    CAST_MIRACAST_MODE = 0,
    CAST_DLNA_MODE,
    CAST_AIRPLAY_MODE
};

typedef int (*PlayerCallback)(void* pUserData, int eMessageId, void* param);

typedef void* Player;


#ifdef __cplusplus
extern "C" {
#endif

Player* WFD_PlayerCreate(void);

void WFD_PlayerDestroy(Player* pl);

int WFD_PlayerSetCallback(Player* pl, PlayerCallback callback, void* pUserData);


//*******************************  START  **********************************//
//** Play Control APIs.
//**

int WFD_PlayerStart(Player* pl);

int WFD_PlayerStop(Player* pl);      //* media stream information is still kept by the player.

int WFD_PlayerPause(Player* pl);

enum EPLAYERSTATUS WFD_PlayerGetStatus(Player* pl);

//* for seek operation, mute be called under paused status.
int WFD_PlayerReset(Player* pl, int64_t nSeekTimeUs);

//* must be called under stopped status, all stream information cleared.
int WFD_PlayerClear(Player* pl);


//********************************  END  ***********************************//

//*******************************  START  **********************************//
//** Streaming Control APIs.
//**

int WFD_PlayerRequestStreamBuffer(Player*       pl,
                              int             nRequireSize,
                              void**          ppBuf,
                              int*            pBufSize,
                              void**          ppRingBuf,
                              int*            pRingBufSize,
                              enum EMEDIATYPE eMediaType,
                              int             nStreamIndex);

int WFD_PlayerSubmitStreamData(Player*            pl,
                           MediaStreamDataInfo* pDataInfo,
                           enum EMEDIATYPE      eMediaType,
                           int                  nStreamIndex);





//* how much video stream data in stream buffere.
int WFD_PlayerGetVideoStreamDataSize(Player* pl);

//* how many stream frame in buffer.
int WFD_PlayerGetVideoStreamFrameNum(Player* pl);

int WFD_PlayerGetAudioPcmDataSize(Player* pl);

//********************************  END  ***********************************//

//*******************************  START  **********************************//
//** Video APIs.
//**

int WFD_PlayerSetVideoStreamInfo(Player* pl, VideoStreamInfo* pStreamInfo);


int WFD_PlayerHasVideo(Player* pl);

//********************************  END  ***********************************//

//*******************************  START  **********************************//
//** Audio APIs.
//**

int WFD_PlayerSetAudioStreamInfo(Player* pl, AudioStreamInfo* pStreamInfo,
        int nStreamNum, int nDefaultStream);




int WFD_PlayerHasAudio(Player* pl);


//hkw switch audio track for IPTV
int WFD_PlayerStopAudio(Player* pl);

int WFD_PlayerStartAudio(Player* pl);

//********************************  END  ***********************************//

//*******************************  START  **********************************//
//** Display Control APIs.
//**
int WFD_PlayerSetWindow(Player* pl, LayerCtrl* pLc);

//********************************  END  ***********************************//

//*******************************  START  **********************************//
//** Audio Output Control APIs.
//**
int WFD_PlayerSetAudioSink(Player* pl, SoundCtrl* pAudioSink);

//set cast mode: airplay/miracast/DLNA
//miracast neet to crop
int WFD_PlayerSetCastMode(Player *pl, enum CASTMODE eCastMode);

//0:0' ,1:90' ,2:180' ,3:270'
int WFD_PlayerSetRotateDegree(Player *pl, enum ROTATEDEGREE eRotateDegree);

int WFD_PlayerSetFullScreen(Player *pl, int fullScreen);
//********************************  END  ***********************************//


#ifdef __cplusplus
}
#endif

#endif
