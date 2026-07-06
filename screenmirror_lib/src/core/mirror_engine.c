#include "screenmirror_internal.h"
#include "protocols/miracast.h"
#include "protocols/dlna.h"
#include "protocols/airplay.h"
#include "protocols/wired.h"
#include "protocols/wireless.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* 全局引擎实例 */
MirrorEngine *g_mirror_engine = NULL;

/* ── protocol ops lookup ─────────────────────────────────────────────────── */

static ProtocolOps *get_protocol_ops(MirrorMode mode)
{
    switch (mode) {
    case MIRROR_MODE_MIRACAST:  return miracast_get_ops();
    case MIRROR_MODE_DLNA:      return dlna_get_ops();
    case MIRROR_MODE_AIRPLAY:   return airplay_get_ops();
    case MIRROR_MODE_WIRED:     return wired_get_ops();
    case MIRROR_MODE_WIRELESS:  return wireless_get_ops();
    default:                    return NULL;
    }
}

static const char *mode_name(MirrorMode mode)
{
    switch (mode) {
    case MIRROR_MODE_MIRACAST: return "Miracast";
    case MIRROR_MODE_DLNA:     return "DLNA";
    case MIRROR_MODE_AIRPLAY:  return "AirPlay";
    case MIRROR_MODE_WIRED:    return "USB-Wired";
    case MIRROR_MODE_WIRELESS: return "Wireless(AWCast)";
    default:                   return "Unknown";
    }
}

/**
 * 初始化投屏库
 */
int screenmirror_init(void)
{
    if (g_mirror_engine != NULL) {
        return MIRROR_ERR_ALREADY_INIT;
    }

    g_mirror_engine = (MirrorEngine *)malloc(sizeof(MirrorEngine));
    if (g_mirror_engine == NULL) {
        return MIRROR_ERR_OUT_OF_MEMORY;
    }

    memset(g_mirror_engine, 0, sizeof(MirrorEngine));

    if (pthread_mutex_init(&g_mirror_engine->lock, NULL) != 0) {
        free(g_mirror_engine);
        g_mirror_engine = NULL;
        return MIRROR_ERR_UNKNOWN;
    }

    g_mirror_engine->event_sys = event_system_create();
    if (g_mirror_engine->event_sys == NULL) {
        pthread_mutex_destroy(&g_mirror_engine->lock);
        free(g_mirror_engine);
        g_mirror_engine = NULL;
        return MIRROR_ERR_OUT_OF_MEMORY;
    }

    g_mirror_engine->state_machine = state_machine_create();
    if (g_mirror_engine->state_machine == NULL) {
        event_system_destroy(g_mirror_engine->event_sys);
        pthread_mutex_destroy(&g_mirror_engine->lock);
        free(g_mirror_engine);
        g_mirror_engine = NULL;
        return MIRROR_ERR_OUT_OF_MEMORY;
    }

    g_mirror_engine->state            = MIRROR_STATE_IDLE;
    g_mirror_engine->initialized      = true;
    g_mirror_engine->discovery_running = false;
    g_mirror_engine->protocol_ops     = NULL;
    g_mirror_engine->protocol_handle  = NULL;

    printf("[MIRROR] Library initialized (v%s)\n", screenmirror_get_version());

    return MIRROR_ERR_SUCCESS;
}

/**
 * 反初始化投屏库
 */
int screenmirror_exit(void)
{
    if (g_mirror_engine == NULL) {
        return MIRROR_ERR_NOT_INIT;
    }

    pthread_mutex_lock(&g_mirror_engine->lock);

    if (g_mirror_engine->discovery_running) {
        if (g_mirror_engine->protocol_ops &&
            g_mirror_engine->protocol_ops->stop_discovery)
            g_mirror_engine->protocol_ops->stop_discovery();
        g_mirror_engine->discovery_running = false;
    }

    if (g_mirror_engine->state != MIRROR_STATE_IDLE) {
        if (g_mirror_engine->protocol_ops &&
            g_mirror_engine->protocol_ops->disconnect)
            g_mirror_engine->protocol_ops->disconnect();
    }

    if (g_mirror_engine->protocol_ops &&
        g_mirror_engine->protocol_ops->exit)
        g_mirror_engine->protocol_ops->exit();
    g_mirror_engine->protocol_ops    = NULL;
    g_mirror_engine->protocol_handle = NULL;

    state_machine_destroy(g_mirror_engine->state_machine);
    event_system_destroy(g_mirror_engine->event_sys);

    pthread_mutex_unlock(&g_mirror_engine->lock);
    pthread_mutex_destroy(&g_mirror_engine->lock);

    free(g_mirror_engine);
    g_mirror_engine = NULL;

    printf("[MIRROR] Library exited\n");

    return MIRROR_ERR_SUCCESS;
}

