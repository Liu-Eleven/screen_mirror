/*
 * Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
 * All rights reserved.
 *
 * File : tsdemux.c
 * Description : ts demux for IPTV
 * History :
 *
 */

//#define CONFIG_LOG_LEVEL OPTION_LOG_LEVEL_DETAIL
#define LOG_TAG "tsdemux"

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include "tsdemux.h"
#include "cdx_log.h"
#include "CdxTsParser.h"

//packet size of different format
#define TS_FEC_PACKET_SIZE 204
#define TS_DVHS_PACKET_SIZE 192
#define TS_PACKET_SIZE 188

#define SUBMIT_INCOMPLETE_PES_PAYLOAD (0)

//#define WFD_DEBUG_SAVE_TS
#ifdef WFD_DEBUG_SAVE_TS
static const char* dbg_wfd_ts = "/wfd_ts.ts";
static FILE* dbg_wfd_ts_fp = NULL;
#endif

#define TS_RB16(p) (((*(p)) << 8) | (*((p) + 1))) //p必须()包起来
#define TS_RB32(p) (((*(p)) << 24) | ((*((p) + 1)) << 16) | ((*((p) + 2)) << 8) | (*((p) + 3)))

static inline int64_t get_pts(const unsigned char* pts)
{
    int64_t timeStamp = (int64_t)((pts[0] >> 1) & 0x07) << 30;
    timeStamp |= (TS_RB16(pts + 1) >> 1) << 15;
    timeStamp |= TS_RB16(pts + 3) >> 1;
    return timeStamp;
}

int push_es_data(unsigned char* data, int len, int is_new_frame, void* param)
{
    unsigned char *data_ptr = data;
    filter_t* filter = (filter_t*)param;
    if(is_new_frame) //新帧
    {
        if(filter->es_valid_size > 0) //有上一帧残余, es->pes_payload_size为0走这里正常, 不为0时上一帧可能已经存在丢帧
        {
            filter->es_ctrl_bits |= LAST_PART_BIT; // es->pes_payload_size不为0时此处待考究

            //submit es data
            filter->es_data_info.ctrlBits = filter->es_ctrl_bits;
            filter->es_data_info.dataLen  = filter->es_valid_size;
            filter->updatedatacb(&filter->es_data_info, filter->cookie);

            //clear status
            filter->es_buffer_free_size = 0; //解码器buffer未填充的大小
            filter->es_valid_size = 0; //解码器buffer已填充的大小
            filter->es_ctrl_bits = 0;
        }
    }

    while(len > 0)
    {
        if(filter->es_buffer_free_size == 0) //无解码器buffer
        {
            //request buffer
            if(filter->requestbufcb(&filter->md_buf, filter->cookie) != 0) //请求解码器buffer失败, CTC要求退出
                return -1;

            filter->es_buffer_ptr = filter->md_buf.buffer; //记录解码器buffer未填充位置
            filter->es_buffer_free_size = filter->md_buf.bufferSize;
        }

        if(is_new_frame)
        {
            filter->es_ctrl_bits |= FIRST_PART_BIT;
            if(filter->pts != (int64_t)-1) //一帧填充一次pts
            {
                filter->es_ctrl_bits |= PTS_VALID_BIT;
                //I:DTS=PTS,P:DTS<PTS,B:DTS>PTS
                filter->es_data_info.pts = filter->dts * 1000 / 90; //ISO/IEC13818-1, us
            }
            is_new_frame = 0;
        }

#if SUBMIT_INCOMPLETE_PES_PAYLOAD
        if((int)filter->es_buffer_free_size > len) //进入后必须退出循环
        {
            memcpy(filter->es_buffer_ptr, data_ptr, len);
            filter->es_valid_size += len;
            filter->es_buffer_ptr += len;
            filter->es_buffer_free_size -= len;
            len = 0;
        }
        else //拿到的解码器buffer不够
        {
            memcpy(filter->es_buffer_ptr, data_ptr, filter->es_buffer_free_size);
            len -= filter->es_buffer_free_size;
            filter->es_valid_size += filter->es_buffer_free_size;
            data_ptr += filter->es_buffer_free_size;
            filter->es_buffer_free_size = 0;
        }

        if(filter->pes_payload_size > 0)
        {
            if(filter->es_valid_size == filter->pes_payload_size)
                filter->es_ctrl_bits |= LAST_PART_BIT;
            else if(filter->es_valid_size < filter->pes_payload_size)
                filter->pes_payload_size -= filter->es_valid_size;
            else
                filter->pes_payload_size = 0; //需要保证下一个PES包的payload正常填充
        }

        //submit es data
        filter->es_data_info.ctrlBits = filter->es_ctrl_bits;
        filter->es_data_info.dataLen  = filter->es_valid_size;
        filter->updatedatacb(&filter->es_data_info, filter->cookie); //后续数据的pts值待考究

        filter->es_buffer_free_size = 0; //需重新请求解码器buffer
        filter->es_valid_size = 0;
        filter->es_ctrl_bits = 0;
#else
        if((int)filter->es_buffer_free_size > len) //进入后必须退出循环
        {
            memcpy(filter->es_buffer_ptr, data_ptr, len);
            filter->es_valid_size += len;
            filter->es_buffer_ptr += len;
            filter->es_buffer_free_size -= len;
            len = 0;

            if(filter->pes_payload_size > 0 && filter->es_valid_size >= filter->pes_payload_size) //有明确PES包payload长度, 且数据量超过或满足了一个包的payload长度, 需关于大于的情况
            {
                if(filter->es_valid_size == filter->pes_payload_size)
                    filter->es_ctrl_bits |= LAST_PART_BIT; //大于情况有风险
                else
                    filter->pes_payload_size = 0; //需要保证下一个PES包的payload正常填充

                //submit es data
                filter->es_data_info.ctrlBits = filter->es_ctrl_bits;
                filter->es_data_info.dataLen  = filter->es_valid_size;
                filter->updatedatacb(&filter->es_data_info, filter->cookie);

                filter->es_buffer_free_size = 0;
                filter->es_valid_size = 0;
                filter->es_ctrl_bits = 0;
            }
        }
        else //拿到的解码器buffer不够
        {
            memcpy(filter->es_buffer_ptr, data_ptr, filter->es_buffer_free_size);
            len -= filter->es_buffer_free_size;
            filter->es_valid_size += filter->es_buffer_free_size;
            data_ptr += filter->es_buffer_free_size;

            if(filter->pes_payload_size > 0)
            {
                if(filter->es_valid_size == filter->pes_payload_size)
                    filter->es_ctrl_bits |= LAST_PART_BIT;
                else if(filter->es_valid_size < filter->pes_payload_size)
                    filter->pes_payload_size -= filter->es_valid_size;
                else
                    filter->pes_payload_size = 0; //需要保证下一个PES包的payload正常填充
            }

            //submit es data
            filter->es_data_info.ctrlBits = filter->es_ctrl_bits;
            filter->es_data_info.dataLen  = filter->es_valid_size;
            filter->updatedatacb(&filter->es_data_info, filter->cookie); //后续数据的pts值待考究

            filter->es_buffer_free_size = 0;
            filter->es_valid_size = 0;
            filter->es_ctrl_bits = 0;
        }
#endif
    }

    return 0;
}

