#include "protocols/wfd_protocol.h"
#include "platform/cedarx_platform.h"
#include <dlfcn.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

typedef int (*FnNoArgInt)(void);
typedef int (*FnIntArg)(int);
typedef int (*FnStrArg)(const char *);

typedef struct {
    pthread_mutex_t lock;
    bool initialized;
    bool connected;
    MirrorState state;
    void *libwfd2;
    FnNoArgInt fn_init;
    FnNoArgInt fn_deinit;
    FnIntArg fn_start_discovery;
    FnNoArgInt fn_stop_discovery;
} WfdProtocolCtx;

static WfdProtocolCtx g_wfd_ctx = {
    .initialized = false,
    .connected = false,
    .state = MIRROR_STATE_IDLE,
    .libwfd2 = NULL,
};

static void* resolve_any(void *lib, const char *const *symbols, size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        void *fn = dlsym(lib, symbols[i]);
        if (fn != NULL) {
            return fn;
        }
    }
    return NULL;
}

static int wfd_init(void)
{
    if (g_wfd_ctx.initialized) {
        return MIRROR_ERR_SUCCESS;
    }

    if (pthread_mutex_init(&g_wfd_ctx.lock, NULL) != 0) {
        return MIRROR_ERR_UNKNOWN;
    }

    pthread_mutex_lock(&g_wfd_ctx.lock);

    g_wfd_ctx.libwfd2 = dlopen("libwfd2.so", RTLD_LAZY | RTLD_LOCAL);
    if (g_wfd_ctx.libwfd2 != NULL) {
        static const char *const init_syms[] = {"Miracast_Init", "WFD_Init"};
        static const char *const deinit_syms[] = {"Miracast_DeInit", "WFD_Deinit", "WFD_Exit"};
        static const char *const start_syms[] = {"Miracast_Start", "WFD_StartDiscovery"};
        static const char *const stop_syms[] = {"Miracast_Stop", "WFD_StopDiscovery"};

        g_wfd_ctx.fn_init = (FnNoArgInt)resolve_any(g_wfd_ctx.libwfd2, init_syms, 2);
        g_wfd_ctx.fn_deinit = (FnNoArgInt)resolve_any(g_wfd_ctx.libwfd2, deinit_syms, 3);
        g_wfd_ctx.fn_start_discovery = (FnIntArg)resolve_any(g_wfd_ctx.libwfd2, start_syms, 2);
        g_wfd_ctx.fn_stop_discovery = (FnNoArgInt)resolve_any(g_wfd_ctx.libwfd2, stop_syms, 2);
    } else {
        printf("[WFD] libwfd2.so not found, using fallback stub behavior\n");
    }

    g_wfd_ctx.initialized = true;
    g_wfd_ctx.state = MIRROR_STATE_IDLE;

    pthread_mutex_unlock(&g_wfd_ctx.lock);

    (void)cedarx_platform_init();
    return MIRROR_ERR_SUCCESS;
}

static void wfd_exit(void)
{
    if (!g_wfd_ctx.initialized) {
        return;
    }

    pthread_mutex_lock(&g_wfd_ctx.lock);

    if (g_wfd_ctx.fn_deinit != NULL) {
        g_wfd_ctx.fn_deinit();
    }
    if (g_wfd_ctx.libwfd2 != NULL) {
        dlclose(g_wfd_ctx.libwfd2);
        g_wfd_ctx.libwfd2 = NULL;
    }
    g_wfd_ctx.initialized = false;
    g_wfd_ctx.connected = false;
    g_wfd_ctx.state = MIRROR_STATE_IDLE;

    pthread_mutex_unlock(&g_wfd_ctx.lock);
    pthread_mutex_destroy(&g_wfd_ctx.lock);
}

static int wfd_start_discovery(int timeout_ms)
{
    if (!g_wfd_ctx.initialized) {
        int ret = wfd_init();
        if (ret != MIRROR_ERR_SUCCESS) {
            return ret;
        }
    }

    pthread_mutex_lock(&g_wfd_ctx.lock);
    g_wfd_ctx.state = MIRROR_STATE_DISCOVERING;
    pthread_mutex_unlock(&g_wfd_ctx.lock);

    if (g_wfd_ctx.fn_start_discovery != NULL) {
        return g_wfd_ctx.fn_start_discovery(timeout_ms);
    }
    return MIRROR_ERR_SUCCESS;
}

