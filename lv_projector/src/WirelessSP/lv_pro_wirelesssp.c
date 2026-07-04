/*
 * lv_pro_wirelesssp.c
 *
 */

#include "lv_pro_wirelesssp.h"

#include <stdlib.h>

#include "../AWCast/awcast.h"
#include "../Network/lv_pro_res_wifi.h"
#include "../Network/lv_pro_wifi_activity.h"
#include "../lv_pro_launcher.h"
#include "../widget/lv_pro_img_text.h"
#include "../widget/lv_pro_res.h"
#include "Common/user_common/lv_pro_user_common.h"
#include "lv_common.h"
#include "page.h"

lv_obj_t *WirelessSP_activity;
lv_group_t *WirelessSP_group;

static int wirelesssp_wifimode = -1;  // STA = 0, AP = 1
static const char *ssid = "allwinner-ap";
static const char *psk = "Aa123456";
static const int channel = 6;

static void wirelesssp_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t *key = lv_event_get_param(e);

    if (code == LV_EVENT_KEY) {
        if (*key == LV_KEY_BACK)
            switch_page(PAGE_HOME);
    }
}

int lv_Pro_wirelesssp_create_ap(void)
{
    lv_pro_refresh_wifi_state(true);  // 切AP后需要重置STA

    if (lv_pro_res_wifi_ap_on(ssid, psk, channel) == 0) {
        char wifi_ap_title[64];
        snprintf(wifi_ap_title, sizeof(wifi_ap_title), "%s\n%s", ssid, psk);

        lv_obj_t *wifi_ap_lable = lv_label_create(WirelessSP_activity);
        lv_obj_set_style_text_font(wifi_ap_lable, &GENERAL_FONT_MID, 0);
        lv_obj_set_style_text_color(wifi_ap_lable, lv_color_white(), 0);
        lv_label_set_recolor(wifi_ap_lable, true);
        lv_label_set_text_fmt(wifi_ap_lable, "#ffffff %s #", wifi_ap_title);
        lv_obj_align(wifi_ap_lable, LV_ALIGN_TOP_RIGHT, -50, 10);
        return 0;
    }
    return -1;
}


static void lv_pro_wirelesssp_ssid_info(void)
{
    char device_name_info[64] = {0};

    sprintf(device_name_info,"%s: %s",lv_get_string(STR_CAST_SSID_DEVICE),__Get_AWCast_device_name());
    lv_label_set_text_fmt(lv_obj_get_child(lv_obj_get_child(WirelessSP_activity, 2), 0), "#ffffff %s #", device_name_info);
}

static void wirelesssp_activity_event_handler(lv_event_t *e)
{
     lv_event_code_t code = lv_event_get_code(e);

     if (code == LV_EVENT_SCREEN_LOADED) {
     } else if (code == LV_EVENT_SCREEN_UNLOADED) {
        lv_pro_wirelesssp_deinit();
     }
}

