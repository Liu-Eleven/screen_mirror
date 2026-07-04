/*
 * lv_pro_ebook_res.c
 *
 *  Created on: 2024/12/12
 *      Author: hongjiasen
 */

#include "lv_pro_res_ebook_player_int.h"
#include "lv_pro_res_conv_to_utf8.h"

extern lv_obj_t *ui_ebook_label;
extern lv_obj_t *ui_ebook_label_page;
extern lv_obj_t *ebook_label;

static txt_ebook_data txt_data = {0};
static uint32_t ebook_malloc_size = 2048;
static char *read_buff_tmp = NULL;
static uint16_t *get_buff_mul = NULL;        // buff to show
static uint16_t *str_mul = NULL;             // Temporary buff
static line_data_t *ebook_line_info = NULL;  // Each line of information

static txt_ebook_data txt_data_tmp = {0};
static pthread_t read_ebook_thread_id = 0;
static int pthread_read_ebook_state = -1;
// When the thread reads the file, the total number of pages is updated periodically
static lv_timer_t *updata_page_timer = NULL;
// The content read by the thread is saved by page in the form of a linked list
static glist *ebook_list = NULL;
static list_head_t *media_list = NULL;

int lv_pro_ebook_res_open(char *ebookPath);
void lv_pro_ebook_res_change_ebook_txt_info(int keypad_value, int set_page);
void lv_pro_ebook_res_close();

static int ebook_read_file(char *ebook_file_name);
static int ebook_read_file_task(char *name);
static void ebook_updata_page_timer_cb(lv_timer_t *t);
static void *read_ebook_thread_task(void *arg);
static int ebook_read_file_pthread_stop(void);
static void win_ebook_close(void);

static void ebook_show_page_info();
static void ebook_free_buff(void);
static void ebook_free_data(void *data, void *user_data);

static void readtyped(FILE *fp_read);
static uint32_t fgetws_ex(char *string, int n, FILE *fp);

int lv_pro_ebook_res_open(char *ebookPath)
{
    EBOOK_DBG("\n");
    return ebook_read_file(ebookPath);
}