static int ts_analyze(const unsigned char* buf, int size, int packet_size, int* index)
{
    int stat[204];
    int i = 0;
    int x = 0;
    int best_score = 0;
    memset(stat, 0, packet_size * sizeof(int));

    for(x = i = 0; i < size; i++)
    {
        if(buf[i] == 0x47)
        {
            stat[x]++;
            if(stat[x] > best_score)
            {
                best_score = stat[x];
                if(index)
                    *index = x;
            }
        }

        x++;
        if(x == packet_size)
            x = 0;
    }

    return best_score;
}

//auto detect FEC presence, must have at least 1024 bytes
static int get_ts_packet_size(const unsigned char* buf, int size)
{
    int score, fec_score, dvhs_score;
    if(size < (TS_FEC_PACKET_SIZE * 5)) //没必要加1
        return -1;

    score = ts_analyze(buf, size, TS_PACKET_SIZE, NULL);
    dvhs_score = ts_analyze(buf, size, TS_DVHS_PACKET_SIZE, NULL);
    fec_score = ts_analyze(buf, size, TS_FEC_PACKET_SIZE, NULL);

    if((score > fec_score) && (score > dvhs_score))
        return TS_PACKET_SIZE;
    else if((dvhs_score > score) && (dvhs_score > fec_score))
        return TS_DVHS_PACKET_SIZE;
    else if((fec_score > dvhs_score) && (fec_score > score))
        return TS_FEC_PACKET_SIZE;
    else
        return -1;
}

