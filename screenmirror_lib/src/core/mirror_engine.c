#include "screenmirror_internal.h"
#include "network/discovery.h"
#include "protocols/miracast.h"
#include "protocols/airplay.h"
#include "protocols/dlna.h"
#include "protocols/wired.h"
#include "protocols/wireless.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* 全局引擎实例 */
MirrorEngine *g_mirror_engine = NULL;

static ProtocolOps* get_protocol_ops_by_mode(MirrorMode mode)
{
    switch (mode) {
        case MIRROR_MODE_MIRACAST:
            return miracast_get_ops();
        case MIRROR_MODE_AIRPLAY:
            return airplay_get_ops();
        case MIRROR_MODE_DLNA:
            return dlna_get_ops();
        case MIRROR_MODE_WIRED:
            return wired_get_ops();
        case MIRROR_MODE_WIRELESS:
            return wireless_get_ops();
        default:
            return NULL;
    }
}

static void emit_state_event(MirrorEventType type, void *data)
{
    if (g_mirror_engine == NULL || g_mirror_engine->event_sys == NULL) {
        return;
    }

    MirrorEvent event = {
        .type = type,
        .error_code = 0,
        .error_msg = NULL,
        .data = data,
    };
    event_system_emit(g_mirror_engine->event_sys, &event);
    if (g_mirror_engine->user_event_callback != NULL) {
        g_mirror_engine->user_event_callback(&event, g_mirror_engine->user_callback_data);
    }
}

static void engine_discovery_callback(const MirrorDeviceInfo *devices, int device_count, void *user_data)
{
    (void)user_data;

    if (g_mirror_engine == NULL) {
        return;
    }

    if (g_mirror_engine->discovery_callback != NULL) {
        g_mirror_engine->discovery_callback(devices, device_count, g_mirror_engine->discovery_callback_data);
    }

    emit_state_event(MIRROR_EVENT_DEVICE_FOUND, (void *)devices);
}

/**
 * 初始化投屏库
 */
int screenmirror_init(void)
{
    if (g_mirror_engine != NULL) {
        return MIRROR_ERR_ALREADY_INIT;
    }

    /* 创建引擎 */
    g_mirror_engine = (MirrorEngine *)malloc(sizeof(MirrorEngine));
    if (g_mirror_engine == NULL) {
        return MIRROR_ERR_OUT_OF_MEMORY;
    }

    memset(g_mirror_engine, 0, sizeof(MirrorEngine));

    /* 初始化互斥锁 */
    if (pthread_mutex_init(&g_mirror_engine->lock, NULL) != 0) {
        free(g_mirror_engine);
        g_mirror_engine = NULL;
        return MIRROR_ERR_UNKNOWN;
    }

    /* 创建事件系统 */
    g_mirror_engine->event_sys = event_system_create();
    if (g_mirror_engine->event_sys == NULL) {
        pthread_mutex_destroy(&g_mirror_engine->lock);
        free(g_mirror_engine);
        g_mirror_engine = NULL;
        return MIRROR_ERR_OUT_OF_MEMORY;
    }

    /* 创建状态机 */
    g_mirror_engine->state_machine = state_machine_create();
    if (g_mirror_engine->state_machine == NULL) {
        event_system_destroy(g_mirror_engine->event_sys);
        pthread_mutex_destroy(&g_mirror_engine->lock);
        free(g_mirror_engine);
        g_mirror_engine = NULL;
        return MIRROR_ERR_OUT_OF_MEMORY;
    }

    g_mirror_engine->state = MIRROR_STATE_IDLE;
    g_mirror_engine->initialized = true;
    g_mirror_engine->discovery_running = false;

    printf("[MIRROR] Library initialized successfully\n");

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

    (void)screenmirror_stop_discovery();
    (void)screenmirror_disconnect();

    /* 销毁资源 */
    if (g_mirror_engine->protocol_ops != NULL && g_mirror_engine->protocol_ops->exit != NULL) {
        g_mirror_engine->protocol_ops->exit();
    }

    pthread_mutex_lock(&g_mirror_engine->lock);
    state_machine_destroy(g_mirror_engine->state_machine);
    event_system_destroy(g_mirror_engine->event_sys);

    pthread_mutex_unlock(&g_mirror_engine->lock);
    pthread_mutex_destroy(&g_mirror_engine->lock);

    free(g_mirror_engine);
    g_mirror_engine = NULL;

    printf("[MIRROR] Library exited successfully\n");

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
    g_mirror_engine->user_callback_data = user_data;

    pthread_mutex_unlock(&g_mirror_engine->lock);

    return MIRROR_ERR_SUCCESS;
}

