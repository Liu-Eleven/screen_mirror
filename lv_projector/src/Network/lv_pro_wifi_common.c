
#include "lv_pro_wifi_common.h"

#if ENABLE_WIFI

extern lv_obj_t *lv_pro_wifi_activity;
extern lv_group_t *wifi_menu_group;
extern lv_group_t *wifi_paired_list_group;
extern lv_group_t *wifi_scan_list_group;

#ifdef LVGL_PROJECTOR_WIFI_CONFIG_PATH
static const char *config_path = LVGL_PROJECTOR_WIFI_CONFIG_PATH;
#else
static const char *config_path = "/etc/wifi";
#endif

static const char *wifi_resource_paths[] = {
    [WIFI_OPEN_LOGO] = "A:/usr/share/lv_projector/wifi_logo_open.png",
    [WIFI_CLOSE_LOGO] = "A:/usr/share/lv_projector/wifi_logo_close.png",
    [WIFI_SEARCH_LOGO] = "A:/usr/share/lv_projector/search_fresh.png",
    [WIFI_ADDNETWORK_LOGO] = "A:/usr/share/lv_projector/add_network.png",
    [WIFI_BACKHOME_LOGO] = "A:/usr/share/lv_projector/backhome.png",
    [WIFI_SCAN0_LOGO] = "A:/usr/share/lv_projector/wifi_scan0_logo.png",
    [WIFI_SCAN1_LOGO] = "A:/usr/share/lv_projector/wifi_scan1_logo.png",
    [WIFI_SCAN2_LOGO] = "A:/usr/share/lv_projector/wifi_scan1_logo.png",
    [WIFI_SCAN3_LOGO] = "A:/usr/share/lv_projector/wifi_scan3_logo.png"};

char *lv_pro_wifi_get_resource_path(WIFIResourceType type)
{
    if (type < WIFI_RESOURCE_COUNT) {
        return wifi_resource_paths[type];
    }
    return NULL;
}

