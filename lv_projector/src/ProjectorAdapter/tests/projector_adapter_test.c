#include "projector_adapter.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    int state_count;
    int device_count;
    ProjectorAdapterState last_state;
    ProjectorAdapterDevice last_device;
} AdapterProbe;

static void record_state(ProjectorAdapterState state,
                         const char *message,
                         void *user_data)
{
    AdapterProbe *probe = (AdapterProbe *)user_data;

    assert(message != NULL);
    probe->state_count++;
    probe->last_state = state;
}

static void record_devices(const ProjectorAdapterDevice *devices,
                           int device_count,
                           void *user_data)
{
    AdapterProbe *probe = (AdapterProbe *)user_data;

    assert(device_count > 0);
    probe->device_count = device_count;
    probe->last_device = devices[0];
}

int main(void)
{
    AdapterProbe probe = {0};
    ProjectorAdapterConfig config = {
        .protocol = PROJECTOR_PROTOCOL_MIRACAST,
        .max_bitrate = 8000,
        .resolution_width = 1280,
        .resolution_height = 720,
        .refresh_rate = 60,
        .connect_timeout_ms = 3000,
        .enable_audio = true,
        .enable_hdcp = false,
        .device_name = "Adapter Test Device",
        .extra_config = NULL,
    };
    ProjectorAdapterDevice current_device;

    assert(projector_adapter_init(&config) == 0);
    assert(strcmp(projector_adapter_get_device_name(), "Adapter Test Device") == 0);
    assert(projector_adapter_set_callbacks(record_state, record_devices, &probe) == 0);

    assert(projector_ui_on_screen_loaded() == 0);
    assert(probe.device_count == 1);
    assert(probe.last_state == PROJECTOR_ADAPTER_STATE_DISCOVERING);
    assert(strcmp(probe.last_device.name, "Miracast Demo Device") == 0);

    assert(projector_ui_on_device_selected(&probe.last_device) == 0);
    assert(projector_adapter_get_state() == PROJECTOR_ADAPTER_STATE_CONNECTED);
    assert(projector_adapter_get_current_device(&current_device) == 0);
    assert(strcmp(current_device.name, "Miracast Demo Device") == 0);

    assert(projector_ui_on_protocol_selected(PROJECTOR_PROTOCOL_AIRPLAY) == 0);
    assert(projector_adapter_get_protocol() == PROJECTOR_PROTOCOL_AIRPLAY);

    assert(projector_ui_on_disconnect_requested() == 0);
    assert(projector_ui_on_screen_unloaded() == 0);
    assert(projector_adapter_deinit() == 0);

    printf("projector_adapter_test passed\n");
    return 0;
}