/**
 * 开始设备发现
 */
int screenmirror_start_discovery(MirrorMode mode, int timeout_ms,
                                MirrorDeviceListCallback callback,
                                void *user_data)
{
    if (g_mirror_engine == NULL) {
        return MIRROR_ERR_NOT_INIT;
    }

    if (callback == NULL || timeout_ms <= 0) {
        return MIRROR_ERR_INVALID_PARAM;
    }

    pthread_mutex_lock(&g_mirror_engine->lock);

    if (g_mirror_engine->discovery_running) {
        pthread_mutex_unlock(&g_mirror_engine->lock);
        return MIRROR_ERR_DEVICE_BUSY;
    }

    ProtocolOps *ops = get_protocol_ops_by_mode(mode);
    if (ops == NULL) {
        pthread_mutex_unlock(&g_mirror_engine->lock);
        return MIRROR_ERR_INVALID_PARAM;
    }

    g_mirror_engine->current_mode = mode;
    g_mirror_engine->protocol_ops = ops;
    g_mirror_engine->discovery_running = true;
    g_mirror_engine->state = MIRROR_STATE_DISCOVERING;
    g_mirror_engine->discovery_callback = callback;
    g_mirror_engine->discovery_callback_data = user_data;

    /* 更新状态机 */
    state_machine_transition(g_mirror_engine->state_machine, MIRROR_STATE_DISCOVERING);

    pthread_mutex_unlock(&g_mirror_engine->lock);

    if (ops->init != NULL && ops->init() != MIRROR_ERR_SUCCESS) {
        pthread_mutex_lock(&g_mirror_engine->lock);
        g_mirror_engine->discovery_running = false;
        g_mirror_engine->state = MIRROR_STATE_IDLE;
        pthread_mutex_unlock(&g_mirror_engine->lock);
        return MIRROR_ERR_UNKNOWN;
    }
    if (ops->start_discovery != NULL && ops->start_discovery(timeout_ms) != MIRROR_ERR_SUCCESS) {
        pthread_mutex_lock(&g_mirror_engine->lock);
        g_mirror_engine->discovery_running = false;
        g_mirror_engine->state = MIRROR_STATE_IDLE;
        pthread_mutex_unlock(&g_mirror_engine->lock);
        return MIRROR_ERR_UNKNOWN;
    }
    if (discovery_start(mode, timeout_ms, engine_discovery_callback, NULL) != MIRROR_ERR_SUCCESS) {
        pthread_mutex_lock(&g_mirror_engine->lock);
        g_mirror_engine->discovery_running = false;
        g_mirror_engine->state = MIRROR_STATE_IDLE;
        pthread_mutex_unlock(&g_mirror_engine->lock);
        return MIRROR_ERR_UNKNOWN;
    }

    emit_state_event(MIRROR_EVENT_STATE_CHANGED, NULL);

    return MIRROR_ERR_SUCCESS;
}

/**
 * 停止设备发现
 */
