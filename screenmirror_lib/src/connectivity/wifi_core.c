/*
 * wifi_core.c - WiFi management layer (LVGL-free)
 *
 * On H133 (ENABLE_WIFI defined): calls into libwifimg.so via <wifimg.h>.
 * Without H133: all functions are stubs that return 0 / print log messages.
 *
 * LVGL dependencies stripped: lv_pro_wifi_msg_enqueue  -> wifi_core_event_cb
 *                              lv_pro_res_wifi_logo_show -> removed (UI only)
 *                              lv_pro_wifi_freshui_ack   -> removed (UI only)
 */

#include "wifi_core.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── H133 BSP headers (only when building for hardware) ─────────────────── */
#ifdef ENABLE_WIFI
#ifdef LV_USE_LINUX_WIFI2_MODE
#include <wifi_log.h>
#include <wifimg.h>
#endif
#endif

/* ── internal state ──────────────────────────────────────────────────────── */

#define SCAN_LIST_LEN   100
#define RSSI_HIGH       (-40)
#define RSSI_MEDIUM     (-60)
#define RSSI_LOW        (-80)

#define BAND_NONE  0
#define BAND_2_4G  1
#define BAND_5G    2

typedef enum {
    WIFI_STATE_IDLE,
    WIFI_STATE_OPENING,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_DISCONNECTING,
    WIFI_STATE_CLOSING,
} wifi_internal_state_t;

static struct {
    bool                  initialized;
    wifi_core_event_cb_t  event_cb;
    void                 *user_data;
    wifi_internal_state_t state;
    pthread_mutex_t       lock;
    bool                  connected;
} g_wifi = {
    .initialized = false,
    .event_cb    = NULL,
    .user_data   = NULL,
    .state       = WIFI_STATE_IDLE,
    .lock        = PTHREAD_MUTEX_INITIALIZER,
    .connected   = false,
};

#ifdef ENABLE_WIFI
#ifdef LV_USE_LINUX_WIFI2_MODE

static wifi_core_scan_result_t scan_results[SCAN_LIST_LEN];
static int scan_results_num = 0;

/* ── internal helpers ──────────────────────────────────────────────────── */

static void set_state(wifi_internal_state_t s)
{
    pthread_mutex_lock(&g_wifi.lock);
    g_wifi.state = s;
    pthread_mutex_unlock(&g_wifi.lock);
}

static void emit_event(wifi_core_event_t ev, void *data, int count)
{
    if (g_wifi.event_cb)
        g_wifi.event_cb(ev, data, count, g_wifi.user_data);
}

static uint32_t freq_to_channel(uint32_t freq)
{
    int band;
    if (freq >= 4910 && freq <= 4980)
        band = BAND_5G;
    else if (freq >= 2407 && freq <= 2484)
        band = BAND_2_4G;
    else
        return 0;

    if (band == BAND_2_4G) {
        if (freq == 2484) return 14;
        if (freq == 2407) return 0;
        if (freq > 2407 && freq <= 2472 && (freq - 2407) % 5 == 0)
            return (freq - 2407) / 5;
    } else {
        if ((freq - 4000) % 5 == 0)
            return (freq - 4000) / 5;
    }
    return 0;
}

/* ── libwifimg message callback ─────────────────────────────────────────── */

