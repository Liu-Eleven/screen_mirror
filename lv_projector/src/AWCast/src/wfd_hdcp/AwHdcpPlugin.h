//#include <media/hardware/HDCPAPI.h>
#include "AwHdcpImpl.h"

#ifndef AW_HDCP_PLUGIN_H
#define AW_HDCP_PLUGIN_H

#define STREAM_NUM 5
#define INVALID_OPERATION 80

struct AwHdcpPlugin;

extern "C" {
    void *createHDCPModule(void *cookie);
    AwHdcpPlugin *createHDCPModuleForDecryption(void *cookie);
}

typedef struct native_handle
{
    int version;        /* sizeof(native_handle_t) */
    int numFds;         /* number of file-descriptors at &data[0] */
    int numInts;        /* number of ints at &data[numFds] */
    int data[0];        /* numFds + numInts ints */
} native_handle_t;

typedef void (*hdcp_callback_type)(void*, int, int);
typedef const native_handle_t* buffer_handle_t;

struct AwHdcpPlugin
{
    AwHdcpPlugin(void * cookie);
    virtual ~AwHdcpPlugin();

    virtual status_t initAsync(const char *addr, unsigned port);

    // Request to shutdown the active HDCP session.
    virtual status_t shutdownAsync();

    // Returns the capability bitmask of this HDCP session.
    virtual uint32_t getCaps() {
        return 0;
    }

    virtual status_t encrypt(
            const void * /*inData*/, size_t /*size*/, uint32_t /*streamCTR*/,
            uint64_t * /*outInputCTR*/, void * /*outData*/) {
        return INVALID_OPERATION;
    }

    virtual status_t encryptNative(
            buffer_handle_t /*buffer*/, size_t /*offset*/, size_t /*size*/,
            uint32_t /*streamCTR*/, uint64_t * /*outInputCTR*/, void * /*outData*/) {
        return INVALID_OPERATION;
    }

    virtual status_t decrypt(
            const void * inData, size_t size,
            uint32_t streamCTR, uint64_t inputCTR,
            void * outData);

    static void hdcp_event_callbak(void* cookie, int event, int msg);
    void uint32ToBytes(uint32_t value, unsigned char *bytes);
    status_t aesCTR128(unsigned char *plain, int plainlen, unsigned char *key,
                 unsigned char *iv, unsigned char *cipher, unsigned char *ctrval);
    int validStreamIdx;
    uint32_t stmctr[STREAM_NUM];
    uint64_t inctr[STREAM_NUM];
    unsigned char ctr[STREAM_NUM][16];

private:
    AwHdcpImpl* mHdcpImpl;

    void* mUserCookie;

    AwHdcpPlugin(const AwHdcpPlugin &);
    AwHdcpPlugin &operator=(const AwHdcpPlugin &);
};

#endif
