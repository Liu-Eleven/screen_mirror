/*
 * lv_pro_launcher.c
 *
 */

#include "lv_pro_launcher.h"
#include "widget/lv_pro_res.h"
#include "widget/lv_pro_set_btn_style1.h"
#include "Media/lv_pro_media.h"
#include "Source/lv_pro_source.h"
#include "WiredSP/lv_pro_wiredsp.h"
#include "WirelessSP/lv_pro_wirelesssp.h"
#include "Setting/lv_pro_setting.h"
#include "Network/lv_pro_wifi_activity.h"
#include "Bluetooth/lv_pro_bt_activity.h"
#include "AWCast/awcast.h"
#include "usbcast.h"
#include "Layer/focus/lv_ui_focuswindow.h"
#include "Layer/volume/volume.h"
#include "Layer/wifi_bt/lv_ui_wifi.h"
#include "Layer/usb/lv_ui_usb.h"
#include "Common/setting/system_setting.h"
#include "Common/language/string/lv_string_id.h"
#include "lv_common.h"
#include "sys_param.h"
#include "page.h"
#include "key_event.h"
#if USE_BSD_EVDEV
#include <dev/evdev/input.h>
#else
#include <linux/input.h>
#endif

lv_obj_t *cur_channel;

lv_obj_t *launcher_activity;
lv_group_t *launcher_group;
lv_obj_t *launcher_cont;
lv_group_t *key_group;
lv_group_t *prev_group;
lv_obj_flag_t signalIn_flag;

static lv_obj_t *launcher_media;
static lv_obj_t *launcher_source;
#if ENABLE_WIFI
static lv_obj_t *launcher_wifi;
#endif
static lv_obj_t *launcher_wired_sp;
static lv_obj_t *launcher_wireless_sp;
#if ENABLE_BT
static lv_obj_t *launcher_bluetooth;
#endif
static lv_obj_t *launcher_setting;
static uint8_t key_event_enable = 1;

bool wireless_autoon_proc_run = false;
bool wireless_modules_loaded = false;

void update_launcher_label(void)
{
    lv_label_set_text(((lv_pro_set_btn_style1_t *)launcher_media)->name, lv_get_string(STR_MEDIA_TITLE));
    lv_label_set_text(((lv_pro_set_btn_style1_t *)launcher_source)->name, lv_get_string(STR_CHANNEL_TITLE));
#if ENABLE_WIFI
    lv_label_set_text(((lv_pro_set_btn_style1_t *)launcher_wifi)->name, lv_get_string(STR_WIFI_TITLE));
#endif
    lv_label_set_text(((lv_pro_set_btn_style1_t *)launcher_wired_sp)->name, lv_get_string(STR_WIRED_TITLE));
    lv_label_set_text(((lv_pro_set_btn_style1_t *)launcher_wireless_sp)->name, lv_get_string(STR_WIRELESS_TITLE));
#if ENABLE_BT
    lv_label_set_text(((lv_pro_set_btn_style1_t *)launcher_bluetooth)->name, lv_get_string(STR_BT_NAME));
#endif
    lv_label_set_text(((lv_pro_set_btn_style1_t *)launcher_setting)->name, lv_get_string(STR_SETTING_TITLE));
}

void del_all_obj_for_ota(void)
{
    if (launcher_activity)
        lv_obj_del(launcher_activity);
    if (launcher_cont)
        lv_obj_del(launcher_cont);
    if (Media_activity)
        lv_obj_del(Media_activity);
    if (WiredSP_activity)
        lv_obj_del(WiredSP_activity);
    if (WirelessSP_activity)
        lv_obj_del(WirelessSP_activity);
#if ENABLE_WIFI
    if (lv_pro_wifi_activity)
        lv_obj_del(lv_pro_wifi_activity);
#endif
#if ENABLE_BT
    if (lv_pro_bt_activity)
        lv_obj_del(lv_pro_bt_activity);
#endif
}

void set_current_screen_channel(lv_obj_t *obj)
{
    cur_channel = obj;

#if 0
    if (obj == launcher_activity)
        printf("screen_channel: launcher\n");
    else if (obj == Setting_activity)
        printf("screen_channel: setting\n");
    else if (obj == Media_activity)
        printf("screen_channel: media\n");
    else if (obj == SignalIn_activity)
        printf("screen_channel: source\n");
    else if (obj == WiredSP_activity)
        printf("screen_channel: wired sp\n");
    else if (obj == WirelessSP_activity)
        printf("screen_channel: wireless sp\n");
#if ENABLE_WIFI
    else if (obj == lv_pro_wifi_activity)
        printf("screen_channel: wifi\n");
#endif
#if ENABLE_BT
    else if (obj == lv_pro_bt_activity)
        printf("screen_channel: bt\n");
#endif
    else if (obj == NULL)
        printf("screen_channel: null\n");
    else
        printf("screen_channel: others\n");
#endif
}

