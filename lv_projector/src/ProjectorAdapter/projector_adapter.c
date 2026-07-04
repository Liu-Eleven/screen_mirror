#include "projector_adapter_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static ProjectorAdapterContext g_projector_adapter = {
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .initialized = false,
    .state = PROJECTOR_ADAPTER_STATE_IDLE,
    .protocol = PROJECTOR_PROTOCOL_MIRACAST,
    .config = {
        .protocol = PROJECTOR_PROTOCOL_MIRACAST,
        .max_bitrate = 10000,
        .resolution_width = 1920,
        .resolution_height = 1080,
        .refresh_rate = 60,
        .connect_timeout_ms = 5000,
        .enable_audio = true,
        .enable_hdcp = false,
        .device_name = NULL,
        .extra_config = NULL,
    },
    .device_name = "LV Projector",
};

static void projector_adapter_copy_string(char *dst, size_t dst_size,
                                          const char *src)
{
    if (dst == NULL || dst_size == 0) {
        return;
    }

    if (src == NULL) {
        dst[0] = '\0';
        return;
    }

    snprintf(dst, dst_size, "%s", src);
}

static void projector_adapter_copy_config_unlocked(ProjectorAdapterContext *ctx,
                                                   const ProjectorAdapterConfig *config)
{
    if (ctx == NULL) {
        return;
    }

    if (config != NULL) {
        ctx->config = *config;
        ctx->protocol = config->protocol;
        projector_adapter_copy_string(
            ctx->device_name, sizeof(ctx->device_name),
            (config->device_name != NULL && config->device_name[0] != '\0')
                ? config->device_name
                : "LV Projector");
    } else {
        ctx->protocol = PROJECTOR_PROTOCOL_MIRACAST;
        projector_adapter_copy_string(ctx->device_name, sizeof(ctx->device_name),
                                      "LV Projector");
    }

    ctx->config.protocol = ctx->protocol;
    ctx->config.device_name = ctx->device_name;
}

static const char *projector_adapter_state_message(ProjectorAdapterState state)
{
    switch (state) {
        case PROJECTOR_ADAPTER_STATE_IDLE:
            return "投屏服务已停止";
        case PROJECTOR_ADAPTER_STATE_DISCOVERING:
            return "正在发现投屏设备";
        case PROJECTOR_ADAPTER_STATE_CONNECTING:
            return "正在连接投屏设备";
        case PROJECTOR_ADAPTER_STATE_CONNECTED:
            return "投屏连接已建立";
        case PROJECTOR_ADAPTER_STATE_STREAMING:
            return "投屏传输中";
        case PROJECTOR_ADAPTER_STATE_PAUSED:
            return "投屏已暂停";
        case PROJECTOR_ADAPTER_STATE_ERROR:
        default:
            return "投屏服务异常";
    }
}

ProjectorAdapterContext *projector_adapter_get_context(void)
{
    return &g_projector_adapter;
}

MirrorMode projector_adapter_to_mirror_mode(ProjectorAdapterProtocol protocol)
{
    switch (protocol) {
        case PROJECTOR_PROTOCOL_AIRPLAY:
            return MIRROR_MODE_AIRPLAY;
        case PROJECTOR_PROTOCOL_DLNA:
            return MIRROR_MODE_DLNA;
        case PROJECTOR_PROTOCOL_WIRED:
            return MIRROR_MODE_WIRED;
        case PROJECTOR_PROTOCOL_WIRELESS:
            return MIRROR_MODE_WIRELESS;
        case PROJECTOR_PROTOCOL_MIRACAST:
        default:
            return MIRROR_MODE_MIRACAST;
    }
}

ProjectorAdapterProtocol projector_adapter_from_mirror_mode(MirrorMode mode)
{
    switch (mode) {
        case MIRROR_MODE_AIRPLAY:
            return PROJECTOR_PROTOCOL_AIRPLAY;
        case MIRROR_MODE_DLNA:
            return PROJECTOR_PROTOCOL_DLNA;
        case MIRROR_MODE_WIRED:
            return PROJECTOR_PROTOCOL_WIRED;
        case MIRROR_MODE_WIRELESS:
            return PROJECTOR_PROTOCOL_WIRELESS;
        case MIRROR_MODE_MIRACAST:
        default:
            return PROJECTOR_PROTOCOL_MIRACAST;
    }
}

