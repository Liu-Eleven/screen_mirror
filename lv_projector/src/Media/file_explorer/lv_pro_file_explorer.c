/**
 * @file lv_pro_file_explorer.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_pro_file_explorer.h"
#include "../../widget/lv_pro_res.h"
#include "../../widget/lv_pro_set_btn_style1.h"
#include "../middle_ware/lv_pro_res_conv_to_utf8.h"
#include "../lv_pro_media.h"
#include "lv_common.h"
#include "lv_string_id.h"

#if LV_USE_PRO_FILE_EXPLORER != 0
#include <string.h>

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &lv_pro_file_explorer_class

#define FILE_EXPLORER_HEAD_HEIGHT               (60)
#define FILE_EXPLORER_QUICK_ACCESS_AREA_WIDTH   (22)
#define FILE_EXPLORER_BROWER_AREA_WIDTH         (100 - FILE_EXPLORER_QUICK_ACCESS_AREA_WIDTH)

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_pro_file_explorer_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_pro_file_explorer_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);

static void brower_file_event_handler(lv_event_t * e);
#if LV_PRO_FILE_EXPLORER_QUICK_ACCESS
    static void quick_access_event_handler(lv_event_t * e);
    static void quick_access_ctrl_btn_event_handler(lv_event_t * e);
#endif

static void init_style(lv_obj_t * obj);
static void show_dir(lv_obj_t * obj, char * path);
static void strip_ext(char * dir);
static bool is_begin_with(const char * str1, const char * str2);
static int col_cnt = 4;
static int cell_width = 250;
static int cell_height = 250;
static int cell_pad = 40;
lv_group_t *File_left_group;
extern lv_group_t *File_right_group;
#define LV_FILE_DIRECTORY "A:/usr/share/lv_projector/file_dir.png"

static char *filetype_icon[] = {
    "A:/usr/share/lv_projector/file_movie.png",
    "A:/usr/share/lv_projector/file_music.png",
    "A:/usr/share/lv_projector/file_picture.png",
    "A:/usr/share/lv_projector/file_txt.png",
};

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_pro_file_explorer_class = {
    .constructor_cb = lv_pro_file_explorer_constructor,
    .destructor_cb  = lv_pro_file_explorer_destructor,
    .width_def      = LV_SIZE_CONTENT,
    .height_def     = LV_SIZE_CONTENT,
    .instance_size  = sizeof(lv_pro_file_explorer_t),
    .base_class     = &lv_obj_class
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * lv_pro_file_explorer_create(lv_obj_t * parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}


/*=====================
 * Setter functions
 *====================*/
#if LV_PRO_FILE_EXPLORER_QUICK_ACCESS
void lv_pro_file_explorer_set_quick_access_path(lv_obj_t * obj, lv_pro_file_explorer_dir_t dir, char * path)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_pro_file_explorer_t * explorer = (lv_pro_file_explorer_t *)obj;

    /*If path is unavailable */
    if((path == NULL) || (strlen(path) <= 0)) return;

    char ** dir_str = NULL;
    switch (dir)
    {
        case LV_PRO_EXPLORER_HOME_DIR:
            dir_str = &(explorer->home_dir);
            break;
        case LV_PRO_EXPLORER_MUSIC_DIR:
            dir_str = &(explorer->music_dir);
            break;
        case LV_PRO_EXPLORER_PICTURES_DIR:
            dir_str = &(explorer->pictures_dir);
            break;
        case LV_PRO_EXPLORER_VIDEO_DIR:
            dir_str = &(explorer->video_dir);
            break;
        case LV_PRO_EXPLORER_DOCS_DIR:
            dir_str = &(explorer->docs_dir);
            break;

        default:
            return;
            break;
    }

    /*Free the old text*/
    if(*dir_str != NULL) {
        lv_mem_free(*dir_str);
        *dir_str = NULL;
    }

    /*Get the size of the text*/
    size_t len = strlen(path) + 1;

    /*Allocate space for the new text*/
    *dir_str = lv_mem_alloc(len);
    LV_ASSERT_MALLOC(*dir_str);
    if(*dir_str == NULL) return;

    strcpy(*dir_str, path);
}


void lv_pro_file_explorer_set_quick_access_state(lv_obj_t * obj, bool state)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_pro_file_explorer_t * explorer = (lv_pro_file_explorer_t *)obj;

    if(state != (lv_obj_has_state(explorer->quick_access_ctrl_btn, LV_STATE_CHECKED)))
        return;

    if (!lv_obj_has_state(explorer->quick_access_ctrl_btn, LV_STATE_CHECKED))
        lv_obj_add_state(explorer->quick_access_ctrl_btn, LV_STATE_CHECKED);
    else lv_obj_clear_state(explorer->quick_access_ctrl_btn, LV_STATE_CHECKED);

    lv_event_send(explorer->quick_access_ctrl_btn, LV_EVENT_VALUE_CHANGED, NULL);
}
#endif



