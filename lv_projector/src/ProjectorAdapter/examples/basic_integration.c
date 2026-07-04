#include "projector_adapter.h"

#include <stdio.h>
#include <string.h>

static void on_state_changed(ProjectorAdapterState state,
                             const char *message,
                             void *user_data)
{
    (void)user_data;
    printf("state=%d message=%s\n", state, message);
}

static void on_devices_found(const ProjectorAdapterDevice *devices,
                             int device_count,
                             void *user_data)
{
    ProjectorAdapterDevice *selected = (ProjectorAdapterDevice *)user_data;

    if (device_count <= 0) {
        return;
    }

    *selected = devices[0];
}

int main(void)
{
    ProjectorAdapterConfig config = {
        .protocol = PROJECTOR_PROTOCOL_MIRACAST,
        .max_bitrate = 10000,
        .resolution_width = 1920,
        .resolution_height = 1080,
        .refresh_rate = 60,
        .connect_timeout_ms = 5000,
        .enable_audio = true,
        .enable_hdcp = false,
        .device_name = "LV Projector",
        .extra_config = NULL,
    };
    ProjectorAdapterDevice device;

    memset(&device, 0, sizeof(device));
    projector_adapter_init(&config);
    projector_adapter_set_callbacks(on_state_changed, on_devices_found, &device);

    projector_ui_on_screen_loaded();
    if (device.name[0] != '\0') {
        projector_ui_on_device_selected(&device);
    }

    projector_ui_on_disconnect_requested();
    projector_adapter_deinit();
    return 0;
}
