#ifndef _LV_UI_USB_H
#define _LV_UI_USB_H

#include "lvgl/lvgl.h"

#define USB_MOUNT_MAX_COUNT (8)
extern lv_obj_t *usb_show;

void create_ui_usb_icon(lv_obj_t *parent);
void del_ui_usb_icon(void);
void display_ui_usb_icon(void);
void hide_ui_usb_icon(void);
int get_usb_status(void);
#endif
