//#define LOG_NDEBUG 0
#define LOG_TAG "WFDPlayer2"

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/time.h>

/* frameworks/av/media/libcedarx/libcore/base */
#include <cdx_log.h>
#include <CdxQueue.h>
#include <wfd_player.h>



extern "C" {
#include "tsdemux.h"
#include "tsdemux_i.h"
#include "ts_buffer.h"
#include "ts_prog_parser.h"
}

#include "layerControl.h"
#include <WFDSink.h>

//#include "WFDRTSPThread.h"
#include "WFDPlayer2.h"
#include <miracast2.h>
#include "iniparserapi.h"
#include "AwHdcpPlugin.h"
#include "tsound_ctrl.h"

#define ES_VIDEO_BUFFER_SIZE (512 * 1024)
#define ES_AUDIO_BUFFER_SIZE (1 * 1024)

extern "C"
{
	extern LayerCtrl* WFD_LayerCreate_DE();
	//extern SoundCtrl* TinaSoundDeviceInit();
	extern SoundCtrl* TSoundDeviceCreate(AudioFrameCallback callback, void* pUser);
	extern SoundCtrl* SoundDeviceCreate();
}

struct SinkParamS
{
	WFDSink *sink;
	int enableHDCP;
	char const *rtspURL;
	FeedDataCB1 feedDataCB;
	RTSPStatusHandler1 rtspStatusHandler;
	void * wfdManager;
	const char *videoFormat;
};
static struct SinkParamS sp;
	
static int SemTimedWait(sem_t* sem, int64_t time_ms);
struct WFDPCtxS
{
	AwPoolT *pool;

	WFDPInterface *iter;

    WFDSink *sink;
//    WFDRTSPThread *sink_thread;

	Player *mPlayer;

	CdxQueueT *rtp_buf_queue;
	CdxQueueT *tpp_buf_queue;

	pthread_cond_t buf_cond;
	pthread_mutex_t buf_mutex;
	
	LayerCtrl *disp;
	SoundCtrl *snd;

	pthread_t demux_pid;
	pthread_t wfdsink_pid;

	TPPhandlerT *tpp;
	void *tsdmx_hdr;
//	CdxListT tpp_buf_list;

//	int parse_program_fin;
	struct ProgramInfoS pi[8];
	int p_num;
	demux_filter_param_t ts_vfilter;
	demux_filter_param_t ts_afilter;
    char *mVideoBufForTsDemux;
    char *mAudioBufForTsDemux;

	int64_t ptsSecs;
	int exit_flag;
    sem_t exit_flag_sem;
	int bPlayerSetup;

	//add for wifidisplay hdcp
	int mEnableHdcp;
	AwHdcpPlugin* mHDCP;
	WfdHdcpOpsS mHDCPOps;
	void* userCookie ;
};

int WFDPlayer2::dmxcb_InitHdcpModule(void *handle)
{
	WFDPlayer2* player = (WFDPlayer2*) handle;
	CDX_CHECK(player);
	return player->PlayerHdcpInit();
}

int WFDPlayer2::dmxcb_DeInitHdcpModule(void *handle)
{
	WFDPlayer2* player = (WFDPlayer2*) handle;
	CDX_CHECK(player);
	return player->PlayerHdcpDeInit();
}

int WFDPlayer2::dmxcb_DecrypData(void *handle, const uint8_t privateData[16],
						const uint8_t *in, uint8_t *out, uint32_t len) {
	WFDPlayer2* player = (WFDPlayer2*) handle;
	CDX_CHECK(player);
	return player->PlayerDecryptHdcpData(privateData, in, out, len);
}

int WFDPlayer2::PlayerHdcpInit()
{
	CDX_LOGD("++++++++++++++++++++");
	return OK;
}

int WFDPlayer2::PlayerHdcpDeInit()
{
	CDX_LOGD("--------------------");
	return OK;
}

