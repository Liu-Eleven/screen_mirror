#ifndef _PICTURE_PLAYER_INT_H_
#define _PICTURE_PLAYER_INT_H_

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/select.h>
#include "include/sys_msg.h"

#include "lv_pro_res_mid_list.h"

typedef enum {
    MSG_TYPE_PICTURE_DEC_ERROR,
    MSG_TYPE_PICTURE_PLAY,
    MSG_TYPE_PICTURE_PLAY_NEXT,
    MSG_TYPE_PICTURE_PLAY_LIST,
    MSG_TYPE_PICTURE_UNSUPPORT_RESOLUTION,
    MSG_TYPE_PICTURE_REFRESH_STATUS,
} lv_picture_msg_type_t;

typedef struct {
    lv_picture_msg_type_t type;
    void *data;
    bool free_data;
} lv_picture_msg_t;


struct PictureInfo {
    char resolution[11];
    char filesize[10];
    char date[11];
    char time[9];
};

int lv_pro_picture_msg_enqueue(lv_picture_msg_type_t type, void *data, bool free_data);
int lv_pro_res_picture_list_init(char *cur_path, char *fn);
int lv_pro_res_picture_list_deinit(void);
void lv_pro_res_picture_play_mode(int mode , int index);
list_head_t* lv_pro_res_picture_get_media_list();
void lv_pro_res_picture_get_info(struct PictureInfo *pi);

#endif
