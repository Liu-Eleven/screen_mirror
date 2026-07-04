/*
 * Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
 * All rights reserved.
 *
 * File : tsdemux_i.h
 * Description : ts demux for IPTV
 * History :
 *
 */

#ifndef TSDEMUX_I_H
#define TSDEMUX_I_H

#ifdef __cplusplus
extern "C" {
#endif

#define FIRST_PART_BIT 0x01
#define LAST_PART_BIT 0x02
#define PTS_VALID_BIT 0x04
#define RANDOM_ACCESS_FRAME_BIT 0x08

//define callback function
typedef int (*pdemux_callback_t)(void* param, void* cookie);

typedef enum {
    DMX_CODEC_UNKOWN = 0,
    DMX_CODEC_H264,
    DMX_CODEC_H265
}demux_codec_type;

typedef struct MEDIA_BUFFER
{
    unsigned char *buffer;
    unsigned int bufferSize;
}md_buf_t;

typedef struct MEDIA_DATA_INFO
{
    int64_t pts;
    unsigned int dataLen;
    unsigned int ctrlBits;
}md_data_info_t;

typedef struct DEMUX_FILTER_PARAM
{
    demux_codec_type codec_type;
    pdemux_callback_t request_buffer_cb;
    pdemux_callback_t update_data_cb;
    void *cookie;
}demux_filter_param_t;

#ifdef __cplusplus
}
#endif

#endif
