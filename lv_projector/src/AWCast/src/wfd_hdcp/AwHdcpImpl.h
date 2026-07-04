#ifndef AW_HDCP_IMPL_H
#define AW_HDCP_IMPL_H

#include <fcntl.h>
#include <pthread.h>
#include "AwHdcpReceiver.h"

//struct AwHdcpImpl : public RefBase
struct AwHdcpImpl
{
    typedef  void* (AwHdcpImpl::*HdcpReceiverPtr)(void);
    typedef  void* (*PthreadPtr)(void*);
    void  startThread(void);
    void* threadLoop(void);

    status_t startAsync(const char *host, unsigned port);
    void stop();

    virtual ~AwHdcpImpl();

    HdcpReceiver mHdcpReceiver;
};

#endif