/*
 * Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
 * All rights reserved.
 *
 * File : videoDecComponent.h
 * Description : video decoder component
 * History :
 *
 */

#ifndef WFD_VIDEO_DECODE_COMPONENT
#define WFD_VIDEO_DECODE_COMPONENT

#include "player_i.h"
#include "vdecoder.h"
#include "avtimer.h"

enum VIDEORENDERCALLBACKMG
{
    VIDEO_RENDER_VIDEO_INFO = 0,
    VIDEO_RENDER_REQUEST_BUFFER,
    VIDEO_RENDER_DISPLAYER_BUFFER,
    VIDEO_RENDER_RETURN_BUFFER,
    VIDEO_RENDER_RESOLUTION_CHANGE,
};

typedef struct VideoDecComp VideoDecComp;

typedef int (*VideoRenderCallback)(void* pUserData, int eMessageId, void* param);

VideoDecComp* WFD_VideoDecCompCreate(void);

int WFD_VideoDecCompDestroy(VideoDecComp* v);

int WFD_VideoDecCompStart(VideoDecComp* v);

int WFD_VideoDecCompStop(VideoDecComp* v);

int WFD_VideoDecCompPause(VideoDecComp* v);


int WFD_VideoDecCompReset(VideoDecComp* v);


int WFD_VideoDecCompSetCallback(VideoDecComp* v, PlayerCallback callback, void* pUserData);

int WFD_VideoDecCompSetDecodeKeyFrameOnly(VideoDecComp* v, int bDecodeKeyFrameOnly);


int WFD_VideoDecCompSetVideoStreamInfo(VideoDecComp* v, VideoStreamInfo* pStreamInfo,
        VConfig* pVconfig);

int WFD_VideoDecCompGetVideoStreamInfo(VideoDecComp* v, VideoStreamInfo* pStreamInfo);

int WFD_VideoDecCompSetTimer(VideoDecComp* v, AvTimer* timer);

int WFD_VideoDecCompRequestStreamBuffer(VideoDecComp* v,
                                    int           nRequireSize,
                                    char**        ppBuf,
                                    int*          pBufSize,
                                    char**        ppRingBuf,
                                    int*          pRingBufSize,
                                    int           nStreamIndex);

int WFD_VideoDecCompSubmitStreamData(VideoDecComp*        v,
                                 VideoStreamDataInfo* pDataInfo,
                                 int                  nStreamIndex);


int WFD_VideoDecCompStreamDataSize(VideoDecComp* v, int nStreamIndex);

int WFD_VideoDecCompStreamFrameNum(VideoDecComp* v, int nStreamIndex);

VideoPicture* WFD_VideoDecCompRequestPicture(VideoDecComp* v, int nStreamIndex,
        int* bResolutionChanged);

int WFD_ValidPictureNum(VideoDecComp* p, int nStreamIndex);

int WFD_VideoDecCompReturnPicture(VideoDecComp* v, VideoPicture* pPicture);


int WFD_VideoDecCompReopenVideoEngine(VideoDecComp* v);


//*******************************  START  **********************************//
//** for new display structure interface.
//**
FbmBufInfo* WFD_VideoDecCompGetVideoFbmBufInfo(VideoDecComp* v);

VideoPicture* WFD_VideoDecCompSetVideoFbmBufAddress(VideoDecComp* v, VideoPicture* pVideoPicture,
       int bForbidUseFlag);

int WFD_VideoDecCompSetVideoFbmBufRelease(VideoDecComp* v);

VideoPicture* WFD_VideoDecCompRequestReleasePicture(VideoDecComp* v);

VideoPicture* WFD_VideoDecCompReturnRelasePicture(VideoDecComp* v, VideoPicture* pVpicture,
        int bForbidUseFlag);

//********************************  END  ***********************************//

#endif