void lv_pro_ebook_res_change_ebook_txt_info(int keypad_value, int set_page)
{
    uint32_t lseek_num_tmp = 0;
    int i;
    ebook_page_info_t *ebook_page_info = NULL;
    int offset_page = 0;

    if (keypad_value == LV_KEY_DOWN) {
        if (txt_data.cur_page == txt_data.page_all) return;

        lseek_num_tmp = 0;
        txt_data.cur_page++;
        if (pthread_read_ebook_state == 1) {
            ebook_list = glist_nth((glist *)ebook_list, 1);
            ebook_page_info = ebook_list->data;
            lseek_num_tmp = ebook_page_info->e_page_byte;
            txt_data.lseek_num = lseek_num_tmp;
            fseek(txt_data.fp_ebook, ebook_page_info->e_page_seek - ebook_page_info->e_page_byte,
                  SEEK_SET);
        } else {
            if (txt_data.cur_page == txt_data.page_all) {
                lseek_num_tmp = txt_data.file_ebook_size;
                txt_data.lseek_num = lseek_num_tmp;
            } else {
                lseek_num_tmp =
                    ebook_line_info[((txt_data.cur_page) * txt_data.ebook_page_line) - 1]
                        .line_byte -
                    ebook_line_info[((txt_data.cur_page - 1) * txt_data.ebook_page_line) - 1]
                        .line_byte;
                txt_data.lseek_num = lseek_num_tmp;
            }
        }
    } else if (keypad_value == LV_KEY_UP) {
        if (txt_data.cur_page == 1) return;

        txt_data.cur_page--;
        lseek_num_tmp = 0;
        if (pthread_read_ebook_state == 1) {
            ebook_list = glist_nth_prev((glist *)ebook_list, 1);
            ebook_page_info = ebook_list->data;
            if (txt_data.cur_page == 1)
                lseek_num_tmp = ebook_page_info->e_page_byte;
            else
                lseek_num_tmp = ebook_page_info->e_page_byte;
            txt_data.lseek_num = lseek_num_tmp;
            txt_data.file_ebook_size = txt_data.file_ebook_seek_size + lseek_num_tmp;
            fseek(txt_data.fp_ebook, ebook_page_info->e_page_seek - ebook_page_info->e_page_byte,
                  SEEK_SET);
            if (txt_data.cur_page == 1) {
                if (txt_data.cur_fp_type == UTF8_TYPE || txt_data.cur_fp_type == UTF16_LE_TYPE ||
                    txt_data.cur_fp_type == UTF16_BE_TYPE)
                    fseek(txt_data.fp_ebook, 2, SEEK_SET);
            }
        } else {
            if (txt_data.cur_page == 1)
                lseek_num_tmp =
                    ebook_line_info[((txt_data.cur_page) * txt_data.ebook_page_line) - 1].line_byte;
            else
                lseek_num_tmp =
                    ebook_line_info[((txt_data.cur_page) * txt_data.ebook_page_line) - 1]
                        .line_byte -
                    ebook_line_info[((txt_data.cur_page - 1) * txt_data.ebook_page_line) - 1]
                        .line_byte;
            txt_data.lseek_num = lseek_num_tmp;
            txt_data.file_ebook_size = txt_data.file_ebook_seek_size + lseek_num_tmp;
        }
    } else {
        if (pthread_read_ebook_state == 0 || set_page <= 0 || set_page > txt_data.page_all ||
            set_page == txt_data.cur_page) {
            return;
        }

        if (txt_data.cur_page > set_page) {
            offset_page = txt_data.cur_page - set_page;
            ebook_list = glist_nth_prev((glist *)ebook_list, offset_page);
        } else if (txt_data.cur_page < set_page) {
            offset_page = set_page - txt_data.cur_page;
            ebook_list = glist_nth((glist *)ebook_list, offset_page);
        }

        txt_data.cur_page = set_page;
        // ebook_list = glist_nth((glist *)ebook_list,1);
        ebook_page_info = ebook_list->data;
        lseek_num_tmp = ebook_page_info->e_page_byte;
        txt_data.lseek_num = lseek_num_tmp;
        fseek(txt_data.fp_ebook, ebook_page_info->e_page_seek - ebook_page_info->e_page_byte,
              SEEK_SET);
    }

    if (txt_data.lseek_num * 2 > ebook_malloc_size) {
        if (str_mul != NULL) {
            free(str_mul);
            str_mul = NULL;
            ebook_malloc_size = txt_data.lseek_num * 2;
            str_mul = (uint16_t *)malloc(ebook_malloc_size * 2);
            if (str_mul == NULL) {
                EBOOK_ERR("str_mul malloc faild\n");
                return;
            }
        }
        if (get_buff_mul != NULL) {
            free(get_buff_mul);
            get_buff_mul = NULL;
            ebook_malloc_size = txt_data.lseek_num * 2;
            get_buff_mul = (uint16_t *)malloc(ebook_malloc_size);
            if (get_buff_mul == NULL) {
                EBOOK_ERR("get_buff_mul malloc faild\n");
                return;
            }
        }
    }
    memset(str_mul, 0, ebook_malloc_size * 2);
    memset(get_buff_mul, 0, ebook_malloc_size);

    if (pthread_read_ebook_state == 1) {
        fread((uint8_t *)get_buff_mul, txt_data.lseek_num, 1, txt_data.fp_ebook);
    } else {
        if (txt_data.cur_page == 1) {
            if (txt_data.cur_fp_type == UTF8_TYPE || txt_data.cur_fp_type == UTF16_LE_TYPE ||
                txt_data.cur_fp_type == UTF16_BE_TYPE) {
                // fseek(txt_data.fp_ebook,2,SEEK_SET);
                memcpy((uint8_t *)get_buff_mul,
                       read_buff_tmp +
                           (txt_data.file_size - txt_data.file_ebook_size) * sizeof(char) +
                           2 * sizeof(char),
                       txt_data.lseek_num * sizeof(char));
            } else {
                memcpy(
                    (uint8_t *)get_buff_mul,
                    read_buff_tmp + (txt_data.file_size - txt_data.file_ebook_size) * sizeof(char),
                    txt_data.lseek_num * sizeof(char));
            }
        } else {
            memcpy((uint8_t *)get_buff_mul,
                   read_buff_tmp + (txt_data.file_size - txt_data.file_ebook_size) * sizeof(char),
                   txt_data.lseek_num * sizeof(char));
        }
    }
    txt_data.file_ebook_seek_size = txt_data.file_ebook_size;
    txt_data.file_ebook_size -= txt_data.lseek_num;

    if (txt_data.cur_fp_type == UTF8_TYPE) {
        lv_label_set_text(ui_ebook_label, (char *)get_buff_mul);
    } else if (txt_data.cur_fp_type == UTF16_LE_TYPE ||
               txt_data.cur_fp_type == UTF16_LE_TYPE_DIS_BOM) {
        ComUniStrCopyChar((uint8_t *)str_mul, (uint8_t *)get_buff_mul);
        memset(get_buff_mul, 0, ebook_malloc_size);
        utf16_to_utf8_t((uint8_t *)get_buff_mul, str_mul, txt_data.lseek_num);
        lv_label_set_text(ui_ebook_label, (char *)get_buff_mul);
    } else if (txt_data.cur_fp_type == UTF16_BE_TYPE ||
               txt_data.cur_fp_type == UTF16_BE_TYPE_DIS_BOM) {
        ComUniStrToMB(get_buff_mul);
        ComUniStrCopyChar((uint8_t *)str_mul, (uint8_t *)get_buff_mul);
        memset(get_buff_mul, 0, ebook_malloc_size);
        utf16_to_utf8_t((uint8_t *)get_buff_mul, str_mul, txt_data.lseek_num);
        lv_label_set_text(ui_ebook_label, (char *)get_buff_mul);
    } else {
        if (txt_data.check_utf8) {
            lv_label_set_text(ui_ebook_label, (char *)get_buff_mul);
        } else {
            convert_gb2312_to_unicode((uint8_t *)get_buff_mul, lseek_num_tmp, str_mul,
                                      ebook_malloc_size);
            ComUniStrToMB((uint16_t *)str_mul);
            memset(get_buff_mul, 0, ebook_malloc_size);
            ComUniStrCopyChar((uint8_t *)get_buff_mul, (uint8_t *)str_mul);
            memset(str_mul, 0, ebook_malloc_size * 2);
            utf16_to_utf8_t((uint8_t *)str_mul, get_buff_mul, lseek_num_tmp);
            lv_label_set_text(ui_ebook_label, (char *)str_mul);
        }
    }

    ebook_show_page_info();
}

void lv_pro_ebook_res_close()
{
    EBOOK_DBG("\n");
    win_ebook_close();
}

