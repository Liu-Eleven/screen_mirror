#ifndef CEDARX_PLATFORM_H
#define CEDARX_PLATFORM_H

#include "screenmirror.h"
#include <stdbool.h>

/* Allwinner Cedarx 平台适配 */

/**
 * Cedarx 平台初始化
 */
int cedarx_platform_init(void);

/**
 * Cedarx 平台反初始化
 */
void cedarx_platform_exit(void);

int cedarx_platform_open_session(const MirrorConfig *config);
void cedarx_platform_close_session(void);
bool cedarx_platform_is_ready(void);

/**
 * 获取 Cedarx 编码器句柄
 */
void* cedarx_get_encoder(void);

/**
 * 获取 Cedarx 渲染器句柄
 */
void* cedarx_get_renderer(void);
void* cedarx_get_audio_recorder(void);

#endif /* CEDARX_PLATFORM_H */
