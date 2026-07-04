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
#ifndef SYSTEM_SETTING_H
#define SYSTEM_SETTING_H

#define SYSTEM_SETTING_MAINKEY		("system")
#define BG_SUBKEY                   ("background_color")
#define PROJECTION_MODE_SUBKEY      ("projection_mode")
#define AUTO_SLEEP_SUBKEY           ("auto_sleep")
#define OSD_TIMER_SUBKEY            ("osd_timer")
#define OSD_LANGUAGE_SUBKEY         ("language")

#define BG_COLOR_0_ID               (0)
#define BG_COLOR_1_ID               (1)
#define BG_COLOR_2_ID               (2)
#define BG_COLOR_0_NAME             ("color0")
#define BG_COLOR_1_NAME             ("color1")
#define BG_COLOR_2_NAME             ("color2")
#define BG_COLOR_0_RGB              (0x1a3761)
#define BG_COLOR_1_RGB              (0x031FFF)
#define BG_COLOR_2_RGB              (0x464646)


#define PROJECTION_FRONT_ID         (0)
#define PROJECTION_CEILING_FRONT_ID (1)
#define PROJECTION_REAR_ID          (2)
#define PROJECTION_REAR_CEILING_ID  (3)
#define PROJECTION_INVALID          (4)

#define AUTO_SLEEP_OFF_ID           (0)
#define AUTO_SLEEP_10M_ID           (1)
#define AUTO_SLEEP_30M_ID           (2)
#define AUTO_SLEEP_60M_ID           (3)
#define AUTO_SLEEP_90M_ID           (4)
#define AUTO_SLEEP_120M_ID          (5)

#define OSD_TIMER_OFF_ID            (0)
#define OSD_TIMER_5S_ID             (1)
#define OSD_TIMER_10S_ID            (2)
#define OSD_TIMER_15S_ID            (3)
#define OSD_TIMER_20S_ID            (4)
#define OSD_TIMER_25S_ID            (5)
#define OSD_TIMER_30S_ID            (6)


int factory_set_background_style(int value);
int factory_get_background_style(void);
int factory_get_background_style_string(char *value);

int factory_reset_and_clear_networkfiles(void);
int factory_reset_setting(void);

int factory_set_projection_mode(int value);
int factory_get_projection_mode(int is_from_disp_config_ini);

int factory_set_system_auto_sleep(int value);
int factory_get_system_auto_sleep(void);

int factory_set_osd_timer(int value);
int factory_get_osd_timer(void);

int factory_set_osd_language(int value);
int factory_get_osd_language(void);

#endif
