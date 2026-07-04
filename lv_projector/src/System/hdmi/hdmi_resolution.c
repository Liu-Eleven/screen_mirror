#include "hdmi_resolution.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <linux/fb.h>
#include <pthread.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <time.h>

/* lvgl_mutex is defined in lv_pro_res.c and used to protect LVGL API
 * calls from non-main threads. We need it here to safely call
 * sunxifb_refresh_resolution from the hotplug thread. */
extern pthread_mutex_t lvgl_mutex;

#define DISP_HDMI_SUPPORT_MODE 0xc4
#define DISP_DEVICE_SWITCH 0x0F
#define DISP_DEVICE_SET_CONFIG 0x14
#define DISP_GET_OUTPUT_TYPE 0x09
#define DISP_OUTPUT_TYPE_NONE 0
#define DISP_OUTPUT_TYPE_HDMI 4
#define FBIO_RESET_FB          0x4634

enum disp_csc_type {
    DISP_CSC_TYPE_RGB = 0,
    DISP_CSC_TYPE_YUV444 = 1,
    DISP_CSC_TYPE_YUV422 = 2,
    DISP_CSC_TYPE_YUV420 = 3,
};

enum disp_data_bits {
    DISP_DATA_8BITS = 0,
    DISP_DATA_10BITS = 1,
    DISP_DATA_12BITS = 2,
};

enum disp_eotf {
    DISP_EOTF_RESERVED = 0x000,
    DISP_EOTF_BT709 = 0x001,
    DISP_EOTF_UNDEF = 0x002,
    DISP_EOTF_GAMMA22 = 0x004,
    DISP_EOTF_GAMMA28 = 0x005,
    DISP_EOTF_BT601 = 0x006,
    DISP_EOTF_SMPTE240M = 0x007,
    DISP_EOTF_LINEAR = 0x008,
    DISP_EOTF_SMPTE2084 = 0x010,
    DISP_EOTF_ARIB_STD_B67 = 0x012,
};

enum disp_color_space {
    DISP_UNDEF = 0x00,
    DISP_UNDEF_F = 0x01,
    DISP_GBR = 0x100,
    DISP_BT709 = 0x101,
    DISP_FCC = 0x102,
    DISP_BT470BG = 0x103,
    DISP_BT601 = 0x104,
    DISP_SMPTE240M = 0x105,
    DISP_BT2020NC = 0x107,
    DISP_BT2020C = 0x108,
};

enum disp_dvi_hdmi {
    DISP_DVI_HDMI_UNDEFINED = 0,
    DISP_DVI = 1,
    DISP_HDMI = 2,
};

enum disp_color_range {
    DISP_COLOR_RANGE_DEFAULT = 0,
    DISP_COLOR_RANGE_0_255 = 1,
    DISP_COLOR_RANGE_16_235 = 2,
};

enum disp_scan_info {
    DISP_SCANINFO_NO_DATA = 0,
    OVERSCAN = 1,
    UNDERSCAN = 2,
};

struct disp_device_config {
    unsigned int type;
    unsigned int mode;
    unsigned int format;
    unsigned int bits;
    unsigned int eotf;
    unsigned int cs;
    unsigned int dvi_hdmi;
    unsigned int range;
    unsigned int scan;
    unsigned int aspect_ratio;
    unsigned int reserve1;
};

