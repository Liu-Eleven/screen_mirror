#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "sys_param.h"
#include "lvgl/lvgl.h"
#include "picture_setting.h"
#include "system_setting.h"
#include "keystone_setting.h"
#include "sound_setting.h"
#if ENABLE_KSC
#include "ksc/lv_ksc.h"
#endif
#include "volume/volume.h"

sys_param_t sys_param;

void lv_set_sys_param(sys_param_id id, int value)
{
    switch(id) {
        /* PQ */
        case P_PICTURE_MODE:
            sys_param.picture_data.picture_mode = value;
            factory_set_picture_mode_id(value);
            break;
        case P_CONTRAST:
            sys_param.picture_data.contrast = value;
            factory_set_current_pq_para(PQ_CONTRAST_NAME, value, 1);
            break;
        case P_BRIGHTNESS:
            sys_param.picture_data.brightness = value;
            factory_set_current_pq_para(PQ_BRIGHTNESS_NAME, value, 1);
            break;
        case P_SHARPNESS:
            sys_param.picture_data.sharpness = value;
            factory_set_current_pq_para(PQ_SHARPNESS_NAME, value, 1);
            break;
        case P_COLOR:
            sys_param.picture_data.color = value;
            factory_set_current_pq_para(PQ_COLOR_NAME, value, 1);
            break;
        case P_COLOR_TEMP:
            sys_param.picture_data.color_temp = value;
            factory_set_current_pq_para(PQ_COLOR_TEMP_NAME, value, 1);
            break;
        case P_ASPECT_RATIO:
            sys_param.sys_data.aspect_ratio = value;
            factory_set_current_pq_para(PQ_ASPECT_RATIO_NAME, value, 1);
            break;
        case P_SYS_ZOOM_DIS_MODE:
            sys_param.sys_data.zoom_mode = value;
            factory_set_current_pq_para(PQ_ZOOM_NAME, value, 1);
            break;

        /* audio */
        case P_VOLUME:
            sys_param.audio_data.volume = value;
            factory_set_sound_volume(value);
            break;
        case P_MUTE:
            sys_param.audio_data.mute = value;
            break;
        case P_SOUND_MODE:
            sys_param.audio_data.sound_mode = value;
            factory_set_sound_mode(value);
            break;
        case P_SOUND_BASS:
            sys_param.audio_data.bass = value;
            factory_set_aq_param(sys_param.audio_data.sound_mode, SOUND_BASS, value);
            break;
        case P_SOUND_TREBLE:
            sys_param.audio_data.treble = value;
            factory_set_aq_param(sys_param.audio_data.sound_mode, SOUND_TREBLE, value);
            break;
        case P_SOUND_OUT_MODE:
            sys_param.audio_data.sound_output_mode = value;
            factory_set_sound_output(value);
            break;

        /* system */
        case P_OSD_LANGUAGE:
            sys_param.sys_data.language = value;
            factory_set_osd_language(value);
            break;
        case P_COLOR_BG:
            sys_param.sys_data.color_bg = value;
            factory_set_background_style(value);
            break;
        case P_FLIP_MODE:
            sys_param.sys_data.projection_mode = value;
            factory_set_projection_mode(value);
            break;
        case P_AUTOSLEEP:
            sys_param.sys_data.auto_sleep = value;
            factory_set_system_auto_sleep(value);
            break;
        case P_OSD_TIME:
            sys_param.sys_data.osd_timer = value;
            factory_set_osd_timer(value);
            break;
        case P_DISPLAY_W:
            sys_param.sys_data.display_w = value;
            break;
        case P_DISPLAY_H:
            sys_param.sys_data.display_h = value;
            break;
        case P_VERSION_INFO:
            sys_param.sys_data.firmware_version = value;

        /* keystone */
        case P_AUTO_KEYSTONE:
            sys_param.keystone_data.auto_keystone = value;
            factory_set_auto_keystone_mode(value);
            break;
        case P_KEYSTONE_TOP_LEFT_X:
            sys_param.keystone_data.keystone_tl_pos_x = value;
            factory_set_manual_keystone_para(MK_TOP_LEFT_X_ID, value);
            break;
        case P_KEYSTONE_TOP_LEFT_Y:
            sys_param.keystone_data.keystone_tl_pos_y = value;
            factory_set_manual_keystone_para(MK_TOP_LEFT_Y_ID, value);
            break;
        case P_KEYSTONE_TOP_RIGHT_X:
            sys_param.keystone_data.keystone_tr_pos_x = value;
            factory_set_manual_keystone_para(MK_TOP_RIGHT_X_ID, value);
            break;
        case P_KEYSTONE_TOP_RIGHT_Y:
            sys_param.keystone_data.keystone_tr_pos_y = value;
            factory_set_manual_keystone_para(MK_TOP_RIGHT_Y_ID, value);
            break;
        case P_KEYSTONE_BOTTOM_LEFT_X:
            sys_param.keystone_data.keystone_bl_pos_x = value;
            factory_set_manual_keystone_para(MK_BOTTOM_LEFT_X_ID, value);
            break;
        case P_KEYSTONE_BOTTOM_LEFT_Y:
            sys_param.keystone_data.keystone_bl_pos_y = value;
            factory_set_manual_keystone_para(MK_BOTTOM_LEFT_Y_ID, value);
            break;
        case P_KEYSTONE_BOTTOM_RIGHT_X:
            sys_param.keystone_data.keystone_br_pos_x = value;
            factory_set_manual_keystone_para(MK_BOTTOM_RIGHT_X_ID, value);
            break;
        case P_KEYSTONE_BOTTOM_RIGHT_Y:
            sys_param.keystone_data.keystone_br_pos_y = value;
            factory_set_manual_keystone_para(MK_BOTTOM_RIGHT_Y_ID, value);
            break;


        default:
            break;
   }
}

