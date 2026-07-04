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
#ifndef PICTURE_SETTING_H
#define PICTURE_SETTING_H

#define PICTURE_MODE_STANDARD_ID	0
#define PICTURE_MODE_DYNAMIC_ID		1
#define PICTURE_MODE_MILD_ID		2
#define PICTURE_MODE_USER_ID		3

#define PICTURE_MAINKEY             ("picture")
#define PICTURE_MODE_MAINKEY        ("mode")
#define PICTURE_MODE_STANDARD_NAME  ("standard")
#define PICTURE_MODE_DYNAMIC_NAME   ("dynamic")
#define PICTURE_MODE_MILD_NAME      ("mild")
#define PICTURE_MODE_USER_NAME      ("user")
#define PICTURE_COLOR_TEMP_MAINKEY   ("color_temp")

#define PQ_COLOR_TEMP_STANDARD_NAME ("standard")
#define PQ_COLOR_TEMP_COLD_NAME     ("cold")
#define PQ_COLOR_TEMP_WARM_NAME     ("warm")
#define PQ_COLOR_TEMP_STANDARD_ID   (0)
#define PQ_COLOR_TEMP_COLD_ID       (1)
#define PQ_COLOR_TEMP_WARM_ID       (2)

#define PQ_CONTRAST_NAME            ("contrast")
#define PQ_BRIGHTNESS_NAME          ("brightness")
#define PQ_COLOR_NAME               ("color")
#define PQ_SHARPNESS_NAME           ("sharpness")
#define PQ_ZOOM_NAME                ("zoom")
#define PQ_ASPECT_RATIO_NAME        ("aspect_ratio")
#define PQ_COLOR_TEMP_NAME          ("color_temp")

#define COLOR_TEMP_STANDARD         (0)
#define COLOR_TEMP_COLD             (1)
#define COLOR_TEMP_WARM             (2)

#define ASPECT_RATIO_16_9_ID        (0)
#define ASPECT_RATIO_4_3_ID         (1)

#define PQ_CONTRAST_UI_MIN_VALUE            (0)
#define PQ_CONTRAST_UI_MAX_VALUE            (100)
#define PQ_BRIGHTNESS_UI_MIN_VALUE          (0)
#define PQ_BRIGHTNESS_UI_MAX_VALUE          (100)
#define PQ_COLOR_UI_MIN_VALUE               (0)
#define PQ_COLOR_UI_MAX_VALUE               (100)
#define PQ_SHARPNESS_UI_MIN_VALUE           (0)
#define PQ_SHARPNESS_UI_MAX_VALUE           (10)
#define PQ_COLOR_TEMP_UI_MIN_VALUE          (-50)
#define PQ_COLOR_TEMP_UI_MAX_VALUE          (150)
#define PQ_ZOOM_UI_MIN_VALUE                (80)
#define PQ_ZOOM_UI_MAX_VALUE                (100)
#define PQ_ASPECT_RATIO_UI_MIN_VALUE        (0)
#define PQ_ASPECT_RATIO_UI_MAX_VALUE        (1)

#define PQ_CONTRAST_HW_MIN_VALUE            (0)
#define PQ_CONTRAST_HW_MAX_VALUE            (100)
#define PQ_BRIGHTNESS_HW_MIN_VALUE          (0)
#define PQ_BRIGHTNESS_HW_MAX_VALUE          (100)
#define PQ_COLOR_HW_MIN_VALUE               (0)
#define PQ_COLOR_HW_MAX_VALUE               (100)
#define PQ_SHARPNESS_HW_MIN_VALUE           (0)
#define PQ_SHARPNESS_HW_MAX_VALUE           (100)
#define PQ_COLOR_TEMP_HW_MIN_VALUE          (0)
#define PQ_COLOR_TEMP_HW_MAX_VALUE          (2)
#define PQ_ZOOM_HW_MIN_VALUE                (60)
#define PQ_ZOOM_HW_MAX_VALUE                (100)
#define PQ_ASPECT_RATIO_HW_MIN_VALUE        (0)
#define PQ_ASPECT_RATIO_HW_MAX_VALUE        (1)

int init_picture_setting(void);
void set_all_pq_for_current_picture_mode(void);

int factory_set_picture_mode_id(int id);
int factory_get_picture_mode_id(void);

int factory_set_current_pq_para(char *subkey, int value, int save);
int factory_get_current_pq_para(char *subkey);
int factory_get_current_pq_para_string(char *subkey, char *value);

#endif
