

#ifndef LV_PRO_USER_COMMON_H_
#define LV_PRO_USER_COMMON_H_

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Common/language/string/lv_string_id.h"
#include "lv_common.h"
#include "sys_param.h"

extern lv_obj_t *lv_pro_create_message_box(char *str, uint32_t period);
extern void lv_pro_msgbox_del_handle(lv_timer_t *timer_);

extern int lv_pro_create_message2_box(char *str);
extern void lv_pro_del_message2_box(void);

void lv_pro_msgbox_msg_clear();
void lv_pro_msgbox_msg_open_on_top(void *icon, char *str_msg, uint32_t timeout);

#endif /* LV_PRO_USER_COMMON_H_ */
