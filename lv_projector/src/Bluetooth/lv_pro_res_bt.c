/*
 * lv_pro_res_bt.c
 *
 *  Created on: 2024/09/13
 *      Author: hongjiasen
 */

#include "lv_pro_res_bt.h"

#include "lv_pro_bt_common.h"

#if ENABLE_BT

#ifdef LV_USE_LINUX_BT4_MODE

int bt_mode = 0;  // source:0  sink:1

pthread_mutex_t lv_pro_bt_discovered_devices_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t bt_state_mutex = PTHREAD_MUTEX_INITIALIZER;

static lv_pro_bt_state_t current_bt_state = LVGL_BT_STATE_IDLE;

dev_list_t *lv_pro_bt_discovered_devices;

static btmg_callback_t *bt_callback;
static int song_playing_pos;
static int song_playing_len;
static int is_background = 1;
static bool isbtInit = false;

btmg_callback_t *factory_bt_callback = NULL;

void lv_pro_set_bt_state(lv_pro_bt_state_t new_state)
{
    pthread_mutex_lock(&bt_state_mutex);
    current_bt_state = new_state;
    pthread_mutex_unlock(&bt_state_mutex);
}

lv_pro_bt_state_t lv_pro_get_bt_state(void)
{
    lv_pro_bt_state_t state;
    pthread_mutex_lock(&bt_state_mutex);
    state = current_bt_state;
    pthread_mutex_unlock(&bt_state_mutex);
    return state;
}

bool lv_pro_is_bt_state_idle(void)
{
    bool idle;
    pthread_mutex_lock(&bt_state_mutex);
    idle = (current_bt_state == LVGL_BT_STATE_IDLE);
    pthread_mutex_unlock(&bt_state_mutex);
    return idle;
}

static void update_bluealsa_card(const char *bd_addr, int is_connected)
{
#ifdef LVGL_PROJECTOR_BLUETOOTH_CONFIG_PATH
    const char *config_path = LVGL_PROJECTOR_BLUETOOTH_CONFIG_PATH;
#else
    const char *config_path = "/etc/bluetooth";
#endif
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/bluealsa_card", config_path);

    FILE *file = fopen(file_path, "w");  // 使用 "w" 模式打开文件以清空内容
    if (file == NULL) {
        BT_ERR("Failed to open %s\n", file_path);
        return;
    }

    if (is_connected) {
        // 写入蓝牙设备信息
        fprintf(file, "bluealsa:DEV=%s\n", bd_addr);
    }

    fclose(file);
}

void clear_bluealsa_card()
{
#ifdef LVGL_PROJECTOR_BLUETOOTH_CONFIG_PATH
    const char *config_path = LVGL_PROJECTOR_BLUETOOTH_CONFIG_PATH;
#else
    const char *config_path = "/etc/bluetooth";
#endif
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/bluealsa_card", config_path);

    FILE *file = fopen(file_path, "w");  // 以 "w" 模式打开文件以清空内容
    if (file == NULL) {
        BT_ERR("Failed to open %s\n", file_path);
        return;
    }
    fclose(file);  // 直接关闭文件就能清空内容
}

