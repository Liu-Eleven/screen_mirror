#ifndef _MEDIA_PLAYER_INT_H_
#define _MEDIA_PLAYER_INT_H_

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
    MSG_TYPE_MEDIA_ERROR,
    MSG_TYPE_MEDIA_PLAY,
    MSG_TYPE_MEDIA_DESTORY,
    MSG_TYPE_MEDIA_PLAY_NEXT,
    MSG_TYPE_MEDIA_PLAY_LIST,
    MSG_TYPE_MEDIA_AUDIO_UNSUPPORTED,
    MSG_TYPE_MEDIA_REFRESH_INFO,
    MSG_TYPE_MEDIA_VIDEO_SIZE_CHANGE,
} lv_media_msg_type_t;

typedef struct {
    lv_media_msg_type_t type;
    void *data;
    bool free_data;
} lv_media_msg_t;

struct MovieInfo {
    char resolution[11];
    int audio;
    int subtitle;
    char filesize[10];
};

typedef enum MovieRatio
{
    Movie_Ratio_Auto   = 0,
    Movie_Ratio_16_9   = 1,
    Movie_Ratio_4_3    = 2,
} MovieRatio;

#define MUSIC_INFO_MAXSIZE 64

struct MusicInfo {
    char album[MUSIC_INFO_MAXSIZE];
    char artist[MUSIC_INFO_MAXSIZE];
    char filesize[10];
};

#define MOVIE_SUBTITLE_SUPPORT
#ifdef MOVIE_SUBTITLE_SUPPORT
typedef struct {
	int type; //0: pic; 1: text
	int w;
	int h;
	char *data;
} lv_subtitle_t;

typedef enum subtitles_event_t_ {

    SUBTITLES_EVENT_SHOW,
    SUBTITLES_EVENT_HIDDEN,
    SUBTITLES_EVENT_PAUSE,
    SUBTITLES_EVENT_RESUME,
    SUBTITLES_EVENT_STOP,
} subtitles_event_t;

#define MAX_EXT_SUBTITLE_NUM 24
typedef struct ext_subtitle {
    int ext_subs_count;
    char ** uris;
} ext_subtitle_t ;

ext_subtitle_t * ext_subtitle_data_get(void);
int ext_subtitles_init(void);
int ext_subtitle_deinit(void);

void subtitles_event_send(int e, lv_subtitle_t *subtitle);
#endif

int lv_pro_media_msg_enqueue(lv_media_msg_type_t type, void *data, bool free_data);
int lv_pro_res_media_init(void);
int lv_pro_res_media_deinit(void);
int lv_pro_res_media_list_init(char *cur_path, char *fn);
int lv_pro_res_media_list_deinit(void);
void lv_pro_res_media_destory(void);
void lv_pro_res_media_play_mode(int mode , int index);
int lv_pro_res_media_play(void);
int lv_pro_res_media_pause(void);
int lv_pro_res_media_stop(void);
int lv_pro_res_media_get_volumn(void);
int lv_pro_res_media_set_volumn(int volume);
int lv_pro_res_media_set_looping(bool bLoop);
void lv_pro_res_media_set_seek(bool backward);
int lv_pro_res_media_set_speed(int speed);
int lv_pro_res_movie_set_ratio(MovieRatio ratio);
void lv_pro_res_movie_get_info(struct MovieInfo *mi);
int lv_pro_res_movie_switch_audio(int nStreamIndex);
int lv_pro_res_movie_switch_subtitle(int nStreamIndex);
void lv_pro_res_music_get_info(struct MusicInfo *mi);
void lv_pro_res_media_get_time(char *curTime, char *totalTime, double *percent);
list_head_t* lv_pro_res_media_get_media_list();
bool lv_pro_res_media_get_playing();

void lv_pro_movie_refresh_info(void);
void lv_pro_music_refresh_info(void);
void lv_pro_media_reset_screen_resolution(void);

#endif
