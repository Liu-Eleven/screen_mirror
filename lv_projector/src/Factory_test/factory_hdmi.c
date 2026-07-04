#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "lvgl/lvgl.h"
#include "lv_common.h"
#include "factory_test.h"
#include "hdmiapi.h"

extern lv_obj_t *factory_ui;

void factory_HdmiEvent_Callback(TSourceId source_id, TMsgId msg, void *data)    /*deal signal interface hot open and close*/
{
    if(source_id == kSourceId_HDMI_1) {
        switch (msg) {
            case kMsgId_HPD: {
                int *hpd = (int *)data;
                if (*hpd) {
                    printf("Signal In!\n");
                } else {
                    printf("No Signal!\n");
                    lv_obj_invalidate(factory_ui);
                }
                break;
            }
        }
    }
}

void factory_test_hdmi_start(void)
{
    hdmi_select_source(kSourceId_HDMI_1);
}

void factory_test_hdmi_stop(void)
{
    hdmi_select_source(kSourceId_VideoDec);
    lv_obj_invalidate(factory_ui);
}

void factory_test_hdmi_init(int x, int y, int w_ratio, int h_ratio)
{
    struct displayParam disp_param;
    struct screenParam  screen_param;

    memset(&disp_param, 0, sizeof(struct displayParam));
    memset(&screen_param, 0, sizeof(struct screenParam));

    /* get param first */
#if ENABLE_HDMIRX
    hdmirx_app_get_display_param(&disp_param, &screen_param);
#endif

    /* change it */
    disp_param.zorder = 32;
    disp_param.win_x = x;
    disp_param.win_y = y;

    if (screen_param.width[0] == 1920 || screen_param.width[0] == 1280) {
        disp_param.win_w = screen_param.width[0] * w_ratio / 100;
        disp_param.win_h = screen_param.height[0] * h_ratio / 100;
    } else {
        disp_param.win_w = 400;
        disp_param.win_h = 300;
    }

    /* set disp param */
#if ENABLE_HDMIRX
    hdmirx_app_set_display_param(disp_param);

    hdmirx_app_register_callback(factory_HdmiEvent_Callback);
#endif
}