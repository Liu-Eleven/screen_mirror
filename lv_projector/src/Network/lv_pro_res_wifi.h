/*
 * lv_pro_res_wifi.h
 *
 *  Created on: 2024/09/20
 *      Author: JASON
 */

#ifndef LV_PRO_RES_WIFI_H_
#define LV_PRO_RES_WIFI_H_

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>

#include "../Layer/wifi_bt/lv_ui_wifi.h"
#include "lv_common.h"

#if ENABLE_WIFI

#ifdef LV_USE_LINUX_WIFI2_MODE
#include <wifimg.h>
#endif

#define BAND_NOME 0
#define BAND_2_4G 1
#define BAND_5G 2

#define SCAN_LIST_LEN 100

// 定义 RSSI 阈值
#define RSSI_HIGH_THRESHOLD -40    // 高信号强度阈值
#define RSSI_MEDIUM_THRESHOLD -60  // 中信号强度阈值
#define RSSI_LOW_THRESHOLD -80     // 低信号强度阈值

typedef enum {
    LVGL_WIFI_STATE_IDLE,
    LVGL_WIFI_STATE_BUSY,
    LVGL_WIFI_STATE_OPENING,
    LVGL_WIFI_STATE_CONNECTING,
    LVGL_WIFI_STATE_DISCONNECTING,
    LVGL_WIFI_STATE_PAIRING,
    LVGL_WIFI_STATE_UNPAIRING,
    LVGL_WIFI_STATE_CLOSING
} lv_pro_wifi_state_t;

typedef struct {
    char ssid[64];
    char bssid[18];
    int rssi;
    int conn_state;
    bool dev_found;
} wifi_paired_result_t;

extern wifi_scan_result_t wifi_scan_res[SCAN_LIST_LEN];

int lv_pro_res_wifi_init(void);
void lv_pro_res_wifi_deinit(void);
int lv_pro_res_wifi_on(void);
int lv_pro_res_wifi_ap_on(const char *ssid, const char *psk, int channel);
int lv_pro_res_wifi_off(void);
int lv_pro_res_wifi_scan(void);
int lv_pro_res_wifi_connect(char *ssid, const char *password, int sec);
int lv_pro_res_wifi_disconnect(void);
int lv_pro_res_wifi_sta_auto_conn(char *ssid);
int lv_pro_res_wifi_sta_unpair_networks(char *ssid);

void lv_pro_set_wifi_state(lv_pro_wifi_state_t new_state);
lv_pro_wifi_state_t lv_pro_get_wifi_state(void);
bool lv_pro_is_wifi_state_idle(void);
bool lv_pro_res_wifi_sta_is_connect(void);
void lv_pro_res_wifi_logo_show(bool conn);

int lv_pro_res_wifi_get_scan_results_num(void);
void lv_pro_res_wifi_set_scan_status_flag(int state);
int lv_pro_res_wifi_get_scan_status_flag(void);

bool is_ssid_valid(const char *ssid);
wifi_secure_t char_to_key_mgmt(const char *key_mgmt_str);
char *key_mgmt_to_char(char *key_mgmt_buf, wifi_secure_t key_mgmt);
int lv_pro_res_wifi_get_dev(const char *ssid, char *pwd, char *key_mgnt);
int lv_pro_res_wifi_get_security_type(wifi_secure_t security);
void lv_pro_res_wifi_uint8tochar(char *mac_addr_char, uint8_t *mac_addr_uint8);
const char *get_rssi_signal_strength(int rssi);

#endif /* ENABLE_WIFI */

#endif /* LV_PRO_RES_WIFI_H_ */
