/*
 * lv_pro_ebook_activity.h
 *
 *  Created on: 2024/12/10
 *      Author: hongjiasen
 */

#ifndef LV_PRO_EBOOK_ACTIVITY_H_
#define LV_PRO_EBOOK_ACTIVITY_H_

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../Common/user_common/lv_pro_user_common.h"
#include "../lv_pro_launcher.h"
#include "../lv_pro_media_layout.h"
#include "../widget/lv_pro_res.h"
#include "Common/language/string/lv_string_id.h"
#include "Common/message/lv_pro_msg.h"
#include "Layer/prompt/lv_ui_prompt.h"
#include "include/sys_msg.h"
#include "lv_common.h"
#include "lvgl/lvgl.h"
#include "page.h"

#if defined(LV_USE_RESOLUTION_1080P)
#define EBOOK_LAB_PADHOR 5
#define EBOOK_PAGELAB_PADHOR -48
#define CTRLBAR_PCT_H 32
#define CTRLBAR_BTNPCT_W 95
#define CTRLBAR_BTNPCT_H 95
#define WINBAR_PADHOR 0
#define CTRLBAR_LABPCT_H 14
#define PLAYBAR_X_OFS 0
#define PLAYBAR_Y_OFS -3
#define PLAYNAME_Y_OFS 0
#define CTRLBAR_BTN_WIDTH 150
#define CTRLBAR_BTN_HEIGHT 120
#else
#define EBOOK_LAB_PADHOR 20
#define EBOOK_PAGELAB_PADHOR -200
#define CTRLBAR_PCT_H 31
#define CTRLBAR_BTNPCT_W 74
#define CTRLBAR_BTNPCT_H 67
#define WINBAR_PADHOR 5
#define CTRLBAR_LABPCT_H 12
#define PLAYBAR_X_OFS -8
#define PLAYBAR_Y_OFS -10
#define PLAYNAME_Y_OFS 10
#define CTRLBAR_BTN_WIDTH 110
#define CTRLBAR_BTN_HEIGHT 90
#endif

extern lv_obj_t *lv_pro_ebook_activity;
extern lv_group_t *lv_pro_ebook_group;
extern lv_obj_t *ui_ebook_label_page;
extern lv_obj_t *ui_ebook_label;

int lv_pro_ebook_init(char *cur_path, char *fn);
void lv_pro_ebook_ctrlbar_timer_en(bool en);

#endif /* LV_PRO_EBOOK_ACTIVITY_H_ */