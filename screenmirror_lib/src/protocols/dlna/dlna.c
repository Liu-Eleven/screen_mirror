#include "protocols/dlna.h"
#include "platform/cedarx_platform.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>

static pthread_mutex_t g_dlna_lock = PTHREAD_MUTEX_INITIALIZER;
static MirrorState g_dlna_state = MIRROR_STATE_IDLE;
static bool g_dlna_inited = false;

static int dlna_init(void)
{
    pthread_mutex_lock(&g_dlna_lock);
    if (!g_dlna_inited) {
        (void)cedarx_platform_init();
        g_dlna_inited = true;
        g_dlna_state = MIRROR_STATE_IDLE;
    }
    pthread_mutex_unlock(&g_dlna_lock);
    return MIRROR_ERR_SUCCESS;
}

static void dlna_exit(void)
{
    pthread_mutex_lock(&g_dlna_lock);
    g_dlna_inited = false;
    g_dlna_state = MIRROR_STATE_IDLE;
    pthread_mutex_unlock(&g_dlna_lock);
}

static int dlna_start_discovery(int timeout_ms)
{
    (void)timeout_ms;
    pthread_mutex_lock(&g_dlna_lock);
    g_dlna_state = MIRROR_STATE_DISCOVERING;
    pthread_mutex_unlock(&g_dlna_lock);
    return MIRROR_ERR_SUCCESS;
}

static void dlna_stop_discovery(void)
{
    pthread_mutex_lock(&g_dlna_lock);
    g_dlna_state = MIRROR_STATE_IDLE;
    pthread_mutex_unlock(&g_dlna_lock);
}

static int dlna_connect(const MirrorDeviceInfo *device,
                       const MirrorConfig *config)
{
    if (device == NULL || config == NULL) {
        return MIRROR_ERR_INVALID_PARAM;
    }

    int ret = cedarx_platform_open_session(config);
    if (ret != MIRROR_ERR_SUCCESS) {
        return ret;
    }

    pthread_mutex_lock(&g_dlna_lock);
    g_dlna_state = MIRROR_STATE_CONNECTED;
    pthread_mutex_unlock(&g_dlna_lock);
    return MIRROR_ERR_SUCCESS;
}

static void dlna_disconnect(void)
{
    cedarx_platform_close_session();
    pthread_mutex_lock(&g_dlna_lock);
    g_dlna_state = MIRROR_STATE_IDLE;
    pthread_mutex_unlock(&g_dlna_lock);
}

static int dlna_send_video(const uint8_t *data, int size)
{
    (void)data;
    if (size <= 0) {
        return MIRROR_ERR_INVALID_PARAM;
    }
    return size;
}

static int dlna_send_audio(const uint8_t *data, int size)
{
    (void)data;
    if (size <= 0) {
        return MIRROR_ERR_INVALID_PARAM;
    }
    return size;
}

static int dlna_control(const char *command)
{
    if (command == NULL) {
        return MIRROR_ERR_INVALID_PARAM;
    }
    return MIRROR_ERR_SUCCESS;
}

static MirrorState dlna_get_state(void)
{
    MirrorState state;
    pthread_mutex_lock(&g_dlna_lock);
    state = g_dlna_state;
    pthread_mutex_unlock(&g_dlna_lock);
    return state;
}

/* 协议操作函数集 */
static ProtocolOps dlna_ops = {
    .init = dlna_init,
    .exit = dlna_exit,
    .start_discovery = dlna_start_discovery,
    .stop_discovery = dlna_stop_discovery,
    .connect = dlna_connect,
    .disconnect = dlna_disconnect,
    .send_video = dlna_send_video,
    .send_audio = dlna_send_audio,
    .control = dlna_control,
    .get_state = dlna_get_state,
};

DlnaContext* dlna_create(const MirrorDeviceInfo *device,
                        const MirrorConfig *config)
{
    if (device == NULL || config == NULL) {
        return NULL;
    }

    DlnaContext *ctx = (DlnaContext *)malloc(sizeof(DlnaContext));
    if (ctx == NULL) {
        return NULL;
    }

    memset(ctx, 0, sizeof(DlnaContext));
    ctx->session_id = (uint32_t)time(NULL);
    ctx->state = 0;  /* idle */

    printf("[DLNA] Context created (session: %u)\n", ctx->session_id);

    return ctx;
}

void dlna_destroy(DlnaContext *ctx)
{
    if (ctx != NULL) {
        printf("[DLNA] Context destroyed\n");
        free(ctx);
    }
}

ProtocolOps* dlna_get_ops(void)
{
    return &dlna_ops;
}