static int ebook_read_file(char *ebook_file_name)
{
    struct stat e_sa;
    char read_line[MAX_FILE_NAME] = {0};
    uint32_t read_tmp = 0;
    uint32_t read_size_tmp = 0;
    int i = 0;
    uint32_t page_size = 0;
    uint16_t line_size = 0;

    /*1、free memory */
    win_ebook_close();

    uint8_t offset_tmp = 0;
    memset(read_line, 0, MAX_FILE_NAME);
    if (ebook_file_name) {
        strncpy(read_line, ebook_file_name, MAX_FILE_NAME);
    }

    /*2、open file */
    txt_data.fp_ebook = fopen(read_line, "r");
    if (txt_data.fp_ebook == NULL) {
        EBOOK_ERR("open %s file failed\n", read_line);
        return -1;
    }
    if (stat(read_line, &e_sa) == -1) {
        EBOOK_ERR("stat failed\n");
        return -1;
    }
    if (e_sa.st_size > EBOOK_FILE_SIZE_MAX) {
        EBOOK_ERR("The file exceeds the maximum size \n");
        return -1;
    }

    EBOOK_DBG("open %s file successed!\n", read_line);

    if (get_buff_mul == NULL) get_buff_mul = (uint16_t *)malloc(ebook_malloc_size);
    if (str_mul == NULL) str_mul = (uint16_t *)malloc(ebook_malloc_size * 2);
    memset(get_buff_mul, 0, ebook_malloc_size);
    memset(str_mul, 0, ebook_malloc_size * 2);

    txt_data.cur_page = 1;
    txt_data.lseek_num = 0;
    txt_data.file_ebook_seek_size = 0;
    txt_data.file_ebook_size = 0;
    fseek(txt_data.fp_ebook, 0, SEEK_SET);
    readtyped(txt_data.fp_ebook);
    txt_data.file_ebook_size = (long)e_sa.st_size;
    txt_data.file_size = txt_data.file_ebook_size;
    txt_data.file_ebook_seek_size = txt_data.file_ebook_size;

    if (txt_data.cur_fp_type == UTF8_TYPE || txt_data.cur_fp_type == UTF16_LE_TYPE ||
        txt_data.cur_fp_type == UTF16_BE_TYPE)
        fseek(txt_data.fp_ebook, 2, SEEK_SET);
    else
        fseek(txt_data.fp_ebook, 0, SEEK_SET);

    /*3、Get the number of text lines(read_tmp), Preload up to 512K */
    if (txt_data.cur_fp_type == UTF16_LE_TYPE || txt_data.cur_fp_type == UTF16_BE_TYPE ||
        txt_data.cur_fp_type == UTF16_LE_TYPE_DIS_BOM ||
        txt_data.cur_fp_type == UTF16_BE_TYPE_DIS_BOM) {
        while ((line_size = fgetws_ex(read_line, MAX_FILE_NAME, txt_data.fp_ebook)) > 0) {
            read_size_tmp += line_size;
            if (read_size_tmp > DISPLAY_BUFF_SIZE * MAX_FILE_NAME) break;
            if (line_size > txt_data.ebook_line_max_bytes) {
                read_tmp += line_size / txt_data.ebook_line_max_bytes;
                if (line_size % txt_data.ebook_line_max_bytes) {
                    read_tmp++;
                }
            } else {
                read_tmp++;
            }
            memset(read_line, 0, MAX_FILE_NAME);
        }
    } else {
        while (fgets(read_line, MAX_FILE_NAME, txt_data.fp_ebook)) {
            line_size = strlen(read_line);
            read_size_tmp += line_size;
            if (read_size_tmp > DISPLAY_BUFF_SIZE * MAX_FILE_NAME) break;
            if (line_size > txt_data.ebook_line_max_bytes) {
                read_tmp += line_size / txt_data.ebook_line_max_bytes;
                if (line_size % txt_data.ebook_line_max_bytes) {
                    read_tmp++;
                }
            } else {
                read_tmp++;
            }
            memset(read_line, 0, sizeof(read_line));
        }
    }

    if (read_tmp == 0) {
        lv_label_set_text(ui_ebook_label, "");
        txt_data.file_ebook_seek_size = 0;
        txt_data.file_ebook_seek_size = 0;
        txt_data.read_offset = 0;
        txt_data.lseek_num = 0;
        txt_data.save_line_num = 0;
        txt_data.ebook_all_line = 0;
        txt_data.cur_page = 1;
        txt_data.page_all = 1;
        goto empty_file;
    }

    /*4、Record each line id and line number */
    ebook_line_info = malloc((read_tmp + 1) * sizeof(line_data_t));
    memset(read_line, 0, sizeof(read_line));
    read_tmp = 0;
    read_size_tmp = 0;

    if (txt_data.cur_fp_type == UTF8_TYPE || txt_data.cur_fp_type == UTF16_LE_TYPE ||
        txt_data.cur_fp_type == UTF16_BE_TYPE)
        fseek(txt_data.fp_ebook, 2, SEEK_SET);
    else
        fseek(txt_data.fp_ebook, 0, SEEK_SET);

    if (txt_data.cur_fp_type == UTF16_LE_TYPE || txt_data.cur_fp_type == UTF16_BE_TYPE ||
        txt_data.cur_fp_type == UTF16_LE_TYPE_DIS_BOM ||
        txt_data.cur_fp_type == UTF16_BE_TYPE_DIS_BOM) {
        while ((line_size = fgetws_ex(read_line, MAX_FILE_NAME, txt_data.fp_ebook)) > 0) {
            read_size_tmp += line_size;
            if (read_size_tmp > DISPLAY_BUFF_SIZE * MAX_FILE_NAME) {
                read_size_tmp -= line_size;
                break;
            }
            if (line_size > txt_data.ebook_line_max_bytes) {
                for (i = 0; i < line_size / txt_data.ebook_line_max_bytes; i++) {
                    ebook_line_info[read_tmp + i].line_number = read_tmp + i;
                    txt_data.lseek_num += txt_data.ebook_line_max_bytes;
                    ebook_line_info[read_tmp + i].line_byte = txt_data.lseek_num;
                }
                read_tmp += line_size / txt_data.ebook_line_max_bytes;
                if (line_size % txt_data.ebook_line_max_bytes) {
                    ebook_line_info[read_tmp].line_number = read_tmp;
                    txt_data.lseek_num += line_size % txt_data.ebook_line_max_bytes;
                    ebook_line_info[read_tmp].line_byte = txt_data.lseek_num;
                    read_tmp++;
                }
            } else {
                ebook_line_info[read_tmp].line_number = read_tmp;
                txt_data.lseek_num += line_size;
                ebook_line_info[read_tmp].line_byte = txt_data.lseek_num;
                read_tmp++;
            }
            if (txt_data.read_offset == (DISPLAY_BUFF_SIZE)*MAX_FILE_NAME &&
                read_size_tmp >= (txt_data.read_offset / 2) && offset_tmp == 0) {
                offset_tmp = 1;
                txt_data.save_line_num = read_tmp;
            } else if (txt_data.read_offset <= (DISPLAY_BUFF_SIZE)*MAX_FILE_NAME &&
                       read_size_tmp >= (txt_data.read_offset) && offset_tmp == 0) {
                offset_tmp = 1;
                txt_data.save_line_num = read_tmp;
            }
            memset(read_line, 0, MAX_FILE_NAME);
        }
    } else {
        while (fgets(read_line, MAX_FILE_NAME, txt_data.fp_ebook)) {
            line_size = strlen(read_line);
            read_size_tmp += line_size;
            if (read_size_tmp > DISPLAY_BUFF_SIZE * MAX_FILE_NAME) {
                read_size_tmp -= line_size;
                break;
            }
            if (line_size > txt_data.ebook_line_max_bytes) {
                for (i = 0; i < line_size / txt_data.ebook_line_max_bytes; i++) {
                    ebook_line_info[read_tmp + i].line_number = read_tmp + i;
                    txt_data.lseek_num += txt_data.ebook_line_max_bytes;
                    ebook_line_info[read_tmp + i].line_byte = txt_data.lseek_num;
                }
                read_tmp += line_size / txt_data.ebook_line_max_bytes;
                if (line_size % txt_data.ebook_line_max_bytes) {
                    ebook_line_info[read_tmp].line_number = read_tmp;
                    txt_data.lseek_num += line_size % txt_data.ebook_line_max_bytes;
                    ebook_line_info[read_tmp].line_byte = txt_data.lseek_num;
                    read_tmp++;
                }
            } else {
                ebook_line_info[read_tmp].line_number = read_tmp;
                txt_data.lseek_num += line_size;
                ebook_line_info[read_tmp].line_byte = txt_data.lseek_num;
                read_tmp++;
            }
            if (txt_data.read_offset == (DISPLAY_BUFF_SIZE)*MAX_FILE_NAME &&
                read_size_tmp >= (txt_data.read_offset / 2) && offset_tmp == 0) {
                offset_tmp = 1;
                txt_data.save_line_num = read_tmp;
            } else if (txt_data.read_offset <= (DISPLAY_BUFF_SIZE)*MAX_FILE_NAME &&
                       read_size_tmp >= (txt_data.read_offset) && offset_tmp == 0) {
                offset_tmp = 1;
                txt_data.save_line_num = read_tmp;
            }
            memset(read_line, 0, sizeof(read_line));
        }
    }

    /*5、The total number of characters in the expected loading page of the application */
    if (txt_data.cur_fp_type == UTF8_TYPE || txt_data.cur_fp_type == UTF16_LE_TYPE ||
        txt_data.cur_fp_type == UTF16_BE_TYPE)
        fseek(txt_data.fp_ebook, 2, SEEK_SET);
    else
        fseek(txt_data.fp_ebook, 0, SEEK_SET);

    read_buff_tmp = malloc(read_size_tmp);
    fread(read_buff_tmp, 1, read_size_tmp, txt_data.fp_ebook);
    txt_data.ebook_all_line = read_tmp;
    txt_data.cur_page = txt_data.save_line_num / txt_data.ebook_page_line + 1;
    txt_data.page_all = read_tmp / txt_data.ebook_page_line;
    if (read_tmp % txt_data.ebook_page_line) txt_data.page_all++;
    if (txt_data.page_all == 0) txt_data.page_all = 1;

    /*6、Recalculating the current page requires memory */
    if (txt_data.cur_fp_type == UTF8_TYPE || txt_data.cur_fp_type == UTF16_LE_TYPE ||
        txt_data.cur_fp_type == UTF16_BE_TYPE)
        fseek(txt_data.fp_ebook, 2, SEEK_SET);
    else
        fseek(txt_data.fp_ebook, 0, SEEK_SET);

    if (txt_data.cur_page == 1) {
        if (read_tmp < txt_data.ebook_page_line && read_tmp > 0)
            page_size = ebook_line_info[read_tmp - 1].line_byte;
        else
            page_size =
                ebook_line_info[((txt_data.cur_page) * txt_data.ebook_page_line) - 1].line_byte;
    } else {
        if (txt_data.cur_page == txt_data.page_all)
            page_size =
                ebook_line_info[read_tmp - 1].line_byte -
                ebook_line_info[((txt_data.cur_page - 1) * txt_data.ebook_page_line) - 1].line_byte;
        else
            page_size =
                ebook_line_info[((txt_data.cur_page) * txt_data.ebook_page_line) - 1].line_byte -
                ebook_line_info[((txt_data.cur_page - 1) * txt_data.ebook_page_line) - 1].line_byte;
    }

    if (page_size > ebook_malloc_size) {
        ebook_malloc_size = page_size;
        if (str_mul != NULL) {
            void *tmp_str_mul = (uint16_t *)realloc(str_mul, ebook_malloc_size * 2);
            if (tmp_str_mul == NULL) {
                free(str_mul);
                EBOOK_ERR("str_mul realloc faild\n");
                return -1;
            }
            str_mul = tmp_str_mul;
        }
        if (get_buff_mul != NULL) {
            void *tmp_get_buff_mul = (uint16_t *)realloc(get_buff_mul, ebook_malloc_size);
            if (tmp_get_buff_mul == NULL) {
                free(get_buff_mul);
                EBOOK_ERR("get_buff_mul realloc faild\n");
                return -1;
            }
            get_buff_mul = tmp_get_buff_mul;
        }
    }

    /*7、Copies the specified content to the cache */
    if (txt_data.cur_page == 1) {
        if (read_tmp < txt_data.ebook_page_line && read_tmp > 0) {
            memcpy((uint8_t *)get_buff_mul, read_buff_tmp, ebook_line_info[read_tmp - 1].line_byte);
            page_size = ebook_line_info[read_tmp - 1].line_byte;
            txt_data.file_ebook_size -= ebook_line_info[read_tmp - 1].line_byte;
        } else {
            memcpy((uint8_t *)get_buff_mul, read_buff_tmp,
                   ebook_line_info[((txt_data.cur_page) * txt_data.ebook_page_line) - 1].line_byte);
            page_size =
                ebook_line_info[((txt_data.cur_page) * txt_data.ebook_page_line) - 1].line_byte;
            txt_data.file_ebook_size -=
                ebook_line_info[((txt_data.cur_page) * txt_data.ebook_page_line) - 1].line_byte;
        }
    } else {
        if (txt_data.cur_page == txt_data.page_all) {
            memcpy((uint8_t *)get_buff_mul,
                   read_buff_tmp +
                       ebook_line_info[((txt_data.cur_page - 1) * txt_data.ebook_page_line) - 1]
                           .line_byte,
                   ebook_line_info[read_tmp - 1].line_byte -
                       ebook_line_info[((txt_data.cur_page - 1) * txt_data.ebook_page_line) - 1]
                           .line_byte);
            page_size =
                ebook_line_info[read_tmp - 1].line_byte -
                ebook_line_info[((txt_data.cur_page - 1) * txt_data.ebook_page_line) - 1].line_byte;
            txt_data.file_ebook_size -= ebook_line_info[read_tmp - 1].line_byte;
        } else {
            memcpy((uint8_t *)get_buff_mul,
                   read_buff_tmp +
                       ebook_line_info[((txt_data.cur_page - 1) * txt_data.ebook_page_line) - 1]
                           .line_byte,
                   ebook_line_info[((txt_data.cur_page) * txt_data.ebook_page_line) - 1].line_byte -
                       ebook_line_info[((txt_data.cur_page - 1) * txt_data.ebook_page_line) - 1]
                           .line_byte);
            page_size =
                ebook_line_info[((txt_data.cur_page) * txt_data.ebook_page_line) - 1].line_byte -
                ebook_line_info[((txt_data.cur_page - 1) * txt_data.ebook_page_line) - 1].line_byte;
            txt_data.file_ebook_size -=
                ebook_line_info[((txt_data.cur_page) * txt_data.ebook_page_line) - 1].line_byte;
        }
    }

    /*8、Display format processing */
    if (txt_data.cur_fp_type == UTF8_TYPE) {
        lv_label_set_text(ui_ebook_label, (char *)get_buff_mul);
    } else if (txt_data.cur_fp_type == UTF16_LE_TYPE ||
               txt_data.cur_fp_type == UTF16_LE_TYPE_DIS_BOM) {
        ComUniStrCopyChar((uint8_t *)str_mul, (uint8_t *)get_buff_mul);
        memset(get_buff_mul, 0, ebook_malloc_size);
        utf16_to_utf8_t((uint8_t *)get_buff_mul, str_mul, page_size);
        lv_label_set_text(ui_ebook_label, (char *)get_buff_mul);
    } else if (txt_data.cur_fp_type == UTF16_BE_TYPE ||
               txt_data.cur_fp_type == UTF16_BE_TYPE_DIS_BOM) {
        ComUniStrToMB((uint16_t *)get_buff_mul);
        ComUniStrCopyChar((uint8_t *)str_mul, (uint8_t *)get_buff_mul);
        memset(get_buff_mul, 0, ebook_malloc_size);
        utf16_to_utf8_t((uint8_t *)get_buff_mul, str_mul, page_size);
        lv_label_set_text(ui_ebook_label, (char *)get_buff_mul);
    } else {
        if (IsUTF8(get_buff_mul, page_size)) {
            txt_data.check_utf8 = true;
            lv_label_set_text(ui_ebook_label, (char *)get_buff_mul);
        } else {
            convert_gb2312_to_unicode((uint8_t *)get_buff_mul, page_size, str_mul,
                                      ebook_malloc_size);
            ComUniStrToMB((uint16_t *)str_mul);
            memset(get_buff_mul, 0, ebook_malloc_size);
            ComUniStrCopyChar((uint8_t *)get_buff_mul, (uint8_t *)str_mul);
            memset(str_mul, 0, ebook_malloc_size * 2);
            utf16_to_utf8_t((uint8_t *)str_mul, get_buff_mul, page_size);
            lv_label_set_text(ui_ebook_label, (char *)str_mul);
        }
    }

    read_tmp = 0;
    txt_data.file_ebook_seek_size = txt_data.file_ebook_size;
    txt_data.file_ebook_seek_size += page_size;

empty_file:
    ebook_show_page_info();
    ebook_read_file_task(ebook_file_name);
    return 0;
}

