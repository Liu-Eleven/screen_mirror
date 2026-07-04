#include "projector_adapter_internal.h"

static const char *projector_adapter_event_message(const MirrorEvent *event)
{
    if (event != NULL && event->error_msg != NULL) {
        return event->error_msg;
    }

    switch (event != NULL ? event->type : MIRROR_EVENT_ERROR) {
        case MIRROR_EVENT_CONNECTED:
            return "投屏连接已建立";
        case MIRROR_EVENT_DISCONNECTED:
            return "投屏连接已断开";
        case MIRROR_EVENT_DEVICE_FOUND:
            return "已发现投屏设备";
        case MIRROR_EVENT_DISCOVERY_FINISHED:
            return "设备发现已完成";
        case MIRROR_EVENT_STATE_CHANGED:
            return NULL;
        case MIRROR_EVENT_ERROR:
        default:
            return "投屏服务异常";
    }
}

void projector_adapter_handle_discovery_result(const MirrorDeviceInfo *devices,
                                               int device_count,
                                               void *user_data)
{
    (void)user_data;

    projector_adapter_notify_devices(devices, device_count);
    if (device_count > 0) {
        projector_adapter_notify_state(PROJECTOR_ADAPTER_STATE_DISCOVERING,
                                       "已同步发现设备列表");
    }
}

void projector_adapter_handle_mirror_event(const MirrorEvent *event, void *user_data)
{
    ProjectorAdapterContext *ctx = (ProjectorAdapterContext *)user_data;
    const MirrorDeviceInfo *device = NULL;
    ProjectorAdapterState state;
    const char *message;

    if (ctx == NULL || event == NULL) {
        return;
    }

    if (event->data != NULL &&
        (event->type == MIRROR_EVENT_CONNECTED ||
         event->type == MIRROR_EVENT_DEVICE_FOUND ||
         event->type == MIRROR_EVENT_STATE_CHANGED)) {
        device = (const MirrorDeviceInfo *)event->data;
        projector_adapter_store_current_device(device);
    }

    switch (event->type) {
        case MIRROR_EVENT_STATE_CHANGED:
            state = projector_adapter_from_mirror_state(screenmirror_get_state());
            projector_adapter_notify_state(state, NULL);
            break;
        case MIRROR_EVENT_CONNECTED:
            projector_adapter_notify_state(PROJECTOR_ADAPTER_STATE_CONNECTED,
                                           projector_adapter_event_message(event));
            break;
        case MIRROR_EVENT_DISCONNECTED:
            projector_adapter_notify_state(PROJECTOR_ADAPTER_STATE_IDLE,
                                           projector_adapter_event_message(event));
            break;
        case MIRROR_EVENT_DISCOVERY_FINISHED:
            state = projector_adapter_from_mirror_state(screenmirror_get_state());
            projector_adapter_notify_state(state, projector_adapter_event_message(event));
            break;
        case MIRROR_EVENT_DEVICE_FOUND:
            message = projector_adapter_event_message(event);
            projector_adapter_notify_state(PROJECTOR_ADAPTER_STATE_DISCOVERING, message);
            break;
        case MIRROR_EVENT_ERROR:
        default:
            projector_adapter_notify_state(PROJECTOR_ADAPTER_STATE_ERROR,
                                           projector_adapter_event_message(event));
            break;
    }
}