int WFDPlayer2::PlayerDecryptHdcpData(const uint8_t privateData[16],
                          const uint8_t *in, uint8_t *out, uint32_t len)
{
	status_t err = INVALID_OPERATION;
	if(ctx->mHDCP!=NULL) {

		CDX_LOGV("start to decrypt hdcp pes data! len = %d", len);

		uint32_t streamCTR = ((privateData[1] & 0x06) >> 1) << 30 | //get bit 31..30
					privateData[2] << 22 | // get bit 29..22
					((privateData[3] & 0xFE) >> 1) << 15 | // get bit 21..15
					privateData[4] << 7 | // get bit 14..7
					((privateData[5] & 0xFE) >> 1); // get bit 6..0

		uint64_t outputCTR = ((privateData[7] & 0x1E) >> 1) << 28 |  //get bit 63..60
				privateData[8] << 20 | //get bit 59..52
				((privateData[9] & 0xFE) >> 1) << 13 | //get bit 51..45
				privateData[10] << 5 | //get bit 44..37
				((privateData[11] & 0xF8) >> 3); //get bit 36..32 ((privateData[11] & 0xF1) >> 3)

		outputCTR = outputCTR << 32;

		outputCTR |= ((privateData[11] & 0x06) >> 1) << 30 | //get bit 31..30 ((privateData[11] & 0x07) >> 1) << 30 | //get bit 31..30
				privateData[12] << 22 | // get bit 29..22
				((privateData[13] & 0xFE) >> 1) << 15 | // get bit 21..15
				privateData[14] << 7 | // get bit 14..7
				((privateData[15] & 0xFE) >> 1);// get bit 6..0

		err = ctx->mHDCP->decrypt(
				in, len,
				streamCTR  /* streamCTR */,
				outputCTR,
				out);
	}
	return err;
}

static int PlayerCB_Process(void *p, int messageId, void* param)
{
    WFDPlayer2 *player = (WFDPlayer2 *)p;
    return player->EventNotify(messageId, param);
}

WFDPlayer2::WFDPlayer2(WFDPInterface *iter)
{
	ctx = (struct WFDPCtxS *)malloc(sizeof(struct WFDPCtxS));
	memset(ctx, 0x00, sizeof(struct WFDPCtxS));
	ctx->iter = iter;
	ctx->exit_flag = 0;
	ctx->pool = AwPoolCreate(NULL);
	//ctx->sink = NULL;
	ctx->rtp_buf_queue = CdxQueueCreate(ctx->pool);
	ctx->tpp_buf_queue = CdxQueueCreate(ctx->pool);
    sem_init(&ctx->exit_flag_sem, 0, 0);

	pthread_cond_init(&ctx->buf_cond, NULL);
	pthread_mutex_init(&ctx->buf_mutex, NULL);
	ctx->bPlayerSetup = 0;
	ctx->mEnableHdcp = 1;
	
	// if(property_get("wfdsink.enable.hdcp", value, NULL) && (!strcmp(value, "1") || !strcasecmp(value, "true")))
	// {
	// 	ctx->mEnableHdcp = 1;
	// } else {
	// 	ctx->mEnableHdcp = 0;
	// }
	// //Print version and commit id, then record them into property
	// CDX_LOGI("********** WFDPlayer2 info *****************");
	// // CDX_LOGI("WFDPlayer2 version: (%s)", WFD_LIB_VERSION_CODE);
	// // CDX_LOGI("WFDPlayer2 commit id: (%s)", WFD_LIB_COMMIT_ID);
    // CDX_LOGI("WFDPlayer2 hdcp:%d", ctx->mEnableHdcp);
	// CDX_LOGI("************* end **************************");

	// if(property_set(WFD_LIB_VERSION_PROPERTY_KEY, WFD_LIB_VERSION_CODE)!=0) {
	// 	CDX_LOGW("Fail to record wfd version info into property!");
	// }
	// if(property_set(WFD_LIB_COMMIT_PROPERTY_KEY, WFD_LIB_COMMIT_ID)!=0) {
	// 	CDX_LOGW("Fail to record wfd commit info into property!");
	// }	
}

WFDPlayer2::~WFDPlayer2()
{
    if (sp.sink)
    {
        free(sp.sink);
        sp.sink = NULL;
    }

	pthread_mutex_destroy(&ctx->buf_mutex);
	pthread_cond_destroy(&ctx->buf_cond);
    sem_destroy(&ctx->exit_flag_sem);
    AwPoolDestroy(ctx->pool);
	free(ctx);
    CDX_LOGD("~WFDPlayer2.");
}

