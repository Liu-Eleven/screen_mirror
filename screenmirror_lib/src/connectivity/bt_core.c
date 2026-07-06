/*
 * bt_core.c - Bluetooth management layer (LVGL-free)
 *
 * On H133 (ENABLE_BT defined): calls into libbtmanager.so via <bt_manager.h>.
 * Without H133: all functions are stubs.
 *
 * LVGL dependencies stripped:
 *   lv_pro_bt_msg_enqueue        -> bt_core_event_cb
 *   lv_pro_res_bt_logo_show      -> removed (UI only)
 *   lv_pro_bt_freshui_ack        -> removed (UI only)
 *   bt_pro_info (profile config) -> embedded locally as bt_core_profile_info
 */

#include "bt_core.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── H133 BSP headers ──────────────────────────────────────────────────── */
#ifdef ENABLE_BT
#ifdef LV_USE_LINUX_BT4_MODE
#include <bt_dev_list.h>
#include <bt_log.h>
#include <bt_manager.h>

#ifndef BTMG_A2DP_SINK_ENABLE
#define BTMG_A2DP_SINK_ENABLE   (1 << 0)
#endif
#ifndef BTMG_A2DP_SOUCE_ENABLE
#define BTMG_A2DP_SOUCE_ENABLE  (1 << 1)
#endif
#ifndef BTMG_AVRCP_ENABLE
#define BTMG_AVRCP_ENABLE       (1 << 2)
#endif
#endif /* LV_USE_LINUX_BT4_MODE */
#endif /* ENABLE_BT */

/* ── internal state ──────────────────────────────────────────────────────── */

#ifdef ENABLE_BT
#ifdef LV_USE_LINUX_BT4_MODE

#define BT_CONFIG_PATH "/etc/bluetooth"

static struct {
    bool                initialized;
    bool                sink_mode;    /* true = sink, false = source */
    bool                connected;
    bt_core_event_cb_t  event_cb;
    void               *user_data;
    pthread_mutex_t     dev_mutex;
    pthread_mutex_t     state_mutex;
    dev_list_t         *discovered_devices;
    btmg_callback_t    *callback;
} g_bt = {
    .initialized       = false,
    .sink_mode         = false,
    .connected         = false,
    .event_cb          = NULL,
    .user_data         = NULL,
    .dev_mutex         = PTHREAD_MUTEX_INITIALIZER,
    .state_mutex       = PTHREAD_MUTEX_INITIALIZER,
    .discovered_devices = NULL,
    .callback          = NULL,
};

/* ── helpers ─────────────────────────────────────────────────────────────── */

static void emit_event(bt_core_event_t ev, const bt_core_device_t *data)
{
    if (g_bt.event_cb)
        g_bt.event_cb(ev, data, g_bt.user_data);
}

static void update_bluealsa_card(const char *bd_addr, bool connected)
{
    char path[256];
    snprintf(path, sizeof(path), "%s/bluealsa_card", BT_CONFIG_PATH);
    FILE *f = fopen(path, "w");
    if (!f) return;
    if (connected)
        fprintf(f, "bluealsa:DEV=%s\n", bd_addr);
    fclose(f);
}

/* ── btmanager callbacks (no LVGL calls) ───────────────────────────────── */

static void adapter_status_cb(btmg_adapter_state_t status)
{
    switch (status) {
    case BTMG_ADAPTER_OFF:
        printf("[BT] adapter off\n");
        update_bluealsa_card("", false);
        pthread_mutex_lock(&g_bt.state_mutex);
        g_bt.connected = false;
        pthread_mutex_unlock(&g_bt.state_mutex);
        emit_event(BT_CORE_EVENT_POWER_OFF, NULL);
        break;

    case BTMG_ADAPTER_ON: {
        printf("[BT] adapter on\n");
        char addr[18] = {0};
        char name[64] = {0};
        char suffix[8] = {0};
        bt_manager_get_adapter_address(addr);
        /* generate a short device name from MAC */
        snprintf(suffix, sizeof(suffix), "%c%c%c%c",
                 addr[12], addr[13], addr[15], addr[16]);
        snprintf(name, sizeof(name), "AWCast-BT_%s", suffix);
        bt_manager_set_adapter_name(name);
        bt_manager_agent_set_io_capability(BTMG_IO_CAP_NOINPUTNOOUTPUT);
        bt_manager_set_scan_mode(BTMG_SCAN_MODE_CONNECTABLE_DISCOVERABLE);
        bt_manager_set_page_timeout(4000);
        emit_event(BT_CORE_EVENT_POWER_ON, NULL);
        break;
    }
    default: break;
    }
}

