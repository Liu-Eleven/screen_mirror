/*
 * dlna.c - DLNA protocol implementation
 *
 * On H133 (H133_BOARD defined): uses AWD_Instance/Start/Stop/Destroy
 * from libawdlna.so, via awcast_api.h declarations.
 * Renderer handle (RenderT) is treated as opaque; on H133 the real
 * CedarRender_Instance() would be called.
 *
 * Without H133: stub implementation.
 */

#include "protocols/dlna.h"
#include "bsp/awcast_api.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

/* ── internal state ──────────────────────────────────────────────────────── */

static struct {
    int              state;   /* 0=idle, 1=running, 2=streaming */
    char             device_name[64];
    char             uuid[64];
    void            *awd;    /* AWDlnaT* on H133 */
    void            *render; /* RenderT* on H133 */
    pthread_mutex_t  lock;
} g_dlna = {
    .state       = 0,
    .device_name = "AWCast",
    .uuid        = "",
    .awd         = NULL,
    .render      = NULL,
    .lock        = PTHREAD_MUTEX_INITIALIZER,
};

#ifdef H133_BOARD

/* ── UUID generation ─────────────────────────────────────────────────────── */
static void generate_uuid(char *uuid, size_t len)
{
    unsigned char rnd[16];
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd >= 0) {
        ssize_t n = read(fd, rnd, sizeof(rnd));
        close(fd);
        if (n == (ssize_t)sizeof(rnd)) {
            /* Set version 4 bits (random UUID) */
            rnd[6] = (rnd[6] & 0x0f) | 0x40;
            rnd[8] = (rnd[8] & 0x3f) | 0x80;
            snprintf(uuid, len,
                "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x"
                "-%02x%02x%02x%02x%02x%02x",
                rnd[0], rnd[1], rnd[2], rnd[3],
                rnd[4], rnd[5], rnd[6], rnd[7],
                rnd[8], rnd[9],
                rnd[10], rnd[11], rnd[12], rnd[13], rnd[14], rnd[15]);
            return;
        }
    }
    /* Fallback: use time-based random (less secure but functional) */
    srand((unsigned int)time(NULL) ^ (unsigned int)getpid());
    snprintf(uuid, len, "%08x-%04x-4%03x-%04x-%012x",
             rand(), rand() & 0xFFFF, rand() & 0x0FFF,
             (rand() & 0x3FFF) | 0x8000, rand());
}