static void wifi_msg_cb(wifi_msg_data_t *msg)
{
    if (!msg) return;

    switch (msg->id) {
    case WIFI_MSG_ID_DEV_STATUS:
        switch (msg->data.d_status) {
        case WLAN_STATUS_DOWN:
            printf("[WIFI] wlan down\n");
            emit_event(WIFI_CORE_EVENT_DEV_DOWN, NULL, 0);
            break;
        case WLAN_STATUS_UP:
            printf("[WIFI] wlan up\n");
            emit_event(WIFI_CORE_EVENT_DEV_UP, NULL, 0);
            break;
        default: break;
        }
        break;

    case WIFI_MSG_ID_STA_CN_EVENT:
        switch (msg->data.event) {
        case WIFI_DISCONNECTED:
            printf("[WIFI] STA disconnected\n");
            pthread_mutex_lock(&g_wifi.lock);
            g_wifi.connected = false;
            g_wifi.state     = WIFI_STATE_IDLE;
            pthread_mutex_unlock(&g_wifi.lock);
            emit_event(WIFI_CORE_EVENT_DISCONNECTED, NULL, 0);
            break;

        case WIFI_CONNECT_TIMEOUT:
        case WIFI_DHCP_TIMEOUT:
            printf("[WIFI] connect/DHCP timeout\n");
            pthread_mutex_lock(&g_wifi.lock);
            g_wifi.connected = false;
            g_wifi.state     = WIFI_STATE_IDLE;
            pthread_mutex_unlock(&g_wifi.lock);
            emit_event(WIFI_CORE_EVENT_CONNECT_FAILED, NULL, 0);
            break;

        case WIFI_DHCP_SUCCESS:
            printf("[WIFI] DHCP success – connected\n");
            pthread_mutex_lock(&g_wifi.lock);
            g_wifi.connected = true;
            g_wifi.state     = WIFI_STATE_IDLE;
            pthread_mutex_unlock(&g_wifi.lock);
            emit_event(WIFI_CORE_EVENT_CONNECTED, NULL, 0);
            break;

        case WIFI_SCAN_RESULTS: {
            /* collect scan entries */
            scan_results_num = 0;
            wifi_scan_result_t raw[SCAN_LIST_LEN];
            int n = 0;
            wifi_get_scan_results(raw, &n, SCAN_LIST_LEN);
            for (int i = 0; i < n && scan_results_num < SCAN_LIST_LEN; i++) {
                wifi_core_scan_result_t *r = &scan_results[scan_results_num];
                memset(r, 0, sizeof(*r));
                strncpy(r->ssid,  raw[i].ssid,  sizeof(r->ssid)  - 1);
                strncpy(r->bssid, raw[i].bssid, sizeof(r->bssid) - 1);
                r->rssi     = raw[i].rssi;
                r->security = (int)raw[i].key_mgmt;
                r->freq     = raw[i].freq;
                r->channel  = (int)freq_to_channel(raw[i].freq);
                scan_results_num++;
            }
            emit_event(WIFI_CORE_EVENT_SCAN_DONE, scan_results, scan_results_num);
            break;
        }
        default: break;
        }
        break;

    case WIFI_MSG_ID_AP_CN_EVENT:
        switch (msg->data.ap_event) {
        case WIFI_AP_ENABLED:
            printf("[WIFI] AP enabled\n");
            emit_event(WIFI_CORE_EVENT_AP_STARTED, NULL, 0);
            break;
        case WIFI_AP_DISABLED:
            printf("[WIFI] AP disabled\n");
            emit_event(WIFI_CORE_EVENT_AP_STOPPED, NULL, 0);
            break;
        default: break;
        }
        break;

    case WIFI_MSG_ID_P2P_CN_EVENT:
        switch (msg->data.event) {
        case WIFI_P2P_GROUP_STARTED:
            printf("[WIFI] P2P group started\n");
            emit_event(WIFI_CORE_EVENT_P2P_CONNECTED, NULL, 0);
            break;
        case WIFI_P2P_GROUP_REMOVED:
            printf("[WIFI] P2P group removed\n");
            emit_event(WIFI_CORE_EVENT_P2P_DISCONNECTED, NULL, 0);
            break;
        default: break;
        }
        break;

    default: break;
    }
}

#endif /* LV_USE_LINUX_WIFI2_MODE */
#endif /* ENABLE_WIFI */

/* ══════════════════════════════════════════════════════════════════════════
 *  Public API
 * ══════════════════════════════════════════════════════════════════════════ */

