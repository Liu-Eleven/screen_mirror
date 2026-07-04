/*
 * lv_pro_res_ebook_player_int.h
 *
 *  Created on: 2024/12/12
 *      Author: hongjiasen
 */

#ifndef LV_PRO_EBOOK_RES_H_
#define LV_PRO_EBOOK_RES_H_

#include <dirent.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../../Common/user_common/lv_pro_user_common.h"
#include "../file_explorer/lv_pro_file_explorer.h"
#include "gb_2312.h"
#include "glist.h"
#include "lv_pro_res_mid_list.h"
#include "lvgl/lvgl.h"

#define EBOOK_DEBUG_MODE 1
#ifdef EBOOK_DEBUG_MODE
#define EBOOK_DBG(fmt, ...) printf("[LVGL EBOOK DBG] [%s]: " fmt, __func__, ##__VA_ARGS__)
#else
#define EBOOK_DBG(fmt, ...)
#endif

#define EBOOK_INF(fmt, ...) printf("[LVGL EBOOK INF] [%s]: " fmt, __func__, ##__VA_ARGS__)
#define EBOOK_WRN(fmt, ...) printf("[LVGL EBOOK WRN] [%s]: " fmt, __func__, ##__VA_ARGS__)
#define EBOOK_ERR(fmt, ...) printf("[LVGL EBOOK ERR] [%s]: " fmt, __func__, ##__VA_ARGS__)

#define MAX_FILE_NAME 1024

#define EBOOK_FILE_SIZE_MAX 30 * MAX_FILE_NAME *MAX_FILE_NAME  // 30M ebook preview size
#define DISPLAY_BUFF_SIZE 512

typedef enum FP_TYPE {
    UTF8_TYPE = 1,
    UTF16_BE_TYPE,
    UTF16_LE_TYPE,
    UTF16_BE_TYPE_DIS_BOM,
    UTF16_LE_TYPE_DIS_BOM,
    TYPE_NULL,
} FP_TYPE_T;

typedef struct line_data {
    uint32_t line_number;
    uint32_t line_byte;
} line_data_t;

typedef struct {
    uint32_t e_page_num;
    uint32_t e_page_byte;
    uint32_t e_page_seek;
} ebook_page_info_t;

typedef struct {
    FILE *fp_ebook;               // file descriptor
    bool is_ebook_bgmusic;        // Whether to turn on background music for ebooks
    bool check_utf8;              // Check for UTF-8 encoding
    UINT8 cur_fp_type;            // Current ebook encoding format
    UINT16 ebook_page_line;       // How many lines does an ebook display on a page
    UINT16 ebook_line_max_bytes;  // How many bytes are displayed in an ebook line
    UINT16 save_line_num;         // Total number of lines in the file
    UINT32 lseek_num;             // The number of bytes of the current page
    UINT32 cur_page;              // Number of pages currently read
    UINT32 last_save_page;        // Number of pages read last time
    UINT32 page_all;              // Total page count
    UINT32 ebook_all_line;        // Total lines
    UINT32 read_offset;           // The offset of the current page to 0
    long file_size;               // Total file size
    long file_ebook_size;         // The number of bytes remaining
    long file_ebook_seek_size;    // The number of bytes left on the previous page
} txt_ebook_data;

struct EBookInfo {
    char filesize[10];
};

int lv_pro_ebook_res_open(char *ebookPath);
void lv_pro_ebook_res_change_ebook_txt_info(int keypad_value, int set_page);
void lv_pro_ebook_res_close();

int lv_pro_ebook_res_list_init(char *cur_path, char *fn);
int lv_pro_ebook_res_list_deinit(void);
int lv_pro_ebook_res_play_mode(int mode, int index);
int lv_pro_ebook_res_trySwitch_ebook(int mode, int index);
list_head_t *lv_pro_ebook_res_get_media_list(void);
void lv_pro_ebook_res_get_info(struct EBookInfo *pi);

#endif /*LV_PRO_EBOOK_RES_H_ */
