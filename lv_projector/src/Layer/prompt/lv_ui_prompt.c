#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "Layer/usb/lv_ui_usb.h"
#include "Layer/wifi_bt/lv_ui_wifi.h"
#include "lv_common.h"
#include "lvgl/lvgl.h"

static lv_obj_t* prompt_show;

lv_obj_t* main_page_prompt_create(lv_obj_t* parent, void* icon)
{
    if (!prompt_show) {
        prompt_show = lv_obj_create(parent);
        lv_obj_set_size(prompt_show, lv_pct(80), lv_pct(12));
        lv_obj_set_style_bg_opa(prompt_show, LV_OPA_0, 0);
        lv_obj_align(prompt_show, LV_ALIGN_TOP_RIGHT, -100, 20);
        lv_obj_set_style_pad_all(prompt_show, 0, 0);
        lv_obj_set_style_border_width(prompt_show, 0, 0);
        lv_obj_set_scrollbar_mode(prompt_show, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_flex_flow(prompt_show, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(prompt_show, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    }

    lv_obj_t* show = lv_obj_create(prompt_show);
    lv_obj_set_size(show, 40, LV_PCT(100));
    lv_obj_set_style_border_width(show, 0, 0);
    lv_obj_set_style_pad_hor(show, 0, 0);

    lv_obj_set_scrollbar_mode(show, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(show, LV_OPA_0, 0);
    lv_obj_set_style_text_color(show, lv_color_white(), 0);
    lv_obj_set_style_border_width(show, 0, 0);
    lv_obj_set_style_pad_all(show, 0, 0);

    lv_obj_t* img = lv_img_create(show);
    lv_obj_align(img, LV_ALIGN_LEFT_MID, 0, 0);
    lv_img_set_src(img, icon);

#if 1  // #if WIFI_BT_NAME_SHOW
    lv_obj_t* __label = lv_label_create(show);
    lv_obj_align_to(__label, img, LV_ALIGN_OUT_RIGHT_TOP, 10, -3);
    lv_obj_set_size(__label, 0, LV_SIZE_CONTENT);
    lv_label_set_text(__label, "");
    lv_obj_set_style_text_font(__label, &GENERAL_FONT_MID, 0);
    lv_label_set_long_mode(__label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_anim_speed(__label, 30, 0);
#endif
    return show;
}
