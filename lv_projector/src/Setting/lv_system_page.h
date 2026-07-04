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
#ifndef SYSTEM_PAGE_H
#define SYSTEM_PAGE_H


extern lv_obj_t *sub_system_page;
extern lv_obj_t *system_obj;
extern lv_group_t *Setting_system_group;
extern lv_group_t *mbox_item_group;

extern lv_group_t *language_item_group;
extern lv_obj_t *language_cont_obj;
extern lv_obj_t *language_label;

extern lv_obj_t *projection_cont_obj;
extern lv_obj_t *projection_label;

extern lv_group_t *homepage_bg_item_group;
extern lv_obj_t *homepage_bg_cont_obj;
extern lv_obj_t *homepage_bg_label;
extern lv_obj_t *homepage_bg_item0;
extern lv_obj_t *homepage_bg_item1;
extern lv_obj_t *homepage_bg_item2;

extern lv_obj_t *ota_cont_obj;

extern lv_obj_t *reset_setting_cont_obj;

extern lv_obj_t *auto_sleep_cont_obj;
extern lv_obj_t *auto_sleep_label;
extern lv_group_t *auto_sleep_item_group;

extern lv_obj_t *osd_timer_cont_obj;
extern lv_obj_t *osd_timer_label;
extern lv_group_t *osd_timer_item_group;

#endif