const char *lv_pro_wifi_msg_type_to_string(wifi_lvgl_msg_type_t msg_type)
{
    switch (msg_type) {
    case MSG_TYPE_WIFI_WORK_STATE_CLOSE:
        return "MSG_TYPE_WIFI_WORK_STATE_CLOSE";
    case MSG_TYPE_WIFI_WORK_STATE_CLOSESUCCESS:
        return "MSG_TYPE_WIFI_WORK_STATE_CLOSESUCCESS";
    case MSG_TYPE_WIFI_WORK_STATE_CLOSEFAIL:
        return "MSG_TYPE_WIFI_WORK_STATE_CLOSEFAIL";
    case MSG_TYPE_WIFI_WORK_STATE_OPEN:
        return "MSG_TYPE_WIFI_WORK_STATE_OPEN";
    case MSG_TYPE_WIFI_WORK_STATE_OPENSUCCESS:
        return "MSG_TYPE_WIFI_WORK_STATE_OPENSUCCESS";
    case MSG_TYPE_WIFI_WORK_STATE_OPENFAIL:
        return "MSG_TYPE_WIFI_WORK_STATE_OPENFAIL";
    case MSG_TYPE_WIFI_MODE_SWITCH:
        return "MSG_TYPE_WIFI_MODE_SWITCH";
    case MSG_TYPE_WIFI_MODE_SWITCH_BTN_ENABLE:
        return "MSG_TYPE_WIFI_MODE_SWITCH_BTN_ENABLE";

    case MSG_TYPE_WIFI_CLEARHIDDEN_SCANSTATE_OBJ:
        return "MSG_TYPE_WIFI_CLEARHIDDEN_SCANSTATE_OBJ";
    case MSG_TYPE_WIFI_HIDDEN_ALL_SCANDEV_OBJ:
        return "MSG_TYPE_WIFI_HIDDEN_ALL_SCANDEV_OBJ";
    case MSG_TYPE_WIFI_FOCUSID_SCANDEV_OBJ:
        return "MSG_TYPE_WIFI_FOCUSID_SCANDEV_OBJ";
    case MSG_TYPE_WIFI_HIDDEN_DEV_OBJ:
        return "MSG_TYPE_WIFI_HIDDEN_DEV_OBJ";
    case MSG_TYPE_WIFI_HIDDEN_SCANSTATE_OBJ:
        return "MSG_TYPE_WIFI_HIDDEN_SCANSTATE_OBJ";
    case MSG_TYPE_WIFI_UPDATE_SCANDEV_OBJ:
        return "MSG_TYPE_WIFI_UPDATE_SCANDEV_OBJ";
    case MSG_TYPE_WIFI_UPDATE_SCANDEVNUM_OBJ:
        return "MSG_TYPE_WIFI_UPDATE_SCANDEVNUM_OBJ";

    case MSG_TYPE_WIFI_DELETE_PAIR_DEV:
        return "MSG_TYPE_WIFI_DELETE_PAIR_DEV";
    case MSG_TYPE_WIFI_HIDDEN_ALL_PAIRDEV_OBJ:
        return "MSG_TYPE_WIFI_HIDDEN_ALL_PAIRDEV_OBJ";
    case MSG_TYPE_WIFI_UPDATE_PAIRDEV_OBJ:
        return "MSG_TYPE_WIFI_UPDATE_PAIRDEV_OBJ";
    case MSG_TYPE_WIFI_UPDATE_PAIRNUMBER_OBJ:
        return "MSG_TYPE_WIFI_UPDATE_PAIRNUMBER_OBJ";

    case MSG_TYPE_WIFI_CHANGED_DEV_STATE:
        return "MSG_TYPE_WIFI_CHANGED_DEV_STATE";
    case MSG_TYPE_WIFI_CHANGED_HOME_CONNLOGO:
        return "MSG_TYPE_WIFI_CHANGED_HOME_CONNLOGO";
    case MSG_TYPE_WIFI_CHANGED_HOME_DISCONNLOGO:
        return "MSG_TYPE_WIFI_CHANGED_HOME_DISCONNLOGO";
    case MSG_TYPE_WIFI_CHANGED_TO_MENUGROUP:
        return "MSG_TYPE_WIFI_CHANGED_TO_MENUGROUP";
    case MSG_TYPE_WIFI_CHANGED_TO_RIGHTGROUP:
        return "MSG_TYPE_WIFI_CHANGED_TO_RIGHTGROUP";
    case MSG_TYPE_WIFI_TRY_GRUOP_FOCUS:
        return "MSG_TYPE_WIFI_TRY_GRUOP_FOCUS";

    case MSG_TYPE_WIFI_CLICKED_PAIRDEV:
        return "MSG_TYPE_WIFI_CLICKED_PAIRDEV";
    case MSG_TYPE_WIFI_CONN_PAIRDEV:
        return "MSG_TYPE_WIFI_CONN_PAIRDEV";
    case MSG_TYPE_WIFI_NEWCONN_PAIRDEV:
        return "MSG_TYPE_WIFI_NEWCONN_PAIRDEV";
    case MSG_TYPE_WIFI_DISCONN_PAIRDEV:
        return "MSG_TYPE_WIFI_DISCONN_PAIRDEV";
    case MSG_TYPE_WIFI_DELETED_PAIRDEV:
        return "MSG_TYPE_WIFI_DELETED_PAIRDEV";
    case MSG_TYPE_WIFI_CLICKER_SCANDEV:
        return "MSG_TYPE_WIFI_CLICKER_SCANDEV";
    case MSG_TYPE_WIFI_CONN_SCANDEV:
        return "MSG_TYPE_WIFI_CONN_SCANDEV";
    case MSG_TYPE_WIFI_CONN_SCANDEV_FAIL:
        return "MSG_TYPE_WIFI_CONN_SCANDEV_FAIL";
    case MSG_TYPE_WIFI_CREARE_PAIRSTATE_PROC:
        return "MSG_TYPE_WIFI_CREARE_PAIRSTATE_PROC";
    case MSG_TYPE_WIFI_CREARE_SCANDEV_PROC:
        return "MSG_TYPE_WIFI_CREARE_SCANDEV_PROC";
    case MSG_TYPE_WIFI_MANUALLY_ADD_DEV:
        return "MSG_TYPE_WIFI_MANUALLY_ADD_DEV";
    case MSG_TYPE_WIFI_LVGL_UI_DEINIT:
        return "MSG_TYPE_WIFI_LVGL_UI_DEINIT";
    case MSG_TYPE_WIFI_LVGL_UI_INIT:
        return "MSG_TYPE_WIFI_LVGL_UI_INIT";
    case MSG_TYPE_WIFI_SET_DEV_NOTSAVESTATE:
        return "MSG_TYPE_WIFI_SET_DEV_NOTSAVESTATE";
    case MSG_TYPE_WIFI_CONN_PAIRDEV_FAIL:
        return "MSG_TYPE_WIFI_CONN_PAIRDEV_FAIL";
    case MSG_TYPE_WIFI_DISCONN_PAIRDEV_FAIL:
        return "MSG_TYPE_WIFI_DISCONN_PAIRDEV_FAIL";
    case MSG_TYPE_WIFI_STOP_PAIRPROC:
        return "MSG_TYPE_WIFI_STOP_PAIRPROC";
    case MSG_TYPE_WIFI_STOP_SCANPROC:
        return "MSG_TYPE_WIFI_STOP_SCANPROC";
    case MSG_TYPE_WIFI_STATE_CONNECTED:
        return "MSG_TYPE_WIFI_STATE_CONNECTED";
    case MSG_TYPE_WIFI_STATE_DISCONNED:
        return "MSG_TYPE_WIFI_STATE_DISCONNED";
    case MSG_TYPE_WIFI_LV_KEY_UP:
        return "MSG_TYPE_WIFI_LV_KEY_UP";
    case MSG_TYPE_WIFI_LV_KEY_DOWN:
        return "MSG_TYPE_WIFI_LV_KEY_DOWN";

    default:
        return "UNKNOWN_MSG_TYPE";
    }
}