/* ── network reachability check (for DLNA) ───────────────────────────────── */
static int check_net_connection(const char *host, int port)
{
    struct sockaddr_in sa;
    struct hostent *he = gethostbyname(host);
    if (!he) return -1;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port   = htons(port);
    memcpy(&sa.sin_addr, he->h_addr_list[0], he->h_length);
    struct timeval tv = {.tv_sec = 2, .tv_usec = 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    int ret = connect(sock, (struct sockaddr *)&sa, sizeof(sa));
    close(sock);
    return (ret == 0) ? 0 : -1;
}

/* ── DLNA event listener ─────────────────────────────────────────────────── */
static int dlna_notify(int event, void *param)
{
    (void)param;
    printf("[DLNA] event: %d\n", event);
    switch (event) {
    case AWD_EVENT_ENTRY:
        printf("[DLNA] media playback started\n");
        pthread_mutex_lock(&g_dlna.lock);
        g_dlna.state = 2;
        pthread_mutex_unlock(&g_dlna.lock);
        break;
    case AWD_EVENT_QUIT:
        printf("[DLNA] media playback stopped\n");
        pthread_mutex_lock(&g_dlna.lock);
        g_dlna.state = 1;
        pthread_mutex_unlock(&g_dlna.lock);
        break;
    default: break;
    }
    return 0;
}

static struct AWD_LinstenerS dlna_listener = { .notify = dlna_notify };

#endif /* H133_BOARD */

/* ── ProtocolOps implementation ──────────────────────────────────────────── */

static int dlna_init(void)
{
    printf("[DLNA] init\n");
#ifdef H133_BOARD
    /* Network check – DLNA requires IP connectivity */
    if (check_net_connection("8.8.8.8", 53) != 0) {
        printf("[DLNA] no network connectivity, DLNA unavailable\n");
        return MIRROR_ERR_CONNECT_FAILED;
    }

    generate_uuid(g_dlna.uuid, sizeof(g_dlna.uuid));

    /* On H133, create the Cedarx renderer.
     * CedarRender_Instance() is provided by libcedarx / libirender. */
    /* g_dlna.render = CedarRender_Instance(NULL); */
    /* For now we pass NULL render to AWD_Instance; BSP integrator should
     * call dlna_set_render() if a real renderer is required. */

    AWDlnaT *awd = AWD_Instance(g_dlna.device_name, g_dlna.uuid,
                                 &dlna_listener,
                                 (RenderT *)g_dlna.render);
    if (!awd) {
        printf("[DLNA] AWD_Instance failed\n");
        return MIRROR_ERR_UNKNOWN;
    }
    pthread_mutex_lock(&g_dlna.lock);
    g_dlna.awd   = awd;
    g_dlna.state = 0;
    pthread_mutex_unlock(&g_dlna.lock);
#endif
    return MIRROR_ERR_SUCCESS;
}

static void dlna_exit(void)
{
    printf("[DLNA] exit\n");
#ifdef H133_BOARD
    pthread_mutex_lock(&g_dlna.lock);
    AWDlnaT *awd = (AWDlnaT *)g_dlna.awd;
    g_dlna.awd   = NULL;
    g_dlna.state = 0;
    pthread_mutex_unlock(&g_dlna.lock);

    if (awd) {
        AWD_Stop(awd);
        AWD_Destroy(awd);
    }
    if (g_dlna.render) {
        /* RenderDestroy(g_dlna.render); */
        g_dlna.render = NULL;
    }
#endif
}

static int dlna_start_discovery(int timeout_ms)
{
    (void)timeout_ms;
    printf("[DLNA] start_discovery (UPnP announce)\n");
#ifdef H133_BOARD
    pthread_mutex_lock(&g_dlna.lock);
    AWDlnaT *awd = (AWDlnaT *)g_dlna.awd;
    pthread_mutex_unlock(&g_dlna.lock);

    if (!awd) {
        if (dlna_init() != MIRROR_ERR_SUCCESS)
            return MIRROR_ERR_UNKNOWN;
        pthread_mutex_lock(&g_dlna.lock);
        awd = (AWDlnaT *)g_dlna.awd;
        pthread_mutex_unlock(&g_dlna.lock);
    }
    if (!awd) return MIRROR_ERR_UNKNOWN;

    int ret = AWD_Start(awd);
    if (ret == 0) {
        pthread_mutex_lock(&g_dlna.lock);
        g_dlna.state = 1;
        pthread_mutex_unlock(&g_dlna.lock);
    }
    return (ret == 0) ? MIRROR_ERR_SUCCESS : MIRROR_ERR_UNKNOWN;
#else
    return MIRROR_ERR_SUCCESS;
#endif
}

static void dlna_stop_discovery(void)
{
    printf("[DLNA] stop_discovery\n");
#ifdef H133_BOARD
    pthread_mutex_lock(&g_dlna.lock);
    AWDlnaT *awd = (AWDlnaT *)g_dlna.awd;
    pthread_mutex_unlock(&g_dlna.lock);
    if (awd) {
        AWD_Stop(awd);
        AWD_Render_Stop();
    }
    pthread_mutex_lock(&g_dlna.lock);
    g_dlna.state = 0;
    pthread_mutex_unlock(&g_dlna.lock);
#endif
}

static int dlna_connect(const MirrorDeviceInfo *device,
                        const MirrorConfig *config)
{
    (void)device; (void)config;
    printf("[DLNA] connect\n");
    return dlna_start_discovery(0);
}

static void dlna_disconnect(void)
{
    printf("[DLNA] disconnect\n");
    dlna_stop_discovery();
}

static int dlna_send_video(const uint8_t *data, int size)
{
    /* DLNA is a pull protocol; the renderer pulls from the URL.
     * send_video is not used. */
    (void)data;
    return size;
}

static int dlna_send_audio(const uint8_t *data, int size)
{
    (void)data;
    return size;
}

static int dlna_control(const char *command)
{
    printf("[DLNA] control: %s\n", command ? command : "(null)");
    if (!command) return MIRROR_ERR_INVALID_PARAM;
    if (strcmp(command, "stop") == 0) {
        dlna_disconnect();
    } else if (strcmp(command, "start") == 0) {
        dlna_start_discovery(0);
    }
    return MIRROR_ERR_SUCCESS;
}

static MirrorState dlna_get_state(void)
{
    int s;
    pthread_mutex_lock(&g_dlna.lock);
    s = g_dlna.state;
    pthread_mutex_unlock(&g_dlna.lock);
    if (s == 2) return MIRROR_STATE_STREAMING;
    if (s == 1) return MIRROR_STATE_DISCOVERING;
    return MIRROR_STATE_IDLE;
}

/* ── public symbols ──────────────────────────────────────────────────────── */

static ProtocolOps dlna_ops = {
    .init            = dlna_init,
    .exit            = dlna_exit,
    .start_discovery = dlna_start_discovery,
    .stop_discovery  = dlna_stop_discovery,
    .connect         = dlna_connect,
    .disconnect      = dlna_disconnect,
    .send_video      = dlna_send_video,
    .send_audio      = dlna_send_audio,
    .control         = dlna_control,
    .get_state       = dlna_get_state,
};

DlnaContext* dlna_create(const MirrorDeviceInfo *device,
                         const MirrorConfig *config)
{
    if (!device || !config) return NULL;
    DlnaContext *ctx = (DlnaContext *)calloc(1, sizeof(DlnaContext));
    if (!ctx) return NULL;
    ctx->session_id = (uint32_t)time(NULL);
    ctx->state      = 0;
    strncpy(g_dlna.device_name, device->name,
            sizeof(g_dlna.device_name) - 1);
    printf("[DLNA] context created (session: %u)\n", ctx->session_id);
    return ctx;
}

void dlna_destroy(DlnaContext *ctx)
{
    if (ctx) {
        printf("[DLNA] context destroyed\n");
        free(ctx);
    }
}

ProtocolOps* dlna_get_ops(void)
{
    return &dlna_ops;
}

/** Allow the engine to set an external renderer (H133 only). */
void dlna_set_render(void *render)
{
    pthread_mutex_lock(&g_dlna.lock);
    g_dlna.render = render;
    pthread_mutex_unlock(&g_dlna.lock);
}