static void scan_status_cb(btmg_scan_state_t status)
{
    if (status == BTMG_SCAN_STARTED) {
        printf("[BT] scan started\n");
        emit_event(BT_CORE_EVENT_SCAN_STARTED, NULL);
    } else if (status == BTMG_SCAN_STOPPED) {
        printf("[BT] scan stopped\n");
        emit_event(BT_CORE_EVENT_SCAN_STOPPED, NULL);
    }
}

static void dev_add_cb(btmg_bt_device_t *dev)
{
    if (!dev) return;
    pthread_mutex_lock(&g_bt.dev_mutex);
    if (!btmg_dev_list_find_device(g_bt.discovered_devices, dev->remote_address))
        btmg_dev_list_add_device(g_bt.discovered_devices,
                                  dev->remote_name, dev->remote_address, dev->r_class);
    pthread_mutex_unlock(&g_bt.dev_mutex);

    bt_core_device_t d;
    memset(&d, 0, sizeof(d));
    strncpy(d.name,     dev->remote_name,    sizeof(d.name)     - 1);
    strncpy(d.mac_addr, dev->remote_address, sizeof(d.mac_addr) - 1);
    d.rssi = dev->rssi;
    d.cod  = dev->r_class;
    emit_event(BT_CORE_EVENT_DEVICE_FOUND, &d);
}

static void dev_remove_cb(btmg_bt_device_t *dev)
{
    if (!dev) return;
    pthread_mutex_lock(&g_bt.dev_mutex);
    btmg_dev_list_remove_device(g_bt.discovered_devices, dev->remote_address);
    pthread_mutex_unlock(&g_bt.dev_mutex);

    bt_core_device_t d;
    memset(&d, 0, sizeof(d));
    strncpy(d.mac_addr, dev->remote_address, sizeof(d.mac_addr) - 1);
    emit_event(BT_CORE_EVENT_DEVICE_LOST, &d);
}

static void bond_state_cb(btmg_bond_state_t state, const char *addr)
{
    if (state == BTMG_BOND_STATE_BONDED) {
        printf("[BT] bonded: %s\n", addr);
        pthread_mutex_lock(&g_bt.dev_mutex);
        btmg_dev_list_remove_device(g_bt.discovered_devices, addr);
        pthread_mutex_unlock(&g_bt.dev_mutex);

        bt_core_device_t d;
        memset(&d, 0, sizeof(d));
        strncpy(d.mac_addr, addr, sizeof(d.mac_addr) - 1);
        d.bonded = true;
        emit_event(BT_CORE_EVENT_BONDED, &d);
    }
}

static void agent_request_pincode_cb(void *handle, char *dev)
{
    printf("[BT] pincode request from %s\n", dev);
    bt_manager_agent_send_pincode(handle, "0000");
}
static void agent_request_passkey_cb(void *handle, char *dev)
{
    unsigned int pk = (unsigned int)(rand() % 1000000);
    printf("[BT] passkey request from %s, sending %06u\n", dev, pk);
    bt_manager_agent_send_passkey(handle, pk);
}
static void agent_display_pincode_cb(char *dev, char *pin)  { (void)dev; (void)pin; }
static void agent_display_passkey_cb(char *dev, unsigned int pk, unsigned int en)
    { (void)dev; (void)pk; (void)en; }
static void agent_confirm_passkey_cb(void *h, char *dev, unsigned int pk)
    { (void)dev; (void)pk; bt_manager_agent_pair_send_empty_response(h); }
static void agent_authorize_cb(void *h, char *dev)
    { (void)dev; bt_manager_agent_pair_send_empty_response(h); }
