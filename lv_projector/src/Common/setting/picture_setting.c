#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include "setting_common.h"
#include "picture_setting.h"
#include "pq/pq_control.h"

const char *Cur_Picture_Mode = PICTURE_MODE_STANDARD_NAME;
int Cur_Picture_ID = PICTURE_MODE_STANDARD_ID;

static int get_pq_para_string(char *mainkey, char *subkey, char *value)
{
	if (mainkey != NULL && subkey != NULL) {
		if (setting_config_read(mainkey, subkey, value)) {
			loge("read_picture_config error, miankey: %s, subkey: %s\n", mainkey, subkey);
			return -1;
		}
		return 0;
	}
	return -1;
}

int get_pq_para(char *mainkey, char *subkey)
{
    char value[32] = {0};
	if (mainkey != NULL && subkey != NULL) {
		if (setting_config_read(mainkey, subkey, value)) {
			loge("read_picture_config error, miankey: %s, subkey: %s\n", mainkey, subkey);
			return -1;
		}
		return atoi(value);
	}
	return -1;
}

int set_pq_para(char *mainkey, char *subkey, char *value)
{
	if (mainkey != NULL && subkey != NULL && value != NULL) {
		if (setting_config_write(mainkey, subkey, value)) {
			loge("set_picture_config error, miankey: %s, subkey: %s\n", mainkey, subkey);
			return -1;
		}
		return 0;
	}
	return -1;
}

int factory_get_picture_mode_id(void)
{
    char value[32] = {0};
    if (setting_config_read(PICTURE_MAINKEY, PICTURE_MODE_MAINKEY, value)) {
        loge("read_picture_config error, miankey: %s, subkey: %s\n", PICTURE_MAINKEY, PICTURE_MODE_MAINKEY);
        return -1;
    }
    Cur_Picture_ID = atoi(value);

    switch (Cur_Picture_ID) {
        case PICTURE_MODE_STANDARD_ID:
            Cur_Picture_Mode = PICTURE_MODE_STANDARD_NAME;
            break;
        case PICTURE_MODE_DYNAMIC_ID:
            Cur_Picture_Mode = PICTURE_MODE_DYNAMIC_NAME;
            break;
        case PICTURE_MODE_MILD_ID:
            Cur_Picture_Mode = PICTURE_MODE_MILD_NAME;
            break;
        case PICTURE_MODE_USER_ID:
            Cur_Picture_Mode = PICTURE_MODE_USER_NAME;
            break;
        default:
            Cur_Picture_Mode = PICTURE_MODE_STANDARD_NAME;
    }
    return Cur_Picture_ID;
}

int factory_set_picture_mode_id(int id)
{
    const char *value;
    char str_id[32] = {0};
    switch (id) {
        case PICTURE_MODE_STANDARD_ID:
            value = PICTURE_MODE_STANDARD_NAME;
            break;
        case PICTURE_MODE_DYNAMIC_ID:
            value = PICTURE_MODE_DYNAMIC_NAME;
            break;
        case PICTURE_MODE_MILD_ID:
            value = PICTURE_MODE_MILD_NAME;
            break;
        case PICTURE_MODE_USER_ID:
            value = PICTURE_MODE_USER_NAME;
            break;
        default: {
            value = PICTURE_MODE_STANDARD_NAME;
            id = PICTURE_MODE_STANDARD_ID;
            }
    }
    sprintf(str_id, "%d", id);
    if (setting_config_write(PICTURE_MAINKEY, PICTURE_MODE_MAINKEY, str_id)) {
        loge("read_picture_config error, name: %s\n", PICTURE_MAINKEY);
        return -1;
    }
    Cur_Picture_Mode = value;
    Cur_Picture_ID = id;
	return 0;
}

int factory_get_current_pq_para_string(char *subkey, char *value)
{
    const char *mainkey = Cur_Picture_Mode;
    if (!strncmp(subkey, PQ_ZOOM_NAME, strlen(PQ_ZOOM_NAME)) ||
            !strncmp(subkey, PQ_ASPECT_RATIO_NAME, strlen(PQ_ASPECT_RATIO_NAME))) {
        mainkey = PICTURE_MAINKEY;
    } else if (!strncmp(subkey, PQ_COLOR_TEMP_NAME, strlen(PQ_COLOR_TEMP_NAME))) {
        mainkey = PICTURE_COLOR_TEMP_MAINKEY;
        subkey = "mode";
    }
	return get_pq_para_string(mainkey, subkey, value);
}

