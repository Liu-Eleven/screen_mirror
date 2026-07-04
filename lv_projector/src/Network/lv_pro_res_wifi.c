/*
 * lv_pro_res_wifi.c
 *
 *  Created on: 2024/09/20
 *      Author: JASON
 */

#include "lv_pro_res_wifi.h"

#include "lv_pro_wifi_common.h"

#if ENABLE_WIFI

#ifdef LV_USE_LINUX_WIFI2_MODE

#include <string.h>
#include <wifi_log.h>

wifi_scan_result_t wifi_scan_res[SCAN_LIST_LEN];
static int wifi_scan_complete_flag;
static int scan_results_num;

static pthread_mutex_t wifi_res_mutex = PTHREAD_MUTEX_INITIALIZER;
static lv_pro_wifi_state_t current_wifi_state = LVGL_WIFI_STATE_IDLE;

void lv_pro_set_wifi_state(lv_pro_wifi_state_t new_state)
{
    pthread_mutex_lock(&wifi_res_mutex);
    current_wifi_state = new_state;
    pthread_mutex_unlock(&wifi_res_mutex);
}

lv_pro_wifi_state_t lv_pro_get_wifi_state(void)
{
    lv_pro_wifi_state_t state;
    pthread_mutex_lock(&wifi_res_mutex);
    state = current_wifi_state;
    pthread_mutex_unlock(&wifi_res_mutex);
    return state;
}

bool lv_pro_is_wifi_state_idle(void)
{
    bool idle;
    pthread_mutex_lock(&wifi_res_mutex);
    idle = (current_wifi_state == LVGL_WIFI_STATE_IDLE);
    pthread_mutex_unlock(&wifi_res_mutex);
    return idle;
}

const char *get_rssi_signal_strength(int rssi)
{
    const char *signal_strength;

    //  signal_strength = lv_get_string(STR_WIFI_CONN);
    if (rssi >= RSSI_HIGH_THRESHOLD) {
        signal_strength = "High";
    } else if (rssi >= RSSI_MEDIUM_THRESHOLD) {
        signal_strength = "Medium";
    } else if (rssi >= RSSI_LOW_THRESHOLD) {
        signal_strength = "Low";
    } else {
        signal_strength = "No Signal";
    }

    return signal_strength;
}

static uint32_t freq_to_channel(uint32_t freq)
{
    int band;
    uint32_t channel = 0;
    if ((freq <= 4980) && (freq >= 4910)) {
        band = BAND_5G;
    } else if ((freq >= 2407) && (freq <= 2484)) {
        band = BAND_2_4G;
    } else {
        band = BAND_NOME;
    }
    switch (band) {
    case BAND_2_4G:
        if (freq == 2484) {
            channel = 14;
        } else if (freq == 2407) {
            channel = 0;
        } else if ((freq <= 2472) && (freq > 2407)) {
            if (((freq - 2407) % 5) == 0) {
                channel = ((freq - 2407) / 5);
            } else {
                channel = 1000;
            }
        } else {
            channel = 1000;
        }
        break;
    case BAND_5G:
        if (((freq - 4000) % 5) == 0) {
            channel = ((freq - 4000) / 5);
        } else {
            channel = 1000;
        }
        break;
    case BAND_NOME:
    default:
        channel = 1000;
        break;
    }
    return channel;
}

char *key_mgmt_to_char(char *key_mgmt_buf, wifi_secure_t key_mgmt)
{
    if (key_mgmt & WIFI_SEC_WEP) {
        strcat(key_mgmt_buf, "[WEP]");
    }
    if (key_mgmt & WIFI_SEC_WPA_PSK) {
        strcat(key_mgmt_buf, "[WPA]");
    }
    if (key_mgmt & WIFI_SEC_WPA2_PSK) {
        strcat(key_mgmt_buf, "[WPA2]");
    }
    if (key_mgmt & WIFI_SEC_WPA2_PSK_SHA256) {
        strcat(key_mgmt_buf, "[WPA2-SHA]");
    }
    if (key_mgmt & WIFI_SEC_WPA3_PSK) {
        strcat(key_mgmt_buf, "[WPA3]");
    }
    if (key_mgmt & WIFI_SEC_EAP) {
        strcat(key_mgmt_buf, "[EAP]");
    }
    if (key_mgmt == WIFI_SEC_NONE) {
        strcat(key_mgmt_buf, "[NONE]");
    }
    return key_mgmt_buf;
}

