/*
 * Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
 * All rights reserved.
 *
 * File : audioRenderComponent.h
 * Description : audio render component
 * History :
 *
 */

#ifndef WFD_AUDIO_RENDER_H
#define WFD_AUDIO_RENDER_H

#include "player_i.h"
#include "audioDecComponent.h"
#include "soundControl.h"

typedef struct AudioRenderComp AudioRenderComp;

AudioRenderComp* WFD_AudioRenderCompCreate(void);

int WFD_AudioRenderCompDestroy(AudioRenderComp* a);

int WFD_AudioRenderCompStart(AudioRenderComp* a);

int WFD_AudioRenderCompStop(AudioRenderComp* a);

int WFD_AudioRenderCompPause(AudioRenderComp* a);

int WFD_AudioRenderCompReset(AudioRenderComp* a);

int WFD_AudioRenderCompSetEOS(AudioRenderComp* a);

int WFD_AudioRenderCompSetCallback(AudioRenderComp* a, PlayerCallback callback, void* pUserData);

int WFD_AudioRenderCompSetTimer(AudioRenderComp* a, AvTimer* timer);

int WFD_AudioRenderCompSetAudioSink(AudioRenderComp* a, SoundCtrl* pAudioSink);

int WFD_AudioRenderCompSetDecodeComp(AudioRenderComp* a, AudioDecComp* d);

#endif
