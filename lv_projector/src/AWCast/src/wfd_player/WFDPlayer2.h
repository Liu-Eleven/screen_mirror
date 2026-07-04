#ifndef WFD_PLAYER2_H
#define WFD_PLAYER2_H


//#include <layerControl.h>
#include "wfd_player.h"
typedef struct WFDPCtxS WFDPCtxT;

#define WFD_LIB_VERSION_PROPERTY_KEY "media.wfdsink.version_code"
#define WFD_LIB_COMMIT_PROPERTY_KEY  "media.wfdsink.commit_code"


#ifdef __cplusplus
class WFDPInterface
{
public:
	virtual ~WFDPInterface(){};
	virtual int handleRTSPStatus(int status) = 0;

	virtual void ExceptionHandler(void) = 0;

	virtual void *requestSurface(void) = 0;

	virtual int EventNotify(int event, void *param) = 0;
};

class WFDPlayer2
{
public:
    WFDPlayer2(WFDPInterface *iter);
    ~WFDPlayer2();

	int Setup(void *disp_hdr, void *snd_hdr);

    int Start(const char *url, int isenablehdcp, const char *videoFormat);

    int Stop();

    int Play();

	int EventNotify(int event, void *param);

	int StreamStatusHandler(int status);

	int StreamFeedData(void* buffer, int len);

	int RequestVideoBuffer(void *param);

	int UpdateVideoData(void *param);

	int RequestAudioBuffer(void *param);

	int UpdateAudioData(void *param);

	int PlayerInitAVandStart();

    int SetCastMode(enum CASTMODE eCastMode);

    int SetRotateDegree(enum ROTATEDEGREE eRotateDegree);

    static int streamcb_FeedData(void* cookie, void* buffer, int len);

	void DemuxProc();

	static void *DemuxThreadProc(void *param);

	static int dmxcb_RequestVideoBuffer(void *param, void *cookie);

	static int dmxcb_UpdateVideoData(void *param, void *cookie);

	static int dmxcb_RequestAudioBuffer(void *param, void *cookie);

	static int dmxcb_UpdateAudioData(void *param, void *cookie);

	//Add for wifidisplay hdcp
	int PlayerHdcpInit();

	int PlayerHdcpDeInit();

	int PlayerDecryptHdcpData(const uint8_t privateData[16],
                          const uint8_t *in, uint8_t *out, uint32_t len);

	static int dmxcb_InitHdcpModule(void *handle);

	static int dmxcb_DeInitHdcpModule(void *handle);

	static int dmxcb_DecrypData(void *handle, const uint8_t privateData[16],
                          const uint8_t *in, uint8_t *out, uint32_t len);

    int callbackProcess(int eMessageId, int cropx, int croph);

    int setScreenRotation(int rotation);

private:
	WFDPCtxT *ctx;

};
#endif

#endif //WFD_PLAYER2_H