int lv_pro_wifi_msg_enqueue(wifi_lvgl_msg_type_t type, void *data, bool free_data)
{
    if (!lv_pro_wifi_activity &&
        (type != MSG_TYPE_WIFI_STATE_CONNECTED && type != MSG_TYPE_WIFI_STATE_DISCONNED)) {
        if (free_data) free(data);
        WIFI_WRN("ui not active!,need ignore\n");
        return;
    }

    lvgl_wifi_msg_t *_msg = (lvgl_wifi_msg_t *)malloc(sizeof(lvgl_wifi_msg_t));
    _msg->type = type;
    _msg->data = data;
    _msg->free_data = free_data;
    if (type == MSG_TYPE_WIFI_STATE_CONNECTED) {
        system_send_msg(MSG_TYPE_WIFI_STA_CONNECT, _msg);
    } else if (type == MSG_TYPE_WIFI_STATE_DISCONNED) {
        system_send_msg(MSG_TYPE_WIFI_STA_DISCONNECT, _msg);
    } else {
        system_send_msg(MSG_TYPE_WIFI_FRESH_UI, _msg);
    }
}

int lv_pro_wifi_group_devid(lv_group_t *group, lv_obj_t *obj)
{
    int id = 0;

    if (obj == NULL || group == NULL) return -1;

    LV_LOG_TRACE("begin");

    lv_obj_t **obj_i;
    _LV_LL_READ(&group->obj_ll, obj_i)
    {
        if (!((*obj_i)->flags & LV_OBJ_FLAG_HIDDEN)) {
            id++;
        }
        if ((*obj_i) == obj) {
            return id;
        }
    }
    return -1;
}

int lv_pro_wifi_group_num_all(lv_group_t *group)
{
    int id = 0;

    if (group == NULL) return -1;

    LV_LOG_TRACE("begin");

    lv_obj_t **obj_i;
    _LV_LL_READ(&group->obj_ll, obj_i)
    {
        if (!((*obj_i)->flags & LV_OBJ_FLAG_HIDDEN)) {
            id++;
        }
    }
    return id;
}

static int focus_on_object(lv_group_t *group, lv_obj_t *obj)
{
    lv_group_defocus_obj(NULL);
    lv_indev_set_group(evdev_indev, group);
    lv_group_set_default(group);
    lv_group_focus_obj(obj);
    return 0;
}