static void lv_pro_res_bt_test_adapter_status_cb(btmg_adapter_state_t status)
{
    char bt_addr[18] = {0};
    char bt_name_buf[64] = {0};
    char bt_name[64] = {0};

    if (status == BTMG_ADAPTER_OFF) {
        BT_INF("BT is off\n");
        clear_bluealsa_card();
        lv_pro_res_bt_logo_show(false);
        lv_pro_set_bt_state(LVGL_BT_STATE_IDLE);
    } else if (status == BTMG_ADAPTER_ON) {
        BT_INF("BT is ON\n");
        clear_bluealsa_card();
        bt_manager_get_adapter_address(bt_addr);
        snprintf(bt_name_buf, sizeof(bt_name_buf), "%c%c%c%c", bt_addr[12], bt_addr[13],
                 bt_addr[15], bt_addr[16]);
        snprintf(bt_name, sizeof(bt_name), "LV_PRO_BT_%s", bt_name_buf);
        bt_manager_set_adapter_name(bt_name);
        if (is_background)
            bt_manager_agent_set_io_capability(BTMG_IO_CAP_NOINPUTNOOUTPUT);
        else
            bt_manager_agent_set_io_capability(BTMG_IO_CAP_KEYBOARDDISPLAY);
        bt_manager_set_scan_mode(BTMG_SCAN_MODE_CONNECTABLE_DISCOVERABLE);
        bt_manager_set_page_timeout(4000);
        lv_pro_set_bt_state(LVGL_BT_STATE_IDLE);
        lv_pro_bt_freshui_ack();
    } else if (status == BTMG_ADAPTER_TURNING_ON) {
        BT_INF("bt is turnning on.\n");
        lv_pro_set_bt_state(LVGL_BT_STATE_OPENING);
    } else if (status == BTMG_ADAPTER_TURNING_OFF) {
        BT_INF("bt is turnning off.\n");
        lv_pro_set_bt_state(LVGL_BT_STATE_CLOSING);
    }
}

static void lv_pro_res_bt_test_scan_status_cb(btmg_scan_state_t status)
{
    if (status == BTMG_SCAN_STARTED) {
        BT_INF("start scanning.\n");
    } else if (status == BTMG_SCAN_STOPPED) {
        BT_INF("scanning stop.\n");
    }
}

static void lv_pro_res_bt_test_dev_add_cb(btmg_bt_device_t *device)
{
    dev_node_t *dev_node = NULL;

    pthread_mutex_lock(&lv_pro_bt_discovered_devices_mutex);
    BT_INF("address:%s, name:%s, class:%d, icon:%s, rssi:%d\n", device->remote_address,
           device->remote_name, device->r_class, device->icon, device->rssi);

    dev_node = btmg_dev_list_find_device(lv_pro_bt_discovered_devices, device->remote_address);
    if (dev_node != NULL) {
        pthread_mutex_unlock(&lv_pro_bt_discovered_devices_mutex);
        return;
    }

    btmg_dev_list_add_device(lv_pro_bt_discovered_devices, device->remote_name,
                             device->remote_address, device->r_class);
    pthread_mutex_unlock(&lv_pro_bt_discovered_devices_mutex);
}

static void lv_pro_res_bt_test_dev_remove_cb(btmg_bt_device_t *device)
{
    dev_node_t *dev_node = NULL;
    pthread_mutex_lock(&lv_pro_bt_discovered_devices_mutex);
    BT_INF("address:%s,name:%s\n", device->remote_address, device->remote_name);

    btmg_dev_list_remove_device(lv_pro_bt_discovered_devices, device->remote_address);
    pthread_mutex_unlock(&lv_pro_bt_discovered_devices_mutex);
}

static void lv_pro_res_bt_test_update_rssi_cb(const char *address, int rssi)
{
    dev_node_t *dev_node = NULL;
    pthread_mutex_lock(&lv_pro_bt_discovered_devices_mutex);
    dev_node = btmg_dev_list_find_device(lv_pro_bt_discovered_devices, address);
    if (dev_node != NULL) {
        BT_INF("address:%s,name:%s,rssi:%d\n", dev_node->dev_addr, dev_node->dev_name, rssi);
        pthread_mutex_unlock(&lv_pro_bt_discovered_devices_mutex);
        return;
    }
    pthread_mutex_unlock(&lv_pro_bt_discovered_devices_mutex);
}

static void lv_pro_res_bt_test_bond_state_cb(btmg_bond_state_t state, const char *bd_addr)
{
    dev_node_t *dev_discovered_node = NULL;

    pthread_mutex_lock(&lv_pro_bt_discovered_devices_mutex);

    BT_DBG("bonded device state:%d, addr:%s\n", state, bd_addr);

    dev_discovered_node = btmg_dev_list_find_device(lv_pro_bt_discovered_devices, bd_addr);
    if (state == BTMG_BOND_STATE_BONDED) {
        if (dev_discovered_node != NULL) {
            btmg_dev_list_remove_device(lv_pro_bt_discovered_devices, bd_addr);
        }
        BT_INF("Pairing state for %s is BONDED\n", bd_addr);
    } else if (state == BTMG_BOND_STATE_BONDING) {
        BT_INF("Pairing state for %s is BONDEDING\n", bd_addr);
    }
    pthread_mutex_unlock(&lv_pro_bt_discovered_devices_mutex);
}

