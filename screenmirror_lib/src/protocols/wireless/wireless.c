#include "protocols/wireless.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>

static pthread_mutex_t g_wireless_lock = PTHREAD_MUTEX_INITIALIZER;
static MirrorState g_wireless_state = MIRROR_STATE_IDLE;

static int wireless_init(void)
{
    pthread_mutex_lock(&g_wireless_lock);
    g_wireless_state = MIRROR_STATE_IDLE;
    pthread_mutex_unlock(&g_wireless_lock);
    return MIRROR_ERR_SUCCESS;
}

static void wireless_exit(void)
{
    pthread_mutex_lock(&g_wireless_lock);
    g_wireless_state = MIRROR_STATE_IDLE;
    pthread_mutex_unlock(&g_wireless_lock);
}

static int wireless_start_discovery(int timeout_ms)
{
    (void)timeout_ms;
    pthread_mutex_lock(&g_wireless_lock);
    g_wireless_state = MIRROR_STATE_DISCOVERING;
    pthread_mutex_unlock(&g_wireless_lock);
    return MIRROR_ERR_SUCCESS;
}

static void wireless_stop_discovery(void)
{
    pthread_mutex_lock(&g_wireless_lock);
    g_wireless_state = MIRROR_STATE_IDLE;
    pthread_mutex_unlock(&g_wireless_lock);
}

static int wireless_connect(const MirrorDeviceInfo *device,
                           const MirrorConfig *config)
{
    (void)config;
    if (device == NULL) {
        return MIRROR_ERR_INVALID_PARAM;
    }
    pthread_mutex_lock(&g_wireless_lock);
    g_wireless_state = MIRROR_STATE_CONNECTED;
    pthread_mutex_unlock(&g_wireless_lock);
    return MIRROR_ERR_SUCCESS;
}

static void wireless_disconnect(void)
{
    pthread_mutex_lock(&g_wireless_lock);
    g_wireless_state = MIRROR_STATE_IDLE;
    pthread_mutex_unlock(&g_wireless_lock);
}

static int wireless_send_video(const uint8_t *data, int size)
{
    (void)data;
    if (size <= 0) {
        return MIRROR_ERR_INVALID_PARAM;
    }
    return size;
}

static int wireless_send_audio(const uint8_t *data, int size)
{
    (void)data;
    if (size <= 0) {
        return MIRROR_ERR_INVALID_PARAM;
    }
    return size;
}

static int wireless_control(const char *command)
{
    if (command == NULL) {
        return MIRROR_ERR_INVALID_PARAM;
    }
    return MIRROR_ERR_SUCCESS;
}

static MirrorState wireless_get_state(void)
{
    MirrorState state;
    pthread_mutex_lock(&g_wireless_lock);
    state = g_wireless_state;
    pthread_mutex_unlock(&g_wireless_lock);
    return state;
}

/* 协议操作函数集 */
static ProtocolOps wireless_ops = {
    .init = wireless_init,
    .exit = wireless_exit,
    .start_discovery = wireless_start_discovery,
    .stop_discovery = wireless_stop_discovery,
    .connect = wireless_connect,
    .disconnect = wireless_disconnect,
    .send_video = wireless_send_video,
    .send_audio = wireless_send_audio,
    .control = wireless_control,
    .get_state = wireless_get_state,
};

WirelessContext* wireless_create(const MirrorDeviceInfo *device,
                                const MirrorConfig *config)
{
    if (device == NULL || config == NULL) {
        return NULL;
    }

    WirelessContext *ctx = (WirelessContext *)malloc(sizeof(WirelessContext));
    if (ctx == NULL) {
        return NULL;
    }

    memset(ctx, 0, sizeof(WirelessContext));
    ctx->session_id = (uint32_t)time(NULL);
    ctx->state = 0;  /* idle */
    ctx->tcp_port = 5000;  /* 默认端口 */
    strncpy(ctx->ip_address, device->ip_address, sizeof(ctx->ip_address) - 1);

    printf("[WIRELESS] Context created (session: %u, IP: %s, Port: %d)\n",
           ctx->session_id, ctx->ip_address, ctx->tcp_port);

    return ctx;
}

void wireless_destroy(WirelessContext *ctx)
{
    if (ctx != NULL) {
        printf("[WIRELESS] Context destroyed\n");
        free(ctx);
    }
}

ProtocolOps* wireless_get_ops(void)
{
    return &wireless_ops;
}
