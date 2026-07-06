/*
 * wireless.c - All-in-one wireless cast (AWCast integration)
 *
 * MIRROR_MODE_WIRELESS starts the complete AWCast service which
 * simultaneously listens for Miracast (P2P), DLNA (UPnP) and AirPlay
 * connections.  The "first to connect" wins.
 *
 * On H133 (H133_BOARD defined): calls AWCast_init / AWCast_StartService /
 * AWCast_StopService / AWCast_exit from the lv_projector AWCast module.
 *
 * Without H133: stub implementation.
 */

#include "protocols/wireless.h"
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
} g_wireless = {
    .state = 0,
    .lock  = PTHREAD_MUTEX_INITIALIZER,
};

/* ── ProtocolOps implementation ──────────────────────────────────────────── */

static int wireless_init(void)
{
    printf("[WIRELESS] init (AWCast)\n");
#ifdef H133_BOARD
    int ret = AWCast_init();
    if (ret != 0) {
        printf("[WIRELESS] AWCast_init failed: %d\n", ret);
        return MIRROR_ERR_UNKNOWN;
    }
#endif
    pthread_mutex_lock(&g_wireless.lock);
    g_wireless.state = 0;
    pthread_mutex_unlock(&g_wireless.lock);
    return MIRROR_ERR_SUCCESS;
}

static void wireless_exit(void)
{
    printf("[WIRELESS] exit (AWCast)\n");
#ifdef H133_BOARD
    AWCast_StopService();
    AWCast_exit();
#endif
    pthread_mutex_lock(&g_wireless.lock);
    g_wireless.state = 0;
    pthread_mutex_unlock(&g_wireless.lock);
}

static int wireless_start_discovery(int timeout_ms)
{
    (void)timeout_ms;
    printf("[WIRELESS] start_discovery – starting AWCast service\n");
#ifdef H133_BOARD
    int ret = AWCast_StartService();
    if (ret != 0) {
        printf("[WIRELESS] AWCast_StartService failed: %d\n", ret);
        return MIRROR_ERR_UNKNOWN;
    }
    pthread_mutex_lock(&g_wireless.lock);
    g_wireless.state = 1;
    pthread_mutex_unlock(&g_wireless.lock);
#endif
    return MIRROR_ERR_SUCCESS;
}

static void wireless_stop_discovery(void)
{
    printf("[WIRELESS] stop_discovery\n");
#ifdef H133_BOARD
    AWCast_StopService();
    pthread_mutex_lock(&g_wireless.lock);
    g_wireless.state = 0;
    pthread_mutex_unlock(&g_wireless.lock);
#endif
}

static int wireless_connect(const MirrorDeviceInfo *device,
                            const MirrorConfig *config)
{
    (void)device; (void)config;
    printf("[WIRELESS] connect – ensuring AWCast service is running\n");
#ifdef H133_BOARD
    pthread_mutex_lock(&g_wireless.lock);
    int s = g_wireless.state;
    pthread_mutex_unlock(&g_wireless.lock);
    if (s == 0) {
        return wireless_start_discovery(0);
    }
#endif
    return MIRROR_ERR_SUCCESS;
}

static void wireless_disconnect(void)
{
    printf("[WIRELESS] disconnect\n");
#ifdef H133_BOARD
    AWCast_StopService();
    pthread_mutex_lock(&g_wireless.lock);
    g_wireless.state = 0;
    pthread_mutex_unlock(&g_wireless.lock);
#endif
}

static int wireless_send_video(const uint8_t *data, int size)
{
    /* AWCast is sink-side: video arrives via network and is rendered
     * internally.  This callback is not applicable. */
    (void)data;
    return size;
}

static int wireless_send_audio(const uint8_t *data, int size)
{
    (void)data;
    return size;
}

static int wireless_control(const char *command)
{
    printf("[WIRELESS] control: %s\n", command ? command : "(null)");
    if (!command) return MIRROR_ERR_INVALID_PARAM;
    if (strcmp(command, "stop") == 0) {
        wireless_disconnect();
    } else if (strcmp(command, "start") == 0) {
        wireless_start_discovery(0);
    } else if (strcmp(command, "reset") == 0) {
#ifdef H133_BOARD
        AWCast_reset();
#endif
    }
    return MIRROR_ERR_SUCCESS;
}

static MirrorState wireless_get_state(void)
{
    int s;
    pthread_mutex_lock(&g_wireless.lock);
    s = g_wireless.state;
    pthread_mutex_unlock(&g_wireless.lock);
    if (s == 2) return MIRROR_STATE_STREAMING;
    if (s == 1) return MIRROR_STATE_DISCOVERING;
    return MIRROR_STATE_IDLE;
}

/* ── public symbols ──────────────────────────────────────────────────────── */

static ProtocolOps wireless_ops = {
    .init            = wireless_init,
    .exit            = wireless_exit,
    .start_discovery = wireless_start_discovery,
    .stop_discovery  = wireless_stop_discovery,
    .connect         = wireless_connect,
    .disconnect      = wireless_disconnect,
    .send_video      = wireless_send_video,
    .send_audio      = wireless_send_audio,
    .control         = wireless_control,
    .get_state       = wireless_get_state,
};

WirelessContext* wireless_create(const MirrorDeviceInfo *device,
                                 const MirrorConfig *config)
{
    if (!device || !config) return NULL;

    WirelessContext *ctx = (WirelessContext *)calloc(1, sizeof(WirelessContext));
    if (!ctx) return NULL;
    ctx->session_id = (uint32_t)time(NULL);
    ctx->state      = 0;
    ctx->tcp_port   = 7236;  /* WFD / Miracast default port */
    strncpy(ctx->ip_address, device->ip_address, sizeof(ctx->ip_address) - 1);

    printf("[WIRELESS] context created (session: %u)\n", ctx->session_id);
    return ctx;
}

void wireless_destroy(WirelessContext *ctx)
{
    if (ctx) {
        printf("[WIRELESS] context destroyed\n");
        free(ctx);
    }
}

ProtocolOps* wireless_get_ops(void)
{
    return &wireless_ops;
}