int WFDPlayer2::Setup(void *disp_hdr, void *snd_hdr)
{
	int ret = 0;
	
	CDX_LOGI("WFD2 Setup");

//	CDX_CHECK(disp_hdr != NULL);

    ctx->mPlayer = WFD_PlayerCreate();
    if (ctx->mPlayer == NULL)
    {
        CDX_LOGE("PlayerCreate faied");
        return -1;
    }

	WFD_PlayerSetCallback(ctx->mPlayer, PlayerCB_Process, this);

	/* init disp */
	ctx->disp = WFD_LayerCreate_DE();
	
	CDX_LOGI("WFD2 Setup");

    WFD_PlayerSetWindow(ctx->mPlayer, ctx->disp);

#if 0
	/* init snd */
    if (/*GetConfigParamterInt("sound_ahub", 0) == 1*/0) /* use AHub sound control */
    {
        ctx->snd = SoundDeviceCreate();//for H616 who has Ahub
    }
    else
    {
        ctx->snd = TinaSoundDeviceInit();//for h3 without Ahub
    }
#endif

    //ctx->snd = TinaSoundDeviceInit();
	ctx->snd = TSoundDeviceCreate(NULL, NULL);
    WFD_PlayerSetAudioSink(ctx->mPlayer, ctx->snd);

    ctx->bPlayerSetup = 1;

    return 0;
}

static enum EAUDIOCODECFORMAT MpegStreamtype2CdxCodectype(uint32_t stream_type)
{
	switch (stream_type)
	{
		case PMT_STREAM_TYPE_MPEG2_P7_A_AAC:
			return AUDIO_CODEC_FORMAT_MPEG_AAC_LC;
		default:
			CDX_LOGE("unknow stream type '0x%x'", stream_type);
			return AUDIO_CODEC_FORMAT_UNKNOWN;
	}
	return AUDIO_CODEC_FORMAT_UNKNOWN;
}

int WFDPlayer2::PlayerInitAVandStart()
{
    int    ret = 0;

    //* set audio stream info.
    AudioStreamInfo streamInfo;
    memset(&streamInfo, 0, sizeof(streamInfo));

    streamInfo.eCodecFormat          = MpegStreamtype2CdxCodectype(ctx->pi[0].audio_codec_type);
    streamInfo.nChannelNum           = 2;
    streamInfo.nSampleRate           = 44100;
    streamInfo.nCodecSpecificDataLen = 0;
    streamInfo.pCodecSpecificData    = NULL;
    streamInfo.nFlags = (streamInfo.eCodecFormat == AUDIO_CODEC_FORMAT_MPEG_AAC_LC) ? 1 : 0;

    CDX_LOGD("******* audio info *******************");
    CDX_LOGD("audio streamInfo.eCodecFormat(%d)", streamInfo.eCodecFormat);
    CDX_LOGD("streamInfo.nChannelNum(%d)",        streamInfo.nChannelNum);
    CDX_LOGD("streamInfo.nSampleRate(%d)",        streamInfo.nSampleRate);
    CDX_LOGD("streamInfo.nBitsPerSample(%d)",     streamInfo.nBitsPerSample);
    CDX_LOGD("streamInfo.nCodecSpecificDataLen(%d)", streamInfo.nCodecSpecificDataLen);
    CDX_LOGD("************* end *******************");

    ret = WFD_PlayerSetAudioStreamInfo(ctx->mPlayer, &streamInfo, 1, 0);
	if (ret != 0)
	{
		CDX_LOGE("WFD_PlayerSetVideoStreamInfo err ");
		return -1;
	}

    VideoStreamInfo vsi;

    //* set video stream info.
    memset(&vsi,0,sizeof(vsi));
    vsi.eCodecFormat = VIDEO_CODEC_FORMAT_H264;
    ret = WFD_PlayerSetVideoStreamInfo(ctx->mPlayer, &vsi);
	if (ret != 0)
	{
		CDX_LOGE("WFD_PlayerSetVideoStreamInfo err ");
		return -1;
	}
    SetCastMode(CAST_MIRACAST_MODE);

    CDX_LOGD("*********** video info **********************");
    CDX_LOGD("vFmt:%d", vsi.eCodecFormat);
    CDX_LOGD("**********************************************");

	ret = WFD_PlayerStart(ctx->mPlayer);
    return ret;
}


