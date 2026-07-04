#ifndef WIRELESS_H
#define WIRELESS_H

#include "screenmirror_internal.h"

/* WiFi 无线投屏实现 */

typedef struct {
    uint32_t session_id;
    int state;
    void *wifi_handle;
    char ip_address[16];
    int tcp_port;
} WirelessContext;

/**
 * 创建无线投屏实例
 */
WirelessContext* wireless_create(const MirrorDeviceInfo *device,
                                const MirrorConfig *config);

/**
 * 销毁无线投屏实例
 */
void wireless_destroy(WirelessContext *ctx);

/**
 * 获取协议操作函数集
 */
ProtocolOps* wireless_get_ops(void);

#endif /* WIRELESS_H */