/**
 * 设置事件回调
 */
int screenmirror_set_event_callback(MirrorEventCallback callback,
                                    void *user_data)
{
    if (g_mirror_engine == NULL) {
        return MIRROR_ERR_NOT_INIT;
    }
    if (callback == NULL) {
        return MIRROR_ERR_INVALID_PARAM;
    }

    pthread_mutex_lock(&g_mirror_engine->lock);
    g_mirror_engine->user_event_callback = callback;
    g_mirror_engine->user_callback_data  = user_data;
    pthread_mutex_unlock(&g_mirror_engine->lock);

    return MIRROR_ERR_SUCCESS;
}

/* ── internal helpers ────────────────────────────────────────────────────── */

static void emit(MirrorEventType type, int code, const char *msg, void *data)
{
    MirrorEvent event = {
        .type       = type,
        .error_code = code,
        .error_msg  = msg,
        .data       = data,
    };
    event_system_emit(g_mirror_engine->event_sys, &event);
    if (g_mirror_engine->user_event_callback)
        g_mirror_engine->user_event_callback(&event,
                                             g_mirror_engine->user_callback_data);
}

/* ── protocol lifecycle (must be called with lock held) ─────────────────── */

static int activate_protocol(MirrorMode mode)
{
    ProtocolOps *ops = get_protocol_ops(mode);
    if (!ops) {
        printf("[MIRROR] no protocol ops for mode %d\n", (int)mode);
        return MIRROR_ERR_INVALID_PARAM;
    }

    /* exit previous protocol if different */
    if (g_mirror_engine->protocol_ops &&
        g_mirror_engine->protocol_ops != ops) {
        if (g_mirror_engine->protocol_ops->exit)
            g_mirror_engine->protocol_ops->exit();
    }

    g_mirror_engine->protocol_ops = ops;

    if (ops->init) {
        int ret = ops->init();
        if (ret != MIRROR_ERR_SUCCESS) {
            printf("[MIRROR] protocol init failed (%s): %d\n",
                   mode_name(mode), ret);
            g_mirror_engine->protocol_ops = NULL;
            return ret;
        }
    }

    return MIRROR_ERR_SUCCESS;
}

/**
 * 开始设备发现
 */
int screenmirror_start_discovery(MirrorMode mode, int timeout_ms,
                                 MirrorDeviceListCallback callback,
                                 void *user_data)
{
    if (g_mirror_engine == NULL)           return MIRROR_ERR_NOT_INIT;
    if (callback == NULL || timeout_ms <= 0) return MIRROR_ERR_INVALID_PARAM;

    pthread_mutex_lock(&g_mirror_engine->lock);

    if (g_mirror_engine->discovery_running) {
        pthread_mutex_unlock(&g_mirror_engine->lock);
        return MIRROR_ERR_DEVICE_BUSY;
    }

    int ret = activate_protocol(mode);
    if (ret != MIRROR_ERR_SUCCESS) {
        pthread_mutex_unlock(&g_mirror_engine->lock);
        return ret;
    }

    g_mirror_engine->current_mode     = mode;
    g_mirror_engine->discovery_running = true;
    g_mirror_engine->state             = MIRROR_STATE_DISCOVERING;
    state_machine_transition(g_mirror_engine->state_machine,
                              MIRROR_STATE_DISCOVERING);

    int disc_ret = MIRROR_ERR_SUCCESS;
    if (g_mirror_engine->protocol_ops->start_discovery)
        disc_ret = g_mirror_engine->protocol_ops->start_discovery(timeout_ms);

    pthread_mutex_unlock(&g_mirror_engine->lock);

    if (disc_ret != MIRROR_ERR_SUCCESS) {
        pthread_mutex_lock(&g_mirror_engine->lock);
        g_mirror_engine->discovery_running = false;
        g_mirror_engine->state             = MIRROR_STATE_IDLE;
        state_machine_transition(g_mirror_engine->state_machine, MIRROR_STATE_IDLE);
        pthread_mutex_unlock(&g_mirror_engine->lock);
        return disc_ret;
    }

    printf("[MIRROR] Discovery started: mode=%s timeout=%d ms\n",
           mode_name(mode), timeout_ms);

    emit(MIRROR_EVENT_STATE_CHANGED, 0, NULL, NULL);

    return MIRROR_ERR_SUCCESS;
}