static int find_start_code_pos(unsigned char* buf, int32_t len) {
    int pos = -1;
    int i = 0;
    for(i = 2; i < len; i++) {
        if(buf[i] == 1) { //找0x000001
            if(!buf[i - 1] && !buf[i - 2]) {
                pos = i - 2;
                if(i > 2 && !buf[i - 3])
                    pos--;
            }
            break;
        }
    }
    return pos;
}

static int handle_packet_payload(unsigned char* buf, int buf_size, int is_pes_start, filter_t* filter)
{
    if(is_pes_start)
    {
        filter->pes_state = MPEGTS_PESHEADER; //PES包header第一部分解析状态
        filter->pes_data_index = 0; //重置位置记录
        filter->pes_payload_size = 0; //PES包payload大小
    }

    unsigned char *p = buf;
    int i = 0, len = 0, is_new_frame = 0;
    while(buf_size > 0)
    {
        switch(filter->pes_state)
        {
            case MPEGTS_PESHEADER:
                len = PES_START_SIZE - filter->pes_data_index;
                if(len > buf_size)
                    len = buf_size;

                for(i = 0; i < len; i++)
                {
                    filter->pes_header[filter->pes_data_index++] = *p++; //将PES_header_data_length字段和该字段前数据解析保存
                }
                buf_size -= len;

                if(filter->pes_data_index == PES_START_SIZE) //已解析PES_header_data_length字段
                {
                    int skip = 1;
                    if(filter->pes_header[0] == 0x00 && filter->pes_header[1] == 0x00 && filter->pes_header[2] == 0x01) //检查PES包起始码
                    {
                        int code = filter->pes_header[3] | 0x100; //取stream_id
                        if((code <= 0x1df && code >= 0x1c0 ) ||
                            (code <= 0x1ef && code >= 0x1e0 ) ||
                            (code == 0x1fd) || (code == 0x1bd)) //follow ffmpeg
                        {
                            //a zero total size means the PES size is unbounded
                            filter->pes_total_size = TS_RB16(filter->pes_header + 4); //PES_packet_length字段
                            if(filter->pes_total_size)
                                filter->pes_total_size += 6; //3+1+2, PES包总长度

                            filter->pes_header_size = filter->pes_header[8] + 9; //PES包头总长度

                            if(filter->pes_total_size > 6)
                                filter->pes_payload_size = filter->pes_total_size - filter->pes_header_size;

                            skip = 0;
                            filter->pes_state = MPEGTS_PESHEADER_FILL;
                        }
                    }

                    if(skip)
                    {
                        //not a pes or unsuppored stream id
                        filter->pes_state = MPEGTS_PESSKIP;
                    }
                }
                break;
            case MPEGTS_PESHEADER_FILL:
                len = filter->pes_header_size - filter->pes_data_index; //PES包头剩余大小
                if(len > buf_size)
                    len = buf_size;

                for(i = 0; i < len; i++)
                {
                    filter->pes_header[filter->pes_data_index++] = *p++; //将PES包头数据解析保存
                }
                buf_size -= len;

                if(filter->pes_data_index == filter->pes_header_size) //已解析PES包头数据
                {
                    const unsigned char* r;
                    unsigned int         flags;
                    uint32_t  pes_header_remain_bytes = filter->pes_header[8];
                    uint32_t  pes_header_data_start_index = 9;

                    flags = filter->pes_header[7]; //PES包第八个字节
                    r = filter->pes_header + 9; //PES包第十个字节
                    filter->pts = (int64_t)-1; //默认值

                    if((flags & 0xc0) == 0x80) //only PTS
                    {
                        filter->pts = get_pts(r);
                        filter->dts = filter->pts;
                        r += 5;
                        pes_header_data_start_index += 5;
                        pes_header_remain_bytes -= 5;
                    }
                    else if((flags & 0xc0) == 0xc0) //PTS and DTS
                    {
                        filter->pts = get_pts(r);
                        filter->dts = get_pts(r+5);
                        pes_header_data_start_index += 10;
                        pes_header_remain_bytes -= 10;
                        r += 10;
                    }
                    /*
                    * Check if extension flag is set, which means pes stream has been encrypted.
                    * 1... .... pts-flag: True   1bit
                    * .0.. .... dts-flag: False   1bit
                    * ..0. .... escr-flag: False   1bit
                    * ...0 .... es-rate-flag: False   1bit
                    * .... 0... dsm-trick-mode-flag: False    1bit
                    * .... .0.. additional-copy-info-flag: False  1bit
                    * .... ..0. crc-flag: False     1bit
                    * .... ...1 extension-flag: True    1bit (This will be 1 if stream is enctrypted)
                    */
                    if(flags & 0x01) {
                        logv("This is an encrypted pes stream! pes_header_size = %d", filter->pes_header_size);
                        if(pes_header_remain_bytes > 1) {
                            uint8_t enctrypt_flag = filter->pes_header[pes_header_data_start_index++];
                            pes_header_remain_bytes--;
                            if((enctrypt_flag & 0x80) && (pes_header_remain_bytes >= 16)) {
                                logv("start to get private data!");
                                memcpy(filter->pes_private_data, &filter->pes_header[pes_header_data_start_index], 16);
                                filter->is_pes_decrypted = 1;
                            }
                        } else {
                            logw("invalid pes header size with extension flag! ignore extension flag!");
                        }
                    }
                    else {
                        filter->is_pes_decrypted = 0;
                    }                    
                    //we got the full header, now we parse and get the payload
                    filter->pes_state = MPEGTS_PESPAYLOAD;
                    filter->is_new_frame = 1; //PES包的payload开始
                }
                break;
            case MPEGTS_PESPAYLOAD:
                if(filter->pes_total_size)
                {
                    len = filter->pes_total_size - filter->pes_data_index;
                    if((len > buf_size) || (len < 0))
                    {
                        len = buf_size;
                    }
                }
                else
                {
                    len = buf_size;
                }

                //if pes payload is encrypted, we should call hdcp decrypt first.
                if(filter->is_pes_decrypted && filter->hdcpOps!=NULL) {
                    filter->hdcpOps->decrypt(filter->cookie,filter->pes_private_data, p, p, len);
                }

                is_new_frame = filter->is_new_frame;
                if(len > 0)
                {
                    if(len >= 3 && is_new_frame && (filter->codec_type == DMX_CODEC_H264 || filter->codec_type == DMX_CODEC_H265))
                    {
                        int pos = find_start_code_pos(p, len); //len需要大于等于3
                        if(pos > 0) //找到了但有上一帧残余
                        {
                            //logw("abnormal pes payload, redirect at start offset %d", pos);
                            int tmp = filter->pes_payload_size;
                            filter->pes_payload_size = 0;
                            if(filter->push_es_data(p, pos, 0, filter) != 0) //填充上一帧的剩余数据
                                return -1;
                            filter->pes_payload_size = tmp;
                            p += pos;
                            len -= pos;
                        }
                        else if(pos < 0) //没找到0x000001
                        {
                            //logw("pid '%x' type '%d', abnormal pes payload, redirect ingore %d", filter->pid, filter->codec_type, len);
                            is_new_frame = 0;
                        }
                    }
                    if(filter->push_es_data(p, len, is_new_frame, filter) != 0)
                        return -1;

                    if(filter->codec_type == DMX_CODEC_H264 || filter->codec_type == DMX_CODEC_H265)
                    {
                        if(is_new_frame) //要找且找到了就不再找, 如要找而未找到则继续找
                            filter->is_new_frame = 0;
                    }
                    else //其他编码格式, 新帧标记只有MPEGTS_PESHEADER_FILL后的第一次submit会置起
                    {
                        filter->is_new_frame = 0;
                    }

                    if(filter->pts != (int64_t)-1)
                        filter->pre_pts = filter->pts;

                    filter->pts = -1;
                }

                buf_size = 0;
                break;
            case MPEGTS_PESSKIP:
                buf_size = 0;
                break;
        }
    }

    return 0;
}