enum disp_tv_mode {
    DISP_TV_MOD_480I = 0,
    DISP_TV_MOD_576I = 1,
    DISP_TV_MOD_480P = 2,
    DISP_TV_MOD_576P = 3,
    DISP_TV_MOD_720P_50HZ = 4,
    DISP_TV_MOD_720P_60HZ = 5,
    DISP_TV_MOD_1080I_50HZ = 6,
    DISP_TV_MOD_1080I_60HZ = 7,
    DISP_TV_MOD_1080P_24HZ = 8,
    DISP_TV_MOD_1080P_50HZ = 9,
    DISP_TV_MOD_1080P_60HZ = 0xa,
    DISP_TV_MOD_1080P_25HZ = 0x1a,
    DISP_TV_MOD_1080P_30HZ = 0x1b,
    DISP_TV_MOD_3840_2160P_30HZ = 0x1c,
    DISP_TV_MOD_3840_2160P_25HZ = 0x1d,
    DISP_TV_MOD_3840_2160P_24HZ = 0x1e,
    DISP_TV_MOD_3840_2160P_60HZ = 0x22,
    DISP_TV_MOD_3840_2160P_50HZ = 0x21,
    DISP_TV_MOD_2560_1440P_60HZ = 0x23,
    DISP_VESA_MOD_640_480P_60 = 0x50,
    DISP_VESA_MOD_800_600P_60 = 0x51,
    DISP_VESA_MOD_1024_768P_60 = 0x52,
    DISP_VESA_MOD_1280_768P_60 = 0x53,
    DISP_VESA_MOD_1280_800P_60 = 0x54,
    DISP_VESA_MOD_1366_768P_60 = 0x55,
    DISP_VESA_MOD_1440_900P_60 = 0x56,
    DISP_VESA_MOD_1600_900P_60 = 0x57,
    DISP_VESA_MOD_1920_1080P_60 = 0x58,
    DISP_VESA_MOD_1920_1200P_60 = 0x59,
};

struct disp_output {
    unsigned int type;
    unsigned int mode;
};

static const struct {
    int mode;
    int width;
    int height;
    int refresh_rate;
    const char *name;
} g_supported_modes[] = {
    { HDMI_MODE_AUTO,               0,    0,    0, "Auto" },
    { DISP_TV_MOD_3840_2160P_30HZ, 3840, 2160, 30, "4K@30Hz" },
    { DISP_TV_MOD_3840_2160P_25HZ, 3840, 2160, 25, "4K@25Hz" },
    { DISP_TV_MOD_3840_2160P_24HZ, 3840, 2160, 24, "4K@24Hz" },
    { DISP_TV_MOD_1080P_60HZ, 1920, 1080, 60, "1080p@60Hz" },
    { DISP_TV_MOD_1080P_50HZ, 1920, 1080, 50, "1080p@50Hz" },
    { DISP_TV_MOD_1080P_30HZ, 1920, 1080, 30, "1080p@30Hz" },
    { DISP_TV_MOD_1080P_25HZ, 1920, 1080, 25, "1080p@25Hz" },
    { DISP_TV_MOD_1080P_24HZ, 1920, 1080, 24, "1080p@24Hz" },
    { DISP_TV_MOD_720P_60HZ, 1280, 720, 60, "720p@60Hz" },
    { DISP_TV_MOD_720P_50HZ, 1280, 720, 50, "720p@50Hz" },
    { DISP_VESA_MOD_1920_1080P_60, 1920, 1080, 60, "VGA-1080p@60Hz" },
    { DISP_VESA_MOD_1920_1200P_60, 1920, 1200, 60, "1920x1200@60Hz" },
    { DISP_VESA_MOD_1600_900P_60, 1600, 900, 60, "1600x900@60Hz" },
    { DISP_VESA_MOD_1440_900P_60, 1440, 900, 60, "1440x900@60Hz" },
    { DISP_VESA_MOD_1366_768P_60, 1366, 768, 60, "1366x768@60Hz" },
    { DISP_VESA_MOD_1280_800P_60, 1280, 800, 60, "1280x800@60Hz" },
    { DISP_VESA_MOD_1280_768P_60, 1280, 768, 60, "1280x768@60Hz" },
    { DISP_VESA_MOD_1024_768P_60, 1024, 768, 60, "1024x768@60Hz" },
    { DISP_VESA_MOD_800_600P_60, 800, 600, 60, "800x600@60Hz" },
    { DISP_VESA_MOD_640_480P_60, 640, 480, 60, "640x480@60Hz" },
};

#define SUPPORTED_MODE_COUNT (sizeof(g_supported_modes) / sizeof(g_supported_modes[0]))

