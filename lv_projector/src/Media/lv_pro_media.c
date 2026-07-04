/*
 * lv_pro_media.c
 *
 */

#include "lv_pro_media.h"
#include "lv_pro_media_layout.h"
#include "../widget/lv_pro_set_btn_style1.h"
#include "../lv_pro_launcher.h"
#include "file_explorer/lv_pro_file_explorer.h"
#include "Music/lv_pro_music_activity.h"
#include "Movie/lv_pro_movie_activity.h"
#include "Picture/lv_pro_picture_activity.h"
#include "EBook/lv_pro_ebook_activity.h"
#include "usb/lv_ui_usb.h"
#include "wifi_bt/lv_ui_wifi.h"
#include "usb/lv_ui_usb.h"
#include "disk_manager/DiskManager.h"

lv_obj_t *Media_activity;
lv_group_t *Media_group;
lv_obj_t *Disk_activity;
lv_group_t *Disk_group;
lv_obj_t *File_activity;
lv_group_t *File_right_group;

FileType media_filetype = FileType_Movie;
static char media_root_dir[MOUNT_PATH_MAX_LENGTH+3] = "A:/tmp/USB0/";
static char media_disk_name[2] = "C";
static int media_usb_index = 0;
bool media_mode_single = false;

static void media_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    char *user_data = (char*) lv_event_get_user_data(e);
    uint32_t *key = lv_event_get_param(e);
    struct _lv_obj_t * cur_obj = lv_event_get_target(e);
	struct _lv_obj_t *target_obj;

    if (code == LV_EVENT_KEY) {
        if (*key == LV_KEY_ENTER) {
            if (DiskManager_GetDiskNum() > 0) {
                switch_page(PAGE_DISK);
                if (!strcmp(user_data, "Movie")) {
                    media_filetype = FileType_Movie;
                } else if (!strcmp(user_data, "Music")) {
                    media_filetype = FileType_Music;
                } else if (!strcmp(user_data, "Picture")) {
                    media_filetype = FileType_Picture;
                } else if (!strcmp(user_data, "eBook")) {
                    media_filetype = FileType_TXT;
                }
            } else {
                lv_msgbox_msg_open(lv_layer_top(), lv_get_string(STR_INSERT_USB_DEV),
                        2000, NULL, NULL);
            }
        } else if (*key == LV_KEY_BACK) {
            switch_page(PAGE_HOME);
        }
    }
}

static void disk_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    int user_data = (int) lv_event_get_user_data(e);
    uint32_t *key = lv_event_get_param(e);
    struct _lv_obj_t * cur_obj = lv_event_get_target(e);
	struct _lv_obj_t *target_obj;

    if (code == LV_EVENT_KEY) {
        if (*key == LV_KEY_ENTER) {
            if (user_data == 0) {
                switch_page(PAGE_MEDIA);
            } else if (user_data > 0 && user_data < USB_MOUNT_MAX_COUNT + 1) {
                DiskInfo_t *disk_info = DiskManager_GetDiskInfoByIndex(user_data - 1);
                lv_memset_00(media_root_dir, sizeof(media_root_dir));
                snprintf(media_root_dir, sizeof(media_root_dir), "A:%s/", disk_info->MountPoint);
                snprintf(media_disk_name, 2, "%c", user_data + 66);
                media_usb_index = disk_info->index;
                switch_page(PAGE_FILE);
            }
        } else if (*key == LV_KEY_BACK) {
            switch_page(PAGE_MEDIA);
        }
    }
}

