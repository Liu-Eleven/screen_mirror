/*
 * lv_pro_wiredsp.c
 *
 */

#include "lv_pro_wiredsp.h"
#include <stdlib.h>
#include "../widget/lv_pro_res.h"
#include "../widget/lv_pro_img_text.h"
#include "../lv_pro_launcher.h"
#include "lv_common.h"
#include "lv_string_id.h"
#include "page.h"

lv_obj_t *WiredSP_activity;
lv_group_t *WiredSP_group;

static void wiredsp_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t *key = lv_event_get_param(e);

    if (code == LV_EVENT_KEY) {
        if (*key == LV_KEY_BACK)
            switch_page(PAGE_HOME);
    }
}

void lv_pro_wiredsp_init(void)
{
    if (!WiredSP_activity) {
        WiredSP_activity = lv_obj_create(NULL);
        WiredSP_group = lv_group_create();
    } else
        return;

    lv_obj_set_size(WiredSP_activity, lv_pct(100), lv_pct(100));
    lv_obj_add_style(WiredSP_activity, &lv_pro_res.set_bg_blue, 0);
    lv_obj_clear_flag(WiredSP_activity, LV_OBJ_FLAG_SCROLLABLE);

    char *device_img[] = { "A:/usr/share/lv_projector/cast/wired_android.png",
            "A:/usr/share/lv_projector/cast/wired_ios.png" };
    char *cont_title[2];
    cont_title[0] = lv_get_string(STR_ANDROID_WIREDSP);
    cont_title[1] = lv_get_string(STR_ALPHAS_IOSCAST_STR1);

    char android_info[256] = {0};
    sprintf(android_info, "%s\n%s\n\n%s\n%s\n%s", lv_get_string(STR_ANDROID_WIREDSP_INFO3),
            lv_get_string(STR_ANDROID_WIREDSP_INFO4), lv_get_string(STR_ANDROID_WIREDSP_INFO5),
            lv_get_string(STR_ANDROID_WIREDSP_INFO6), lv_get_string(STR_ANDROID_WIREDSP_INFO7));

    char *cont_text[2];
    cont_text[0] = android_info;
    cont_text[1] = lv_get_string(STR_ALPHAS_IOSCAST_STR2);

    /*Create a container with grid*/
    lv_obj_t *cont = lv_obj_create(WiredSP_activity);
    lv_obj_set_size(cont, lv_pct(90), lv_pct(60));
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_align(cont, LV_ALIGN_TOP_LEFT, lv_pct(5), 0);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(cont, col_dsc, row_dsc);
    lv_obj_set_style_pad_column(cont, 20, 0);
    lv_obj_set_style_pad_row(cont, 20, 0);
    lv_obj_set_layout(cont, LV_LAYOUT_GRID);

    for (uint32_t i = 0; i < 2; i++) {
        uint8_t col = i % 2;
        uint8_t row = 0;

        lv_obj_t *img_text = lv_pro_img_text_create(cont);
        lv_obj_set_size(img_text, lv_pct(40), lv_pct(60));
        lv_obj_set_grid_cell(img_text, LV_GRID_ALIGN_STRETCH, col, 1,
                         LV_GRID_ALIGN_STRETCH, row, 1);
        lv_pro_set_img_text_src(img_text, device_img[i], cont_title[i], cont_text[i], 350);
    }

    /*Create a container with qr_code and note*/
    lv_obj_t *qr_code_cont = lv_obj_create(WiredSP_activity);
    lv_obj_set_size(qr_code_cont, lv_pct(90), lv_pct(30));
    lv_obj_set_style_border_width(qr_code_cont, 0, 0);
    lv_obj_add_style(qr_code_cont, &lv_pro_res.set_bg_black, 0);
    lv_obj_align(qr_code_cont, LV_ALIGN_BOTTOM_LEFT, lv_pct(5), -40);
    lv_obj_clear_flag(qr_code_cont, LV_OBJ_FLAG_SCROLLABLE);

	char *qr_code_img = "A:/usr/share/lv_projector/qr_code.png";
	char *note_title = lv_get_string(STR_ANDROID_SCAN_QR);
	char note_text[256] = {0};
    sprintf(note_text, "%s\n%s", lv_get_string(STR_ANDROID_WIREDSP_INFO1), lv_get_string(STR_ANDROID_WIREDSP_INFO2));

    lv_obj_t *img_text = lv_pro_img_text_create(qr_code_cont);
    lv_obj_set_size(img_text, lv_pct(60), lv_pct(95));
    lv_obj_align(img_text, LV_ALIGN_LEFT_MID, 20, 0);
    lv_pro_set_img_text_src(img_text, qr_code_img, note_title, note_text, 600);

    lv_obj_t *note_label = lv_label_create(qr_code_cont);
    lv_obj_set_style_text_font(note_label, &GENERAL_FONT_BIG, 0);
    lv_label_set_recolor(note_label, true);
    lv_label_set_text_fmt(note_label, "#ffffff %s #", lv_get_string(STR_INSERT_ANDROID_OR_APPLE));
    lv_obj_align(note_label, LV_ALIGN_RIGHT_MID, -20, 0);
    lv_obj_add_event_cb(note_label, wiredsp_event_handler, LV_EVENT_KEY, NULL);
    lv_group_add_obj(WiredSP_group, note_label);

    lv_obj_update_layout(WiredSP_activity);
}

void lv_pro_wiredsp_exit(void)
{
    if (!WiredSP_activity)
        return;

    lv_group_remove_all_objs(WiredSP_group);
    lv_obj_del(WiredSP_activity);
    lv_group_del(WiredSP_group);
    WiredSP_group = NULL;
    WiredSP_activity = NULL;
}

static int create_wiredsp(void)
{
    lv_pro_wiredsp_init();
    load_current_channel(WiredSP_activity, WiredSP_group);

    USBCast_init();
    USBCast_StartService();
    return 0;
}

static int destory_wiredsp(void)
{
    USBCastExitShow();
    USBCast_StopService();
    USBCast_exit();
    lv_pro_wiredsp_exit();
    return 0;
}

static int show_wiredsp(void)
{
    return 0;
}

static int hide_wiredsp(void)
{
    return 0;
}

static page_interface_t page_wiredsp =
{
    .ops =
    {
        create_wiredsp,
        destory_wiredsp,
        show_wiredsp,
        hide_wiredsp,
    },
    .info =
    {
        .id = PAGE_WIRED_SP,
        .user_data = NULL
    }
};

void REGISTER_PAGE_WIRED_SP(void)
{
    reg_page(&page_wiredsp);
}
