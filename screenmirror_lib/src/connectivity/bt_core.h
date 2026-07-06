/*
 * bt_core.h - Bluetooth management layer (stripped of LVGL)
 *
 * Wraps Allwinner libbtmanager.so with a clean event-driven C API.
 * Conditionally compiled under ENABLE_BT / H133_BOARD.
 */

#ifndef BT_CORE_H
#define BT_CORE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== BT event types ========== */
typedef enum {
    BT_CORE_EVENT_POWER_ON,          /* adapter turned on */
    BT_CORE_EVENT_POWER_OFF,         /* adapter turned off */
    BT_CORE_EVENT_SCAN_STARTED,      /* device scan started */
    BT_CORE_EVENT_SCAN_STOPPED,      /* device scan stopped */
    BT_CORE_EVENT_DEVICE_FOUND,      /* new device discovered during scan */
    BT_CORE_EVENT_DEVICE_LOST,       /* previously found device no longer visible */
    BT_CORE_EVENT_A2DP_CONNECTED,    /* A2DP source connected */
    BT_CORE_EVENT_A2DP_DISCONNECTED, /* A2DP source disconnected */
    BT_CORE_EVENT_A2DP_CONNECT_FAILED,
    BT_CORE_EVENT_BONDED,            /* device paired successfully */
} bt_core_event_t;

/* Device info returned during scan / paired-device query */
typedef struct {
    char name[249];
    char mac_addr[18];
    int  rssi;
    int  cod;        /* class of device */
    bool connected;
    bool bonded;
} bt_core_device_t;

/**
 * BT event callback.
 * @param event:     event type
 * @param data:      bt_core_device_t* for DEVICE_FOUND/LOST/A2DP_*, NULL otherwise
 * @param user_data: caller-supplied opaque pointer
 */
typedef void (*bt_core_event_cb_t)(bt_core_event_t event,
                                   const bt_core_device_t *data,
                                   void *user_data);

/* ========== API ========== */

/**
 * Initialize Bluetooth manager.
 * @param cb:        event callback (may be NULL)
 * @param user_data: opaque pointer
 * @param sink_mode: false = A2DP Source (projector sends audio to BT speaker)
 *                   true  = A2DP Sink   (projector receives audio from phone)
 * @return 0 on success
 */
int bt_core_init(bt_core_event_cb_t cb, void *user_data, bool sink_mode);

/** Deinitialize and release all BT resources. */
void bt_core_deinit(void);

/** Enable Bluetooth adapter. */
int bt_core_on(void);

/** Disable Bluetooth adapter. */
int bt_core_off(void);

/**
 * Start or stop device scanning.
 * @param start: true to start scanning, false to stop
 */
int bt_core_scan(bool start);

/**
 * Connect to a remote Bluetooth device (A2DP).
 * @param mac_addr: remote device MAC in "XX:XX:XX:XX:XX:XX" format
 */
int bt_core_connect(const char *mac_addr);

/**
 * Disconnect from a remote Bluetooth device.
 * @param mac_addr: remote device MAC
 */
int bt_core_disconnect(const char *mac_addr);

/**
 * Remove (unpair) a bonded device.
 * @param mac_addr: remote device MAC
 */
int bt_core_unpair(const char *mac_addr);

/**
 * Get the list of paired devices.
 * Caller is responsible for calling bt_core_free_devices() when done.
 * @param devices: output array pointer
 * @param count:   output count
 */
int bt_core_get_paired_devices(bt_core_device_t **devices, int *count);

/** Free memory allocated by bt_core_get_paired_devices(). */
void bt_core_free_devices(bt_core_device_t *devices, int count);

/** Disconnect any existing A2DP connection (reads state from file). */
int bt_core_disconnect_old_connection(void);

/** Returns true if an A2DP source connection is currently active. */
bool bt_core_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif /* BT_CORE_H */