static int ebook_read_file_task(char *name)
{
    int ret = -1;
    if (pthread_read_ebook_state == 0) {
        EBOOK_ERR("ebook_read_file_task restart!\n");
        return 0;
    }

    // create the message task
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x5000);
    // pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); //release task resource itself

    if (pthread_create(&read_ebook_thread_id, &attr, read_ebook_thread_task, (void *)name)) {
        return -1;
    }
    pthread_attr_destroy(&attr);

    updata_page_timer = lv_timer_create(ebook_updata_page_timer_cb, 1000, NULL);

    return ret;
}

static void ebook_updata_page_timer_cb(lv_timer_t *t)
{
    if (pthread_read_ebook_state == 0) {
        if (txt_data_tmp.page_all > txt_data.page_all) txt_data.page_all = txt_data_tmp.page_all;
    } else if (pthread_read_ebook_state == 1) {
        lv_timer_pause(updata_page_timer);
    }
    ebook_show_page_info();
}

static void *read_ebook_thread_task(void *arg)
{
    char read_line[MAX_FILE_NAME] = {0};
    ebook_page_info_t *ebook_page_info = NULL;
    uint32_t read_tmp = 0;
    uint16_t line_size = 0;
    uint32_t last_save_byte_size = 0;
    uint32_t read_size_tmp = 0;
    uint32_t last_prev_offset_size = 0;
    uint32_t last_next_offset_size = 0;
    uint8_t read_direct = 1;  // 0:prev 1:down
    uint32_t offset_block = 0;
    int i;
    uint32_t prev_page_tmp = 0;
    line_data_t ebook_line_info_tmp[txt_data.ebook_page_line];
    memset(ebook_line_info_tmp, 0, sizeof(line_data_t) * txt_data.ebook_page_line);
    memset(&txt_data_tmp, 0, sizeof(txt_ebook_data));
    memset(read_line, 0, MAX_FILE_NAME);
    strcpy(read_line, arg);
    pthread_read_ebook_state = 0;

    if (txt_data.cur_fp_type == UTF16_LE_TYPE || txt_data.cur_fp_type == UTF16_BE_TYPE ||
        txt_data.cur_fp_type == UTF16_LE_TYPE_DIS_BOM ||
        txt_data.cur_fp_type == UTF16_BE_TYPE_DIS_BOM) {
        while ((line_size = fgetws_ex(read_line, MAX_FILE_NAME, txt_data.fp_ebook)) > 0) {
            if (pthread_read_ebook_state == -1) break;
            read_size_tmp += line_size;
            if (line_size > txt_data.ebook_line_max_bytes) {
                for (i = 0; i < line_size / txt_data.ebook_line_max_bytes; i++) {
                    ebook_line_info_tmp[read_tmp].line_number = read_tmp;
                    txt_data_tmp.lseek_num += txt_data.ebook_line_max_bytes;
                    ebook_line_info_tmp[read_tmp].line_byte = txt_data_tmp.lseek_num;
                    if ((read_tmp % (txt_data.ebook_page_line - 1) == 0) && read_tmp != 0) {
                        ebook_page_info = malloc(sizeof(ebook_page_info_t));
                        ebook_page_info->e_page_byte = txt_data_tmp.lseek_num - last_save_byte_size;
                        ebook_page_info->e_page_seek = txt_data_tmp.lseek_num;
                        last_save_byte_size = ebook_page_info->e_page_seek;
                        read_tmp = 0;
                        ebook_page_info->e_page_num = txt_data_tmp.page_all++;
                        ebook_list = glist_append(ebook_list, (void *)ebook_page_info);
                    } else
                        read_tmp++;
                }
                if (line_size % txt_data.ebook_line_max_bytes) {
                    ebook_line_info_tmp[read_tmp].line_number = read_tmp;
                    txt_data_tmp.lseek_num += line_size % txt_data.ebook_line_max_bytes;
                    ebook_line_info_tmp[read_tmp].line_byte = txt_data_tmp.lseek_num;
                    if ((read_tmp) % (txt_data.ebook_page_line - 1) == 0 && read_tmp != 0) {
                        ebook_page_info = malloc(sizeof(ebook_page_info_t));
                        ebook_page_info->e_page_byte = txt_data_tmp.lseek_num - last_save_byte_size;
                        ebook_page_info->e_page_seek = txt_data_tmp.lseek_num;
                        last_save_byte_size = ebook_page_info->e_page_seek;
                        read_tmp = 0;
                        ebook_page_info->e_page_num = txt_data_tmp.page_all++;
                        ebook_list = glist_append(ebook_list, (void *)ebook_page_info);
                    } else
                        read_tmp++;
                }
            } else {
                ebook_line_info_tmp[read_tmp].line_number = read_tmp;
                txt_data_tmp.lseek_num += line_size;
                ebook_line_info_tmp[read_tmp].line_byte = txt_data_tmp.lseek_num;
                if ((read_tmp) % (txt_data.ebook_page_line - 1) == 0 && read_tmp != 0) {
                    ebook_page_info = malloc(sizeof(ebook_page_info_t));
                    ebook_page_info->e_page_byte = txt_data_tmp.lseek_num - last_save_byte_size;
                    ebook_page_info->e_page_seek = txt_data_tmp.lseek_num;
                    last_save_byte_size = ebook_page_info->e_page_seek;
                    read_tmp = 0;
                    ebook_page_info->e_page_num = txt_data_tmp.page_all++;
                    ebook_list = glist_append(ebook_list, (void *)ebook_page_info);
                } else
                    read_tmp++;
            }
        }
        line_size = strlen(read_line);
        if (read_tmp > 0 && read_tmp < txt_data.ebook_page_line) {
            ebook_page_info = malloc(sizeof(ebook_page_info_t));
            ebook_page_info->e_page_byte = txt_data_tmp.lseek_num - last_save_byte_size;
            ebook_page_info->e_page_seek = txt_data_tmp.lseek_num;
            last_save_byte_size = ebook_page_info->e_page_seek;
            read_tmp = 0;
            ebook_page_info->e_page_num = txt_data_tmp.page_all++;
            ebook_list = glist_append(ebook_list, (void *)ebook_page_info);
        }
    } else {
        while (fgets(read_line, MAX_FILE_NAME, txt_data.fp_ebook)) {
            if (pthread_read_ebook_state == -1) break;
            line_size = strlen(read_line);
            read_size_tmp += line_size;
            if (line_size > txt_data.ebook_line_max_bytes) {
                for (i = 0; i < line_size / txt_data.ebook_line_max_bytes; i++) {
                    ebook_line_info_tmp[read_tmp].line_number = read_tmp;
                    txt_data_tmp.lseek_num += txt_data.ebook_line_max_bytes;
                    ebook_line_info_tmp[read_tmp].line_byte = txt_data_tmp.lseek_num;
                    if ((read_tmp % (txt_data.ebook_page_line - 1) == 0) && read_tmp != 0) {
                        ebook_page_info = malloc(sizeof(ebook_page_info_t));
                        ebook_page_info->e_page_byte = txt_data_tmp.lseek_num - last_save_byte_size;
                        ebook_page_info->e_page_seek = txt_data_tmp.lseek_num;
                        last_save_byte_size = ebook_page_info->e_page_seek;
                        read_tmp = 0;
                        ebook_page_info->e_page_num = txt_data_tmp.page_all++;
                        ebook_list = glist_append(ebook_list, (void *)ebook_page_info);
                    } else
                        read_tmp++;
                }
                if (line_size % txt_data.ebook_line_max_bytes) {
                    ebook_line_info_tmp[read_tmp].line_number = read_tmp;
                    txt_data_tmp.lseek_num += line_size % txt_data.ebook_line_max_bytes;
                    ebook_line_info_tmp[read_tmp].line_byte = txt_data_tmp.lseek_num;
                    if ((read_tmp) % (txt_data.ebook_page_line - 1) == 0 && read_tmp != 0) {
                        ebook_page_info = malloc(sizeof(ebook_page_info_t));
                        ebook_page_info->e_page_byte = txt_data_tmp.lseek_num - last_save_byte_size;
                        ebook_page_info->e_page_seek = txt_data_tmp.lseek_num;
                        last_save_byte_size = ebook_page_info->e_page_seek;
                        read_tmp = 0;
                        ebook_page_info->e_page_num = txt_data_tmp.page_all++;
                        ebook_list = glist_append(ebook_list, (void *)ebook_page_info);
                    } else
                        read_tmp++;
                }
            } else {
                ebook_line_info_tmp[read_tmp].line_number = read_tmp;
                txt_data_tmp.lseek_num += line_size;
                ebook_line_info_tmp[read_tmp].line_byte = txt_data_tmp.lseek_num;
                if ((read_tmp) % (txt_data.ebook_page_line - 1) == 0 && read_tmp != 0) {
                    ebook_page_info = malloc(sizeof(ebook_page_info_t));
                    ebook_page_info->e_page_byte = txt_data_tmp.lseek_num - last_save_byte_size;
                    ebook_page_info->e_page_seek = txt_data_tmp.lseek_num;
                    last_save_byte_size = ebook_page_info->e_page_seek;
                    read_tmp = 0;
                    ebook_page_info->e_page_num = txt_data_tmp.page_all++;
                    ebook_list = glist_append(ebook_list, (void *)ebook_page_info);
                } else
                    read_tmp++;
            }
        }
        line_size = strlen(read_line);
        if (read_tmp > 0 && read_tmp < txt_data.ebook_page_line) {
            ebook_page_info = malloc(sizeof(ebook_page_info_t));
            ebook_page_info->e_page_byte = txt_data_tmp.lseek_num - last_save_byte_size;
            ebook_page_info->e_page_seek = txt_data_tmp.lseek_num;
            last_save_byte_size = ebook_page_info->e_page_seek;
            read_tmp = 0;
            ebook_page_info->e_page_num = txt_data_tmp.page_all++;
            ebook_list = glist_append(ebook_list, (void *)ebook_page_info);
        }
    }

    if (pthread_read_ebook_state == -1) return NULL;

    pthread_read_ebook_state = 1;
    if (txt_data.read_offset > ((DISPLAY_BUFF_SIZE / 2) * MAX_FILE_NAME))
        txt_data.cur_page = txt_data.cur_page + (txt_data.last_save_page -
                                                 txt_data.save_line_num / txt_data.ebook_page_line);
    else
        txt_data.cur_page = txt_data.cur_page;

    txt_data.page_all = txt_data_tmp.page_all;
    if (txt_data.page_all == 0) txt_data.page_all = 1;

    ebook_show_page_info();
    ebook_list = glist_nth(ebook_list, txt_data.cur_page - 1);
    txt_data_tmp.page_all -= 1;

    if (read_buff_tmp) {
        free(read_buff_tmp);
        read_buff_tmp = NULL;
    }
    return NULL;
}

