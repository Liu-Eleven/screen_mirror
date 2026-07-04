#ifndef FACTORY_TEST_H
#define FACTORY_TEST_H

#include <stdio.h>
#include <stdlib.h>

#include "lv_common.h"
#include "lvgl/lvgl.h"

#define FACTORY_KEY_NAME    ("factory_data")

#define REBOOT_TEST_COUNT   (50)
#define WIFI_TEST_COUNT     (100)
#define BT_TEST_COUNT       (100)

typedef struct {
    char usb_status;
    char gsensor_status;
    char bt_status;
    char wifi_status;
} dev_test_info_t;

typedef struct {
    int status;
    int test_time;
    int wifi_test_count;
    int bt_test_count;
    int reboot_test_count;
    char version[64];
    dev_test_info_t dev_info;
} factory_info_t;

/* wifi */
#if ENABLE_WIFI
typedef struct {
    bool wifiTask_run;
    bool autoONOFF;
    int wifi_state;
    lv_obj_t *wifi_list;
    lv_obj_t *wifi_switch;
    factory_info_t *info;
} factory_wifi_info_t;

extern void *factory_wifi_task(void *parm);
#endif

/* BT */
#if ENABLE_BT
typedef struct {
    bool btTask_run;
    bool autoONOFF;
    int bt_state;
    lv_obj_t *bt_list;
    lv_obj_t *bt_switch;
    factory_info_t *info;
} factory_bt_info_t;

extern void *factory_bt_task(void *parm);
#endif

extern factory_info_t *factory_info;
extern lv_obj_t *factory_ui_group;

/* HDMI */
#define HDMI_DISPLAY_W_RATIO    (40)
#define HDMI_DISPLAY_H_RATIO    (35)

#endif