int lv_pro_res_wifi_get_security_type(wifi_secure_t security)
{
    if (security & WIFI_SEC_WPA3_PSK) {
        return WIFI_SEC_WPA3_PSK;
    } else if (security & WIFI_SEC_WPA2_PSK_SHA256) {
        return WIFI_SEC_WPA2_PSK_SHA256;
    } else if (security & WIFI_SEC_WPA2_PSK) {
        return WIFI_SEC_WPA2_PSK;
    } else if (security & WIFI_SEC_WPA_PSK) {
        return WIFI_SEC_WPA_PSK;
    } else if (security & WIFI_SEC_WEP) {
        return WIFI_SEC_WEP;
    } else if (security & WIFI_SEC_EAP) {
        return WIFI_SEC_EAP;
    } else {
        return WIFI_SEC_NONE;
    }
}

wifi_secure_t char_to_key_mgmt(const char *key_mgmt_str)
{
    if (strcmp(key_mgmt_str, "WEP") == 0) {
        return WIFI_SEC_WEP;
    } else if (strcmp(key_mgmt_str, "WPA-PSK") == 0) {
        return WIFI_SEC_WPA_PSK;
    } else if (strcmp(key_mgmt_str, "WPA2-PSK") == 0) {
        return WIFI_SEC_WPA2_PSK;
    } else if (strcmp(key_mgmt_str, "WPA2-PSK-SHA256") == 0) {
        return WIFI_SEC_WPA2_PSK_SHA256;
    } else if (strcmp(key_mgmt_str, "WPA3-PSK") == 0) {
        return WIFI_SEC_WPA3_PSK;
    } else if (strcmp(key_mgmt_str, "SAE") == 0) {
        return WIFI_SEC_WPA3_PSK;
    } else if (strcmp(key_mgmt_str, "EAP") == 0) {
        return WIFI_SEC_EAP;
    } else {
        return WIFI_SEC_NONE;
    }
}