int wifi_core_init(wifi_core_event_cb_t cb, void *user_data)
{
    if (g_wifi.initialized) {
        printf("[WIFI] already initialized\n");
        return 0;
    }

    g_wifi.event_cb   = cb;
    g_wifi.user_data  = user_data;
    g_wifi.connected  = false;
    g_wifi.state      = WIFI_STATE_IDLE;

#ifdef ENABLE_WIFI
#ifdef LV_USE_LINUX_WIFI2_MODE
    wmg_set_debug_level(4);
    int ret = wifimanager_init();
    if (ret != 0) {
        printf("[WIFI] wifimanager_init failed: %d\n", ret);
        return ret;
    }
#endif
#else
    printf("[WIFI] stub: init (no hardware)\n");
#endif

    g_wifi.initialized = true;
    printf("[WIFI] initialized\n");
    return 0;
}

void wifi_core_deinit(void)
{
    if (!g_wifi.initialized) return;

#ifdef ENABLE_WIFI
#ifdef LV_USE_LINUX_WIFI2_MODE
    wifimanager_deinit();
#endif
#else
    printf("[WIFI] stub: deinit\n");
#endif

    g_wifi.initialized = false;
    g_wifi.connected   = false;
    printf("[WIFI] deinitialized\n");
}

int wifi_core_on(void)
{
#ifdef ENABLE_WIFI
#ifdef LV_USE_LINUX_WIFI2_MODE
    static char cb_arg[] = "sta+p2p";
    int ret = wifi_on(WIFI_STATION_P2P);
    if (ret != 0) {
        printf("[WIFI] wifi_on failed: %d\n", ret);
        return ret;
    }
    ret = wifi_register_msg_cb(wifi_msg_cb, cb_arg);
    if (ret != 0) {
        printf("[WIFI] register msg cb failed: %d\n", ret);
        return ret;
    }
    wifi_sta_auto_reconnect(true);
    return 0;
#endif
#endif
    printf("[WIFI] stub: on\n");
    return 0;
}

int wifi_core_off(void)
{
#ifdef ENABLE_WIFI
#ifdef LV_USE_LINUX_WIFI2_MODE
    return wifi_off(WIFI_STATION_P2P);
#endif
#endif
    printf("[WIFI] stub: off\n");
    return 0;
}

int wifi_core_ap_on(const char *ssid, const char *psk, int channel)
{
    if (!ssid || !psk) return -1;

#ifdef ENABLE_WIFI
#ifdef LV_USE_LINUX_WIFI2_MODE
    static char ap_cb_arg[] = "ap mode";
    char ssid_buf[SSID_MAX_LEN + 1];
    char psk_buf[PSK_MAX_LEN + 1];

    memset(ssid_buf, 0, sizeof(ssid_buf));
    memset(psk_buf,  0, sizeof(psk_buf));
    strncpy(ssid_buf, ssid, SSID_MAX_LEN);
    strncpy(psk_buf,  psk,  PSK_MAX_LEN);

    wifi_ap_config_t ap_config = {
        .ssid    = ssid_buf,
        .psk     = psk_buf,
        .sec     = WIFI_SEC_WPA2_PSK,
        .channel = channel,
    };

    int ret = wifi_on(WIFI_AP);
    if (ret != 0) {
        printf("[WIFI] wifi_on(AP) failed: %d\n", ret);
        return ret;
    }
    ret = wifi_register_msg_cb(wifi_msg_cb, ap_cb_arg);
    if (ret != 0) {
        printf("[WIFI] register AP cb failed: %d\n", ret);
        return ret;
    }
    ret = wifi_ap_enable(&ap_config);
    if (ret == 0)
        printf("[WIFI] AP started: ssid=%s ch=%d\n", ssid_buf, channel);
    else
        printf("[WIFI] AP enable failed: %d\n", ret);
    return ret;
#endif
#endif
    printf("[WIFI] stub: ap_on ssid=%s\n", ssid);
    return 0;
}

int wifi_core_ap_off(void)
{
#ifdef ENABLE_WIFI
#ifdef LV_USE_LINUX_WIFI2_MODE
    return wifi_ap_disable();
#endif
#endif
    printf("[WIFI] stub: ap_off\n");
    return 0;
}

