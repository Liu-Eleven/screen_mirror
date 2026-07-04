#include "lv_motor.h"
#include "lv_common.h"

#define MOTOR_STATUS_MIDDLE          (0)
#define MOTOR_STATUS_FAR_LEFT        (1)
#define MOTOR_STATUS_FAR_RIGHT       (2)

int cur_motor_status;

int lv_get_motor_status(void)
{
    return cur_motor_status;
}

static void __lv_get_motor_status(void)
{
    int fd;
    char status;
    fd = open("/sys/devices/platform/motor0/motor_ctrl", O_RDONLY);
    if (fd < 0) {
        loge("Failed to open motor_ctrl\n");
        return;
    }

    if (read(fd, &status, sizeof(status)) < 0) {
        loge("Failed to read motor_ctrl\n");
        close(fd);
        return;
    }
    close(fd);

    cur_motor_status = status - '0';
    if (cur_motor_status > 2) {
        cur_motor_status = 0;
    }
}

void lv_motor_forward(int step)
{
    int fd;

    __lv_get_motor_status();

    fd = open("/dev/motor0",O_RDWR);
    if (fd < 0) {
        loge("Failed to open motor0\n");
        return;
    }

    if (ioctl(fd, MOTOR_IOC_CW_MODE, &step)) {
        loge("motor ioctl error\n");
    }

    close(fd);
}

void lv_motor_reverse(int step)
{
    int fd;

    __lv_get_motor_status();

    fd = open("/dev/motor0",O_RDWR);
    if (fd < 0) {
        loge("Failed to open motor0\n");
        return;
    }

    if (ioctl(fd, MOTOR_IOC_CCW_MODE, &step)) {
        loge("motor ioctl error\n");
    }

    close(fd);
}

static int lv_get_motorLimiter_flag(void)
{
    int limit_fd;
    char status;

    limit_fd = open("/sys/devices/platform/motor_limiter/motor_limiter", O_RDONLY);
    if (limit_fd < 0)
        return;

    if (read(limit_fd,&status,sizeof(status)) < 0) {
        close(limit_fd);
        return;
    }

    int s_status = status - '0';
    if (s_status <= 0)
        s_status = 0;
    else
        s_status = 1;

    close(limit_fd);
    return s_status;
}

void PowerOn_motor_reset(int step)
{
    int motor_fd;

    motor_fd = open("/dev/motor0",O_RDWR);
    if(motor_fd < 0)
        return;

    if (!lv_get_motorLimiter_flag()) {
        if (ioctl(motor_fd, MOTOR_IOC_CCW_MODE, &step)) {
            close(motor_fd);
            return;
        }
    } else {
        close(motor_fd);
        return;
    }

    if (!lv_get_motorLimiter_flag()) {
        if (ioctl(motor_fd, MOTOR_IOC_CW_MODE, &step)) {
            close(motor_fd);
            return;
        }
    } else {
        close(motor_fd);
        return;
    }
}