static void wifi_msg_cb(wifi_msg_data_t *msg)
{
    char msg_cb_arg[32] = "NULL";
    char *msg_cb_arg_p;
    // msg cb test, need to pass static parameters
    if (msg->private_data) {
        msg_cb_arg_p = (char *)msg->private_data;
    } else {
        msg_cb_arg_p = msg_cb_arg;
    }
    WIFI_DBG("** wmg cb: ");
    switch (msg->id) {
    case WIFI_MSG_ID_DEV_STATUS:
        printf("dev ");
        switch (msg->data.d_status) {
        case WLAN_STATUS_DOWN:
            printf("down ");
            break;
        case WLAN_STATUS_UP:
            printf("up ");
            lv_pro_wifi_freshui_ack();
            break;
        default:
            printf("unknow ");
            break;
        }
        printf("arg:%s\n", msg_cb_arg_p);
        break;
    case WIFI_MSG_ID_STA_CN_EVENT:
        printf("sta cn event ");
        switch (msg->data.event) {
        case WIFI_DISCONNECTED:
            printf("disconnect ");
            lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_STATE_DISCONNED, NULL, false);
            lv_pro_res_wifi_logo_show(false);
            lv_pro_wifi_freshui_ack();
            break;
        case WIFI_SCAN_STARTED:
            printf("scan started ");
            break;
        case WIFI_SCAN_FAILED:
            printf("scan failed ");
            break;
        case WIFI_SCAN_RESULTS:
            printf("scan results ");
            break;
        case WIFI_NETWORK_NOT_FOUND:
            printf("network not found ");
            break;
        case WIFI_PASSWORD_INCORRECT:
            printf("password incorrect ");
            break;
        case WIFI_AUTHENTIACATION:
            printf("authentiacation ");
            break;
        case WIFI_AUTH_REJECT:
            printf("auth reject ");
            break;
        case WIFI_ASSOCIATING:
            printf("associating ");
            break;
        case WIFI_ASSOC_REJECT:
            printf("assoc reject ");
            break;
        case WIFI_ASSOCIATED:
            printf("associated ");
            break;
        case WIFI_4WAY_HANDSHAKE:
            printf("4 way handshake ");
            break;
        case WIFI_GROUNP_HANDSHAKE:
            printf("grounp handshake ");
            break;
        case WIFI_GROUNP_HANDSHAKE_DONE:
            printf("handshake done ");
            break;
        case WIFI_CONNECTED:
            printf("connected ");
            /* 降低刷新频率，当DCHP完成再刷新 */
            // lv_pro_wifi_freshui_ack();
            break;
        case WIFI_CONNECT_TIMEOUT:
            printf("connect timeout ");
            lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_STATE_DISCONNED, NULL, false);
            lv_pro_res_wifi_logo_show(false);
            lv_pro_wifi_freshui_ack();
            break;
        case WIFI_DEAUTH:
            printf("deauth ");
            break;
        case WIFI_DHCP_START:
            printf("dhcp start ");
            break;
        case WIFI_DHCP_TIMEOUT:
            printf("dhcp timeout ");
            lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_STATE_DISCONNED, NULL, false);
            lv_pro_res_wifi_logo_show(false);
            lv_pro_wifi_freshui_ack();
            break;
        case WIFI_DHCP_SUCCESS:
            printf("dhcp success ");
            lv_pro_wifi_msg_enqueue(MSG_TYPE_WIFI_STATE_CONNECTED, NULL, false);
            lv_pro_res_wifi_logo_show(true);
            lv_pro_wifi_freshui_ack();
            break;
        case WIFI_TERMINATING:
            printf("terminating ");
            break;
        case WIFI_UNKNOWN:
        default:
            printf("unknow ");
            break;
        }
        printf("arg:%s\n", msg_cb_arg_p);
        break;
    case WIFI_MSG_ID_STA_STATE_CHANGE:
        printf("sta state ");
        switch (msg->data.state) {
        case WIFI_STA_IDLE:
            printf("idle ");
            break;
        case WIFI_STA_CONNECTING:
            printf("connecting ");
            break;
        case WIFI_STA_CONNECTED:
            printf("connected ");
            break;
        case WIFI_STA_OBTAINING_IP:
            printf("obtaining ip ");
            break;
        case WIFI_STA_NET_CONNECTED:
            printf("net connected ");
            break;
        case WIFI_STA_DHCP_TIMEOUT:
            printf("dhcp timeout ");
            break;
        case WIFI_STA_DISCONNECTING:
            printf("disconnecting ");
            break;
        case WIFI_STA_DISCONNECTED:
            printf("disconnected ");
            break;
        default:
            printf("unknow ");
            break;
        }
        printf("arg:%s\n", msg_cb_arg_p);
        break;
    case WIFI_MSG_ID_AP_CN_EVENT:
        printf("ap cn");
        switch (msg->data.ap_event) {
        case WIFI_AP_ENABLED:
            printf("enable ");
            break;
        case WIFI_AP_DISABLED:
            printf("disable ");
            break;
        case WIFI_AP_STA_DISCONNECTED:
            printf("sta disconnected ");
            break;
        case WIFI_AP_STA_CONNECTED:
            printf("sta connected ");
            break;
        case WIFI_AP_UNKNOWN:
        default:
            printf("event unknow ");
            break;
        }
        printf("arg:%s\n", msg_cb_arg_p);
        break;
    case WIFI_MSG_ID_MONITOR:
        printf("monitor ");
        switch (msg->data.mon_state) {
        case WIFI_MONITOR_DISABLE:
            printf("disable arg:%s\n", msg_cb_arg_p);
            break;
        case WIFI_MONITOR_ENABLE:
            printf("enable arg:%s\n", msg_cb_arg_p);
            break;
        default:
            printf("frame arg:%s\ndata channel %d\ndata len %d\n", msg_cb_arg_p,
                   msg->data.frame->channel, msg->data.frame->len);
            break;
        }
        break;
    case WIFI_MSG_ID_P2P_CN_EVENT:
        printf("p2p cn event ");
        switch (msg->data.event) {
        case WIFI_P2P_DEV_FOUND:
            printf("dev found ");
            break;
        case WIFI_P2P_DEV_LOST:
            printf("dev lost ");
            break;
        case WIFI_P2P_PBC_REQ:
            printf("pbc req ");
            break;
        case WIFI_P2P_GO_NEG_RQ:
            printf("go neg rq ");
            break;
        case WIFI_P2P_GO_NEG_SUCCESS:
            printf("go neg success ");
            break;
        case WIFI_P2P_GO_NEG_FAILURE:
            printf("go neg failure ");
            break;
        case WIFI_P2P_GROUP_FOR_SUCCESS:
            printf("group for success ");
            break;
        case WIFI_P2P_GROUP_FOR_FAILURE:
            printf("group for failure ");
            break;
        case WIFI_P2P_GROUP_STARTED:
            printf("group started ");
            break;
        case WIFI_P2P_GROUP_REMOVED:
            printf("group remove ");
            break;
        case WIFI_P2P_CROSS_CONNECT_ENABLE:
            printf("cross connect enable ");
            break;
        case WIFI_P2P_CROSS_CONNECT_DISABLE:
            printf("cross connect disable ");
            break;
        case WIFI_P2P_SCAN_STARTED:
            printf("scan started ");
            break;
        case WIFI_P2P_SCAN_RESULTS:
            printf("scan results ");
            break;
        case WIFI_P2P_GROUP_DHCP_DNS_FAILURE:
            printf("group dhcp dns failure ");
            break;
        case WIFI_P2P_GROUP_DHCP_SUCCESS:
            printf("group dhcp success ");
            break;
        case WIFI_P2P_GROUP_DHCP_FAILURE:
            printf("group dhcp failure ");
            break;
        case WIFI_P2P_GROUP_DNS_SUCCESS:
            printf("group dns success ");
            break;
        case WIFI_P2P_GROUP_DNS_FAILURE:
            printf("group dns failure ");
            break;
        case WIFI_P2P_UNKNOWN:
        default:
            printf("unknow ");
            break;
        }
        printf("arg:%s\n", msg_cb_arg_p);
        break;
    case WIFI_MSG_ID_P2P_STATE_CHANGE:
        printf("p2p state ");
        switch (msg->data.state) {
        case WIFI_P2P_ENABLE:
            printf("enable ");
            break;
        case WIFI_P2P_DISABLE:
            printf("disable ");
            break;
        case WIFI_P2P_CONNECTD_GC:
            printf("connected gc");
            break;
        case WIFI_P2P_CONNECTD_GO:
            printf("connected go");
            break;
        case WIFI_P2P_DISCONNECTD:
            printf("disconnected ");
            break;
        default:
            printf("unknow ");
            break;
        }
        printf("arg:%s\n", msg_cb_arg_p);
        break;
    default:
        break;
    }
}