static int ebook_read_file_pthread_stop(void)
{
    if (read_ebook_thread_id) {
        pthread_read_ebook_state = -1;
        pthread_join(read_ebook_thread_id, NULL);
    }
    read_ebook_thread_id = 0;
    pthread_read_ebook_state = -1;

    return 0;
}

static void win_ebook_close(void)
{
    if (NULL == txt_data.fp_ebook) return;

    if (read_buff_tmp) {
        free(read_buff_tmp);
        read_buff_tmp = NULL;
    }

    ebook_read_file_pthread_stop();
    usleep(20 * 1000);
    ebook_free_buff();
}

static void ebook_show_page_info()
{
    char page_info[64] = {0};
    uint32_t cur_page_tmp = 0;

    if (pthread_read_ebook_state == 1) {
        sprintf(page_info, "%ld / %ld", txt_data.cur_page, txt_data.page_all);
    } else {
        if (txt_data.read_offset >= ((DISPLAY_BUFF_SIZE / 2) * MAX_FILE_NAME)) {
            cur_page_tmp = txt_data.cur_page + (txt_data.last_save_page -
                                                txt_data.save_line_num / txt_data.ebook_page_line);
            sprintf(page_info, "%ld / %ld",
                    txt_data.cur_page + (txt_data.last_save_page -
                                         txt_data.save_line_num / txt_data.ebook_page_line),
                    txt_data.page_all + (txt_data.last_save_page -
                                         txt_data.save_line_num / txt_data.ebook_page_line));
        } else {
            cur_page_tmp = txt_data.cur_page;
            sprintf(page_info, "%ld / %ld", txt_data.cur_page, txt_data.page_all);
        }
    }
    lv_label_set_text(ui_ebook_label_page, page_info);
}

