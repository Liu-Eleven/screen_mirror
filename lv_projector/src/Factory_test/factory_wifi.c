#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "../Common/language/string/lv_string_id.h"
#include "../Network/lv_pro_res_wifi.h"
#include "../Network/lv_pro_wifi_common.h"
#include "../lv_pro_launcher.h"
#include "factory_test.h"
#include "lv_common.h"
#include "lv_string_id.h"
#include "lvgl/lvgl.h"

#if ENABLE_WIFI

lv_obj_t *wifi_scanlist_obj;
static lv_obj_t *wifi_scan_item[SCAN_LIST_LEN];

static void set_wifi_switch(lv_obj_t *btn, int state)
{
    pthread_mutex_lock(&lvgl_mutex);

    if (state == 1) {
        lv_obj_add_state(btn, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(btn, LV_STATE_CHECKED);
    }
    pthread_mutex_unlock(&lvgl_mutex);
}

static void wifi_update_icon(lv_obj_t *icon, int rssi)
{
    const char *icon_path = NULL;

    if (rssi >= RSSI_HIGH_THRESHOLD) {
        icon_path = lv_pro_wifi_get_resource_path(WIFI_SCAN0_LOGO);
    } else if (rssi >= RSSI_MEDIUM_THRESHOLD) {
        icon_path = lv_pro_wifi_get_resource_path(WIFI_SCAN1_LOGO);
    } else if (rssi >= RSSI_LOW_THRESHOLD) {
        icon_path = lv_pro_wifi_get_resource_path(WIFI_SCAN2_LOGO);
    } else {
        icon_path = lv_pro_wifi_get_resource_path(WIFI_SCAN3_LOGO);
    }

    lv_img_set_src(icon, icon_path);
}

static void create_wifi_scan_item(int i)
{
    pthread_mutex_lock(&lvgl_mutex);
    wifi_scan_item[i] = lv_list_add_btn(wifi_scanlist_obj, NULL, NULL);
    lv_obj_set_width(wifi_scan_item[i], LV_PCT(100));

    // 设置背景颜色
    lv_obj_set_style_bg_color(wifi_scan_item[i], lv_color_hex(0x800080),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(wifi_scan_item[i], LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

    // 创建图标
    lv_obj_t *img = lv_img_create(wifi_scan_item[i]);
    const char *icon_path = lv_pro_wifi_get_resource_path(WIFI_SCAN0_LOGO);
    lv_img_set_src(img, icon_path);
    lv_obj_align(img, LV_ALIGN_LEFT_MID, 10, 0);

    // 设备名称
    lv_obj_t *devname = lv_label_create(wifi_scan_item[i]);
    lv_obj_set_width(devname, LV_PCT(60));
    lv_label_set_text(devname, "NAME");
    lv_label_set_long_mode(devname, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_font(devname, &GENERAL_FONT_MID, 0);
    lv_obj_set_style_text_color(devname, lv_color_white(), 0);
    lv_obj_align_to(devname, img, LV_ALIGN_OUT_RIGHT_BOTTOM, 10, 0);

    // 加密方式
    lv_obj_t *mgmt = lv_label_create(wifi_scan_item[i]);
    lv_obj_set_width(mgmt, LV_PCT(20));
    lv_label_set_text(mgmt, "OPEN");
    lv_label_set_long_mode(mgmt, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_font(mgmt, &GENERAL_FONT_SMALL, 0);
    lv_obj_set_style_text_color(mgmt, lv_color_hex(0xFFE4C4), 0);
    lv_obj_align_to(mgmt, devname, LV_ALIGN_OUT_RIGHT_BOTTOM, 10, 2);
    pthread_mutex_unlock(&lvgl_mutex);
}

static void set_wifi_scan_item(lv_obj_t *item, WIFIDeviceMsg *device)
{
    pthread_mutex_lock(&lvgl_mutex);
    lv_obj_t *img = lv_obj_get_child(item, 0);
    lv_obj_t *name = lv_obj_get_child(item, 1);
    lv_obj_t *mgmt = lv_obj_get_child(item, 2);

    wifi_update_icon(img, device->rssi);
    lv_label_set_text(name, device->ssid);
    lv_label_set_text(mgmt, device->mgmt);
    pthread_mutex_unlock(&lvgl_mutex);
}

static void hidden_wifi_scan_items()
{
    pthread_mutex_lock(&lvgl_mutex);
    for (int i = 0; i < SCAN_LIST_LEN; i++) {
        if (wifi_scan_item[i]) lv_obj_add_flag(wifi_scan_item[i], LV_OBJ_FLAG_HIDDEN);
    }
    pthread_mutex_unlock(&lvgl_mutex);
}

static void clear_wifi_scan_items()
{
    pthread_mutex_lock(&lvgl_mutex);
    for (int i = 0; i < SCAN_LIST_LEN; i++) {
        if (wifi_scan_item[i]) lv_obj_del(wifi_scan_item[i]);
        wifi_scan_item[i] = NULL;
    }
    pthread_mutex_unlock(&lvgl_mutex);
}

void *factory_wifi_task(void *parm)
{
    factory_wifi_info_t *wifiTask = (factory_wifi_info_t *)parm;
    wifi_scanlist_obj = wifiTask->wifi_list;

    int ret = -1;
    int scanNum = 0;

    // 1.强制关闭wifi
    lv_pro_res_wifi_deinit();
    sleep(2);

    lv_pro_res_wifi_init();
    sleep(2);

    while (wifiTask->wifiTask_run) {
        // task1:如果WIFI需要自动打开且WIFI为关闭状态
        if (wifiTask->autoONOFF == true && wifiTask->wifi_state == 0) {
            ret = lv_pro_res_wifi_on();
            if (ret == 0) {
                set_wifi_switch(wifiTask->wifi_switch, 1);
                wifi_sta_auto_reconnect(false);
                sleep(2);
                wifiTask->wifi_state = 1;
                wifiTask->info->dev_info.wifi_status = 1;
            } else {
                perror("wifi open failed!\n");
                break;
            }
        }

        // 如果WIFI未打开
        if (wifiTask->wifi_state == 0) {
            sleep(2);
            continue;
        }

        // task2:扫描wifi
        ret = lv_pro_res_wifi_scan();
        scanNum = lv_pro_res_wifi_get_scan_results_num();

        // task3:显示列表
        if (ret == 0 && scanNum > 0) {
            for (int i = 0; i < scanNum && i < SCAN_LIST_LEN; i++) {
                create_wifi_scan_item(i);
                WIFIDeviceMsg *device = lv_pro_wifi_scandev_init(&wifi_scan_res[i], i);
                set_wifi_scan_item(wifi_scan_item[i], device);
                free(device);
            }
        }

        sleep(10);

        // task4:如果WIFI需要自动关闭且WIFI为打开状态
        if (wifiTask->autoONOFF == true && wifiTask->wifi_state == 1) {
            ret = lv_pro_res_wifi_off();
            set_wifi_switch(wifiTask->wifi_switch, 0);
            sleep(2);

            wifiTask->wifi_state = 0;
            wifiTask->info->dev_info.wifi_status = 0;
        }

        hidden_wifi_scan_items();
        clear_wifi_scan_items();

        wifiTask->info->wifi_test_count++;
        sleep(5);
    }

    lv_pro_res_wifi_deinit();

    free(wifiTask);
    return NULL;
}
#endif