static void lv_pro_res_bt_test_pair_ask(const char *prompt, char *buffer)
{
    BT_DBG("%s\n", prompt);
    if (fgets(buffer, 17, stdin) == NULL) fprintf(stdout, "\ncmd fgets error\n");
}

void lv_pro_res_bt_test_agent_request_pincode_cb(void *handle, char *device)
{
    char buffer[17] = {0};

    fprintf(stdout, "AGENT:(%s)Request pincode\n", device);
    lv_pro_res_bt_test_pair_ask("Enter PIN Code: \n", buffer);
    bt_manager_agent_send_pincode(handle, buffer);
}

void lv_pro_res_bt_test_agent_display_pincode_cb(char *device, char *pincode)
{
    BT_INF("AGENT: display pincode:%s\n", pincode);
}

void lv_pro_res_bt_test_agent_request_passkey_cb(void *handle, char *device)
{
    unsigned int passkey;

    passkey = (unsigned int)rand() % 1000000;
    BT_INF(
        "AGENT: request passkey:%u, \
            please enter it on the device %s\n",
        passkey, device);
    bt_manager_agent_send_passkey(handle, passkey);
}

void lv_pro_res_bt_test_agent_display_passkey_cb(char *device, unsigned int passkey,
                                                 unsigned int entered)
{
    BT_INF("AGENT: display passkey:%06u, device:%s, enter:%u\n", passkey, device, entered);
}

void lv_pro_res_bt_test_agent_request_confirm_passkey_cb(void *handle, char *device,
                                                         unsigned int passkey)
{
    BT_INF("AGENT: %s request confirmation,passkey:%06u\n", device, passkey);
    bt_manager_agent_pair_send_empty_response(handle);
}

void lv_pro_res_bt_test_agent_request_authorize_cb(void *handle, char *device)
{
    BT_INF("AGENT: %s request authorization\n", device);
    bt_manager_agent_pair_send_empty_response(handle);
}

void lv_pro_res_bt_test_agent_authorize_service_cb(void *handle, char *device, char *uuid)
{
    BT_INF("AGENT: %s Authorize Service %s\n", device, uuid);
    bt_manager_agent_pair_send_empty_response(handle);
}

static void lv_pro_res_bt_test_a2dp_sink_connection_state_cb(
    const char *bd_addr, btmg_a2dp_sink_connection_state_t state)
{
    if (state == BTMG_A2DP_SINK_DISCONNECTED) {
        BT_INF("A2DP sink disconnected with device: %s\n", bd_addr);
        bt_manager_set_scan_mode(BTMG_SCAN_MODE_CONNECTABLE_DISCOVERABLE);
        lv_pro_set_bt_state(LVGL_BT_STATE_IDLE);
    } else if (state == BTMG_A2DP_SINK_CONNECTING) {
        BT_INF("A2DP sink connecting with device: %s\n", bd_addr);
        lv_pro_set_bt_state(LVGL_BT_STATE_CONNECTING);
        return;
    } else if (state == BTMG_A2DP_SINK_CONNECTED) {
        BT_INF("A2DP sink connected with device: %s\n", bd_addr);
        lv_pro_set_bt_state(LVGL_BT_STATE_IDLE);
    } else if (state == BTMG_A2DP_SINK_DISCONNECTING) {
        BT_INF("A2DP sink disconnecting with device: %s\n", bd_addr);
        lv_pro_set_bt_state(LVGL_BT_STATE_DISCONNECTING);
        return;
    }
    lv_pro_bt_freshui_ack();
}