void WFDPlayer2::DemuxProc()
{
	TSBufferT *buf;
	int ret;

	/* parse program */
	ctx->tpp = TPP_Instance();
	while (1)
	{
		if (ctx->exit_flag)
		{
			CDX_LOGW("exit request...");
			return;
		}
		buf = (TSBufferT *)CdxQueuePop(ctx->rtp_buf_queue);
		if (!buf)
		{
			usleep(100000); /* 100ms */
			continue;
		}

		CdxQueuePush(ctx->tpp_buf_queue, buf);
		ret = TPP_Fill(ctx->tpp, buf);
		if (ret > 0)
		{
			ctx->p_num = TPP_GetProgram(ctx->tpp, ctx->pi);
			CDX_CHECK(ret == ctx->p_num);
			CDX_LOGD("program num '%d'", ctx->p_num);
			break;
		}
	}
	while(!ctx->bPlayerSetup)
	{
           CDX_LOGD("wait for player setup.");
	    usleep(10*1000);
        }
	ret = PlayerInitAVandStart();
	if (ret != 0)
	{
		CDX_LOGE("PlayerInitAVandStart err ");
		return ;
	}
	/* prepare ts filter */
	/* video */
	if (ctx->pi[0].video_codec_type == PMT_STREAM_TYPE_MPEG_P10_V_AVC)
	{
		ctx->ts_vfilter.codec_type = DMX_CODEC_H264;
	}
	else
	{
		ctx->ts_vfilter.codec_type = DMX_CODEC_UNKOWN;
		CDX_LOGE("codec:'%d' not support, maybe unexpect error...", 
			ctx->pi[0].video_codec_type);
	}
	ctx->ts_vfilter.request_buffer_cb = dmxcb_RequestVideoBuffer;
	ctx->ts_vfilter.update_data_cb	  = dmxcb_UpdateVideoData;
	ctx->ts_vfilter.cookie = (void*)this;
	ret = ts_demux_open_filter(ctx->tsdmx_hdr, ctx->pi[0].video_pid, &ctx->ts_vfilter);

	/* audio */
	ctx->ts_afilter.codec_type = DMX_CODEC_UNKOWN; /* audio */
	ctx->ts_afilter.request_buffer_cb = dmxcb_RequestAudioBuffer;
	ctx->ts_afilter.update_data_cb	  = dmxcb_UpdateAudioData;
	ctx->ts_afilter.cookie = (void*)this;
	ret = ts_demux_open_filter(ctx->tsdmx_hdr, ctx->pi[0].audio_pid[0], &ctx->ts_afilter);

	if(ctx->mEnableHdcp) {
		ts_demux_set_hdcp_ops(ctx->tsdmx_hdr, ctx->pi[0].video_pid, &ctx->mHDCPOps);
		ts_demux_set_hdcp_ops(ctx->tsdmx_hdr, ctx->pi[0].audio_pid[0], &ctx->mHDCPOps);
	}
	
	/* push tpp_buf */
	int tpp_buf_count = 0;
	while (2)
	{
		buf = (TSBufferT *)CdxQueuePop(ctx->tpp_buf_queue);
		if (!buf)
		{
			CDX_LOGD("tpp buf end, cnt: '%d'", tpp_buf_count);
			break;
		}
		tpp_buf_count++;

		ret = ts_demux_handle_packets(ctx->tsdmx_hdr,
						TSBufferGetbufptr(buf), TSBufferGetbufsize(buf));
	}

	while (3)
	{
		if (ctx->exit_flag)
		{
			CDX_LOGW("exit request...");
			break;
		}
		buf = (TSBufferT *)CdxQueuePop(ctx->rtp_buf_queue);
		if (!buf)
		{
//		    ts.tv_sec += WAIT_TIME_SECONDS;

#if 1
//			CDX_LOGD("no buf come, wait...");
			struct timespec   ts;
		  	struct timeval    tp;

		    gettimeofday(&tp, NULL);

		    /* Convert from timeval to timespec */
		    ts.tv_sec  = tp.tv_sec + 1; /* 1s timeout */
		    ts.tv_nsec = tp.tv_usec * 1000;
			pthread_mutex_lock(&ctx->buf_mutex);
			pthread_cond_timedwait(&ctx->buf_cond, &ctx->buf_mutex, &ts);
			pthread_mutex_unlock(&ctx->buf_mutex);

			int64_t before;
			before = tp.tv_usec;
//		    gettimeofday(&tp, NULL);
//			CDX_LOGD("buf come wait '%.3f'", (tp.tv_usec - before)/1000000.0);

#else
			usleep(20000); /* 20ms */
#endif
			continue;
		}
		ret = ts_demux_handle_packets(ctx->tsdmx_hdr,
						TSBufferGetbufptr(buf), TSBufferGetbufsize(buf));
		TSBufferDecref(buf);
	}
    sem_post(&ctx->exit_flag_sem);
	return ;
}