/**
 * 停止设备发现
 */
int screenmirror_stop_discovery(void)
{
    if (g_mirror_engine == NULL) return MIRROR_ERR_NOT_INIT;

    pthread_mutex_lock(&g_mirror_engine->lock);

    if (!g_mirror_engine->discovery_running) {
        pthread_mutex_unlock(&g_mirror_engine->lock);
        return MIRROR_ERR_SUCCESS;
    }

    if (g_mirror_engine->protocol_ops &&
        g_mirror_engine->protocol_ops->stop_discovery)
        g_mirror_engine->protocol_ops->stop_discovery();

    g_mirror_engine->discovery_running = false;
    g_mirror_engine->state             = MIRROR_STATE_IDLE;
    state_machine_transition(g_mirror_engine->state_machine, MIRROR_STATE_IDLE);

    pthread_mutex_unlock(&g_mirror_engine->lock);

    printf("[MIRROR] Discovery stopped\n");
    emit(MIRROR_EVENT_DISCOVERY_FINISHED, 0, NULL, NULL);

    return MIRROR_ERR_SUCCESS;
}

/**
 * 连接指定设备
 */
int screenmirror_connect(const MirrorDeviceInfo *device,
                         const MirrorConfig *config)
{
    if (g_mirror_engine == NULL)        return MIRROR_ERR_NOT_INIT;
    if (device == NULL || config == NULL) return MIRROR_ERR_INVALID_PARAM;

    pthread_mutex_lock(&g_mirror_engine->lock);

    if (g_mirror_engine->state != MIRROR_STATE_IDLE &&
        g_mirror_engine->state != MIRROR_STATE_DISCOVERING) {
        pthread_mutex_unlock(&g_mirror_engine->lock);
        return MIRROR_ERR_DEVICE_BUSY;
    }

    /* activate / init the correct protocol backend */
    int ret = activate_protocol(config->mode);
    if (ret != MIRROR_ERR_SUCCESS) {
        pthread_mutex_unlock(&g_mirror_engine->lock);
        return ret;
    }

    memcpy(&g_mirror_engine->current_device, device, sizeof(MirrorDeviceInfo));
    memcpy(&g_mirror_engine->config,         config,  sizeof(MirrorConfig));
    g_mirror_engine->current_mode = config->mode;
    g_mirror_engine->state        = MIRROR_STATE_CONNECTING;
    state_machine_transition(g_mirror_engine->state_machine,
                              MIRROR_STATE_CONNECTING);

    int conn_ret = MIRROR_ERR_SUCCESS;
    if (g_mirror_engine->protocol_ops->connect)
        conn_ret = g_mirror_engine->protocol_ops->connect(device, config);

    if (conn_ret == MIRROR_ERR_SUCCESS) {
        g_mirror_engine->state = MIRROR_STATE_CONNECTED;
        state_machine_transition(g_mirror_engine->state_machine,
                                  MIRROR_STATE_CONNECTED);
    } else {
        g_mirror_engine->state = MIRROR_STATE_ERROR;
        state_machine_transition(g_mirror_engine->state_machine,
                                  MIRROR_STATE_ERROR);
    }

    pthread_mutex_unlock(&g_mirror_engine->lock);

    if (conn_ret == MIRROR_ERR_SUCCESS) {
        printf("[MIRROR] Connected to %s via %s\n",
               device->name, mode_name(config->mode));
        emit(MIRROR_EVENT_CONNECTED, 0, NULL, (void *)device);
    } else {
        printf("[MIRROR] Connect failed to %s: %d\n", device->name, conn_ret);
        emit(MIRROR_EVENT_ERROR, conn_ret, "connect failed", NULL);
    }

    return conn_ret;
}