lv_obj_t * get_current_screen_channel(void)
{
    return cur_channel;
}

void set_current_ui_group(lv_group_t *g)
{
    prev_group = lv_group_get_default();
    if (prev_group) {
        lv_group_defocus_obj(NULL);
        lv_indev_reset(evdev_indev, lv_indev_get_obj_act());
    }
    lv_indev_set_group(evdev_indev, g);
    lv_group_set_default(g);
    lv_event_send(lv_group_get_focused(g), LV_EVENT_FOCUSED, evdev_indev);
}

void load_current_channel(lv_obj_t *activity, lv_group_t *group)
{
    lv_indev_reset(evdev_indev, lv_indev_get_obj_act());

    if (group)
        set_current_ui_group(group);

    if (activity)
        lv_scr_load_anim(activity, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);

    set_current_screen_channel(activity);
}

void recovery_prev_ui_group(void)
{
    if (prev_group)
        set_current_ui_group(prev_group);
}

void enable_key_event(void)
{
    key_event_enable = 1;
}

void disable_key_event(void)
{
    key_event_enable = 0;
}

int is_global_key_go_new_channel(uint32_t key)
{
    uint32_t lv_key_map[] = {
        LV_KEY_HOME,
        LV_KEY_MENU,
        U_KEY_SOURCE,
    };

    for (int i = 0; i < (sizeof(lv_key_map)/sizeof(lv_key_map[0])); i++) {
        if (key == lv_key_map[i]) {
            return 0;
        }
    }
    return -1;
}

/* ir key code map to lv_keys*/
static uint32_t keypad_key_map2_lvkey(uint32_t act_key)
{
    switch(act_key) {
        case KEY_HOME:
            return LV_KEY_HOME;
        case KEY_MENU:
            return LV_KEY_MENU;

        case KEY_EPG:
            return U_KEY_SOURCE;

        case KEY_CAMERA_UP:
            return U_KEY_FOCUS_UP;
        case KEY_CAMERA_DOWN:
            return U_KEY_FOCUS_DOWN;

        case KEY_MUTE:
            return U_KEY_MUTE;
        case KEY_VOLUMEUP:
            return U_KEY_VOLUME_UP;
        case KEY_VOLUMEDOWN:
            return U_KEY_VOLUME_DOWN;

        case KEY_POWER:
            return U_KEY_POWER;
        default:
            return act_key;
    }
    return act_key;
}

uint32_t do_global_event(uint32_t key)
{
    reset_autosleep_timer();
    reset_osd_timer();
    uint32_t lv_key;
    page_id_t last_page_id = get_last_page_id();

    if (!key_event_enable)
        return 0;

    /* need send message to others moduls */
    lv_key = keypad_key_map2_lvkey(key);
    if (lv_key != key || get_factor_test_flag()) {
        if (lv_key == LV_KEY_MENU && get_current_page_id() == PAGE_SETTING) {
            return key;
        } else if (lv_key == U_KEY_SOURCE && (last_page_id >= PAGE_FILE) && (last_page_id <= PAGE_EBOOK) &&
                get_current_page_id() == PAGE_SETTING) {
            return key;
        } else if (lv_key == U_KEY_SOURCE && get_current_page_id() == PAGE_SOURCE) {
            lv_event_send(lv_group_get_focused(lv_group_get_default()), LV_EVENT_KEY, &lv_key);
            return key;
        } else {
            lv_event_send(lv_group_get_focused(lv_group_get_default()), LV_EVENT_KEY, &lv_key);
        }
        /* go factroy test that don't deal key */
        if (get_factor_test_flag())
            return key;
    }

    if (lv_key == LV_KEY_HOME) {
        if (get_current_page_id() != PAGE_HOME)
            switch_page(PAGE_HOME);
        else
            set_current_ui_group(launcher_group);
    } else if (lv_key == LV_KEY_MENU) {
        page_id_t cur_page_id = get_current_page_id();
        if (cur_page_id == PAGE_SOURCE || ((cur_page_id >= PAGE_FILE) && (cur_page_id <= PAGE_EBOOK))) {
            switch_page_create_hide(PAGE_SETTING);
        } else {
            switch_page(PAGE_SETTING);
        }
        lv_group_focus_obj(lv_group_get_head_obj_ex(Setting_group));
    } else if (lv_key == U_KEY_MUTE) {
        if (is_volume_mute()) {
            create_ui_volume(KEY_RESERVED);
            lv_set_mute(false);
        } else {
            lv_set_mute(true);
        }
        create_ui_mute_icon();
    } else if (lv_key == U_KEY_VOLUME_UP) {
        if (lv_get_volume() == 0 || is_volume_mute()) {
            create_ui_volume(U_KEY_VOLUME_UP);
            lv_set_mute(false);
            create_ui_mute_icon();
        } else {
            create_ui_volume(U_KEY_VOLUME_UP);
        }
    } else if (lv_key == U_KEY_VOLUME_DOWN) {
        create_ui_volume(U_KEY_VOLUME_DOWN);
        long cur_vol = lv_get_volume();
        if (cur_vol <= 0) {
            lv_set_mute(true);
            create_ui_mute_icon();
        } else if (is_volume_mute()) {
            lv_set_mute(false);
            create_ui_mute_icon();
        }
    } else if (lv_key == U_KEY_FOCUS_UP) {
        lv_motor_forward(3);
        lens_focus_key_send_handler(U_KEY_FOCUS_UP);
    } else if (lv_key == U_KEY_FOCUS_DOWN) {
        lv_motor_reverse(3);
        lens_focus_key_send_handler(U_KEY_FOCUS_DOWN);
    } else if (lv_key == U_KEY_SOURCE) {
        if (get_last_page_id() == PAGE_SOURCE && get_current_page_id() == PAGE_SETTING)
            switch_page_show_destory(PAGE_SOURCE);
        else
            switch_page(PAGE_SOURCE);
    } else if (lv_key == U_KEY_POWER) {
        create_ui_powerkey_msgbox();
    }
    return key;
}

