#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "setting_common.h"
#include "system_setting.h"
#include "lv_common.h"

int factory_get_background_style(void)
{
    char value_str[32] = {0};
	if (setting_config_read(SYSTEM_SETTING_MAINKEY, BG_SUBKEY, value_str)) {
        loge("read_picture_config error, name: %s\n", BG_SUBKEY);
        return -1;
    }
    return atoi(value_str);
}

int factory_get_background_style_string(char *value)
{
	if (setting_config_read(SYSTEM_SETTING_MAINKEY, BG_SUBKEY, value)) {
		loge("read_picture_config error, name: %s\n", BG_SUBKEY);
		return -1;
	}
	return 0;
}

int factory_set_background_style(int value)
{
    char value_str[32] = {0};
    sprintf(value_str, "%d", value);
    if (setting_config_write(SYSTEM_SETTING_MAINKEY, BG_SUBKEY, value_str)) {
        loge("set_picture_config error, name: %s\n", BG_SUBKEY);
        return -1;
    }
	return atoi(value_str);
}

int factory_reset_and_clear_networkfiles(void)
{
    const char *files_to_delete[] = {
        "/etc/bluetooth/bluealsa_card",
        "/etc/bluetooth/bt_connstate",
        "/etc/bluetooth/bt_state_on",
        "/etc/bluetooth/keys/*",
        "/etc/lib/bluetooth/*",
        "/etc/wifi/wifi_state_on",
        "/etc/wifi/wpa_supplicant/sockets"
    };

    for (size_t i = 0; i < sizeof(files_to_delete) / sizeof(files_to_delete[0]); i++) {
        char cmd[128] = {0};
        sprintf(cmd, "rm -rf %s", files_to_delete[i]);
        if (system(cmd) != 0) {
            loge("Failed to delete %s\n", files_to_delete[i]);
        }
        sync();
    }

    const char *wpa_supplicant_conf_path = "/etc/wifi/wpa_supplicant/wpa_supplicant.conf";
    FILE *file = fopen(wpa_supplicant_conf_path, "w");
    if (file != NULL) {
        fprintf(file, "disable_scan_offload=1\n");
        fprintf(file, "update_config=1\n");
        fprintf(file, "wowlan_triggers=any\n");
        fclose(file);
    } else {
        loge("Failed to open %s for writing\n", wpa_supplicant_conf_path);
        return -1;
    }

    return 0;
}

int factory_reset_setting(void)
{
    char cmd[64] = {0};
    sprintf(cmd, "%s %s %s", "cp", DEFAULT_SETTING_FILE_PATH, USER_SETTING_FILE_PATH);
	if (!system(cmd)) {
        sync();
        system("reboot");
        return 0;
    }
    loge("factory_reset_setting fail\n");
	return -1;
}

int factory_get_osd_language(void)
{
    int value[32] = {0};

    if (setting_config_read(SYSTEM_SETTING_MAINKEY, OSD_LANGUAGE_SUBKEY, value)) {
        loge("read_picture_config error, name: %s\n", OSD_LANGUAGE_SUBKEY);
        return -1;
    }
    return atoi(value);
}

int factory_set_osd_language(int value)
{
    char value_str[32] = {0};
    if (value < LANGUAGE_ENGLISH || value > LANGUAGE_MAX)
        return -1;
    sprintf(value_str, "%d", value);
    if (setting_config_write(SYSTEM_SETTING_MAINKEY, OSD_LANGUAGE_SUBKEY, value_str)) {
        loge("set_picture_config error, name: %s\n", OSD_LANGUAGE_SUBKEY);
        return -1;
    }
    return 0;
}

int factory_set_projection_mode(int value)
{
    char value_str[32] = {0};
    if (value < PROJECTION_FRONT_ID || value > PROJECTION_INVALID)
        return -1;
    sprintf(value_str, "%d", value);
    if (setting_config_write(SYSTEM_SETTING_MAINKEY, PROJECTION_MODE_SUBKEY, value_str)) {
        loge("set_picture_config error, name: %s\n", PROJECTION_MODE_SUBKEY);
        return -1;
    }

    /* display driver use disp_config.ini */
    display_set_ksc_flip_mode(value);
    return 0;
}

int factory_get_projection_mode(int is_from_disp_config_ini)
{
    int value[32] = {0};
    int mode;

    printf("is_from_disp_config_ini: %d\n", is_from_disp_config_ini);

    /* display driver use disp_config.ini */
    if(is_from_disp_config_ini != 0)
        mode = display_get_ksc_flip_mode_from_disp_config_ini();
    else
        mode = display_get_ksc_flip_mode();

    if (mode == PROJECTION_REAR_ID)
        return PROJECTION_REAR_ID;
    else if (mode == PROJECTION_CEILING_FRONT_ID)
        return PROJECTION_CEILING_FRONT_ID;
    else if (mode == PROJECTION_FRONT_ID)
        return PROJECTION_FRONT_ID;
    else if (mode == PROJECTION_REAR_CEILING_ID)
        return PROJECTION_REAR_CEILING_ID;
    else {
        if (setting_config_read(SYSTEM_SETTING_MAINKEY, PROJECTION_MODE_SUBKEY, value)) {
            loge("read_picture_config error, name: %s\n", PROJECTION_MODE_SUBKEY);
            return -1;
        }
        return atoi(value);
    }
    return PROJECTION_REAR_ID;
}

int factory_set_system_auto_sleep(int value)
{
    char value_str[32] = {0};

    sprintf(value_str, "%d", value);
    if (setting_config_write(SYSTEM_SETTING_MAINKEY, AUTO_SLEEP_SUBKEY, value_str)) {
		loge("set_picture_config error, name: %s\n", AUTO_SLEEP_SUBKEY);
		return -1;
	}
	return 0;
}

int factory_get_system_auto_sleep(void)
{
    int value_str[32] = {0};

    if (setting_config_read(SYSTEM_SETTING_MAINKEY, AUTO_SLEEP_SUBKEY, value_str)) {
        loge("read_picture_config error, name: %s\n", AUTO_SLEEP_SUBKEY);
        return -1;
	}

    return atoi(value_str);
}

int factory_set_osd_timer(int value)
{
    char value_str[32] = {0};

    sprintf(value_str, "%d", value);
    if (setting_config_write(SYSTEM_SETTING_MAINKEY, OSD_TIMER_SUBKEY, value_str)) {
		loge("set_picture_config error, name: %s\n", OSD_TIMER_SUBKEY);
		return -1;
	}
	return 0;
}

int factory_get_osd_timer(void)
{
    int value_str[32] = {0};

    if (setting_config_read(SYSTEM_SETTING_MAINKEY, OSD_TIMER_SUBKEY, value_str)) {
        loge("read_picture_config error, name: %s\n", OSD_TIMER_SUBKEY);
        return -1;
	}

    return atoi(value_str);
}