/**
 * 断开连接
 */
int screenmirror_disconnect(void)
{
    if (g_mirror_engine == NULL) return MIRROR_ERR_NOT_INIT;

    pthread_mutex_lock(&g_mirror_engine->lock);

    if (g_mirror_engine->state == MIRROR_STATE_IDLE) {
        pthread_mutex_unlock(&g_mirror_engine->lock);
        return MIRROR_ERR_SUCCESS;
    }

    if (g_mirror_engine->protocol_ops &&
        g_mirror_engine->protocol_ops->disconnect)
        g_mirror_engine->protocol_ops->disconnect();

    g_mirror_engine->state = MIRROR_STATE_IDLE;
    state_machine_transition(g_mirror_engine->state_machine, MIRROR_STATE_IDLE);

    pthread_mutex_unlock(&g_mirror_engine->lock);

    printf("[MIRROR] Disconnected\n");
    emit(MIRROR_EVENT_DISCONNECTED, 0, NULL, NULL);

    return MIRROR_ERR_SUCCESS;
}

/**
 * 获取当前状态
 */
MirrorState screenmirror_get_state(void)
{
    if (g_mirror_engine == NULL) return MIRROR_STATE_IDLE;
    pthread_mutex_lock(&g_mirror_engine->lock);
    MirrorState s = g_mirror_engine->state;
    pthread_mutex_unlock(&g_mirror_engine->lock);
    return s;
}

/**
 * 获取当前连接的设备信息
 */
const MirrorDeviceInfo* screenmirror_get_device_info(void)
{
    if (g_mirror_engine == NULL) return NULL;
    if (g_mirror_engine->state == MIRROR_STATE_IDLE) return NULL;
    return &g_mirror_engine->current_device;
}

/**
 * 获取当前投屏配置
 */
const MirrorConfig* screenmirror_get_config(void)
{
    if (g_mirror_engine == NULL) return NULL;
    if (g_mirror_engine->state == MIRROR_STATE_IDLE) return NULL;
    return &g_mirror_engine->config;
}

/**
 * 发送视频数据帧
 */
int screenmirror_send_video_frame(const uint8_t *data, int size)
{
    if (g_mirror_engine == NULL)   return MIRROR_ERR_NOT_INIT;
    if (data == NULL || size <= 0) return MIRROR_ERR_INVALID_PARAM;

    if (g_mirror_engine->state != MIRROR_STATE_STREAMING &&
        g_mirror_engine->state != MIRROR_STATE_CONNECTED)
        return MIRROR_ERR_DEVICE_BUSY;

    if (g_mirror_engine->protocol_ops &&
        g_mirror_engine->protocol_ops->send_video)
        return g_mirror_engine->protocol_ops->send_video(data, size);

    return size;
}

/**
 * 发送音频数据帧
 */
int screenmirror_send_audio_frame(const uint8_t *data, int size)
{
    if (g_mirror_engine == NULL)   return MIRROR_ERR_NOT_INIT;
    if (data == NULL || size <= 0) return MIRROR_ERR_INVALID_PARAM;

    if (g_mirror_engine->state != MIRROR_STATE_STREAMING &&
        g_mirror_engine->state != MIRROR_STATE_CONNECTED)
        return MIRROR_ERR_DEVICE_BUSY;

    if (g_mirror_engine->protocol_ops &&
        g_mirror_engine->protocol_ops->send_audio)
        return g_mirror_engine->protocol_ops->send_audio(data, size);

    return size;
}

/**
 * 投屏控制命令
 */
int screenmirror_control(const char *command)
{
    if (g_mirror_engine == NULL) return MIRROR_ERR_NOT_INIT;
    if (command == NULL)         return MIRROR_ERR_INVALID_PARAM;

    printf("[MIRROR] control: %s\n", command);

    if (g_mirror_engine->protocol_ops &&
        g_mirror_engine->protocol_ops->control)
        return g_mirror_engine->protocol_ops->control(command);

    return MIRROR_ERR_SUCCESS;
}

/**
 * 获取库版本
 */
const char* screenmirror_get_version(void)
{
    return "libscreenmirror v2.0.0";
}
