#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "lv_common.h"

#define FIRMWARE_VERSION_PATH   ("/proc/sys/kernel/version")

static int getmonth(const char *m)
{
    char *g_months[12] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    for (int i = 0; i < 12; i++) {
        if (!strncmp(g_months[i], m, strlen(m))) {
            printf("getmonth %d\n", i + 1);
            return i + 1;
        }
    }
    return 0;
}

uint32_t get_firmware_version(void)
{
    char str[64];
    int fd = -1, ret = -1;
    char firmware_version_str[32] = "0000000000";
    char *p;
    char month[8] = {0};
    char day[8] = {0};
    char time[16] = {0};
    char year[8] = {0};

    fd = open(FIRMWARE_VERSION_PATH, O_RDONLY, 0444);
    if (fd < 0) {
        loge("open failed: %s:%s", FIRMWARE_VERSION_PATH, strerror(errno));
        goto OUT;
    }
    memset(str, 0, sizeof(str));
    ret = read(fd, str, 64);
    if (ret < 0) {
        loge("read fail:%s", strerror(errno));
        goto OUT;
    }

    //printf("firmware_version %s\n", str);

    /* format: #14 PREEMPT Thu Nov  7 16:45:36 CST 2024 */
    strtok_r(str, " ", &p);   // #14
    strtok_r(NULL, " ", &p);  // PREEMPT
    strtok_r(NULL, " ", &p);  // Thu
    strncpy(month, strtok_r(NULL, " ", &p), 4);
    strncpy(day, strtok_r(NULL, " ", &p), 2);
    strncpy(time, strtok_r(NULL, " ", &p), 8);
    strtok_r(NULL, " ", &p);
    strncpy(year, strtok_r(NULL, " ", &p), 4);

    memset(firmware_version_str, 0, sizeof(firmware_version_str));
    sprintf(firmware_version_str, "%c%c%02d%02d%c%c%c%c", year[2], year[3],
        getmonth(month), atoi(day), time[0], time[1], time[3], time[4]);

OUT:
    if (fd > 0)
        close(fd);
    //printf("firmware_version_str %s\n", firmware_version_str);
    return atoi(firmware_version_str);
}

void system_power_off(void)
{
    system("poweroff");
}

void system_standby(void)
{
    printf("system_standby\n");
}

static void* system_init_late_thread(void *arg)
{
    reg_all_page();

    lv_init_sys_param_late();

#if ENABLE_AW_ACTIVATE
    AWActivate_init();
#endif

#if HDMI_MAX_PORT_NUM
    hdmi_control_init();
#endif

#if ENABLE_AUTO_BURN_KEY
    check_user_key();
    hdmi_hdcp14_reloadkey();
#endif
    return NULL;
}

void system_init_early(void)
{
#if ENABLE_DISK_MANAGER
    DiskManager_Init();
    DiskManager_detect();
#endif

#if ENABLE_MOTOR
    PowerOn_motor_reset(15);
#endif

    lv_init_sys_param_early();

#if ENABLE_PQ
    lv_init_pq();
#endif
}

void system_init_late(void)
{
    int ret = 0;
    pthread_t wait_pthread;

    ret = pthread_create(&wait_pthread, NULL, &system_init_late_thread, (void *)NULL);
    if (ret < 0)
        printf("system_init_late_thread error\n");
}
