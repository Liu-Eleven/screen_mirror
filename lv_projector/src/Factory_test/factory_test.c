#include "factory_test.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "../lv_pro_launcher.h"
#include "key_event.h"
#include "lv_common.h"
#include "lv_string_id.h"
#include "lvgl/lvgl.h"
#include "page.h"
#include "sys_param.h"
#include "hdmiapi.h"
#include "../widget/lv_pro_res.h"

#define label_angle 10

lv_obj_t *factory_ui;
lv_obj_t *factory_ui_group;

lv_obj_t *hdmi_source_obj;

lv_obj_t *test_result_obj;
lv_obj_t *test_result_lable;

lv_obj_t *usb_text_lable;
lv_obj_t *usb_value_lable;

lv_obj_t *sensor_text_lable;
lv_obj_t *sensor_value_lable;

lv_obj_t *wifi_test_text_lable;
lv_obj_t *wifi_test_value_lable;

lv_obj_t *bt_test_text_lable;
lv_obj_t *bt_test_value_lable;

lv_obj_t *test_time_text_lable;
lv_obj_t *test_time_value_lable;

lv_obj_t *reboot_count_text_lable;
lv_obj_t *reboot_count_value_lable;

lv_obj_t *version_text_lable;

#if ENABLE_WIFI
lv_obj_t *wifi_lable;
lv_obj_t *wifi_switch;
lv_obj_t *wifi_list;
factory_wifi_info_t *factory_wifi_info;
#endif

#if ENABLE_BT
lv_obj_t *bt_lable;
lv_obj_t *bt_switch;
lv_obj_t *bt_list;
factory_bt_info_t *factory_bt_info;
#endif

#define IMAGE_MAX_COUNT    4
lv_obj_t *image_page;
uint8_t image_count;

lv_obj_t *aging_page;

static int factory_test_flag;
static int factory_test_count;

static lv_timer_t * factory_test_timer = NULL;

int get_factor_test_flag(void) { return factory_test_flag; }

int check_factor_test(void)
{
    printf("check_factor_test %d\n", factory_test_count);
    if (factory_test_count < 8) {
        factory_test_count++;
        return -1;
    }

    factory_test_count = 0;
    return 0;
}

static void update_factory_test_all_date(void)
{
    char value[128];

    if (DiskManager_GetDiskNum() > 0) {
        lv_obj_set_style_text_color(usb_value_lable, COLOR_GREEN, 0);
        lv_label_set_text(usb_value_lable, "已插入");
    } else {
        lv_obj_set_style_text_color(usb_value_lable, COLOR_RED, 0);
        lv_label_set_text(usb_value_lable, "未插入");
    }

    memset(value, 0, sizeof(value));
    sprintf(value, "已完成%d次", factory_info->wifi_test_count);
    lv_label_set_text(wifi_test_value_lable, value);

    memset(value, 0, sizeof(value));
    sprintf(value, "已完成%d次", factory_info->bt_test_count);
    lv_label_set_text(bt_test_value_lable, value);

    memset(value, 0, sizeof(value));
    sprintf(value, "%d次", factory_info->reboot_test_count);
    lv_label_set_text(reboot_count_value_lable, value);
}

static void factory_test_timer_handler(lv_timer_t * timer)
{
    update_factory_test_all_date();
}

static void factory_test_aging_event(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t *key = lv_event_get_param(e);
    lv_obj_t * cur_obj = lv_event_get_target(e);
    lv_obj_t * next_obj;

    if (code == LV_EVENT_KEY) {
        if (*key == LV_KEY_ENTER) {

        } else if (*key == LV_KEY_BACK) {
            lv_obj_del(aging_page);
            aging_page = NULL;
            set_current_ui_group(factory_ui_group);
            factory_test_hdmi_start();
        }
    }
}

