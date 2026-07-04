#include "screenmirror_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* 全局引擎实例 */
MirrorEngine *g_mirror_engine = NULL;

static void fill_mock_device(MirrorMode mode, MirrorDeviceInfo *device)
{
    static const char *names[] = {
        "Miracast Demo Device",
        "AirPlay Demo Device",
        "DLNA Demo Device",
        "USB Demo Device",
        "Wireless Demo Device",
    };

    if (device == NULL) {
        return;
    }

    memset(device, 0, sizeof(*device));
    strncpy(device->name, names[mode], sizeof(device->name) - 1);
    strncpy(device->mac_address, "00:11:22:33:44:55",
            sizeof(device->mac_address) - 1);
    strncpy(device->ip_address, "192.168.1.100",
            sizeof(device->ip_address) - 1);
    device->signal_strength = 80;
    device->mode = mode;
    device->model_id = 1;
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
    bool discovery_running = false;
    bool has_active_session = false;

    if (g_mirror_engine == NULL) {
        return MIRROR_ERR_NOT_INIT;
    }

    pthread_mutex_lock(&g_mirror_engine->lock);
    discovery_running = g_mirror_engine->discovery_running;
    has_active_session = (g_mirror_engine->state != MIRROR_STATE_IDLE);
    pthread_mutex_unlock(&g_mirror_engine->lock);

    if (discovery_running) {
        screenmirror_stop_discovery();
    }

    if (has_active_session) {
        screenmirror_disconnect();
    }

    pthread_mutex_lock(&g_mirror_engine->lock);

    /* 销毁资源 */
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
    MirrorEvent event;
    MirrorDeviceInfo device;

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

    g_mirror_engine->current_mode = mode;
    g_mirror_engine->discovery_running = true;
    g_mirror_engine->state = MIRROR_STATE_DISCOVERING;

    /* 更新状态机 */
    state_machine_transition(g_mirror_engine->state_machine, MIRROR_STATE_DISCOVERING);

    pthread_mutex_unlock(&g_mirror_engine->lock);

    printf("[MIRROR] Discovery started for mode: %d, timeout: %d ms\n", mode, timeout_ms);

    /* 发送状态变化事件 */
    event = (MirrorEvent) {
        .type = MIRROR_EVENT_STATE_CHANGED,
        .error_code = 0,
        .error_msg = NULL,
        .data = NULL,
    };
    event_system_emit(g_mirror_engine->event_sys, &event);

    if (g_mirror_engine->user_event_callback) {
        g_mirror_engine->user_event_callback(&event, g_mirror_engine->user_callback_data);
    }

    fill_mock_device(mode, &device);
    callback(&device, 1, user_data);

    event = (MirrorEvent) {
        .type = MIRROR_EVENT_DEVICE_FOUND,
        .error_code = 0,
        .error_msg = NULL,
        .data = &device,
    };
    event_system_emit(g_mirror_engine->event_sys, &event);

    if (g_mirror_engine->user_event_callback) {
        g_mirror_engine->user_event_callback(&event, g_mirror_engine->user_callback_data);
    }

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

    g_mirror_engine->discovery_running = false;
    g_mirror_engine->state = MIRROR_STATE_IDLE;
    if (state_machine_get_state(g_mirror_engine->state_machine) != MIRROR_STATE_IDLE) {
        state_machine_transition(g_mirror_engine->state_machine, MIRROR_STATE_IDLE);
    }

    pthread_mutex_unlock(&g_mirror_engine->lock);

    printf("[MIRROR] Discovery stopped\n");

    /* 发送发现完成事件 */
    MirrorEvent event = {
        .type = MIRROR_EVENT_DISCOVERY_FINISHED,
        .error_code = 0,
        .error_msg = NULL,
        .data = NULL,
    };
    event_system_emit(g_mirror_engine->event_sys, &event);

    if (g_mirror_engine->user_event_callback) {
        g_mirror_engine->user_event_callback(&event, g_mirror_engine->user_callback_data);
    }

    return MIRROR_ERR_SUCCESS;
}

/**
 * 连接指定设备
 */
int screenmirror_connect(const MirrorDeviceInfo *device,
                        const MirrorConfig *config)
{
    MirrorEvent event;

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

    /* 保存设备和配置信息 */
    memcpy(&g_mirror_engine->current_device, device, sizeof(MirrorDeviceInfo));
    memcpy(&g_mirror_engine->config, config, sizeof(MirrorConfig));
    g_mirror_engine->current_mode = config->mode;
    g_mirror_engine->discovery_running = false;

    /* 更新状态 */
    g_mirror_engine->state = MIRROR_STATE_CONNECTING;
    state_machine_transition(g_mirror_engine->state_machine, MIRROR_STATE_CONNECTING);

    pthread_mutex_unlock(&g_mirror_engine->lock);

    printf("[MIRROR] Connecting to device: %s (mode: %d)\n",
           device->name, config->mode);

    /* 发送连接开始事件 */
    event = (MirrorEvent) {
        .type = MIRROR_EVENT_STATE_CHANGED,
        .error_code = 0,
        .error_msg = NULL,
        .data = (void *)device,
    };
    event_system_emit(g_mirror_engine->event_sys, &event);

    if (g_mirror_engine->user_event_callback) {
        g_mirror_engine->user_event_callback(&event, g_mirror_engine->user_callback_data);
    }

    pthread_mutex_lock(&g_mirror_engine->lock);
    g_mirror_engine->state = MIRROR_STATE_CONNECTED;
    state_machine_transition(g_mirror_engine->state_machine, MIRROR_STATE_CONNECTED);
    pthread_mutex_unlock(&g_mirror_engine->lock);

    event = (MirrorEvent) {
        .type = MIRROR_EVENT_CONNECTED,
        .error_code = 0,
        .error_msg = NULL,
        .data = (void *)device,
    };
    event_system_emit(g_mirror_engine->event_sys, &event);

    if (g_mirror_engine->user_event_callback) {
        g_mirror_engine->user_event_callback(&event, g_mirror_engine->user_callback_data);
    }

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

    g_mirror_engine->discovery_running = false;
    g_mirror_engine->state = MIRROR_STATE_IDLE;
    state_machine_transition(g_mirror_engine->state_machine, MIRROR_STATE_IDLE);

    pthread_mutex_unlock(&g_mirror_engine->lock);

    printf("[MIRROR] Disconnected from device\n");

    /* 发送断开连接事件 */
    MirrorEvent event = {
        .type = MIRROR_EVENT_DISCONNECTED,
        .error_code = 0,
        .error_msg = NULL,
        .data = NULL,
    };
    event_system_emit(g_mirror_engine->event_sys, &event);

    if (g_mirror_engine->user_event_callback) {
        g_mirror_engine->user_event_callback(&event, g_mirror_engine->user_callback_data);
    }

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

    /* TODO: 调用协议层发送 */
    return size;
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

    /* TODO: 调用协议层发送 */
    return size;
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

    printf("[MIRROR] Control command: %s\n", command);

    /* TODO: 处理控制命令 */

    return MIRROR_ERR_SUCCESS;
}

/**
 * 获取库版本
 */
const char* screenmirror_get_version(void)
{
    return "libscreenmirror v1.0.0";
}