void *WFDPlayer2::DemuxThreadProc(void *param)
{
	WFDPlayer2 *player = (WFDPlayer2 *)param;
	player->DemuxProc();
	return NULL;
}

static int streamcb_StatusHandler(void* cookie, WFD_RTSP_Status status)
{
    WFDPlayer2 *player = (WFDPlayer2 *)cookie;
    CDX_CHECK(player);
	return player->StreamStatusHandler((int)status);
}


static void *WFDSinkThread(void *param)
{
	int ret;
	struct SinkParamS *sp = (struct SinkParamS *)param;
	
	CDX_LOGD("WFDSinkThread start...");
	
	ret = startWFDSink(&sp->sink, sp->enableHDCP, sp->rtspURL,
				sp->feedDataCB, sp->rtspStatusHandler, sp->wfdManager, sp->videoFormat);
				
	CDX_LOGD("WFDSinkThread end...");
	
	return NULL;
}

int WFDPlayer2::Start(const char *url, int isenablehdcp, const char *videoFormat)
{
	CDX_LOGD("WFDPlayer2::Start");

	int ret = 0;

	/* rtsp */
	sp.sink = NULL;
	sp.enableHDCP = isenablehdcp;
	sp.rtspURL = url;
	sp.feedDataCB = streamcb_FeedData;
	sp.rtspStatusHandler = streamcb_StatusHandler;
	sp.wfdManager = (void*)this;
	sp.videoFormat = videoFormat;

	ret = pthread_create(&ctx->wfdsink_pid, NULL, WFDSinkThread, &sp);
	if (ret != 0)
	{
		CDX_LOGD("startWFDSink failure...");
		return -1;
	}

	/* ts demux */
    ctx->tsdmx_hdr = ts_demux_open();
    if (ctx->tsdmx_hdr == NULL)
    {
        CDX_LOGE("ts_demux_open faied");
        return -1;
    }
	
	ret = pthread_create(&ctx->demux_pid, NULL, DemuxThreadProc, this);

	ctx->mEnableHdcp = isenablehdcp;

	if(ctx->mEnableHdcp) {

		// sp<IServiceManager> mServiceManager= defaultServiceManager();
		// sp<IBinder> mBinder = mServiceManager->getService(String16("media.player"));
		// sp<IMediaPlayerService> mMediaPlayerService = interface_cast<IMediaPlayerService>(mBinder);
		// CDX_CHECK(mMediaPlayerService != NULL);

		//ctx->mHDCP = mMediaPlayerService->makeHDCP(false);

		ctx->mHDCP = createHDCPModuleForDecryption(ctx->userCookie);

		if (ctx->mHDCP == NULL) {
			CDX_LOGE("Fail to init hdcp module.");
			return -1;
		}

		CDX_LOGD("start set hdcp operation!");

		//Init hdcp operation for ts demux.
		ctx->mHDCPOps.init = dmxcb_InitHdcpModule;
		ctx->mHDCPOps.deinit = dmxcb_DeInitHdcpModule;
		ctx->mHDCPOps.decrypt = dmxcb_DecrypData;

		CDX_LOGD("start set hdcp observer!");

		char localIP[] = "0.0.0.0";
		unsigned int listenPort = 20500;

		CDX_LOGD("setObserver call ok!");
		if(ctx->mHDCP->initAsync(localIP, listenPort) == 0)
			CDX_LOGD("HDCP init async successfully!");
		CDX_LOGD("initAsync call ok!");
	}

	return ret;
}

