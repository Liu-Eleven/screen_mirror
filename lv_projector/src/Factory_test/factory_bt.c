#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "../Bluetooth/lv_pro_bt_common.h"
#include "../Bluetooth/lv_pro_res_bt.h"
#include "../Common/language/string/lv_string_id.h"
#include "../lv_pro_launcher.h"
#include "factory_test.h"
#include "lv_common.h"
#include "lv_string_id.h"
#include "lvgl/lvgl.h"

#if ENABLE_BT

static int scan_resid = 0;
lv_obj_t *bt_scanlist_obj;
static lv_obj_t *bt_scan_item[SCAN_LIST_LEN];

static void set_bt_switch(lv_obj_t *btn, int state)
{
    pthread_mutex_lock(&lvgl_mutex);

    if (state == 1) {
        lv_obj_add_state(btn, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(btn, LV_STATE_CHECKED);
    }
    pthread_mutex_unlock(&lvgl_mutex);
}

static void bt_update_icon(lv_obj_t *icon, int cod)
{
    const char *icon_path = NULL;

    if (cod == 5898764) {
        icon_path = lv_pro_bt_get_resource_path(BT_SCAN_LOGO_PHONE);
    } else if (cod == 2360340) {
        icon_path = lv_pro_bt_get_resource_path(BT_SCAN_LOGO_HEADSET);
    } else if (cod == 2752780) {
        icon_path = lv_pro_bt_get_resource_path(BT_SCAN_LOGO_COMPUTER);
    } else {
        icon_path = lv_pro_bt_get_resource_path(BT_SCAN_LOGO_DF);
    }

    lv_img_set_src(icon, icon_path);
}

static void create_bt_scan_item(int i)
{
    pthread_mutex_lock(&lvgl_mutex);
    bt_scan_item[i] = lv_list_add_btn(bt_scanlist_obj, NULL, NULL);
    lv_obj_set_width(bt_scan_item[i], LV_PCT(100));

    // 设置背景颜色
    lv_obj_set_style_bg_color(bt_scan_item[i], lv_color_hex(0x800080),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(bt_scan_item[i], LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

    // 创建图标
    lv_obj_t *img = lv_img_create(bt_scan_item[i]);
    const char *icon_path = lv_pro_bt_get_resource_path(BT_SCAN_LOGO_DF);
    lv_img_set_src(img, icon_path);
    lv_obj_align(img, LV_ALIGN_LEFT_MID, 10, 0);

    // 设备名称
    lv_obj_t *devname = lv_label_create(bt_scan_item[i]);
    lv_obj_set_width(devname, LV_PCT(80));
    lv_label_set_text(devname, "NAME");
    lv_label_set_long_mode(devname, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_font(devname, &GENERAL_FONT_MID, 0);
    lv_obj_set_style_text_color(devname, lv_color_white(), 0);
    lv_obj_align_to(devname, img, LV_ALIGN_OUT_RIGHT_BOTTOM, 10, 0);

    pthread_mutex_unlock(&lvgl_mutex);
}

static void set_bt_scan_item(lv_obj_t *item, btmg_bt_device_t *device)
{
    pthread_mutex_lock(&lvgl_mutex);
    lv_obj_t *img = lv_obj_get_child(item, 0);
    lv_obj_t *name = lv_obj_get_child(item, 1);

    bt_update_icon(img, device->r_class);
    lv_label_set_text(name, device->remote_name);
    pthread_mutex_unlock(&lvgl_mutex);
}

static void hidden_bt_scan_items()
{
    pthread_mutex_lock(&lvgl_mutex);
    for (int i = 0; i < SCAN_LIST_LEN; i++) {
        if (bt_scan_item[i]) lv_obj_add_flag(bt_scan_item[i], LV_OBJ_FLAG_HIDDEN);
    }
    pthread_mutex_unlock(&lvgl_mutex);
}

static void clear_bt_scan_items()
{
    pthread_mutex_lock(&lvgl_mutex);
    for (int i = 0; i < SCAN_LIST_LEN; i++) {
        if (bt_scan_item[i]) lv_obj_del(bt_scan_item[i]);
        bt_scan_item[i] = NULL;
    }
    pthread_mutex_unlock(&lvgl_mutex);
}

static void lv_pro_factory_bt_dev_add_cb(btmg_bt_device_t *device)
{
    dev_node_t *dev_node = NULL;

    BT_INF("address:%s, name:%s, class:%d, icon:%s, rssi:%d\n", device->remote_address,
           device->remote_name, device->r_class, device->icon, device->rssi);

    if (scan_resid < SCAN_LIST_LEN) {
        create_bt_scan_item(scan_resid);
        set_bt_scan_item(bt_scan_item[scan_resid], device);
        scan_resid++;
    }
}

void *factory_bt_task(void *parm)
{
    factory_bt_info_t *btTask = (factory_bt_info_t *)parm;
    bt_scanlist_obj = btTask->bt_list;

    int ret = -1;
    int scanNum = 0;
    scan_resid = 0;

    // 1.强制关闭bt
    lv_pro_res_bt_deinit();
    sleep(2);

    lv_pro_res_bt_init();
    sleep(2);

    factory_bt_callback->btmg_gap_cb.gap_device_add_cb = lv_pro_factory_bt_dev_add_cb;

    while (btTask->btTask_run) {
        // task1:如果bt需要自动打开且bt为关闭状态
        if (btTask->autoONOFF == true && btTask->bt_state == 0) {
            ret = lv_pro_res_bt_on();
            set_bt_switch(btTask->bt_switch, 1);
            sleep(2);
            if (ret == 0) {
                btTask->bt_state = 1;
                btTask->info->dev_info.bt_status = 1;
            } else {
                perror("bt open failed!\n");
                break;
            }
        }

        // 如果bt未打开
        if (btTask->bt_state == 0) {
            sleep(2);
            continue;
        }

        // task2:扫描bt
        ret = lv_pro_res_bt_scan(1);
        sleep(10);

        ret = lv_pro_res_bt_scan(0);
        sleep(2);

        // task3:如果bt需要自动关闭且bt为打开状态
        if (btTask->autoONOFF == true && btTask->bt_state == 1) {
            ret = lv_pro_res_bt_off();
            set_bt_switch(btTask->bt_switch, 0);
            sleep(2);

            btTask->bt_state = 0;
            btTask->info->dev_info.bt_status = 0;
        }

        hidden_bt_scan_items();
        clear_bt_scan_items();
        scan_resid = 0;  // reset

        btTask->info->bt_test_count++;
        sleep(5);
    }

    lv_pro_res_bt_deinit();

    free(btTask);
    return NULL;
}
#endif