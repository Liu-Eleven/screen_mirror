#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "lv_common.h"
#if ENABLE_KSC
#include "ksc/lv_ksc.h"
#endif
#include "lv_display.h"

static int set_flip_from_secure_storage(char *name, int value)
{
    char buf[128] = {0};

    sprintf(buf, "%d", value);
    private_data_write(name, buf, strlen(buf));

    return 0;
}

static int get_flip_from_secure_storage(char *name, char *value_str)
{
    int len;

    if (private_data_read(name, value_str, sizeof(value_str), &len))
        return -1;

    //printf("%s = %s\n", name, value_str);
    return 0;
}

int display_set_ksc_flip_mode(int mode)
{
#if ENABLE_KSC
    char value_str[32] = {0};
    char tmp[64] = {0};
    int flip_h;
    int flip_v;
    int  disp_flip_v;

    if (mode == MIRROR_H_0_V_0) {
        flip_h = 0;
        flip_v = 0;
    } else if (mode == MIRROR_H_0_V_1) {
        flip_h = 0;
        flip_v = 1;
    } else if (mode == MIRROR_H_1_V_0) {
        flip_h = 1;
        flip_v = 0;
    } else if (mode == MIRROR_H_1_V_1) {
        flip_h = 1;
        flip_v = 1;
    } else {
        flip_h = 0;
        flip_v = 0;
    }

#if 0
    sprintf(value_str, "%d", flip_h);
    if (writeConfig(DISPLAY_CONFIG_PATH, DISP_KSC_MAINKEY, DISP_KSC_FLIP_H_SUBKEY, value_str, 0)) {
        loge("display write config error, name: %s\n", DISP_KSC_FLIP_H_SUBKEY);
        return -1;
    }

    memset(value_str, 0, sizeof(value_str));
    sprintf(value_str, "%d", flip_v);
    if (writeConfig(DISPLAY_CONFIG_PATH, DISP_KSC_MAINKEY, DISP_KSC_FLIP_V_SUBKEY, value_str, 1)) {
        loge("display write config error, name: %s\n", DISP_KSC_FLIP_V_SUBKEY);
        return -1;
    }

    sprintf(tmp, "cp %s %s", DISPLAY_CONFIG_PATH, DISPLAY_CONFIG_BAK_PATH);
    system(tmp);

    memset(tmp, 0, sizeof(tmp));
    sprintf(tmp, "fsync -d %s", DISPLAY_CONFIG_BAK_PATH);
    system(tmp);
#else
    disp_flip_v = ((flip_h & 0x1) << 4) | (flip_v & 0x1);
    set_flip_from_secure_storage("disp_flip", disp_flip_v);
#endif
#endif
    return 0;
}

int display_get_ksc_flip_mode(void)
{
#if ENABLE_KSC
    char value_str[32] = {0};
    int flip_h;
    int flip_v;

    if (get_flip_from_secure_storage("disp_flip", value_str)) {
        if (readConfig(DISPLAY_CONFIG_PATH, DISP_KSC_MAINKEY, DISP_KSC_FLIP_H_SUBKEY, value_str)) {
            loge("display read config error, name: %s\n", DISP_KSC_FLIP_H_SUBKEY);
            return -1;
        }
        flip_h = atoi(value_str);

        memset(value_str, 0, sizeof(value_str));
        if (readConfig(DISPLAY_CONFIG_PATH, DISP_KSC_MAINKEY, DISP_KSC_FLIP_V_SUBKEY, value_str)) {
            loge("display read config error, name: %s\n", DISP_KSC_FLIP_H_SUBKEY);
            return -1;
        }
        flip_v = atoi(value_str);
    } else {
        flip_h = (atoi(value_str) >> 4)& 0x1;
        flip_v = atoi(value_str) & 0x1;
    }


    if (flip_h == 0 && flip_v == 0)
        return MIRROR_H_0_V_0;
    else if (flip_h == 0 && flip_v == 1)
        return MIRROR_H_0_V_1;
    else if (flip_h == 1 && flip_v == 0)
        return MIRROR_H_1_V_0;
    else if (flip_h == 1 && flip_v == 1)
        return MIRROR_H_1_V_1;
    else
        return -1;
#else
    return -1;
#endif
}

int display_get_ksc_flip_mode_from_disp_config_ini(void)
{
#if ENABLE_KSC
    char value_str[32] = {0};
    int flip_h;
    int flip_v;

    if (readConfig(DISPLAY_CONFIG_PATH, DISP_KSC_MAINKEY, DISP_KSC_FLIP_H_SUBKEY, value_str)) {
        loge("display read config error, name: %s\n", DISP_KSC_FLIP_H_SUBKEY);
        return -1;
    }
    flip_h = atoi(value_str);

    memset(value_str, 0, sizeof(value_str));
    if (readConfig(DISPLAY_CONFIG_PATH, DISP_KSC_MAINKEY, DISP_KSC_FLIP_V_SUBKEY, value_str)) {
        loge("display read config error, name: %s\n", DISP_KSC_FLIP_H_SUBKEY);
        return -1;
    }
    flip_v = atoi(value_str);

    if (flip_h == 0 && flip_v == 0)
        return MIRROR_H_0_V_0;
    else if (flip_h == 0 && flip_v == 1)
        return MIRROR_H_0_V_1;
    else if (flip_h == 1 && flip_v == 0)
        return MIRROR_H_1_V_0;
    else if (flip_h == 1 && flip_v == 1)
        return MIRROR_H_1_V_1;
    else
        return -1;
#else
    return -1;
#endif
}