int WFDPlayer2::Stop()
{
	CDX_LOGD("WFDPlayer2::Stop");
	ctx->exit_flag = 1;
	CDX_LOGD("WFDPlayer2::Stop trace...exit_flag_sem");
    SemTimedWait(&ctx->exit_flag_sem, 200);

    if (sp.sink)
    {
	    CDX_LOGD("WFDPlayer2::Stop trace... stopWFDSink");
        stopWFDSink(&sp.sink);
        free(sp.sink);
        sp.sink = NULL;
    }
	pthread_join(ctx->wfdsink_pid, NULL);
	CDX_LOGD("WFDPlayer2::Stop trace...wfdsink_pid");

	pthread_join(ctx->demux_pid, NULL);
	CDX_LOGD("WFDPlayer2::Stop trace...demux_pid");

#if 0
    if (ctx->sink_thread)
    {
        ctx->sink_thread->requestExit();
        delete ctx->sink_thread;
        ctx->sink_thread = NULL;
    }
#endif
	if (ctx->mPlayer)
	{
		WFD_PlayerStop(ctx->mPlayer);
		WFD_PlayerDestroy(ctx->mPlayer);
		ctx->mPlayer = NULL;
	}
	if (ctx->tpp)
	{
		TPP_Destroy(ctx->tpp);
	}

	if (ctx->tsdmx_hdr)
	{
		ts_demux_close(ctx->tsdmx_hdr);
		ctx->tsdmx_hdr = NULL;
	}

    TSBufferT *rtp_buf = NULL;
	while ((rtp_buf = (TSBufferT *)CdxQueuePop(ctx->tpp_buf_queue)) != NULL)
	{
		TSBufferDestroy(rtp_buf);
	}

	while ((rtp_buf = (TSBufferT *)CdxQueuePop(ctx->rtp_buf_queue)) != NULL)
	{
		TSBufferDestroy(rtp_buf);
	}

	if(ctx->mEnableHdcp) {
		ctx->mHDCP->shutdownAsync();
		CDX_LOGD("shut down hdcp connection!");
	}
    ctx->bPlayerSetup = 0;
	return 0;
}

int WFDPlayer2::Play()
{
	int ret = 0;
	CDX_LOGD("WFDPlayer2::Play");
//    ret = WFD_PlayerStart(ctx->mPlayer);
    return ret;
}

int WFDPlayer2::StreamStatusHandler(int status)
{
	CDX_CHECK(ctx->iter);
	return ctx->iter->handleRTSPStatus((int)status);
}

int WFDPlayer2::StreamFeedData(void* buffer, int len)
{
    TSBufferT *rtp_buf;

	rtp_buf = TSBufferCreate(ctx->pool, (uint8_t *)buffer, len);

	CdxQueuePush(ctx->rtp_buf_queue, rtp_buf);

	pthread_mutex_lock(&ctx->buf_mutex);
	pthread_cond_signal(&ctx->buf_cond);
	pthread_mutex_unlock(&ctx->buf_mutex);
	return 0;
}

int WFDPlayer2::streamcb_FeedData(void *cookie, void* buffer, int len)
{
    WFDPlayer2 *player = (WFDPlayer2 *)cookie;
    CDX_CHECK(player);
    return player->StreamFeedData(buffer, len);
}