static void ebook_free_buff(void)
{
    // txt_data.read_page_num = cur_page;
    if (updata_page_timer) {
        lv_timer_pause(updata_page_timer);
        lv_timer_del(updata_page_timer);
        updata_page_timer = NULL;
    }
    if (get_buff_mul != NULL) {
        free(get_buff_mul);
        get_buff_mul = NULL;
    }
    if (str_mul != NULL) {
        free(str_mul);
        str_mul = NULL;
    }
    if (ebook_line_info != NULL) {
        free(ebook_line_info);
        ebook_line_info = NULL;
    }
    fclose(txt_data.fp_ebook);
    txt_data.fp_ebook = NULL;
    memset(&txt_data, 0, sizeof(txt_ebook_data));
    memset(&txt_data_tmp, 0, sizeof(txt_ebook_data));
    glist_free_full(ebook_list, ebook_free_data);
    ebook_list = NULL;

    txt_data.page_all = 1;
}

static void ebook_free_data(void *data, void *user_data)
{
    (void)user_data;
    if (data) free(data);
}

static void readtyped(FILE *fp_read)
{
    unsigned char buff[MAX_FILE_NAME];
    int ret = 0;
    if (fp_read == NULL) {
        return;
    }
    memset(buff, 0, MAX_FILE_NAME);
    // fgets(buff,MAX_FILE_NAME,fp_read);
    int i = 0;
    while (i < MAX_FILE_NAME && !feof(fp_read)) {
        fread(&buff[i], sizeof(char), 1, fp_read);
        if (i == 0) {
            i++;
            continue;
        }
        if ((buff[i - 1] == 0x0a && buff[i] == 0x0) || (buff[i - 1] == '\0' && buff[i] == '\n') ||
            (buff[i - 1] == 0x0d && buff[i] == 0x0a) || (buff[i - 1] == 0x00 && buff[i] == 0x0a)) {
            break;
        }
        i++;
    }
    if (i >= MAX_FILE_NAME) i = MAX_FILE_NAME - 1;
    if ((i % 2) == 0 && !(buff[i - 1] == 0x0d && buff[i] == 0x0a)) {
        i++;
        if (i >= MAX_FILE_NAME) i = MAX_FILE_NAME - 1;
    }
    txt_data.ebook_page_line = 15;
    txt_data.ebook_line_max_bytes = 80;
    if (buff[0] == 0xef && buff[1] == 0xbb)
        txt_data.cur_fp_type = UTF8_TYPE;
    else if (buff[0] == 0xff && buff[1] == 0xfe) {
        txt_data.cur_fp_type = UTF16_LE_TYPE;
    } else if (buff[0] == 0xfe && buff[1] == 0xff) {
        txt_data.cur_fp_type = UTF16_BE_TYPE;
    } else if ((buff[i - 1] != 0x00 && buff[i] == 0x00)) {
        txt_data.cur_fp_type = UTF16_LE_TYPE_DIS_BOM;
    } else if ((buff[i - 1] == 0x00 && buff[i] != 0x00)) {
        txt_data.cur_fp_type = UTF16_BE_TYPE_DIS_BOM;
    } else
        txt_data.cur_fp_type = TYPE_NULL;
}