static hdmi_resolution_t *g_resolution_list = NULL;
static int g_resolution_count = 0;
static int g_current_mode = DISP_TV_MOD_1080P_60HZ;
static int g_disp_fd = -1;
static hdmi_resolution_change_callback_t g_resolution_callback = NULL;

/* HDMI uevent listener thread */
static pthread_t g_hotplug_thread;
static volatile bool g_hotplug_running = false;

/* Debounce: minimum interval (ms) between hotplug resolution changes.
 * Prevents rapid FBIO_RESET_FB + sunxifb_refresh_resolution calls
 * that cause "double free or corruption" during fast plug/unplug. */
#define HOTPLUG_DEBOUNCE_MS 500
static struct timespec g_last_hotplug_time = {0, 0};

void hdmi_set_resolution_change_callback(hdmi_resolution_change_callback_t callback) {
    g_resolution_callback = callback;
}

static int hdmi_check_mode_support(int mode)
{
    unsigned long args[4] = {0};
    int ret;

    if (g_disp_fd < 0) {
        g_disp_fd = open(HDMI_DISP_DEVICE, O_RDWR);
        if (g_disp_fd < 0) {
            return 0;
        }
    }
    args[0] = 0;
    args[1] = mode;
    ret = ioctl(g_disp_fd, DISP_HDMI_SUPPORT_MODE, (unsigned long)args);
    return (ret == 1);
}

int hdmi_get_resolutions(hdmi_resolution_t **list, int *count)
{
    int i;
    int supported_count = 0;
    hdmi_resolution_t *temp_list = NULL;

    if (g_resolution_list != NULL) {
        *list = g_resolution_list;
        *count = g_resolution_count;
        return g_resolution_count;
    }

    for (i = 0; i < SUPPORTED_MODE_COUNT; i++) {
        if (g_supported_modes[i].mode == HDMI_MODE_AUTO ||
            hdmi_check_mode_support(g_supported_modes[i].mode)) {
            temp_list = (hdmi_resolution_t *)realloc(temp_list,
                (supported_count + 1) * sizeof(hdmi_resolution_t));
            if (temp_list) {
                temp_list[supported_count].mode = g_supported_modes[i].mode;
                temp_list[supported_count].width = g_supported_modes[i].width;
                temp_list[supported_count].height = g_supported_modes[i].height;
                temp_list[supported_count].refresh_rate = g_supported_modes[i].refresh_rate;
                snprintf(temp_list[supported_count].name, sizeof(temp_list[supported_count].name),
                         "%s", g_supported_modes[i].name);
                supported_count++;
            }
        }
    }

    if (supported_count == 0) {
        if (temp_list) {
            free(temp_list);
        }
        supported_count = 2;
        temp_list = (hdmi_resolution_t *)malloc(supported_count * sizeof(hdmi_resolution_t));
        if (temp_list) {
            temp_list[0].mode = DISP_TV_MOD_720P_60HZ;
            temp_list[0].width = 1280;
            temp_list[0].height = 720;
            temp_list[0].refresh_rate = 60;
            snprintf(temp_list[0].name, sizeof(temp_list[0].name), "720p@60Hz");

            temp_list[1].mode = DISP_TV_MOD_1080P_60HZ;
            temp_list[1].width = 1920;
            temp_list[1].height = 1080;
            temp_list[1].refresh_rate = 60;
            snprintf(temp_list[1].name, sizeof(temp_list[1].name), "1080p@60Hz");
        }
    }

    g_resolution_list = temp_list;
    g_resolution_count = supported_count;
    *list = temp_list;
    *count = supported_count;
    return supported_count;
}

void hdmi_free_resolutions(hdmi_resolution_t *list)
{
    if (list == g_resolution_list) {
        return;
    }
    free(list);
}