int lv_pro_wifi_group_change(lv_group_t *group, bool back_found, lv_obj_t *obj)
{
    if (group == NULL) return -1;

    if (obj == NULL) obj = lv_group_get_focused(group);

    if (obj != NULL && !(obj->flags & LV_OBJ_FLAG_HIDDEN)) {
        return focus_on_object(group, obj);
    }

    lv_obj_t **obj_i;
    if (!back_found) {
        _LV_LL_READ(&group->obj_ll, obj_i)
        {
            if (!((*obj_i)->flags & LV_OBJ_FLAG_HIDDEN)) {
                return focus_on_object(group, *obj_i);
            }
        }
    } else {
        _LV_LL_READ_BACK(&group->obj_ll, obj_i)
        {
            if (!((*obj_i)->flags & LV_OBJ_FLAG_HIDDEN)) {
                return focus_on_object(group, *obj_i);
            }
        }
    }
    lv_obj_t *current_obj = lv_group_get_focused(lv_group_get_default());
    lv_group_set_editing(lv_group_get_default(), false);
    lv_group_focus_obj(current_obj);
    return -1;
}

void lv_pro_wifi_group_printf_debug(lv_obj_t *group, int direction)
{
#ifdef LV_PRO_WIFI_ACTIVITY_GROUP_DEBUG

    if (direction) {
        WIFI_DBG("key in:");
    } else {
        WIFI_DBG("key out:");
    }

    int value = -1;
    lv_obj_t *obj = lv_group_get_focused(group);

    if (obj) {
        value = lv_group_get_obj_id(group, obj);
    }

    if (group == wifi_menu_group) {
        printf(" WiFi Menu Groupid=%d\n", value);
    } else if (group == wifi_scan_list_group) {
        printf(" WiFi Scan List Groupid=%d\n", value);
    } else if (group == wifi_paired_list_group) {
        printf(" WiFi Paired List Groupid=%d\n", value);
    } else {
        printf(" WiFi NULL Group ERRid=%d\n", value);
    }
#endif
}

int lv_pro_wifi_group_user_focus_check_cb(lv_group_t *group)
{
    return 1;  // 返回1，交给2级处理
}

void lv_pro_wifi_group_obj_clear_focus(lv_group_t *group)
{
    if (group == NULL) return -1;
    lv_group_set_editing(group, false);

    lv_obj_t *obj = lv_group_get_focused(group);
    if (obj) {
        lv_res_t res = lv_event_send(obj, LV_EVENT_DEFOCUSED, NULL);
        if (res != LV_RES_OK) return;
        lv_obj_invalidate(obj);
    }
    group->obj_focus = NULL;
}

// update obj
void lv_pro_wifi_hidden_all_devobj(lv_obj_t **items, int length)
{
    if (items == NULL) {
        WIFI_ERR("items NULL!\n");
        return;
    }

    if (length <= 0) {
        WIFI_ERR("Invalid length: %d\n", length);
        return;
    }

    for (int i = 0; i < length; i++) {
        if (items[i] == NULL) {
            // WIFI_WRN("Item at index %d is NULL, skipping.\n", i);
            continue;
        }
        lv_obj_add_flag(items[i], LV_OBJ_FLAG_HIDDEN);
    }
}

void lv_pro_wifi_clear_devobj(void *obj)
{
    lv_obj_t *item = (lv_obj_t *)obj;

    if (item == NULL) {
        WIFI_ERR("items NULL!\n");
        return;
    }

    lv_pro_set_btn_style6_set_name_str(item, "");
    lv_pro_set_btn_style6_set_mac_addr_str(item, "");
    lv_pro_set_btn_style6_set_state_str(item, "");
    lv_pro_set_btn_style6_set_sec_value(item, 0);
    lv_pro_set_btn_style6_set_rssi_value(item, 0);
    lv_obj_add_flag(item, LV_OBJ_FLAG_HIDDEN);
}