static void lv_pro_disk_ui_init(void) {
    lv_obj_t *cell;
    lv_obj_t *cell_btn;
    int cell_width = DISK_CELL;
    int cell_height = cell_width;
    unsigned int disk_num = 0;
    char disk_name[2];

    Disk_activity = lv_obj_create(NULL);
    Disk_group = lv_group_create();

    lv_obj_set_size(Disk_activity, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(Disk_activity, COLOR_BLUE, 0);

    lv_obj_t *disk_cont = lv_obj_create(Disk_activity);
    lv_obj_set_size(disk_cont, lv_pct(80), lv_pct(40));
    lv_obj_center(disk_cont);
    lv_obj_add_style(disk_cont, &lv_pro_res.set_bg_transp, 0);

    lv_obj_set_style_pad_column(disk_cont, 80, 0);
    lv_obj_set_style_pad_all(disk_cont, 10, 0);
    lv_obj_set_scrollbar_mode(disk_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(disk_cont, LV_FLEX_FLOW_ROW);

    lv_obj_t *back_cell = lv_pro_set_btn_style1_create(disk_cont);
    lv_obj_set_size(back_cell, cell_width, cell_height);
    lv_pro_set_btn_style1_src(back_cell, &GENERAL_FONT_MID,
                        lv_color_hex(0xff8000), LV_OPA_TRANSP,
                        "A:/usr/share/lv_projector/disk_back.png", lv_get_string(STR_BACK));
    lv_obj_t *back_cell_btn = lv_pro_set_btn_style1_get_btn(back_cell);
    lv_group_add_obj(Disk_group, back_cell_btn);
    lv_obj_add_event_cb(back_cell, disk_event_handler, LV_EVENT_KEY, 0);

    disk_num = DiskManager_GetDiskNum();
    for (int i = 0; i < disk_num; i++) {
        snprintf(disk_name, 2, "%c", 67 + i); //"67" means "C" in ASCII
        cell = lv_pro_set_btn_style1_create(disk_cont);
        lv_obj_set_size(cell, cell_width, cell_height);
        lv_pro_set_btn_style1_src(cell, &GENERAL_FONT_MID,
                            lv_color_hex(0xff8000), LV_OPA_TRANSP,
                            "A:/usr/share/lv_projector/media_disk.png", disk_name);
        cell_btn = lv_pro_set_btn_style1_get_btn(cell);
        lv_group_add_obj(Disk_group, cell_btn);
        lv_obj_add_event_cb(cell, disk_event_handler, LV_EVENT_KEY, i + 1);
    }
}

static void lv_pro_disk_ui_exit(void) {
    if (Disk_group) {
        lv_group_remove_all_objs(Disk_group);
        lv_group_del(Disk_group);
        Disk_group = NULL;
    }
    if (Disk_activity) {
        lv_obj_del(Disk_activity);
        Disk_activity = NULL;
    }
}

static void lv_pro_disk_ui_fresh(void) {
    if (Disk_activity) {
        lv_obj_t *cell;
        lv_obj_t *cell_btn;
        int cell_width = DISK_CELL;
        int cell_height = cell_width;
        unsigned int disk_num = 0;
        char disk_name[2];

        lv_obj_t *disk_cont = lv_obj_get_child(Disk_activity, 0);
        lv_obj_clean(disk_cont);
        lv_group_remove_all_objs(Disk_group);

        lv_obj_t *back_cell = lv_pro_set_btn_style1_create(disk_cont);
        lv_obj_set_size(back_cell, cell_width, cell_height);
        lv_pro_set_btn_style1_src(back_cell, &GENERAL_FONT_MID,
                            lv_color_hex(0xff8000), LV_OPA_TRANSP,
                            "A:/usr/share/lv_projector/disk_back.png", lv_get_string(STR_BACK));
        lv_obj_t *back_cell_btn = lv_pro_set_btn_style1_get_btn(back_cell);
        lv_group_add_obj(Disk_group, back_cell_btn);
        lv_obj_add_event_cb(back_cell, disk_event_handler, LV_EVENT_KEY, 0);

        disk_num = DiskManager_GetDiskNum();
        for (int i = 0; i < disk_num; i++) {
            snprintf(disk_name, 2, "%c", 67 + i); //"67" means "C" in ASCII
            cell = lv_pro_set_btn_style1_create(disk_cont);
            lv_obj_set_size(cell, cell_width, cell_height);
            lv_pro_set_btn_style1_src(cell, &GENERAL_FONT_MID,
                                lv_color_hex(0xff8000), LV_OPA_TRANSP,
                                "A:/usr/share/lv_projector/media_disk.png", disk_name);
            cell_btn = lv_pro_set_btn_style1_get_btn(cell);
            lv_group_add_obj(Disk_group, cell_btn);
            lv_obj_add_event_cb(cell, disk_event_handler, LV_EVENT_KEY, i + 1);
        }
    }
}

static int create_disk_ui(void)
{
    lv_pro_disk_ui_init();
    load_current_channel(Disk_activity, Disk_group);

    return 0;
}

static int destory_disk_ui(void)
{
    lv_pro_disk_ui_exit();
    return 0;
}

static int show_disk_ui(void)
{
    return 0;
}

static int hide_disk_ui(void)
{
    return 0;
}

static page_interface_t page_disk_ui =
{
    .ops =
    {
        create_disk_ui,
        destory_disk_ui,
        show_disk_ui,
        hide_disk_ui,
    },
    .info =
    {
        .id = PAGE_DISK,
        .user_data = NULL
    }
};

void REGISTER_PAGE_DISK(void)
{
    reg_page(&page_disk_ui);
}


static void file_explorer_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    uint32_t *key = lv_event_get_param(e);
    struct _lv_obj_t *target_obj;

    if(code == LV_EVENT_VALUE_CHANGED) {
        char * cur_path =  lv_pro_file_explorer_get_cur_path(obj);
        char * sel_fn = lv_pro_file_explorer_get_sel_fn(obj);
        if (media_filetype == FileType_Movie) {
            lv_pro_res_media_list_init(cur_path + 2, sel_fn);
            switch_page_create_hide(PAGE_MOVIE);
        }
        else if (media_filetype == FileType_Music) {
            lv_pro_res_media_list_init(cur_path + 2, sel_fn);
            switch_page_create_hide(PAGE_MUSIC);
        }
        else if (media_filetype == FileType_Picture) {
            lv_pro_res_picture_list_init(cur_path + 2, sel_fn);
            switch_page_create_hide(PAGE_PICTURE);
        }
        else if (media_filetype == FileType_TXT) {
            if (lv_pro_ebook_init(cur_path + 2, sel_fn) == 0) {
                switch_page_create_hide(PAGE_EBOOK);
            } else {
                lv_pro_msgbox_msg_open_on_top(&IDB_Icon_unsupported, lv_get_string(STR_FILE_FAIL),
                                              500);
            }
        }
    }
}

static void lv_pro_file_ui_init(void) {
    File_activity = lv_obj_create(NULL);
    File_right_group = lv_group_create();
    lv_obj_set_size(File_activity, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(File_activity, COLOR_BLUE, 0);

    lv_obj_t * file_explorer = lv_pro_file_explorer_create(File_activity);
    lv_pro_file_explorer_open_dir(file_explorer, media_root_dir, media_disk_name);

    lv_obj_add_event_cb(file_explorer, file_explorer_event_handler, LV_EVENT_VALUE_CHANGED, NULL);

}

void lv_pro_file_ui_exit(void) {
    if (File_right_group) {
        lv_group_remove_all_objs(File_right_group);
        lv_group_del(File_right_group);
        File_right_group = NULL;
    }
    if (File_activity) {
        lv_obj_del_async(File_activity);
        File_activity = NULL;
    }
}

static int create_file_ui(void)
{
    lv_pro_file_ui_init();
    load_current_channel(File_activity, File_right_group);

    return 0;
}

static int destory_file_ui(void)
{
    lv_pro_file_ui_exit();
    return 0;
}

static int show_file_ui(void)
{
    load_current_channel(File_activity, File_right_group);
    return 0;
}

static int hide_file_ui(void)
{
    return 0;
}

static page_interface_t page_file_ui =
{
    .ops =
    {
        create_file_ui,
        destory_file_ui,
        show_file_ui,
        hide_file_ui,
    },
    .info =
    {
        .id = PAGE_FILE,
        .user_data = NULL
    }
};

void REGISTER_PAGE_FILE(void)
{
    reg_page(&page_file_ui);
}


static void media_load_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_SCREEN_LOADED) {
        display_ui_usb_icon();
    } else if (code == LV_EVENT_SCREEN_UNLOADED) {
        hide_ui_usb_icon();
    }
}

