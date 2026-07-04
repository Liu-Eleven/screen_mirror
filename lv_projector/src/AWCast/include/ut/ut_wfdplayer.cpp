//#define LOG_NDEBUG 0
#define LOG_TAG "UT_WFDPlayer2"

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

#include <wfd_player.h>


#include "layerControl.h"
#include "tsdemux_i.h"

#include "tsound_ctrl.h"
#include "ut_wfdplayer.h"
#include <miracast2.h>

#define PMT_STREAM_TYPE_MPEG2_P7_A_AAC  0x0f

#define ES_VIDEO_BUFFER_SIZE (512 * 1024)
#define ES_AUDIO_BUFFER_SIZE (1 * 1024)

extern "C"
{
	extern LayerCtrl* LayerCreate_DE();
	extern SoundCtrl* TSoundDeviceCreate(AudioFrameCallback callback, void* pUser);
}

static int SemTimedWait(sem_t* sem, int64_t time_ms);
struct UT_WFDPCtxS
{

	Player *mPlayer;


	
	LayerCtrl *disp;
	SoundCtrl *snd;

	pthread_t demux_pid;
	pthread_t wfdsink_pid;

//	CdxListT tpp_buf_list;

//	int parse_program_fin;
//	struct ProgramInfoS pi[8];
	int p_num;
    char *mVideoBufForTsDemux;
    char *mAudioBufForTsDemux;

	int ptsSecs;
	int exit_flag;
    sem_t exit_flag_sem;
	int bPlayerSetup;
};

static int PlayerCB_Process(void *p, int messageId, void* param)
{
    UT_WFDPlayer2 *player = (UT_WFDPlayer2 *)p;
    return player->EventNotify(messageId, param);
}

UT_WFDPlayer2::UT_WFDPlayer2()
{
	ctx = (struct UT_WFDPCtxS *)malloc(sizeof(struct UT_WFDPCtxS));
	memset(ctx, 0x00, sizeof(struct UT_WFDPCtxS));
	ctx->exit_flag = 0;
    sem_init(&ctx->exit_flag_sem, 0, 0);
	ctx->bPlayerSetup = 0;
}

UT_WFDPlayer2::~UT_WFDPlayer2()
{
    sem_destroy(&ctx->exit_flag_sem);

	free(ctx);
    CDX_LOGD("~UT_WFDPlayer2.");
}