int screenmirror_stop_discovery(void)
{
    if (g_mirror_engine == NULL) {
        return MIRROR_ERR_NOT_INIT;
    }

    pthread_mutex_lock(&g_mirror_engine->lock);

    if (!g_mirror_engine->discovery_running) {
        pthread_mutex_unlock(&g_mirror_engine->lock);
        return MIRROR_ERR_SUCCESS;
    }

    ProtocolOps *ops = g_mirror_engine->protocol_ops;
    g_mirror_engine->discovery_running = false;
    if (g_mirror_engine->state != MIRROR_STATE_IDLE) {
        g_mirror_engine->state = MIRROR_STATE_IDLE;
        (void)state_machine_transition(g_mirror_engine->state_machine, MIRROR_STATE_IDLE);
    }

    pthread_mutex_unlock(&g_mirror_engine->lock);

    discovery_stop();
    if (ops != NULL && ops->stop_discovery != NULL) {
        ops->stop_discovery();
    }

    emit_state_event(MIRROR_EVENT_DISCOVERY_FINISHED, NULL);

    return MIRROR_ERR_SUCCESS;
}

/**
 * 连接指定设备
 */
int screenmirror_connect(const MirrorDeviceInfo *device,
                        const MirrorConfig *config)
{
    if (g_mirror_engine == NULL) {
        return MIRROR_ERR_NOT_INIT;
    }

    if (device == NULL || config == NULL) {
        return MIRROR_ERR_INVALID_PARAM;
    }

    pthread_mutex_lock(&g_mirror_engine->lock);

    if (g_mirror_engine->state != MIRROR_STATE_IDLE &&
        g_mirror_engine->state != MIRROR_STATE_DISCOVERING) {
        pthread_mutex_unlock(&g_mirror_engine->lock);
        return MIRROR_ERR_DEVICE_BUSY;
    }

    ProtocolOps *ops = get_protocol_ops_by_mode(config->mode);
    if (ops == NULL) {
        pthread_mutex_unlock(&g_mirror_engine->lock);
        return MIRROR_ERR_INVALID_PARAM;
    }

    /* 保存设备和配置信息 */
    memcpy(&g_mirror_engine->current_device, device, sizeof(MirrorDeviceInfo));
    memcpy(&g_mirror_engine->config, config, sizeof(MirrorConfig));
    g_mirror_engine->current_mode = config->mode;
    g_mirror_engine->protocol_ops = ops;

    /* 更新状态 */
    g_mirror_engine->state = MIRROR_STATE_CONNECTING;
    state_machine_transition(g_mirror_engine->state_machine, MIRROR_STATE_CONNECTING);

    pthread_mutex_unlock(&g_mirror_engine->lock);

    if (ops->init != NULL && ops->init() != MIRROR_ERR_SUCCESS) {
        return MIRROR_ERR_UNKNOWN;
    }
    if (ops->connect != NULL && ops->connect(device, config) != MIRROR_ERR_SUCCESS) {
        pthread_mutex_lock(&g_mirror_engine->lock);
        g_mirror_engine->state = MIRROR_STATE_ERROR;
        pthread_mutex_unlock(&g_mirror_engine->lock);
        return MIRROR_ERR_CONNECT_FAILED;
    }

    pthread_mutex_lock(&g_mirror_engine->lock);
    g_mirror_engine->state = MIRROR_STATE_CONNECTED;
    state_machine_transition(g_mirror_engine->state_machine, MIRROR_STATE_CONNECTED);
    pthread_mutex_unlock(&g_mirror_engine->lock);

    emit_state_event(MIRROR_EVENT_CONNECTED, (void *)device);

    return MIRROR_ERR_SUCCESS;
}

/**
 * 断开连接
 */
