
#define ZLOG_TAG "Awesome_TPP"

#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <cdx_log.h>
#include <CdxList.h>
#include <ts_buffer.h>
#include <CdxQueue.h>
#include <AwPool.h>

#include <CdxBitReader.h>
#include <ts_types.h>

#include <ts_prog_parser.h>

#define CRC32_SIZE 4

enum 
{
    TPP_STATE_INITINAL = 0,
    TPP_STATE_RUNNING,
    TPP_STATE_STOP,
    TPP_STATE_FINISHED,
    TPP_STATE_TIMEOUT,
    TPP_STATE_FAULT,
};
 
#define TPP_STATUS_ERROR (0x1 << 31)
#define TPP_STATUS_PAT_F (0x1)
#define TPP_STATUS_SDT_F (0x1 << 2)
#define TPP_STATUS_PMT_F (0x1 << 3)

//#define TPP_STATUS_INITINAL 0x0
#define TPP_STATUS_ALLDONE (TPP_STATUS_PAT_F|TPP_STATUS_PMT_F)

#define TS_DESC_TAG_SERVICE 0x48

#define TPP_TIMEOUT_5S 5000000

struct TsPackageS
{
    TSBufferT *rawbuf;
    uint8_t *ptr;
    uint16_t pid;
    CdxListNodeT node;
};

struct TsArrayS
{
    int data_came;
    CdxListT list;
};

struct TsPackagePoolS
{
    struct TsArrayS ts_arrs[8192]; 
};

struct TPPctxS
{
    AwPoolT *pool;

    uint32_t state;
    uint32_t status_flags;
    int64_t start_time;
    
    uint32_t program_cnt;
    uint32_t valid_program_cnt;
    struct ProgramInfoS program_infos[16];
    int program_imap_tab[1024];
    
    struct TsPackagePoolS *tsp_pool;
};

static inline uint16_t TS_GetPID(uint8_t *tsbuf)
{
    return ((tsbuf[1]&0x1f)<<8) + tsbuf[2];
//    return (*(uint16_t *)(tsbuf+1))>>3;
}

static struct TsPackageS *newTsPackage(AwPoolT *pool, TSBufferT *buf, int offset)
{
    struct TsPackageS *tsp;
    
    tsp = Palloc(pool, sizeof(*tsp));

    tsp->rawbuf = TSBufferIncref(buf);
    tsp->ptr = TSBufferGetbufptr(buf) + offset;
    tsp->pid = TS_GetPID(tsp->ptr);
    CDX_LOGD("new TS Package, off:'%d', pid:'%hu'", offset, tsp->pid);
    return tsp;
}

static int deleteTsPackage(AwPoolT *pool, struct TsPackageS *tsp)
{
    TSBufferDecref(tsp->rawbuf);
    Pfree(pool, tsp);
    return 0;
}

static int releaseTspPool(TPPhandlerT *tpp)
{
    int i = 0;
    for (; i < 8192; i++)
    {
        if (tpp->tsp_pool->ts_arrs[i].data_came)
        {
            struct TsPackageS *tsp, *ntsp;
            CdxListForEachEntrySafe(tsp, ntsp, &tpp->tsp_pool->ts_arrs[i].list, node)
            {
                CdxListDel(&tsp->node);
                deleteTsPackage(tpp->pool, tsp);
            }
        }
    }
    return 0;
}

static int deliver2Pool(TPPhandlerT *tpp, TSBufferT *buf)
{
//    TSBufferT *buf = NULL;
    int buf_size;
    uint8_t *buf_ptr;

    buf_size = TSBufferGetbufsize(buf);
    buf_ptr = TSBufferGetbufptr(buf);
    int offset = 0;
    uint16_t pid;
    for (offset = 0; offset < buf_size; offset += 188)
    {
        pid = TS_GetPID(buf_ptr + offset);
        
        if (tpp->tsp_pool->ts_arrs[pid].data_came == 0)
        {
            CdxListInit(&(tpp->tsp_pool->ts_arrs[pid].list));
        }
        if (tpp->tsp_pool->ts_arrs[pid].data_came < 5)
        {
            struct TsPackageS *tsp = newTsPackage(tpp->pool, buf, offset);
            
            CdxListAddTail(&tsp->node, &tpp->tsp_pool->ts_arrs[pid].list);
            tpp->tsp_pool->ts_arrs[pid].data_came++;
        }
        /* else drop */
    }

    TSBufferDecref(buf);
    /* parse PID */

    return 0;
}