void lv_pro_wifi_update_icon(void *obj, int rssi)
{
    lv_pro_set_btn_style6_t *btn = (lv_pro_set_btn_style6_t *)obj;

    if (btn == NULL) {
        WIFI_ERR("btn NULL!\n");
        return;
    }

    if (rssi >= RSSI_HIGH_THRESHOLD) {
        lv_pro_set_btn_style6_set_btn_icon(btn, lv_pro_wifi_get_resource_path(WIFI_SCAN0_LOGO));
    } else if (rssi >= RSSI_MEDIUM_THRESHOLD) {
        lv_pro_set_btn_style6_set_btn_icon(btn, lv_pro_wifi_get_resource_path(WIFI_SCAN1_LOGO));
    } else if (rssi >= RSSI_LOW_THRESHOLD) {
        lv_pro_set_btn_style6_set_btn_icon(btn, lv_pro_wifi_get_resource_path(WIFI_SCAN2_LOGO));
    } else {
        lv_pro_set_btn_style6_set_btn_icon(btn, lv_pro_wifi_get_resource_path(WIFI_SCAN3_LOGO));
    }
}

void lv_pro_wifi_update_devobj(lv_obj_t *item, WIFIDeviceMsg *device)
{
    if (item == NULL) {
        WIFI_ERR("items NULL!\n");
        return;
    }
    lv_pro_wifi_update_icon((void *)item, device->rssi);
    lv_pro_set_btn_style6_set_name_str(item, device->ssid);
    lv_pro_set_btn_style6_set_mac_addr_str(item, device->bssid);

    const char *state_str = (device->status == WIFI_STATE_CONNECTED) ? lv_get_string(STR_WIFI_CONN)
                            : (device->status == WIFI_STATE_SAVED)   ? lv_get_string(STR_WIFI_SAVED)
                                                                     : NULL;
    if (!state_str) {
        state_str = device->mgmt;
    }
    lv_pro_set_btn_style6_set_state_str(item, state_str);
    lv_pro_set_btn_style6_set_sec_value(item, device->sec);
    lv_pro_set_btn_style6_set_rssi_value(item, device->rssi);
    lv_obj_clear_flag(item, LV_OBJ_FLAG_HIDDEN);
}

void lv_pro_wifi_set_logostate(void *obj, int state)
{
    lv_pro_set_btn_style3_t *btn = (lv_pro_set_btn_style3_t *)obj;

    if (!btn) return;

    lv_obj_clear_state(btn, LV_STATE_DISABLED);
    lv_obj_clear_state(btn->btn, LV_STATE_DISABLED);

    switch (state) {
    case 0: {
        lv_obj_clear_state(btn->btn, LV_STATE_CHECKED);
        lv_img_set_src(btn->img, btn->src_off_img);
        break;
    }
    case 1: {
        lv_obj_add_state(btn->btn, LV_STATE_CHECKED);
        lv_img_set_src(btn->img, btn->src_on_img);
        break;
    }
    case 2: {
        if (!lv_obj_has_state(btn->btn, LV_STATE_CHECKED)) {
            lv_obj_add_state(btn->btn, LV_STATE_CHECKED);
            lv_img_set_src(btn->img, btn->src_on_img);
        } else {
            lv_obj_clear_state(btn->btn, LV_STATE_CHECKED);
            lv_img_set_src(btn->img, btn->src_off_img);
        }
        break;
    }
    }
}

WIFIDeviceMsg *lv_pro_wifi_scandev_init(wifi_scan_result_t *dev, int dev_id)
{
    WIFIDeviceMsg *device = malloc(sizeof(WIFIDeviceMsg));
    device->id = dev_id;
    snprintf(device->ssid, sizeof(device->ssid), "%s", dev->ssid);
    lv_pro_res_wifi_uint8tochar(device->bssid, dev->bssid);
    device->freq = dev->freq;
    device->rssi = dev->rssi;
    device->status = WIFI_STATE_UNSAVE;
    memset(device->mgmt, 0, sizeof(device->mgmt));
    key_mgmt_to_char(device->mgmt, dev->key_mgmt);
    device->scan_action = dev->scan_action;
    device->sec = lv_pro_res_wifi_get_security_type(dev->key_mgmt);
    return device;
}

char *get_wireless_ko_path(void)
{
    char *file_path = (char *)malloc(256);

    snprintf(file_path, 256, "%s/load_wifi_modules.sh", config_path);

    return file_path;
}

void update_file_wifi_state(const char *state)
{
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/wifi_state_on", config_path);

    FILE *file = fopen(file_path, "w");
    if (file == NULL) {
        perror("Failed to open file");
        return;
    }
    fprintf(file, "%s\n", state);
    fclose(file);
}