static void wfd_stop_discovery(void)
{
    if (!g_wfd_ctx.initialized) {
        return;
    }

    if (g_wfd_ctx.fn_stop_discovery != NULL) {
        g_wfd_ctx.fn_stop_discovery();
    }

    pthread_mutex_lock(&g_wfd_ctx.lock);
    g_wfd_ctx.state = MIRROR_STATE_IDLE;
    pthread_mutex_unlock(&g_wfd_ctx.lock);
}

static int wfd_connect(const MirrorDeviceInfo *device, const MirrorConfig *config)
{
    if (device == NULL || config == NULL) {
        return MIRROR_ERR_INVALID_PARAM;
    }

    if (!g_wfd_ctx.initialized) {
        int ret = wfd_init();
        if (ret != MIRROR_ERR_SUCCESS) {
            return ret;
        }
    }

    int ret = cedarx_platform_open_session(config);
    if (ret != MIRROR_ERR_SUCCESS) {
        return ret;
    }

    pthread_mutex_lock(&g_wfd_ctx.lock);
    g_wfd_ctx.connected = true;
    g_wfd_ctx.state = MIRROR_STATE_CONNECTED;
    pthread_mutex_unlock(&g_wfd_ctx.lock);

    printf("[WFD] Connected device: %s (%s)\n", device->name, device->ip_address);
    return MIRROR_ERR_SUCCESS;
}

static void wfd_disconnect(void)
{
    if (!g_wfd_ctx.initialized) {
        return;
    }

    cedarx_platform_close_session();

    pthread_mutex_lock(&g_wfd_ctx.lock);
    g_wfd_ctx.connected = false;
    g_wfd_ctx.state = MIRROR_STATE_IDLE;
    pthread_mutex_unlock(&g_wfd_ctx.lock);
}

static int wfd_send_video(const uint8_t *data, int size)
{
    if (data == NULL || size <= 0) {
        return MIRROR_ERR_INVALID_PARAM;
    }

    if (!g_wfd_ctx.connected) {
        return MIRROR_ERR_DEVICE_BUSY;
    }

    return size;
}

static int wfd_send_audio(const uint8_t *data, int size)
{
    if (data == NULL || size <= 0) {
        return MIRROR_ERR_INVALID_PARAM;
    }

    if (!g_wfd_ctx.connected) {
        return MIRROR_ERR_DEVICE_BUSY;
    }

    return size;
}

static int wfd_control(const char *command)
{
    if (command == NULL) {
        return MIRROR_ERR_INVALID_PARAM;
    }

    pthread_mutex_lock(&g_wfd_ctx.lock);
    if (strcmp(command, "pause") == 0) {
        g_wfd_ctx.state = MIRROR_STATE_PAUSED;
    } else if (strcmp(command, "resume") == 0) {
        g_wfd_ctx.state = MIRROR_STATE_STREAMING;
    } else if (strcmp(command, "stop") == 0) {
        g_wfd_ctx.state = MIRROR_STATE_IDLE;
    }
    pthread_mutex_unlock(&g_wfd_ctx.lock);

    return MIRROR_ERR_SUCCESS;
}

static MirrorState wfd_get_state(void)
{
    MirrorState state = MIRROR_STATE_IDLE;

    if (!g_wfd_ctx.initialized) {
        return state;
    }

    pthread_mutex_lock(&g_wfd_ctx.lock);
    state = g_wfd_ctx.state;
    pthread_mutex_unlock(&g_wfd_ctx.lock);

    return state;
}

static ProtocolOps g_wfd_ops = {
    .init = wfd_init,
    .exit = wfd_exit,
    .start_discovery = wfd_start_discovery,
    .stop_discovery = wfd_stop_discovery,
    .connect = wfd_connect,
    .disconnect = wfd_disconnect,
    .send_video = wfd_send_video,
    .send_audio = wfd_send_audio,
    .control = wfd_control,
    .get_state = wfd_get_state,
};

ProtocolOps* wfd_protocol_get_ops(void)
{
    return &g_wfd_ops;
}
