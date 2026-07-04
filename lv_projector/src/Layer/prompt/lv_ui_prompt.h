#ifndef _LV_UI_PROMPT_H
#define _LV_UI_PROMPT_H

#include "lvgl/lvgl.h"

typedef struct {
    char* status;
    void* devName;
} main_page_prompt_data_t;

lv_obj_t* main_page_prompt_create(lv_obj_t* parent, void* icon);

#endif
