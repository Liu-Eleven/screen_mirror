#ifndef __HDMI_RESOLUTION_H__
#define __HDMI_RESOLUTION_H__

#include <stdint.h>
#include <stdbool.h>

#define HDMI_EDID_PATH "/sys/devices/virtual/hdmi/hdmi/attr/edid"
#define HDMI_DISP_DEVICE "/dev/disp"
#define HDMI_RESOLUTION_SAVE_KEY "hdmi_resolution_mode"

/* Auto mode encoding: (0xFF << 8) | actual_mode
 * Example: auto + 4K30(0x1c) = 0xFF1C = 65308
 * This allows uboot and userspace to know both the mode and auto flag */
#define HDMI_MODE_AUTO 0xFF
#define HDMI_MODE_ENCODE(mode) (mode | (0xFF << 8))
#define HDMI_MODE_IS_AUTO(val) (((val) & 0xFF00) == 0xFF00)
#define HDMI_MODE_GET_ACTUAL(val) ((val) & 0xFF)

#define DISP_OUTPUT_TYPE_HDMI 4
#define DISP_DEVICE_SWITCH 0x0F

typedef struct {
    int mode;
    int width;
    int height;
    int refresh_rate;
    char name[32];
} hdmi_resolution_t;

int hdmi_get_resolutions(hdmi_resolution_t **list, int *count);

void hdmi_free_resolutions(hdmi_resolution_t *list);

int hdmi_set_resolution(int mode);

int hdmi_get_current_resolution(void);

int hdmi_save_resolution(int mode);

int hdmi_load_saved_resolution(void);

int hdmi_resolution_init(void);

void hdmi_resolution_deinit(void);

void hdmi_resolution_reset_cache(void);

int hdmi_find_best_resolution(void);

void hdmi_hotplug_start(void);

void hdmi_hotplug_stop(void);

/* Callback function type for resolution change notification */
typedef void (*hdmi_resolution_change_callback_t)(int new_width, int new_height);
void hdmi_set_resolution_change_callback(hdmi_resolution_change_callback_t callback);

#endif