ProjectorAdapterState projector_adapter_from_mirror_state(MirrorState state)
{
    switch (state) {
        case MIRROR_STATE_DISCOVERING:
            return PROJECTOR_ADAPTER_STATE_DISCOVERING;
        case MIRROR_STATE_CONNECTING:
            return PROJECTOR_ADAPTER_STATE_CONNECTING;
        case MIRROR_STATE_CONNECTED:
            return PROJECTOR_ADAPTER_STATE_CONNECTED;
        case MIRROR_STATE_STREAMING:
            return PROJECTOR_ADAPTER_STATE_STREAMING;
        case MIRROR_STATE_PAUSED:
            return PROJECTOR_ADAPTER_STATE_PAUSED;
        case MIRROR_STATE_ERROR:
            return PROJECTOR_ADAPTER_STATE_ERROR;
        case MIRROR_STATE_IDLE:
        default:
            return PROJECTOR_ADAPTER_STATE_IDLE;
    }
}

int projector_adapter_build_mirror_config(MirrorConfig *config,
                                          const ProjectorAdapterConfig *adapter_config)
{
    if (config == NULL || adapter_config == NULL) {
        return PROJECTOR_ADAPTER_ERR_INVALID_PARAM;
    }

    memset(config, 0, sizeof(*config));
    config->mode = projector_adapter_to_mirror_mode(adapter_config->protocol);
    config->max_bitrate = adapter_config->max_bitrate;
    config->resolution_width = adapter_config->resolution_width;
    config->resolution_height = adapter_config->resolution_height;
    config->refresh_rate = adapter_config->refresh_rate;
    config->connect_timeout_ms = adapter_config->connect_timeout_ms;
    config->enable_audio = adapter_config->enable_audio;
    config->enable_hdcp = adapter_config->enable_hdcp;
    config->extra_config = adapter_config->extra_config;
    return 0;
}

int projector_adapter_build_mirror_device(MirrorDeviceInfo *device,
                                          const ProjectorAdapterDevice *adapter_device)
{
    if (device == NULL || adapter_device == NULL) {
        return PROJECTOR_ADAPTER_ERR_INVALID_PARAM;
    }

    memset(device, 0, sizeof(*device));
    projector_adapter_copy_string(device->name, sizeof(device->name),
                                  adapter_device->name);
    projector_adapter_copy_string(device->mac_address,
                                  sizeof(device->mac_address),
                                  adapter_device->mac_address);
    projector_adapter_copy_string(device->ip_address, sizeof(device->ip_address),
                                  adapter_device->ip_address);
    device->signal_strength = adapter_device->signal_strength;
    device->mode = projector_adapter_to_mirror_mode(adapter_device->protocol);
    device->model_id = adapter_device->model_id;
    device->platform_data = adapter_device->platform_data;
    return 0;
}

void projector_adapter_store_current_device(const MirrorDeviceInfo *device)
{
    ProjectorAdapterContext *ctx = projector_adapter_get_context();

    if (ctx == NULL || device == NULL) {
        return;
    }

    pthread_mutex_lock(&ctx->lock);
    memset(&ctx->current_device, 0, sizeof(ctx->current_device));
    projector_adapter_copy_string(ctx->current_device.name,
                                  sizeof(ctx->current_device.name),
                                  device->name);
    projector_adapter_copy_string(ctx->current_device.mac_address,
                                  sizeof(ctx->current_device.mac_address),
                                  device->mac_address);
    projector_adapter_copy_string(ctx->current_device.ip_address,
                                  sizeof(ctx->current_device.ip_address),
                                  device->ip_address);
    ctx->current_device.signal_strength = device->signal_strength;
    ctx->current_device.protocol = projector_adapter_from_mirror_mode(device->mode);
    ctx->current_device.model_id = device->model_id;
    ctx->current_device.platform_data = device->platform_data;
    ctx->has_current_device = true;
    pthread_mutex_unlock(&ctx->lock);
}

void projector_adapter_notify_state(ProjectorAdapterState state,
                                    const char *message)
{
    ProjectorAdapterContext *ctx = projector_adapter_get_context();
    ProjectorAdapterStateCallback callback = NULL;
    void *user_data = NULL;

    pthread_mutex_lock(&ctx->lock);
    ctx->state = state;
    callback = ctx->state_callback;
    user_data = ctx->callback_user_data;
    pthread_mutex_unlock(&ctx->lock);

    if (callback != NULL) {
        callback(state,
                 message != NULL ? message : projector_adapter_state_message(state),
                 user_data);
    }
}

