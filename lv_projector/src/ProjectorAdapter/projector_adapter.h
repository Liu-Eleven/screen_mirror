#ifndef PROJECTOR_ADAPTER_H
#define PROJECTOR_ADAPTER_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PROJECTOR_PROTOCOL_MIRACAST,
    PROJECTOR_PROTOCOL_AIRPLAY,
    PROJECTOR_PROTOCOL_DLNA,
    PROJECTOR_PROTOCOL_WIRED,
    PROJECTOR_PROTOCOL_WIRELESS,
} ProjectorAdapterProtocol;

typedef enum {
    PROJECTOR_ADAPTER_STATE_IDLE,
    PROJECTOR_ADAPTER_STATE_DISCOVERING,
    PROJECTOR_ADAPTER_STATE_CONNECTING,
    PROJECTOR_ADAPTER_STATE_CONNECTED,
    PROJECTOR_ADAPTER_STATE_STREAMING,
    PROJECTOR_ADAPTER_STATE_PAUSED,
    PROJECTOR_ADAPTER_STATE_ERROR,
} ProjectorAdapterState;

typedef struct {
    char name[128];
    char mac_address[32];
    char ip_address[16];
    int signal_strength;
    ProjectorAdapterProtocol protocol;
    uint32_t model_id;
    void *platform_data;
} ProjectorAdapterDevice;

typedef struct {
    ProjectorAdapterProtocol protocol;
    int max_bitrate;
    int resolution_width;
    int resolution_height;
    int refresh_rate;
    int connect_timeout_ms;
    bool enable_audio;
    bool enable_hdcp;
    const char *device_name;
    void *extra_config;
} ProjectorAdapterConfig;

typedef void (*ProjectorAdapterStateCallback)(ProjectorAdapterState state,
                                              const char *message,
                                              void *user_data);
typedef void (*ProjectorAdapterDeviceCallback)(const ProjectorAdapterDevice *devices,
                                               int device_count,
                                               void *user_data);

int projector_adapter_init(const ProjectorAdapterConfig *config);
int projector_adapter_deinit(void);
int projector_adapter_set_callbacks(ProjectorAdapterStateCallback state_callback,
                                    ProjectorAdapterDeviceCallback device_callback,
                                    void *user_data);
int projector_adapter_update_config(const ProjectorAdapterConfig *config);
int projector_adapter_start_services(ProjectorAdapterProtocol protocol);
int projector_adapter_stop_services(void);
int projector_adapter_on_protocol_selected(ProjectorAdapterProtocol protocol);
int projector_adapter_on_device_selected(const ProjectorAdapterDevice *device);
int projector_adapter_disconnect(void);

ProjectorAdapterState projector_adapter_get_state(void);
ProjectorAdapterProtocol projector_adapter_get_protocol(void);
int projector_adapter_get_current_device(ProjectorAdapterDevice *device);
const char *projector_adapter_get_device_name(void);

int projector_ui_on_screen_loaded(void);
int projector_ui_on_screen_unloaded(void);
int projector_ui_on_protocol_selected(ProjectorAdapterProtocol protocol);
int projector_ui_on_device_selected(const ProjectorAdapterDevice *device);
int projector_ui_on_disconnect_requested(void);

#ifdef __cplusplus
}
#endif

#endif