void set_obj_default_outline_style(lv_obj_t *obj)
{
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_outline_width(obj, 4, 0);
    lv_obj_set_style_outline_color(obj, lv_color_hex(0xFFFFFFF), 0);
    lv_obj_set_style_outline_pad(obj, 0, 0);
    lv_obj_set_style_outline_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_outline_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
}

static void wireless_sp_message(lv_timer_t * timer)
{
    lv_obj_t *obj = (lv_obj_t *) timer->user_data;
    lv_obj_del(obj);
    lv_timer_del(timer);
}

static void create_wireless_popup(void)
{
    lv_obj_t *m_obj_msg = lv_obj_create(lv_scr_act());
    lv_obj_set_size(m_obj_msg,LV_PCT(30),LV_PCT(25));
    lv_obj_set_align(m_obj_msg, LV_ALIGN_CENTER);
    lv_obj_clear_flag(m_obj_msg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(m_obj_msg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(m_obj_msg, lv_color_hex(0x4D72E0), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(m_obj_msg, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *ui_img = lv_img_create(m_obj_msg);
    lv_img_set_src(ui_img, "A:/usr/share/lv_projector/disable.png");
    lv_obj_set_align(ui_img, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_img, LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_clear_flag(ui_img, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * ui_lab = lv_label_create(m_obj_msg);
    lv_obj_set_width(ui_lab, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_lab, LV_SIZE_CONTENT);
    lv_obj_set_align(ui_lab, LV_ALIGN_BOTTOM_MID);
    lv_label_set_long_mode(ui_lab,LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_color(ui_lab, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_lab, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_lab, &GENERAL_FONT_BIG, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ui_lab, lv_get_string(STR_WIFI_NOT_CONNECT));

    lv_timer_t * timer = lv_timer_create(wireless_sp_message, 3000, m_obj_msg);
    timer->user_data = m_obj_msg;
}

static void event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    char *user_data = (char *)lv_event_get_user_data(e);
    struct _lv_obj_t *cur_obj = lv_event_get_target(e);
    uint32_t *key = lv_event_get_param(e);
    struct _lv_obj_t *target_obj;

    if (code == LV_EVENT_KEY) {
        if (*key == LV_KEY_ENTER) {
            if (!strcmp(user_data, "Media")) {
                switch_page(PAGE_MEDIA);
            } else if (!strcmp(user_data, "Signal Input")) {
                switch_page(PAGE_SOURCE);
            } else if (!strcmp(user_data, "Network")) {
#if ENABLE_WIFI
                if (lv_pro_wifi_init() == 0) {
                    switch_page(PAGE_WIFI);
                }
#endif
            } else if (!strcmp(user_data, "Wired SP")) {
                switch_page(PAGE_WIRED_SP);
            } else if (!strcmp(user_data, "Wireless SP")) {
                if (!lv_pro_network_state()) {  // if (getSsid() != 0) {
                    create_wireless_popup();
                    return;
                }
                switch_page(PAGE_WIRELESS_SP);
            } else if (!strcmp(user_data, "Bluetooth")) {
#if ENABLE_BT
                if (lv_pro_bt_init() == 0) {
                    switch_page(PAGE_BT);
                }
#endif
            } else if (!strcmp(user_data, "Setting")) {
                switch_page(PAGE_SETTING);
            }
        }
    }
}

static void lv_create_ui_prompt(void)
{
    create_ui_activate_icon(lv_layer_top());
    create_ui_usb_icon(lv_layer_top());
    create_ui_wifi_bt_icon(lv_layer_top());
}

void lv_pro_launcher_init(void) {
    launcher_activity = lv_obj_create(NULL);
    lv_scr_load(launcher_activity);
    launcher_group = lv_group_create();
    key_group = lv_group_create();  // just global temp group

    lv_obj_set_size(launcher_activity, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(launcher_activity, lv_color_hex(0x000000), 0);

    char *png_path[] = { "A:/usr/share/lv_projector/media.png",
            "A:/usr/share/lv_projector/signal_input.png",
            "A:/usr/share/lv_projector/network.png",
            "A:/usr/share/lv_projector/wired_sp.png",
            "A:/usr/share/lv_projector/wireless_sp.png",
            "A:/usr/share/lv_projector/bluetooth.png",
            "A:/usr/share/lv_projector/setting.png" };
    char *png_name[] = { "Media", "Signal Input", "Network", "Wired SP", "Wireless SP", "Bluetooth", "Setting" };

    int launcher_color[][3] = {
        {233,   38,    22},
        {128,   128,   255},
        {168,   227,   155},
        {128,   0,     255},
        {255,   128,   0},
        {255,   128,   192},
        {255,   201,   14},
    };

    /*Create a container with grid*/
    launcher_cont = lv_obj_create(launcher_activity);
    lv_obj_t *cont = launcher_cont;
    lv_obj_set_size(cont, lv_pct(80), lv_pct(80));
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_center(cont);

    static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(cont, col_dsc, row_dsc);
    lv_obj_set_style_pad_column(cont, 30, 0);
    lv_obj_set_style_pad_row(cont, 30, 0);
    lv_obj_set_layout(cont, LV_LAYOUT_GRID);

    /* 1: media */
    launcher_media = lv_pro_set_btn_style1_create(cont);
    lv_obj_set_size(launcher_media, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_grid_cell(launcher_media, LV_GRID_ALIGN_STRETCH, 0, 2,
                         LV_GRID_ALIGN_STRETCH, 0, 1);
    lv_pro_set_btn_style1_src(launcher_media, &GENERAL_FONT_BIG,
                        lv_color_make(launcher_color[0][0], launcher_color[0][1], launcher_color[0][2]),
                        LV_OPA_COVER, png_path[0], lv_get_string(STR_MEDIA_TITLE));
    lv_group_add_obj(launcher_group, lv_pro_set_btn_style1_get_btn(launcher_media));
    lv_obj_add_event_cb(launcher_media, event_handler, LV_EVENT_KEY, png_name[0]);

#if ENABLE_HDMIRX
    /* 2: signal input */
    launcher_source = lv_pro_set_btn_style1_create(cont);
    lv_obj_set_size(launcher_source, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_grid_cell(launcher_source, LV_GRID_ALIGN_STRETCH, 2, 1,
                         LV_GRID_ALIGN_STRETCH, 0, 1);
    lv_pro_set_btn_style1_src(launcher_source, &GENERAL_FONT_BIG,
                        lv_color_make(launcher_color[1][0], launcher_color[1][1], launcher_color[1][2]),
                        LV_OPA_COVER, png_path[1], lv_get_string(STR_CHANNEL_TITLE));
    lv_group_add_obj(launcher_group, lv_pro_set_btn_style1_get_btn(launcher_source));
    lv_obj_add_event_cb(launcher_source, event_handler, LV_EVENT_KEY, png_name[1]);
#endif

    /* 3: network */
#if ENABLE_WIFI
    launcher_wifi = lv_pro_set_btn_style1_create(cont);
    lv_obj_set_size(launcher_wifi, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_grid_cell(launcher_wifi, LV_GRID_ALIGN_STRETCH, 3, 1,
                         LV_GRID_ALIGN_STRETCH, 0, 1);
    lv_pro_set_btn_style1_src(launcher_wifi, &GENERAL_FONT_BIG,
                        lv_color_make(launcher_color[2][0], launcher_color[2][1], launcher_color[2][2]),
                        LV_OPA_COVER, png_path[2], lv_get_string(STR_WIFI_TITLE));
    lv_group_add_obj(launcher_group, lv_pro_set_btn_style1_get_btn(launcher_wifi));
    lv_obj_add_event_cb(launcher_wifi, event_handler, LV_EVENT_KEY, png_name[2]);
#endif
    /* 4: wired screen projection */
    launcher_wired_sp = lv_pro_set_btn_style1_create(cont);
    lv_obj_set_size(launcher_wired_sp, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_grid_cell(launcher_wired_sp, LV_GRID_ALIGN_STRETCH, 0, 1,
                         LV_GRID_ALIGN_STRETCH, 1, 1);
    lv_pro_set_btn_style1_src(launcher_wired_sp, &GENERAL_FONT_BIG,
                        lv_color_make(launcher_color[3][0], launcher_color[3][1], launcher_color[3][2]),
                        LV_OPA_COVER, png_path[3], lv_get_string(STR_WIRED_TITLE));
    lv_group_add_obj(launcher_group, lv_pro_set_btn_style1_get_btn(launcher_wired_sp));
    lv_obj_add_event_cb(launcher_wired_sp, event_handler, LV_EVENT_KEY, png_name[3]);

    /* 5: wireless screen projection */
    launcher_wireless_sp = lv_pro_set_btn_style1_create(cont);
    lv_obj_set_size(launcher_wireless_sp, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_grid_cell(launcher_wireless_sp, LV_GRID_ALIGN_STRETCH, 1, 1,
                         LV_GRID_ALIGN_STRETCH, 1, 1);
    lv_pro_set_btn_style1_src(launcher_wireless_sp, &GENERAL_FONT_BIG,
                        lv_color_make(launcher_color[4][0], launcher_color[4][1], launcher_color[4][2]),
                        LV_OPA_COVER, png_path[4], lv_get_string(STR_WIRELESS_TITLE));
    lv_group_add_obj(launcher_group, lv_pro_set_btn_style1_get_btn(launcher_wireless_sp));
    lv_obj_add_event_cb(launcher_wireless_sp, event_handler, LV_EVENT_KEY, png_name[4]);

    /* 6: bluetooth */
#if ENABLE_BT
    launcher_bluetooth = lv_pro_set_btn_style1_create(cont);
    lv_obj_set_size(launcher_bluetooth, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_grid_cell(launcher_bluetooth, LV_GRID_ALIGN_STRETCH, 2, 1,
                         LV_GRID_ALIGN_STRETCH, 1, 1);
    lv_pro_set_btn_style1_src(launcher_bluetooth, &GENERAL_FONT_BIG,
                        lv_color_make(launcher_color[5][0], launcher_color[5][1], launcher_color[5][2]),
                        LV_OPA_COVER, png_path[5], lv_get_string(STR_BT_NAME));
    lv_group_add_obj(launcher_group, lv_pro_set_btn_style1_get_btn(launcher_bluetooth));
    lv_obj_add_event_cb(launcher_bluetooth, event_handler, LV_EVENT_KEY, png_name[5]);
#endif

    /* 7: settings */
    launcher_setting = lv_pro_set_btn_style1_create(cont);
    lv_obj_set_size(launcher_setting, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_grid_cell(launcher_setting, LV_GRID_ALIGN_STRETCH, 3, 1,
                         LV_GRID_ALIGN_STRETCH, 1, 1);
    lv_pro_set_btn_style1_src(launcher_setting, &GENERAL_FONT_BIG,
                        lv_color_make(launcher_color[6][0], launcher_color[6][1], launcher_color[6][2]),
                        LV_OPA_COVER, png_path[6], lv_get_string(STR_SETTING_TITLE));
    lv_group_add_obj(launcher_group, lv_pro_set_btn_style1_get_btn(launcher_setting));
    lv_obj_add_event_cb(launcher_setting, event_handler, LV_EVENT_KEY, png_name[6]);

#if ENABLE_HDMIRX == 0 &&  ENABLE_BT == 0 && ENABLE_WIFI == 1
    static lv_coord_t col_dsc_new[] = {
            LV_GRID_FR(1),
            LV_GRID_CONTENT,
            LV_GRID_CONTENT,
            LV_GRID_CONTENT,
            LV_GRID_FR(1),
          LV_GRID_TEMPLATE_LAST
        };
        static lv_coord_t row_dsc_new[] = {
                LV_GRID_CONTENT,
                LV_GRID_CONTENT,
                LV_GRID_TEMPLATE_LAST
        };
        lv_obj_set_grid_dsc_array(cont, col_dsc_new, row_dsc_new);

        lv_obj_set_style_pad_column(cont, 40, 0);
        lv_obj_set_style_pad_row(cont, 40, 0);

        if (launcher_media) {
            lv_obj_set_grid_cell(launcher_media,
                                 LV_GRID_ALIGN_CENTER, 1, 2,
                                 LV_GRID_ALIGN_CENTER, 0, 1);
        }
		#if ENABLE_WIFI
        if (launcher_wifi) {
            lv_obj_set_grid_cell(launcher_wifi,
                                 LV_GRID_ALIGN_CENTER, 3, 1,
                                 LV_GRID_ALIGN_CENTER, 0, 1);
        }
		#endif
        if (launcher_wired_sp) {
        lv_obj_set_grid_cell(launcher_wired_sp,
                             LV_GRID_ALIGN_CENTER, 1, 1,
                             LV_GRID_ALIGN_CENTER, 1, 1);
        }
        if (launcher_wireless_sp) {
        lv_obj_set_grid_cell(launcher_wireless_sp,
                             LV_GRID_ALIGN_CENTER, 2, 1,
                             LV_GRID_ALIGN_CENTER, 1, 1);
        }
        if (launcher_setting) {
        lv_obj_set_grid_cell(launcher_setting,
                             LV_GRID_ALIGN_CENTER, 3, 1,
                             LV_GRID_ALIGN_CENTER, 1, 1);
        }
#endif
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_update_layout(launcher_activity);
}

#if ENABLE_BT == 1 || ENABLE_WIFI == 1
static int load_wifi_module(const char *script_path)
{
    int ret = system(script_path);

    if (ret == -1) {
        printf("failed to execute script: %s\n", script_path);
        return -1;
    }

    int exit_status = WEXITSTATUS(ret);

    if (exit_status == 0) {
        return 0;
    } else {
        printf("script returned with error code: %d\n", exit_status);
        return -1;
    }
}

static void *wireless_autoon_proc(void *arg)
{
    int ret;
    wireless_autoon_proc_run = true;

#if ENABLE_WIFI
    lv_pro_wifi_lanucher_auto_on();
#endif

#if ENABLE_BT
    clear_bluealsa_card();
    lv_pro_bt_lanucher_auto_on();
#endif

    char *script_path = get_wireless_ko_path();

    if (load_wifi_module(script_path) != 0) {
        printf("failed to load modules using script!\n");
        wireless_modules_loaded = false;
    } else {
        wireless_modules_loaded = true;
    }

    free(script_path);
    wireless_autoon_proc_run = false;

    return NULL;
}

static void lv_pro_network_auto_on(void)
{
    pthread_t wireless_thread;
    if (pthread_create(&wireless_thread, NULL, wireless_autoon_proc, NULL) != 0) {
        printf("Failed to create wifi thread\n");
        return -1;
    }
    pthread_detach(wireless_thread);
}
#endif

void lv_projector_ui_init(void)
{
#if ENABLE_BT == 1 || ENABLE_WIFI == 1
    lv_pro_network_auto_on();
#endif
    lv_pro_launcher_init();

    lv_create_ui_prompt();

    lv_pro_signalin_init();

    lv_pro_setting_init();

    set_current_ui_group(launcher_group);
    set_current_screen_channel(launcher_activity);
    set_current_page_id(PAGE_HOME);
}

static int create_launcher(void)
{
    load_current_channel(launcher_activity, launcher_group);
    return 0;
}

static int destory_launcher(void)
{
    return 0;
}

static int show_launcher(void)
{
    return 0;
}

static int hide_launcher(void)
{
    return 0;
}

static page_interface_t page_launcher =
{
    .ops =
    {
        create_launcher,
        destory_launcher,
        show_launcher,
        hide_launcher,
    },
    .info =
    {
        .id = PAGE_HOME,
        .user_data = NULL
    }
};

void REGISTER_PAGE_HOME(void)
{
    reg_page(&page_launcher);
}

void update_launcher_fresh_layout(void)
{
    printf("update_launcher_fresh_layout\n");

    if (!launcher_activity || !launcher_cont) {
        printf("launcher not initialized\n");
        return;
    }

    // Get current screen resolution from display driver
    lv_disp_t *disp = lv_obj_get_disp(launcher_activity);
    lv_coord_t screen_w = lv_disp_get_hor_res(disp);
    lv_coord_t screen_h = lv_disp_get_ver_res(disp);

    // Recalculate launcher_cont size and position (80% of screen, centered)
    lv_coord_t cont_w = screen_w * 80 / 100;
    lv_coord_t cont_h = screen_h * 80 / 100;

    // Set launcher_cont size and position with fixed pixel values
    lv_obj_set_size(launcher_cont, cont_w, cont_h);
    lv_obj_align(launcher_cont, LV_ALIGN_CENTER, 0, 0);

    // Get content area of launcher_cont
    lv_coord_t pad_left = lv_obj_get_style_pad_left(launcher_cont, LV_PART_MAIN);
    lv_coord_t pad_right = lv_obj_get_style_pad_right(launcher_cont, LV_PART_MAIN);
    lv_coord_t pad_top = lv_obj_get_style_pad_top(launcher_cont, LV_PART_MAIN);
    lv_coord_t pad_bottom = lv_obj_get_style_pad_bottom(launcher_cont, LV_PART_MAIN);
    lv_coord_t border_w = lv_obj_get_style_border_width(launcher_cont, LV_PART_MAIN);
    lv_coord_t content_w = cont_w - pad_left - pad_right - 2 * border_w;
    lv_coord_t content_h = cont_h - pad_top - pad_bottom - 2 * border_w;
    lv_coord_t pad_col = lv_obj_get_style_pad_column(launcher_cont, LV_PART_MAIN);
    lv_coord_t pad_row = lv_obj_get_style_pad_row(launcher_cont, LV_PART_MAIN);

    // Content area origin relative to launcher_cont (for lv_obj_set_pos)
    lv_coord_t cx = pad_left + border_w;
    lv_coord_t cy = pad_top + border_w;

    //printf("launcher_fresh: cont=%dx%d content=%dx%d pad_col=%d pad_row=%d\n",cont_w, cont_h, content_w, content_h, pad_col, pad_row);

    // Grid layout for ENABLE_HDMIRX==0 && ENABLE_BT==0 && ENABLE_WIFI==1:
    //   cols: FR(1), CONTENT, CONTENT, CONTENT, FR(1)  (5 columns)
    //   rows: CONTENT, CONTENT  (2 rows)
    //   pad_column=40, pad_row=40
    //   media:       col 1-2, row 0, CENTER
    //   wifi:        col 3,   row 0, CENTER
    //   wired_sp:    col 1,   row 1, CENTER
    //   wireless_sp: col 2,   row 1, CENTER
    //   setting:     col 3,   row 1, CENTER
    //
    // Since lv_obj_get_self_width returns 0 for these buttons (w_layout/h_layout
    // were set by the initial STRETCH grid), we calculate button sizes based on
    // the original 4-column FR(1) grid layout, scaled to the new content area.

    // Original layout: 4 columns FR(1), 2 rows FR(1)
    // Each column width = (content_w - 3 * pad_col) / 4
    // Each row height = (content_h - 1 * pad_row) / 2
    // media spans 2 columns: width = 2 * col_w + pad_col
    lv_coord_t col_w = (content_w - 3 * pad_col) / 4;
    lv_coord_t row_h = (content_h - 1 * pad_row) / 2;

    // Button sizes based on grid cells
    lv_coord_t media_w = 2 * col_w + pad_col;  // spans 2 columns
    lv_coord_t media_h = row_h;
    lv_coord_t wifi_w = col_w;
    lv_coord_t wifi_h = row_h;
    lv_coord_t wired_w = col_w;
    lv_coord_t wired_h = row_h;
    lv_coord_t wireless_w = col_w;
    lv_coord_t wireless_h = row_h;
    lv_coord_t setting_w = col_w;
    lv_coord_t setting_h = row_h;

    // For the 5-column grid (FR(1), CONTENT, CONTENT, CONTENT, FR(1)):
    // CONTENT columns have width = col_w, FR(1) columns share remaining space
    lv_coord_t col1_w = col_w;
    lv_coord_t col2_w = col_w;
    lv_coord_t col3_w = col_w;
    lv_coord_t content_cols_total = col1_w + col2_w + col3_w + 4 * pad_col;
    lv_coord_t fr_total = content_w - content_cols_total;
    if (fr_total < 0) fr_total = 0;
    lv_coord_t col0_w = fr_total / 2;
    lv_coord_t col4_w = fr_total - col0_w;

    // Row heights
    lv_coord_t row0_h = row_h;
    lv_coord_t row1_h = row_h;

    // Column x positions (relative to content area origin)
    lv_coord_t col_x[5];
    col_x[0] = 0;
    col_x[1] = col_x[0] + col0_w + pad_col;
    col_x[2] = col_x[1] + col1_w + pad_col;
    col_x[3] = col_x[2] + col2_w + pad_col;
    col_x[4] = col_x[3] + col3_w + pad_col;

    // Row y positions (relative to content area origin)
    lv_coord_t row_y[2];
    row_y[0] = 0;
    row_y[1] = row0_h + pad_row;
    /*
    printf("launcher_fresh: cols=%d,%d,%d,%d,%d rows=%d,%d btn_sizes: media=%dx%d wifi=%dx%d wired=%dx%d wireless=%dx%d setting=%dx%d\n",
           col0_w, col1_w, col2_w, col3_w, col4_w, row0_h, row1_h,
           media_w, media_h, wifi_w, wifi_h, wired_w, wired_h, wireless_w, wireless_h, setting_w, setting_h);
    */
    // Position and size each button
    // lv_obj_set_pos coordinates are relative to parent (launcher_cont)

    // media: col 1-2, row 0, CENTER
    {
        lv_coord_t cell_w = col1_w + pad_col + col2_w;
        lv_coord_t x = cx + col_x[1] + (cell_w - media_w) / 2;
        lv_coord_t y = cy + row_y[0] + (row0_h - media_h) / 2;
        lv_obj_set_pos(launcher_media, x, y);
        lv_obj_set_size(launcher_media, media_w, media_h);
    }
	#if ENABLE_WIFI
    // wifi: col 3, row 0, CENTER
    {
        lv_coord_t x = cx + col_x[3] + (col3_w - wifi_w) / 2;
        lv_coord_t y = cy + row_y[0] + (row0_h - wifi_h) / 2;
        lv_obj_set_pos(launcher_wifi, x, y);
        lv_obj_set_size(launcher_wifi, wifi_w, wifi_h);
    }
	#endif
    // wired_sp: col 1, row 1, CENTER
    {
        lv_coord_t x = cx + col_x[1] + (col1_w - wired_w) / 2;
        lv_coord_t y = cy + row_y[1] + (row1_h - wired_h) / 2;
        lv_obj_set_pos(launcher_wired_sp, x, y);
        lv_obj_set_size(launcher_wired_sp, wired_w, wired_h);
    }
    // wireless_sp: col 2, row 1, CENTER
    {
        lv_coord_t x = cx + col_x[2] + (col2_w - wireless_w) / 2;
        lv_coord_t y = cy + row_y[1] + (row1_h - wireless_h) / 2;
        lv_obj_set_pos(launcher_wireless_sp, x, y);
        lv_obj_set_size(launcher_wireless_sp, wireless_w, wireless_h);
    }
    // setting: col 3, row 1, CENTER
    {
        lv_coord_t x = cx + col_x[3] + (col3_w - setting_w) / 2;
        lv_coord_t y = cy + row_y[1] + (row1_h - setting_h) / 2;
        lv_obj_set_pos(launcher_setting, x, y);
        lv_obj_set_size(launcher_setting, setting_w, setting_h);
    }

    // Update inner button sizes and re-align image and label
    lv_pro_set_btn_style1_t *btn_media = (lv_pro_set_btn_style1_t *)launcher_media;
    lv_obj_set_size(btn_media->btn, media_w, media_h);
    lv_obj_align(btn_media->img, LV_ALIGN_CENTER, 0, -20);
    lv_obj_align_to(btn_media->name, btn_media->img, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

	#if ENABLE_WIFI
    lv_pro_set_btn_style1_t *btn_wifi = (lv_pro_set_btn_style1_t *)launcher_wifi;
    lv_obj_set_size(btn_wifi->btn, wifi_w, wifi_h);
    lv_obj_align(btn_wifi->img, LV_ALIGN_CENTER, 0, -20);
    lv_obj_align_to(btn_wifi->name, btn_wifi->img, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
	#endif
    lv_pro_set_btn_style1_t *btn_wired = (lv_pro_set_btn_style1_t *)launcher_wired_sp;
    lv_obj_set_size(btn_wired->btn, wired_w, wired_h);
    lv_obj_align(btn_wired->img, LV_ALIGN_CENTER, 0, -20);
    lv_obj_align_to(btn_wired->name, btn_wired->img, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    lv_pro_set_btn_style1_t *btn_wireless = (lv_pro_set_btn_style1_t *)launcher_wireless_sp;
    lv_obj_set_size(btn_wireless->btn, wireless_w, wireless_h);
    lv_obj_align(btn_wireless->img, LV_ALIGN_CENTER, 0, -20);
    lv_obj_align_to(btn_wireless->name, btn_wireless->img, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    lv_pro_set_btn_style1_t *btn_setting = (lv_pro_set_btn_style1_t *)launcher_setting;
    lv_obj_set_size(btn_setting->btn, setting_w, setting_h);
    lv_obj_align(btn_setting->img, LV_ALIGN_CENTER, 0, -20);
    lv_obj_align_to(btn_setting->name, btn_setting->img, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    // Invalidate all objects
    lv_obj_invalidate(launcher_media);
	#if ENABLE_WIFI
    lv_obj_invalidate(launcher_wifi);
	#endif
    lv_obj_invalidate(launcher_wired_sp);
    lv_obj_invalidate(launcher_wireless_sp);
    lv_obj_invalidate(launcher_setting);
    lv_obj_invalidate(launcher_cont);
    lv_obj_invalidate(launcher_activity);

    // 立即刷新显示
    lv_refr_now(NULL);
}
