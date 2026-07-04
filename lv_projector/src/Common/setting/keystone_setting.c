#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "setting_common.h"
#include "keystone_setting.h"

int factory_get_manual_keystone_para(int id)
{
    char *subkey;
    char str_value[32] = {0};
    switch (id) {
        case MK_TOP_LEFT_X_ID:
            subkey = MK_TOP_LEFT_X_NAME;
            break;
        case MK_TOP_LEFT_Y_ID:
            subkey = MK_TOP_LEFT_Y_NAME;
            break;
        case MK_TOP_RIGHT_X_ID:
            subkey = MK_TOP_RIGHT_X_NAME;
            break;
        case MK_TOP_RIGHT_Y_ID:
            subkey = MK_TOP_RIGHT_Y_NAME;
            break;
        case MK_BOTTOM_LEFT_X_ID:
            subkey = MK_BOTTOM_LEFT_X_NAME;
            break;
        case MK_BOTTOM_LEFT_Y_ID:
            subkey = MK_BOTTOM_LEFT_Y_NAME;
            break;
        case MK_BOTTOM_RIGHT_X_ID:
            subkey = MK_BOTTOM_RIGHT_X_NAME;
            break;
        case MK_BOTTOM_RIGHT_Y_ID:
            subkey = MK_BOTTOM_RIGHT_Y_NAME;
            break;
        default:
            subkey = MK_TOP_LEFT_X_NAME;
    }

    setting_config_read(KEYSTONE_MAINKEY, subkey, str_value);
    return atoi(str_value);
}

int factory_set_manual_keystone_para(int id, int value)
{
    char *subkey;
    char str_value[32] = {0};
    switch (id) {
        case MK_TOP_LEFT_X_ID:
            subkey = MK_TOP_LEFT_X_NAME;
            break;
        case MK_TOP_LEFT_Y_ID:
            subkey = MK_TOP_LEFT_Y_NAME;
            break;
        case MK_TOP_RIGHT_X_ID:
            subkey = MK_TOP_RIGHT_X_NAME;
            break;
        case MK_TOP_RIGHT_Y_ID:
            subkey = MK_TOP_RIGHT_Y_NAME;
            break;
        case MK_BOTTOM_LEFT_X_ID:
            subkey = MK_BOTTOM_LEFT_X_NAME;
            break;
        case MK_BOTTOM_LEFT_Y_ID:
            subkey = MK_BOTTOM_LEFT_Y_NAME;
            break;
        case MK_BOTTOM_RIGHT_X_ID:
            subkey = MK_BOTTOM_RIGHT_X_NAME;
            break;
        case MK_BOTTOM_RIGHT_Y_ID:
            subkey = MK_BOTTOM_RIGHT_Y_NAME;
            break;
        default:
            subkey = MK_TOP_LEFT_X_NAME;
    }
    sprintf(str_value, "%d", value);
    return setting_config_write(KEYSTONE_MAINKEY, subkey, str_value);
}

int factory_get_auto_keystone_mode(void)
{
    char str_value[32] = {0};

    setting_config_read(KEYSTONE_MAINKEY, AUTO_KEYSTONE_NAME, str_value);
    return atoi(str_value);
}

int factory_set_auto_keystone_mode(int value)
{
    char str_value[32] = {0};
    sprintf(str_value, "%d", value);
    return setting_config_write(KEYSTONE_MAINKEY, AUTO_KEYSTONE_NAME, str_value);
}
