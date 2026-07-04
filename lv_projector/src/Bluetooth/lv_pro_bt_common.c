/*
 * lv_pro_bt_common.c
 *
 *  Created on: 2024/01/08
 *      Author: JASON
 */

#include "lv_pro_bt_common.h"

#if ENABLE_BT

SelectDeviceInfo selectBT = {0};

extern lv_obj_t *lv_pro_bt_activity;
extern lv_group_t *bt_menu_group;
extern lv_group_t *bt_paired_list_group;
extern lv_group_t *bt_scan_list_group;

#ifdef LVGL_PROJECTOR_BT_CONFIG_PATH
static const char *config_path = LVGL_PROJECTOR_BT_CONFIG_PATH;
#else
static const char *config_path = "/etc/bluetooth";
#endif

const char *resource_paths[] = {
    [BT_OPEN_LOGO] = "A:/usr/share/lv_projector/bt_logo.png",
    [BT_CLOSE_LOGO] = "A:/usr/share/lv_projector/bt_logo.png",
    [BT_SEARCH_LOGO] = "A:/usr/share/lv_projector/search_fresh.png",
    [BT_BACKHOME_LOGO] = "A:/usr/share/lv_projector/backhome.png",
    [BT_SCAN_LOGO_DF] = "A:/usr/share/lv_projector/bt_scan_logo_1.png",
    [BT_SCAN_LOGO_PHONE] = "A:/usr/share/lv_projector/bt_scan_logo_2.png",
    [BT_SCAN_LOGO_HEADSET] = "A:/usr/share/lv_projector/bt_scan_logo_3.png",
    [BT_SCAN_LOGO_COMPUTER] = "A:/usr/share/lv_projector/bt_scan_logo_4.png"};

char *lv_pro_bt_get_resource_path(ResourceType type)
{
    if (type < RESOURCE_COUNT) {
        return resource_paths[type];
    }
    return NULL;
}

