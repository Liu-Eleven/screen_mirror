#ifndef WFD_RTSP_THREAD_H
#define WFD_RTSP_THREAD_H

//#include <utils/threads.h>
#include "WFDSink.h"

using namespace android;

struct WFDRTSPThread : public Thread {
    WFDRTSPThread(WFDSink* sink, int enableHDCP, char* rtspURL, FeedDataCB1 feedDataCB, RTSPStatusHandler1 rtspStatusHandler, void* wfdManager, WFD_RTSP_VIDEO_Format videoFormat);

protected:
    virtual bool threadLoop();

private:
    WFDSink* mSink;
    int mEnableHDCP;
    char* mRTSPURL;
    FeedDataCB1 mFeedDataCB;
    RTSPStatusHandler1 mRTSPStatusHandler;
    void* mWFDManager;
    WFD_RTSP_VIDEO_Format mVideoFormat;
};
#endif //WFD_RTSP_THREAD_H