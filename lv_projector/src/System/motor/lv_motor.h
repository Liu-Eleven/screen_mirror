#ifndef LV_MOTOR_H
#define LV_MOTOR_H

#include <pthread.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>
#include <linux/ioctl.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <signal.h>

#define MOTOR_IOC_CW_MODE	_IOW('k', 1, __u8)
#define MOTOR_IOC_CCW_MODE	_IOW('k', 2, __u8)
#define MOTOR_IOC_CWQ_MODE	_IOW('k', 3, __u8)
#define MOTOR_IOC_CCWQ_MODE	_IOW('k', 4, __u8)

void lv_motor_forward(int step);
void lv_motor_reverse(int step);
int lv_get_motor_status(void);

#endif
