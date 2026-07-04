#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "setting_common.h"
#include "sound_setting.h"

int factory_get_sound_mode(void)
{
    char str_value[32] = {0};

    setting_config_read(SOUND_MAINKEY, SOUND_MODE_NAME, str_value);
    return atoi(str_value);
}

int factory_set_sound_mode(int value)
{
    char str_value[32] = {0};
    switch (value) {
        case SOUND_MODE_STANDARD:
        case SOUND_MODE_MUSIC:
        case SOUND_MODE_MOVIC:
        case SOUND_MODE_SPORTS:
        case SOUND_MODE_USER:
            break;
        default:
            value = 0;
    }
    sprintf(str_value, "%d", value);
    return setting_config_write(SOUND_MAINKEY, SOUND_MODE_NAME, str_value);
}

int factory_get_sound_output(void)
{
    char str_value[32] = {0};

    setting_config_read(SOUND_MAINKEY, SOUND_OUTPUT_NAME, str_value);
    return atoi(str_value);
}

int factory_set_sound_output(int value)
{
    char str_value[32] = {0};
    switch (value) {
        case SOUND_OUTPUT_SPEAKER:
        case SOUND_OUTPUT_ARC:
        case SOUND_OUTPUT_BT:
        case SOUND_OUTPUT_HEADPHONE:
        case SOUND_OUTPUT_OWA:
            break;
        default:
            value = 0;
    }
    sprintf(str_value, "%d", value);
    return setting_config_write(SOUND_MAINKEY, SOUND_OUTPUT_NAME, str_value);
}

int factory_get_sound_volume(void)
{
    char str_value[32] = {0};

    setting_config_read(SOUND_MAINKEY, SOUND_VOLUME, str_value);
    return atoi(str_value);
}

int factory_set_sound_volume(int value)
{
    char str_value[32] = {0};
    sprintf(str_value, "%d", value);
    return setting_config_write(SOUND_MAINKEY, SOUND_VOLUME, str_value);
}

int factory_get_aq_param(int mode, char *subkey)
{
    char str_value[32] = {0};
    char *mainkey;

    switch (mode) {
        case SOUND_MODE_STANDARD:
            mainkey = SOUND_MODE_STANDARD_NAME;
            break;
        case SOUND_MODE_MUSIC:
            mainkey = SOUND_MODE_MUSIC_NAME;
            break;
        case SOUND_MODE_MOVIC:
            mainkey = SOUND_MODE_MOVIE_NAME;
            break;
        case SOUND_MODE_SPORTS:
            mainkey = SOUND_MODE_SPORT_NAME;
            break;
        case SOUND_MODE_USER:
            mainkey = SOUND_MODE_USER_NAME;
            break;
        default:
            mainkey = SOUND_MODE_STANDARD_NAME;
            break;
    }

    setting_config_read(mainkey, subkey, str_value);
    return atoi(str_value);
}

int factory_set_aq_param(int mode, char *subkey, int value)
{
    char *mainkey;
    char str_value[32] = {0};

    switch (mode) {
        case SOUND_MODE_STANDARD:
            mainkey = SOUND_MODE_STANDARD_NAME;
            break;
        case SOUND_MODE_MUSIC:
            mainkey = SOUND_MODE_MUSIC_NAME;
            break;
        case SOUND_MODE_MOVIC:
            mainkey = SOUND_MODE_MOVIE_NAME;
            break;
        case SOUND_MODE_SPORTS:
            mainkey = SOUND_MODE_SPORT_NAME;
            break;
        case SOUND_MODE_USER:
            mainkey = SOUND_MODE_USER_NAME;
            break;
        default:
            mainkey = SOUND_MODE_STANDARD_NAME;
            break;
    }

    sprintf(str_value, "%d", value);
    return setting_config_write(mainkey, subkey, str_value);
}
