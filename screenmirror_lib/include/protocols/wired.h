#ifndef WIRED_H
#define WIRED_H

#include "screenmirror_internal.h"

/* USB 有线投屏实现 */

typedef struct {
    uint32_t session_id;
    int state;
    void *usb_handle;
    int usb_fd;
} WiredContext;

/**
 * 创建有线投屏实例
 */
WiredContext* wired_create(const MirrorDeviceInfo *device,
                          const MirrorConfig *config);

/**
 * 销毁有线投屏实例
 */
void wired_destroy(WiredContext *ctx);

/**
 * 获取协议操作函数集
 */
ProtocolOps* wired_get_ops(void);

#endif /* WIRED_H */