/*=====================
 * Getter functions
 *====================*/
char * lv_pro_file_explorer_get_sel_fn(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_pro_file_explorer_t * explorer = (lv_pro_file_explorer_t *)obj;

    return explorer->sel_fp;
}

char * lv_pro_file_explorer_get_cur_path(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_pro_file_explorer_t * explorer = (lv_pro_file_explorer_t *)obj;

    return explorer->cur_path;
}

lv_obj_t * lv_pro_file_explorer_get_file_list(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_pro_file_explorer_t * explorer = (lv_pro_file_explorer_t *)obj;

    return explorer->file_list;
}

lv_obj_t * lv_pro_file_explorer_get_head(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_pro_file_explorer_t * explorer = (lv_pro_file_explorer_t *)obj;

    return explorer->head_area;
}

lv_obj_t * lv_pro_file_explorer_get_path_obj(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_pro_file_explorer_t * explorer = (lv_pro_file_explorer_t *)obj;

    return explorer->path_label;
}

#if LV_PRO_FILE_EXPLORER_QUICK_ACCESS
lv_obj_t * lv_pro_file_explorer_get_places_list(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_pro_file_explorer_t * explorer = (lv_pro_file_explorer_t *)obj;

    return explorer->list_places;
}

lv_obj_t * lv_pro_file_explorer_get_quick_access_ctrl_btn(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_pro_file_explorer_t * explorer = (lv_pro_file_explorer_t *)obj;

    return explorer->quick_access_ctrl_btn;
}
#endif


/*=====================
 * Other functions
 *====================*/
void lv_pro_file_explorer_open_dir(lv_obj_t * obj, char * dir, char * disk)
{
    lv_pro_file_explorer_t * explorer = (lv_pro_file_explorer_t *)obj;

    memset(&explorer->file_info, 0, sizeof(file_info_t));
    explorer->file_info.root_dir = dir;
    explorer->file_info.disk_name = disk;

    show_dir(obj, dir);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void lv_pro_file_explorer_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_pro_file_explorer_t * explorer = (lv_pro_file_explorer_t *)obj;

#if LV_PRO_FILE_EXPLORER_QUICK_ACCESS
    explorer->home_dir = NULL;
    explorer->video_dir = NULL;
    explorer->pictures_dir = NULL;
    explorer->music_dir = NULL;
    explorer->docs_dir = NULL;
#endif

    lv_memset_00(explorer->cur_path, sizeof(explorer->cur_path));

    lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_COLUMN);

    explorer->cont = lv_obj_create(obj);
    lv_obj_set_width(explorer->cont, LV_PCT(100));
    lv_obj_set_flex_grow(explorer->cont, 1);

#if LV_PRO_FILE_EXPLORER_QUICK_ACCESS
    // 左侧快速访问栏区域
    explorer->quick_access_area = lv_obj_create(explorer->cont);
    lv_obj_set_size(explorer->quick_access_area, LV_PCT(FILE_EXPLORER_QUICK_ACCESS_AREA_WIDTH), LV_PCT(100));
    lv_obj_set_flex_flow(explorer->quick_access_area, LV_FLEX_FLOW_COLUMN);
#endif

    /* 右侧文件浏览区域 */
    explorer->brower_area = lv_obj_create(explorer->cont);
#if LV_PRO_FILE_EXPLORER_QUICK_ACCESS
    lv_obj_set_size(explorer->brower_area, LV_PCT(FILE_EXPLORER_BROWER_AREA_WIDTH), LV_PCT(100));
#else
   lv_obj_set_size(explorer->brower_area, LV_PCT(100), LV_PCT(100));
