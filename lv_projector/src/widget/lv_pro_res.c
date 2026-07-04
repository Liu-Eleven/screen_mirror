
#include "lv_pro_res.h"

lv_pro_res_t lv_pro_res;
pthread_mutex_t lvgl_mutex;

static void lv_pro_res_style_init(void) {
    lv_style_init(&lv_pro_res.set_bg_transp);
    lv_style_set_radius(&lv_pro_res.set_bg_transp, 0);
    lv_style_set_border_width(&lv_pro_res.set_bg_transp, 0);
    lv_style_set_pad_all(&lv_pro_res.set_bg_transp, 0);
    lv_style_set_shadow_width(&lv_pro_res.set_bg_transp, 0);
    lv_style_set_bg_opa(&lv_pro_res.set_bg_transp, LV_OPA_TRANSP);

    lv_style_init(&lv_pro_res.set_bg_grey);
    lv_style_set_radius(&lv_pro_res.set_bg_grey, 0);
    lv_style_set_border_width(&lv_pro_res.set_bg_grey, 0);
    lv_style_set_pad_all(&lv_pro_res.set_bg_grey, 0);
    lv_style_set_shadow_width(&lv_pro_res.set_bg_grey, 0);
    lv_style_set_bg_opa(&lv_pro_res.set_bg_grey, LV_OPA_COVER);
    lv_style_set_bg_color(&lv_pro_res.set_bg_grey, lv_color_hex(0xc0c0c0));

    lv_style_init(&lv_pro_res.set_bg_black);
    lv_style_set_radius(&lv_pro_res.set_bg_black, 0);
    lv_style_set_border_width(&lv_pro_res.set_bg_black, 0);
    lv_style_set_pad_all(&lv_pro_res.set_bg_black, 0);
    lv_style_set_shadow_width(&lv_pro_res.set_bg_black, 0);
    lv_style_set_bg_opa(&lv_pro_res.set_bg_black, LV_OPA_COVER);
    lv_style_set_bg_color(&lv_pro_res.set_bg_black, lv_color_hex(0x000000));

    lv_style_init(&lv_pro_res.set_bg_blue);
    lv_style_set_radius(&lv_pro_res.set_bg_blue, 0);
    lv_style_set_border_width(&lv_pro_res.set_bg_blue, 0);
    lv_style_set_pad_all(&lv_pro_res.set_bg_blue, 0);
    lv_style_set_shadow_width(&lv_pro_res.set_bg_blue, 0);
    lv_style_set_bg_opa(&lv_pro_res.set_bg_blue, LV_OPA_COVER);
    lv_style_set_bg_color(&lv_pro_res.set_bg_blue, lv_color_hex(0x0080ff));

    lv_style_init(&lv_pro_res.label_white);
    lv_style_set_text_color(&lv_pro_res.label_white, lv_color_hex(0xffffff));
}

void lv_pro_res_init(void) {
    pthread_mutex_init(&lvgl_mutex, NULL);

    lv_pro_res_style_init();
}