char *lv_pro_bt_msg_type_to_string(bt_lvgl_msg_type_t msg_type)
{
    switch (msg_type) {
    case MSG_TYPE_BT_WORK_STATE_CLOSE:
        return "MSG_TYPE_BT_WORK_STATE_CLOSE";
    case MSG_TYPE_BT_WORK_STATE_CLOSESUCCESS:
        return "MSG_TYPE_BT_WORK_STATE_CLOSESUCCESS";
    case MSG_TYPE_BT_WORK_STATE_CLOSEFAIL:
        return "MSG_TYPE_BT_WORK_STATE_CLOSEFAIL";
    case MSG_TYPE_BT_WORK_STATE_OPEN:
        return "MSG_TYPE_BT_WORK_STATE_OPEN";
    case MSG_TYPE_BT_WORK_STATE_OPENSUCCESS:
        return "MSG_TYPE_BT_WORK_STATE_OPENSUCCESS";
    case MSG_TYPE_BT_WORK_STATE_OPENFAIL:
        return "MSG_TYPE_BT_WORK_STATE_OPENFAIL";
    case MSG_TYPE_BT_MODE_SWITCH:
        return "MSG_TYPE_BT_MODE_SWITCH";
    case MSG_TYPE_BT_MODE_SWITCH_BTN_ENABLE:
        return "MSG_TYPE_BT_MODE_SWITCH_BTN_ENABLE";
    case MSG_TYPE_BT_SRC_OPEN_TRY_OPEN:
        return "MSG_TYPE_BT_SRC_OPEN_TRY_OPEN";

    case MSG_TYPE_BT_CLEARHIDDEN_SCANSTATE_OBJ:
        return "MSG_TYPE_BT_CLEARHIDDEN_SCANSTATE_OBJ";
    case MSG_TYPE_BT_HIDDEN_ALL_SCANDEV_OBJ:
        return "MSG_TYPE_BT_HIDDEN_ALL_SCANDEV_OBJ";
    case MSG_TYPE_BT_HIDDEN_DEV_OBJ:
        return "MSG_TYPE_BT_HIDDEN_DEV_OBJ";
    case MSG_TYPE_BT_HIDDEN_SCANSTATE_OBJ:
        return "MSG_TYPE_BT_HIDDEN_SCANSTATE_OBJ";
    case MSG_TYPE_BT_UPDATE_SCANDEV_OBJ:
        return "MSG_TYPE_BT_UPDATE_SCANDEV_OBJ";

    case MSG_TYPE_BT_DELETE_PAIR_DEV:
        return "MSG_TYPE_BT_DELETE_PAIR_DEV";
    case MSG_TYPE_BT_HIDDEN_ALL_PAIRDEV_OBJ:
        return "MSG_TYPE_BT_HIDDEN_ALL_PAIRDEV_OBJ";
    case MSG_TYPE_BT_UPDATE_PAIRDEV_OBJ:
        return "MSG_TYPE_BT_UPDATE_PAIRDEV_OBJ";
    case MSG_TYPE_BT_UPDATE_PAIRNUMBER_OBJ:
        return "MSG_TYPE_BT_UPDATE_PAIRNUMBER_OBJ";

    case MSG_TYPE_BT_CHANGED_DEV_STATE:
        return "MSG_TYPE_BT_CHANGED_DEV_STATE";
    case MSG_TYPE_BT_CHANGED_HOME_CONNLOGO:
        return "MSG_TYPE_BT_CHANGED_HOME_CONNLOGO";
    case MSG_TYPE_BT_CHANGED_HOME_DISCONNLOGO:
        return "MSG_TYPE_BT_CHANGED_HOME_DISCONNLOGO";
    case MSG_TYPE_BT_CHANGED_TO_MENUGROUP:
        return "MSG_TYPE_BT_CHANGED_TO_MENUGROUP";
    case MSG_TYPE_BT_CHANGED_TO_RIGHTGROUP:
        return "MSG_TYPE_BT_CHANGED_TO_RIGHTGROUP";
    case MSG_TYPE_BT_TRY_GRUOP_FOCUS:
        return "MSG_TYPE_BT_TRY_GRUOP_FOCUS";

    case MSG_TYPE_BT_CLICKED_PAIRDEV:
        return "MSG_TYPE_BT_CLICKED_PAIRDEV";
    case MSG_TYPE_BT_CONN_PAIRDEV:
        return "MSG_TYPE_BT_CONN_PAIRDEV";
    case MSG_TYPE_BT_DISCONN_PAIRDEV:
        return "MSG_TYPE_BT_DISCONN_PAIRDEV";
    case MSG_TYPE_BT_DELETED_PAIRDEV:
        return "MSG_TYPE_BT_DELETED_PAIRDEV";
    case MSG_TYPE_BT_CONN_SCANDEV:
        return "MSG_TYPE_BT_CONN_SCANDEV";
    case MSG_TYPE_BT_CREARE_FRESHUI_PROC:
        return "MSG_TYPE_BT_CREARE_FRESHUI_PROC";
    case MSG_TYPE_BT_CREARE_SCANDEV_PROC:
        return "MSG_TYPE_BT_CREARE_SCANDEV_PROC";
    case MSG_TYPE_BT_LVGL_UI_DEINIT:
        return "MSG_TYPE_BT_LVGL_UI_DEINIT";
    case MSG_TYPE_BT_LVGL_UI_INIT:
        return "MSG_TYPE_BT_LVGL_UI_INIT";
    case MSG_TYPE_BT_SET_DEV_NOTSAVESTATE:
        return "MSG_TYPE_BT_SET_DEV_NOTSAVESTATE";
    case MSG_TYPE_BT_SET_DEV_SAVEDSTATE:
        return "MSG_TYPE_BT_SET_DEV_SAVEDSTATE";
    case MSG_TYPE_BT_STOP_PAIRPROC:
        return "MSG_TYPE_BT_STOP_PAIRPROC";
    case MSG_TYPE_BT_STOP_SCANPROC:
        return "MSG_TYPE_BT_STOP_SCANPROC";
    case MSG_TYPE_BT_A2DPSRC_CONNECTED:
        return "MSG_TYPE_BT_A2DPSRC_CONNECTED";
    case MSG_TYPE_BT_A2DPSRC_DISCONNED:
        return "MSG_TYPE_BT_A2DPSRC_DISCONNED";
    case MSG_TYPE_BT_LV_KEY_UP:
        return "MSG_TYPE_BT_LV_KEY_UP";
    case MSG_TYPE_BT_LV_KEY_DOWN:
        return "MSG_TYPE_BT_LV_KEY_DOWN";

    default:
        return "UNKNOWN_MSG_TYPE";
    }
}

int lv_pro_bt_msg_enqueue(bt_lvgl_msg_type_t type, void *data, bool free_data)
{
    /*Avoid thread processing after the page exits. If there is a wild pointer in the obj sent at
     * this time, it should be ignored.*/
    if (!lv_pro_bt_activity &&
        (type != MSG_TYPE_BT_A2DPSRC_CONNECTED && type != MSG_TYPE_BT_A2DPSRC_DISCONNED)) {
        if (free_data) free(data);
        BT_WRN("ui not active!,need ignore\n");
        return;
    }

    lvgl_bt_msg_t *_msg = (lvgl_bt_msg_t *)malloc(sizeof(lvgl_bt_msg_t));
    _msg->type = type;
    _msg->data = data;
    _msg->free_data = free_data;
    if (type == MSG_TYPE_BT_A2DPSRC_CONNECTED) {
        system_send_msg(MSG_TYPE_BT_A2DPSRC_CONNECT, _msg);
    } else if (type == MSG_TYPE_BT_A2DPSRC_DISCONNED) {
        system_send_msg(MSG_TYPE_BT_A2DPSRC_DISCONNECT, _msg);
    } else {
        system_send_msg(MSG_TYPE_BT_FRESH_UI, _msg);
    }
}

