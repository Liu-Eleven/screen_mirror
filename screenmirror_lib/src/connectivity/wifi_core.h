/*
 * wifi_core.h - WiFi management layer (stripped of LVGL)
 *
 * Wraps Allwinner libwifimg.so with a clean event-driven C API.
 * Conditionally compiled under ENABLE_WIFI / H133_BOARD.
 */

#ifndef WIFI_CORE_H
#define WIFI_CORE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== WiFi event types ========== */
typedef enum {
    WIFI_CORE_EVENT_CONNECTED,       /* STA connected + DHCP success */
    WIFI_CORE_EVENT_DISCONNECTED,    /* STA disconnected */
    WIFI_CORE_EVENT_CONNECT_FAILED,  /* connect timeout / password wrong */
    WIFI_CORE_EVENT_SCAN_DONE,       /* scan finished */
    WIFI_CORE_EVENT_AP_STARTED,      /* AP mode enabled */
    WIFI_CORE_EVENT_AP_STOPPED,      /* AP mode disabled */
    WIFI_CORE_EVENT_DEV_UP,          /* wlan interface up */
    WIFI_CORE_EVENT_DEV_DOWN,        /* wlan interface down */
    WIFI_CORE_EVENT_P2P_CONNECTED,   /* P2P group formed (for Miracast) */
    WIFI_CORE_EVENT_P2P_DISCONNECTED,/* P2P group removed */
} wifi_core_event_t;

/* Scan result entry */
typedef struct {
    char ssid[64];
    char bssid[18];
    int  rssi;
    int  security;   /* maps to wifi_secure_t on H133 */
    int  channel;
    int  freq;
} wifi_core_scan_result_t;

/* Connection info */
typedef struct {
    char ssid[64];
    char ip[16];
    char mac[18];
} wifi_core_conn_info_t;

/**
 * WiFi event callback.
 * @param event:    event type
 * @param data:     optional payload (wifi_core_scan_result_t* for SCAN_DONE, etc.)
 * @param count:    number of entries in data array (for SCAN_DONE)
 * @param user_data: caller-supplied opaque pointer
 */
typedef void (*wifi_core_event_cb_t)(wifi_core_event_t event,
                                     void *data, int count,
                                     void *user_data);

/* ========== API ========== */

/**
 * Initialize WiFi manager.  Must be called before any other wifi_core_* function.
 * @param cb:        event callback (may be NULL)
 * @param user_data: opaque pointer passed to each callback invocation
 * @return 0 on success, negative on failure
 */
int wifi_core_init(wifi_core_event_cb_t cb, void *user_data);

/**
 * Deinitialize WiFi manager and release all resources.
 */
void wifi_core_deinit(void);

/** Enable WiFi in STA+P2P mode. */
int wifi_core_on(void);

/** Disable WiFi. */
int wifi_core_off(void);

/**
 * Start WiFi AP mode.
 * @param ssid:    AP SSID
 * @param psk:     AP passphrase
 * @param channel: WiFi channel (1-13 for 2.4G, 36+ for 5G)
 */
int wifi_core_ap_on(const char *ssid, const char *psk, int channel);

/** Stop WiFi AP mode. */
int wifi_core_ap_off(void);

/** Trigger an async WiFi scan.  Results delivered via WIFI_CORE_EVENT_SCAN_DONE. */
int wifi_core_scan(void);

/**
 * Connect to a WiFi network.
 * @param ssid:     target SSID
 * @param password: WPA/WPA2/WPA3 passphrase (empty string for open networks)
 * @param security: security type (0 = NONE, use wifi_secure_t values on H133)
 */
int wifi_core_connect(const char *ssid, const char *password, int security);

/** Disconnect from current AP. */
int wifi_core_disconnect(void);

/**
 * Auto-connect to a previously saved network (blocking until DHCP or timeout).
 * @param ssid: target SSID
 */
int wifi_core_auto_connect(const char *ssid);

/**
 * Remove (forget) a saved network.
 * @param ssid: SSID of the network to forget
 */
int wifi_core_forget_network(const char *ssid);

/** Returns true if currently connected to an AP with a valid IP. */
bool wifi_core_is_connected(void);

/**
 * Get current connection info.
 * @param info: output structure (caller-allocated)
 * @return 0 if connected and info filled, negative otherwise
 */
int wifi_core_get_conn_info(wifi_core_conn_info_t *info);

/* ========== Signal-strength helpers ========== */
const char *wifi_core_rssi_to_str(int rssi);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_CORE_H */