static int TS_ParseHeader(CdxBitReaderT *br, struct TS_HeaderS *ts_hdr)
{
    int ret = 0;
    
    ts_hdr->sync_byte = CdxBitReaderGetBits(br, 8);
    if (ts_hdr->sync_byte != 0x47)
    {
        CDX_LOGE("sync_byte err '%u', bad package!", ts_hdr->sync_byte);
        ret = -1;
        goto out;
    }

    ts_hdr->transport_error_indicator = CdxBitReaderGetBits(br, 1);
    if (ts_hdr->transport_error_indicator)
    {
        CDX_LOGW("err package, pls drop it.");
        ret = -1;
        goto out;
    }
    
    CdxBitReaderSkipBits(br, 17);
    /* 
     * we not need this byte:
     * payload_unit_start_indicator:1
     * transport_priority:1
     * PID:13
     * transport_scrambling_control:2
     */ 

    ts_hdr->adaption_field_control = CdxBitReaderGetBits(br, 2);

    CdxBitReaderSkipBits(br, 4); /* continuity_counter:4 */
        
out:
    return ret;
}

static int TS_ParseAdaptfield(CdxBitReaderT *br, struct TS_AdaptionFieldS *adaption_field)
{
    int ret = 0;
    
    adaption_field->adaption_field_length = CdxBitReaderGetBits(br, 8);
    if (adaption_field->adaption_field_length > 183)
    {
        CDX_LOGW("Package error!");
        ret = -1;
        goto out;
    }

    // TODO: not support adaption field now.
    CdxBitReaderSkipBits(br, adaption_field->adaption_field_length);

out:
    return ret;
}

static int TS_ParsePAT(CdxBitReaderT *br, struct PAT_S *pat, AwPoolT *pool)
{
    int ret = 0;
    uint32_t section_off = 0;
    
    pat->table_id = CdxBitReaderGetBits(br, 8);
    if (pat->table_id != 0x00)
    {
        CDX_LOGW("Package error! table_ID: '%x'", pat->table_id);
        ret = -1;
        goto out;
    }

    CdxBitReaderSkipBits(br, 4);
    /*
     * uint32_t section_syntax_indicator:1
     * uint32_t zero:1
     * uint32_t reserved_1:2
     */

    pat->section_length = CdxBitReaderGetBits(br, 12);

    CdxBitReaderSkipBits(br, 40);
    /* 
     *   uint32_t transport_stream_id:16;
     *   uint32_t reserved_2:2;
     *   uint32_t version_number:5;
     *   uint32_t current_next_indicator:1;
     *   uint32_t section_number:8;
     *   uint32_t last_section_number:8;
     */

    section_off = 5;
    
    pat->program_cnt = 0;
    CdxListInit(&pat->program_list);

    for (; section_off < (uint32_t)pat->section_length - CRC32_SIZE; section_off += 4)
    {
        struct PAT_ProgramInfoS *program_info = Palloc(pool, sizeof(*program_info));
        program_info->program_number = CdxBitReaderGetBits(br, 16);
        CdxBitReaderSkipBits(br, 3);  /* reserved:3 */
        program_info->program_map_PID = CdxBitReaderGetBits(br, 13);

        pat->program_cnt++;
        CdxListAddTail(&program_info->node, &pat->program_list);
    }

    pat->CRC_32 = CdxBitReaderGetBits(br, 32);
out:
    return ret;
}

static int TS_ParseDescriptor(CdxBitReaderT *br, uint32_t size, CdxListT *desc_list, AwPoolT *pool)
{
    uint32_t off = 0;
    int desc_cnt = 0;
    
    while (off < size - 2)
    {
        struct TS_DescriptorS *desc = Palloc(pool, sizeof(*desc));
        desc->descriptor_tag = CdxBitReaderGetBits(br, 8);
        desc->descriptor_length = CdxBitReaderGetBits(br, 8);
        memcpy(desc->data, CdxBitReaderData(br), desc->descriptor_length);
        CdxBitReaderSkipBits(br, desc->descriptor_length*8);

        desc_cnt++;
        CdxListAddTail(&desc->node, desc_list);

        off += (desc->descriptor_length + 2);
    }

    return desc_cnt;
}

