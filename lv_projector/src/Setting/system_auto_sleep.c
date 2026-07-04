#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "lvgl/lvgl.h"
#include <lv_common.h>
#include "../Common/setting/system_setting.h"
#include "../System/system_api.h"
#include "lv_common.h"
#include "sys_param.h"
#include "../Common/language/string/lv_string_id.h"
#include "lv_pro_launcher.h"

#define AUTO_SLEEP_UNIT  (60000) // 1 mim
#define COUNT_DOWN 60

typedef struct {
    lv_obj_t *label;
    lv_obj_t *prev_obj;
} sleep_timer_data_t;

static lv_obj_t *sleep_obj = NULL;
static lv_obj_t *auto_sleep_prev_obj = NULL;
static lv_timer_t *auto_sleep_sure_timer = NULL;
static lv_timer_t *auto_sleep_timer = NULL;
static int countdown = COUNT_DOWN;

#define set_pad_and_border_and_outline(obj) do { \
    lv_obj_set_style_pad_hor(obj, 0, 0);\
    lv_obj_set_style_border_width(obj, 0, 0); \
     lv_obj_set_style_outline_width(obj,0,0);\
} while(0)

static bool autosleep_is_valid(void)
{
    lv_obj_t *cur_screen;
    bool valid = true;

    cur_screen = get_current_screen_channel();
    if (cur_screen == Media_activity ||
        cur_screen == WirelessSP_activity ||
        cur_screen == WiredSP_activity ||
        cur_screen == SignalIn_activity) {

        valid = false;
    }

    if (false == valid)
        reset_autosleep_timer();

    return valid;
}

static void auto_sleep_sure_timer_handle(lv_timer_t *timer_)
{
    sleep_timer_data_t *timer_data = (sleep_timer_data_t* )timer_->user_data;

    if(countdown==0){
        printf("Enter %ld(S) auto sleep!\n", auto_sleep_timer->period/1000);

        if (timer_data->prev_obj)
            lv_group_focus_obj(timer_data->prev_obj);

        lv_mem_free(timer_data);

        if (auto_sleep_sure_timer)
            lv_timer_del(auto_sleep_sure_timer);
        auto_sleep_sure_timer = NULL;

        if (auto_sleep_timer)
             lv_timer_del(auto_sleep_timer);
        auto_sleep_timer = NULL;

        if (sleep_obj)
            lv_obj_del(sleep_obj);
        sleep_obj = NULL;

        system_standby();
        return;
    }
    lv_obj_t *label = (lv_obj_t *)timer_data->label;
    lv_label_set_text_fmt(label, "%d %s", countdown--, lv_get_string(STR_SET_SLEEP_COUNTDOWN));
    lv_obj_set_style_text_font(label, &GENERAL_FONT_MID, 0);
}

static void auto_sleep_sure_event_handle(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    sleep_timer_data_t *timer_data = (sleep_timer_data_t *)lv_event_get_user_data(e);

    if(code == LV_EVENT_KEY){
        if (auto_sleep_sure_timer)
            lv_timer_del(auto_sleep_sure_timer);
        auto_sleep_sure_timer = NULL;

        if (sleep_obj)
            lv_obj_del(sleep_obj);
        sleep_obj = NULL;

        lv_group_focus_obj(timer_data->prev_obj);
	lv_mem_free(timer_data);
        reset_autosleep_timer();
    }
}