static void agent_authorize_service_cb(void *h, char *dev, char *uuid)
    { (void)dev; (void)uuid; bt_manager_agent_pair_send_empty_response(h); }

static void a2dp_sink_conn_cb(const char *addr, btmg_a2dp_sink_connection_state_t state)
{
    (void)addr;
    if (state == BTMG_A2DP_SINK_CONNECTED) {
        printf("[BT] A2DP sink connected: %s\n", addr);
        bt_core_device_t d; memset(&d, 0, sizeof(d));
        strncpy(d.mac_addr, addr, sizeof(d.mac_addr) - 1);
        d.connected = true;
        emit_event(BT_CORE_EVENT_A2DP_CONNECTED, &d);
    } else if (state == BTMG_A2DP_SINK_DISCONNECTED) {
        printf("[BT] A2DP sink disconnected: %s\n", addr);
        bt_manager_set_scan_mode(BTMG_SCAN_MODE_CONNECTABLE_DISCOVERABLE);
        bt_core_device_t d; memset(&d, 0, sizeof(d));
        strncpy(d.mac_addr, addr, sizeof(d.mac_addr) - 1);
        emit_event(BT_CORE_EVENT_A2DP_DISCONNECTED, &d);
    }
}

static void a2dp_src_conn_cb(const char *addr, btmg_a2dp_source_connection_state_t state)
{
    if (state == BTMG_A2DP_SOURCE_CONNECTED) {
        printf("[BT] A2DP source connected: %s\n", addr);
        bt_manager_set_link_supervision_timeout(addr, 1600);
        bt_manager_set_scan_mode(BTMG_SCAN_MODE_NONE);
        update_bluealsa_card(addr, true);
        pthread_mutex_lock(&g_bt.state_mutex);
        g_bt.connected = true;
        pthread_mutex_unlock(&g_bt.state_mutex);
        bt_core_device_t d; memset(&d, 0, sizeof(d));
        strncpy(d.mac_addr, addr, sizeof(d.mac_addr) - 1);
        d.connected = true;
        emit_event(BT_CORE_EVENT_A2DP_CONNECTED, &d);
    } else if (state == BTMG_A2DP_SOURCE_DISCONNECTED) {
        printf("[BT] A2DP source disconnected: %s\n", addr);
        update_bluealsa_card("", false);
        pthread_mutex_lock(&g_bt.state_mutex);
        g_bt.connected = false;
        pthread_mutex_unlock(&g_bt.state_mutex);
        bt_manager_set_scan_mode(BTMG_SCAN_MODE_CONNECTABLE_DISCOVERABLE);
        bt_core_device_t d; memset(&d, 0, sizeof(d));
        strncpy(d.mac_addr, addr, sizeof(d.mac_addr) - 1);
        emit_event(BT_CORE_EVENT_A2DP_DISCONNECTED, &d);
    } else if (state == BTMG_A2DP_SOURCE_CONNECT_FAILED ||
               state == BTMG_A2DP_SOURCE_DISCONNEC_FAILED) {
        printf("[BT] A2DP source connect/disconnect failed: %s\n", addr);
        bt_core_device_t d; memset(&d, 0, sizeof(d));
        strncpy(d.mac_addr, addr, sizeof(d.mac_addr) - 1);
        emit_event(BT_CORE_EVENT_A2DP_CONNECT_FAILED, &d);
    }
}

/* ── _bt_init helper ────────────────────────────────────────────────────── */