int screenmirror_disconnect(void)
{
    if (g_mirror_engine == NULL) {
        return MIRROR_ERR_NOT_INIT;
    }

    pthread_mutex_lock(&g_mirror_engine->lock);

    if (g_mirror_engine->state == MIRROR_STATE_IDLE) {
        pthread_mutex_unlock(&g_mirror_engine->lock);
        return MIRROR_ERR_SUCCESS;
    }

    ProtocolOps *ops = g_mirror_engine->protocol_ops;
    g_mirror_engine->state = MIRROR_STATE_IDLE;
    state_machine_transition(g_mirror_engine->state_machine, MIRROR_STATE_IDLE);

    pthread_mutex_unlock(&g_mirror_engine->lock);

    if (ops != NULL && ops->disconnect != NULL) {
        ops->disconnect();
    }

    emit_state_event(MIRROR_EVENT_DISCONNECTED, NULL);

    return MIRROR_ERR_SUCCESS;
}

/**
 * 获取当前状态
 */
MirrorState screenmirror_get_state(void)
{
    if (g_mirror_engine == NULL) {
        return MIRROR_STATE_IDLE;
    }

    pthread_mutex_lock(&g_mirror_engine->lock);
    MirrorState state = g_mirror_engine->state;
    pthread_mutex_unlock(&g_mirror_engine->lock);

    return state;
}

/**
 * 获取当前连接的设备信息
 */
const MirrorDeviceInfo* screenmirror_get_device_info(void)
{
    if (g_mirror_engine == NULL) {
        return NULL;
    }

    if (g_mirror_engine->state == MIRROR_STATE_IDLE) {
        return NULL;
    }

    return &g_mirror_engine->current_device;
}

/**
 * 获取当前投屏配置
 */
const MirrorConfig* screenmirror_get_config(void)
{
    if (g_mirror_engine == NULL) {
        return NULL;
    }

    if (g_mirror_engine->state == MIRROR_STATE_IDLE) {
        return NULL;
    }

    return &g_mirror_engine->config;
}

/**
 * 发送视频数据帧
 */
int screenmirror_send_video_frame(const uint8_t *data, int size)
{
    if (g_mirror_engine == NULL) {
        return MIRROR_ERR_NOT_INIT;
    }

    if (data == NULL || size <= 0) {
        return MIRROR_ERR_INVALID_PARAM;
    }

    if (g_mirror_engine->state != MIRROR_STATE_STREAMING &&
        g_mirror_engine->state != MIRROR_STATE_CONNECTED) {
        return MIRROR_ERR_DEVICE_BUSY;
    }

    if (g_mirror_engine->protocol_ops == NULL || g_mirror_engine->protocol_ops->send_video == NULL) {
        return MIRROR_ERR_NOT_INIT;
    }

    return g_mirror_engine->protocol_ops->send_video(data, size);
}

/**
 * 发送音频数据帧
 */
int screenmirror_send_audio_frame(const uint8_t *data, int size)
{
    if (g_mirror_engine == NULL) {
        return MIRROR_ERR_NOT_INIT;
    }

    if (data == NULL || size <= 0) {
        return MIRROR_ERR_INVALID_PARAM;
    }

    if (g_mirror_engine->state != MIRROR_STATE_STREAMING &&
        g_mirror_engine->state != MIRROR_STATE_CONNECTED) {
        return MIRROR_ERR_DEVICE_BUSY;
    }

    if (g_mirror_engine->protocol_ops == NULL || g_mirror_engine->protocol_ops->send_audio == NULL) {
        return MIRROR_ERR_NOT_INIT;
    }

    return g_mirror_engine->protocol_ops->send_audio(data, size);
}

/**
 * 投屏控制命令
 */
int screenmirror_control(const char *command)
{
    if (g_mirror_engine == NULL) {
        return MIRROR_ERR_NOT_INIT;
    }

    if (command == NULL) {
        return MIRROR_ERR_INVALID_PARAM;
    }

    if (g_mirror_engine->protocol_ops == NULL || g_mirror_engine->protocol_ops->control == NULL) {
        return MIRROR_ERR_NOT_INIT;
    }

    return g_mirror_engine->protocol_ops->control(command);
}

/**
 * 获取库版本
 */
const char* screenmirror_get_version(void)
{
    return "libscreenmirror v1.0.0";
}
