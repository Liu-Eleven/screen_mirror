#ifndef WFD_PLAYER2_H
#define WFD_PLAYER2_H

typedef struct UT_WFDPCtxS UT_WFDPCtxT;

class UT_WFDPlayer2
{
public:
    UT_WFDPlayer2();
    ~UT_WFDPlayer2();

	int Setup(void *disp_hdr, void *snd_hdr);
	
    int Start(const char *url);
    
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
    
    static int streamcb_FeedData(void* cookie, void* buffer, int len);
    
	void DemuxProc();
	
	static void *DemuxThreadProc(void *param);

	static int dmxcb_RequestVideoBuffer(void *param, void *cookie);
	
	static int dmxcb_UpdateVideoData(void *param, void *cookie);

	static int dmxcb_RequestAudioBuffer(void *param, void *cookie);
	
	static int dmxcb_UpdateAudioData(void *param, void *cookie);

private:
	UT_WFDPCtxT *ctx;

};

#endif
