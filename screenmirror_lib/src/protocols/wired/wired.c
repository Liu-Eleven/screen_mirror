#include "protocols/wired.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>

static pthread_mutex_t g_wired_lock = PTHREAD_MUTEX_INITIALIZER;
static MirrorState g_wired_state = MIRROR_STATE_IDLE;

static int wired_init(void)
{
    pthread_mutex_lock(&g_wired_lock);
    g_wired_state = MIRROR_STATE_IDLE;
    pthread_mutex_unlock(&g_wired_lock);
    return MIRROR_ERR_SUCCESS;
}

static void wired_exit(void)
{
    pthread_mutex_lock(&g_wired_lock);
    g_wired_state = MIRROR_STATE_IDLE;
    pthread_mutex_unlock(&g_wired_lock);
}

static int wired_start_discovery(int timeout_ms)
{
    (void)timeout_ms;
    pthread_mutex_lock(&g_wired_lock);
    g_wired_state = MIRROR_STATE_DISCOVERING;
    pthread_mutex_unlock(&g_wired_lock);
    return MIRROR_ERR_SUCCESS;
}

static void wired_stop_discovery(void)
{
    pthread_mutex_lock(&g_wired_lock);
    g_wired_state = MIRROR_STATE_IDLE;
    pthread_mutex_unlock(&g_wired_lock);
}

static int wired_connect(const MirrorDeviceInfo *device,
                        const MirrorConfig *config)
{
    (void)config;
    if (device == NULL) {
        return MIRROR_ERR_INVALID_PARAM;
    }
    pthread_mutex_lock(&g_wired_lock);
    g_wired_state = MIRROR_STATE_CONNECTED;
    pthread_mutex_unlock(&g_wired_lock);
    return MIRROR_ERR_SUCCESS;
}

static void wired_disconnect(void)
{
    pthread_mutex_lock(&g_wired_lock);
    g_wired_state = MIRROR_STATE_IDLE;
    pthread_mutex_unlock(&g_wired_lock);
}

static int wired_send_video(const uint8_t *data, int size)
{
    (void)data;
    if (size <= 0) {
        return MIRROR_ERR_INVALID_PARAM;
    }
    return size;
}

static int wired_send_audio(const uint8_t *data, int size)
{
    (void)data;
    if (size <= 0) {
        return MIRROR_ERR_INVALID_PARAM;
    }
    return size;
}

static int wired_control(const char *command)
{
    if (command == NULL) {
        return MIRROR_ERR_INVALID_PARAM;
    }
    return MIRROR_ERR_SUCCESS;
}

static MirrorState wired_get_state(void)
{
    MirrorState state;
    pthread_mutex_lock(&g_wired_lock);
    state = g_wired_state;
    pthread_mutex_unlock(&g_wired_lock);
    return state;
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