void factory_test_aging_ui(void)
{
    aging_page = lv_obj_create(lv_layer_top());
    lv_obj_set_size(aging_page, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(aging_page, COLOR_WHITE, 0);
    lv_obj_set_style_bg_opa(aging_page, LV_OPA_100, 0);
    lv_obj_clear_flag(aging_page, LV_OBJ_FLAG_SCROLLABLE);

    lv_group_only_add_one_obj(key_group, aging_page);
    lv_obj_add_event_cb(aging_page, factory_test_aging_event, LV_EVENT_KEY, NULL);
    set_current_ui_group(key_group);
}

static char *png_path[IMAGE_MAX_COUNT] = {
                "A:/usr/share/lv_projector/factory/image_scale.png",
                "A:/usr/share/lv_projector/factory/iamge_focus.png",
                "A:/usr/share/lv_projector/factory/image_black.png",
                "A:/usr/share/lv_projector/factory/image_white.png"};

static void factory_test_image_event(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t *key = lv_event_get_param(e);
    lv_obj_t * cur_obj = lv_event_get_target(e);
    lv_obj_t * next_obj;

    if (code == LV_EVENT_KEY) {
        if (*key == LV_KEY_ENTER) {
            image_count++;
            if (image_count > (IMAGE_MAX_COUNT - 1))
                image_count = 0;
            lv_img_set_src(lv_obj_get_child(image_page, 0), png_path[image_count]);
        } else if (*key == LV_KEY_BACK) {
            lv_obj_del(image_page);
            image_page = NULL;
            set_current_ui_group(factory_ui_group);
            factory_test_hdmi_start();
        }
    }
}

void factory_test_image_ui(void)
{
    lv_obj_t * img_obj;
    image_page = lv_obj_create(lv_layer_top());
    lv_obj_set_size(image_page, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(image_page, LV_OPA_100, 0);
    lv_obj_set_style_border_width(image_page, 0, 0);
    lv_obj_set_style_outline_width(image_page, 0, 0);
    lv_obj_clear_flag(image_page, LV_OBJ_FLAG_SCROLLABLE);

    image_count = 0;
    img_obj = lv_img_create(image_page);
    lv_obj_align(img_obj, LV_ALIGN_CENTER, 0, 0);
    lv_img_set_src(img_obj, png_path[image_count]);

    lv_group_only_add_one_obj(key_group, img_obj);
    lv_obj_add_event_cb(img_obj, factory_test_image_event, LV_EVENT_KEY, NULL);
    set_current_ui_group(key_group);
}

lv_obj_t * factory_create_text_value(lv_obj_t * parent, char * txt, char * value)
{
    lv_obj_t * obj = lv_obj_create(parent);
    lv_obj_set_size(obj, LV_PCT(100), LV_PCT(10));
    lv_obj_add_style(obj, &lv_pro_res.set_bg_black, 0);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(obj, 0, 0);

    lv_obj_t * text_label = NULL;
    lv_obj_t * value_label = NULL;

    if(txt) {
        text_label = lv_label_create(obj);
        lv_label_set_text(text_label, txt);
        lv_obj_add_style(text_label, &lv_pro_res.label_white, 0);
        lv_obj_set_style_text_font(text_label, &GENERAL_FONT_MID, 0);
        lv_label_set_long_mode(text_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_align(text_label, LV_ALIGN_LEFT_MID, LV_PCT(7), 0);
        lv_obj_set_width(text_label, LV_PCT(34));
    }

    if(value) {
        value_label = lv_label_create(obj);
        lv_label_set_text(value_label, value);
        lv_obj_add_style(value_label, &lv_pro_res.label_white, 0);
        lv_obj_set_style_text_font(value_label, &GENERAL_FONT_MID, 0);
        lv_label_set_long_mode(value_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_align(value_label, LV_ALIGN_LEFT_MID, LV_PCT(48), 0);
        lv_obj_set_width(value_label, LV_PCT(45));
    }

    return obj;
}

static void factory_test_system_info_event(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t *key = lv_event_get_param(e);
    lv_obj_t * cur_obj = lv_event_get_target(e);

    if (code == LV_EVENT_KEY) {
        if (*key == LV_KEY_BACK) {
            lv_obj_del(cur_obj);
            cur_obj = NULL;
            set_current_ui_group(factory_ui_group);
            factory_test_hdmi_start();
        }
    }
}

void factory_test_system_info_ui(void)
{
    lv_obj_t *sys_info_obj = lv_obj_create(lv_layer_top());
    lv_obj_set_size(sys_info_obj, LV_PCT(100), LV_PCT(100));
    lv_obj_add_style(sys_info_obj, &lv_pro_res.set_bg_grey, 0);

    lv_obj_t *left_obj = lv_obj_create(sys_info_obj);
    lv_obj_set_size(left_obj, LV_PCT(50), LV_PCT(100));
    lv_obj_add_style(left_obj, &lv_pro_res.set_bg_grey, 0);
    lv_obj_set_scrollbar_mode(left_obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_align(left_obj, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *SN_obj = lv_obj_create(left_obj);
    lv_obj_set_size(SN_obj, LV_PCT(100), 240);
    lv_obj_add_style(SN_obj, &lv_pro_res.set_bg_transp, 0);
    lv_obj_set_scrollbar_mode(SN_obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_align(SN_obj, LV_ALIGN_TOP_MID, 0, 35);

    char *SN_num = "xxxxxxx";
    lv_obj_t *SN_lable = lv_label_create(SN_obj);
    lv_obj_set_size(SN_lable, LV_PCT(100), 50);
    lv_obj_align(SN_lable, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_font(SN_lable, &GENERAL_FONT_BIG, 0);
    lv_label_set_text_fmt(SN_lable, "SN号 : %s", SN_num);
    lv_obj_set_style_text_align(SN_lable, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t * SN_qr = lv_qrcode_create(SN_obj, 150, COLOR_BLACK, COLOR_WHITE);
    lv_obj_align_to(SN_qr, SN_lable, LV_ALIGN_OUT_BOTTOM_MID, 0, 15);
    lv_qrcode_update(SN_qr, SN_num, strlen(SN_num));

    lv_obj_t *WIFI_MAC_obj = lv_obj_create(left_obj);
    lv_obj_set_size(WIFI_MAC_obj, LV_PCT(100), 240);
    lv_obj_add_style(WIFI_MAC_obj, &lv_pro_res.set_bg_transp, 0);
    lv_obj_set_scrollbar_mode(WIFI_MAC_obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_align_to(WIFI_MAC_obj, SN_obj, LV_ALIGN_OUT_BOTTOM_MID, 0, 35);

    char *WIFI_MAC_addr = "xxxxxxx";
    lv_obj_t *WIFI_MAC_lable = lv_label_create(WIFI_MAC_obj);
    lv_obj_set_size(WIFI_MAC_lable, LV_PCT(100), 50);
    lv_obj_align(WIFI_MAC_lable, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_font(WIFI_MAC_lable, &GENERAL_FONT_BIG, 0);
    lv_label_set_text_fmt(WIFI_MAC_lable, "Wi-Fi MAC地址 : %s", WIFI_MAC_addr);
    lv_obj_set_style_text_align(WIFI_MAC_lable, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t * WIFI_MAC_qr = lv_qrcode_create(WIFI_MAC_obj, 150, COLOR_BLACK, COLOR_WHITE);
    lv_obj_align_to(WIFI_MAC_qr, WIFI_MAC_lable, LV_ALIGN_OUT_BOTTOM_MID, 0, 15);
    lv_qrcode_update(WIFI_MAC_qr, WIFI_MAC_addr, strlen(WIFI_MAC_addr));

    lv_obj_t *activate_lable = lv_label_create(left_obj);
    lv_obj_set_size(activate_lable, LV_PCT(100), 50);
    lv_obj_align_to(activate_lable, WIFI_MAC_obj, LV_ALIGN_OUT_BOTTOM_MID, 0, 40);
    lv_obj_set_style_text_font(activate_lable, &GENERAL_FONT_BIG, 0);
    lv_label_set_text(activate_lable, "同屏激活信息 : xxxxxxx");
    lv_obj_set_style_text_align(activate_lable, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t *right_obj = lv_obj_create(sys_info_obj);
    lv_obj_set_size(right_obj, LV_PCT(50), LV_PCT(100));
    lv_obj_add_style(right_obj, &lv_pro_res.set_bg_grey, 0);
    lv_obj_align(right_obj, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_border_width(right_obj, 0, 0);
    lv_obj_set_style_pad_all(right_obj, 0, 0);
    lv_obj_set_style_pad_row(right_obj, 5, 0);
    lv_obj_set_style_pad_top(right_obj, 10, 0);
    lv_obj_set_style_pad_bottom(right_obj, 10, 0);
    lv_obj_set_style_radius(right_obj, 0, 0);
    lv_obj_set_style_shadow_width(right_obj, 0, 0);
    lv_obj_set_scrollbar_mode(right_obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(right_obj, LV_FLEX_FLOW_COLUMN);

    factory_create_text_value(right_obj, "产品型号", "Projector");
    factory_create_text_value(right_obj, "内核版本", "xxxxxxx");
    factory_create_text_value(right_obj, "版本号", "xxxxxxx");
    factory_create_text_value(right_obj, "序列号", "xxxxxxx");
    factory_create_text_value(right_obj, "Mac地址", "xxxxxxx");
    factory_create_text_value(right_obj, "Wi-Fi Mac地址", "xxxxxxx");
    factory_create_text_value(right_obj, "Flash", "xxxxxxx");
    factory_create_text_value(right_obj, "HDCP KEY", "xxxxxxx");
    factory_create_text_value(right_obj, "设备ID", "xxxxxxx");

    lv_group_only_add_one_obj(key_group, sys_info_obj);
    lv_obj_add_event_cb(sys_info_obj, factory_test_system_info_event, LV_EVENT_KEY, NULL);
    set_current_ui_group(key_group);
}

void factory_do_key_event(uint32_t lv_key)
{
    if (lv_key == LV_KEY_HOME) {
        printf("factory: LV_KEY_HOME\n");
    } else if (lv_key == LV_KEY_MENU) {
        create_ui_reset_factory_msgbox();
    } else if (lv_key == U_KEY_MUTE) {
        //factory_test_aging_ui();
        factory_test_hdmi_stop();
        switch_page(PAGE_MEDIA_STRESS);
    } else if (lv_key == U_KEY_VOLUME_UP) {
        factory_test_hdmi_stop();
        factory_test_system_info_ui();
    } else if (lv_key == U_KEY_VOLUME_DOWN) {
        create_ui_recovery_factory_msgbox();
    } else if (lv_key == U_KEY_FOCUS_UP) {
        lv_motor_forward(3);
        lens_focus_key_send_handler(U_KEY_FOCUS_UP);
    } else if (lv_key == U_KEY_FOCUS_DOWN) {
        lv_motor_reverse(3);
        lens_focus_key_send_handler(U_KEY_FOCUS_DOWN);
    } else if (lv_key == U_KEY_SOURCE) {
        factory_test_hdmi_stop();
        factory_test_image_ui();
    } else if (lv_key == U_KEY_POWER) {
        printf("factory: U_KEY_POWER\n");
    } else if (lv_key == LV_KEY_BACK) {
        printf("factory: LV_KEY_BACK\n");
    }
}

static void factory_activity_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    const uint32_t *key_param = lv_event_get_param(e);
    uint32_t key = (key_param != NULL) ? *key_param : 0;

    if (code == LV_EVENT_KEY) {
        factory_do_key_event(key);
    }
}

void factory_system_init(void)
{
    factory_test_hdmi_init(0, 0, HDMI_DISPLAY_W_RATIO, HDMI_DISPLAY_H_RATIO);
    factory_test_hdmi_start();
}

void factory_ui_init(void)
{
    char value[128];
    printf("go to factory_test_init\n");

    factory_ui = lv_obj_create(NULL);
    factory_ui_group = lv_group_create();
    lv_obj_add_event_cb(factory_ui, factory_activity_event_handler, LV_EVENT_KEY, NULL);
    lv_group_add_obj(factory_ui_group, factory_ui);

    lv_obj_set_size(factory_ui, LV_PCT(100), LV_PCT(100));
    lv_obj_set_align(factory_ui, LV_ALIGN_CENTER);
    lv_obj_clear_flag(factory_ui, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(factory_ui, COLOR_WHITE, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* HDMI test */
    hdmi_source_obj = lv_obj_create(factory_ui);
    lv_obj_set_width(hdmi_source_obj, LV_PCT(HDMI_DISPLAY_W_RATIO));
    lv_obj_set_height(hdmi_source_obj, LV_PCT(HDMI_DISPLAY_H_RATIO));
    lv_obj_set_align(hdmi_source_obj, LV_ALIGN_TOP_LEFT);
    lv_obj_set_style_bg_color(hdmi_source_obj, COLOR_BLUE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_scrollbar_mode(hdmi_source_obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(hdmi_source_obj, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *hdmi_lable = lv_label_create(hdmi_source_obj);
    lv_obj_set_width(hdmi_lable, LV_PCT(30));
    lv_obj_set_height(hdmi_lable, LV_PCT(10));
    lv_obj_set_align(hdmi_lable, LV_ALIGN_CENTER);
    lv_obj_set_style_text_font(hdmi_lable, &GENERAL_FONT_MID, 0);
    lv_obj_set_style_text_color(hdmi_lable, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(hdmi_lable, "视频没有信号");

    /* Test resutl */
    test_result_obj = lv_obj_create(factory_ui);
    lv_obj_set_width(test_result_obj, LV_PCT(40));
    lv_obj_set_height(test_result_obj, LV_PCT(15));
    lv_obj_set_y(test_result_obj, LV_PCT(35) + 1);
    lv_obj_set_align(test_result_obj, LV_ALIGN_TOP_LEFT);
    lv_obj_set_style_bg_color(test_result_obj, COLOR_RED, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(test_result_obj, LV_OBJ_FLAG_SCROLLABLE);

    test_result_lable = lv_label_create(test_result_obj);
    lv_obj_set_align(test_result_lable, LV_ALIGN_CENTER);
    lv_label_set_text(test_result_lable, "厂测NG");
    lv_obj_set_style_text_font(test_result_lable, &GENERAL_FONT_BIG, 0);

    /* UST Test */
    usb_text_lable = lv_label_create(factory_ui);
    lv_obj_set_width(usb_text_lable, LV_PCT(20));
    lv_obj_set_height(usb_text_lable, LV_PCT(6));
    lv_obj_align_to(usb_text_lable, test_result_obj, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_text_font(usb_text_lable, &GENERAL_FONT_MID, 0);
    lv_label_set_text(usb_text_lable, "USB测试:");

    usb_value_lable = lv_label_create(factory_ui);
    lv_obj_set_width(usb_value_lable, LV_PCT(20));
    lv_obj_set_height(usb_value_lable, LV_PCT(6));
    lv_obj_align_to(usb_value_lable, test_result_obj, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 0);
    lv_obj_set_style_text_font(usb_value_lable, &GENERAL_FONT_MID, 0);

    /* Gsensor Test */
    sensor_text_lable = lv_label_create(factory_ui);
    lv_obj_set_width(sensor_text_lable, LV_PCT(20));
    lv_obj_set_height(sensor_text_lable, LV_PCT(6));
    lv_obj_align_to(sensor_text_lable, usb_text_lable, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_text_font(sensor_text_lable, &GENERAL_FONT_MID, 0);
    lv_label_set_text(sensor_text_lable, "Sensor:");

    sensor_value_lable = lv_label_create(factory_ui);
    lv_obj_set_width(sensor_value_lable, LV_PCT(20));
    lv_obj_set_height(sensor_value_lable, LV_PCT(6));
    lv_obj_align_to(sensor_value_lable, usb_value_lable, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_text_font(sensor_value_lable, &GENERAL_FONT_MID, 0);
    lv_label_set_text(sensor_value_lable, "x, y, z");

    /* WIFI Test */
    wifi_test_text_lable = lv_label_create(factory_ui);
    lv_obj_set_width(wifi_test_text_lable, LV_PCT(20));
    lv_obj_set_height(wifi_test_text_lable, LV_PCT(6));
    lv_obj_align_to(wifi_test_text_lable, sensor_text_lable, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_text_font(wifi_test_text_lable, &GENERAL_FONT_MID, 0);
    lv_label_set_text(wifi_test_text_lable, "wifi压力测试:");

    wifi_test_value_lable = lv_label_create(factory_ui);
    lv_obj_set_width(wifi_test_value_lable, LV_PCT(20));
    lv_obj_set_height(wifi_test_value_lable, LV_PCT(6));
    lv_obj_align_to(wifi_test_value_lable, sensor_value_lable, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_text_font(wifi_test_value_lable, &GENERAL_FONT_MID, 0);

    /* BT Test */
    bt_test_text_lable = lv_label_create(factory_ui);
    lv_obj_set_width(bt_test_text_lable, LV_PCT(20));
    lv_obj_set_height(bt_test_text_lable, LV_PCT(6));
    lv_obj_align_to(bt_test_text_lable, wifi_test_text_lable, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_text_font(bt_test_text_lable, &GENERAL_FONT_MID, 0);
    lv_label_set_text(bt_test_text_lable, "蓝牙压力测试:");

    bt_test_value_lable = lv_label_create(factory_ui);
    lv_obj_set_width(bt_test_value_lable, LV_PCT(20));
    lv_obj_set_height(bt_test_value_lable, LV_PCT(6));
    lv_obj_align_to(bt_test_value_lable, wifi_test_value_lable, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_text_font(bt_test_value_lable, &GENERAL_FONT_MID, 0);

    /* Test time */
    test_time_text_lable = lv_label_create(factory_ui);
    lv_obj_set_width(test_time_text_lable, LV_PCT(20));
    lv_obj_set_height(test_time_text_lable, LV_PCT(6));
    lv_obj_align_to(test_time_text_lable, bt_test_text_lable, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_text_font(test_time_text_lable, &GENERAL_FONT_MID, 0);
    lv_label_set_text(test_time_text_lable, "老化时间:");

    test_time_value_lable = lv_label_create(factory_ui);
    lv_obj_set_width(test_time_value_lable, LV_PCT(20));
    lv_obj_set_height(test_time_value_lable, LV_PCT(6));
    lv_obj_align_to(test_time_value_lable, bt_test_value_lable, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_text_font(test_time_value_lable, &GENERAL_FONT_MID, 0);
    lv_label_set_text(test_time_value_lable, "12:34:56");

    /* Reboot time */
    reboot_count_text_lable = lv_label_create(factory_ui);
    lv_obj_set_width(reboot_count_text_lable, LV_PCT(20));
    lv_obj_set_height(reboot_count_text_lable, LV_PCT(6));
    lv_obj_align_to(reboot_count_text_lable, test_time_text_lable, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_text_font(reboot_count_text_lable, &GENERAL_FONT_MID, 0);
    lv_label_set_text(reboot_count_text_lable, "重启次数:");

    reboot_count_value_lable = lv_label_create(factory_ui);
    lv_obj_set_width(reboot_count_value_lable, LV_PCT(20));
    lv_obj_set_height(reboot_count_value_lable, LV_PCT(6));
    lv_obj_align_to(reboot_count_value_lable, test_time_value_lable, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_text_font(reboot_count_value_lable, &GENERAL_FONT_MID, 0);

    /* Version */
    memset(value, 0, sizeof(value));
    version_text_lable = lv_label_create(factory_ui);
    lv_obj_set_width(version_text_lable, LV_PCT(40));
    lv_obj_set_height(version_text_lable, LV_PCT(6));
    lv_obj_align_to(version_text_lable, reboot_count_text_lable, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_text_font(version_text_lable, &GENERAL_FONT_MID, 0);
    sprintf(value, "版本号：%u", lv_get_sys_param(P_VERSION_INFO));
    lv_label_set_text(version_text_lable, value);

#if ENABLE_WIFI
    /* Wifi Test */
    lv_obj_t *wifi_switch_cont = lv_obj_create(factory_ui);
    lv_obj_set_size(wifi_switch_cont, LV_PCT(27), LV_PCT(6));
    lv_obj_set_align(wifi_switch_cont, LV_ALIGN_TOP_LEFT);
    lv_obj_set_pos(wifi_switch_cont, LV_PCT(40) + 1, 7);
    lv_obj_set_style_bg_opa(wifi_switch_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_scrollbar_mode(wifi_switch_cont, LV_SCROLLBAR_MODE_OFF);

    wifi_lable = lv_label_create(wifi_switch_cont);
    lv_label_set_text(wifi_lable, "附件的wifi");
    lv_obj_set_style_text_font(wifi_lable, &GENERAL_FONT_MID, 0);
    lv_obj_align(wifi_lable, LV_ALIGN_LEFT_MID, 0, 0);

    wifi_switch = lv_switch_create(wifi_switch_cont);
    lv_obj_align(wifi_switch, LV_ALIGN_RIGHT_MID, 0, 0);

    wifi_list = lv_list_create(factory_ui);
    lv_obj_set_size(wifi_list, LV_PCT(27), LV_PCT(80));
    lv_obj_set_pos(wifi_list, LV_PCT(40) + 1, LV_PCT(6));
    lv_obj_set_align(wifi_list, LV_ALIGN_TOP_LEFT);
    lv_obj_set_style_border_width(wifi_list, 2, 0);
    lv_obj_set_style_border_color(wifi_list, COLOR_WHITE, 0);
    lv_obj_set_style_radius(wifi_list, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(wifi_list, COLOR_BLUE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(wifi_list, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(wifi_list, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_scrollbar_mode(wifi_list, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_text_font(wifi_list, &GENERAL_FONT_MID, LV_PART_MAIN);
    lv_obj_set_style_text_color(wifi_list, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(wifi_list, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(wifi_list, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
#endif

#if ENABLE_BT
    /* BT Test */
    lv_obj_t *bt_switch_cont = lv_obj_create(factory_ui);
    lv_obj_set_size(bt_switch_cont, LV_PCT(27), LV_PCT(6));
    lv_obj_set_align(bt_switch_cont, LV_ALIGN_TOP_LEFT);
    lv_obj_set_pos(bt_switch_cont, LV_PCT(67) + 2, 7);
    lv_obj_set_style_bg_opa(bt_switch_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_scrollbar_mode(bt_switch_cont, LV_SCROLLBAR_MODE_OFF);

    bt_lable = lv_label_create(bt_switch_cont);
    lv_label_set_text(bt_lable, "附件的蓝牙");
    lv_obj_set_style_text_font(bt_lable, &GENERAL_FONT_MID, 0);
    lv_obj_align(bt_lable, LV_ALIGN_LEFT_MID, 0, 0);

    bt_switch = lv_switch_create(bt_switch_cont);
    lv_obj_align(bt_switch, LV_ALIGN_RIGHT_MID, 0, 0);

    bt_list = lv_list_create(factory_ui);
    lv_obj_set_size(bt_list, LV_PCT(27), LV_PCT(80));
    lv_obj_set_pos(bt_list, LV_PCT(67) + 2, LV_PCT(6));
    lv_obj_set_align(bt_list, LV_ALIGN_TOP_LEFT);
    lv_obj_set_style_border_width(bt_list, 2, 0);
    lv_obj_set_style_border_color(bt_list, COLOR_WHITE, 0);
    lv_obj_set_style_radius(bt_list, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(bt_list, COLOR_BLUE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(bt_list, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(bt_list, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_scrollbar_mode(bt_list, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_text_font(bt_list, &GENERAL_FONT_MID, LV_PART_MAIN);
    lv_obj_set_style_text_color(bt_list, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(bt_list, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(bt_list, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
#endif

    lv_obj_update_layout(factory_ui);

    factory_test_timer = lv_timer_create(factory_test_timer_handler, 2000, NULL);

    load_current_channel(factory_ui, factory_ui_group);

#if ENABLE_WIFI
    /* wifi task */
    pthread_t wifi_thread;
    factory_wifi_info = malloc(sizeof(factory_wifi_info_t));
    factory_wifi_info->wifiTask_run = true;
    factory_wifi_info->autoONOFF = true;
    factory_wifi_info->wifi_state = false;
    factory_wifi_info->info = factory_info;
    factory_wifi_info->wifi_list = wifi_list;
    factory_wifi_info->wifi_switch = wifi_switch;
    if (pthread_create(&wifi_thread, NULL, factory_wifi_task, factory_wifi_info) != 0) {
        printf("Failed to create factory wifi task thread\n");
    }
    pthread_detach(wifi_thread);
#endif

#if ENABLE_BT
    /* bt task */
    pthread_t bt_thread;
    factory_bt_info = malloc(sizeof(factory_bt_info_t));
    factory_bt_info->btTask_run = true;
    factory_bt_info->autoONOFF = true;
    factory_bt_info->bt_state = false;
    factory_bt_info->info = factory_info;
    factory_bt_info->bt_list = bt_list;
    factory_bt_info->bt_switch = bt_switch;
    if (pthread_create(&bt_thread, NULL, factory_bt_task, factory_bt_info) != 0) {
        printf("Failed to create factory bt task thread\n");
    }
    pthread_detach(bt_thread);
#endif
}

static int create_factory_test(void)
{
    factory_test_flag = 1;

    if (!factory_info)
        factory_data_init();

    factory_system_init();
    factory_ui_init();
    update_factory_test_all_date();

    return 0;
}

static int destory_factory_test(void)
{
    factory_test_flag = 0;
    return 0;
}

static int show_factory_test(void) { return 0; }

static int hide_factory_test(void) { return 0; }

static page_interface_t page_factory_test = {
    .ops =
    {
        create_factory_test,
        destory_factory_test,
        show_factory_test,
        hide_factory_test,
    },
    .info =
    {
        .id = PAGE_FACTORY_TEST,
        .user_data = NULL
    }
};

void REGISTER_PAGE_FACTORY_TEST(void) { reg_page(&page_factory_test); }
