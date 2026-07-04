#ifndef LV_UI_MSGBOX_H
#define LV_UI_MSGBOX_H

#include "lvgl/lvgl.h"
#include "Common/language/string/lv_string_id.h"

typedef void (*msg_timeout_func)(void *user_data);

void lv_msgbox_msg_open(lv_obj_t *parent, char *msg_str, uint32_t timeout,
        msg_timeout_func timeout_func, void *user_data);
void lv_msgbox_msg_close(void);


#endif  /*LV_UI_MSGBOX_H*/
