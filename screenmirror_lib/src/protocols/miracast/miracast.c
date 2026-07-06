/*
 * miracast.c - Miracast protocol implementation
 *
 * On H133 (H133_BOARD defined): calls Miracast_Init/Start/Stop/DeInit
 * from libwfd2.so via the awcast_api.h declarations.
 *
 * Without H133: stub implementation that logs operations.
 */

#include "protocols/miracast.h"
#include "bsp/awcast_api.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

/* ── internal state ──────────────────────────────────────────────────────── */

#define HDCP_KEY_FILE  "/data/miracast.dat"
#define HDCP_KEY_SIZE  902   /* KEYSETSIZE - 10 */

static struct {
    int          state;    /* 0=idle, 1=running, 2=connected */
    char         device_name[64];
    pthread_mutex_t lock;
} g_miracast = {
    .state       = 0,
    .device_name = "AWCast",
    .lock        = PTHREAD_MUTEX_INITIALIZER,
};

#ifdef H133_BOARD

static int load_hdcp_key(void)
{
    unsigned char key_buf[1024] = {0};
    int fd = open(HDCP_KEY_FILE, O_RDONLY);
    if (fd < 0) {
        printf("[MIRACAST] HDCP key file not found, running without HDCP\n");
        return -1;
    }
    ssize_t n = read(fd, key_buf, sizeof(key_buf));
    close(fd);
    if (n != HDCP_KEY_SIZE) {
        printf("[MIRACAST] HDCP key size mismatch (%zd != %d)\n", n, HDCP_KEY_SIZE);
        return -1;
    }
    Miracast_SetKey(key_buf);
    printf("[MIRACAST] HDCP key loaded\n");
    return 0;
}

static int __miracast_event_cb(MIRACAST_EVENT_CALLBACK_E event, void *priv)
{
    (void)priv;
    printf("[MIRACAST] event: %d\n", (int)event);
    switch (event) {
    case MIRACAST_CBK_P2P_CONNECTED:
        pthread_mutex_lock(&g_miracast.lock);
        g_miracast.state = 2;
        pthread_mutex_unlock(&g_miracast.lock);
        break;
    case MIRACAST_CBK_P2P_DISCONNECTED:
        pthread_mutex_lock(&g_miracast.lock);
        g_miracast.state = 1;  /* back to listening */
        pthread_mutex_unlock(&g_miracast.lock);
        break;
    case MIRACAST_CBK_PLAYER_FIRST_SHOW:
        printf("[MIRACAST] first video frame shown\n");
        break;
    default:
        break;
    }
    return 0;
}

#endif /* H133_BOARD */

/* ── ProtocolOps implementation ──────────────────────────────────────────── */

static int miracast_init(void)
{
    printf("[MIRACAST] init\n");
#ifdef H133_BOARD
    int hdcp = (load_hdcp_key() == 0) ? 1 : 0;
    int ret = Miracast_Init(hdcp);
    if (ret != 0) {
        printf("[MIRACAST] Miracast_Init failed: %d\n", ret);
        return MIRROR_ERR_UNKNOWN;
    }
    pthread_mutex_lock(&g_miracast.lock);
    g_miracast.state = 0;
    pthread_mutex_unlock(&g_miracast.lock);
#endif
    return MIRROR_ERR_SUCCESS;
}

static void miracast_exit(void)
{
    printf("[MIRACAST] exit\n");
#ifdef H133_BOARD
    Miracast_Stop();
    Miracast_DeInit();
    pthread_mutex_lock(&g_miracast.lock);
    g_miracast.state = 0;
    pthread_mutex_unlock(&g_miracast.lock);
#endif
}

static int miracast_start_discovery(int timeout_ms)
{
    (void)timeout_ms;
    printf("[MIRACAST] start_discovery (P2P listen)\n");
#ifdef H133_BOARD
    int ret = Miracast_Start(g_miracast.device_name, __miracast_event_cb);
    if (ret != 0) {
        printf("[MIRACAST] Miracast_Start failed: %d\n", ret);
        return MIRROR_ERR_UNKNOWN;
    }
    pthread_mutex_lock(&g_miracast.lock);
    g_miracast.state = 1;
    pthread_mutex_unlock(&g_miracast.lock);
#endif
    return MIRROR_ERR_SUCCESS;
}