int WFDPlayer2::RequestVideoBuffer(void *param)
{
	int ret = 0;
	void *pBuf0 = NULL;
	int nBufSize0 = 0;
	void *pBuf1 = NULL;
	int nBufSize1 = 0;
    md_buf_t *mdBuf;

    mdBuf = (md_buf_t *)param;

	while (1)
	{
		ret = WFD_PlayerRequestStreamBuffer(ctx->mPlayer, ES_VIDEO_BUFFER_SIZE,
				&pBuf0, &nBufSize0, &pBuf1, &nBufSize1, MEDIA_TYPE_VIDEO, 0);
		if (ret == 0)
		{
			ctx->mVideoBufForTsDemux = (char*)pBuf0;
            mdBuf->buffer = (unsigned char*)pBuf0;
            mdBuf->bufferSize = nBufSize0;
            break;
		}
		else
		{
			CDX_LOGD("waiting for video stream buffer size %d", ES_VIDEO_BUFFER_SIZE);
			if (ctx->exit_flag)
			{
				CDX_LOGW("exit request...");
				return 0;
			}
			usleep(10000); /* 10ms */
		}
	}

	return 0;
}

int WFDPlayer2::dmxcb_RequestVideoBuffer(void *param, void *cookie)
{
    WFDPlayer2 *player = (WFDPlayer2 *)cookie;
    CDX_CHECK(player);
	player->RequestVideoBuffer(param);

	return 0;
}

static int64_t Utils_GetTimeUS() 
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return (int64_t)tv.tv_sec * 1000000ll + tv.tv_usec;
}

int WFDPlayer2::UpdateVideoData(void *param)
{
	int ret;
    int bIsFirst;
    int bIsLast;
    int64_t nPts;
    md_data_info_t *mdDataInfo;

    mdDataInfo = (md_data_info_t*)param;

    bIsFirst = (mdDataInfo->ctrlBits & FIRST_PART_BIT) ? 1 : 0;
    bIsLast  = (mdDataInfo->ctrlBits & LAST_PART_BIT) ? 1 : 0;
    nPts     = (mdDataInfo->ctrlBits & PTS_VALID_BIT) ? mdDataInfo->pts : -1;

    MediaStreamDataInfo dataInfo;
    memset(&dataInfo, 0, sizeof(MediaStreamDataInfo));
    int                 nStreamIndex = 0;

    dataInfo.pData  = ctx->mVideoBufForTsDemux;
    dataInfo.nLength      = mdDataInfo->dataLen;
    dataInfo.nPts         = nPts;  //* input pts is in 90KHz.
    dataInfo.bIsFirstPart = bIsFirst;
    dataInfo.bIsLastPart  = bIsLast;

	int64_t ptsSecs = (int64_t)(nPts/1000000);
	if (ctx->ptsSecs <= ptsSecs-10) //print every 10s
	{
		ctx->ptsSecs = ptsSecs;
		CDX_LOGD("pic_trace: (%.3f s) @(%.3f ms)queue into player", nPts/1000000.0, 
				Utils_GetTimeUS()/1000.0);
	}
    logv("stream frame num %d, stream data size %d.", WFD_PlayerGetVideoStreamFrameNum(ctx->mPlayer), WFD_PlayerGetVideoStreamDataSize(ctx->mPlayer));
    ret = WFD_PlayerSubmitStreamData(ctx->mPlayer, &dataInfo, MEDIA_TYPE_VIDEO, nStreamIndex);
	return 0;
}

int WFDPlayer2::dmxcb_UpdateVideoData(void *param, void *cookie)
{
    WFDPlayer2 *player = (WFDPlayer2 *)cookie;
    CDX_CHECK(player);
	player->UpdateVideoData(param);
	return 0;
}

int WFDPlayer2::RequestAudioBuffer(void *param)
{
    md_buf_t *mdBuf;

    mdBuf = (md_buf_t*)param;

    int             ret = 0;
    void*           pBuf0 = NULL;
    int             nBufSize0 = 0;
    void*           pBuf1 = NULL;
    int             nBufSize1 = 0;

    while(1)
    {
        ret = WFD_PlayerRequestStreamBuffer(ctx->mPlayer, ES_AUDIO_BUFFER_SIZE,
                &pBuf0, &nBufSize0, &pBuf1, &nBufSize1, MEDIA_TYPE_AUDIO, 0);
        if(ret == 0)
        {
            ctx->mAudioBufForTsDemux = (char*)pBuf0;
            mdBuf->buffer = (unsigned char*)pBuf0;
            mdBuf->bufferSize = nBufSize0;
            break;
        }
        else
        {
            CDX_LOGD("waiting for audio stream buffer size %d", ES_AUDIO_BUFFER_SIZE);
			if (ctx->exit_flag)
			{
				CDX_LOGW("exit request...");
				return 0;
			}
            usleep(10*1000); /* 100ms */
        }
    }
	return 0;
}