static int _bt_init(bool sink_mode)
{
    bt_manager_set_loglevel(BTMG_LOG_LEVEL_DEBUG);
    btmg_set_log_file_path("/tmp/btmg.log");

    if (bt_manager_preinit(&g_bt.callback) != 0) {
        printf("[BT] preinit failed\n");
        return -1;
    }

    if (sink_mode)
        bt_manager_enable_profile(BTMG_A2DP_SINK_ENABLE | BTMG_AVRCP_ENABLE);
    else
        bt_manager_enable_profile(BTMG_A2DP_SOUCE_ENABLE | BTMG_AVRCP_ENABLE);

    g_bt.callback->btmg_adapter_cb.adapter_state_cb      = adapter_status_cb;
    g_bt.callback->btmg_gap_cb.gap_scan_status_cb        = scan_status_cb;
    g_bt.callback->btmg_gap_cb.gap_device_add_cb         = dev_add_cb;
    g_bt.callback->btmg_gap_cb.gap_device_remove_cb      = dev_remove_cb;
    g_bt.callback->btmg_gap_cb.gap_bond_state_cb         = bond_state_cb;

    g_bt.callback->btmg_agent_cb.agent_request_pincode         = agent_request_pincode_cb;
    g_bt.callback->btmg_agent_cb.agent_display_pincode         = agent_display_pincode_cb;
    g_bt.callback->btmg_agent_cb.agent_request_passkey         = agent_request_passkey_cb;
    g_bt.callback->btmg_agent_cb.agent_display_passkey         = agent_display_passkey_cb;
    g_bt.callback->btmg_agent_cb.agent_request_confirm_passkey = agent_confirm_passkey_cb;
    g_bt.callback->btmg_agent_cb.agent_request_authorize       = agent_authorize_cb;
    g_bt.callback->btmg_agent_cb.agent_authorize_service       = agent_authorize_service_cb;

    if (sink_mode)
        g_bt.callback->btmg_a2dp_sink_cb.a2dp_sink_connection_state_cb = a2dp_sink_conn_cb;
    else
        g_bt.callback->btmg_a2dp_source_cb.a2dp_source_connection_state_cb = a2dp_src_conn_cb;

    if (bt_manager_init(g_bt.callback) != 0) {
        printf("[BT] manager init failed\n");
        return -1;
    }

    g_bt.discovered_devices = btmg_dev_list_new();
    if (!g_bt.discovered_devices) {
        bt_manager_deinit(g_bt.callback);
        return -1;
    }
    return 0;
}

#endif /* LV_USE_LINUX_BT4_MODE */
#endif /* ENABLE_BT */

/* ══════════════════════════════════════════════════════════════════════════
 *  Public API
 * ══════════════════════════════════════════════════════════════════════════ */

int bt_core_init(bt_core_event_cb_t cb, void *user_data, bool sink_mode)
{
#ifdef ENABLE_BT
#ifdef LV_USE_LINUX_BT4_MODE
    if (g_bt.initialized) {
        printf("[BT] already initialized\n");
        return 0;
    }
    g_bt.event_cb   = cb;
    g_bt.user_data  = user_data;
    g_bt.sink_mode  = sink_mode;
    g_bt.connected  = false;

    int ret = _bt_init(sink_mode);
    if (ret == 0) {
        g_bt.initialized = true;
        printf("[BT] initialized (mode: %s)\n", sink_mode ? "sink" : "source");
    }
    return ret;
#endif
#endif
    printf("[BT] stub: init (no hardware)\n");
    (void)cb; (void)user_data; (void)sink_mode;
    return 0;
}

void bt_core_deinit(void)
{
#ifdef ENABLE_BT
#ifdef LV_USE_LINUX_BT4_MODE
    if (!g_bt.initialized) return;

    bt_manager_enable(false);

    pthread_mutex_lock(&g_bt.dev_mutex);
    if (g_bt.discovered_devices) {
        btmg_dev_list_free(g_bt.discovered_devices);
        g_bt.discovered_devices = NULL;
    }
    pthread_mutex_unlock(&g_bt.dev_mutex);

    bt_manager_deinit(g_bt.callback);
    btmg_log_stop();
    g_bt.initialized = false;
    g_bt.connected   = false;
    printf("[BT] deinitialized\n");
    return;
#endif
#endif
    printf("[BT] stub: deinit\n");
}

int bt_core_on(void)
{
#ifdef ENABLE_BT
#ifdef LV_USE_LINUX_BT4_MODE
    int ret = bt_manager_enable(true);
    if (ret != 0)
        printf("[BT] enable failed: %d\n", ret);
    return ret;
#endif
#endif
    printf("[BT] stub: on\n");
    return 0;
}