static void miracast_stop_discovery(void)
{
    printf("[MIRACAST] stop_discovery\n");
#ifdef H133_BOARD
    Miracast_Stop();
    pthread_mutex_lock(&g_miracast.lock);
    g_miracast.state = 0;
    pthread_mutex_unlock(&g_miracast.lock);
#endif
}

static int miracast_connect(const MirrorDeviceInfo *device,
                            const MirrorConfig *config)
{
    (void)config;
    printf("[MIRACAST] connect to: %s\n", device ? device->name : "NULL");
    /* Miracast connection is initiated by the sender (phone/laptop);
     * the receiver (us) just needs to be in listening state.
     * If not already started, start the service now. */
#ifdef H133_BOARD
    pthread_mutex_lock(&g_miracast.lock);
    int state = g_miracast.state;
    pthread_mutex_unlock(&g_miracast.lock);
    if (state == 0) {
        return miracast_start_discovery(0);
    }
#endif
    return MIRROR_ERR_SUCCESS;
}

static void miracast_disconnect(void)
{
    printf("[MIRACAST] disconnect\n");
#ifdef H133_BOARD
    Miracast_Stop();
    pthread_mutex_lock(&g_miracast.lock);
    g_miracast.state = 0;
    pthread_mutex_unlock(&g_miracast.lock);
#endif
}

static int miracast_send_video(const uint8_t *data, int size)
{
    /* Miracast sink: video is rendered internally by libwfd_player.
     * This function is not applicable for the sink role. */
    (void)data;
    return size;
}

static int miracast_send_audio(const uint8_t *data, int size)
{
    /* Same as send_video – audio is handled internally. */
    (void)data;
    return size;
}

static int miracast_control(const char *command)
{
    printf("[MIRACAST] control: %s\n", command ? command : "(null)");
    if (!command) return MIRROR_ERR_INVALID_PARAM;
    if (strcmp(command, "stop") == 0) {
        miracast_disconnect();
    } else if (strcmp(command, "start") == 0) {
        miracast_start_discovery(0);
    }
    return MIRROR_ERR_SUCCESS;
}

static MirrorState miracast_get_state(void)
{
    int s;
    pthread_mutex_lock(&g_miracast.lock);
    s = g_miracast.state;
    pthread_mutex_unlock(&g_miracast.lock);
    if (s == 2) return MIRROR_STATE_STREAMING;
    if (s == 1) return MIRROR_STATE_DISCOVERING;
    return MIRROR_STATE_IDLE;
}

/* ── public symbols ──────────────────────────────────────────────────────── */

static ProtocolOps miracast_ops = {
    .init            = miracast_init,
    .exit            = miracast_exit,
    .start_discovery = miracast_start_discovery,
    .stop_discovery  = miracast_stop_discovery,
    .connect         = miracast_connect,
    .disconnect      = miracast_disconnect,
    .send_video      = miracast_send_video,
    .send_audio      = miracast_send_audio,
    .control         = miracast_control,
    .get_state       = miracast_get_state,
};

MiracastContext* miracast_create(const MirrorDeviceInfo *device,
                                 const MirrorConfig *config)
{
    if (!device || !config) return NULL;

    MiracastContext *ctx = (MiracastContext *)calloc(1, sizeof(MiracastContext));
    if (!ctx) return NULL;
    ctx->session_id = (uint32_t)time(NULL);
    ctx->state      = 0;

    /* store device name for P2P advertising */
    strncpy(g_miracast.device_name, device->name,
            sizeof(g_miracast.device_name) - 1);

    printf("[MIRACAST] context created (session: %u)\n", ctx->session_id);
    return ctx;
}

void miracast_destroy(MiracastContext *ctx)
{
    if (ctx) {
        printf("[MIRACAST] context destroyed\n");
        free(ctx);
    }
}

ProtocolOps* miracast_get_ops(void)
{
    return &miracast_ops;
}

/** Allow the engine to set the P2P advertisement name. */
void miracast_set_device_name(const char *name)
{
    if (name) {
        strncpy(g_miracast.device_name, name,
                sizeof(g_miracast.device_name) - 1);
    }
}