int wifi_core_scan(void)
{
#ifdef ENABLE_WIFI
#ifdef LV_USE_LINUX_WIFI2_MODE
    int ret = wifi_scan();
    if (ret != 0)
        printf("[WIFI] scan failed: %d\n", ret);
    return ret;
#endif
#endif
    printf("[WIFI] stub: scan\n");
    return 0;
}

int wifi_core_connect(const char *ssid, const char *password, int security)
{
    if (!ssid) return -1;

#ifdef ENABLE_WIFI
#ifdef LV_USE_LINUX_WIFI2_MODE
    wifi_sta_cn_para_t para;
    memset(&para, 0, sizeof(para));
    strncpy(para.ssid, ssid, sizeof(para.ssid) - 1);
    if (password && *password)
        strncpy(para.password, password, sizeof(para.password) - 1);
    para.sec = (wifi_secure_t)security;

    set_state(WIFI_STATE_CONNECTING);
    int ret = wifi_connect(&para);
    if (ret != 0) {
        set_state(WIFI_STATE_IDLE);
        printf("[WIFI] connect failed: %d\n", ret);
    }
    return ret;
#endif
#endif
    printf("[WIFI] stub: connect ssid=%s\n", ssid);
    return 0;
}

int wifi_core_disconnect(void)
{
#ifdef ENABLE_WIFI
#ifdef LV_USE_LINUX_WIFI2_MODE
    set_state(WIFI_STATE_DISCONNECTING);
    int ret = wifi_disconnect();
    if (ret != 0)
        printf("[WIFI] disconnect failed: %d\n", ret);
    return ret;
#endif
#endif
    printf("[WIFI] stub: disconnect\n");
    return 0;
}

int wifi_core_auto_connect(const char *ssid)
{
    if (!ssid) return -1;
#ifdef ENABLE_WIFI
#ifdef LV_USE_LINUX_WIFI2_MODE
    return wifi_sta_connect(ssid, NULL, 0);
#endif
#endif
    printf("[WIFI] stub: auto_connect ssid=%s\n", ssid);
    return 0;
}

int wifi_core_forget_network(const char *ssid)
{
    if (!ssid) return -1;
#ifdef ENABLE_WIFI
#ifdef LV_USE_LINUX_WIFI2_MODE
    return wifi_sta_remove_networks(ssid);
#endif
#endif
    printf("[WIFI] stub: forget ssid=%s\n", ssid);
    return 0;
}

bool wifi_core_is_connected(void)
{
    bool ret;
    pthread_mutex_lock(&g_wifi.lock);
    ret = g_wifi.connected;
    pthread_mutex_unlock(&g_wifi.lock);
    return ret;
}

int wifi_core_get_conn_info(wifi_core_conn_info_t *info)
{
    if (!info) return -1;
    memset(info, 0, sizeof(*info));

#ifdef ENABLE_WIFI
#ifdef LV_USE_LINUX_WIFI2_MODE
    wifi_sta_info_t sta;
    memset(&sta, 0, sizeof(sta));
    if (wifi_get_sta_info(&sta) == 0) {
        strncpy(info->ssid, sta.ssid, sizeof(info->ssid) - 1);
        strncpy(info->ip,   sta.ip,   sizeof(info->ip)   - 1);
        snprintf(info->mac, sizeof(info->mac), "%02x:%02x:%02x:%02x:%02x:%02x",
                 sta.bssid[0], sta.bssid[1], sta.bssid[2],
                 sta.bssid[3], sta.bssid[4], sta.bssid[5]);
        return 0;
    }
    return -1;
#endif
#endif
    strncpy(info->ssid, "stub-ssid", sizeof(info->ssid) - 1);
    strncpy(info->ip,   "0.0.0.0",  sizeof(info->ip)   - 1);
    return 0;
}

const char *wifi_core_rssi_to_str(int rssi)
{
    if (rssi >= RSSI_HIGH)   return "High";
    if (rssi >= RSSI_MEDIUM) return "Medium";
    if (rssi >= RSSI_LOW)    return "Low";
    return "None";
}