static uint32_t fgetws_ex(char *string, int n, FILE *fp)
{
    if (!string || !fp || feof(fp) || n <= 0) {
        return 0;
    }
    int i = 0;
    while (i < n - 1 && !feof(fp)) {
        fread(&string[i], sizeof(uint16_t), 1, fp);
        if ((string[i - 2] == 0x0d && string[i - 1] == 0x0 && string[i] == 0x0a &&
             string[i + 1] == 0x0) ||
            (string[i] == '\0' && string[i + 1] == '\n')) {
            i += 2;
            break;
        }
        i += 2;
    }
    string[i] = 0x0;
    return i;  // string;
}

static int lv_pro_ebook_file_switch()
{
    lv_pro_ebook_res_close();

    list_head_t *my_media_list = lv_pro_ebook_res_get_media_list();
    if (!my_media_list) {
        EBOOK_ERR("get my_media_list fail\n");
        return -1;
    }
    char *ebookPath = my_media_list->current_node->path;

    return lv_pro_ebook_res_open(ebookPath);
}

int lv_pro_ebook_res_list_init(char *cur_path, char *fn)
{
    DIR *dp;
    struct dirent *dirp;
    char compete_path[FILE_PATH_MAXT_LEN];
    char compete_name[FILE_NAME_MAXT_LEN];
    int index = 0;
    int cur_index = 1;

    media_list = create_list();

    dp = opendir(cur_path);
    if (!dp) {
        EBOOK_ERR("open directory %s error\n", cur_path);
        return -1;
    }

    /*Get bmp file name */
    while ((dirp = readdir(dp))) {
        if (lv_pro_check_file_type(dirp->d_name, FileType_TXT) == true) {
            EBOOK_DBG("ebook filename: %s/%s\n", cur_path, dirp->d_name);
            memset(compete_path, 0, sizeof(compete_path));
            memset(compete_name, 0, sizeof(compete_name));
            snprintf(compete_path, FILE_PATH_MAXT_LEN, "%s/%s", cur_path, dirp->d_name);
            snprintf(compete_name, FILE_NAME_MAXT_LEN, "%s", dirp->d_name);
            list_append(media_list, compete_path, compete_name, MEDIA_F_TYPE_TXT);
            ++index;
            if (!strcmp(dirp->d_name, fn)) {
                cur_index = index;
            }
        }
    }

    closedir(dp);

    /* Initialize the default txt, the ebook play activity can be played immediately */
    list_skip_to_index(media_list, cur_index);

    return 0;
}