int get_file_wifi_state()
{
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/wifi_state_on", config_path);

    FILE *file = fopen(file_path, "r+");

    if (file == NULL) {
        // File doesn't exist, create it and set wifi to off
        file = fopen(file_path, "w");
        if (file == NULL) {
            perror("Failed to create file");
            return -1;
        }
        fprintf(file, "%s\n", WIFI_STATE_OFF);
        fclose(file);
        return 0;
    }

    int state_result;
    char state[4];

    if (fgets(state, sizeof(state), file) != NULL) {
        state[strcspn(state, "\n")] = 0;
        if (strcmp(state, WIFI_STATE_ON) == 0) {
            state_result = 1;
        } else {
            state_result = 0;
        }
    } else {
        state_result = -1;
    }

    fclose(file);
    return state_result;
}

// 将 UTF-8 字符串转为小写十六进制格式
void utf8_to_hex(const char *input, char *hex_buffer, size_t buffer_size)
{
    if (buffer_size < 3) {
        hex_buffer[0] = '\0';
        return;
    }
    for (size_t i = 0; input[i] != '\0' && (i * 2 + 2) < buffer_size; i++) {
        sprintf(hex_buffer + i * 2, "%02x", (unsigned char)input[i]);
    }
    hex_buffer[strlen(hex_buffer)] = '\0';
}

bool is_hex_string(const char *str)
{
    for (const char *p = str; *p; p++) {
        if (!isxdigit(*p)) return false;
    }
    return true;
}

int lv_pro_res_wifi_get_dev(const char *ssid, char *pwd, char *key_mgnt)
{
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/wpa_supplicant/wpa_supplicant.conf", config_path);

    FILE *file = fopen(file_path, "r");
    if (!file) {
        perror("Failed to open config file");
        return 0;
    }

    char hex_ssid[512] = {0};
    utf8_to_hex(ssid, hex_ssid, sizeof(hex_ssid));
    char quoted_ssid[256];
    snprintf(quoted_ssid, sizeof(quoted_ssid), "\"%s\"", ssid);

    char line[256];
    int found = 0;

    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "ssid=")) {
            char *equal_pos = strchr(line, '=');
            if (!equal_pos) continue;
            equal_pos++;

            // 提取ssid值（可能带引号或不带）
            char *start = equal_pos;
            while (*start == ' ' || *start == '\t') start++;  // 跳过空格

            char *end = start;
            while (*end && *end != '\n' && *end != ' ' && *end != '\t' && *end != '#')
                end++;  // 找到值结尾
            size_t len = end - start;

            char current_ssid[512];
            strncpy(current_ssid, start, len);
            current_ssid[len] = '\0';

            // 情况1：带引号的ASCII SSID（如 "MyWiFi"）
            if (current_ssid[0] == '"' && current_ssid[len - 1] == '"') {
                current_ssid[len - 1] = '\0';               // 去掉尾部引号
                if (strcmp(current_ssid + 1, ssid) == 0) {  // 去掉头部引号比较
                    found = 1;
                }
            }

            else if (is_hex_string(current_ssid)) {
                if (strcasecmp(current_ssid, hex_ssid) == 0) {
                    found = 1;
                }
            }
        }

        if (found && strstr(line, "psk=")) {
            char *start = strchr(line, '"');
            if (start) {
                start++;
                char *end = strchr(start, '"');
                if (end) {
                    strncpy(pwd, start, end - start);
                    pwd[end - start] = '\0';
                }
            } else {
                sscanf(line, " psk=%255s", pwd);
            }
        }

        if (found && strstr(line, "proto=")) {
            char proto[20];
            sscanf(line, " proto=%19s", proto);
            if (strcasecmp(proto, "RSN") == 0) {
                strcpy(key_mgnt, "WPA2-PSK");
                fclose(file);
                return 1;
            }
        }

        if (found && strstr(line, "key_mgmt=")) {
            sscanf(line, " key_mgmt=%255s", key_mgnt);
            fclose(file);
            return 1;
        }
    }

    fclose(file);
    return found ? 1 : 0;
}

#endif /* ENABLE_WIFI */