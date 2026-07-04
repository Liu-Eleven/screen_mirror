#include "lv_ui_usb.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "Layer/prompt/lv_ui_prompt.h"
#include "disk_manager/DiskManager.h"
#include "lvgl/lvgl.h"
#include "sys_msg.h"

lv_obj_t *usb_show;
static int8_t usb_state;
static hotplug_message_focus_win_t *USBRegisterInfo = NULL;

LV_IMG_DECLARE(usb_con);
LV_IMG_DECLARE(usb_discon);

static void usb_manager_callback(hotplug_disk_message_t *hotplug_message)
{
#if 0
    printf("DiskInfo->index=%d\n", hotplug_message->diskinfo->index);
    printf("DiskInfo->DeviceName=%s\n", hotplug_message->diskinfo->DeviceName);
    printf("DiskInfo->MountPoint=%s\n", hotplug_message->diskinfo->MountPoint);
    printf("DiskInfo->operate=%d\n", (int)hotplug_message->operate);
    printf("DiskInfo->MediaType=%d\n", (int)hotplug_message->diskinfo->MediaType);
    printf("DiskInfo->Major=%d\n", (int)hotplug_message->diskinfo->Major);
    printf("DiskInfo->Minor=%d\n", (int)hotplug_message->diskinfo->Minor);
#endif
    // if disk  pullout when it is using, it will mount failed when disk is inserted again.
    if (hotplug_message->operate == MEDIUM_PLUGOUT) {
        int data = hotplug_message->diskinfo->index;
        system_send_msg(MSG_TYPE_USB_UNMOUNT, data);
        usb_state = 0;
    } else if (hotplug_message->operate == MEDIUM_PLUGIN || hotplug_message->operate == MEDIUM_STATIC_PLUGIN) {
        system_send_msg(MSG_TYPE_USB_MOUNT, NULL);
        usb_state = 1;
    }
}

void update_usb_icon(int status)
{
    if (status)
        lv_img_set_src(lv_obj_get_child(usb_show, 0), &usb_con);
    else
        lv_img_set_src(lv_obj_get_child(usb_show, 0), &usb_discon);
}

static void usb_hotplug_init(void)
{
    if (USBRegisterInfo == NULL) {
        USBRegisterInfo = malloc(sizeof(hotplug_message_focus_win_t));
        if (USBRegisterInfo != NULL) {
            memset(USBRegisterInfo, 0x00, sizeof(hotplug_message_focus_win_t));
            USBRegisterInfo->CallBackFunction = usb_manager_callback;
            strcpy(USBRegisterInfo->Cur_Win, "usb_ui");
        }
    }
    DiskManager_Register(USBRegisterInfo);
}

int get_usb_status(void) { return usb_state; }

void del_usb_icon(void) {}

void display_ui_usb_icon(void)
{
    lv_obj_clear_flag(usb_show, LV_OBJ_FLAG_HIDDEN);
}

void hide_ui_usb_icon(void)
{
    lv_obj_add_flag(usb_show, LV_OBJ_FLAG_HIDDEN);
}

void create_ui_usb_icon(lv_obj_t *parent)
{
    usb_show = main_page_prompt_create(parent, &usb_discon);

    usb_hotplug_init();
}
