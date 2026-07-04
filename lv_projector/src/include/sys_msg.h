#ifndef SYS_MSG_H
#define SYS_MSG_H

typedef enum {
    // key
    MSG_TYPE_TEST = 0,
    MSG_TYPE_KEY = 1,

    // message
    MSG_TYPE_MSG = 10,
    MSG_TYPE_OPEN_WIN,
    MSG_TYPE_CLOSE_WIN,

    MSG_TYPE_USB_MOUNT,
    MSG_TYPE_USB_UNMOUNT,
    MSG_TYPE_USB_MOUNT_FAIL,
    MSG_TYPE_USB_UNMOUNT_FAIL,
    MSG_TYPE_SD_MOUNT,
    MSG_TYPE_SD_UNMOUNT,
    MSG_TYPE_SD_MOUNT_FAIL,
    MSG_TYPE_SD_UNMOUNT_FAIL,

    MSG_TYPE_WIFI_FRESH_UI,
    MSG_TYPE_WIFI_STA_CONNECT,
    MSG_TYPE_WIFI_STA_DISCONNECT,

    MSG_TYPE_BT_FRESH_UI,
    MSG_TYPE_BT_A2DPSRC_CONNECT,
    MSG_TYPE_BT_A2DPSRC_DISCONNECT,

    MSG_TYPE_MEDIA_FRESH_UI,
    MSG_TYPE_PICTURE_FRESH_UI,

    MSG_TYPE_PAGE_CREATE_DESTORY,
    MSG_TYPE_PAGE_CREATE_HIDE,
    MSG_TYPE_PAGE_SHOW_DESTORY,
} system_msg_type_t;

typedef struct {
    system_msg_type_t type;
    void *data;
} system_msg_t;

void system_message_process(void);
int system_receive_msg(system_msg_t *msg);
int system_send_msg(system_msg_type_t msg_type, void *data);

#endif
