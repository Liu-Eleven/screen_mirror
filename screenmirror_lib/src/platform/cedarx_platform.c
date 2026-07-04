#include "platform/cedarx_platform.h"
#include "screenmirror_internal.h"
#include <dlfcn.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    bool initialized;
    bool session_open;
    MirrorConfig config;
    pthread_mutex_t lock;
    void *libwfd2;
    void *libtplayer;
    void *libwfd_player;
    void *libawrecorder;
} CedarxPlatformCtx;

static CedarxPlatformCtx g_cedarx_ctx = {
    .initialized = false,
    .session_open = false,
};

static void* open_bsp_library(const char *name)
{
    void *handle = dlopen(name, RTLD_LAZY | RTLD_LOCAL);
    if (handle == NULL) {
        printf("[PLATFORM] Optional BSP library not available: %s (%s)\n", name, dlerror());
    }
    return handle;
}

/**
 * Cedarx 平台初始化
 */
int cedarx_platform_init(void)
{
    if (g_cedarx_ctx.initialized) {
        return MIRROR_ERR_SUCCESS;
    }

    if (pthread_mutex_init(&g_cedarx_ctx.lock, NULL) != 0) {
        return MIRROR_ERR_UNKNOWN;
    }

    pthread_mutex_lock(&g_cedarx_ctx.lock);

    g_cedarx_ctx.libwfd2 = open_bsp_library("libwfd2.so");
    g_cedarx_ctx.libtplayer = open_bsp_library("libtplayer.so");
    g_cedarx_ctx.libwfd_player = open_bsp_library("libwfd_player.so");
    g_cedarx_ctx.libawrecorder = open_bsp_library("libawrecorder.so");
    g_cedarx_ctx.initialized = true;

    pthread_mutex_unlock(&g_cedarx_ctx.lock);

    printf("[PLATFORM] Cedarx platform initialized\n");
    return MIRROR_ERR_SUCCESS;
}

/**
 * Cedarx 平台反初始化
 */
void cedarx_platform_exit(void)
{
    if (!g_cedarx_ctx.initialized) {
        return;
    }

    pthread_mutex_lock(&g_cedarx_ctx.lock);

    if (g_cedarx_ctx.libwfd2 != NULL) {
        dlclose(g_cedarx_ctx.libwfd2);
        g_cedarx_ctx.libwfd2 = NULL;
    }
    if (g_cedarx_ctx.libtplayer != NULL) {
        dlclose(g_cedarx_ctx.libtplayer);
        g_cedarx_ctx.libtplayer = NULL;
    }
    if (g_cedarx_ctx.libwfd_player != NULL) {
        dlclose(g_cedarx_ctx.libwfd_player);
        g_cedarx_ctx.libwfd_player = NULL;
    }
    if (g_cedarx_ctx.libawrecorder != NULL) {
        dlclose(g_cedarx_ctx.libawrecorder);
        g_cedarx_ctx.libawrecorder = NULL;
    }

    g_cedarx_ctx.session_open = false;
    g_cedarx_ctx.initialized = false;

    pthread_mutex_unlock(&g_cedarx_ctx.lock);
    pthread_mutex_destroy(&g_cedarx_ctx.lock);

    printf("[PLATFORM] Cedarx platform exited\n");
}

int cedarx_platform_open_session(const MirrorConfig *config)
{
    if (config == NULL) {
        return MIRROR_ERR_INVALID_PARAM;
    }

    if (!g_cedarx_ctx.initialized) {
        int ret = cedarx_platform_init();
        if (ret != MIRROR_ERR_SUCCESS) {
            return ret;
        }
    }

    pthread_mutex_lock(&g_cedarx_ctx.lock);
    memcpy(&g_cedarx_ctx.config, config, sizeof(MirrorConfig));
    g_cedarx_ctx.session_open = true;
    pthread_mutex_unlock(&g_cedarx_ctx.lock);

    printf("[PLATFORM] Session opened: %dx%d@%d, audio=%d\n",
           config->resolution_width,
           config->resolution_height,
           config->refresh_rate,
           config->enable_audio ? 1 : 0);

    return MIRROR_ERR_SUCCESS;
}

void cedarx_platform_close_session(void)
{
    if (!g_cedarx_ctx.initialized) {
        return;
    }

    pthread_mutex_lock(&g_cedarx_ctx.lock);
    g_cedarx_ctx.session_open = false;
    pthread_mutex_unlock(&g_cedarx_ctx.lock);
}

bool cedarx_platform_is_ready(void)
{
    bool ready;

    if (!g_cedarx_ctx.initialized) {
        return false;
    }

    pthread_mutex_lock(&g_cedarx_ctx.lock);
    ready = g_cedarx_ctx.session_open;
    pthread_mutex_unlock(&g_cedarx_ctx.lock);

    return ready;
}

/**
 * 获取 Cedarx 编码器句柄
 */
void* cedarx_get_encoder(void)
{
    if (!g_cedarx_ctx.initialized) {
        return NULL;
    }

    return g_cedarx_ctx.libtplayer;
}

/**
 * 获取 Cedarx 渲染器句柄
 */
void* cedarx_get_renderer(void)
{
    if (!g_cedarx_ctx.initialized) {
        return NULL;
    }

    return g_cedarx_ctx.libwfd_player;
}

void* cedarx_get_audio_recorder(void)
{
    if (!g_cedarx_ctx.initialized) {
        return NULL;
    }

    return g_cedarx_ctx.libawrecorder;
}