void lv_pro_media_ui_init(void) {
    Media_activity = lv_obj_create(NULL);
    Media_group = lv_group_create();

    lv_obj_set_size(Media_activity, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(Media_activity, COLOR_BLUE, 0);

    lv_obj_add_event_cb(Media_activity, media_load_event_handler, LV_EVENT_SCREEN_LOADED, NULL);
    lv_obj_add_event_cb(Media_activity, media_load_event_handler, LV_EVENT_SCREEN_UNLOADED, NULL);

    char *png_path[] = { "A:/usr/share/lv_projector/movie.png",
            "A:/usr/share/lv_projector/music.png",
            "A:/usr/share/lv_projector/picture.png",
            "A:/usr/share/lv_projector/ebook.png" };
    char *png_name[] = { "Movie", "Music", "Picture", "eBook" };

    int media_label[] = { STR_MOVIE, STR_MUSIC, STR_PHOTO, STR_TEXT };

    /*Create a container with grid*/
    lv_obj_t *cont = lv_obj_create(Media_activity);
    lv_obj_set_size(cont, lv_pct(80), lv_pct(50));
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_center(cont);

    lv_obj_set_style_pad_column(cont, MEDIA_BTN_COL, 0);
    lv_obj_set_style_pad_all(cont, 10, 0);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);

    for (int i = 0; i < 4; i++) {
        lv_obj_t *media_obj = lv_pro_set_btn_style1_create(cont);
        lv_obj_set_size(media_obj, MEDIA_BTN_WIDTH, MEDIA_BTN_HEIGHT);
        lv_pro_set_btn_style1_src(media_obj, &GENERAL_FONT_BIG,
                            lv_color_hex(0xff8000), LV_OPA_COVER, png_path[i],
                            lv_get_string(media_label[i]));
        lv_obj_add_event_cb(media_obj, media_event_handler, LV_EVENT_KEY, png_name[i]);
        lv_obj_t *media_obj_btn = lv_pro_set_btn_style1_get_btn(media_obj);
        lv_group_add_obj(Media_group, media_obj_btn);
    }

    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_update_layout(Media_activity);
}