int lv_pro_res_wifi_init(void)
{
    wifi_scan_complete_flag = 0;
    wmg_set_debug_level(4);
    return wifimanager_init();
}

void lv_pro_res_wifi_deinit(void)
{
    wifimanager_deinit();
    wifi_scan_complete_flag = 0;
}

int lv_pro_res_wifi_on(void)
{
    static char sta_and_p2p_msg_cb_char[64] = "sta and p2p mode msg cb arg test";

    int ret = wifi_on(WIFI_STATION_P2P);
    if (ret == 0) {
        ret = wifi_register_msg_cb(wifi_msg_cb, (void *)sta_and_p2p_msg_cb_char);
        if (ret) {
            WIFI_ERR("register msg cb failed\n");
            return ret;  // 返回注册函数的错误代码
        }
        wifi_sta_auto_reconnect(true);
        return 0;  // 返回成功
    }

    return ret;  // 返回 wifi_on 的错误代码
}

int lv_pro_res_wifi_ap_on(const char *ssid, const char *psk, int channel)
{
    static char ap_msg_cb_char[32] = "ap msg cb arg";

    char ssid_buf[SSID_MAX_LEN + 1];
    char psk_buf[PSK_MAX_LEN + 1];
    wifi_ap_config_t ap_config;

    // 清空缓冲区
    memset(ssid_buf, '\0', sizeof(ssid_buf));
    memset(psk_buf, '\0', sizeof(psk_buf));

    // 复制 SSID 和 PSK
    strncpy(ssid_buf, ssid, SSID_MAX_LEN);
    strncpy(psk_buf, psk, PSK_MAX_LEN);

    // 配置 AP
    ap_config.ssid = ssid_buf;
    ap_config.psk = psk_buf;
    ap_config.sec = WIFI_SEC_WPA2_PSK;
    ap_config.channel = channel;

    // 开启 AP 模式
    int ret = wifi_on(WIFI_AP);
    if (ret != 0) {
        WIFI_ERR("wifi on ap failed\n");
        return ret;  // 返回 wifi_on 的错误代码
    }

    // 注册消息回调
    ret = wifi_register_msg_cb(wifi_msg_cb, (void *)ap_msg_cb_char);
    if (ret) {
        WIFI_ERR("register msg cb failed\n");
        return ret;  // 返回注册失败的错误代码
    }

    // 启用 AP
    ret = wifi_ap_enable(&ap_config);
    if (ret == 0) {
        WIFI_INF("ap enable success, ssid=%s, psk=%s, sec=%d, channel=%d\n", ssid_buf, psk_buf,
                 ap_config.sec, ap_config.channel);
    } else {
        WIFI_ERR("ap enable failed\n");
    }

    return ret;  // 返回最后的 ret 值
}