static void lv_pro_res_bt_test_a2dp_source_connection_state_cb(
    const char *bd_addr, btmg_a2dp_source_connection_state_t state)
{
    if (state == BTMG_A2DP_SOURCE_DISCONNECTED) {
        BT_INF("A2DP source disconnected with device: %s\n", bd_addr);
        lv_pro_bt_msg_enqueue(MSG_TYPE_BT_A2DPSRC_DISCONNED, NULL, false);
        bt_manager_set_scan_mode(BTMG_SCAN_MODE_CONNECTABLE_DISCOVERABLE);
        lv_pro_set_bt_state(LVGL_BT_STATE_IDLE);
        clear_bluealsa_card();
        update_file_bt_connectstate(BT_STATE_DISCONN, NULL, NULL);
        lv_pro_res_bt_logo_show(false);
    } else if (state == BTMG_A2DP_SOURCE_CONNECTING) {
        BT_INF("A2DP source connecting with device: %s\n", bd_addr);
        lv_pro_set_bt_state(LVGL_BT_STATE_CONNECTING);
        return;
    } else if (state == BTMG_A2DP_SOURCE_CONNECTED) {
        lv_pro_bt_msg_enqueue(MSG_TYPE_BT_A2DPSRC_CONNECTED, NULL, false);
        bt_manager_set_link_supervision_timeout(bd_addr, 1600);
        bt_manager_set_scan_mode(BTMG_SCAN_MODE_NONE);
        BT_INF("A2DP source connected with device: %s\n", bd_addr);
        update_bluealsa_card(bd_addr, 1);
        char devName[256];
        memset(devName, '\0', sizeof(devName));
        lv_pro_res_bt_get_devname(bd_addr, devName);
        update_file_bt_connectstate(BT_STATE_CONN, bd_addr, devName);
        lv_pro_res_bt_logo_show(true);
        lv_pro_set_bt_state(LVGL_BT_STATE_IDLE);
    } else if (state == BTMG_A2DP_SOURCE_DISCONNECTING) {
        BT_INF("A2DP source disconnecting with device: %s\n", bd_addr);
        lv_pro_set_bt_state(LVGL_BT_STATE_DISCONNECTING);
        return;
    } else if (state == BTMG_A2DP_SOURCE_CONNECT_FAILED) {
        BT_INF("A2DP source connect with device: %s failed!\n", bd_addr);
        lv_pro_set_bt_state(LVGL_BT_STATE_IDLE);
    } else if (state == BTMG_A2DP_SOURCE_DISCONNEC_FAILED) {
        BT_INF("A2DP source disconnect with device: %s failed!\n", bd_addr);
        lv_pro_set_bt_state(LVGL_BT_STATE_IDLE);
    }
    lv_pro_bt_freshui_ack();
}

static void lv_pro_res_bt_test_a2dp_sink_audio_state_cb(const char *bd_addr,
                                                        btmg_a2dp_sink_audio_state_t state)
{
    if (state == BTMG_A2DP_SINK_AUDIO_SUSPENDED) {
        BT_INF("A2DP sink audio suspended with device: %s\n", bd_addr);
    } else if (state == BTMG_A2DP_SINK_AUDIO_STOPPED) {
        BT_INF("A2DP sink audio stopped with device: %s\n", bd_addr);
    } else if (state == BTMG_A2DP_SINK_AUDIO_STARTED) {
        BT_INF("A2DP sink audio started with device: %s\n", bd_addr);
    }
}