static int allocate_filter(int pid, demux_filter_param_t* filter_param, mpegts_context_t* ctx)
{
    if(ctx->pids[pid])
    {
        //pid aready exists
        return -1;
    }

    filter_t *filter = (filter_t*)malloc(sizeof(filter_t));
    memset(filter, 0, sizeof(filter_t));
    filter->pid = pid;
    filter->requestbufcb = filter_param->request_buffer_cb;
    filter->updatedatacb = filter_param->update_data_cb;
    filter->push_es_data = push_es_data;
    filter->codec_type = filter_param->codec_type;
    filter->cookie = filter_param->cookie;

    ctx->pids[pid] = filter;
    
	logd("allocate_filter pid '%x' type '%d', ", filter->pid, filter->codec_type);

    return 0;
}

static int free_filter(int pid, mpegts_context_t* ctx)
{
    filter_t *filter = ctx->pids[pid];
    if(!filter)
    {
        return -1;
    }

    free(filter);
    ctx->pids[pid] = NULL;
    return 0;
}

void* ts_demux_open(void)
{
    mpegts_context_t *mp = (mpegts_context_t*)malloc(sizeof(mpegts_context_t));
    memset(mp, 0, sizeof(mpegts_context_t));

#ifdef WFD_DEBUG_SAVE_TS
		dbg_wfd_ts_fp = fopen(dbg_wfd_ts, "wb");
#endif
    return (void*)mp;
}