int lv_get_sys_param(sys_param_id id)
{
    switch(id) {
        /* PQ */
        case P_PICTURE_MODE:
            return sys_param.picture_data.picture_mode;
        case P_CONTRAST:
            return sys_param.picture_data.contrast;
        case P_BRIGHTNESS:
            return sys_param.picture_data.brightness;
        case P_SHARPNESS:
            return sys_param.picture_data.sharpness;
        case P_COLOR:
            return sys_param.picture_data.color;
        case P_COLOR_TEMP:
            return sys_param.picture_data.color_temp;
        case P_ASPECT_RATIO:
            return sys_param.sys_data.aspect_ratio;
        case P_SYS_ZOOM_DIS_MODE:
            return sys_param.sys_data.zoom_mode;

        /* audio */
        case P_VOLUME:
            return sys_param.audio_data.volume;
        case P_MUTE:
            return sys_param.audio_data.mute;
        case P_SOUND_MODE:
            return sys_param.audio_data.sound_mode;
        case P_SOUND_BASS:
            return sys_param.audio_data.bass;
        case P_SOUND_TREBLE:
            return sys_param.audio_data.treble;
        case P_SOUND_OUT_MODE:
            return sys_param.audio_data.sound_output_mode;

        /* system */
        case P_OSD_LANGUAGE:
            return sys_param.sys_data.language;
        case P_COLOR_BG:
            return sys_param.sys_data.color_bg;
        case P_FLIP_MODE:
            return sys_param.sys_data.projection_mode;
        case P_AUTOSLEEP:
            return sys_param.sys_data.auto_sleep;
        case P_OSD_TIME:
            return sys_param.sys_data.osd_timer;

        case P_DISPLAY_W:
            return sys_param.sys_data.display_w;
            break;
        case P_DISPLAY_H:
            return sys_param.sys_data.display_h;
            break;
        case P_VERSION_INFO:
            return sys_param.sys_data.firmware_version;

        /* keystone */
        case P_AUTO_KEYSTONE:
            return sys_param.keystone_data.auto_keystone;
            break;
        case P_KEYSTONE_TOP_LEFT_X:
            return sys_param.keystone_data.keystone_tl_pos_x;
        case P_KEYSTONE_TOP_LEFT_Y:
            return sys_param.keystone_data.keystone_tl_pos_y;
        case P_KEYSTONE_TOP_RIGHT_X:
            return sys_param.keystone_data.keystone_tr_pos_x;
        case P_KEYSTONE_TOP_RIGHT_Y:
            return sys_param.keystone_data.keystone_tr_pos_y;
        case P_KEYSTONE_BOTTOM_LEFT_X:
            return sys_param.keystone_data.keystone_bl_pos_x;
        case P_KEYSTONE_BOTTOM_LEFT_Y:
            return sys_param.keystone_data.keystone_bl_pos_y;
        case P_KEYSTONE_BOTTOM_RIGHT_X:
            return sys_param.keystone_data.keystone_br_pos_x;
        case P_KEYSTONE_BOTTOM_RIGHT_Y:
            return sys_param.keystone_data.keystone_br_pos_y;


        default:
            break;
   }
   return 0;
}