static void lv_pro_res_bt_test_avrcp_play_state_cb(const char *bd_addr,
                                                   btmg_avrcp_play_state_t state)
{
    if (state == BTMG_AVRCP_PLAYSTATE_STOPPED) {
        BT_INF("BT playing music stopped with device: %s\n", bd_addr);
    } else if (state == BTMG_AVRCP_PLAYSTATE_PLAYING) {
        BT_INF("BT palying music playing with device: %s\n", bd_addr);
    } else if (state == BTMG_AVRCP_PLAYSTATE_PAUSED) {
        BT_INF("BT palying music paused with device: %s\n", bd_addr);
    } else if (state == BTMG_AVRCP_PLAYSTATE_FWD_SEEK) {
        BT_INF("BT palying music FWD SEEK with device: %s\n", bd_addr);
    } else if (state == BTMG_AVRCP_PLAYSTATE_REV_SEEK) {
        BT_INF("BT palying music REV SEEK with device: %s\n", bd_addr);
    } else if (state == BTMG_AVRCP_PLAYSTATE_FORWARD) {
        BT_INF("BT palying music forward with device: %s\n", bd_addr);
    } else if (state == BTMG_AVRCP_PLAYSTATE_BACKWARD) {
        BT_INF("BT palying music backward with device: %s\n", bd_addr);
    } else if (state == BTMG_AVRCP_PLAYSTATE_ERROR) {
        BT_INF("BT palying music ERROR with device: %s\n", bd_addr);
    }
}
static void lv_pro_res_bt_test_avrcp_track_changed_cb(const char *bd_addr,
                                                      btmg_track_info_t track_info)
{
    BT_DBG("BT playing device: %s\n", bd_addr);
    BT_INF("BT playing music title: %s\n", track_info.title);
    BT_INF("BT playing music artist: %s\n", track_info.artist);
    BT_INF("BT playing music album: %s\n", track_info.album);
    BT_DBG("BT playing music track number: %s\n", track_info.track_num);
    BT_DBG("BT playing music total number of tracks: %s\n", track_info.num_tracks);
    BT_DBG("BT playing music genre: %s\n", track_info.genre);
    BT_DBG("BT playing music duration: %s\n", track_info.duration);
}

static void lv_pro_res_bt_test_avrcp_audio_volume_cb(const char *bd_addr, unsigned int volume)
{
    BT_INF("AVRCP audio volume:%s : %u\n", bd_addr, volume);
}

static void lv_pro_res_bt_test_avrcp_play_position_cb(const char *bd_addr, int song_len,
                                                      int song_pos)
{
    if (song_playing_pos > song_pos && song_playing_len != song_len) {
        BT_INF("AVRCP playing song switched\n");
    }
    song_playing_len = song_len;
    song_playing_pos = song_pos;
    BT_DBG("%s,playing song len:%d,position:%d\n", bd_addr, song_len, song_pos);
}