int lv_pro_res_wifi_ap_off() { return wifi_ap_disable(); }

int lv_pro_res_wifi_off(void) { return wifi_off(WIFI_STATION_P2P); }

void lv_pro_res_wifi_uint8tochar(char *mac_addr_char, uint8_t *mac_addr_uint8)
{
    sprintf(mac_addr_char, "%02x:%02x:%02x:%02x:%02x:%02x", (unsigned char)mac_addr_uint8[0],
            (unsigned char)mac_addr_uint8[1], (unsigned char)mac_addr_uint8[2],
            (unsigned char)mac_addr_uint8[3], (unsigned char)mac_addr_uint8[4],
            (unsigned char)mac_addr_uint8[5]);
    mac_addr_char[17] = '\0';
}

#define MAX_SSID_ENTRIES SCAN_LIST_LEN

typedef struct {
    uint8_t bssid[6];
    bool used;
    char ssid[33];  // assuming SSID length is up to 32 characters + null terminator
} ssid_entry_t;

ssid_entry_t ssid_table[MAX_SSID_ENTRIES];

static void ssid_table_init()
{
    for (int i = 0; i < MAX_SSID_ENTRIES; i++) {
        ssid_table[i].used = false;
    }
}

static bool ssid_exists(const char *ssid)
{
    for (int i = 0; i < MAX_SSID_ENTRIES; i++) {
        if (ssid_table[i].used && strcmp(ssid_table[i].ssid, ssid) == 0) {
            return true;
        }
    }
    return false;
}

static void ssid_add(const uint8_t *bssid, const char *ssid)
{
    for (int i = 0; i < MAX_SSID_ENTRIES; i++) {
        if (!ssid_table[i].used) {
            memcpy(ssid_table[i].bssid, bssid, 6);
            strncpy(ssid_table[i].ssid, ssid, 32);
            ssid_table[i].ssid[32] = '\0';  // Ensure null-termination
            ssid_table[i].used = true;
            return;
        }
    }
}

bool is_ssid_valid(const char *ssid)
{
    if (ssid == NULL || strlen(ssid) == 0) {
        return true;  // 字符串为空
    }

    for (size_t i = 0; i < strlen(ssid); i++) {
        if (!isspace((unsigned char)ssid[i])) {
            return false;  // 找到非空格字符，返回无效
        }
    }

    return true;  // 字符串全是空格
}

static int compare_conn_state(const void *a, const void *b)
{
    const wifi_paired_result_t *deviceA = *(const wifi_paired_result_t **)a;
    const wifi_paired_result_t *deviceB = *(const wifi_paired_result_t **)b;
    return deviceB->conn_state - deviceA->conn_state;  // conn_state 为 1 的排在前面
}

int lv_pro_res_wifi_paired_devices(wifi_paired_result_t **devices, int *scan_count)
{
    pthread_mutex_lock(&wifi_res_mutex);
    wmg_status_t ret;
    wifi_sta_list_t sta_list_networks;
    wifi_sta_list_nod_t sta_list_nod[MAX_SSID_ENTRIES] = {0};
    int entry = 0, cnt = 0;

    sta_list_networks.list_nod = sta_list_nod;
    sta_list_networks.list_num = MAX_SSID_ENTRIES;

    ret = wifi_sta_list_networks(&sta_list_networks);
    if (ret == 0) {
        entry = sta_list_networks.list_num >= sta_list_networks.sys_list_num
                    ? sta_list_networks.sys_list_num
                    : sta_list_networks.list_num;
        if (entry > 0) {
            for (cnt = 0; cnt < entry; cnt++) {
                wifi_paired_result_t *tmp =
                    (wifi_paired_result_t *)malloc(sizeof(wifi_paired_result_t));
                if (tmp == NULL) {
                    WIFI_DBG("%s: tmp malloc err\n", __func__);
                    pthread_mutex_unlock(&wifi_res_mutex);
                    return -1;
                }

                // WIFI_DBG("%s %d\t%s\t%s\t%s\n", __func__, sta_list_networks.list_nod[cnt].id,
                //          sta_list_networks.list_nod[cnt].ssid,
                //          sta_list_networks.list_nod[cnt].bssid,
                //          sta_list_networks.list_nod[cnt].flags);

                tmp->conn_state = 0;  // 默认连接状态
                char *__ssid = sta_list_networks.list_nod[cnt].ssid;
                char *__bssid = sta_list_networks.list_nod[cnt].bssid;
                memset(tmp->ssid, '\0', sizeof(tmp->ssid));
                memset(tmp->bssid, '\0', sizeof(tmp->bssid));
                strncpy(tmp->ssid, __ssid, strlen(__ssid));
                strncpy(tmp->bssid, __bssid, strlen(__bssid));
                tmp->rssi = 0;
                devices[cnt] = tmp;
            }
        } else {
            WIFI_DBG("System has no entry saved\n");
        }
    }

    if (cnt > 0) {
        wifi_sta_info_t wifi_sta_info;
        memset(&wifi_sta_info, 0, sizeof(wifi_sta_info_t));
        ret = wifi_sta_get_info(&wifi_sta_info);
        if (ret == 0) {
            // WIFI_DBG("connect sta ssid: %s\n", wifi_sta_info.ssid);
            for (int i = 0; i < cnt; i++) {
                if (strcmp(wifi_sta_info.ssid, devices[i]->ssid) == 0) {
                    devices[i]->conn_state = 1;  // 设置当前连接状态
                    break;
                }
            }
        }
    }

    *scan_count = cnt;

    // 排序 devices 根据 conn_state
    if (cnt > 0) {
        qsort(devices, cnt, sizeof(wifi_paired_result_t *), compare_conn_state);
    }
    pthread_mutex_unlock(&wifi_res_mutex);
    return 0;
}

