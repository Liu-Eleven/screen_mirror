#include "lv_pro_res_picture_player_int.h"
#include "lvgl/lvgl.h"
#include "../Picture/lv_pro_picture_activity.h"
#include "../file_explorer/lv_pro_file_explorer.h"
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include "../../Layer/msgbox/lv_ui_msgbox.h"
#include "lv_pro_res_debug.h"

static list_head_t *media_list;
extern lv_obj_t *picture_cont;
extern lv_obj_t *picture_img;
extern pthread_mutex_t picture_destory_mutex;
static int pic_width;
static int pic_height;
int pic_zoom_factor = 256;

int lv_pro_set_picture_rect(int width, int height)
{
    if (picture_img == NULL) {
        mw_err("picture_img is NULL, cannot set zoom\n");
        return 256;
    }

    int zoom_factor = 256;
    if (width > LV_HOR_RES || height > LV_VER_RES) {
        float width_ratio = (float)width / (float)LV_HOR_RES;
        float height_ratio = (float)height / (float)LV_VER_RES;
        if (width_ratio > height_ratio) {
            zoom_factor = 256 / width_ratio;
            lv_img_set_zoom(picture_img, zoom_factor);
        } else {
            zoom_factor = 256 / height_ratio;
            lv_img_set_zoom(picture_img, zoom_factor);
        }
    }
    return zoom_factor;
}

int lv_pro_picture_msg_enqueue(lv_picture_msg_type_t type, void *data, bool free_data)
{
    lv_picture_msg_t *_msg = (lv_picture_msg_t *)malloc(sizeof(lv_picture_msg_t));
    _msg->type = type;
    _msg->data = data;
    _msg->free_data = free_data;
    system_send_msg(MSG_TYPE_PICTURE_FRESH_UI, _msg);
}

static void media_picture_msg_callback(void)
{
    lv_pro_picture_msg_enqueue(MSG_TYPE_PICTURE_PLAY_NEXT, 1, false);
}

void media_picture_message_process(system_msg_t *sys_msg)
{
    lv_group_t *__key_group;
    lv_picture_msg_t *msg;
    void *btn;

    if (sys_msg == NULL) {
        mw_err("sys msg null!\n");
        return;
    }

    if (sys_msg->type != MSG_TYPE_PICTURE_FRESH_UI) {
        return;
    }

    msg = sys_msg->data;

    switch (msg->type) {
    case MSG_TYPE_PICTURE_PLAY:
        lv_pro_res_picture_play_mode(3, 0);
        break;

    case MSG_TYPE_PICTURE_PLAY_NEXT:
        lv_img_cache_invalidate_src(NULL);
        lv_pro_res_picture_play_mode((int)msg->data, 0);
        break;

    case MSG_TYPE_PICTURE_PLAY_LIST:
        lv_img_cache_invalidate_src(NULL);
        lv_pro_res_picture_play_mode(2, (int)msg->data);
        break;

    case MSG_TYPE_PICTURE_DEC_ERROR:
        lv_msgbox_msg_open(lv_scr_act(), lv_get_string(STR_PIC_ERROR),
                2000, media_picture_msg_callback, NULL);
        break;

    case MSG_TYPE_PICTURE_UNSUPPORT_RESOLUTION:
        lv_msgbox_msg_open(lv_scr_act(), lv_get_string(STR_PIC_ERROR),
                2000, media_picture_msg_callback, NULL);
        break;

    case MSG_TYPE_PICTURE_REFRESH_STATUS:
        pthread_mutex_lock(&picture_destory_mutex);
        if (picture_img != NULL) {
            pic_zoom_factor = lv_pro_set_picture_rect(pic_width, pic_height);
            lv_obj_clear_flag(picture_img, LV_OBJ_FLAG_HIDDEN);
            lv_pro_picture_play_status(true);
        }
        pthread_mutex_unlock(&picture_destory_mutex);
        break;
    }

MSG_END:
    if (msg->free_data) {
        free(msg->data);
    }
    if (msg) {
        free(msg);
    }
}

static int lv_pro_get_picture_resoltion(const void *src)
{
    lv_img_header_t header;
    if (lv_img_decoder_get_info(src, &header) != LV_RES_OK) {
        if (picture_img != NULL) {
            lv_obj_update_layout(picture_img);
            pic_width = lv_obj_get_width(picture_img);
            pic_height = lv_obj_get_height(picture_img);
            mw_dbg("get picture resolution: %dx%d.\n", pic_width, pic_height);
        }
    } else {
        pic_width = header.w;
        pic_height = header.h;
        mw_dbg("get picture resolution from header: %dx%d.\n", pic_width, pic_height);
    }
    if ((pic_width == 0) || (pic_height == 0)) {
        lv_pro_picture_msg_enqueue(MSG_TYPE_PICTURE_DEC_ERROR, NULL, false);
        return -1;
    } else if ((pic_width > 4095) || (pic_height > 4095) || (pic_width*pic_height > 2048*2048)) {
        lv_pro_picture_msg_enqueue(MSG_TYPE_PICTURE_UNSUPPORT_RESOLUTION, NULL, false);
        return -1;
    }
    return 0;
}

