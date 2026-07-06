/*
 * wired.c - USB wired cast (USBCast / WSP integration)
 *
 * On H133 (H133_BOARD defined): calls WSP_init / WSP_StartService /
 * WSP_StopService / WSP_exit from the lv_projector USBCast module.
 * The WSP module internally dlopen(WIRED_LIB) to load Android-wire
 * and AirPlay-wire protocols.
 *
 * Without H133: stub implementation.
 */

#include "protocols/wired.h"
#include "bsp/awcast_api.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>

/* ── internal state ──────────────────────────────────────────────────────── */

static struct {
    int              state;   /* 0=idle, 1=listening, 2=connected */
    pthread_mutex_t  lock;
} g_wired = {
    .state = 0,
    .lock  = PTHREAD_MUTEX_INITIALIZER,
};

/* ── ProtocolOps implementation ──────────────────────────────────────────── */

static int wired_init(void)
{
    printf("[WIRED] init (USBCast)\n");
#ifdef H133_BOARD
    int ret = WSP_init();
    if (ret != 0) {
        printf("[WIRED] WSP_init failed: %d\n", ret);
        return MIRROR_ERR_UNKNOWN;
    }
#endif
    pthread_mutex_lock(&g_wired.lock);
    g_wired.state = 0;
    pthread_mutex_unlock(&g_wired.lock);
    return MIRROR_ERR_SUCCESS;
}

static void wired_exit(void)
{
    printf("[WIRED] exit (USBCast)\n");
#ifdef H133_BOARD
    WSP_StopService();
    WSP_exit();
#endif
    pthread_mutex_lock(&g_wired.lock);
    g_wired.state = 0;
    pthread_mutex_unlock(&g_wired.lock);
}

static int wired_start_discovery(int timeout_ms)
{
    (void)timeout_ms;
    printf("[WIRED] start_discovery – starting USBCast service\n");
#ifdef H133_BOARD
    int ret = WSP_StartService();
    if (ret != 0) {
        printf("[WIRED] WSP_StartService failed: %d\n", ret);
        return MIRROR_ERR_UNKNOWN;
    }
    pthread_mutex_lock(&g_wired.lock);
    g_wired.state = 1;
    pthread_mutex_unlock(&g_wired.lock);
#endif
    return MIRROR_ERR_SUCCESS;
}

static void wired_stop_discovery(void)
{
    printf("[WIRED] stop_discovery\n");
#ifdef H133_BOARD
    WSP_StopService();
    pthread_mutex_lock(&g_wired.lock);
    g_wired.state = 0;
    pthread_mutex_unlock(&g_wired.lock);
#endif
}

static int wired_connect(const MirrorDeviceInfo *device,
                         const MirrorConfig *config)
{
    (void)device; (void)config;
    printf("[WIRED] connect – ensuring USBCast service is running\n");
#ifdef H133_BOARD
    pthread_mutex_lock(&g_wired.lock);
    int s = g_wired.state;
    pthread_mutex_unlock(&g_wired.lock);
    if (s == 0) {
        return wired_start_discovery(0);
    }
#endif
    return MIRROR_ERR_SUCCESS;
}

static void wired_disconnect(void)
{
    printf("[WIRED] disconnect\n");
#ifdef H133_BOARD
    WSP_StopService();
    pthread_mutex_lock(&g_wired.lock);
    g_wired.state = 0;
    pthread_mutex_unlock(&g_wired.lock);
#endif
}

static int wired_send_video(const uint8_t *data, int size)
{
    /* USBCast is sink-side: video data arrives via USB and is rendered
     * internally by libthirdparty_mirror.so.  Not applicable here. */
    (void)data;
    return size;
}

static int wired_send_audio(const uint8_t *data, int size)
{
    (void)data;
    return size;
}

static int wired_control(const char *command)
{
    printf("[WIRED] control: %s\n", command ? command : "(null)");
    if (!command) return MIRROR_ERR_INVALID_PARAM;
    if (strcmp(command, "stop") == 0) {
        wired_disconnect();
    } else if (strcmp(command, "start") == 0) {
        wired_start_discovery(0);
    }
    return MIRROR_ERR_SUCCESS;
}

static MirrorState wired_get_state(void)
{
    int s;
    pthread_mutex_lock(&g_wired.lock);
    s = g_wired.state;
    pthread_mutex_unlock(&g_wired.lock);
    if (s == 2) return MIRROR_STATE_STREAMING;
    if (s == 1) return MIRROR_STATE_DISCOVERING;
    return MIRROR_STATE_IDLE;
}

/* ── public symbols ──────────────────────────────────────────────────────── */

static ProtocolOps wired_ops = {
    .init            = wired_init,
    .exit            = wired_exit,
    .start_discovery = wired_start_discovery,
    .stop_discovery  = wired_stop_discovery,
    .connect         = wired_connect,
    .disconnect      = wired_disconnect,
    .send_video      = wired_send_video,
    .send_audio      = wired_send_audio,
    .control         = wired_control,
    .get_state       = wired_get_state,
};

WiredContext* wired_create(const MirrorDeviceInfo *device,
                           const MirrorConfig *config)
{
    if (!device || !config) return NULL;

    WiredContext *ctx = (WiredContext *)calloc(1, sizeof(WiredContext));
    if (!ctx) return NULL;
    ctx->session_id = (uint32_t)time(NULL);
    ctx->state      = 0;
    ctx->usb_fd     = -1;

    printf("[WIRED] context created (session: %u)\n", ctx->session_id);
    return ctx;
}

void wired_destroy(WiredContext *ctx)
{
    if (ctx) {
        printf("[WIRED] context destroyed\n");
        free(ctx);
    }
}

ProtocolOps* wired_get_ops(void)
{
    return &wired_ops;
}
