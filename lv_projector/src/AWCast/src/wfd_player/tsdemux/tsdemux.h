/*
 * Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
 * All rights reserved.
 *
 * File : tsdemux.h
 * Description : ts demux for IPTV
 * History :
 *
 */

#ifndef TSDEMUX_H
#define TSDEMUX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "tsdemux_i.h"
#include <stdint.h>
#include "CdxParser.h"

#define NB_PID_MAX 8192
#define PES_START_SIZE 9 //PES_header_data_length字段和该字段前数据的总长度
#define MAX_PES_HEADER_SIZE (9 + 255)

//define callback function to push data
typedef int (*push_data_cb)(unsigned char* data, int len, int is_new_frame, void* parm);

typedef struct WfdHdcpOpsS
{
    int (*init)(void *handle);
    int (*deinit)(void *handle);
    int (*decrypt)(void *handle, const uint8_t privateData[16],
                          const uint8_t *in, uint8_t *out, uint32_t len);
} WfdHdcpOpsS;

typedef enum MPEGTS_PES_STATE
{
    MPEGTS_PESHEADER = 0,
    MPEGTS_PESHEADER_FILL,
    MPEGTS_PESPAYLOAD,
    MPEGTS_PESSKIP
} mpegts_pes_state_t;

typedef struct FILTER
{
    int pid;
    demux_codec_type codec_type;

    pdemux_callback_t requestbufcb;
    pdemux_callback_t updatedatacb;
    push_data_cb push_es_data; //* push PES data callback function

    //variables below are used for getting PES format
    unsigned char pes_header[MAX_PES_HEADER_SIZE]; //save header data
    unsigned int pes_data_index; //index to indicate location of handled PES header
    unsigned int pes_header_size;
    unsigned int pes_total_size;
    unsigned int pes_payload_size;
    int64_t pts; //presentation tim stamp
    int64_t dts;
    int64_t pre_pts; //previous pts

    //variables below are used for writing ES data
    mpegts_pes_state_t pes_state;
    int is_new_frame;
    unsigned char *es_buffer_ptr; //es buffer current pointer
    unsigned int es_buffer_free_size; //free size of buffer for pushing data
    unsigned int es_valid_size; //written size
    unsigned int es_ctrl_bits; //control bits
    md_data_info_t es_data_info;
    md_buf_t md_buf;

    uint8_t is_pes_decrypted;
    uint8_t pes_private_data[16]; //used for enctryped stream to store pes_private_data;
    struct WfdHdcpOpsS *hdcpOps;
    void *hdcpHandle;

    void *cookie;
} filter_t;

typedef struct MPEGTSCONTEXT
{
    filter_t* pids[NB_PID_MAX];
    int rawPacketSize;
} mpegts_context_t;

void* ts_demux_open(void);
int ts_demux_close(void* handle);
int ts_demux_open_filter(void* handle, int pid, demux_filter_param_t* filter_param);
int ts_demux_close_filter(void* handle, int pid);
int ts_demux_handle_packets(void* handle, unsigned char* pktdata, int data_size);
int ts_demux_set_hdcp_ops(void* handle, int pid, WfdHdcpOpsS* ops);

#ifdef __cplusplus
}
#endif

#endif
