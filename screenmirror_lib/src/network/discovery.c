#include "network/discovery.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

/* 设备发现实现 */

static bool g_discovery_running = false;
static pthread_t g_discovery_thread;
static pthread_mutex_t g_discovery_lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    MirrorMode mode;
    int timeout_ms;
    MirrorDeviceListCallback callback;
    void *user_data;
} DiscoveryThreadArgs;

/**
 * 设备发现线程函数
 */
static void* discovery_thread_func(void *arg)
{
    DiscoveryThreadArgs *args = (DiscoveryThreadArgs *)arg;
    if (args == NULL) {
        return NULL;
    }

    usleep((useconds_t)(args->timeout_ms > 200 ? 200000 : args->timeout_ms * 1000));

    pthread_mutex_lock(&g_discovery_lock);
    bool running = g_discovery_running;
    pthread_mutex_unlock(&g_discovery_lock);
    if (!running) {
        free(args);
        return NULL;
    }

    MirrorDeviceInfo devices[2];
    memset(devices, 0, sizeof(devices));

    snprintf(devices[0].name, sizeof(devices[0].name), "H133-%d-Device-A", args->mode);
    snprintf(devices[0].mac_address, sizeof(devices[0].mac_address), "00:11:22:33:44:%02d", args->mode);
    snprintf(devices[0].ip_address, sizeof(devices[0].ip_address), "192.168.49.1");
    devices[0].signal_strength = 90;
    devices[0].mode = args->mode;

    snprintf(devices[1].name, sizeof(devices[1].name), "H133-%d-Device-B", args->mode);
    snprintf(devices[1].mac_address, sizeof(devices[1].mac_address), "00:11:22:33:55:%02d", args->mode);
    snprintf(devices[1].ip_address, sizeof(devices[1].ip_address), "192.168.49.2");
    devices[1].signal_strength = 75;
    devices[1].mode = args->mode;

    if (args->callback != NULL) {
        args->callback(devices, 2, args->user_data);
    }

    free(args);

    pthread_mutex_lock(&g_discovery_lock);
    g_discovery_running = false;
    pthread_mutex_unlock(&g_discovery_lock);

    return NULL;
}

/**
 * 启动设备发现
 */
int discovery_start(MirrorMode mode, int timeout_ms,
                   MirrorDeviceListCallback callback, void *user_data)
{
    if (callback == NULL || timeout_ms <= 0) {
        return MIRROR_ERR_INVALID_PARAM;
    }

    pthread_mutex_lock(&g_discovery_lock);
    if (g_discovery_running) {
        pthread_mutex_unlock(&g_discovery_lock);
        return MIRROR_ERR_DEVICE_BUSY;
    }
    g_discovery_running = true;
    pthread_mutex_unlock(&g_discovery_lock);

    DiscoveryThreadArgs *thread_args = (DiscoveryThreadArgs *)calloc(1, sizeof(DiscoveryThreadArgs));
    if (thread_args == NULL) {
        pthread_mutex_lock(&g_discovery_lock);
        g_discovery_running = false;
        pthread_mutex_unlock(&g_discovery_lock);
        return MIRROR_ERR_OUT_OF_MEMORY;
    }

    thread_args->mode = mode;
    thread_args->timeout_ms = timeout_ms;
    thread_args->callback = callback;
    thread_args->user_data = user_data;

    if (pthread_create(&g_discovery_thread, NULL, discovery_thread_func, thread_args) != 0) {
        free(thread_args);
        pthread_mutex_lock(&g_discovery_lock);
        g_discovery_running = false;
        pthread_mutex_unlock(&g_discovery_lock);
        return MIRROR_ERR_UNKNOWN;
    }

    return MIRROR_ERR_SUCCESS;
}

/**
 * 停止设备发现
 */
void discovery_stop(void)
{
    pthread_mutex_lock(&g_discovery_lock);
    bool was_running = g_discovery_running;
    g_discovery_running = false;
    pthread_mutex_unlock(&g_discovery_lock);

    if (!was_running) {
        return;
    }

    pthread_join(g_discovery_thread, NULL);
}

/**
 * 获取发现状态
 */
bool discovery_is_running(void)
{
    bool running;
    pthread_mutex_lock(&g_discovery_lock);
    running = g_discovery_running;
    pthread_mutex_unlock(&g_discovery_lock);
    return running;
}
