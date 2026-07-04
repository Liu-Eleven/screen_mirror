#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include "Layer/focus/lv_ui_focuswindow.h"
#include "lvgl/lvgl.h"
#include "key_event.h"
#if USE_BSD_EVDEV
#include <dev/evdev/input.h>
#else
#include <linux/input.h>
#endif

LV_IMG_DECLARE(lens_focus_icon);

#define FOCUS_ROTATE_VAL 450

lv_obj_t *lens_focus_scr = NULL;

static lv_anim_t rotate_animation;
static int focus_rotate_start = 0;
static int focus_rotate_end = 0;
static lv_timer_t * lens_focus_window_timer = NULL;


static void focus_window_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);

    uint32_t *key = lv_event_get_param(e);

    if(code == LV_EVENT_REFRESH)
    {
        if(lv_obj_has_flag(lens_focus_scr, LV_OBJ_FLAG_HIDDEN))
            lv_obj_clear_flag(lens_focus_scr, LV_OBJ_FLAG_HIDDEN);

        if(*key == U_KEY_FOCUS_DOWN)
        {
            if(lv_get_motor_status() != 0x2)
            {
                focus_rotate_end += FOCUS_ROTATE_VAL;
                if(focus_rotate_end > 3600)
                    focus_rotate_end = FOCUS_ROTATE_VAL;

                lv_anim_set_values(&rotate_animation, focus_rotate_start, focus_rotate_end);

                focus_rotate_start += FOCUS_ROTATE_VAL;
                if(focus_rotate_start >= 3600)
                    focus_rotate_start = 0;

                lv_anim_start(&rotate_animation);
            }
            lv_timer_reset(lens_focus_window_timer);
        }
        else if(*key == U_KEY_FOCUS_UP)
        {
            if(lv_get_motor_status() != 0x1)
            {
                focus_rotate_end -= FOCUS_ROTATE_VAL;
                if(focus_rotate_end < -3600)
                    focus_rotate_end = -FOCUS_ROTATE_VAL;

                lv_anim_set_values(&rotate_animation, focus_rotate_start, focus_rotate_end);

                focus_rotate_start -= FOCUS_ROTATE_VAL;
                if(focus_rotate_start <= -3600)
                    focus_rotate_start = 0;

                lv_anim_start(&rotate_animation);
            }
            lv_timer_reset(lens_focus_window_timer);
        }
        else if(*key == LV_KEY_ESC)
        {
            lens_delete_focus_window();
        }
    }
}

static void focus_window_timer_handler(lv_timer_t * timer)
{
    lens_delete_focus_window();
}

static void lens_focus_set_angle(void * img, int32_t v)
{
    lv_img_set_angle(img, v);
}

static lens_focus_rotate_animation(lv_obj_t *obj)
{
    lv_anim_init(&rotate_animation);
    lv_anim_set_var(&rotate_animation, obj);
    lv_anim_set_exec_cb(&rotate_animation, lens_focus_set_angle);
    lv_anim_set_time(&rotate_animation, 100);
    lv_anim_set_repeat_count(&rotate_animation, 1);
}
void lens_delete_focus_window(void)
{
    if(!lv_obj_has_flag(lens_focus_scr, LV_OBJ_FLAG_HIDDEN))
    {
        printf("%s(line %d): delete focus windows !\n", __func__, __LINE__);
        lv_obj_add_flag(lens_focus_scr, LV_OBJ_FLAG_HIDDEN);
        lv_timer_reset(lens_focus_window_timer);
        lv_timer_pause(lens_focus_window_timer);
    }
}

void lens_focus_key_send_handler(uint32_t key)
{
    if(!lens_focus_scr)
        lens_focus_window_init();

	lv_obj_move_foreground(lens_focus_scr);
    lv_event_send(lens_focus_scr, LV_EVENT_REFRESH, &key);
    lv_timer_resume(lens_focus_window_timer);
}

void lens_focus_window_init(void)
{
    printf("%s(line %d): focus windows init !\n", __func__, __LINE__);
    lens_focus_scr = lv_img_create(lv_layer_top());
    lv_img_set_src(lens_focus_scr, &lens_focus_icon);
    lv_obj_align(lens_focus_scr, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(lens_focus_scr, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(lens_focus_scr, focus_window_event_handler, LV_EVENT_REFRESH, NULL);

    lens_focus_rotate_animation(lens_focus_scr);

    lens_focus_window_timer =  lv_timer_create(focus_window_timer_handler, 2000, NULL);
    lv_timer_pause(lens_focus_window_timer);
}