static int TS_ParsePMT(CdxBitReaderT *br, struct PMT_S *pmt, AwPoolT *pool)
{
    int ret = 0;
    uint32_t sect_off = 0;
    
    pmt->table_id = CdxBitReaderGetBits(br, 8);
    if (pmt->table_id != 0x02)
    {
        CDX_LOGW("Package error!");
        ret = -1;
        goto out;
    }
    
    CdxBitReaderSkipBits(br, 4);
    /*
     * uint32_t section_syntax_indicator:1
     * uint32_t zero:1
     * uint32_t reserved_1:2
     */

    pmt->section_length = CdxBitReaderGetBits(br, 12);
    pmt->program_number = CdxBitReaderGetBits(br, 16);

    CdxBitReaderSkipBits(br, 44);
    /*
      uint32_t reserved_2 :2;
      uint32_t version_number :5;
      uint32_t current_next_indicator :1;
      uint32_t section_number :8;
      uint32_t last_section_number :8;
      uint32_t reserved_3 :3;
      uint32_t PCR_PID :13;
      uint32_t reserved_4 :4;
      */

    pmt->program_info_length = CdxBitReaderGetBits(br, 12);
	if (pmt->program_info_length > 0)
	{
	    CdxListInit(&pmt->descriptor_list);
	    pmt->descriptor_cnt = TS_ParseDescriptor(br, pmt->program_info_length, &pmt->descriptor_list, pool);
	}
	
    CdxListInit(&pmt->stream_list);
    sect_off = 9 + pmt->program_info_length;
    while (sect_off < (uint32_t)pmt->section_length - CRC32_SIZE)
    {
        struct PMT_StreamS *pmt_stream = Palloc(pool, sizeof(*pmt_stream));
        memset(pmt_stream, 0x00, sizeof(*pmt_stream));
        pmt_stream->stream_type = CdxBitReaderGetBits(br, 8);
        CdxBitReaderSkipBits(br, 3);
        pmt_stream->elementary_PID = CdxBitReaderGetBits(br, 13);
        CdxBitReaderSkipBits(br, 4);
        pmt_stream->ES_info_length = CdxBitReaderGetBits(br, 12);

        pmt_stream->descriptor_cnt = 0;
        if (pmt_stream->ES_info_length > 0)
        {
	        CdxListInit(&pmt_stream->descriptor_list);       
	        pmt_stream->descriptor_cnt = TS_ParseDescriptor(br, pmt_stream->ES_info_length, &pmt_stream->descriptor_list, pool);
		}
        CdxListAddTail(&pmt_stream->node, &pmt->stream_list);
        sect_off += (5 + pmt_stream->ES_info_length);
    }

    pmt->CRC_32 = CdxBitReaderGetBits(br, 32);
    
out:
    return ret;
}

static TSStreamTypeT getStreamType(struct PMT_StreamS *pmt_stream)
{
    switch (pmt_stream->stream_type)
    { 
    case PMT_STREAM_TYPE_MPEG1_P2_V:
    case PMT_STREAM_TYPE_MPEG2_P2_V:
    case PMT_STREAM_TYPE_MPEG4_P2_V:
    case PMT_STREAM_TYPE_MPEG_P10_V_AVC:     
    case PMT_STREAM_TYPE_MPEG2_P2_V_DC:
    case PMT_STREAM_TYPE_V_VC1:
        return TS_STREAM_TYPE_VIDEO;
        
    case PMT_STREAM_TYPE_MPEG1_P3_A:
    case PMT_STREAM_TYPE_MPEG2_P3_A_MP3:
    case PMT_STREAM_TYPE_MPEG2_P7_A_AAC:
    case PMT_STREAM_TYPE_MPEG4_P3_A:   
    case PMT_STREAM_TYPE_A_AC3:    
    case PMT_STREAM_TYPE_A_EAC3:
    case PMT_STREAM_TYPE_MPEG2_P1_PRI:
        return TS_STREAM_TYPE_AUDIO;
        
    default:
        return TS_STREAM_TYPE_UNKNOW;
        
    }
    return TS_STREAM_TYPE_UNKNOW;
}

