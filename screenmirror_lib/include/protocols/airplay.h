#ifndef AIRPLAY_H
#define AIRPLAY_H

#include "screenmirror_internal.h"

/* AirPlay 协议实现 */

typedef struct {
    uint32_t session_id;
    int state;
    void *platform_handle;
} AirplayContext;

/**
 * 创建 AirPlay 实例
 */
AirplayContext* airplay_create(const MirrorDeviceInfo *device,
                              const MirrorConfig *config);

/**
 * 销毁 AirPlay 实例
 */
void airplay_destroy(AirplayContext *ctx);

/**
 * 获取协议操作函数集
 */
ProtocolOps* airplay_get_ops(void);

#endif /* AIRPLAY_H */