int lv_pro_ebook_res_list_deinit(void)
{
    destroy_list(media_list);
    media_list = NULL;
    return 0;
}

int lv_pro_ebook_res_play_mode(int mode, int index)
{
    if (mode == 1) {
        list_get_next_node(media_list);
    } else if (mode == 0) {
        list_get_pre_node(media_list);
    } else if (mode == 2) {
        list_skip_to_index(media_list, index);
    }
    return lv_pro_ebook_file_switch();
}

int lv_pro_ebook_res_trySwitch_ebook(int mode, int index)
{
    int result, try = 0;

    list_head_t *cur_list = lv_pro_ebook_res_get_media_list();
    int cur_index = cur_list->cur_index;

    if (mode == 0 || mode == 1) {
        do {
            result = lv_pro_ebook_res_play_mode(mode, index);
            if (result == -1) {
                EBOOK_WRN("File opening failed, continue trying next\n");
                try++;
            }

            if (try > 10) {
                EBOOK_ERR("Exceeded the maximum number of attempts\n");
                break;
            }
        } while (result != 0);
    } else {
        result = lv_pro_ebook_res_play_mode(mode, index);
    }

    if (result != 0) {
        list_skip_to_index(media_list, cur_index);
    }

    lv_label_set_text_fmt(ebook_label, "%s",
            lv_pro_ebook_res_get_media_list()->current_node->name);

    return result;
}

list_head_t *lv_pro_ebook_res_get_media_list() { return media_list; }

void lv_pro_ebook_res_get_info(struct EBookInfo *pi)
{
    float filesize = 0;
    char *cur_path = NULL;
    struct stat file_info;

    cur_path = media_list->current_node->path;
    stat(cur_path, &file_info);

    filesize = file_info.st_size;
    if (filesize <= 0) {
        strcpy(pi->filesize, "--");
    } else {
        if (filesize < 1024) {
            snprintf(pi->filesize, sizeof(pi->filesize), "%.2fB", filesize);
        } else if ((filesize >= 1024) && (filesize < 1048576)) {
            filesize = filesize / 1024.0;
            snprintf(pi->filesize, sizeof(pi->filesize), "%.2fKB", filesize);
        } else if ((filesize >= 1048576) && (filesize < 1073741824)) {
            filesize = filesize / 1048576.0;
            snprintf(pi->filesize, sizeof(pi->filesize), "%.2fMB", filesize);
        } else {
            filesize = filesize / 1073741824.0;
            snprintf(pi->filesize, sizeof(pi->filesize), "%.2fGB", filesize);
        }
    }
}
