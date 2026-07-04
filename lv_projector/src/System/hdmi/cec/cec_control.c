#include "lv_common.h"
#include "hdmiapi.h"
#include "key_event.h"

static void parseCecMsg(void *data)
{
    uint8_t logicalAddr;

    TCECMessage* msg = (TCECMessage *)data;
    switch (msg->Content.Msg.ucOpcode) {
        case OPCODE_REPORT_PHYSICAL_ADDRESS:
            /*  When the device is connected, it will report the physical address to the TV. */
            logicalAddr = (msg->Content.Msg.ucHeader & 0xF0) >> 4;
            printf(">>>> logicalAddr: %d connect! <<<<\n", logicalAddr);
            break;
        default:
            printf(">>>> UNKONW <<<<\n");
            break;
    }
}

static void lv_hdmirx_cec_callback(TSourceId source_id, TMsgId msg, void *data)
{
    switch (msg) {
        case kMsgId_CEC:
            parseCecMsg(data);
            break;
        default:
            break;
    }
}

int hdmi_cec_send_key(uint32_t act_key)
{
#if (ENABLE_CEC_KEY_PASS_THROUGH && ENABLE_CEC)
    uint32_t CecKeyCode;

    switch (act_key) {
        case LV_KEY_ENTER:
            CecKeyCode = UsrControlCode_Enter;
            break;
        case LV_KEY_BACK:
            CecKeyCode = UsrControlCode_Exit;
            break;
        case LV_KEY_UP:
            CecKeyCode = UsrControlCode_Up;
            break;
        case LV_KEY_DOWN:
            CecKeyCode = UsrControlCode_Down;
            break;
        case LV_KEY_LEFT:
            CecKeyCode = UsrControlCode_Left;
            break;
        case LV_KEY_RIGHT:
            CecKeyCode = UsrControlCode_Right;
            break;
        default:
            return -1;
    }
#if ENABLE_HDMIRX
    if(!hdmirx_app_cec_press_key(CecKeyCode, NULL))
        hdmirx_app_cec_release_key();
#endif
#endif
    return 0;
}

void cec_control_init(void)
{
    //hdmirx_app_init();

#if ENABLE_CEC
#if ENABLE_HDMIRX
    hdmirx_app_cec_set_status(1);
    hdmirx_app_cec_register_callback(lv_hdmirx_cec_callback);
#endif
#else
    hdmirx_app_cec_set_status(0);
#endif
}
