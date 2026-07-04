#include "screenmirror.h"
#include <stdio.h>
#include <string.h>

static void on_event(const MirrorEvent *event, void *user_data)
{
    (void)user_data;
    if (event != NULL) {
        printf("[TEST] event=%d\n", event->type);
    }
}

static void on_devices(const MirrorDeviceInfo *devices, int count, void *user_data)
{
    (void)user_data;
    printf("[TEST] discovered=%d\n", count);
    if (devices != NULL && count > 0) {
        printf("[TEST] first=%s\n", devices[0].name);
    }
}

int main(void)
{
    MirrorDeviceInfo device;
    MirrorConfig config;
    memset(&device, 0, sizeof(device));
    memset(&config, 0, sizeof(config));

    snprintf(device.name, sizeof(device.name), "h133-test-device");
    snprintf(device.ip_address, sizeof(device.ip_address), "192.168.49.2");
    device.mode = MIRROR_MODE_MIRACAST;

    config.mode = MIRROR_MODE_MIRACAST;
    config.resolution_width = 1280;
    config.resolution_height = 720;
    config.refresh_rate = 60;
    config.enable_audio = true;
    config.connect_timeout_ms = 5000;

    if (screenmirror_init() != 0) {
        return 1;
    }

    (void)screenmirror_set_event_callback(on_event, NULL);
    (void)screenmirror_start_discovery(MIRROR_MODE_MIRACAST, 500, on_devices, NULL);
    (void)screenmirror_connect(&device, &config);
    (void)screenmirror_send_video_frame((const uint8_t *)"v", 1);
    (void)screenmirror_send_audio_frame((const uint8_t *)"a", 1);
    (void)screenmirror_control("stop");
    (void)screenmirror_disconnect();
    (void)screenmirror_stop_discovery();
    (void)screenmirror_exit();

    return 0;
}