void update_picture_param(void)
{
    sys_param.picture_data.picture_mode = factory_get_picture_mode_id();
    sys_param.picture_data.contrast = factory_get_current_pq_para(PQ_CONTRAST_NAME);
    sys_param.picture_data.brightness = factory_get_current_pq_para(PQ_BRIGHTNESS_NAME);
    sys_param.picture_data.sharpness = factory_get_current_pq_para(PQ_SHARPNESS_NAME);
    sys_param.picture_data.color = factory_get_current_pq_para(PQ_COLOR_NAME);
    sys_param.picture_data.color_temp = factory_get_current_pq_para(PQ_COLOR_TEMP_NAME);
    sys_param.sys_data.aspect_ratio = factory_get_current_pq_para(PQ_ASPECT_RATIO_NAME);
    sys_param.sys_data.zoom_mode = factory_get_current_pq_para(PQ_ZOOM_NAME);
}

void update_audio_param(void)
{
    sys_param.audio_data.volume = factory_get_sound_volume();
    sys_param.audio_data.sound_mode = factory_get_sound_mode();
    sys_param.audio_data.bass = factory_get_aq_param(sys_param.audio_data.sound_mode, SOUND_BASS);
    sys_param.audio_data.treble = factory_get_aq_param(sys_param.audio_data.sound_mode, SOUND_TREBLE);
    sys_param.audio_data.sound_output_mode = factory_get_sound_output();

    lv_set_volume(sys_param.audio_data.volume);
    audio_set_eq_mode(sys_param.audio_data.sound_mode);
    audio_set_eq_bass_gain(sys_param.audio_data.bass);
    audio_set_eq_treble_gain(sys_param.audio_data.treble);
}

void update_keystone_param(void)
{
#if ENABLE_KSC
    sys_param.keystone_data.auto_keystone = factory_get_auto_keystone_mode();

    sys_param.keystone_data.keystone_tl_pos_x = factory_get_manual_keystone_para(MK_TOP_LEFT_X_ID);
    sys_param.keystone_data.keystone_tl_pos_y = factory_get_manual_keystone_para(MK_TOP_LEFT_Y_ID);
    sys_param.keystone_data.keystone_tr_pos_x = factory_get_manual_keystone_para(MK_TOP_RIGHT_X_ID);
    sys_param.keystone_data.keystone_tr_pos_y = factory_get_manual_keystone_para(MK_TOP_RIGHT_Y_ID);
    sys_param.keystone_data.keystone_bl_pos_x = factory_get_manual_keystone_para(MK_BOTTOM_LEFT_X_ID);
    sys_param.keystone_data.keystone_bl_pos_y = factory_get_manual_keystone_para(MK_BOTTOM_LEFT_Y_ID);
    sys_param.keystone_data.keystone_br_pos_x = factory_get_manual_keystone_para(MK_BOTTOM_RIGHT_X_ID);
    sys_param.keystone_data.keystone_br_pos_y = factory_get_manual_keystone_para(MK_BOTTOM_RIGHT_Y_ID);

    set_keystone_pos(KEYSTONE_TOP_LEFT_ID, sys_param.keystone_data.keystone_tl_pos_x, sys_param.keystone_data.keystone_tl_pos_y);
    set_keystone_pos(KEYSTONE_TOP_RIGHT_ID, sys_param.keystone_data.keystone_tr_pos_x, sys_param.keystone_data.keystone_tr_pos_y);
    set_keystone_pos(KEYSTONE_BOTTOM_RIGHT_ID, sys_param.keystone_data.keystone_br_pos_x, sys_param.keystone_data.keystone_br_pos_y);
    set_keystone_pos(KEYSTONE_BOTTOM_LEFT_ID, sys_param.keystone_data.keystone_bl_pos_x, sys_param.keystone_data.keystone_bl_pos_y);
    //set_keystone_mirror(sys_param.keystone_data.projection_mode);

    lv_set_zoom();
#endif
}


void update_system_param(void)
{
    sys_param.sys_data.language = factory_get_osd_language();

#if ENABLE_KSC
    sys_param.sys_data.projection_mode = factory_get_projection_mode(0);
#endif

    sys_param.sys_data.color_bg = factory_get_background_style();
    sys_param.sys_data.auto_sleep = factory_get_system_auto_sleep();
    sys_param.sys_data.osd_timer = factory_get_osd_timer();
    sys_param.sys_data.display_w = LV_HOR_RES;
    sys_param.sys_data.display_h = LV_VER_RES;
    sys_param.sys_data.firmware_version = get_firmware_version();
}

void lv_init_sys_param_early(void)
{
#if (ENABLE_KSC && ENABLE_KEY_STONE)
    update_keystone_param();
#endif

    update_system_param();
}

void lv_init_sys_param_late(void)
{
    update_picture_param();
    update_audio_param();
}