void lv_pro_media_ui_exit(void) {
    if (Media_group) {
        lv_group_remove_all_objs(Media_group);
        lv_group_del(Media_group);
        Media_group = NULL;
    }
    if (Media_activity) {
        lv_obj_del(Media_activity);
        Media_activity = NULL;
    }
}

static int create_media_ui(void)
{
    lv_pro_media_ui_init();
    load_current_channel(Media_activity, Media_group);

    return 0;
}

static int destory_media_ui(void)
{
    lv_pro_media_ui_exit();
    return 0;
}

static int show_media_ui(void)
{
    return 0;
}

static int hide_media_ui(void)
{
    return 0;
}

static page_interface_t page_media_ui =
{
    .ops =
    {
        create_media_ui,
        destory_media_ui,
        show_media_ui,
        hide_media_ui,
    },
    .info =
    {
        .id = PAGE_MEDIA,
        .user_data = NULL
    }
};

void REGISTER_PAGE_MEDIA(void)
{
    reg_page(&page_media_ui);
}


void media_disk_message_process(system_msg_t *msg)
{
    if (!msg) return;

    switch (msg->type) {
    case MSG_TYPE_USB_MOUNT:
        if (get_current_page_id() == PAGE_DISK) {
            lv_pro_disk_ui_fresh();
        }
        break;
    case MSG_TYPE_USB_UNMOUNT:
        if (DiskManager_GetDiskNum() != 0) {
            page_id_t cur_page_id = get_current_page_id();
            if (cur_page_id == PAGE_DISK) {
                lv_pro_disk_ui_fresh();
            } else if ((media_usb_index == msg->data) &&
                    (cur_page_id >= PAGE_FILE) && (cur_page_id <= PAGE_EBOOK)) {
                switch_page(PAGE_DISK);
            }
        }
        break;
    default:
        break;
    }
}