// 比较函数，用于 qsort
static int compare_rssi(const void *a, const void *b)
{
    const wifi_scan_result_t *result_a = (const wifi_scan_result_t *)a;
    const wifi_scan_result_t *result_b = (const wifi_scan_result_t *)b;
    return result_b->rssi - result_a->rssi;  // 从高到低排序
}

int lv_pro_res_wifi_scan(void)
{
    char scan_res_char[32] = {0};
    char mac_addr_char[18] = {0};

    for (int i = 0; i < SCAN_LIST_LEN; i++) {
        memset(&wifi_scan_res[i], 0, sizeof(wifi_scan_result_t));
    }

    int i, bss_num = 0, ret = 0;
    ret = wifi_get_scan_results(wifi_scan_res, NULL, &bss_num, SCAN_LIST_LEN);
    if (ret == 0) {
        scan_results_num = (SCAN_LIST_LEN > bss_num ? bss_num : SCAN_LIST_LEN);

        for (i = 0; i < scan_results_num; i++) {
            memset(scan_res_char, '\0', sizeof(scan_res_char));
            memset(mac_addr_char, '\0', sizeof(mac_addr_char));
            lv_pro_res_wifi_uint8tochar(mac_addr_char, wifi_scan_res[i].bssid);
            // WIFI_DBG(
            //     "[%02d]: bssid=%s  channel=%d  freq=%d  rssi=%d  "
            //     "sec=%-15s  ssid=%s\n",
            //     i, mac_addr_char, freq_to_channel(wifi_scan_res[i].freq), wifi_scan_res[i].freq,
            //     wifi_scan_res[i].rssi, key_mgmt_to_char(scan_res_char,
            //     wifi_scan_res[i].key_mgmt), wifi_scan_res[i].ssid);
        }

        ssid_table_init();

        int unique_count = 0;
        for (i = 0; i < scan_results_num; i++) {
            if (!ssid_exists(wifi_scan_res[i].ssid)) {
                ssid_add(wifi_scan_res[i].bssid, wifi_scan_res[i].ssid);
                wifi_scan_result_t *result = &wifi_scan_res[unique_count];
                memcpy(result, &wifi_scan_res[i], sizeof(wifi_scan_result_t));
                unique_count++;
            }
        }
        scan_results_num = unique_count;

        // 对扫描结果根据 rssi 从高到低进行排序
        qsort(wifi_scan_res, scan_results_num, sizeof(wifi_scan_result_t), compare_rssi);

        for (i = 0; i < scan_results_num; i++) {
            memset(scan_res_char, 0, sizeof(scan_res_char));
            memset(mac_addr_char, 0, sizeof(mac_addr_char));
            lv_pro_res_wifi_uint8tochar(mac_addr_char, wifi_scan_res[i].bssid);
            memset(scan_res_char, '\0', sizeof(scan_res_char));
            // WIFI_INF(
            //     "[%02d]: bssid=%s  channel=%d  freq=%d  rssi=%d  "
            //     "sec=%-15s  ssid=%s\n",
            //     i, mac_addr_char, freq_to_channel(wifi_scan_res[i].freq), wifi_scan_res[i].freq,
            //     wifi_scan_res[i].rssi, key_mgmt_to_char(scan_res_char,
            //     wifi_scan_res[i].key_mgmt), wifi_scan_res[i].ssid);
        }
        WIFI_INF("===Wi-Fi scan successful, total %d ap(buff size: %d) time %4f ms===\n",
                 scan_results_num, SCAN_LIST_LEN, 0);

        /*Reduce the number of pairing list refreshes to prevent frequent refreshes and flashes, and
         * optimize the experience*/
#if 0
        lv_pro_res_wifi_set_scan_status_flag(1);
        lv_pro_wifi_freshui_ack();
#endif
        return 0;
    } else {
        scan_results_num = 0;
        WIFI_WRN("===Wi-Fi scan failed, time %4f ms===\n", 0);
        lv_pro_res_wifi_set_scan_status_flag(0);
        return -1;
    }
}

