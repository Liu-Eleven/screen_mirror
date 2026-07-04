
#ifndef LV_PRO_MEDIA_LAYOUT_H
#define LV_PRO_MEDIA_LAYOUT_H

#include "lvgl/lvgl.h"
#include "lv_common.h"
#include "page.h"
#include "lv_string_id.h"

LV_IMG_DECLARE(player_playing);
LV_IMG_DECLARE(player_pause);
LV_IMG_DECLARE(player_backward);
LV_IMG_DECLARE(player_forward);
LV_IMG_DECLARE(player_back);
LV_IMG_DECLARE(player_next);
LV_IMG_DECLARE(player_stop);
LV_IMG_DECLARE(player_sequential);
LV_IMG_DECLARE(player_repeat);
LV_IMG_DECLARE(player_info);
LV_IMG_DECLARE(player_list);
LV_IMG_DECLARE(movie_setup);
LV_IMG_DECLARE(picture_clockwise);
LV_IMG_DECLARE(picture_anticlockwise);
LV_IMG_DECLARE(picture_zoomout);
LV_IMG_DECLARE(picture_zoomin);
LV_IMG_DECLARE(IDB_Icon_unsupported);

#if defined(LV_USE_RESOLUTION_1080P)
    #define MEDIA_BTN_WIDTH 282
    #define MEDIA_BTN_HEIGHT 510
    #define MEDIA_BTN_COL 120
    #define DISK_CELL 300
    #define CTRLBAR_PCT 28
    #define CTRLBAR_BTN_WIDTH 150
    #define CTRLBAR_BTN_HEIGHT 120
    #define CTRLBAR_BTN_COL 100
    #define STATE_ALIGN 20
    #define TIMEBAR_HEIGHT 25
    #define TIMEBAR_ALIGN -40
    #define EBOOK_LINE_SPACE 10
    #define EBOOK_LINE_MAX_BYTE 80
    #define EBOOK_PAGE_LINE 15

    #define INFO_WIDTH_PCT 26
    #define INFO_HEIGHT_PCT 40
    #define INFO_ALIGN_PCT 20
    #define LIST_WIDTH_PCT 26
    #define LIST_HEIGHT_PCT 40
    #define LIST_ALIGN_PCT 20

#else  /* default resolution 720p */
    #define MEDIA_BTN_WIDTH 186
    #define MEDIA_BTN_HEIGHT 330
    #define MEDIA_BTN_COL 78
    #define DISK_CELL 200
    #define CTRLBAR_PCT 31
    #define CTRLBAR_BTN_WIDTH 110
    #define CTRLBAR_BTN_HEIGHT 90
    #define CTRLBAR_BTN_COL 80
    #define STATE_ALIGN 10
    #define TIMEBAR_HEIGHT 21
    #define TIMEBAR_ALIGN -22
    #define EBOOK_LINE_SPACE 7
    #define EBOOK_LINE_MAX_BYTE 60
    #define EBOOK_PAGE_LINE 12

    #define INFO_WIDTH_PCT 30
    #define INFO_HEIGHT_PCT 40
    #define INFO_ALIGN_PCT 20
    #define LIST_WIDTH_PCT 30
    #define LIST_HEIGHT_PCT 50
    #define LIST_ALIGN_PCT 15

#endif

#endif  /*LV_PRO_MEDIA_LAYOUT_H*/