int UT_WFDPlayer2::Setup(void *disp_hdr, void *snd_hdr)
{
	int ret = 0;

    ctx->mPlayer = WFD_PlayerCreate();
    if (ctx->mPlayer == NULL)
    {
        CDX_LOGE("PlayerCreate faied");
        return -1;
    }

	WFD_PlayerSetCallback(ctx->mPlayer, PlayerCB_Process, this);

	/* init disp */
	ctx->disp = LayerCreate_DE();

    WFD_PlayerSetWindow(ctx->mPlayer, ctx->disp);

	/* init snd */
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

int UT_WFDPlayer2::PlayerInitAVandStart()
{
    int    ret = 0;

    //* set audio stream info.
    AudioStreamInfo streamInfo;
    memset(&streamInfo, 0, sizeof(streamInfo));

    streamInfo.eCodecFormat          = MpegStreamtype2CdxCodectype(AUDIO_CODEC_FORMAT_MPEG_AAC_LC);
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
    CDX_LOGD("*********** video info **********************");
    CDX_LOGD("vFmt:%d", vsi.eCodecFormat);
    CDX_LOGD("**********************************************");

	ret = WFD_PlayerStart(ctx->mPlayer);
    return ret;
}

struct UT_SinkParamS
{
	char const *rtspURL;
	void * wfdManager;
};

static void *WFDSinkThread(void *param)
{
	int ret;
	struct UT_SinkParamS *sp = (struct UT_SinkParamS *)param;
	static char *url = (char *)sp->rtspURL;
	static void *wfdp = sp->wfdManager;

	char buf[2048] = {0};
	
	CDX_LOGD("WFDSinkThread start...");
	FILE *fd;
	fd = fopen(url, "r");
	assert(fd != NULL);
	while (1)
	{
		ret = fread(buf, 188, 5, fd);
		if (ret <= 0)
		{
			CDX_LOGD("eos???");
			break;
		}

		UT_WFDPlayer2::streamcb_FeedData(wfdp, buf, ret*188);
	//	usleep(10000);
	}
    fclose(fd);
	CDX_LOGD("WFDSinkThread end...");
	
	return NULL;
}

int UT_WFDPlayer2::Start(const char *url)
{
	CDX_LOGD("UT_WFDPlayer2::Start");

	int ret = 0;

	/* rtsp */
	static struct UT_SinkParamS sp;
	sp.rtspURL = url;
	sp.wfdManager = (void*)this;
	
	ret = pthread_create(&ctx->wfdsink_pid, NULL, WFDSinkThread, &sp);
	if (ret != 0)
	{
		CDX_LOGD("startWFDSink failure...");
		return -1;
	}

	/* ts demux */
	return ret;
}

int UT_WFDPlayer2::Stop()
{
	CDX_LOGD("UT_WFDPlayer2::Stop");
	ctx->exit_flag = 1;

	CDX_LOGD("UT_WFDPlayer2::Stop trace...");

	CDX_LOGD("UT_WFDPlayer2::Stop trace...");
	pthread_join(ctx->wfdsink_pid, NULL);
	CDX_LOGD("UT_WFDPlayer2::Stop trace...");

	pthread_join(ctx->demux_pid, NULL);
	CDX_LOGD("UT_WFDPlayer2::Stop trace...");

	if (ctx->mPlayer)
	{
		WFD_PlayerStop(ctx->mPlayer);
		WFD_PlayerDestroy(ctx->mPlayer);
		ctx->mPlayer = NULL;
	}
        ctx->bPlayerSetup = 0;
	return 0;
}

int UT_WFDPlayer2::Play()
{
	int ret = 0;
	CDX_LOGD("UT_WFDPlayer2::Play");
//    ret = WFD_PlayerStart(ctx->mPlayer);
    return ret;
}

int UT_WFDPlayer2::RequestVideoBuffer(void *param)
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

static int64_t Utils_GetTimeUS() 
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return (int64_t)tv.tv_sec * 1000000ll + tv.tv_usec;
}

int UT_WFDPlayer2::UpdateVideoData(void *param)
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

	int ptsSecs = (int)(nPts/1000000);
	if (ctx->ptsSecs != ptsSecs)
	{
		ctx->ptsSecs = ptsSecs;
		CDX_LOGD("pic_trace: (%.3f s) @(%.3f ms)queue into player", nPts/1000000.0, 
				Utils_GetTimeUS()/1000.0);
	}
  logv("stream frame num %d, stream data size %d.", WFD_PlayerGetVideoStreamFrameNum(ctx->mPlayer), WFD_PlayerGetVideoStreamDataSize(ctx->mPlayer));
    ret = WFD_PlayerSubmitStreamData(ctx->mPlayer, &dataInfo, MEDIA_TYPE_VIDEO, nStreamIndex);
	return 0;
}

int UT_WFDPlayer2::RequestAudioBuffer(void *param)
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

int UT_WFDPlayer2::dmxcb_RequestAudioBuffer(void *param, void *cookie)
{
    UT_WFDPlayer2 *player = (UT_WFDPlayer2 *)cookie;
    CDX_CHECK(player);
	player->RequestAudioBuffer(param);
	return 0;
}

int UT_WFDPlayer2::UpdateAudioData(void *param)
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

int UT_WFDPlayer2::EventNotify(int event, void *param)
{
    switch(event)
    {
        case PLAYBACK_NOTIFY_FIRST_PICTURE:
        {
			
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

int main(int argc, char *argv[])
{
	UT_WFDPlayer2 *wfd_p = NULL;

	if (argc != 2)
	{
		printf("usage: ut_wfdplayer [url]\n");
		return -1;
	}
	wfd_p = new UT_WFDPlayer2;

	if (!wfd_p)
	{
		CDX_LOGE("UT_WFDPlayer2 new failed");
		return -1;
	}
	if (wfd_p->Setup(NULL, NULL) != 0)
	{
		CDX_LOGE("mWFDPlayer->Setup() failed");
		return -1;
	}

	if (wfd_p->Start(argv[1]) != 0)
	{
		CDX_LOGE("mWFDPlayer->Start() failed");
		return -1;
	}

	while (1)
	{
		sleep(2);
	}
	
	return 0;
}