static int focus_on_object(lv_group_t *group, lv_obj_t *obj)
{
    lv_group_defocus_obj(NULL);
    lv_indev_set_group(evdev_indev, group);
    lv_group_set_default(group);
    lv_group_focus_obj(obj);
    return 0;
}

bool lv_pro_bt_group_is_active(void)
{
    lv_group_t *current_group = lv_group_get_default();

    if (current_group == bt_menu_group || current_group == bt_paired_list_group ||
        current_group == bt_scan_list_group) {
        return true;
    }

    return false;
}

int lv_pro_bt_group_change(lv_group_t *group, bool back_found, lv_obj_t *obj)
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

void lv_pro_bt_group_printf_debug(lv_obj_t *group, int direction)
{
#ifdef LV_PRO_BT_ACTIVITY_GROUP_DEBUG

    if (direction) {
        BT_DBG("obj current:");
    } else {
        BT_DBG("obj new:");
    }
    int value = -1;
    lv_obj_t *obj = lv_group_get_focused(group);

    if (obj) {
        value = lv_group_get_obj_id(group, obj);
    }

    if (group == bt_menu_group) {
        printf(" BT Menu Groupid=%d\n", value);
    } else if (group == bt_scan_list_group) {
        printf(" BT Scan List Groupid=%d\n", value);
    } else if (group == bt_paired_list_group) {
        printf(" BT Paired List Groupid=%d\n", value);
    } else {
        printf(" BT NULL Group ERRid=%d\n", value);
    }
#endif
}

int lv_pro_bt_group_user_focus_check_cb(lv_group_t *group)
{
    return 1;  // 返回1，交给2级处理
}

void lv_pro_bt_group_obj_clear_focus(lv_group_t *group)
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

// unpdate obj
void lv_pro_bt_hidden_all_devobj(lv_obj_t **items, int length)
{
    if (items == NULL) {
        BT_ERR("items NULL!\n");
        return;
    }

    if (length <= 0) {
        BT_ERR("Invalid length: %d\n", length);
        return;
    }

    for (int i = 0; i < length; i++) {
        if (items[i] == NULL) {
            // BT_WRN("Item at index %d is NULL, skipping.\n", i);
            continue;
        }
        lv_obj_add_flag(items[i], LV_OBJ_FLAG_HIDDEN);
    }
}

void lv_pro_bt_clear_devobj(void *obj)
{
    lv_obj_t *item = (lv_obj_t *)obj;

    if (item == NULL) {
        BT_ERR("items NULL!\n");
        return;
    }

    lv_pro_set_btn_style6_set_name_str(item, "");
    lv_pro_set_btn_style6_set_mac_addr_str(item, "");
    lv_pro_set_btn_style6_set_state_str(item, "");
    lv_obj_add_flag(item, LV_OBJ_FLAG_HIDDEN);
}

void lv_pro_bt_update_devobj(lv_obj_t *item, BluetoothDeviceMsg *device)
{
    if (device->cod == 5898764) {
        lv_pro_set_btn_style6_set_btn_icon(item, lv_pro_bt_get_resource_path(BT_SCAN_LOGO_PHONE));
    } else if (device->cod == 2360340) {
        lv_pro_set_btn_style6_set_btn_icon(item, lv_pro_bt_get_resource_path(BT_SCAN_LOGO_HEADSET));
    } else if (device->cod == 2752780) {
        lv_pro_set_btn_style6_set_btn_icon(item,
                                           lv_pro_bt_get_resource_path(BT_SCAN_LOGO_COMPUTER));
    } else {
        lv_pro_set_btn_style6_set_btn_icon(item, lv_pro_bt_get_resource_path(BT_SCAN_LOGO_DF));
    }

    lv_pro_set_btn_style6_set_name_str(item, device->name);
    lv_pro_set_btn_style6_set_mac_addr_str(item, device->macAddress);

    const char *state_str = NULL;
    switch (device->status) {
    case BT_STATE_CONNECTED:
        state_str = lv_get_string(STR_BT_CONN);
        break;
    case BT_STATE_SAVED:
        state_str = lv_get_string(STR_BT_SAVED);
        break;
    case BT_STATE_CONNECTING:
        state_str = lv_get_string(STR_BT_CONNECTING);
        break;
    case BT_STATE_DISCONNECTING:
        state_str = lv_get_string(STR_BT_DISCONNECTING);
        break;
    case BT_STATE_DISCONNECTED:
        state_str = lv_get_string(STR_BT_DISCONNECTED);
        break;
    default:
        break;
    }

    lv_pro_set_btn_style6_set_state_str(item, state_str);
    lv_obj_clear_flag(item, LV_OBJ_FLAG_HIDDEN);
}

