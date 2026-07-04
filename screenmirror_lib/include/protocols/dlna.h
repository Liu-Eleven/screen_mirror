#ifndef DLNA_H
#define DLNA_H

#include "screenmirror_internal.h"

/* DLNA 协议实现 */

typedef struct {
    uint32_t session_id;
    int state;
    void *platform_handle;
    void *render_handle;  /* Cedarx 渲染器 */
} DlnaContext;

/**
 * 创建 DLNA 实例
 */
DlnaContext* dlna_create(const MirrorDeviceInfo *device,
                        const MirrorConfig *config);

/**
 * 销毁 DLNA 实例
 */
void dlna_destroy(DlnaContext *ctx);

/**
 * 获取协议操作函数集
 */
ProtocolOps* dlna_get_ops(void);

#endif /* DLNA_H */