static int _bt_init(int profile)
{
    bt_manager_set_loglevel(BTMG_LOG_LEVEL_DEBUG);

    btmg_set_log_file_path("/tmp/btmg.log");
    if (bt_manager_preinit(&bt_callback) != 0) {
        BT_DBG("bt preinit failed!\n");
        return -1;
    }

    if (profile == 0) {
        bt_manager_set_default_profile(true);
    } else {
        bt_manager_enable_profile(profile);
    }
    bt_callback->btmg_adapter_cb.adapter_state_cb = lv_pro_res_bt_test_adapter_status_cb;

    bt_callback->btmg_gap_cb.gap_scan_status_cb = lv_pro_res_bt_test_scan_status_cb;
    bt_callback->btmg_gap_cb.gap_device_add_cb = lv_pro_res_bt_test_dev_add_cb;
    bt_callback->btmg_gap_cb.gap_device_remove_cb = lv_pro_res_bt_test_dev_remove_cb;
    bt_callback->btmg_gap_cb.gap_update_rssi_cb = lv_pro_res_bt_test_update_rssi_cb;
    bt_callback->btmg_gap_cb.gap_bond_state_cb = lv_pro_res_bt_test_bond_state_cb;
    /* bt security callback setting.*/
    bt_callback->btmg_agent_cb.agent_request_pincode = lv_pro_res_bt_test_agent_request_pincode_cb;
    bt_callback->btmg_agent_cb.agent_display_pincode = lv_pro_res_bt_test_agent_display_pincode_cb;
    bt_callback->btmg_agent_cb.agent_request_passkey = lv_pro_res_bt_test_agent_request_passkey_cb;
    bt_callback->btmg_agent_cb.agent_display_passkey = lv_pro_res_bt_test_agent_display_passkey_cb;
    bt_callback->btmg_agent_cb.agent_request_confirm_passkey =
        lv_pro_res_bt_test_agent_request_confirm_passkey_cb;
    bt_callback->btmg_agent_cb.agent_request_authorize =
        lv_pro_res_bt_test_agent_request_authorize_cb;
    bt_callback->btmg_agent_cb.agent_authorize_service =
        lv_pro_res_bt_test_agent_authorize_service_cb;
    /* bt a2dp sink callback*/
    if (bt_pro_info->is_a2dp_sink_enabled) {
        bt_callback->btmg_a2dp_sink_cb.a2dp_sink_connection_state_cb =
            lv_pro_res_bt_test_a2dp_sink_connection_state_cb;
        bt_callback->btmg_a2dp_sink_cb.a2dp_sink_audio_state_cb =
            lv_pro_res_bt_test_a2dp_sink_audio_state_cb;
    }

    /* bt a2dp source callback*/
    if (bt_pro_info->is_a2dp_source_enabled) {
        bt_callback->btmg_a2dp_source_cb.a2dp_source_connection_state_cb =
            lv_pro_res_bt_test_a2dp_source_connection_state_cb;
    }

    /* bt avrcp callback*/
    if (bt_pro_info->is_a2dp_sink_enabled || bt_pro_info->is_a2dp_source_enabled) {
        bt_callback->btmg_avrcp_cb.avrcp_play_state_cb = lv_pro_res_bt_test_avrcp_play_state_cb;
        bt_callback->btmg_avrcp_cb.avrcp_play_position_cb =
            lv_pro_res_bt_test_avrcp_play_position_cb;
        bt_callback->btmg_avrcp_cb.avrcp_track_changed_cb =
            lv_pro_res_bt_test_avrcp_track_changed_cb;
        bt_callback->btmg_avrcp_cb.avrcp_audio_volume_cb = lv_pro_res_bt_test_avrcp_audio_volume_cb;
    }

    if (bt_manager_init(bt_callback) != 0) {
        BT_DBG("bt manager init failed.\n");
        return -1;
    }

    factory_bt_callback = bt_callback;

    lv_pro_bt_discovered_devices = btmg_dev_list_new();
    if (lv_pro_bt_discovered_devices == NULL) goto Failed;

    return 0;
Failed:
    bt_manager_deinit(bt_callback);
    return -1;
}

static void _bt_deinit()
{
    BT_INF("ENTER...\n");
#ifdef ENABLE_LE_BLUETOOTH
    if (bt_pro_info->is_gatts_enabled) bt_gatt_server_deinit();
    if (bt_pro_info->is_gattc_enabled) bt_gatt_client_deinit();
#endif
    if (bt_pro_info->is_a2dp_sink_enabled || bt_pro_info->is_a2dp_source_enabled ||
        bt_pro_info->is_avrcp_enabled || bt_pro_info->is_sppc_enabled ||
        bt_pro_info->is_smp_enabled) {
#ifdef ENABLE_CLASSIC_BLUETOOTH
        bt_manager_enable(false);
        pthread_mutex_lock(&lv_pro_bt_discovered_devices_mutex);
        btmg_dev_list_free(lv_pro_bt_discovered_devices);
        lv_pro_bt_discovered_devices = NULL;
        pthread_mutex_unlock(&lv_pro_bt_discovered_devices_mutex);
#endif
    }
#ifdef ENABLE_LE_BLUETOOTH
    else if (bt_pro_info->is_gatts_enabled || bt_pro_info->is_gattc_enabled) {
        system("/etc/bluetooth/bt_init.sh stop");
    }
#endif
    bt_manager_deinit(bt_callback);
    factory_bt_callback = NULL;
    btmg_log_stop();
}

int lv_pro_res_bt_init(void)
{
    if (isbtInit) {
        BT_WRN("%s:bt already init\n", __func__);
    }

    if (bt_mode) {
        BT_DBG("%s:sink init...\n", __func__);
        return _bt_init(BTMG_A2DP_SINK_ENABLE | BTMG_AVRCP_ENABLE);
    } else {
        BT_DBG("%s:source init...\n", __func__);
        return _bt_init(BTMG_A2DP_SOUCE_ENABLE | BTMG_AVRCP_ENABLE);
    }
    isbtInit = true;
}

int lv_pro_res_bt_deinit(void)
{
    if (isbtInit) _bt_deinit();
    isbtInit = false;

    return 0;
}

