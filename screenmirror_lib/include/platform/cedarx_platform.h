#ifndef CEDARX_PLATFORM_H
#define CEDARX_PLATFORM_H

/* Allwinner Cedarx 平台适配 */

/**
 * Cedarx 平台初始化
 */
int cedarx_platform_init(void);

/**
 * Cedarx 平台反初始化
 */
void cedarx_platform_exit(void);

/**
 * 获取 Cedarx 编码器句柄
 */
void* cedarx_get_encoder(void);

/**
 * 获取 Cedarx 渲染器句柄
 */
void* cedarx_get_renderer(void);

#endif /* CEDARX_PLATFORM_H */