int lv_pro_res_wifi_get_scan_results_num(void)
{
    int res = 0;
    pthread_mutex_lock(&wifi_res_mutex);
    res = scan_results_num;
    pthread_mutex_unlock(&wifi_res_mutex);
    return res;
}

void lv_pro_res_wifi_set_scan_status_flag(int state)
{
    pthread_mutex_lock(&wifi_res_mutex);
    wifi_scan_complete_flag = state;
    pthread_mutex_unlock(&wifi_res_mutex);
}

int lv_pro_res_wifi_get_scan_status_flag(void)
{
    int res = 0;
    pthread_mutex_lock(&wifi_res_mutex);
    res = wifi_scan_complete_flag;
    pthread_mutex_unlock(&wifi_res_mutex);
    return res;
}

static wifi_secure_t scan_get_secure(char *ssid_bssid, int scan_time, bool ssid_connect)
{
    int i, bss_num = 0, get_scan_match_results = 0, get_scan_results_num = 0;
    wifi_scan_result_t scan_res[SCAN_LIST_LEN] = {0};
    char scan_res_char[32] = {0};
    wifi_secure_t get_scan_sec = WIFI_SEC_NONE;
    char bssid_char[18] = {0};

    for (; (scan_time > 0) && !get_scan_match_results; scan_time--) {
#ifdef OS_NET_FREERTOS_OS
        if (scan_time == 1) {
            scan_res->scan_action = true;
        } else {
            scan_res->scan_action = false;
        }
#endif
        if (!wifi_get_scan_results(scan_res, ssid_bssid, &bss_num, SCAN_LIST_LEN)) {
            get_scan_results_num = (SCAN_LIST_LEN > bss_num ? bss_num : SCAN_LIST_LEN);
            WIFI_DBG("==The %d time scan success, get_scan_results_num %d==\n", scan_time,
                     get_scan_results_num);
            for (i = 0; i < get_scan_results_num; i++) {
                WIFI_DBG("==The %d time scan success, ssid: %s sec(%s)==\n", scan_time,
                         scan_res[i].ssid, key_mgmt_to_char(scan_res_char, scan_res[i].key_mgmt));
                memset(scan_res_char, 0, 32);
                if (ssid_connect) {
                    if (!strcmp(scan_res[i].ssid, ssid_bssid)) {
                        WIFI_DBG("==The first get scan results success %s = %s sec(%s)==\n",
                                 scan_res[i].ssid, ssid_bssid,
                                 key_mgmt_to_char(scan_res_char, scan_res[i].key_mgmt));
                        get_scan_match_results = 1;
                    }
                } else {
                    memset(bssid_char, 0, 18);
                    uint8tochar(bssid_char, scan_res[i].bssid);
                    if (!strcmp(bssid_char, ssid_bssid)) {
                        get_scan_match_results = 1;
                    }
                }
                if (get_scan_match_results) {
                    if (scan_res[i].key_mgmt == WIFI_SEC_NONE) {
                        get_scan_match_results = 0;
                        WIFI_WRN("== Match %s result without psk ==\n", ssid_bssid);
                    }
                    if ((scan_res[i].key_mgmt) & WIFI_SEC_WPA_PSK) {
                        get_scan_sec = WIFI_SEC_WPA_PSK;
                    }
                    if ((scan_res[i].key_mgmt) & WIFI_SEC_WPA2_PSK) {
                        get_scan_sec = WIFI_SEC_WPA2_PSK;
                    }
                    if ((scan_res[i].key_mgmt) & WIFI_SEC_WPA2_PSK_SHA256) {
                        get_scan_sec = WIFI_SEC_WPA2_PSK_SHA256;
                    }
                    if ((scan_res[i].key_mgmt) & WIFI_SEC_WPA3_PSK) {
                        get_scan_sec = WIFI_SEC_WPA3_PSK;
                    }
                    if ((scan_res[i].key_mgmt) & WIFI_SEC_WEP) {
                        get_scan_sec = WIFI_SEC_WEP;
                    }
                    break;
                }
            }
        } else {
            WIFI_ERR("==The %d time scan action failed!==\n", scan_time);
        }
    }

    /* judeg get connect ap info success or failed. */
    if (get_scan_match_results) {
        memset(scan_res_char, 0, 32);
        if (ssid_connect)
            WIFI_DBG("==Wi-Fi scan ssid: %s (sec%s)==\n", ssid_bssid,
                     key_mgmt_to_char(scan_res_char, get_scan_sec));
        else
            WIFI_DBG("==Wi-Fi scan bssid: %s (sec%s)==\n", ssid_bssid,
                     key_mgmt_to_char(scan_res_char, get_scan_sec));
    } else {
        WIFI_WRN("==Wi-Fi can't scan ssid or bssid :%s==\n", ssid_bssid);
        WIFI_WRN("==scan_results: %d, scan_max_num: %d==\n", bss_num, SCAN_LIST_LEN);
        // get_scan_sec = -1;
    }
    return get_scan_sec;
}