void projector_adapter_notify_devices(const MirrorDeviceInfo *devices,
                                      int device_count)
{
    ProjectorAdapterContext *ctx = projector_adapter_get_context();
    ProjectorAdapterDeviceCallback callback = NULL;
    void *user_data = NULL;
    ProjectorAdapterDevice *adapter_devices = NULL;
    int i;

    if (devices == NULL || device_count <= 0) {
        return;
    }

    pthread_mutex_lock(&ctx->lock);
    callback = ctx->device_callback;
    user_data = ctx->callback_user_data;
    pthread_mutex_unlock(&ctx->lock);

    if (callback == NULL) {
        return;
    }

    adapter_devices = calloc((size_t)device_count, sizeof(*adapter_devices));
    if (adapter_devices == NULL) {
        char alloc_error[64];

        snprintf(alloc_error, sizeof(alloc_error),
                 "适配层内存不足，无法同步 %d 个设备", device_count);
        projector_adapter_notify_state(PROJECTOR_ADAPTER_STATE_ERROR, alloc_error);
        return;
    }

    for (i = 0; i < device_count; i++) {
        projector_adapter_copy_string(adapter_devices[i].name,
                                      sizeof(adapter_devices[i].name),
                                      devices[i].name);
        projector_adapter_copy_string(adapter_devices[i].mac_address,
                                      sizeof(adapter_devices[i].mac_address),
                                      devices[i].mac_address);
        projector_adapter_copy_string(adapter_devices[i].ip_address,
                                      sizeof(adapter_devices[i].ip_address),
                                      devices[i].ip_address);
        adapter_devices[i].signal_strength = devices[i].signal_strength;
        adapter_devices[i].protocol = projector_adapter_from_mirror_mode(devices[i].mode);
        adapter_devices[i].model_id = devices[i].model_id;
        adapter_devices[i].platform_data = devices[i].platform_data;
    }

    callback(adapter_devices, device_count, user_data);
    free(adapter_devices);
}

int projector_adapter_init(const ProjectorAdapterConfig *config)
{
    ProjectorAdapterContext *ctx = projector_adapter_get_context();
    int ret;

    pthread_mutex_lock(&ctx->lock);
    projector_adapter_copy_config_unlocked(ctx, config);
    if (ctx->initialized) {
        pthread_mutex_unlock(&ctx->lock);
        return 0;
    }
    pthread_mutex_unlock(&ctx->lock);

    ret = screenmirror_init();
    if (ret < 0 && ret != SCREENMIRROR_ADAPTER_ERR_ALREADY_INIT) {
        return ret;
    }

    ret = screenmirror_set_event_callback(projector_adapter_handle_mirror_event, ctx);
    if (ret < 0) {
        return ret;
    }

    pthread_mutex_lock(&ctx->lock);
    ctx->state = PROJECTOR_ADAPTER_STATE_IDLE;
    ctx->initialized = true;
    ctx->has_current_device = false;
    pthread_mutex_unlock(&ctx->lock);

    return 0;
}

int projector_adapter_deinit(void)
{
    ProjectorAdapterContext *ctx = projector_adapter_get_context();
    int ret;

    pthread_mutex_lock(&ctx->lock);
    if (!ctx->initialized) {
        pthread_mutex_unlock(&ctx->lock);
        return 0;
    }
    pthread_mutex_unlock(&ctx->lock);

    projector_adapter_stop_services();

    ret = screenmirror_exit();
    if (ret < 0 && ret != SCREENMIRROR_ADAPTER_ERR_NOT_INIT) {
        return ret;
    }

    pthread_mutex_lock(&ctx->lock);
    ctx->initialized = false;
    ctx->has_current_device = false;
    ctx->state = PROJECTOR_ADAPTER_STATE_IDLE;
    pthread_mutex_unlock(&ctx->lock);
    return 0;
}

int projector_adapter_set_callbacks(ProjectorAdapterStateCallback state_callback,
                                    ProjectorAdapterDeviceCallback device_callback,
                                    void *user_data)
{
    ProjectorAdapterContext *ctx = projector_adapter_get_context();

    pthread_mutex_lock(&ctx->lock);
    ctx->state_callback = state_callback;
    ctx->device_callback = device_callback;
    ctx->callback_user_data = user_data;
    pthread_mutex_unlock(&ctx->lock);

    return 0;
}

