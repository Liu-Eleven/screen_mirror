#define LOG_TAG "WFDRTSPThread"

#include "WFDRTSPThread.h"

#include "cdx_log.h"

//using namespace android;

WFDRTSPThread::WFDRTSPThread(WFDSink* sink, int enableHDCP, char* rtspURL, FeedDataCB1 feedDataCB, RTSPStatusHandler1 rtspStatusHandler, void* wfdManager, WFD_RTSP_VIDEO_Format videoFormat)
    : mSink(sink),
      mEnableHDCP(enableHDCP),
      mRTSPURL(rtspURL),
      mFeedDataCB(feedDataCB),
      mRTSPStatusHandler(rtspStatusHandler),
      mWFDManager(wfdManager)
      mVideoFormat(videoFormat) {
    CDX_LOGD("WFDRTSPThread");
}

bool WFDRTSPThread::threadLoop() {
    int ret = startWFDSink(mSink, mEnableHDCP, mRTSPURL, mFeedDataCB, mRTSPStatusHandler, mWFDManager, mVideoFormat);
    CDX_LOGD("end WFDRTSPThread, ret %d", ret);

    return false;
}