int WFDPlayer2::dmxcb_RequestAudioBuffer(void *param, void *cookie)
{
    WFDPlayer2 *player = (WFDPlayer2 *)cookie;
    CDX_CHECK(player);
	player->RequestAudioBuffer(param);
	return 0;
}

int WFDPlayer2::UpdateAudioData(void *param)
{
	int ret;
    int bIsFirst;
    int bIsLast;
    int64_t nPts;
    md_data_info_t *mdDataInfo;

    mdDataInfo = (md_data_info_t*)param;
    bIsFirst   = (mdDataInfo->ctrlBits & FIRST_PART_BIT) ? 1 : 0;
    bIsLast    = (mdDataInfo->ctrlBits & LAST_PART_BIT) ? 1 : 0;
    nPts       = (mdDataInfo->ctrlBits & PTS_VALID_BIT) ? mdDataInfo->pts : -1;

	MediaStreamDataInfo dataInfo;
	memset(&dataInfo, 0, sizeof(MediaStreamDataInfo));

	dataInfo.pData	= ctx->mAudioBufForTsDemux;
	dataInfo.nLength	  = mdDataInfo->dataLen;
	dataInfo.nPts		  = nPts;  //* input pts is in 90KHz.
	dataInfo.bIsFirstPart = bIsFirst;
	dataInfo.bIsLastPart  = bIsLast;

	ret = WFD_PlayerSubmitStreamData(ctx->mPlayer, &dataInfo, MEDIA_TYPE_AUDIO, 0);
	return 0;
}

int WFDPlayer2::dmxcb_UpdateAudioData(void *param, void *cookie)
{
    WFDPlayer2 *player = (WFDPlayer2 *)cookie;
    CDX_CHECK(player);
	player->UpdateAudioData(param);
	return 0;
}

int WFDPlayer2::EventNotify(int event, void *param)
{
    switch(event)
    {
        case PLAYBACK_NOTIFY_FIRST_PICTURE:
        {
        	ctx->iter->EventNotify(MIRACAST_CBK_PLAYER_FIRST_SHOW, NULL);
            break;
        }
        case PLAYBACK_NOTIFY_VIDEO_SIZE:
        {
            int nWidth  = ((int*)param)[0];
            int nHeight = ((int*)param)[1];
            CDX_LOGD("*************decoded nWidth = %d,nHeight = %d********", nWidth, nHeight);
            break;
        }

        case PLAYBACK_NOTIFY_VIDEO_CROP:
        case PLAYBACK_NOTIFY_VIDEO_UNSUPPORTED:
        case PLAYBACK_NOTIFY_AUDIO_UNSUPPORTED:
        case PLAYBACK_NOTIFY_VIDEO_RENDER_FRAME:
        
        default:
        {
            CDX_LOGW("others message 0x%x ", event);
            break;
        }
    }

    return 0;
}

static int SemTimedWait(sem_t* sem, int64_t time_ms)
{
    int err;

    if(time_ms == -1)
    {
        err = sem_wait(sem);
    }
    else
    {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_nsec += time_ms % 1000 * 1000 * 1000;
        ts.tv_sec += time_ms / 1000 + ts.tv_nsec / (1000 * 1000 * 1000);
        ts.tv_nsec = ts.tv_nsec % (1000*1000*1000);

        err = sem_timedwait(sem, &ts);
    }

    return err;
}

int WFDPlayer2::SetCastMode(enum CASTMODE eCastMode){
    if(ctx->mPlayer != NULL){
        return WFD_PlayerSetCastMode(ctx->mPlayer, eCastMode);
    }else{
        return -1;
    }
}

int WFDPlayer2::SetRotateDegree(enum ROTATEDEGREE eRotateDegree){
    if(ctx->mPlayer != NULL){
        return WFD_PlayerSetRotateDegree(ctx->mPlayer, eRotateDegree);
    }else{
        return -1;
    }
}
