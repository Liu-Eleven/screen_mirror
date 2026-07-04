#ifndef WFD_SINK_H
#define WFD_SINK_H

#ifndef VideoFormats
#define VideoFormats
#include "VideoFormats.h"
#endif

class ourRTSPClient;

typedef enum {
  WFD_SETUP = 1,
  WFD_PLAY = 2,
  WFD_PAUSE = 3,
  WFD_TEARDOWN = 4,
}WFD_RTSP_Status;

typedef struct WFDSink_S WFDSink;

typedef int (*FeedDataCB1)(void* wfdManager, void* buffer, int len);
typedef int (*RTSPStatusHandler1)(void* wfdManager, WFD_RTSP_Status status);

// Define a class to hold per-stream state that we maintain throughout each stream's lifetime:

int startWFDSink(WFDSink** sink, int enableHDCP, char const* rtspURL, FeedDataCB1 feedDataCB, RTSPStatusHandler1 rtspStatusHandler, void* wfdManager, const char *videoFormat);
int stopWFDSink(WFDSink** sink);
#endif //WFD_SINK_H