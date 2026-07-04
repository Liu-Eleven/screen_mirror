#ifndef DISCOVERY_H
#define DISCOVERY_H

#include "screenmirror_internal.h"

/* 设备发现接口 */

/**
 * 启动设备发现线程
 * @param mode: 投屏模式
 * @param timeout_ms: 超时时间
 * @param callback: 发现到设备时的回调
 * @return: 发现线程ID，< 0 表示失败
 */
int discovery_start(MirrorMode mode, int timeout_ms,
                   MirrorDeviceListCallback callback, void *user_data);

/**
 * 停止设备发现
 */
void discovery_stop(void);

/**
 * 获取发现状态
 */
bool discovery_is_running(void);

#endif /* DISCOVERY_H */