static int showProgramInfo(TPPhandlerT *tpp)
{
    int i = 0;
    CDX_LOGD("Program num '%u'", tpp->program_cnt);
    for (; i < (int)tpp->program_cnt; i++)
    {
        CDX_LOGD("Program[%u]: pid:'%u' name:'%s' "
        		"vpid:'0x%x' c:'%x'"
        		"apid:'0x%x' anum'%u'",
        		i, tpp->program_infos[i].pmt_pid, tpp->program_infos[i].name, 
                tpp->program_infos[i].video_pid, tpp->program_infos[i].video_codec_type,
                tpp->program_infos[i].audio_pid[0], tpp->program_infos[i].audio_num);
    }
    return 0;
}

static int parseTSPackage_PAT(TPPhandlerT *tpp, uint8_t *buf)
{
    int ret = 0;
    CdxBitReaderT *br = NULL;
    struct TS_HeaderS ts_hdr;
    struct TS_AdaptionFieldS adaption_field;
    struct PAT_S pat;

    br = CdxBitReaderCreate(buf, 188);
    if (!br)
    {
        CDX_LOGE("err.");
        goto out;
    }
    
    ret = TS_ParseHeader(br, &ts_hdr);
    if (ret != 0)
    {
        CDX_LOGE("err.");
        goto out;
    }

    if (ts_hdr.adaption_field_control & 0x10)
    {
        ret = TS_ParseAdaptfield(br, &adaption_field);
        if (ret != 0)
        {
			CDX_LOGE("err.");
            goto out;
        }
    }
    
    CdxBitReaderSkipBits(br, 8); /* pointer_field:8 */

    ret = TS_ParsePAT(br, &pat, tpp->pool);
    if (ret != 0)
    {
        CDX_LOGE("err.");
        goto out;
    }

    tpp->program_cnt = pat.program_cnt;
    struct PAT_ProgramInfoS *program;
    uint32_t index = 0;
    CdxListForEachEntry(program, &pat.program_list, node)
    {
        tpp->program_infos[index].index = program->program_number;
        tpp->program_infos[index].pmt_pid = program->program_map_PID;

        CDX_LOGD("program->program_number = %d", program->program_number);
        tpp->program_imap_tab[program->program_number] = index;
        index++;
    }
    CDX_CHECK(index == tpp->program_cnt);
//    showProgramInfo(tpp);
out:
    if (br)
    {
        CdxBitReaderDestroy(br);
    }
    return ret;
}

static int parseTSPackage_PMT(TPPhandlerT *tpp, uint8_t *buf)
{
    int ret = 0;
    CdxBitReaderT *br = NULL;
    struct TS_HeaderS ts_hdr;
    struct TS_AdaptionFieldS adaption_field;
    struct PMT_S pmt;

    memset(&pmt, 0x00, sizeof(pmt));

    br = CdxBitReaderCreate(buf, 188);
    if (!br)
    {
        CDX_LOGE("err.");
        goto out;
    }
    
    ret = TS_ParseHeader(br, &ts_hdr);
    if (ret != 0)
    {
        CDX_LOGE("err.");
        goto out;
    }
    
    if (ts_hdr.adaption_field_control & 0x2)
    {
        ret = TS_ParseAdaptfield(br, &adaption_field);
        if (ret != 0)
        {
			CDX_LOGE("err.");
            goto out;
        }
    }
    CdxBitReaderSkipBits(br, 8); /* pointer_field:8 */

    ret = TS_ParsePMT(br, &pmt, tpp->pool);
    if (ret != 0)
    {
        CDX_LOGE("err.");
        goto out;
    }

    CDX_CHECK(tpp->program_cnt > 0);

    int i = tpp->program_imap_tab[pmt.program_number];
    struct PMT_StreamS *stream;
    CdxListForEachEntry(stream, &pmt.stream_list, node)
    {
        TSStreamTypeT stream_type = getStreamType(stream);
        if (stream_type == TS_STREAM_TYPE_VIDEO)
        {
            tpp->program_infos[i].video_pid = stream->elementary_PID;
			tpp->program_infos[i].video_codec_type = stream->stream_type;
        }
        else if (stream_type == TS_STREAM_TYPE_AUDIO)
        {
            int audio_stream_index = tpp->program_infos[i].audio_num;
            tpp->program_infos[i].audio_pid[audio_stream_index] = stream->elementary_PID;
            tpp->program_infos[i].audio_num++;
			tpp->program_infos[i].audio_codec_type = stream->stream_type;
        }
        else
        {
            CDX_LOGW("unknow support stream '%u'", stream->stream_type);
        }
    }

    if ((!tpp->program_infos[i].video_pid) && (tpp->program_infos[i].audio_num == 0))
    {
        CDX_LOGE("Parse PMT finish, but no stream found. ");
        ret = -1;
    }

out:
    if (br)
    {
        CdxBitReaderDestroy(br);
    }
    
    return ret;
}

