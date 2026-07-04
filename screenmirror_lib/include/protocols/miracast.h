#ifndef MIRACAST_H
#define MIRACAST_H

#include "screenmirror_internal.h"

/* Miracast 协议实现 */

typedef struct {
    uint32_t session_id;
    int state;  /* 0: idle, 1: discovering, 2: connecting, 3: connected */
    void *platform_handle;
} MiracastContext;

/**
 * 创建 Miracast 实例
 */
MiracastContext* miracast_create(const MirrorDeviceInfo *device,
                                const MirrorConfig *config);

/**
 * 销毁 Miracast 实例
 */
void miracast_destroy(MiracastContext *ctx);

/**
 * 获取协议操作函数集
 */
ProtocolOps* miracast_get_ops(void);

#endif /* MIRACAST_H */