static int PicturePlayProc(void) {
    if (!picture_activity)
        return 0;

    if(media_list == NULL)
    {
        printf("media_list is null\n");
        return 0;
    }

    if (picture_img) {
        lv_obj_del_async(picture_img);
        picture_img = NULL;
    }
    if (is_end_with(media_list->current_node->path, ".gif") == true) {
        picture_img = lv_gif_create(picture_cont);
        lv_obj_center(picture_img);
        lv_gif_set_src(picture_img, media_list->current_node->path);
        if(((lv_gif_t *)picture_img)->gif != NULL) {
            lv_gif_restart(picture_img);
        }
        lv_pro_get_picture_resoltion(media_list->current_node->path);
    } else {
        picture_img = lv_img_create(picture_cont);
        lv_obj_center(picture_img);
        if (!lv_pro_get_picture_resoltion(media_list->current_node->path)) {
            lv_img_set_src(picture_img, media_list->current_node->path);
        }
        if (picture_img == NULL) {
            mw_err("picture_img is NULL after creation\n");
            return -1;
        }
    }
    lv_obj_add_flag(picture_img, LV_OBJ_FLAG_HIDDEN);
    lv_pro_picture_msg_enqueue(MSG_TYPE_PICTURE_REFRESH_STATUS, NULL, false);

    return 0;
}

int lv_pro_res_picture_list_init(char *cur_path, char *fn) {
    DIR *dp;
    struct dirent *dirp;
    char compete_path[FILE_PATH_MAXT_LEN];
    char compete_name[FILE_NAME_MAXT_LEN];
    int index = 0;
    int cur_index = 1;

    media_list = create_list();

    dp = opendir(cur_path);
    if (!dp) {
        mw_err("open directory %s error\n", cur_path);
        return -1;
    }

    /*Get picture file name */
    while ((dirp = readdir(dp))) {
        if (lv_pro_check_file_type(dirp->d_name, FileType_Picture) == true) {
            //mw_dbg("picture filename: %s/%s\n", cur_path, dirp->d_name);
            memset(compete_path, 0, sizeof(compete_path));
            memset(compete_name, 0, sizeof(compete_name));
            snprintf(compete_path, FILE_PATH_MAXT_LEN, "A:%s/%s", cur_path,
                    dirp->d_name);
            snprintf(compete_name, FILE_NAME_MAXT_LEN, "%s", dirp->d_name);
            list_append(media_list, compete_path, compete_name,
                    MEDIA_F_TYPE_PICTURE);
            ++index;
            if (!strcmp(dirp->d_name, fn)) {
                cur_index = index;
            }
        }
    }

    closedir(dp);

    /* Initialize the default picture, the picture play activity can be played immediately */
    list_skip_to_index(media_list, cur_index);

    return 0;
}

int lv_pro_res_picture_list_deinit(void) {
    destroy_list(media_list);
    media_list = NULL;
    return 0;
}

void lv_pro_res_picture_play_mode(int mode, int index) {
    pthread_mutex_lock(&picture_destory_mutex);
    if(get_current_page_id() != PAGE_PICTURE)
    {
        printf("current page is not PAGE_PICTURE\n");
        goto RETURN;
    }

    if (mode == 1)
    {
        if(list_get_next_node(media_list) == NULL)
            goto RETURN;
    }
    else if (mode == 0)
    {
        if(list_get_pre_node(media_list) == NULL)
            goto RETURN;
    }
    else if (mode == 2)
    {
       if(list_skip_to_index(media_list, index) == -1)
            goto RETURN;
    }

    PicturePlayProc();

RETURN:
    pthread_mutex_unlock(&picture_destory_mutex);
}

list_head_t* lv_pro_res_picture_get_media_list() {
    return media_list;
}

void lv_pro_res_picture_get_info(struct PictureInfo *pi) {
    float filesize = 0;
    char *cur_path = NULL;
    struct stat file_info;

    if (pic_width || pic_height) {
        snprintf(pi->resolution, sizeof(pi->resolution), "%dx%d", pic_width, pic_height);
    } else {
        strcpy(pi->resolution, "--x--");
    }

    cur_path = media_list->current_node->path + 2;
    stat(cur_path, &file_info);

    strftime(pi->date, sizeof(pi->date), "%Y-%m-%d", localtime(&file_info.st_mtime));
    strftime(pi->time, sizeof(pi->time), "%H:%M:%S", localtime(&file_info.st_mtime));

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