int ts_demux_close(void* handle)
{
    int i;
    mpegts_context_t *mp = (mpegts_context_t*)handle;
    for(i = 0; i < NB_PID_MAX; i++)
    {
        if(mp->pids[i])
            free_filter(i, mp);
    }
    free(mp);
    
#ifdef WFD_DEBUG_SAVE_TS
	if(dbg_wfd_ts_fp)
	{
		fclose(dbg_wfd_ts_fp);
		dbg_wfd_ts_fp = NULL;
	}
#endif

    return 0;
}

int ts_demux_open_filter(void* handle, int pid, demux_filter_param_t* filter_param)
{
    mpegts_context_t *mp = (mpegts_context_t*)handle;
    if(pid < NB_PID_MAX)
        allocate_filter(pid, filter_param, mp);
    else
        loge("ts_demux_open_filter error.");
    return 0;
}

int ts_demux_close_filter(void* handle, int pid)
{
    mpegts_context_t *mp = (mpegts_context_t*)handle;
    if((pid < NB_PID_MAX) && mp->pids[pid])
        free_filter(pid, mp);
    else
        loge("ts_demux_close_filter error.");
    return 0;
}

int ts_demux_set_hdcp_ops(void* handle, int pid, WfdHdcpOpsS* ops) {
    mpegts_context_t *mp = (mpegts_context_t*)handle;
    filter_t *filter = mp->pids[pid];

    if(filter!=NULL)
        filter->hdcpOps = ops;
    logd("ts_demux_set_hdcp_ops for pid = %d.", pid);
    return 0;
}

int ts_demux_handle_packets(void* handle, unsigned char* pktdata, int data_size)
{
    mpegts_context_t *mp = (mpegts_context_t*)handle;

#ifdef WFD_DEBUG_SAVE_TS
	fwrite(pktdata, 1, data_size, dbg_wfd_ts_fp);
#endif

    if(mp->rawPacketSize == 0)
    {
        if(data_size >= TS_FEC_PACKET_SIZE * 5) //需要至少5个ts包
            mp->rawPacketSize = get_ts_packet_size(pktdata, data_size);
        else
            mp->rawPacketSize = TS_PACKET_SIZE;
        logd("mp->rawPacketSize: %d", mp->rawPacketSize);
    }
    int pkt_size = mp->rawPacketSize;
    unsigned char *buf_end = pktdata + data_size;
    unsigned char *pkt = pktdata;
    for(; pkt <= (buf_end - pkt_size); pkt += pkt_size)
    {
        if(*pkt != 0x47)
        {
            //resync
            for(; pkt < (buf_end - pkt_size); pkt++) //不能包含等号
            {
                if((*pkt == 0x47) && (*(pkt + pkt_size) == 0x47))
                    break;
            }

            if(*pkt != 0x47) //没找到0x47
                break;
        }

        //开始解析ts包
        int pid = TS_RB16(pkt + 1) & 0x1fff;
        filter_t *filter = mp->pids[pid]; //查找对应pid的filter
        if(!filter) //只处理指定pid流(指定音视频流, 不处理PSI)
            continue;

        //int is_error = pkt[1] & 0x80; //应过滤错误包
        //if(is_error)
        //    continue;

        int is_pes_start = pkt[1] & 0x40; //PES包的开始标记

        //adaptation field
        int afc = (pkt[3] >> 4) & 3;

        //int continuity_counter = pkt[3] & 0x0f; //应同个pid流的连续性来提示丢包, 需记录

        if(afc == 0 || afc == 2) //reserved value
            continue;

        unsigned char *p = pkt + 4;
        if(afc == 3)
        {
            //skip adaptation field
            p += p[0] + 1;
        }

        //if past the end of packet, ignore
        unsigned char *p_end = pkt + pkt_size; //应该取pkt_size
        if(p >= p_end)
            continue;

        if(handle_packet_payload(p, p_end - p, is_pes_start, filter) != 0) //开始解析pes包
            return -1; //丢弃该段ts数据
    }

    int left_bytes = buf_end - pkt; //数据剩余大小, 大于0, 但小于等于一个ts包大小
    return left_bytes;
}
