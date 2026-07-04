#ifndef __OTA_UPDATE_H__
#define __OTA_UPDATE_H__

#include "lvgl/lvgl.h"

#define USE_A_SYSTEM    1
#define USE_B_SYSTEM    2

typedef enum {
    LOCAL_OTA_NO_USB = 0,
    LOCAL_OTA,
    NETWORK_OTA,
} ota_mbox_type_t;


void create_ota_ui(int mode);
int check_local_ota_file(void);

#endif /*__UI_OTA_H__*/
