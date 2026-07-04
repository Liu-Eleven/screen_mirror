#ifndef PROJECTOR_ADAPTER_INTERNAL_H
#define PROJECTOR_ADAPTER_INTERNAL_H

#include "projector_adapter.h"
#include "screenmirror.h"
#include <pthread.h>

typedef struct {
    pthread_mutex_t lock;
    bool initialized;
    ProjectorAdapterState state;
    ProjectorAdapterProtocol protocol;
    ProjectorAdapterConfig config;
    char device_name[128];
    bool has_current_device;
    ProjectorAdapterDevice current_device;
    ProjectorAdapterStateCallback state_callback;
    ProjectorAdapterDeviceCallback device_callback;
    void *callback_user_data;
} ProjectorAdapterContext;

ProjectorAdapterContext *projector_adapter_get_context(void);

MirrorMode projector_adapter_to_mirror_mode(ProjectorAdapterProtocol protocol);
ProjectorAdapterProtocol projector_adapter_from_mirror_mode(MirrorMode mode);
ProjectorAdapterState projector_adapter_from_mirror_state(MirrorState state);
int projector_adapter_build_mirror_config(MirrorConfig *config,
                                          const ProjectorAdapterConfig *adapter_config);
int projector_adapter_build_mirror_device(MirrorDeviceInfo *device,
                                          const ProjectorAdapterDevice *adapter_device);
void projector_adapter_store_current_device(const MirrorDeviceInfo *device);
void projector_adapter_notify_state(ProjectorAdapterState state,
                                    const char *message);
void projector_adapter_notify_devices(const MirrorDeviceInfo *devices,
                                      int device_count);
void projector_adapter_handle_mirror_event(const MirrorEvent *event, void *user_data);
void projector_adapter_handle_discovery_result(const MirrorDeviceInfo *devices,
                                               int device_count,
                                               void *user_data);

#endif