void lv_pro_bt_set_logostate(void *obj, int state)
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

#if ENABLE_WIFI == 0
char *get_wireless_ko_path(void)
{
    char *file_path = (char *)malloc(256);

    snprintf(file_path, 256, "%s/load_wifi_modules.sh", "etc/wifi");

    return file_path;
}
#endif

void update_file_bt_state(const char *state)
{
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/bt_state_on", config_path);

    FILE *file = fopen(file_path, "w");
    if (file == NULL) {
        perror("Failed to open file");
        return;
    }
    fprintf(file, "%s\n", state);
    fclose(file);
}

int get_file_bt_state()
{
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/bt_state_on", config_path);

    FILE *file = fopen(file_path, "r+");

    if (file == NULL) {
        // File doesn't exist, create it and set bt to off
        file = fopen(file_path, "w");
        if (file == NULL) {
            perror("Failed to create file");
            return -1;
        }
        fprintf(file, "%s\n", BT_STATE_OFF);
        fclose(file);
        return 0;
    }

    int state_result;
    char state[4];

    if (fgets(state, sizeof(state), file) != NULL) {
        state[strcspn(state, "\n")] = 0;
        if (strcmp(state, BT_STATE_ON) == 0) {
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

void update_file_bt_connectstate(const char *state, const char *mac, const char *name)
{
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/bt_connstate", config_path);

    FILE *file = fopen(file_path, "w");
    if (file == NULL) {
        perror("Failed to open file");
        return;
    }

    fputs(state, file);
    fputs("\n", file);

    if (mac)
        fputs(mac, file);
    else
        fputs("mac:null", file);
    fputs("\n", file);

    if (name)
        fputs(name, file);
    else
        fputs("name:null", file);
    fputs("\n", file);

    fclose(file);
}

int get_file_bt_connectstate(char *mac, size_t mac_len, char *name, size_t name_len)
{
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/bt_connstate", config_path);

    FILE *file = fopen(file_path, "r+");

    if (file == NULL) {
        file = fopen(file_path, "w");
        if (file == NULL) {
            perror("Failed to create file");
            return -1;
        }
        fputs(BT_STATE_DISCONN, file);
        fputs("\n", file);
        fputs("mac:null", file);
        fputs("\n", file);
        fputs("name:null", file);
        fputs("\n", file);

        fclose(file);
        return 0;
    }

    int state_result = 0;
    char read_str[128];

    if (fgets(read_str, sizeof(read_str), file) != NULL) {
        read_str[strcspn(read_str, "\n")] = '\0';
        BT_DBG("state=%s\n", read_str);
        if (strcmp(read_str, BT_STATE_CONN) == 0) {
            state_result = 1;
        } else {
            state_result = 0;
        }
    } else {
        state_result = -1;
    }

    if (fgets(read_str, sizeof(read_str), file) != NULL) {
        read_str[strcspn(read_str, "\n")] = '\0';
        snprintf(mac, mac_len, "%s", read_str);
        strncat(mac, "\0", 1);
        BT_DBG("mac=%s\n", mac);
    }

    if (fgets(read_str, sizeof(read_str), file) != NULL) {
        read_str[strcspn(read_str, "\n")] = '\0';
        snprintf(name, name_len, "%s", read_str);
        strncat(name, "\0", 1);
        BT_DBG("name=%s\n", name);
    }

    fclose(file);
    return state_result;
}

lv_obj_t *message_box_ui(lv_obj_t *obj, char *str, uint32_t period)
{
    lv_pro_msgbox_del_handle(NULL);
    return lv_pro_create_message_box(str, period);
}

void setSelectDeviceInfo(const char *mac, const char *name, uint32_t cod, lv_pro_bt_state_t state,
                         bool selected)
{
    strncpy(selectBT.selectMac, mac, sizeof(selectBT.selectMac) - 1);
    selectBT.selectMac[sizeof(selectBT.selectMac) - 1] = '\0';

    strncpy(selectBT.selectName, name, sizeof(selectBT.selectName) - 1);
    selectBT.selectName[sizeof(selectBT.selectName) - 1] = '\0';

    selectBT.cod = cod;
    selectBT.bt_state = state;
    selectBT.isselected = selected;
}

void clearSelectDeviceInfo(void) { memset(&selectBT, 0, sizeof(selectBT)); }

SelectDeviceInfo *getSelectDeviceInfo(void) { return &selectBT; }

#endif /* ENABLE_BT */