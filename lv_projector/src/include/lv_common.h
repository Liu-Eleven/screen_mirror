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
#ifndef LV_COMMON_H
#define LV_COMMON_H

#include "lvgl/lvgl.h"
#include "lvgl/src/font/lv_font.h"
#include <projector_config.h>
#if USE_BSD_EVDEV
#include <dev/evdev/input.h>
#else
#include <linux/input.h>
#endif
#include "projector_config.h"

#define loge(fmt, arg...) fprintf(stderr, fmt "\n", ##arg)
#define logw(fmt, arg...)
#define logd(fmt, arg...)
#define logv(fmt, arg...)

#define COLOR_WHITE   lv_color_hex(0xFFFFFF)
#define COLOR_BLACK   lv_color_hex(0x000000)
#define COLOR_BLUE    lv_color_hex(0x031FFF)
#define COLOR_RED     lv_color_hex(0xFF0000)
#define COLOR_GREEN   lv_color_hex(0x00FF00)

/* page */
typedef enum
{
    PAGE_NONE = 0,
    PAGE_HOME,

    PAGE_MEDIA,
    PAGE_DISK,
    PAGE_FILE,
    PAGE_MOVIE,
    PAGE_MUSIC,
    PAGE_PICTURE,
    PAGE_EBOOK,

    PAGE_WIFI,
    PAGE_BT,

    PAGE_SOURCE,

    PAGE_WIRELESS_SP,
    PAGE_AWCAST,
    PAGE_WIRED_SP,
    PAGE_USBCAST,

    PAGE_SETTING,

    PAGE_LOCAL_OTA,
    PAGE_NETWORK_OTA,

    PAGE_FACTORY_TEST,
    PAGE_MEDIA_STRESS,

    PAGE_MAX
} page_id_t;

typedef enum{
    LANGUAGE_ENGLISH,
    LANGUAGE_CHINESE,
    LANGUAGE_FRENCH,
    LANGUAGE_GERMAN,
    LANGUAGE_JAPANESE,
    LANGUAGE_ITALIAN,
    LANGUAGE_SPANISH,
    LANGUAGE_RUSSIAN,
    LANGUAGE_MAX
}language_type_t;

#define FONT_MICROSOFTYAHEI22_ENABLE

#ifdef FONT_MICROSOFTYAHEI22_ENABLE
LV_FONT_DECLARE(Font_MicrosoftYaHei_26)
LV_FONT_DECLARE(Font_MicrosoftYaHei_22)
#else
LV_FONT_DECLARE(Font_MicrosoftYaHei_36)
LV_FONT_DECLARE(Font_MicrosoftYaHei_26)
#endif
LV_FONT_DECLARE(Font_MicrosoftYaHei_14)


#ifdef FONT_MICROSOFTYAHEI22_ENABLE
#define GENERAL_FONT_BIG        Font_MicrosoftYaHei_26
#define GENERAL_FONT_MID        Font_MicrosoftYaHei_22
#define GENERAL_FONT_NORMAL     Font_MicrosoftYaHei_22
#else
#define GENERAL_FONT_BIG        Font_MicrosoftYaHei_36
#define GENERAL_FONT_MID        Font_MicrosoftYaHei_26
#define GENERAL_FONT_NORMAL     Font_MicrosoftYaHei_26
#endif
#define GENERAL_FONT_SMALL      Font_MicrosoftYaHei_14

enum {
    LV_KEY_MENU        = 12,  /*0x0C, '\f'*/
};

uint32_t do_global_event(uint32_t key);

uint8_t *lv_get_string(uint32_t string_id);
uint8_t *lv_get_string_by_langid(uint8_t lang_id, uint32_t string_id);

void set_obj_default_outline_style(lv_obj_t *obj);

void set_current_screen_channel(lv_obj_t *obj);
lv_obj_t * get_current_screen_channel(void);

void set_current_ui_group(lv_group_t *g);

void enable_key_event(void);
void disable_key_event(void);

int is_global_key_go_new_channel(uint32_t key);

#endif