int lv_pro_res_bt_on(void) { return bt_manager_enable(true); }

int lv_pro_res_bt_off(void) { return bt_manager_enable(false); }

int lv_pro_res_bt_scan(int scan)
{
    btmg_scan_filter_t scan_filter = {0};
    scan_filter.type = BTMG_SCAN_BR_EDR;
    scan_filter.rssi = -90;

    if (scan) {
        bt_manager_scan_filter(&scan_filter);
        return bt_manager_start_scan();
    } else {
        return bt_manager_stop_scan();
    }
}

int lv_pro_res_bt_connect(char *mac_addr) { return bt_manager_connect(mac_addr); }

int lv_pro_res_bt_disconnect(char *mac_addr) { return bt_manager_disconnect(mac_addr); }

int lv_pro_res_bt_unpair(char *mac_addr) { return bt_manager_unpair(mac_addr); }

static int compare_connected(const void *a, const void *b)
{
    const btmg_bt_device_t *dev_a = (const btmg_bt_device_t *)a;
    const btmg_bt_device_t *dev_b = (const btmg_bt_device_t *)b;
    return (dev_b->connected - dev_a->connected);
}

void lv_pro_res_bt_paired_devices(btmg_bt_device_t **devices, int *count)
{
    bt_manager_get_paired_devices(devices, count);
    if (*count > 0) {
        qsort(*devices, *count, sizeof(btmg_bt_device_t), compare_connected);
    }
}

void *free_devices_thread(void *arg)
{
    /*Use threads to handle blocking interfaces to prevent the UI from freezing when refreshing
     * paired devices during Bluetooth reconnection*/
    BT_DBG("start\n");
    FreeDevicesArgs *args = (FreeDevicesArgs *)arg;
    bt_manager_free_paired_devices(args->devices, args->count);
    free(args);
    BT_DBG("finish\n");
    pthread_exit((void *)1);
    return NULL;
}

void lv_pro_res_bt_free_devices(btmg_bt_device_t *devices, int count)
{
    FreeDevicesArgs *args = malloc(sizeof(FreeDevicesArgs));
    args->devices = devices;
    args->count = count;

    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, free_devices_thread, args) != 0) {
        BT_ERR("failed to create thread");
        free(args);
    }
}

int lv_pro_res_bt_disconnect_old_connection()
{
    char devMac[18];
    char devName[256];

    int ret = get_file_bt_connectstate(devMac, sizeof(devMac), devName, sizeof(devName));
    BT_DBG("ret=%d devMac=%s devName=%s\n", ret, devMac, devName);
    if (ret == 1) {
        lv_pro_res_bt_disconnect(devMac);
    }

    update_file_bt_connectstate(BT_STATE_DISCONN, NULL, NULL);
    return ret;
}

int lv_pro_res_bt_get_devname(const char *addr, char *name)
{
    bt_manager_get_device_name(addr, name);
}

void lv_pro_res_bt_logo_show(bool conn)
{
    char *dev_name = (char *)malloc(256);
    if (dev_name == NULL) {
        BT_ERR("Error: Failed to allocate memory for dev_name\n");
        return;
    }
    memset(dev_name, '\0', 256);

    if (conn) {
        char dev_mac[18];
        int ret = get_file_bt_connectstate(dev_mac, 18, dev_name, 256);
        BT_DBG("ret=%d dev_mac=%s dev_name=%s\n", ret, dev_mac, dev_name);
    }

    if (bt_show) {
        main_page_prompt_data_t *msg =
            (main_page_prompt_data_t *)malloc(sizeof(main_page_prompt_data_t));
        if (msg == NULL) {
            BT_ERR("Error: Failed to allocate memory for msg\n");
            free(dev_name);
            return;
        }

        msg->status = (conn) ? BT_STATE_CONN : BT_STATE_DISCONN;
        msg->devName = dev_name;

        lv_event_send(bt_show, LV_EVENT_REFRESH, msg);
    } else {
        free(dev_name);
    }
}
#endif

#endif /* ENABLE_BT */