static void auto_sleep_timer_handle(lv_timer_t *timer_)
{
    //关机前一分钟弹出倒数计时，从60到0，按任何键结束计时
    if(autosleep_is_valid() == false || sleep_obj)
    {
        return;
    }

    countdown = COUNT_DOWN;

    sleep_obj = lv_obj_create(lv_layer_top());
    auto_sleep_prev_obj = lv_group_get_focused(lv_group_get_default());
    lv_obj_set_style_text_color(sleep_obj, lv_color_white(),0);
    lv_obj_set_size(sleep_obj, LV_PCT(30), LV_PCT(20));
    lv_obj_center(sleep_obj);
    set_pad_and_border_and_outline(sleep_obj);
    lv_obj_set_style_pad_ver(sleep_obj, 0, 0);
    lv_obj_set_style_bg_color(sleep_obj, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
    lv_obj_set_style_bg_opa(sleep_obj, LV_OPA_90, 0);

    char * auto_sleep_str = lv_get_string(STR_SET_SLEEP_COUNTDOWN);
    lv_coord_t str_width = lv_txt_get_width(auto_sleep_str, strlen(auto_sleep_str), &GENERAL_FONT_MID, 0, LV_TEXT_FLAG_NONE);
    lv_obj_set_width(sleep_obj, 100 + str_width);

    lv_obj_t* label = lv_label_create(sleep_obj);
    lv_obj_center(label);
    lv_obj_set_size(label, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

    sleep_timer_data_t *timer_data = lv_mem_alloc(sizeof(sleep_timer_data_t));
    timer_data->label = label;
    timer_data->prev_obj = auto_sleep_prev_obj;

    if (!auto_sleep_sure_timer)
        auto_sleep_sure_timer = lv_timer_create(auto_sleep_sure_timer_handle, 1000, timer_data);
    lv_timer_set_repeat_count(auto_sleep_sure_timer, COUNT_DOWN+1);
    lv_timer_ready(auto_sleep_sure_timer);

    lv_group_add_obj(lv_group_get_default(), sleep_obj);
   // auto_sleep_prev_obj = lv_group_get_focused(lv_group_get_default());
    lv_group_focus_obj(sleep_obj);
    lv_obj_add_event_cb(sleep_obj, auto_sleep_sure_event_handle, LV_EVENT_ALL, timer_data);
}

void reset_autosleep_timer(void)
{
    if(auto_sleep_timer)
    {
        lv_timer_reset(auto_sleep_timer);
    }
}

void del_autosleep_timer(void)
{
    if (auto_sleep_sure_timer)
        lv_timer_del(auto_sleep_sure_timer);
    auto_sleep_sure_timer = NULL;

    if (auto_sleep_timer)
        lv_timer_del(auto_sleep_timer);
    auto_sleep_timer = NULL;

    if (sleep_obj)
        lv_obj_del(sleep_obj);
    sleep_obj = NULL;
}


int set_system_auto_sleep(int mode){
    printf("set auto sleep >> %d \n", mode);

    if(mode == AUTO_SLEEP_OFF_ID){
        if(auto_sleep_timer){
            lv_timer_del(auto_sleep_timer);
            auto_sleep_timer = NULL;
        }
        return 0;
    }
    if(!auto_sleep_timer){
        auto_sleep_timer = lv_timer_create(auto_sleep_timer_handle, 1*AUTO_SLEEP_UNIT, NULL);//3600000
        lv_timer_set_repeat_count(auto_sleep_timer, -1);
    }

    switch (mode) {
        case AUTO_SLEEP_10M_ID:
            lv_timer_set_period(auto_sleep_timer, 10*AUTO_SLEEP_UNIT);
            break;
        case AUTO_SLEEP_30M_ID:
            lv_timer_set_period(auto_sleep_timer, 30*AUTO_SLEEP_UNIT);
            break;
        case AUTO_SLEEP_60M_ID:
            lv_timer_set_period(auto_sleep_timer, 60*AUTO_SLEEP_UNIT);
            break;
        case AUTO_SLEEP_90M_ID:
            lv_timer_set_period(auto_sleep_timer, 90*AUTO_SLEEP_UNIT);
            break;
        case AUTO_SLEEP_120M_ID:
            lv_timer_set_period(auto_sleep_timer, 120*AUTO_SLEEP_UNIT);
            break;
        default:
            lv_timer_set_period(auto_sleep_timer, 10*AUTO_SLEEP_UNIT);
            break;
    }
    lv_timer_reset(auto_sleep_timer);
    return 1;
}
