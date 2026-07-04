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
#ifndef SOUND_SETTING_H
#define SOUND_SETTING_H

#define SOUND_MAINKEY              ("sound")
#define SOUND_VOLUME               ("volume")
#define SOUND_MODE_NAME            ("sound_mode")
#define SOUND_OUTPUT_NAME          ("sound_output")

#define SOUND_MODE_STANDARD_NAME   ("s_standard")
#define SOUND_MODE_MUSIC_NAME      ("s_music")
#define SOUND_MODE_MOVIE_NAME      ("s_movie")
#define SOUND_MODE_SPORT_NAME      ("s_sport")
#define SOUND_MODE_USER_NAME       ("s_user")

#define SOUND_BASS                  ("bass")
#define SOUND_TREBLE                ("treble")

#define SOUND_MODE_STANDARD        (0)
#define SOUND_MODE_MUSIC           (1)
#define SOUND_MODE_MOVIC           (2)
#define SOUND_MODE_SPORTS          (3)
#define SOUND_MODE_USER            (4)

#define SOUND_OUTPUT_SPEAKER       (0)
#define SOUND_OUTPUT_ARC           (1)
#define SOUND_OUTPUT_BT            (2)
#define SOUND_OUTPUT_HEADPHONE     (3)
#define SOUND_OUTPUT_OWA           (4)

int factory_set_sound_mode(int value);
int factory_get_sounde_mode(void);

int factory_set_sound_output(int value);
int factory_get_sounde_output(void);

int factory_set_sound_volume(int value);
int factory_get_sounde_volume(void);

int factory_set_aq_param(int mode, char *subkey, int value);
int factory_get_aq_param(int mode, char *subkey);

#endif