int hdmi_set_resolution(int mode)
{
    //printf("hdmi_set_resolution start, mode=%d\n", mode);
    int fd;
    unsigned long args[4] = {0};
    struct disp_device_config config;
    int ret;

    /* Auto mode: find best resolution from EDID */
    int actual_mode = 10;//default 1080p@60Hz
    if (mode == HDMI_MODE_AUTO) {
        actual_mode = hdmi_find_best_resolution();
        mode = HDMI_MODE_ENCODE(actual_mode);
        printf("hdmi_set_resolution: auto mode, mode=%d\n", mode);
    } else {
        actual_mode = HDMI_MODE_GET_ACTUAL(mode);
        printf("hdmi_set_resolution: encoded auto mode, actual=%d\n", actual_mode);
        mode = actual_mode;
    }

    fd = open(HDMI_DISP_DEVICE, O_RDWR);
    if (fd < 0) {
        printf("hdmi_set_resolution: open %s failed, fd=%d, errno=%d\n", HDMI_DISP_DEVICE, fd, errno);
        return -1;
    }

    memset(&config, 0, sizeof(config));
    config.type = DISP_OUTPUT_TYPE_HDMI;
    config.mode = mode;
    config.format = DISP_CSC_TYPE_RGB;
    config.bits = DISP_DATA_8BITS;
    config.eotf = DISP_EOTF_GAMMA22;
    config.cs = DISP_BT709;
    config.dvi_hdmi = DISP_HDMI;
    config.range = DISP_COLOR_RANGE_DEFAULT;
    config.scan = UNDERSCAN;

    args[0] = 0;
    args[1] = (unsigned long)&config;
    //printf("hdmi_set_resolution: config type=%u, mode=%u, format=%u, bits=%u\n",config.type, config.mode, config.format, config.bits);

    ret = ioctl(fd, DISP_DEVICE_SET_CONFIG, (unsigned long)args);
    if (ret == 0) {
        /* Reset framebuffer to match new HDMI resolution */
        int fb_fd = open("/dev/fb0", O_RDWR);
        if (fb_fd >= 0) {
            int fb_ret = ioctl(fb_fd, FBIO_RESET_FB, 0);
            close(fb_fd);
        } else {
            printf("hdmi_set_resolution: open /dev/fb0 failed, errno=%d\n", errno);
        }

        /* Get actual display width/height from mode table for LVGL */
        int new_width = 0;
        int new_height = 0;
        int refresh_rate = 0;
        int i;
        for (i = 0; i < SUPPORTED_MODE_COUNT; i++) {
            if (g_supported_modes[i].mode == actual_mode) {
                new_width = g_supported_modes[i].width;
                new_height = g_supported_modes[i].height;
                refresh_rate = g_supported_modes[i].refresh_rate;
                break;
            }
        }

        close(fd);
        g_current_mode = mode;
        /* Save: auto mode encodes as 0xFF00|actual_mode, manual as actual_mode */
        hdmi_save_resolution(mode);
        
        /* Notify callback about resolution change */
        if (g_resolution_callback && new_width > 0 && new_height > 0) {
            g_resolution_callback(new_width, new_height);
        }
        printf("hdmi_set_resolution: actual display size: %dx%d, refresh rate=%d\n", new_width, new_height, refresh_rate);
        return 0;
    }
    close(fd);
    printf("hdmi_set_resolution: DISP_DEVICE_SET_CONFIG failed, try DISP_DEVICE_SWITCH ret=%d\n", ret);
    return -1;
}

int hdmi_get_current_resolution(void)
{
    /* For UI: return HDMI_MODE_AUTO if in auto mode, otherwise actual mode */
    if (HDMI_MODE_IS_AUTO(g_current_mode)) {
        return HDMI_MODE_AUTO;
    }
    return g_current_mode;
}

int hdmi_save_resolution(int mode)
{
    char value[128];
    snprintf(value, sizeof(value), "fw_setenv hdmi_resolution %d", mode);
    system(value);
    return 0;
}

