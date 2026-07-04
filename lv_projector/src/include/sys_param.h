/* Copyright (c) 2019-2035 Allwinner Technology Co., Ltd. ALL rights reserved.

 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the the People's Republic of China and other countries.
 * All Allwinner Technology Co.,Ltd. trademarks are used with permission.

 * DISCLAIMER
 * THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
 * IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
 * IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
 * ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
 * ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
 * COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
 * YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.


 * THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
 * PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
 * THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
 * OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef SYS_PARAMETER_H
#define SYS_PARAMETER_H

#include <stdint.h>

typedef enum {
    P_PICTURE_MODE,
    P_CONTRAST,
    P_BRIGHTNESS,
    P_SHARPNESS,
    P_COLOR,
    P_HUE,
    P_COLOR_TEMP,
    P_NOISE_REDU,
    P_SOUND_MODE,
    P_SOUND_BASS,
    P_SOUND_TREBLE,
    P_SOUND_EQ,
    P_EQ_MODE,
    P_BALANCE,
    P_OSD_LANGUAGE,
    P_ASPECT_RATIO,
    P_CUR_CHANNEL,
    P_FLIP_MODE,
    P_COLOR_BG,
    P_OSD_TIME,
    P_VOLUME,
    P_MUTE,
    P_RESTORE,
    P_UPDATE,
    P_AUTOSLEEP,
    P_KEYSTONE,
    P_AUTO_KEYSTONE,
    P_VERSION_INFO,
    P_DEV_PRODUCT_ID,
    P_DEV_VERSION,
    P_MIRROR_MODE,
    P_AIRCAST_MODE,
    P_MIRROR_FRAME,
    P_BROWSER_LANGUAGE,
    P_SYS_RESOLUTION,
    P_DEVICE_NAME,
    P_DEVICE_PSK,
    P_WIFI_MODE,
    P_WIFI_CHANNEL,
    P_MIRROR_ROTATION,
    P_MIRROR_VSCREEN_AUTO_ROTATION,
    P_DE_TV_SYS,
    P_VMOTOR_COUNT,
    P_SYS_ZOOM_DIS_MODE,
    P_SYS_ZOOM_OUT_COUNT,
	P_SYS_OTA_UPGRADE,
    P_FACTORY_CHANNEL_SELECT,
    P_PQ_ONFF,
    P_DYN_CONTRAST_ONOFF,
    P_MEM_PLAY_MEDIA_TYPE,    
    P_VIDEO_DELAY,
    P_SOUND_OUT_MODE,    
    P_SOUND_SPDIF_MODE,
    P_MIRROR_FULL_VSCREEN,
    P_MIRA_CONTINUE_ON_ERROR,
    P_UM_FULL_SCREEN,
    P_TIMER_POWEROFF,
    P_LANGAGE,
    P_KEYSTONE_TOP_LEFT_X,
    P_KEYSTONE_TOP_LEFT_Y,
    P_KEYSTONE_TOP_RIGHT_X,
    P_KEYSTONE_TOP_RIGHT_Y,
    P_KEYSTONE_BOTTOM_LEFT_X,
    P_KEYSTONE_BOTTOM_LEFT_Y,
    P_KEYSTONE_BOTTOM_RIGHT_X,
    P_KEYSTONE_BOTTOM_RIGHT_Y,
    P_DISPLAY_W,
    P_DISPLAY_H,
} sys_param_id;

struct picturedata{
    uint8_t picture_mode;
    uint8_t contrast;
    uint8_t brightness;
    uint8_t sharpness;
    uint8_t color;
    uint8_t color_temp;
};

struct audiodata{
    uint8_t mute;
    uint8_t volume;
    uint8_t sound_mode;
    int8_t bass;
    int8_t treble;
    uint8_t sound_output_mode;
};

struct keystonedata{
    uint8_t auto_keystone;
    uint16_t keystone_tl_pos_x;
    uint16_t keystone_tl_pos_y;
    uint16_t keystone_tr_pos_x;
    uint16_t keystone_tr_pos_y;
    uint16_t keystone_bl_pos_x;
    uint16_t keystone_bl_pos_y;
    uint16_t keystone_br_pos_x;
    uint16_t keystone_br_pos_y;
};

struct sysdata{
    uint8_t color_bg;
    uint8_t language;
    uint8_t zoom_mode;
    uint8_t aspect_ratio;
    uint8_t projection_mode;
    uint8_t auto_sleep;
    uint8_t osd_timer;
    uint16_t display_w;
    uint16_t display_h;
    uint32_t firmware_version;
};

typedef struct sys_param{
    struct picturedata picture_data;
    struct audiodata audio_data;
    struct keystonedata keystone_data;
    struct sysdata sys_data;
}sys_param_t;

void lv_set_sys_param(sys_param_id id, int value);
int lv_get_sys_param(sys_param_id id);

void lv_init_sys_param(void);
void update_picture_param(void);
void update_audio_param(void);
void update_system_param(void);

#endif