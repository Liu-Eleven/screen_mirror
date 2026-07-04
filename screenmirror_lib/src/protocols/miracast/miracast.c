#include "protocols/miracast.h"
#include "protocols/wfd_protocol.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

static ProtocolOps *g_wfd_ops = NULL;

static int miracast_init(void)
{
    g_wfd_ops = wfd_protocol_get_ops();
    return (g_wfd_ops != NULL) ? g_wfd_ops->init() : MIRROR_ERR_UNKNOWN;
}

static void miracast_exit(void)
{
    if (g_wfd_ops != NULL) {
        g_wfd_ops->exit();
    }
}

static int miracast_start_discovery(int timeout_ms)
{
    if (g_wfd_ops == NULL) {
        return MIRROR_ERR_NOT_INIT;
    }
    return g_wfd_ops->start_discovery(timeout_ms);
}

static void miracast_stop_discovery(void)
{
    if (g_wfd_ops != NULL) {
        g_wfd_ops->stop_discovery();
    }
}

static int miracast_connect(const MirrorDeviceInfo *device,
                           const MirrorConfig *config)
{
    if (g_wfd_ops == NULL) {
        return MIRROR_ERR_NOT_INIT;
    }
    return g_wfd_ops->connect(device, config);
}

static void miracast_disconnect(void)
{
    if (g_wfd_ops != NULL) {
        g_wfd_ops->disconnect();
    }
}

static int miracast_send_video(const uint8_t *data, int size)
{
    if (g_wfd_ops == NULL) {
        return MIRROR_ERR_NOT_INIT;
    }
    return g_wfd_ops->send_video(data, size);
}

static int miracast_send_audio(const uint8_t *data, int size)
{
    if (g_wfd_ops == NULL) {
        return MIRROR_ERR_NOT_INIT;
    }
    return g_wfd_ops->send_audio(data, size);
}

static int miracast_control(const char *command)
{
    if (g_wfd_ops == NULL) {
        return MIRROR_ERR_NOT_INIT;
    }
    return g_wfd_ops->control(command);
}

static MirrorState miracast_get_state(void)
{
    if (g_wfd_ops == NULL) {
        return MIRROR_STATE_IDLE;
    }
    return g_wfd_ops->get_state();
}

/* 协议操作函数集 */
static ProtocolOps miracast_ops = {
    .init = miracast_init,
    .exit = miracast_exit,
    .start_discovery = miracast_start_discovery,
    .stop_discovery = miracast_stop_discovery,
    .connect = miracast_connect,
    .disconnect = miracast_disconnect,
    .send_video = miracast_send_video,
    .send_audio = miracast_send_audio,
    .control = miracast_control,
    .get_state = miracast_get_state,
};

MiracastContext* miracast_create(const MirrorDeviceInfo *device,
                                const MirrorConfig *config)
{
    if (device == NULL || config == NULL) {
        return NULL;
    }

    MiracastContext *ctx = (MiracastContext *)malloc(sizeof(MiracastContext));
    if (ctx == NULL) {
        return NULL;
    }

    memset(ctx, 0, sizeof(MiracastContext));
    ctx->session_id = (uint32_t)time(NULL);  /* 简单的 session ID 生成 */
    ctx->state = 0;  /* idle */

    printf("[MIRACAST] Context created (session: %u)\n", ctx->session_id);

    return ctx;
}

void miracast_destroy(MiracastContext *ctx)
{
    if (ctx != NULL) {
        printf("[MIRACAST] Context destroyed\n");
        free(ctx);
    }
}

ProtocolOps* miracast_get_ops(void)
{
    return &miracast_ops;
}