int hdmi_load_saved_resolution(void)
{
    FILE *fp;
    char value[128];
    int mode;

    fp = popen("fw_printenv hdmi_resolution", "r");
    if (!fp) {
        return DISP_TV_MOD_1080P_60HZ;
    }

    if (fgets(value, sizeof(value), fp) == NULL) {
        pclose(fp);
        return DISP_TV_MOD_1080P_60HZ;
    }
    pclose(fp);
    char *eq = strchr(value, '=');
    if (eq) {
        mode = atoi(eq + 1);
    } else {
        mode = atoi(value);
    }
    if (mode == 0) {
        return DISP_TV_MOD_1080P_60HZ;
    }
    /* Value already contains the encoded format:
     * >= 0xFF00 means auto mode with actual_mode in low byte
     * < 0xFF00 means manual mode */
    return mode;
}

int hdmi_find_best_resolution(void)
{
    /* g_supported_modes[0] is HDMI_MODE_AUTO, start from index 1 */
    int i;
    for (i = 1; i < SUPPORTED_MODE_COUNT; i++) {
        if (hdmi_check_mode_support(g_supported_modes[i].mode)) {
            printf("hdmi_find_best_resolution: found best mode=%d (%s)\n",g_supported_modes[i].mode, g_supported_modes[i].name);
            return g_supported_modes[i].mode;
        }
    }
    /* Fallback to 1080p60 if no mode is supported */
    printf("hdmi_find_best_resolution: no supported mode found, fallback to 1080p60\n");
    return DISP_TV_MOD_1080P_60HZ;
}

int hdmi_resolution_init(void)
{
    g_resolution_list = NULL;
    g_resolution_count = 0;
    g_disp_fd = -1;
    g_current_mode = hdmi_load_saved_resolution();

    /* If in auto mode, notify kernel to set hdmi_auto_mode flag.
     * Do NOT reconfigure HDMI output - uboot/kernel already set
     * the correct resolution during boot. */
    if (HDMI_MODE_IS_AUTO(g_current_mode)) {
        int fd;
        unsigned long args[4] = {0};
        struct disp_device_config config;
        fd = open(HDMI_DISP_DEVICE, O_RDWR);
        if (fd >= 0) {
            memset(&config, 0, sizeof(config));
            config.type = DISP_OUTPUT_TYPE_HDMI;
            config.mode = 0xFF;  /* Tell kernel to set auto_mode flag only */
            args[0] = 0;
            args[1] = (unsigned long)&config;
            ioctl(fd, DISP_DEVICE_SET_CONFIG, (unsigned long)args);
            close(fd);
        }
        printf("hdmi_resolution_init: auto mode, notified kernel\n");
    }

    return 0;
}

void hdmi_resolution_deinit(void)
{
    if (g_resolution_list) {
        free(g_resolution_list);
        g_resolution_list = NULL;
    }
    g_resolution_count = 0;
    if (g_disp_fd >= 0) {
        close(g_disp_fd);
        g_disp_fd = -1;
    }
}

void hdmi_resolution_reset_cache(void)
{
    if (g_resolution_list) {
        free(g_resolution_list);
        g_resolution_list = NULL;
    }
    g_resolution_count = 0;
}


/*
 * HDMI uevent listener thread.
 *
 * The kernel HDMI driver handles hotplug resolution switching:
 *   - On unplug: clears video_enable, disables output
 *   - On replug: EDID parse -> auto-select best resolution -> enable output
 *   - Sends uevent: ACTION=change, HDMI_CONNECTION=plugged, HDMI_RESOLUTION=<disp_mode>
 *
 * This thread listens for those uevents and updates UI's framebuffer
 * by calling the resolution change callback (sunxifb_refresh_resolution).
 */
