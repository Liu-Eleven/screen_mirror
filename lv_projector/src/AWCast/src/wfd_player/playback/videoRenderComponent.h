/*
 * Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
 * All rights reserved.
 *
 * File : videoRenderComponent.h
 * Description : video render component
 * History :
 *
 */

#ifndef WFD_VIDEO_RENDER_H
#define WFD_VIDEO_RENDER_H

#include "player_i.h"
#include "videoDecComponent.h"
#include "framerateEstimater.h"
#include "layerControl.h"

typedef struct VideoRenderComp VideoRenderComp;

VideoRenderComp* WFD_VideoRenderCompCreate(void);

int WFD_VideoRenderCompDestroy(VideoRenderComp* v);

int WFD_VideoRenderCompStart(VideoRenderComp* v);

int WFD_VideoRenderCompStop(VideoRenderComp* v);

int WFD_VideoRenderCompPause(VideoRenderComp* v);

int WFD_VideoRenderCompReset(VideoRenderComp* v);

int WFD_VideoRenderCompSetEOS(VideoRenderComp* v);

int WFD_VideoRenderCompSetCallback(VideoRenderComp* v, PlayerCallback callback, void* pUserData);

int WFD_VideoRenderCompSetTimer(VideoRenderComp* v, AvTimer* timer);

int WFD_VideoRenderCompSetWindow(VideoRenderComp* v, LayerCtrl* lc);

int WFD_VideoRenderCompSetDecodeComp(VideoRenderComp* v, VideoDecComp* d);

int WFD_VideoRenderSet3DMode(VideoRenderComp* v,
                         enum EPICTURE3DMODE ePicture3DMode,
                         enum EDISPLAY3DMODE eDisplay3DMode);

void WFD_VideoRenderCompSetProtecedFlag(VideoRenderComp* v, int bProtectedFlag);

int WFD_VideoRenderCompSetSyncFirstPictureFlag(VideoRenderComp* v, int bSyncFirstPictureFlag);

int WFD_VideoRenderCompSetFrameRateEstimater(VideoRenderComp* v, FramerateEstimater* fe);

int WFD_VideoRenderCompSetPtsStatics(VideoRenderComp* v, int64_t* nPtsList, int64_t* nPtsGetTime);

int WFD_VideoRenderCompSetVideoStreamInfo(VideoRenderComp* v, VideoStreamInfo* pStreamInfo);

int WFD_VideoRenderSetCastMode(VideoRenderComp *v, enum CASTMODE eCastMode);

int WFD_VideoRenderSetRotateDegree(VideoRenderComp *v, enum ROTATEDEGREE eRotateDegree);

int WFD_VideoRenderSetFullScreen(VideoRenderComp *p, int fullScreen);
#endif