static int doParse(TPPhandlerT *tpp)
{
    int ret;
    
    /* parse PAT */
    if (!(tpp->status_flags & TPP_STATUS_PAT_F))
    {
        if (tpp->tsp_pool->ts_arrs[PAT_PID].data_came)
        {
            struct TsPackageS *tsp;
            CdxListForEachEntry(tsp, &tpp->tsp_pool->ts_arrs[PAT_PID].list, node)
            {
                ret = parseTSPackage_PAT(tpp, tsp->ptr);
                if (ret == 0)
                {
                    CDX_LOGD("Parse PAT finish...");
                    tpp->status_flags |= TPP_STATUS_PAT_F;
                    break;
                }
            }
        }
    }

    /* parse PMT */
    if ((tpp->status_flags & TPP_STATUS_PAT_F) 
        && (!(tpp->status_flags & TPP_STATUS_PMT_F)))
    {
        uint32_t i = 0;
        for (; i < tpp->program_cnt; i++)
        {
            uint32_t pid = tpp->program_infos[i].pmt_pid;
            
            if ((!tpp->program_infos[i].valid) && tpp->tsp_pool->ts_arrs[pid].data_came)
            {
                struct TsPackageS *tsp;
                CdxListForEachEntry(tsp, &tpp->tsp_pool->ts_arrs[pid].list, node)
                {
                    ret = parseTSPackage_PMT(tpp, tsp->ptr);
                    if (ret == 0)
                    {
                        tpp->program_infos[i].valid = 1;
                        tpp->valid_program_cnt++;
                        break;
                    }
                }
            }
            
        }

        if (tpp->valid_program_cnt == tpp->program_cnt) /* ALL PMT parse finish */
        {
            CDX_LOGD("Parse PMT finish...");
            tpp->status_flags |= TPP_STATUS_PMT_F;
        }
    }

    return 0;
}

int TPP_Fill(TPPhandlerT *tpp, TSBufferT *buf)
{
    int ret;

    ret = deliver2Pool(tpp, buf);

    ret = doParse(tpp);

	if (tpp->status_flags == TPP_STATUS_ALLDONE)
	{
		CDX_LOGD("Awesome Program search do finish...");
		showProgramInfo(tpp);
		tpp->state = TPP_STATE_FINISHED; 
		return tpp->valid_program_cnt;
	}
	
	return 0;
}

int TPP_GetProgram(TPPhandlerT *tpp, struct ProgramInfoS *pi)
{
	memcpy(pi, tpp->program_infos, sizeof(struct ProgramInfoS)*tpp->valid_program_cnt);
	return tpp->valid_program_cnt;
}

int TPP_Destroy(TPPhandlerT *tpp)
{
	releaseTspPool(tpp);
	Pfree(tpp->pool, tpp->tsp_pool);

	CDX_LOGD("too many list, we use pool to free, may see some leak warn, but it's OK");
	AwPoolDestroy(tpp->pool);
    free(tpp);
    return 0;
}

TPPhandlerT *TPP_Instance()
{
    struct TPPctxS *tpp = malloc(sizeof(*tpp));

    memset(tpp, 0x00, sizeof(*tpp));
    
    tpp->pool = AwPoolCreate(NULL);
    
    tpp->tsp_pool = Palloc(tpp->pool, sizeof(struct TsPackagePoolS));
    memset(tpp->tsp_pool, 0x00, sizeof(struct TsPackagePoolS));
    
    
    return tpp;
}