static void *hdmi_hotplug_thread_func(void *arg)
{
    struct sockaddr_nl addr;
    int sockfd;
    char buf[512];

    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = 0;  /* let kernel auto-assign unique pid, avoid EADDRINUSE */
    addr.nl_groups = 0xffffffff;  /* listen to all kernel multicast groups */

    sockfd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if (sockfd < 0) {
        printf("hdmi_hotplug: uevent socket failed, errno=%d\n", errno);
        return NULL;
    }

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        printf("hdmi_hotplug: uevent bind failed, errno=%d\n", errno);
        close(sockfd);
        return NULL;
    }

    printf("hdmi_hotplug: uevent listener started\n");

    while (g_hotplug_running) {
        ssize_t len = recv(sockfd, buf, sizeof(buf) - 1, 0);
        if (len <= 0) {
            if (errno == EINTR)
                continue;
            break;
        }
        buf[len] = '\0';

        /* Parse uevent: look for HDMI_RESOLUTION=<disp_mode> */
        char *p = buf;
        bool is_hdmi_event = false;
        int hdmi_disp_mode = -1;

        /* uevent format: action\0key1=val1\0key2=val2\0... */
        while (p < buf + len) {
            if (strncmp(p, "HDMI_RESOLUTION=", 16) == 0) {
                hdmi_disp_mode = atoi(p + 16);
                is_hdmi_event = true;
            }
            p += strlen(p) + 1;
        }

        if (is_hdmi_event && hdmi_disp_mode >= 0) {
            int disp_mode = hdmi_disp_mode;
            printf("hdmi_hotplug: received uevent, HDMI_RESOLUTION(disp_mode)=%d\n", disp_mode);

            /* Debounce: skip if last hotplug was too recent */
            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);
            long elapsed_ms = (now.tv_sec - g_last_hotplug_time.tv_sec) * 1000
                            + (now.tv_nsec - g_last_hotplug_time.tv_nsec) / 1000000;
            if (elapsed_ms < HOTPLUG_DEBOUNCE_MS) {
                printf("hdmi_hotplug: debounced, skipping (elapsed=%ldms < %dms)\n",
                       elapsed_ms, HOTPLUG_DEBOUNCE_MS);
                continue;
            }

            if (HDMI_MODE_IS_AUTO(g_current_mode)) {
                /* Auto mode: accept kernel's best resolution */
                int width = 0, height = 0;
                hdmi_resolution_reset_cache();

                /* Reset framebuffer to match new HDMI resolution */
                int fb_fd = open("/dev/fb0", O_RDWR);
                if (fb_fd >= 0) {
                    ioctl(fb_fd, FBIO_RESET_FB, 0);
                    close(fb_fd);
                }

                /* Find resolution from mode table */
                int mi;
                for (mi = 0; mi < SUPPORTED_MODE_COUNT; mi++) {
                    if (g_supported_modes[mi].mode == disp_mode) {
                        width = g_supported_modes[mi].width;
                        height = g_supported_modes[mi].height;
                        break;
                    }
                }
                /* Use lvgl_mutex to protect LVGL API calls in
                 * sunxifb_refresh_resolution (lv_disp_drv_update,
                 * lv_refr_now, etc.) from concurrent access by the
                 * LVGL main thread. */
                if (g_resolution_callback && width > 0 && height > 0) {
                    pthread_mutex_lock(&lvgl_mutex);
                    g_resolution_callback(width, height);
                    pthread_mutex_unlock(&lvgl_mutex);
                }
                printf("hdmi_hotplug: auto mode, disp_mode=%d, UI fb updated to %dx%d\n",disp_mode, width, height);
                clock_gettime(CLOCK_MONOTONIC, &g_last_hotplug_time);
            } 
        }
    }

    close(sockfd);
    printf("hdmi_hotplug: thread exiting\n");
    return NULL;
}

void hdmi_hotplug_start(void)
{
    if (g_hotplug_running)
        return;
    g_hotplug_running = true;
    if (pthread_create(&g_hotplug_thread, NULL, hdmi_hotplug_thread_func, NULL) != 0) {
        printf("hdmi_hotplug: failed to create thread\n");
        g_hotplug_running = false;
    }
}

void hdmi_hotplug_stop(void)
{
    if (!g_hotplug_running)
        return;
    g_hotplug_running = false;
    pthread_join(g_hotplug_thread, NULL);
}