int lv_pro_wirelesssp_init(void)
{
    if (WirelessSP_activity) {
        printf("%s: WirelessSP_activity already init\n", __func__);
        return -1;
    }

    // // 先初始wifi资源
    // if (lv_pro_res_wifi_sta_is_connect()) {
    //     wirelesssp_wifimode = 0;
    // } else {
    //     // int ret = lv_Pro_wirelesssp_create_ap();
    //     // wirelesssp_wifimode = (ret == 0) ? 1 : -1;
    //     wirelesssp_wifimode = 0;
    // }
    wirelesssp_wifimode = 0;
    WirelessSP_activity = lv_obj_create(NULL);
    WirelessSP_group = lv_group_create();
    set_current_ui_group(WirelessSP_group);

    lv_obj_set_size(WirelessSP_activity, lv_pct(100), lv_pct(100));
    lv_obj_add_style(WirelessSP_activity, &lv_pro_res.set_bg_blue, 0);
    lv_obj_clear_flag(WirelessSP_activity, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(WirelessSP_activity, wirelesssp_activity_event_handler, LV_EVENT_SCREEN_LOADED, NULL);
    lv_obj_add_event_cb(WirelessSP_activity, wirelesssp_activity_event_handler, LV_EVENT_SCREEN_UNLOADED, NULL);

    char *device_img[] = { "A:/usr/share/lv_projector/cast/wireless-miracast.png",
                            "A:/usr/share/lv_projector/cast/wireless-ios.png",
                            "A:/usr/share/lv_projector/cast/wireless-dlna.png",
                            "A:/usr/share/lv_projector/cast/wireless-windows.png"};

    char *cont_title[4];
    cont_title[0] = lv_get_string(STR_ALPHAS_MIR_STR1);
    cont_title[1] = lv_get_string(STR_ALPHAS_HOME_IOSCAST);
    cont_title[2] = lv_get_string(STR_ALPHAS_HOME_DLNA);
    cont_title[3] = lv_get_string(STR_ALPHAS_HOME_WINDOWS);

    char *cont_text[4];
    char miracast_info[256] = {0};
    sprintf(miracast_info,"%s\n%s\n%s",lv_get_string(STR_ALPHAS_MIR_STR2),lv_get_string(STR_ALPHAS_MIR_STR3),
                                                    lv_get_string(STR_ALPHAS_MIR_STR4));
    cont_text[0] = miracast_info;

    char ios_info[256] = {0};
    sprintf(ios_info,"%s\n%s\n%s",lv_get_string(STR_ALPHAS_IOSCAST_STR8),lv_get_string(STR_ALPHAS_IOSCAST_STR9),
                                                lv_get_string(STR_ALPHAS_IOSCAST_STR10));
    cont_text[1] = ios_info;

    char dlna_info[256] = {0};
    sprintf(dlna_info,"%s\n%s\n%s",lv_get_string(STR_ALPHAS_DLNA_STR1),lv_get_string(STR_ALPHAS_DLNA_STR2),
                                                lv_get_string(STR_ALPHAS_DLNA_STR3));
    cont_text[2] = dlna_info;

    cont_text[3] = lv_get_string(STR_ALPHAS_WINDOWS_STR1);
    /*Create a container with grid*/
    lv_obj_t *cont = lv_obj_create(WirelessSP_activity);
    lv_obj_set_size(cont, lv_pct(95), lv_pct(60));
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_align(cont, LV_ALIGN_TOP_LEFT, lv_pct(5), lv_pct(10));
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(cont, col_dsc, row_dsc);
    lv_obj_set_style_pad_column(cont, 20, 0);
    lv_obj_set_style_pad_row(cont, 10, 0);
    lv_obj_set_layout(cont, LV_LAYOUT_GRID);

    for (uint32_t i = 0; i < 4; i++) {
        uint8_t col = i % 2;
        uint8_t row = i / 2;

        lv_obj_t *img_text = lv_pro_img_text_create(cont);
        lv_obj_set_size(img_text, lv_pct(100), lv_pct(100));
        lv_obj_set_grid_cell(img_text, LV_GRID_ALIGN_STRETCH, col, 1,
                            LV_GRID_ALIGN_STRETCH, row, 1);
        lv_pro_set_img_text_src(img_text, device_img[i], cont_title[i], cont_text[i], 350);

        lv_pro_img_text_t *btn = (lv_pro_img_text_t*) img_text;
        lv_obj_set_style_text_font(btn->bottom_label, &GENERAL_FONT_SMALL, 0);
        lv_obj_align_to(btn->bottom_label, btn->top_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
    }

    /*Create a container with qr_code and note*/
    lv_obj_t *qr_code_cont = lv_obj_create(WirelessSP_activity);
    lv_obj_set_size(qr_code_cont, lv_pct(90), lv_pct(30));
    lv_obj_set_style_border_width(qr_code_cont, 0, 0);
    lv_obj_add_style(qr_code_cont, &lv_pro_res.set_bg_grey, 0);
    lv_obj_align(qr_code_cont, LV_ALIGN_BOTTOM_LEFT, lv_pct(5), -10);
    lv_obj_clear_flag(qr_code_cont, LV_OBJ_FLAG_SCROLLABLE);

    char *qr_code_img = "A:/usr/share/lv_projector/qr_code.png";
    char *note_title = lv_get_string(STR_QR_CONFIG);

    char connect_info[256] = {0};
    sprintf(connect_info,"%s\n%s",lv_get_string(STR_ALPHAS_WIRELESS_INFO01),lv_get_string(STR_ALPHAS_WIRELESS_INFO02));
    char *note_text = connect_info;
    lv_obj_t *img_text = lv_pro_img_text_create(qr_code_cont);
    lv_obj_set_size(img_text, lv_pct(60), lv_pct(90));
    lv_obj_align(img_text, LV_ALIGN_LEFT_MID, 20, 0);
    lv_pro_set_img_text_src(img_text, qr_code_img, note_title, note_text, 600);

    lv_obj_t *note_label = lv_label_create(qr_code_cont);
    lv_obj_set_style_text_font(note_label, &GENERAL_FONT_MID, 0);
    lv_label_set_recolor(note_label, true);
    lv_label_set_text_fmt(note_label, "#ffffff %s #", lv_get_string(STR_WIFI_CONNECTING));
    lv_obj_align(note_label, LV_ALIGN_RIGHT_MID, -20, 0);
    lv_obj_add_event_cb(note_label, wirelesssp_event_handler, LV_EVENT_KEY, NULL);
    lv_group_add_obj(WirelessSP_group, note_label);
    lv_group_focus_obj(note_label);

    lv_obj_t *ssid_cont = lv_obj_create(WirelessSP_activity);
    lv_obj_set_size(ssid_cont, lv_pct(90), lv_pct(20));
    lv_obj_set_style_border_width(ssid_cont, 0, 0);
    lv_obj_set_style_bg_opa(ssid_cont, LV_OPA_TRANSP, 0);
    lv_obj_align(ssid_cont, LV_ALIGN_TOP_LEFT, lv_pct(5), lv_pct(2));
    lv_obj_clear_flag(ssid_cont, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *ssid_label = lv_label_create(ssid_cont);
    lv_obj_set_style_text_font(ssid_label, &GENERAL_FONT_BIG, 0);
    lv_label_set_recolor(ssid_label, true);
    lv_obj_align(ssid_label, LV_ALIGN_TOP_LEFT, 5, 5);

    lv_obj_update_layout(WirelessSP_activity);
    return 0;
}

int lv_pro_wirelesssp_deinit(void)
{
    if (WirelessSP_group) {
        lv_group_remove_all_objs(WirelessSP_group);
        lv_group_del(WirelessSP_group);
        WirelessSP_group = NULL;
    }
    if (WirelessSP_activity) {
        lv_obj_del(WirelessSP_activity);
        WirelessSP_activity = NULL;
    }
    return 0;
}

static int create_wirelesssp(void)
{
    static lv_obj_t *htc_msg_box = NULL;

    lv_pro_wirelesssp_init();
    load_current_channel(WirelessSP_activity, WirelessSP_group);

    AWCast_init();
    lv_pro_wirelesssp_ssid_info();
    AWCast_StartService();

    /*
    if (wirelesssp_wifimode < 0) {
        if (!lv_obj_is_valid(htc_msg_box)) {
            htc_msg_box =
                lv_pro_create_message_box((char *)lv_get_string(STR_CONNECT_WIFI_FIRST), 3000);
         }
    }*/
    return 0;
}

static int destory_wirelesssp(void)
{
    if (wirelesssp_wifimode == 1)
        lv_pro_res_wifi_ap_off();

    if (wirelesssp_wifimode >= 0) {
        wirelesssp_wifimode = -1;
    }

    AWCastExitShow();
    AWCast_StopService();
    AWCast_exit();
    return 0;
}

static int show_wirelesssp(void)
{
    return 0;
}

static int hide_wirelesssp(void)
{
    return 0;
}

static page_interface_t page_wirelesssp =
{
    .ops =
    {
        create_wirelesssp,
        destory_wirelesssp,
        show_wirelesssp,
        hide_wirelesssp,
    },
    .info =
    {
        .id = PAGE_WIRELESS_SP,
        .user_data = NULL
    }
};

void REGISTER_PAGE_WIRELESS_SP(void)
{
    reg_page(&page_wirelesssp);
}
