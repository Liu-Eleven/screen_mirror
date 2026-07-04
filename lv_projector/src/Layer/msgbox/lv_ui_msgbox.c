#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "lv_common.h"
#include "lv_ui_msgbox.h"

static pthread_mutex_t m_mutex_msgbox = PTHREAD_MUTEX_INITIALIZER;
static lv_obj_t *m_obj_msg = NULL;
static lv_timer_t *msgbox_timer = NULL;
static msg_timeout_func m_msg_timeout_func = NULL;
static void *m_timeout_func_data = NULL;

static void msgbox_msg_clear()
{
    if (msgbox_timer){
        lv_timer_pause(msgbox_timer);
        lv_timer_del(msgbox_timer);
    }
    msgbox_timer = NULL;

    if (m_obj_msg && lv_obj_is_valid(m_obj_msg))
    {
        lv_obj_del(m_obj_msg);
    }
    m_obj_msg = NULL;
}

static void msgbox_timer_cb(lv_timer_t * t)
{
    pthread_mutex_lock(&m_mutex_msgbox);
	msgbox_msg_clear();
	
    if (m_msg_timeout_func){
        m_msg_timeout_func(m_timeout_func_data);
    }
    pthread_mutex_unlock(&m_mutex_msgbox);
}

void lv_msgbox_msg_open(lv_obj_t *parent, char *msg_str, uint32_t timeout,
        msg_timeout_func timeout_func, void *user_data)
{
    lv_obj_t * ui_tip_img;
    lv_obj_t * ui_tip_lab;

    if (!m_obj_msg || !lv_obj_is_valid(m_obj_msg)){
        m_obj_msg = lv_obj_create(parent);
        lv_obj_set_size(m_obj_msg,LV_PCT(40),LV_PCT(30));
        lv_obj_set_align(m_obj_msg, LV_ALIGN_CENTER);
        lv_obj_clear_flag(m_obj_msg, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(m_obj_msg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(m_obj_msg, lv_color_hex(0x4D72E0), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(m_obj_msg, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

        // ui_tip_img
        ui_tip_img = lv_img_create(m_obj_msg);
        lv_obj_set_width(ui_tip_img, LV_SIZE_CONTENT);
        lv_obj_set_height(ui_tip_img, LV_SIZE_CONTENT);
        lv_obj_set_x(ui_tip_img, 0);
        lv_obj_set_y(ui_tip_img, 0);
        lv_img_set_src(ui_tip_img, "A:/usr/share/lv_projector/disable.png");
        lv_obj_set_align(ui_tip_img, LV_ALIGN_CENTER);
        lv_obj_add_flag(ui_tip_img, LV_OBJ_FLAG_ADV_HITTEST);
        lv_obj_clear_flag(ui_tip_img, LV_OBJ_FLAG_SCROLLABLE);

        // ui_tip_lab
        ui_tip_lab = lv_label_create(m_obj_msg);
        lv_obj_set_width(ui_tip_lab, LV_SIZE_CONTENT);
        lv_obj_set_height(ui_tip_lab, LV_SIZE_CONTENT);
        lv_obj_set_x(ui_tip_lab, 0);
        lv_obj_set_y(ui_tip_lab, 0);
        lv_obj_set_align(ui_tip_lab, LV_ALIGN_BOTTOM_MID);
        lv_label_set_long_mode(ui_tip_lab,LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_style_text_color(ui_tip_lab, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(ui_tip_lab, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(ui_tip_lab, msg_str);
        lv_obj_set_style_text_font(ui_tip_lab, &GENERAL_FONT_MID, 0);

        if (0 != timeout){
            msgbox_timer = lv_timer_create(msgbox_timer_cb, timeout, NULL);
        }

    }

	if (msgbox_timer)
    {
	    lv_timer_reset(msgbox_timer);
	    lv_timer_set_period(msgbox_timer, timeout);
	}

    if (timeout_func){
        pthread_mutex_lock(&m_mutex_msgbox);
        m_msg_timeout_func = timeout_func;
        m_timeout_func_data = user_data;
        pthread_mutex_unlock(&m_mutex_msgbox);
    }else{
        m_msg_timeout_func = NULL;
        m_timeout_func_data = NULL;

    }
}

void lv_msgbox_msg_close(void)
{
    pthread_mutex_lock(&m_mutex_msgbox);
	msgbox_msg_clear();
    m_msg_timeout_func = NULL;
    m_timeout_func_data = NULL;
    pthread_mutex_unlock(&m_mutex_msgbox);
}

void lv_msgbox_msg_set_pos(lv_align_t align,lv_coord_t x_ofs, lv_coord_t y_ofs)
{
    if(m_obj_msg==NULL||lv_obj_is_valid(m_obj_msg)==false)
        return;    
    lv_obj_align(m_obj_msg,align,x_ofs,y_ofs);
}

bool lv_msgbox_msg_is_open(void)
{
    if(m_obj_msg)
        return lv_obj_is_valid(m_obj_msg);
    else 
        return false;
}

