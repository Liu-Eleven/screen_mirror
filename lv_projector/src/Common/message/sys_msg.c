#include "sys_msg.h"

#include <lv_common.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "lv_pro_msg.h"
#include "lvgl/lvgl.h"
#include "sound_setting.h"

MessageQueue *m_system_msg_queue;

int system_send_msg(system_msg_type_t msg_type, void *data)
{
    if (m_system_msg_queue == NULL) {
        m_system_msg_queue = lv_pro_msg_create_queue();
        if (m_system_msg_queue == NULL) {
            return -1;
        }
    }
    lv_pro_msg_enqueue(m_system_msg_queue, msg_type, data, false);
    return 0;
}

int system_receive_msg(system_msg_t *msg)
{
    LvMessage *tmp_msg;
    if (m_system_msg_queue == NULL) {
        return -1;
    }
    tmp_msg = lv_pro_msg_dequeue(m_system_msg_queue);
    if (tmp_msg) {
        msg->type = tmp_msg->type;
        msg->data = tmp_msg->data;
        lv_pro_msg_free(tmp_msg);
        return 0;
    }
    return -1;
}

static void com_message_process(system_msg_t *msg)
{
    if (!msg) return;

    switch (msg->type) {
    case MSG_TYPE_TEST:
        // printf("MSG_TYPE_TEST \n");
        break;
    case MSG_TYPE_USB_MOUNT:
        update_usb_icon(1);
        break;
    case MSG_TYPE_USB_UNMOUNT:
        if (DiskManager_GetDiskNum() == 0) {
            update_usb_icon(0);
            page_id_t cur_page_id = get_current_page_id();
            if ((cur_page_id >= PAGE_DISK) && (cur_page_id <= PAGE_EBOOK)) {
                switch_page(PAGE_MEDIA);
            }
        }
        break;
    case MSG_TYPE_WIFI_STA_CONNECT:
        network_ota_check();
        break;
    case MSG_TYPE_PAGE_CREATE_DESTORY:
    case MSG_TYPE_PAGE_CREATE_HIDE:
    case MSG_TYPE_PAGE_SHOW_DESTORY:
        if (get_current_page_id() == PAGE_HOME) {
            display_ui_usb_icon();
            display_ui_wifi_bt_icon();
        } else if (get_current_page_id() == PAGE_MEDIA) {
            display_ui_usb_icon();
            hide_ui_wifi_bt_icon();
        } else {
            hide_ui_usb_icon();
            hide_ui_wifi_bt_icon();
        }
        if (get_current_page_id() == PAGE_HOME ||
            get_current_page_id() == PAGE_WIRELESS_SP ||
            get_current_page_id() == PAGE_WIRED_SP) {
            display_ui_activate_icon();
        } else {
            hide_ui_activate_icon();
        }

        check_powerkey_mbox();
        break;
    case MSG_TYPE_BT_A2DPSRC_CONNECT:
        printf("MSG_TYPE_BT_A2DPSRC_CONNECT\n");
        usleep(20 * 1000);
        audio_set_output_device(SOUND_OUTPUT_BT);
        //
        break;
    case MSG_TYPE_BT_A2DPSRC_DISCONNECT:
        printf("MSG_TYPE_BT_A2DPSRC_DISCONNECT\n");
        audio_set_output_device(SOUND_OUTPUT_SPEAKER);
        //
        break;
    default:
        // printf("com message not support %d\n", msg->type);
        break;
    }
}

void system_message_process(void)
{
    system_msg_t msg;

    if (system_receive_msg(&msg)) return;

    //printf("system_message_process type %d\n", msg.type);

    page_message_process(&msg);
    com_message_process(&msg);
#if ENABLE_WIFI
    wifi_message_process(&msg);
#endif
#if ENABLE_BT
    bt_message_process(&msg);
#endif
    media_disk_message_process(&msg);
    media_player_message_process(&msg);
    media_picture_message_process(&msg);
}
