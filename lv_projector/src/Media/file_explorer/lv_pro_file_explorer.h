/**
 * @file lv_pro_file_explorer.h
 *
 */

#ifndef LV_PRO_FILE_EXPLORER_H
#define LV_PRO_FILE_EXPLORER_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lvgl/lvgl.h"

/*File explorer*/
#define LV_USE_PRO_FILE_EXPLORER                     1
#if LV_USE_PRO_FILE_EXPLORER
    /*Maximum length of path*/
    #define LV_PRO_FILE_EXPLORER_PATH_MAX_LEN        (128)
    /*Quick access bar, 1:use, 0:not use*/
    #define LV_PRO_FILE_EXPLORER_QUICK_ACCESS        0
#endif

#if LV_USE_PRO_FILE_EXPLORER != 0

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef enum {
    LV_PRO_EXPLORER_SORT_KIND,
    LV_PRO_EXPLORER_SORT_NAME,
} lv_pro_file_explorer_sort_t;

#if LV_PRO_FILE_EXPLORER_QUICK_ACCESS
typedef enum {
    LV_PRO_EXPLORER_HOME_DIR,
    LV_PRO_EXPLORER_MUSIC_DIR,
    LV_PRO_EXPLORER_PICTURES_DIR,
    LV_PRO_EXPLORER_VIDEO_DIR,
    LV_PRO_EXPLORER_DOCS_DIR,
    LV_PRO_EXPLORER_MNT_DIR,
    LV_PRO_EXPLORER_FS_DIR,
} lv_pro_file_explorer_dir_t;
#endif

#define DIR_MAX_DEPTH   8
typedef struct {
    char dir_path[64];
    uint32_t focues_id;
} dir_info_t;

typedef struct {
    char *root_dir;
    char *disk_name;
    dir_info_t dir_info[DIR_MAX_DEPTH];
    uint32_t cur_dir_depth;
} file_info_t;

/*Data of canvas*/
typedef struct {
    lv_obj_t obj;
    lv_obj_t * cont;
    lv_obj_t * head_area;
    lv_obj_t * brower_area;
    lv_obj_t * file_list;
    lv_obj_t * quick_access_ctrl_btn;
    lv_obj_t * path_label;
#if LV_PRO_FILE_EXPLORER_QUICK_ACCESS
    lv_obj_t * quick_access_area;
    lv_obj_t * list_device;
    lv_obj_t * list_places;
    char * home_dir;
    char * music_dir;
    char * pictures_dir;
    char * video_dir;
    char * docs_dir;
#endif
    char * sel_fp;
    char   cur_path[LV_PRO_FILE_EXPLORER_PATH_MAX_LEN];
    file_info_t file_info;
} lv_pro_file_explorer_t;

typedef enum FileType
{
    FileType_Movie    = 0,
    FileType_Music    = 1,
    FileType_Picture  = 2,
    FileType_TXT      = 3,
} FileType;

extern FileType media_filetype;

/***********************
 * GLOBAL VARIABLES
 ***********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
lv_obj_t * lv_pro_file_explorer_create(lv_obj_t * parent);

/*=====================
 * Setter functions
 *====================*/

#if LV_PRO_FILE_EXPLORER_QUICK_ACCESS
/**
 * Set file_explorer
 * @param obj   pointer to a label object
 * @param dir   the dir from 'lv_pro_file_explorer_dir_t' enum.
 */
void lv_pro_file_explorer_set_quick_access_path(lv_obj_t * obj, lv_pro_file_explorer_dir_t dir, char * path);

/**
 * Set file_explorer quick access state
 * @param obj   pointer to a label object
 * @param state true:display, false: hide
 */
void lv_pro_file_explorer_set_quick_access_state(lv_obj_t * obj, bool state);
#endif


/**
 * Set file_explorer sort
 * @param obj   pointer to a label object
 * @param sort  the sort from 'lv_pro_file_explorer_sort_t' enum.
 */
void lv_pro_file_explorer_set_sort(lv_obj_t * obj, lv_pro_file_explorer_sort_t sort);

/*=====================
 * Getter functions
 *====================*/

/**
 * Get file explorer Selected file
 * @param obj   pointer to a file explorer object
 * @return      pointer to the file explorer selected file
 */
char * lv_pro_file_explorer_get_sel_fn(lv_obj_t * obj);

/**
 * Get file explorer cur path
 * @param obj   pointer to a file explorer object
 * @return      pointer to the file explorer cur path
 */
char * lv_pro_file_explorer_get_cur_path(lv_obj_t * obj);

/**
 * Get file explorer head area obj
 * @param obj   pointer to a file explorer object
 * @return      pointer to the file explorer head area obj
 */
lv_obj_t * lv_pro_file_explorer_get_head_area(lv_obj_t * obj);

/**
 * Get file explorer path obj(label)
 * @param obj   pointer to a file explorer object
 * @return      pointer to the file explorer path obj(label)
 */
lv_obj_t * lv_pro_file_explorer_get_path_obj(lv_obj_t * obj);

#if LV_PRO_FILE_EXPLORER_QUICK_ACCESS
/**
 * Get file explorer quick access ctrl btn obj(lv_btn)
 * @param obj   pointer to a file explorer object
 * @return      pointer to the file explorer quick access ctrl btn obj(lv_btn)
 */
lv_obj_t * lv_pro_file_explorer_get_quick_access_ctrl_btn(lv_obj_t * obj);

/**
 * Get file explorer places list obj(lv_list)
 * @param obj   pointer to a file explorer object
 * @return      pointer to the file explorer places list obj(lv_list)
 */
lv_obj_t * lv_pro_file_explorer_get_places_list(lv_obj_t * obj);

/**
 * Get file explorer device list obj(lv_list)
 * @param obj   pointer to a file explorer object
 * @return      pointer to the file explorer device list obj(lv_list)
 */
lv_obj_t * lv_pro_file_explorer_get_device_list(lv_obj_t * obj);
#endif

/**
 * Get file explorer file list obj(lv_table)
 * @param obj   pointer to a file explorer object
 * @return      pointer to the file explorer device obj(lv_table)
 */
lv_obj_t * lv_pro_file_explorer_get_file_list(lv_obj_t * obj);


/*=====================
 * Other functions
 *====================*/

/**
 * Open a specified path
 * @param obj   pointer to a file explorer object
 * @param dir   pointer to the path
 */
void lv_pro_file_explorer_open_dir(lv_obj_t * obj, char * dir, char * disk);

bool is_end_with(const char * str1, const char * str2);

bool lv_pro_check_file_type(const char * file_name, FileType filetype);

#ifdef MOVIE_SUBTITLE_SUPPORT
bool lv_pro_file_subtitile_list_create(char * name);
void lv_pro_file_subtitle_list_free();
void* lv_pro_file_subtitile_list_get();
int lv_pro_file_rm_extension(char *str_out, char *str_in);
bool lv_pro_file_optional_filter(char* file_name,char* ext);
void lv_pro_file_get_fullname(char *fullname,char *path, char *name);
#endif

/**********************
 *      MACROS
 **********************/

#endif  /*LV_USE_SKETCHPAD*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_SKETCHPAD_H*/
