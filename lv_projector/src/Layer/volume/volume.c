#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "lvgl/lvgl.h"
#include "sys_param.h"
#include "lv_string_id.h"
#include "lv_common.h"
#include "volume.h"
#include "key_event.h"
#if USE_BSD_EVDEV
#include <dev/evdev/input.h>
#else
#include <linux/input.h>
#endif
#include "audio/audio_api.h"

#define VOLUME_MAP_COUNT    32

static lv_obj_t *volume_scr;
static lv_obj_t *volume_bar;
static lv_timer_t *volume_timer;
static lv_timer_t *mute_timer;
static lv_obj_t *icon;

LV_IMG_DECLARE(volume_mute);
LV_IMG_DECLARE(volume_open);

void create_balance_ball(lv_obj_t* parent, lv_coord_t radius, lv_coord_t width)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(obj, radius, 0);
    lv_obj_set_size(obj,LV_PCT(width),LV_PCT(100));
    lv_obj_set_style_bg_color(obj, lv_palette_lighten(LV_PALETTE_GREY, 4), 0);
}

lv_obj_t * create_display_bar_widget(lv_obj_t *parent, int w, int h){
    lv_obj_t *balance = lv_obj_create(parent);

    lv_obj_set_style_radius(balance, 0, 0);
    lv_obj_set_size(balance,LV_PCT(w),LV_PCT(h));
    lv_obj_set_style_outline_width(balance, 0, 0);
    lv_obj_align(balance, LV_ALIGN_BOTTOM_MID, LV_PCT(1), 0);
    lv_obj_set_style_bg_color(balance, lv_palette_lighten(LV_PALETTE_GREY, 2), 0);
    lv_obj_set_style_bg_opa(balance, LV_OPA_50, 0);
    lv_obj_set_flex_flow(balance, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(balance, 0, 0);
    lv_obj_set_style_pad_gap(balance, 0, 0);
    lv_obj_set_flex_align(balance,LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    return balance;
}

lv_obj_t * create_display_bar_main(lv_obj_t* parent, int w, int h, int ball_count, int width)
{
    lv_obj_t *container = lv_obj_create(parent);
    for(int i=0; i<ball_count; i++){
        create_balance_ball(container, 8, width);
    }

    lv_obj_set_style_bg_color(container, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
    lv_obj_set_size(container,LV_PCT(w),LV_PCT(h));
    lv_obj_set_style_outline_width(container, 0, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_ver(container, 0, 0);
    lv_obj_set_style_pad_gap(container, 1, 0);
    lv_obj_set_style_pad_hor(container, 1, 0);
    lv_obj_set_scrollbar_mode(container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_ROW);

    return container;
}

#define set_pad_and_border_and_outline(obj) do { \
    lv_obj_set_style_pad_hor(obj, 0, 0);\
    lv_obj_set_style_border_width(obj, 0, 0); \
     lv_obj_set_style_outline_width(obj,0,0);\
} while(0)

lv_obj_t *create_display_bar_name_part(lv_obj_t* parent,char* name, int w, int h)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    set_pad_and_border_and_outline(obj);
    lv_obj_align(obj, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_radius(obj,0,0);
    lv_obj_set_size(obj,LV_PCT(w),LV_PCT(h));
    lv_obj_set_style_bg_opa(obj, LV_OPA_0, 0);

    lv_obj_t *label = lv_label_create(obj);

    lv_label_set_text(label, name);
    lv_obj_set_style_text_font(label, &GENERAL_FONT_MID, 0);
    lv_obj_center(label);

    return obj;
}

static void set_volume(uint8_t vol){
    audio_set_volume(vol);
}

void lv_set_mute(bool state){
    audio_set_mute(state);
}

static void volume_timer_handler(lv_timer_t* timer1){
    if(volume_bar){
        lv_obj_del(volume_bar);
        volume_bar = NULL;
    }

   volume_timer = NULL;
}

static void mute_timer_handler(lv_timer_t* timer1)
{
    if(icon){
        lv_obj_del(icon);
        icon = NULL;
    }
    mute_timer = NULL;
}

static void event_handler(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    uint32_t *key = lv_event_get_param(e);
    uint8_t real_volume;

    if (code == LV_EVENT_REFRESH) {
        lv_timer_pause(volume_timer);

        if (*key == U_KEY_VOLUME_UP || *key == U_KEY_VOLUME_DOWN || *key == KEY_RESERVED) {
            int8_t count = lv_obj_get_child_cnt(target);
            if (*key == U_KEY_VOLUME_UP && count < VOLUME_MAP_COUNT) {
                count++;
                create_balance_ball(target,8, 3);
            }
            if (*key == U_KEY_VOLUME_DOWN && (count - 1) >= 0) {
                count--;
                lv_obj_del(lv_obj_get_child(target, count));
            }
            real_volume = count * (100 / VOLUME_MAP_COUNT);
            if (real_volume > -1 && real_volume < 101) {
                set_volume(real_volume);
                lv_set_sys_param(P_VOLUME, real_volume);
            }
        } else {
            return ;
        }
        lv_obj_invalidate(target);
        lv_timer_resume(volume_timer);
        lv_timer_reset(volume_timer);
    }
}

bool is_volume_mute(void)
{
    bool is_mute = false;
    audio_get_mute(&is_mute);
    return is_mute;
}

void lv_set_volume(uint8_t volume)
{
    set_volume(volume);
    return;
}

long lv_get_volume(void)
{
    long cur_vol;
    audio_get_volume(&cur_vol);
    return cur_vol;
}

void volume_screen_init(void )
{
    if (volume_scr == NULL)
        volume_scr =lv_layer_top();
}

void del_volume(){
    if(volume_timer){
        lv_timer_del(volume_timer);
       volume_timer = NULL;
    }
    if(volume_bar){
        lv_obj_del(volume_bar);
        volume_bar = NULL;
    }
}

void create_ui_volume(uint32_t key)
{
    volume_screen_init();

    //printf("volume %d\n", lv_get_sys_param(P_VOLUME));

    if (!volume_bar) {
        volume_bar = create_display_bar_widget(volume_scr, 70, 9);
        long cur_vol, min, max = 0;
        lv_obj_set_style_bg_opa(volume_bar, LV_OPA_60, 0);
        lv_obj_set_style_outline_width(volume_bar, 0, 0);
        create_display_bar_name_part(volume_bar, lv_get_string(STR_VOLUME), 25, 100);
        lv_obj_t * container;
        int map_valume = lv_get_sys_param(P_VOLUME) / (100 / VOLUME_MAP_COUNT);
        if (map_valume > VOLUME_MAP_COUNT)
            container = create_display_bar_main(volume_bar, 65, 44,  VOLUME_MAP_COUNT, 3);
        else
            container = create_display_bar_main(volume_bar, 65, 44,  map_valume, 3);
        lv_obj_add_event_cb(container, event_handler, LV_EVENT_REFRESH, 0);

        if (!volume_timer) {
            volume_timer = lv_timer_create(volume_timer_handler, 2000, container);
            lv_timer_set_repeat_count(volume_timer,1);
            lv_timer_reset(volume_timer);
        }
        lv_event_send(container, LV_EVENT_REFRESH, &key);
    } else {
        if (volume_timer) {
            lv_timer_reset(volume_timer);
        }
        lv_event_send(lv_obj_get_child(volume_bar, 1), LV_EVENT_REFRESH, &key);
    }
}

void create_ui_mute_icon()
{
    volume_screen_init();
    bool is_mute = is_volume_mute();
    if (is_mute) {
        if (!icon) {
            icon = lv_img_create(volume_scr);
            lv_obj_align(icon, LV_ALIGN_TOP_LEFT, 30,30);
        }
        lv_img_set_src(icon, &volume_mute);
        if (mute_timer) {
            lv_timer_pause(mute_timer);
        }
    } else {
        if (!icon) {
            icon = lv_img_create(volume_scr);
            lv_obj_align(icon, LV_ALIGN_TOP_LEFT, 30,30);
        }
        lv_img_set_src(icon, &volume_open);
        if (mute_timer) {
            lv_timer_resume(mute_timer);
            lv_timer_reset(mute_timer);
        } else {
            mute_timer = lv_timer_create(mute_timer_handler, 3000, NULL);
            lv_timer_set_repeat_count(mute_timer,1);
            lv_timer_reset(mute_timer);
        }
    }
}