int projector_adapter_update_config(const ProjectorAdapterConfig *config)
{
    ProjectorAdapterContext *ctx = projector_adapter_get_context();

    pthread_mutex_lock(&ctx->lock);
    projector_adapter_copy_config_unlocked(ctx, config);
    pthread_mutex_unlock(&ctx->lock);

    return 0;
}

int projector_adapter_start_services(ProjectorAdapterProtocol protocol)
{
    ProjectorAdapterContext *ctx = projector_adapter_get_context();
    int timeout_ms;

    pthread_mutex_lock(&ctx->lock);
    if (!ctx->initialized) {
        pthread_mutex_unlock(&ctx->lock);
        return PROJECTOR_ADAPTER_ERR_NOT_READY;
    }
    ctx->protocol = protocol;
    ctx->config.protocol = protocol;
    timeout_ms = ctx->config.connect_timeout_ms > 0 ? ctx->config.connect_timeout_ms : 5000;
    pthread_mutex_unlock(&ctx->lock);

    return screenmirror_start_discovery(projector_adapter_to_mirror_mode(protocol),
                                        timeout_ms,
                                        projector_adapter_handle_discovery_result,
                                        ctx);
}

int projector_adapter_stop_services(void)
{
    int ret = 0;

    ret = screenmirror_stop_discovery();
    if (ret < 0 && ret != SCREENMIRROR_ADAPTER_ERR_NOT_INIT) {
        return ret;
    }

    ret = screenmirror_disconnect();
    if (ret < 0 && ret != SCREENMIRROR_ADAPTER_ERR_NOT_INIT) {
        return ret;
    }

    return 0;
}

int projector_adapter_on_protocol_selected(ProjectorAdapterProtocol protocol)
{
    int ret = projector_adapter_stop_services();
    if (ret < 0) {
        return ret;
    }

    return projector_adapter_start_services(protocol);
}

int projector_adapter_on_device_selected(const ProjectorAdapterDevice *device)
{
    ProjectorAdapterContext *ctx = projector_adapter_get_context();
    MirrorDeviceInfo mirror_device;
    MirrorConfig mirror_config;

    if (device == NULL) {
        return PROJECTOR_ADAPTER_ERR_INVALID_PARAM;
    }

    pthread_mutex_lock(&ctx->lock);
    ctx->config.protocol = device->protocol;
    pthread_mutex_unlock(&ctx->lock);

    if (projector_adapter_build_mirror_device(&mirror_device, device) < 0) {
        return PROJECTOR_ADAPTER_ERR_INVALID_PARAM;
    }
    if (projector_adapter_build_mirror_config(&mirror_config, &ctx->config) < 0) {
        return PROJECTOR_ADAPTER_ERR_INVALID_PARAM;
    }

    projector_adapter_store_current_device(&mirror_device);
    return screenmirror_connect(&mirror_device, &mirror_config);
}

int projector_adapter_disconnect(void)
{
    return screenmirror_disconnect();
}

ProjectorAdapterState projector_adapter_get_state(void)
{
    ProjectorAdapterContext *ctx = projector_adapter_get_context();
    ProjectorAdapterState state;

    pthread_mutex_lock(&ctx->lock);
    state = ctx->state;
    pthread_mutex_unlock(&ctx->lock);

    return state;
}

ProjectorAdapterProtocol projector_adapter_get_protocol(void)
{
    ProjectorAdapterContext *ctx = projector_adapter_get_context();
    ProjectorAdapterProtocol protocol;

    pthread_mutex_lock(&ctx->lock);
    protocol = ctx->protocol;
    pthread_mutex_unlock(&ctx->lock);

    return protocol;
}

int projector_adapter_get_current_device(ProjectorAdapterDevice *device)
{
    ProjectorAdapterContext *ctx = projector_adapter_get_context();

    if (device == NULL) {
        return PROJECTOR_ADAPTER_ERR_INVALID_PARAM;
    }

    pthread_mutex_lock(&ctx->lock);
    if (!ctx->has_current_device) {
        pthread_mutex_unlock(&ctx->lock);
        return PROJECTOR_ADAPTER_ERR_NO_DEVICE;
    }
    *device = ctx->current_device;
    pthread_mutex_unlock(&ctx->lock);
    return 0;
}

const char *projector_adapter_get_device_name(void)
{
    ProjectorAdapterContext *ctx = projector_adapter_get_context();
    const char *device_name;

    pthread_mutex_lock(&ctx->lock);
    device_name = ctx->device_name;
    pthread_mutex_unlock(&ctx->lock);

    return device_name;
}
