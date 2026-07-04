/*
 * Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
 * All rights reserved.
 *
 * File : streamManager.h
 * Description : stream manager
 * History :
 *
 */

#ifndef STREAM_MANAGER_H
#define STREAM_MANAGER_H

#include <pthread.h>

typedef struct StreamFrameS StreamFrame;
struct StreamFrameS {
    void*   pData;
    int     nLength;
    int64_t nPts;
    int64_t nPcr;
    int64_t nDuration;
};

typedef struct StreamManager StreamManager;

StreamManager* WFD_StreamManagerCreate(int nMaxBufferSize, int nMaxFrameNum, int nStreamID);

void WFD_StreamManagerDestroy(StreamManager* pSm);

void WFD_StreamManagerReset(StreamManager* pSm);

int WFD_StreamManagerBufferSize(StreamManager* pSm);

int WFD_StreamManagerStreamFrameNum(StreamManager* pSm);

int WFD_StreamManagerStreamDataSize(StreamManager* pSm);

int WFD_StreamManagerRequestBuffer(StreamManager* pSm, int nRequireSize, char** ppBuf, int* pBufSize);

int WFD_StreamManagerAddStream(StreamManager* pSm, StreamFrame* pStreamFrame);

StreamFrame* WFD_StreamManagerRequestStream(StreamManager* pSm);

StreamFrame* WFD_StreamManagerGetFrameInfo(StreamManager* pSm, int nFrameIndex);

int WFD_StreamManagerReturnStream(StreamManager* pSm, StreamFrame* pStreamFrame);

/* This function is deprecated. It does nothing. */
int WFD_StreamManagerFlushStream(StreamManager* pSm, StreamFrame* pStreamFrame);

/* Rewind the stream to a specific position/time. You should call this function
 * after switch track.
 */
int WFD_StreamManagerRewind(StreamManager *p, int64_t curTime);

#endif
