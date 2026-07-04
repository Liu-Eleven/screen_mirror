#include "lvgl/lvgl.h"
#include "lv_common.h"
#include "hdmi_control.h"

bool hdmi_holplug_state;
TSourceId cur_source_id = kSourceId_HDMI_1;

void set_hdmi_hotplug_state(bool value)
{
    hdmi_holplug_state = value;
}

bool get_hdmi_hotplug_state(void)
{
    return hdmi_holplug_state;
}

#if ENABLE_HDMIRX
void HdmiEvent_Callback(TSourceId source_id, TMsgId msg, void *data)    /*deal signal interface hot open and close*/
{
    int ret;
    if(source_id == hdmirx_app_get_currsource()) {
        switch (msg) {
/*            case kMsgId_HPD: {
                int *hpd = (int *)data;
                if (*hpd) {
                    printf("Signal In!\n");
                    hdmi_show_video();

                } else {
                    printf("No Signal!\n");
                    hdmi_no_signal();
                }
                break;
            }
*/
            case kMsgId_RES_CHANGE: {
                struct hdmi_timing_info *timing = (struct hdmi_timing_info *)data;
                printf("timing->signal : %d\n",timing->signal);
                if (!timing->signal) {
                    printf("No Signal!\n");
                    hdmi_no_signal();
                    break;
                }
                printf("Signal In!\n");
                hdmi_show_video();
                if (timing->width != 0 && timing->height != 0) {
                    printf("Resolution Change!\n");
                    hdmi_update_info(timing->width, timing->height, timing->b_interlace, timing->frame_rate);
                }
                break;
            }
        }
    }
}
#endif

void hdmi_control_init(void)
{
#if ENABLE_HDMIRX
    int ret = hdmirx_app_init();
    if(ret) {
        printf("hdmirx_app_init error!\n");
        return ;
    }

    hdmirx_app_register_callback(HdmiEvent_Callback);
#endif
    cec_control_init();

#if ENABLE_HDMIRX
    hdmirx_app_select_source(kSourceId_VideoDec);
#endif
}

int hdmi_get_source_to_map(void)
{
    switch (cur_source_id) {
        case kSourceId_HDMI_1:
            return 1;
        case kSourceId_HDMI_2:
            return 2;
        case kSourceId_HDMI_3:
            return 3;
        default:
            return 1;
    }
    return 1;
}

TSourceId hdmi_get_source(void)
{
    return cur_source_id;
}

void hdmi_select_source(int id)
{
#if ENABLE_HDMIRX
    if (id >= kSourceId_HDMI_1 && id <= kSourceId_HDMI_3) {
        printf("select hdmi%d\n", hdmi_get_source_to_map());
        hdmirx_app_select_source(kSourceId_VideoDec);
        hdmirx_app_select_source(id);
        cur_source_id = id;
    } else if (id == kSourceId_VideoDec) {
        printf("select kSourceId_VideoDec\n");
        hdmirx_app_select_source(kSourceId_VideoDec);
    }
#endif
}

void hdmi_hdcp14_reloadkey(void)
{
#if ENABLE_HDMIRX
    hdmirx_app_hdcp14_reloadkey();
#endif
}
