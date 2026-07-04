#include "protocols/wired.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

/* USB 有线投屏操作函数 */

static int wired_init(void)
{
    printf("[WIRED] Initializing USB wired protocol\n");
    /* TODO: 初始化 USB 接口 */
    return MIRROR_ERR_SUCCESS;
}

static void wired_exit(void)
{
    printf("[WIRED] Exiting USB wired protocol\n");
    /* TODO: 释放 USB 资源 */
}

static int wired_start_discovery(int timeout_ms)
{
    printf("[WIRED] Starting USB device discovery (timeout: %d ms)\n", timeout_ms);
    /* TODO: 扫描 USB 设备 */
    return MIRROR_ERR_SUCCESS;
}

static void wired_stop_discovery(void)
{
    printf("[WIRED] Stopping USB discovery\n");
    /* TODO: 停止扫描 */
}

static int wired_connect(const MirrorDeviceInfo *device,
                        const MirrorConfig *config)
{
    printf("[WIRED] Connecting USB device: %s\n", device->name);
    /* TODO: 建立 USB 连接 */
    return MIRROR_ERR_SUCCESS;
}

static void wired_disconnect(void)
{
    printf("[WIRED] Disconnecting USB\n");
    /* TODO: 断开 USB 连接 */
}

static int wired_send_video(const uint8_t *data, int size)
{
    printf("[WIRED] Sending video frame (size: %d bytes)\n", size);
    /* TODO: 通过 USB 发送视频 */
    return size;
}

static int wired_send_audio(const uint8_t *data, int size)
{
    printf("[WIRED] Sending audio frame (size: %d bytes)\n", size);
    /* TODO: 通过 USB 发送音频 */
    return size;
}

static int wired_control(const char *command)
{
    printf("[WIRED] Control command: %s\n", command);
    /* TODO: USB 控制命令 */
    return MIRROR_ERR_SUCCESS;
}

static MirrorState wired_get_state(void)
{
    /* TODO: 获取 USB 连接状态 */
    return MIRROR_STATE_CONNECTED;
}

/* 协议操作函数集 */
static ProtocolOps wired_ops = {
    .init = wired_init,
    .exit = wired_exit,
    .start_discovery = wired_start_discovery,
    .stop_discovery = wired_stop_discovery,
    .connect = wired_connect,
    .disconnect = wired_disconnect,
    .send_video = wired_send_video,
    .send_audio = wired_send_audio,
    .control = wired_control,
    .get_state = wired_get_state,
};

WiredContext* wired_create(const MirrorDeviceInfo *device,
                          const MirrorConfig *config)
{
    if (device == NULL || config == NULL) {
        return NULL;
    }

    WiredContext *ctx = (WiredContext *)malloc(sizeof(WiredContext));
    if (ctx == NULL) {
        return NULL;
    }

    memset(ctx, 0, sizeof(WiredContext));
    ctx->session_id = (uint32_t)time(NULL);
    ctx->state = 0;  /* idle */
    ctx->usb_fd = -1;

    printf("[WIRED] Context created (session: %u)\n", ctx->session_id);

    return ctx;
}

void wired_destroy(WiredContext *ctx)
{
    if (ctx != NULL) {
        printf("[WIRED] Context destroyed\n");
        if (ctx->usb_fd >= 0) {
            /* close(ctx->usb_fd); */  /* 如果实际使用 */
        }
        free(ctx);
    }
}

ProtocolOps* wired_get_ops(void)
{
    return &wired_ops;
}
