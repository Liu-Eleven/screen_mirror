#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "lvgl/lvgl.h"
#include <lv_common.h>
#include "../Common/setting/system_setting.h"
#include "../System/system_api.h"


#define LV_1_SECOND_UNIT  (1000)

static lv_timer_t *osd_timer = NULL;

static bool osd_timer_is_valid(void)
{
    bool valid = true;

    if (false == valid)
        reset_osd_timer();

    return valid;
}

static void osd_timer_handle(lv_timer_t *timer_)
{
    if(osd_timer_is_valid() == false) {
        return;
    }

    if(osd_timer) {
        lv_timer_del(osd_timer);
        osd_timer = NULL;
    }

    page_id_t last_page_id = get_last_page_id();
    if (last_page_id == PAGE_SOURCE || ((last_page_id >= PAGE_FILE) && (last_page_id <= PAGE_EBOOK)))
        switch_page_show_destory(last_page_id);
    else
        switch_page(PAGE_HOME);
}

void reset_osd_timer(void)
{
    if(osd_timer) {
        lv_timer_reset(osd_timer);
    }
}

void del_osd_timer(void)
{
    if(osd_timer) {
        lv_timer_del(osd_timer);
        osd_timer = NULL;
    }
}

int set_osd_timer(int mode)
{
    //printf("set osd timer >> %d \n", mode);

    if(mode == OSD_TIMER_OFF_ID) {
        if(osd_timer){
            lv_timer_del(osd_timer);
            osd_timer = NULL;
        }
        return 0;
    }
    if(!osd_timer) {
        osd_timer = lv_timer_create(osd_timer_handle, 5*LV_1_SECOND_UNIT, NULL);
        lv_timer_set_repeat_count(osd_timer, -1);
    }

    switch (mode) {
        case OSD_TIMER_5S_ID:
            lv_timer_set_period(osd_timer, 5*LV_1_SECOND_UNIT);
            break;
        case OSD_TIMER_10S_ID:
            lv_timer_set_period(osd_timer, 10*LV_1_SECOND_UNIT);
            break;
        case OSD_TIMER_15S_ID:
            lv_timer_set_period(osd_timer, 15*LV_1_SECOND_UNIT);
            break;
        case OSD_TIMER_20S_ID:
            lv_timer_set_period(osd_timer, 20*LV_1_SECOND_UNIT);
            break;
        case OSD_TIMER_25S_ID:
            lv_timer_set_period(osd_timer, 25*LV_1_SECOND_UNIT);
            break;
        case OSD_TIMER_30S_ID:
            lv_timer_set_period(osd_timer, 30*LV_1_SECOND_UNIT);
            break;
        default:
            lv_timer_set_period(osd_timer, 5*LV_1_SECOND_UNIT);
            break;
    }
    lv_timer_reset(osd_timer);
    return 1;
}