int bt_core_off(void)
{
#ifdef ENABLE_BT
#ifdef LV_USE_LINUX_BT4_MODE
    int ret = bt_manager_enable(false);
    if (ret != 0)
        printf("[BT] disable failed: %d\n", ret);
    return ret;
#endif
#endif
    printf("[BT] stub: off\n");
    return 0;
}

int bt_core_scan(bool start)
{
#ifdef ENABLE_BT
#ifdef LV_USE_LINUX_BT4_MODE
    if (start) {
        pthread_mutex_lock(&g_bt.dev_mutex);
        if (g_bt.discovered_devices)
            btmg_dev_list_free(g_bt.discovered_devices);
        g_bt.discovered_devices = btmg_dev_list_new();
        pthread_mutex_unlock(&g_bt.dev_mutex);
        return bt_manager_start_scan();
    } else {
        return bt_manager_stop_scan();
    }
#endif
#endif
    printf("[BT] stub: scan(%s)\n", start ? "start" : "stop");
    return 0;
}

int bt_core_connect(const char *mac_addr)
{
    if (!mac_addr) return -1;
#ifdef ENABLE_BT
#ifdef LV_USE_LINUX_BT4_MODE
    return bt_manager_connect(mac_addr);
#endif
#endif
    printf("[BT] stub: connect %s\n", mac_addr);
    return 0;
}

int bt_core_disconnect(const char *mac_addr)
{
    if (!mac_addr) return -1;
#ifdef ENABLE_BT
#ifdef LV_USE_LINUX_BT4_MODE
    return bt_manager_disconnect(mac_addr);
#endif
#endif
    printf("[BT] stub: disconnect %s\n", mac_addr);
    return 0;
}

int bt_core_unpair(const char *mac_addr)
{
    if (!mac_addr) return -1;
#ifdef ENABLE_BT
#ifdef LV_USE_LINUX_BT4_MODE
    return bt_manager_unpair(mac_addr);
#endif
#endif
    printf("[BT] stub: unpair %s\n", mac_addr);
    return 0;
}

int bt_core_get_paired_devices(bt_core_device_t **devices, int *count)
{
    if (!devices || !count) return -1;
    *devices = NULL;
    *count   = 0;

#ifdef ENABLE_BT
#ifdef LV_USE_LINUX_BT4_MODE
    btmg_bt_device_t *raw = NULL;
    int n = 0;
    bt_manager_get_paired_devices(&raw, &n);
    if (n <= 0 || !raw) return 0;

    *devices = (bt_core_device_t *)calloc(n, sizeof(bt_core_device_t));
    if (!*devices) {
        bt_manager_free_paired_devices(raw, n);
        return -1;
    }
    for (int i = 0; i < n; i++) {
        strncpy((*devices)[i].name,     raw[i].remote_name,    248);
        strncpy((*devices)[i].mac_addr, raw[i].remote_address, 17);
        (*devices)[i].connected = raw[i].connected;
        (*devices)[i].bonded    = true;
    }
    *count = n;
    bt_manager_free_paired_devices(raw, n);
    return 0;
#endif
#endif
    return 0;
}

void bt_core_free_devices(bt_core_device_t *devices, int count)
{
    (void)count;
    free(devices);
}

int bt_core_disconnect_old_connection(void)
{
#ifdef ENABLE_BT
#ifdef LV_USE_LINUX_BT4_MODE
    /* Read last connected device from persistent file */
    char path[256];
    snprintf(path, sizeof(path), "%s/bt_connectstate", BT_CONFIG_PATH);
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    char mac[18] = {0};
    if (fscanf(f, "conn %17s", mac) == 1 && mac[0]) {
        printf("[BT] disconnecting old connection: %s\n", mac);
        bt_manager_disconnect(mac);
    }
    fclose(f);
    return 0;
#endif
#endif
    return 0;
}

bool bt_core_is_connected(void)
{
    bool ret = false;
#ifdef ENABLE_BT
#ifdef LV_USE_LINUX_BT4_MODE
    pthread_mutex_lock(&g_bt.state_mutex);
    ret = g_bt.connected;
    pthread_mutex_unlock(&g_bt.state_mutex);
#endif
#endif
    return ret;
}
