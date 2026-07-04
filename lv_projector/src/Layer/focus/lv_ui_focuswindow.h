#ifndef _LV_UI_FOCUS_WINDOW_H_
#define _LV_UI_FOCUS_WINDOW_H_

#include "lvgl/lvgl.h"


void lens_focus_window_init(void);
void lens_delete_focus_window(void);
void lens_focus_key_send_handler(uint32_t key);

#endif