int factory_get_current_pq_para(char *subkey)
{
    char value[32] = {0};
    const char *mainkey = Cur_Picture_Mode;
    if (!strncmp(subkey, PQ_ZOOM_NAME, strlen(PQ_ZOOM_NAME)) ||
            !strncmp(subkey, PQ_ASPECT_RATIO_NAME, strlen(PQ_ASPECT_RATIO_NAME))) {
        mainkey = PICTURE_MAINKEY;
    } else if (!strncmp(subkey, PQ_COLOR_TEMP_NAME, strlen(PQ_COLOR_TEMP_NAME))) {
        mainkey = PICTURE_COLOR_TEMP_MAINKEY;
        subkey = "mode";
    }
	if (!get_pq_para_string(mainkey, subkey, value))
        return atoi(value);
    return -1;
}

int factory_set_current_pq_para(char *subkey, int value, int save)
{
    const char *mainkey = Cur_Picture_Mode;
    const char *path = NULL;
    char ui_value[32] = {0};
    int min_value, max_value;
    int map_value;

    sprintf(ui_value, "%d", value);
	if (!strncmp(subkey, PQ_CONTRAST_NAME, strlen(PQ_CONTRAST_NAME))) {
		min_value = PQ_CONTRAST_HW_MIN_VALUE;
		max_value = PQ_CONTRAST_HW_MAX_VALUE;
		map_value = value;
        path = SOC_CONTRAST_PATH;
	} else if (!strncmp(subkey, PQ_BRIGHTNESS_NAME, strlen(PQ_BRIGHTNESS_NAME))) {
		min_value = PQ_BRIGHTNESS_HW_MIN_VALUE;
		max_value = PQ_BRIGHTNESS_HW_MAX_VALUE;
		map_value = value;
        path = SOC_BRIGHTNESS_PATH;
	} else if (!strncmp(subkey, PQ_SHARPNESS_NAME, strlen(PQ_SHARPNESS_NAME))) {
		min_value = PQ_SHARPNESS_HW_MIN_VALUE;
		max_value = PQ_SHARPNESS_HW_MAX_VALUE;
		map_value = value;
        path = SOC_SHARPNESS_PATH;
	} else if (!strncmp(subkey, PQ_COLOR_TEMP_NAME, strlen(PQ_COLOR_TEMP_NAME))) {
		min_value = PQ_COLOR_TEMP_HW_MIN_VALUE;
		max_value = PQ_COLOR_TEMP_HW_MAX_VALUE;
		map_value = value;
        mainkey = PICTURE_COLOR_TEMP_MAINKEY;
        subkey = "mode";
        path = SOC_COLOR_TEMP_PATH;
    } else if (!strncmp(subkey, PQ_ZOOM_NAME, strlen(PQ_ZOOM_NAME))) {
        min_value = PQ_ZOOM_HW_MIN_VALUE;
        max_value = PQ_ZOOM_HW_MAX_VALUE;
		map_value = value;
        mainkey = PICTURE_MAINKEY;
    } else if (!strncmp(subkey, PQ_ASPECT_RATIO_NAME, strlen(PQ_ASPECT_RATIO_NAME))) {
        min_value = PQ_ASPECT_RATIO_HW_MIN_VALUE;
        max_value = PQ_ASPECT_RATIO_HW_MAX_VALUE;
        map_value = value;
        mainkey = PICTURE_MAINKEY;
    } else if (!strncmp(subkey, PQ_COLOR_NAME, strlen(PQ_COLOR_NAME))) {
        min_value = PQ_COLOR_HW_MIN_VALUE;
        max_value = PQ_COLOR_HW_MAX_VALUE;
        map_value = value;
        path = SOC_COLOR_PATH;
	} else {
		return -1;
	}

	/* set pq */
	if (map_value < min_value || map_value > max_value)
		return -1;

    if (path)
        soc_write_pq(path, map_value);

    //printf("mainkek: %s, subkey: %s, value: %s\n", mainkey, subkey, ui_value);
	/* save pq config */
	if (save)
        return set_pq_para(mainkey, subkey, ui_value);
    return 0;
}