int lv_pro_res_wifi_connect(char *ssid, const char *password, int sec)
{
    wifi_sta_cn_para_t cn_para = {0};
    if (sec == -1) {
        cn_para.sec = scan_get_secure(ssid, 4, true);
    } else {
        cn_para.sec = (wifi_secure_t)sec;
    }
    cn_para.ssid = ssid;
    if (cn_para.sec != WIFI_SEC_NONE) cn_para.password = password;
    cn_para.fast_connect = 0;

    // 打印调试信息
    WIFI_DBG("Device Name: %s, len=%zu\n", ssid, strlen(ssid));
    WIFI_DBG("Device Password: %s len=%zu\n", password, strlen(password));
    WIFI_DBG("Device SEC: %d\n", cn_para.sec);

    // 尝试连接 Wi-Fi
    int ret = wifi_sta_connect(&cn_para);

    if (ret == 0) {
        WIFI_INF("=== Wi-Fi connect use sec(%d) ===\n", cn_para.sec);
        WIFI_INF("=== Wi-Fi connect successful, time %4f ms ===\n", 0.0);  // 根据实际情况替换时间
        return 0;                                                          // 成功返回 0
    }

    // 连接失败，返回错误代码
    return ret;
}

int lv_pro_res_wifi_disconnect(void) { return wifi_sta_disconnect(); }

int lv_pro_res_wifi_sta_auto_conn(char *ssid) { return wifi_sta_auto_connect(ssid); }

int lv_pro_res_wifi_sta_unpair_networks(char *ssid) { return wifi_sta_remove_networks(ssid); }

bool lv_pro_res_wifi_sta_is_connect()
{
    wifi_sta_info_t wifi_sta_info;
    memset(&wifi_sta_info, 0, sizeof(wifi_sta_info_t));
    int ret = wifi_sta_get_info(&wifi_sta_info);
    return (ret == 0);
}

void lv_pro_res_wifi_logo_show(bool conn)
{
    char *dev_name = (char *)malloc(SSID_MAX_LEN);
    if (dev_name == NULL) {
        WIFI_ERR("Error: Failed to allocate memory for dev_name\n");
        return;
    }
    memset(dev_name, '\0', SSID_MAX_LEN);

    if (conn) {
        wifi_sta_info_t wifi_sta_info;
        memset(&wifi_sta_info, 0, sizeof(wifi_sta_info_t));

        int ret = wifi_sta_get_info(&wifi_sta_info);
        if (ret == 0) {
            snprintf(dev_name, SSID_MAX_LEN, "%s", wifi_sta_info.ssid);
        }
    }

    if (wifi_show) {
        main_page_prompt_data_t *msg =
            (main_page_prompt_data_t *)malloc(sizeof(main_page_prompt_data_t));
        if (msg == NULL) {
            WIFI_ERR("Error: Failed to allocate memory for msg\n");
            free(dev_name);
            return;
        }

        msg->status = (conn) ? WIFI_STATE_CONN : WIFI_STATE_DISCONN;
        msg->devName = dev_name;

        lv_event_send(wifi_show, LV_EVENT_REFRESH, msg);
    } else {
        free(dev_name);
    }
}

#endif

#else

int lv_pro_res_wifi_ap_off() { return -1; }

#endif /* ENABLE_WIFI */