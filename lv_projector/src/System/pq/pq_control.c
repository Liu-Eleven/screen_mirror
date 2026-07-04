#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include "lv_common.h"
#include "sys_param.h"
#include "pq_control.h"
#include "../Common/setting/picture_setting.h"

pthread_mutex_t gMutex = PTHREAD_MUTEX_INITIALIZER;

int soc_read_pq(char *path)
{
	char value_str[40] = {0};
	int fd = -1, ret = -1;
	unsigned int result = 0;

    if (path == NULL) {
        loge("soc_write_pq path is null\n");
        return -1;
    }

	pthread_mutex_lock(&gMutex);

	fd = open(path, O_RDWR|O_CREAT, 0644);
	if (fd < 0) {
		loge("open failed: %s:%s", path, strerror(errno));
		goto CLOSE;
	}
	memset(value_str, 0 ,40);
	ret = read(fd, value_str, 40);
	if (ret < 0) {
		loge("read fail:%s", strerror(errno));
		goto CLOSE;
	}
	result = atoi(value_str);
CLOSE:
	if (fd >= 0)
		close(fd);
	pthread_mutex_unlock(&gMutex);
	return result;
}

int soc_write_pq(char *path, int value)
{
	char value_str[40] = {0};
	int fd = -1, ret = -1;

    if (path == NULL) {
        loge("soc_write_pq path is null\n");
        return -1;
    }
	pthread_mutex_lock(&gMutex);
	fd = open(path, O_RDWR|O_CREAT, 0644);
	if (fd < 0) {
		loge("open failed: %s:%s", path, strerror(errno));
		goto CLOSE;
	}
    sprintf(value_str, "%d", value);
	ret = write(fd, value_str, strlen(value_str));
	if (ret != strlen(value_str)) {
		loge("Write %s fail:%s", value_str, strerror(errno));
		goto CLOSE;
	}
	ret = 0;
CLOSE:
	if (fd >= 0)
		close(fd);
	pthread_mutex_unlock(&gMutex);
	return ret;
}


void set_all_pq_for_current_picture_mode(void)
{
    int value;
    int color_temp_value = 255;

    value = lv_get_sys_param(P_CONTRAST);
    if (value > 0)
        factory_set_current_pq_para(PQ_CONTRAST_NAME, value, 0);

    value = lv_get_sys_param(P_BRIGHTNESS);
    if (value > 0)
        factory_set_current_pq_para(PQ_BRIGHTNESS_NAME, value, 0);

    value = lv_get_sys_param(P_SHARPNESS);
    if (value > 0)
        factory_set_current_pq_para(PQ_SHARPNESS_NAME, value, 0);

    value = lv_get_sys_param(P_COLOR);
    if (value > 0)
        factory_set_current_pq_para(PQ_COLOR_NAME, value, 0);

    value = lv_get_sys_param(P_COLOR_TEMP);
    if (value == PQ_COLOR_TEMP_STANDARD_ID) {
        color_temp_value = get_pq_para(PQ_COLOR_TEMP_NAME, PQ_COLOR_TEMP_STANDARD_NAME);
    } else if (value == PQ_COLOR_TEMP_COLD_ID) {
        color_temp_value = get_pq_para(PQ_COLOR_TEMP_NAME, PQ_COLOR_TEMP_COLD_NAME);
    } else if (value == PQ_COLOR_TEMP_WARM_ID) {
        color_temp_value = get_pq_para(PQ_COLOR_TEMP_NAME, PQ_COLOR_TEMP_WARM_NAME);
    }
    if (color_temp_value >= PQ_COLOR_TEMP_UI_MIN_VALUE && color_temp_value <= PQ_COLOR_TEMP_UI_MAX_VALUE)
        soc_write_pq(SOC_COLOR_TEMP_PATH, color_temp_value);
}

int lv_init_pq(void)
{
    /* enable pq mode */
    soc_write_pq(SOC_PQ_MODE_PATH, 2);

    //set_all_pq_for_current_picture_mode();
    return 0;
}