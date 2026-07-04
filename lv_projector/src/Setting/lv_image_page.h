/*
*	image setting
*/
extern lv_obj_t *sub_image_page;
extern lv_obj_t *image_obj;
extern lv_group_t *Setting_image_group;
extern lv_group_t *image_item_group;
extern lv_group_t *image_slider_group;

extern lv_obj_t *image_mode_cont_obj;
extern lv_obj_t *image_mode_label;
extern lv_obj_t *image_mode_standard_item;
extern lv_obj_t *image_mode_dynamic_item;
extern lv_obj_t *image_mode_mild_item;
extern lv_obj_t *image_mode_user_item;

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
#ifndef IMAGE_PAGE_H
#define IMAGE_PAGE_H

extern lv_obj_t *contrast_cont_obj;
extern lv_obj_t *contrast_label;
extern lv_obj_t *contrast_slider_label;
extern lv_obj_t *contrast_slider_bar;

extern lv_obj_t *brightness_cont_obj;
extern lv_obj_t *brightness_label;
extern lv_obj_t *brightness_slider_label;
extern lv_obj_t *brightness_slider_bar;

extern lv_obj_t *color_cont_obj;
extern lv_obj_t *color_label;
extern lv_obj_t *color_slider_label;
extern lv_obj_t *color_slider_bar;

extern lv_obj_t *sharpness_cont_obj;
extern lv_obj_t *sharpness_label;
extern lv_obj_t *sharpness_slider_label;
extern lv_obj_t *sharpness_slider_bar;

extern lv_obj_t *color_temp_cont_obj;
extern lv_obj_t *color_temp_label;
extern lv_obj_t *color_temp_standard_item;
extern lv_obj_t *color_temp_cold_item;
extern lv_obj_t *color_temp_warm_item;
extern lv_group_t *color_temp_item_group;

extern lv_obj_t *aspect_ratio_cont_obj;
extern lv_obj_t *aspect_ratio_label;
extern lv_obj_t *aspect_ratio_16_9_item;
extern lv_obj_t *aspect_ratio_4_3_item;
extern lv_group_t *aspect_ratio_item_group;

extern lv_obj_t *zoom_cont_obj;
extern lv_obj_t *zoom_label;
extern lv_obj_t *zoom_slider_label;
extern lv_obj_t *zoom_slider_bar;

#endif