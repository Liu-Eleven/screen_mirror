#ifndef HDCP_RECEIVER_H_
#define HDCP_RECEIVER_H_

#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/tcp.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>

typedef int status_t;
#define OK 0

struct HdcpReceiver
{
    HdcpReceiver();
    virtual ~HdcpReceiver();
    typedef  void* (HdcpReceiver::*HdcpReceiverPtr)(void);
    typedef  void* (*PthreadPtr)(void*);
    void* creatthread(void);
    status_t initAsync(const char *host, unsigned port);
    void shutdownAsync();
    int getStopIssued(void);
    void setStopIssued(int val);
    status_t on_AKE_Init(int fd, const char* buf);
    status_t on_LC_Init(int fd,const char* buf);
    status_t on_AKE_Transmitter_Info(int fd, const char* buf);
    status_t on_AKE_No_Stored_Km(int fd, const char* buf);
    status_t on_AKE_Stored_Km(int fd, const char* buf);
    status_t onSKE_Send_Eks(int fd, const char* buf);
    status_t AKE_Send_Cert(int fd);
    status_t AKE_Receiver_Info(int fd);
    status_t AKE_Send_Rrx(int fd);
    status_t AKE_Send_H_prime(int fd);
    status_t AKE_Send_Pairing_Info(int fd);
    status_t LC_Send_L_prime(int fd);
    status_t HandleHdcp(int clientSocket);
    status_t MakeSocketNonBlocking(int s);
    long long GetNowUs();
    void setHdcpVersion(int ver);
    unsigned char* getKs();
    unsigned char* getRiv();
    unsigned char* getLc();

    typedef void (*callback_type)(void*, int, int);
    callback_type callback;
    void* user_data;
    void set_callback(callback_type cb, void* ud){
        callback = cb;
        user_data = ud;
    }
    void notifyAuthComplete(){ callback(user_data, 9 /*HDCP_SESSION_ESTABLISHED*/, 0); }
};

#endif  // HDCP_RECEIVER_H_