#endif
    lv_obj_set_flex_flow(explorer->brower_area, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(explorer->brower_area, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // 展示在文件浏览列表之上的区域(head)
    explorer->head_area = lv_obj_create(explorer->brower_area);
    lv_obj_clear_flag(explorer->head_area, LV_OBJ_FLAG_SCROLLABLE);

#if LV_PRO_FILE_EXPLORER_QUICK_ACCESS
    // 快速访问栏控制按钮
    explorer->quick_access_ctrl_btn = lv_btn_create(explorer->head_area);
    lv_obj_align(explorer->quick_access_ctrl_btn, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_add_flag(explorer->quick_access_ctrl_btn, LV_OBJ_FLAG_CHECKABLE);

    // 快速访问控制按钮上的label
    lv_obj_t * label = lv_label_create(explorer->quick_access_ctrl_btn);
    lv_label_set_text(label, LV_SYMBOL_LIST);

    // 快速访问区域的列表
    lv_obj_t * btn;

    // 列表
    explorer->list_places = lv_list_create(explorer->quick_access_area);
    lv_obj_set_size(explorer->list_places, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(lv_list_add_text(explorer->list_places, "PLACES"), lv_palette_main(LV_PALETTE_LIME), 0);

    btn = lv_list_add_btn(explorer->list_places, NULL, LV_SYMBOL_HOME " HOME");
    lv_group_add_obj(File_left_group, btn);
    lv_obj_add_event_cb(btn, quick_access_event_handler, LV_EVENT_ALL, obj);

    btn = lv_list_add_btn(explorer->list_places, NULL, LV_SYMBOL_VIDEO " Video");
    lv_group_add_obj(File_left_group, btn);
    lv_obj_add_event_cb(btn, quick_access_event_handler, LV_EVENT_ALL, obj);

    btn = lv_list_add_btn(explorer->list_places, NULL, LV_SYMBOL_AUDIO " Music");
    lv_group_add_obj(File_left_group, btn);
    lv_obj_add_event_cb(btn, quick_access_event_handler, LV_EVENT_ALL, obj);

    btn = lv_list_add_btn(explorer->list_places, NULL, LV_SYMBOL_IMAGE " Picture");
    lv_group_add_obj(File_left_group, btn);
    lv_obj_add_event_cb(btn, quick_access_event_handler, LV_EVENT_ALL, obj);

    btn = lv_list_add_btn(explorer->list_places, NULL, LV_SYMBOL_FILE "  EBook");
    lv_group_add_obj(File_left_group, btn);
    lv_obj_add_event_cb(btn, quick_access_event_handler, LV_EVENT_ALL, obj);
#endif

    // 展示当前路径
    explorer->path_label = lv_label_create(explorer->head_area);
    lv_obj_set_style_text_font(explorer->path_label, &GENERAL_FONT_MID, 0);
    lv_obj_add_style(explorer->path_label, &lv_pro_res.label_white, 0);
    lv_label_set_text(explorer->path_label, "file explorer");

    // 目录内容展示列表
    explorer->file_list = lv_obj_create(explorer->brower_area);
    lv_obj_set_size(explorer->file_list, LV_PCT(80), LV_PCT(85));
    lv_obj_set_flex_flow(explorer->file_list, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_scrollbar_mode(explorer->file_list, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_all(explorer->file_list, 20, 0);

    cell_width = ((LV_HOR_RES * 0.8) - (cell_pad * (col_cnt - 1)) - 40) / col_cnt;
    cell_height = cell_width;

    // 初始化样式
    init_style(obj);

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_pro_file_explorer_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");
}

static void init_style(lv_obj_t * obj)
{
    lv_pro_file_explorer_t * explorer = (lv_pro_file_explorer_t *)obj;

    // cont区域风格样式
    static lv_style_t cont_style;
    lv_style_init(&cont_style);
    lv_style_set_radius(&cont_style, 0);
    lv_style_set_bg_opa(&cont_style, LV_OPA_0);
    lv_style_set_border_width(&cont_style, 0);
    lv_style_set_outline_width(&cont_style, 0);
    lv_style_set_pad_column(&cont_style, 0);
    lv_style_set_pad_row(&cont_style, 0);
    lv_style_set_flex_flow(&cont_style, LV_FLEX_FLOW_ROW);
    lv_style_set_pad_all(&cont_style, 0);
    lv_style_set_layout(&cont_style, LV_LAYOUT_FLEX);

#if LV_PRO_FILE_EXPLORER_QUICK_ACCESS
    // quick_access区域风格样式
    static lv_style_t quick_access_area_style;
    lv_style_init(&quick_access_area_style);
    lv_style_set_pad_all(&quick_access_area_style, 0);
    lv_style_set_pad_row(&quick_access_area_style, 20);
    lv_style_set_radius(&quick_access_area_style, 0);
    lv_style_set_border_width(&quick_access_area_style, 1);
    lv_style_set_outline_width(&quick_access_area_style, 0);
    lv_style_set_bg_color(&quick_access_area_style, COLOR_BLUE);
#endif

    // 文件查看、浏览区域风格样式
    static lv_style_t brower_area_style;
    lv_style_init(&brower_area_style);
    lv_style_set_pad_all(&brower_area_style, 0);
    lv_style_set_radius(&brower_area_style, 0);
    lv_style_set_border_width(&brower_area_style, 0);
    lv_style_set_outline_width(&brower_area_style, 0);
    lv_style_set_bg_color(&brower_area_style, COLOR_BLUE);

#if LV_PRO_FILE_EXPLORER_QUICK_ACCESS
    // 左侧菜单的风格样式
    static lv_style_t quick_access_list_style;
    lv_style_init(&quick_access_list_style);
    lv_style_set_border_width(&quick_access_list_style, 0);
    lv_style_set_outline_width(&quick_access_list_style, 0);
    lv_style_set_radius(&quick_access_list_style, 0);
    lv_style_set_pad_all(&quick_access_list_style, 0);

    static lv_style_t quick_access_list_btn_style;
    lv_style_init(&quick_access_list_btn_style);
    lv_style_set_border_width(&quick_access_list_btn_style, 0);
    lv_style_set_bg_color(&quick_access_list_btn_style, COLOR_BLUE);
#endif

    // 右侧文件浏览区域的样式风格
    static lv_style_t file_list_style;
    lv_style_init(&file_list_style);
    lv_style_set_bg_color(&file_list_style, COLOR_BLUE);
    lv_style_set_pad_all(&file_list_style, 0);
    lv_style_set_pad_column(&file_list_style, cell_pad);
    lv_style_set_radius(&file_list_style, 0);
    lv_style_set_border_width(&file_list_style, 0);
    lv_style_set_outline_width(&file_list_style, 0);

    // 设置 file_explorer 的样式
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_bg_color(obj, COLOR_BLUE, 0);

    // 在文件浏览列表之上的区域的样式
    lv_obj_set_size(explorer->head_area, LV_PCT(90), LV_PCT(5));
    lv_obj_set_style_bg_color(explorer->head_area, COLOR_BLACK, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(explorer->head_area, 60, LV_PART_MAIN);
    lv_obj_set_style_radius(explorer->head_area, 0, 0);
    lv_obj_set_style_border_width(explorer->head_area, 0, 0);
    lv_obj_set_style_pad_top(explorer->head_area, 0, 0);

#if LV_PRO_FILE_EXPLORER_QUICK_ACCESS
    // 设置快速访问栏控制按钮的样式
    lv_obj_set_style_radius(explorer->quick_access_ctrl_btn, 0, 0);
    lv_obj_set_style_pad_all(explorer->quick_access_ctrl_btn, 5, 0);
    lv_obj_add_event_cb(explorer->quick_access_ctrl_btn, quick_access_ctrl_btn_event_handler, LV_EVENT_VALUE_CHANGED, explorer);
#endif

    lv_obj_add_style(explorer->cont, &cont_style, 0);
    lv_obj_add_style(explorer->brower_area, &brower_area_style, 0);
#if LV_PRO_FILE_EXPLORER_QUICK_ACCESS
    lv_obj_add_style(explorer->quick_access_area, &quick_access_area_style, 0);
    lv_obj_add_style(explorer->list_places, &quick_access_list_style, 0);
#endif
    lv_obj_add_style(explorer->file_list, &file_list_style, 0);

#if LV_PRO_FILE_EXPLORER_QUICK_ACCESS
    uint32_t i, j;
    for(i = 0; i < lv_obj_get_child_cnt(explorer->quick_access_area); i++) {
        lv_obj_t * child = lv_obj_get_child(explorer->quick_access_area, i);
        if(lv_obj_check_type(child, &lv_list_class)) {
            for(j = 0; j < lv_obj_get_child_cnt(child); j++) {
                lv_obj_t * list_child = lv_obj_get_child(child, j);
                if(lv_obj_check_type(list_child, &lv_list_btn_class)) {
                    lv_obj_add_style(list_child, &quick_access_list_btn_style, 0);
                }
            }
        }
    }
#endif

}

#if LV_PRO_FILE_EXPLORER_QUICK_ACCESS
static void quick_access_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);
    lv_obj_t * obj = lv_event_get_user_data(e);
    uint32_t *key = lv_event_get_param(e);

    lv_pro_file_explorer_t * explorer = (lv_pro_file_explorer_t *)obj;

    if(code == LV_EVENT_CLICKED) {
        char ** path = NULL;
        lv_obj_t * label = lv_obj_get_child(btn, -1);
        char * label_text = lv_label_get_text(label);

        if((strcmp(label_text, LV_SYMBOL_HOME " HOME") == 0)) {
            path = &(explorer->home_dir);
        }
        else if((strcmp(label_text, LV_SYMBOL_VIDEO " Video") == 0)) {
            path = &(explorer->video_dir);
        }
        else if((strcmp(label_text, LV_SYMBOL_AUDIO " Music") == 0)) {
            path = &(explorer->music_dir);
        }
        else if((strcmp(label_text, LV_SYMBOL_IMAGE " Picture") == 0)) {
            path = &(explorer->pictures_dir);
        }
        else if((strcmp(label_text, LV_SYMBOL_FILE "  EBook") == 0)) {
            path = &(explorer->docs_dir);
        }

        if (path != NULL)
            show_dir(obj, *path);
    } else if (code == LV_EVENT_KEY) {
        if (*key == LV_KEY_RIGHT) {
            set_current_ui_group(File_right_group);
        }
    }
}

static void quick_access_ctrl_btn_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);
    lv_obj_t * obj = lv_event_get_user_data(e);

    lv_pro_file_explorer_t * explorer = (lv_pro_file_explorer_t *)obj;

    if(code == LV_EVENT_VALUE_CHANGED) {
        if (lv_obj_has_state(btn, LV_STATE_CHECKED)) {
            lv_obj_add_flag(explorer->quick_access_area, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_size(explorer->brower_area, LV_PCT(100), LV_PCT(100));
        }
        else {
            lv_obj_clear_flag(explorer->quick_access_area, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_size(explorer->brower_area, LV_PCT(FILE_EXPLORER_BROWER_AREA_WIDTH), LV_PCT(100));
        }
    }
}
#endif

static void brower_file_find_obj(lv_obj_t *cur_obj, uint32_t key)
{
    lv_group_t *g = lv_obj_get_group(cur_obj);
    int all_objs_count = lv_group_get_obj_count(g);
    int cur_id = lv_group_get_obj_id(g, cur_obj);

    //printf("all_objs_count %d, cur_id %d\n", all_objs_count, cur_id);
    /* cur_id = 0 1 2 3 ... */
    if (key == LV_KEY_UP) {
        if ((cur_id - col_cnt + 1) > 0)
            lv_group_focus_obj_for_id(cur_obj, col_cnt, false);
    } else if (key == LV_KEY_DOWN) {
        if ((cur_id + col_cnt + 1) > all_objs_count)
            lv_group_focus_obj_for_id(cur_obj, all_objs_count - cur_id - 1, true);
        else
            lv_group_focus_obj_for_id(cur_obj, col_cnt, true);
    }
}

static void brower_file_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_user_data(e);
    lv_obj_t * btn = lv_event_get_target(e);
    lv_obj_t * cur_obj = lv_obj_get_parent(btn);
    uint32_t *key = lv_event_get_param(e);
    char file_name[LV_PRO_FILE_EXPLORER_PATH_MAX_LEN];
    char cur_path[LV_PRO_FILE_EXPLORER_PATH_MAX_LEN];
    uint8_t next_dir_flag = 0;
    uint8_t cur_dir_depth = 0;
    int focues_id;

    lv_pro_file_explorer_t * explorer = (lv_pro_file_explorer_t *)obj;
    memset(file_name, 0, sizeof(file_name));
    memset(cur_path, 0, sizeof(cur_path));

    cur_dir_depth = explorer->file_info.cur_dir_depth;

    if(code == LV_EVENT_KEY) {
        if (*key == LV_KEY_ENTER) {
            char * str_fn = NULL;
            str_fn = lv_pro_set_btn_style1_get_name(cur_obj);

            lv_snprintf(cur_path, sizeof(file_name), "%s", explorer->cur_path);
            focues_id = lv_group_get_obj_id(File_right_group, btn);

            if((strcmp(str_fn, "..") == 0 || strcmp(str_fn, lv_get_string(STR_BACK)) == 0) &&
                    (strlen(explorer->cur_path) > 3))
            {
                lv_snprintf(file_name, sizeof(file_name), "%s", explorer->cur_path);
                strip_ext(file_name);
                strip_ext(file_name); // 去掉最后的 '/' 路径

                //printf("brower_file_event_handler %s\n", explorer->cur_path);
                if (explorer->file_info.root_dir != NULL && strcmp(explorer->cur_path, explorer->file_info.root_dir) == 0) {
                    switch_page(PAGE_DISK);
                    return;
                }
                next_dir_flag = 1;
            }
            else
            {
                if(strcmp(str_fn, "..") != 0 && strcmp(str_fn, lv_get_string(STR_BACK)) != 0){
                    lv_snprintf(file_name, sizeof(file_name), "%s%s", explorer->cur_path, str_fn);
                    next_dir_flag = 2;
                }
            }
            lv_fs_dir_t dir;
            if(lv_fs_dir_open(&dir, file_name) == LV_FS_RES_OK) {
                lv_fs_dir_close(&dir);
                show_dir(obj, file_name);

                /* record cur focues obj */
                if (next_dir_flag == 1) {
                    if (cur_dir_depth > 0) {
                        memset(&explorer->file_info.dir_info[cur_dir_depth], 0, sizeof(dir_info_t));
                        explorer->file_info.dir_info[cur_dir_depth].focues_id = 0;
                        cur_dir_depth--;
                        explorer->file_info.cur_dir_depth = cur_dir_depth;
                    }
                } else if (next_dir_flag == 2) {
                    if (cur_dir_depth < (DIR_MAX_DEPTH - 1)) {
                        memset(&explorer->file_info.dir_info[cur_dir_depth], 0, sizeof(dir_info_t));
                        strcpy(explorer->file_info.dir_info[cur_dir_depth].dir_path, cur_path);
                        explorer->file_info.dir_info[cur_dir_depth].focues_id = focues_id;
                    }
                    cur_dir_depth++;
                    explorer->file_info.cur_dir_depth = cur_dir_depth;
                }
            }
            else {
                if(strcmp(str_fn, "..") != 0 && strcmp(str_fn, lv_get_string(STR_BACK)) != 0 &&
                        strlen(str_fn) > 0) {
                    explorer->sel_fp = str_fn;
                    lv_event_send(obj, LV_EVENT_VALUE_CHANGED, NULL);
                }
            }
        } else if(*key == LV_KEY_BACK) {
        //    if(strlen(explorer->cur_path) > 3)
            if (explorer->file_info.root_dir != NULL && strcmp(explorer->cur_path, explorer->file_info.root_dir) == 0) {
                switch_page(PAGE_DISK);
            } else {
                memset(file_name, 0, sizeof(file_name));
                lv_snprintf(file_name, sizeof(file_name), "%s", explorer->cur_path);
                strip_ext(file_name);
                strip_ext(file_name);

                lv_fs_dir_t dir;
                if(lv_fs_dir_open(&dir, file_name) == LV_FS_RES_OK) {
                    lv_fs_dir_close(&dir);
                    show_dir(obj, file_name);

                    /* back and focues last obj */
                    if (cur_dir_depth > 0 && cur_dir_depth < (DIR_MAX_DEPTH - 1)) {
                        if (!strncmp(explorer->file_info.dir_info[cur_dir_depth - 1].dir_path, explorer->cur_path, explorer->cur_path) &&
                            explorer->file_info.dir_info[cur_dir_depth - 1].focues_id > 0) {
                            lv_obj_scroll_to_y(explorer->file_list, explorer->file_info.dir_info[cur_dir_depth - 1].focues_id / col_cnt * cell_height, LV_ANIM_OFF);
                            lv_group_focus_obj(lv_group_get_obj_from_id(File_right_group, explorer->file_info.dir_info[cur_dir_depth - 1].focues_id));
                        }

                        memset(&explorer->file_info.dir_info[cur_dir_depth], 0, sizeof(dir_info_t));
                        cur_dir_depth = cur_dir_depth - 1;
                        explorer->file_info.cur_dir_depth = cur_dir_depth;
                    }
                }
            }
        } else if (*key == LV_KEY_UP || *key == LV_KEY_DOWN) {
            brower_file_find_obj(btn, *key);
        }
    } else if(code == LV_EVENT_PRESSED || code == LV_EVENT_FOCUSED) {
        lv_label_set_long_mode(((lv_pro_set_btn_style1_t *)cur_obj)->name, LV_LABEL_LONG_SCROLL_CIRCULAR);
    } else if(code == LV_EVENT_RELEASED || code == LV_EVENT_DEFOCUSED) {
        lv_label_set_long_mode(((lv_pro_set_btn_style1_t *)cur_obj)->name, LV_LABEL_LONG_DOT);
    } else if(code == LV_EVENT_SIZE_CHANGED) {
     //   lv_table_set_col_width(explorer->file_list, 0, lv_obj_get_width(explorer->file_list));
    }
}


static void show_dir(lv_obj_t * obj, char * path)
{
    lv_pro_file_explorer_t * explorer = (lv_pro_file_explorer_t *)obj;

    char fn[LV_PRO_FILE_EXPLORER_PATH_MAX_LEN];
    lv_fs_dir_t dir;
    lv_fs_res_t res;
    lv_obj_t *cell;
    lv_obj_t *cell_btn;
    lv_obj_t *cell_label;

    res = lv_fs_dir_open(&dir, path);
    if(res != LV_FS_RES_OK) {
        LV_LOG_USER("Open dir error %d!", res);
        return;
    }


    lv_obj_clean(explorer->file_list);
    lv_group_remove_all_objs(File_right_group);

    cell = lv_pro_set_btn_style1_create(explorer->file_list);
    lv_obj_set_size(cell, cell_width, cell_height);
    lv_pro_set_btn_style1_src(cell, &GENERAL_FONT_MID,
                        lv_color_hex(0xff8000), LV_OPA_TRANSP,
                        "A:/usr/share/lv_projector/file_back.png", lv_get_string(STR_BACK));
    cell_btn = lv_pro_set_btn_style1_get_btn(cell);
    lv_group_add_obj(File_right_group, cell_btn);
    lv_obj_add_event_cb(cell, brower_file_event_handler, LV_EVENT_ALL, obj);

    while(1) {
        res = lv_fs_dir_read(&dir, fn);
        if(res != LV_FS_RES_OK) {
            LV_LOG_USER("Driver, file or directory is not exists %d!", res);
            break;
        }

        /*fn is empty, if not more files to read*/
        if(strlen(fn) == 0) {
            LV_LOG_USER("Not more files to read!");
            break;
        }

        // 识别并展示文件
        if (lv_pro_check_file_type(fn, media_filetype) == true) {
            cell = lv_pro_set_btn_style1_create(explorer->file_list);
            lv_obj_set_size(cell, cell_width, cell_height);
            lv_pro_set_btn_style1_src(cell, &GENERAL_FONT_MID, lv_color_hex(0xff8000),
                                LV_OPA_TRANSP, filetype_icon[(int)media_filetype], fn);
            cell_btn = lv_pro_set_btn_style1_get_btn(cell);
            lv_group_add_obj(File_right_group, cell_btn);
            lv_obj_add_event_cb(cell, brower_file_event_handler, LV_EVENT_ALL, obj);
        }
        else if((is_end_with(fn , ".") == true) || (is_end_with(fn , "..") == true)) {
            /*is dir*/
            continue;
        }
        else if(fn[0] == '/') {/*is dir*/
            cell = lv_pro_set_btn_style1_create(explorer->file_list);
            lv_obj_set_size(cell, cell_width, cell_height);
            lv_pro_set_btn_style1_src(cell, &GENERAL_FONT_MID, lv_color_hex(0xff8000),
                                LV_OPA_TRANSP, LV_FILE_DIRECTORY, fn+1);
            cell_btn = lv_pro_set_btn_style1_get_btn(cell);
            lv_group_add_obj(File_right_group, cell_btn);
            lv_obj_add_event_cb(cell, brower_file_event_handler, LV_EVENT_ALL, obj);
        }
        //LV_LOG_USER("%s", fn);
    }

    lv_fs_dir_close(&dir);

    lv_memset_00(explorer->cur_path, sizeof(explorer->cur_path));
    strcpy(explorer->cur_path, path);
    size_t cur_path_len = strlen(explorer->cur_path);
    if((*((explorer->cur_path) + cur_path_len - 1) != '/') && \
            (cur_path_len < LV_PRO_FILE_EXPLORER_PATH_MAX_LEN)) {
        *((explorer->cur_path) + cur_path_len) = '/';
    }
    lv_label_set_text_fmt(explorer->path_label, " %s:/%s", explorer->file_info.disk_name,
            explorer->cur_path + strlen(explorer->file_info.root_dir));
}


// 去掉最后的后缀名
static void strip_ext(char *dir)
{
    char *end = dir + strlen(dir);

    while (end >= dir && *end != '/') {
        --end;
    }

    if (end > dir) {
        *end = '\0';
    }
    else if (end == dir) {
        *(end+1) = '\0';
    }

}



static bool is_begin_with(const char * str1, const char * str2)
{
    if(str1 == NULL || str2 == NULL)
        return false;

    uint16_t len1 = strlen(str1);
    uint16_t len2 = strlen(str2);
    if((len1 < len2) || (len1 == 0 || len2 == 0))
        return false;

    uint16_t i = 0;
    char * p = str2;
    while(*p != '\0')
    {
        if(*p != str1[i])
            return false;

        p++;
        i++;
    }

    return true;
}



bool is_end_with(const char * str1, const char * str2)
{
    if(str1 == NULL || str2 == NULL)
        return false;

    char c1, c2;
    uint16_t len1 = strlen(str1);
    uint16_t len2 = strlen(str2);
    if((len1 < len2) || (len1 == 0 || len2 == 0))
        return false;
    
    while(len2 >= 1)
    {
        c1 = *(str1 + len1 - 1);
        c2 = *(str2 + len2 - 1);

        if ((c1 >= 'A') && (c1 <='Z')) {
            c1 += 32;
        }

        if (c1 != c2)
            return false;

        len2--;
        len1--;
    }

    return true;
}



static char *movie_type_support[] = {
    ".mp4",
    ".mov",
    ".avi",
    ".mkv",
    ".ts",
    ".tp",
    ".mts",
    ".m2ts",
    ".mpg",
    ".mpeg",
    ".m4v",
    ".flv",
    ".vob",
    ".3gp",
    ".dat",
    ".f4v",
    ".3g2",
    /* cut off */
    ".asf",
    ".wmv",
    /* unsupport */
    ".webm",
    ".rm",
    ".rmvb",
    ".dmskm",
    ".skm",
    ".k3g",
    ".h264",
};

static char *music_type_support[] = {
    ".mp3",
    ".mp2",
    ".mp1",
    ".wav",
    ".aac",
    ".m4a",
    ".ape",
    ".flac",
    ".ts",
    ".m4r",
    ".mka",
    /* cut off */
    ".ogg",
    ".wmv",
    ".wma",
    ".opus",
    ".amr",
    ".oma",
    /* unsupport */
    ".ac3",
    ".dts",
    ".caf",
    ".ra",
    ".eac3",
    ".mid",
};

static char *pic_type_support[] = {
    ".png",
    ".bmp",
    ".jpg",
    ".jpeg",
    ".jpe",
    ".gif",
};

static char *txt_type_support[] = {
    ".txt",
    ".lrc",
};

bool lv_pro_check_file_type(const char * file_name, FileType filetype)
{
    int i = 0;
    int count = 0;
    if (filetype == FileType_Movie) {
        count = sizeof(movie_type_support) / sizeof(movie_type_support[0]);
        for (i = 0; i < count; i++) {
            if (is_end_with(file_name, movie_type_support[i]) == true) {
                return true;
            }
        }
    } else if (filetype == FileType_Music) {
        count = sizeof(music_type_support) / sizeof(music_type_support[0]);
        for (i = 0; i < count; i++) {
            if (is_end_with(file_name, music_type_support[i]) == true) {
                return true;
            }
        }
    } else if (filetype == FileType_Picture) {
        count = sizeof(pic_type_support) / sizeof(pic_type_support[0]);
        for (i = 0; i < count; i++) {
            if (is_end_with(file_name, pic_type_support[i]) == true) {
                return true;
            }
        }
    } else if (filetype == FileType_TXT) {
        count = sizeof(txt_type_support) / sizeof(txt_type_support[0]);
        for (i = 0; i < count; i++) {
            if (is_end_with(file_name, txt_type_support[i]) == true) {
                return true;
            }
        }
    }
    return false;
}

#ifdef MOVIE_SUBTITLE_SUPPORT
#include "../middle_ware/glist.h"

static glist* subtitle_glist=NULL;

static bool lv_pro_file_subtitle_filter(char* file_name)
{
    int i;
    int j;
    char *ext_name = NULL;

    for (i = 0, j = -1; file_name[i] != '\0'; i++){
        if (file_name[i] == '.') j = i;
    }

    if (j == -1){
        return false;
    }else{
        j++;
        ext_name = file_name + j;
    }
    if( strcasestr(ext_name,"ass")||
        strcasestr(ext_name,"ssa")||
        strcasestr(ext_name,"idx")||
        strcasestr(ext_name,"sub")||
        strcasestr(ext_name,"smi")||
        strcasestr(ext_name,"sami")||
        strcasestr(ext_name,"txt")||
        strcasestr(ext_name,"mpl")||
        strcasestr(ext_name,"mpl2")||
        strcasestr(ext_name,"srt")||
        strcasestr(ext_name,"lrc")||
        strcasestr(ext_name,"aqt")||
        strcasestr(ext_name,"rt")||
        strcasestr(ext_name,"jss")||
        strcasestr(ext_name,"js")||
        strcasestr(ext_name,"vsf")||
        strcasestr(ext_name,"tts")||
        strcasestr(ext_name,"stl")||
        strcasestr(ext_name,"zeg")||
        strcasestr(ext_name,"ovr")||
        strcasestr(ext_name,"dks")||
        strcasestr(ext_name,"sbt")||
        strcasestr(ext_name,"vkt")||
        strcasestr(ext_name,"pjs")||
        strcasestr(ext_name,"scr")||
        strcasestr(ext_name,"psb")||
        strcasestr(ext_name,"asc")||
        strcasestr(ext_name,"rtf")||
        strcasestr(ext_name,"s2k")||
        strcasestr(ext_name,"sst")||
        strcasestr(ext_name,"ssts")||
        strcasestr(ext_name,"son")
    ){
        return true;
    }else{
        return false;
    }
}

bool lv_pro_file_subtitile_list_create(char * name)
{
    if(lv_pro_file_subtitle_filter(name)){
        char *subtitle_file=strdup(name);
        subtitle_glist=glist_append(subtitle_glist,subtitle_file);
        return true;
    }
    return false;
}

static void lv_pro_file_free_data(void *data, void *user_data)
{
	(void)user_data;
	if (data)
		free(data);
}

void lv_pro_file_subtitle_list_free()
{
    if (subtitle_glist) {
        glist_free_full(subtitle_glist, lv_pro_file_free_data);
    }
    subtitle_glist=NULL;
}

void* lv_pro_file_subtitile_list_get()
{
    return (void*)subtitle_glist;
}


int lv_pro_file_rm_extension(char *str_out, char *str_in)
{
    int len = 0;
    int i = 0;
    len = strlen(str_in);
    for(i=0; i<len; i++){
        if('.' == str_in[i]){
            strncpy(str_out, str_in, i);
            break;
        }
    }
    return 0;
}

bool lv_pro_file_optional_filter(char* file_name,char* ext)
{
    int i;
    int j;
    char *ext_name = NULL;

    for (i = 0, j = -1; file_name[i] != '\0'; i++){
        if (file_name[i] == '.') j = i;
    }

    if (j == -1){
        return false;
    }else{
        j++;
        ext_name = file_name + j;
    }
    if( strcasestr(ext_name,ext)){
        return true;
    }else{
        return false;
    }
}

void lv_pro_file_get_fullname(char *fullname,char *path, char *name)
{
    strip_ext(path);
    strcpy(fullname, path);
    strcat(fullname, name);
}
#endif

#endif  /*LV_USE_PRO_FILE_EXPLORER*/
