#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "lvgl/lvgl.h"
#include <lv_common.h>
#include "../widget/lv_pro_res.h"
#include "ota_update.h"
#include "lv_string_id.h"
#include "ipc/progress_ipc.h"
#include "net/network_ota.h"
#include "lv_system_page.h"
#include "lv_pro_launcher.h"
#include "page.h"
#include <sys/syscall.h>
#include <pthread.h>

#define LOCAL_OTA_FILE  ("/tmp/USB0/update.swu")

static lv_obj_t *ota_page;
static lv_obj_t *ota_mbox;
uint8_t ab_system_path;

pid_t gettid() 
{
    return syscall(SYS_gettid);
}

static int check_ab_system(void)
{
    char command[512] = {0};
    FILE   *stream;

    stream = popen("fw_printenv boot_partition", "r");
    if(!stream) {
        return -1;
    }
    fgets(command, sizeof(command), stream);

    printf("check_ab_system: %s\n", command);
    if (strstr(command, "bootA")) {
        ab_system_path = USE_A_SYSTEM;
        pclose(stream);
        return 0;
    } else if (strstr(command, "bootB")) {
        ab_system_path = USE_B_SYSTEM;
        pclose(stream);
        return 0;
    }

    ab_system_path = 0;
    pclose(stream);
    return -1;
}

static void update_ab_system_info(void)
{
    if (ab_system_path == USE_A_SYSTEM) {
        printf("update AB system info: use B system later\n");
        system("fw_setenv boot_partition bootB");
        system("fw_setenv root_partition rootfsB");
    } else if (ab_system_path == USE_B_SYSTEM) {
        printf("update AB system info: use A system later\n");
        system("fw_setenv boot_partition bootA");
        system("fw_setenv root_partition rootfsA");
    }
}

void ota_set_progressbar(unsigned int progress)
{
	char label_str[32] = {0};

    sprintf(label_str, "%u ", progress);
    lv_slider_set_value(lv_obj_get_child(ota_page, 2), progress, LV_ANIM_OFF);
    lv_label_set_text(lv_obj_get_child(lv_obj_get_child(ota_page, 2), 0), label_str);
}

void ota_set_status(uint32_t string_id)
{
	lv_label_set_text(lv_obj_get_child(ota_page, 3), lv_get_string(string_id));
}

void ota_set_part_obj(uint8_t cur_step, uint8_t all_step)
{
    char str[32] = {0};
    sprintf(str, "(%d/%d)", cur_step, all_step);
    lv_label_set_text(lv_obj_get_child(ota_page, 4), str);
}

void ota_set_part_obj_str(char *str)
{
    if (strlen(str) > 0)
        lv_label_set_text(lv_obj_get_child(ota_page, 4), str);
}

int check_local_ota_file(void)
{
    if (access(LOCAL_OTA_FILE, F_OK) == 0) {
        return 0;
    }
    return -1;
}

static void *progress_update_burn_info(void *arg)
{
    int connfd = -1;
    struct progress_msg msg;
    unsigned int curstep = -1;
    unsigned int percent = -1;
    char command[64] = {0};

    pthread_detach(pthread_self());
    RECOVERY_STATUS status = IDLE;

    pid_t pid = gettid();
    printf("ota progress pid:%d\n", pid);
    while (1) {
        if (connfd < 0) {
            fprintf(stdout, "connect to swupdate\n\n");
            connfd = progress_ipc_connect(1);
        }

        if (progress_ipc_receive(&connfd, &msg) == -1) {
            continue;
        }

		//printf("(%d) %d of %d (%s)\n", msg.cur_step, msg.nsteps, msg.cur_percent, msg.cur_image);
        if (msg.cur_percent != percent) {
            percent = msg.cur_percent;
            ota_set_progressbar(msg.cur_percent);
        }

        if (msg.cur_step != curstep) {
            curstep = msg.cur_step;
            ota_set_part_obj(msg.cur_step, msg.nsteps);
        }

        switch (msg.status) {
        case SUCCESS:
            update_ab_system_info();
            printf("SUCCESS; wait for kill\n");
            while(1);
            ota_set_status(STR_NET_UPG_OK);
            //system("sync");
            break;

        case FAILURE:
            ota_set_status(STR_UPGRADE_ERR);
            break;

        case DONE:
            printf("DONE; wait for kill\n");
            while(1);
            //sleep(3);
            //system("reboot");
            break;

        default:
            break;
        }
        status = msg.status;
    }
    pthread_exit(NULL);
}

static void run_swupdate(int mode)
{
    int ret;
    pthread_t progress_pthread;
    char command[256] = {0};
    if (check_ab_system() == 0) {
        printf("OTA: Update AB system\n");
        if (!mode) {
            if (ab_system_path == USE_A_SYSTEM)
                sprintf(command, "/etc/do_ota.sh %s %s %s &", "local", LOCAL_OTA_FILE, "A");
            else
                sprintf(command, "/etc/do_ota.sh %s %s %s &", "local", LOCAL_OTA_FILE, "B");
        } else {
            if (ab_system_path == USE_A_SYSTEM)
                sprintf(command, "/etc/do_ota.sh %s %s %s &", "local", NET_OTA_FW_PATH, "A");
            else
                sprintf(command, "/etc/do_ota.sh %s %s %s &", "local", NET_OTA_FW_PATH, "B");
        }
    } else {
        printf("OTA: Update Non-AB system\n");
        if (!mode)
            sprintf(command, "/etc/do_ota.sh %s %s &", "local", LOCAL_OTA_FILE);
        else
            sprintf(command, "/etc/do_ota.sh %s %s &", "local", NET_OTA_FW_PATH);
    }
    system(command);

    ret = pthread_create(&progress_pthread, NULL, (void *)progress_update_burn_info, NULL);
	if (ret < 0) {
		printf("%s %d pthread_create error\n", __FILE__, __LINE__);
		return -1;
	}
	return 0;
}

static void *progress_update_download_info(void *arg)
{
    int step;
    char step_str[32] = {0};
    char time_str[32] = {0};
    uint8_t check_is_ok = 0;

    pthread_detach(pthread_self());

    while (1) {
        memset(step_str, 0, sizeof(step_str));
        memset(time_str, 0, sizeof(time_str));
        get_download_info(step_str, time_str);
        //printf("step_str %s, time_str %s\n", step_str, time_str);

        if (strlen(step_str)) {
            step = atoi(step_str);
            ota_set_progressbar(step);
        }

        if (strlen(time_str)) {
            ota_set_part_obj_str(time_str);
        }

        if (step >= 100) {
            if (!check_ota_md5(NET_OTA_FW_PATH, NET_OTA_CONFIG_PATH))
                check_is_ok = 1;
            break;
        }
        sleep(1);
    }
    if (!check_is_ok)
        ota_set_status(STR_UPG_SW_ILLGAL);
    else {
        update_ota_version();
        ota_set_status(STR_UPGRADE_NOT_POWER_OFF);
        run_swupdate(1);
    }
    pthread_exit(NULL);
}

static void run_download(void)
{
    int ret;
    int try_time = 10;
    pthread_t progress_pthread;

    network_download_ota();

    while (try_time > 0) {
        if (!check_local_ota_log())
            break;
        sleep(1);
        try_time--;
    }

    if (try_time <= 0) {
    }

    ret = pthread_create(&progress_pthread, NULL, (void *)progress_update_download_info, NULL);
	if (ret < 0) {
		printf("%s %d pthread_create error\n", __FILE__, __LINE__);
		return -1;
	}
}

void create_ota_ui(int mode)
{
    lv_obj_set_style_bg_color(lv_layer_top(), COLOR_BLACK, 0);
    lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_100, 0);

    ota_page = lv_obj_create(lv_layer_top());
    lv_obj_set_size(ota_page, LV_PCT(45), LV_PCT(45));
    lv_obj_set_style_bg_color(ota_page, lv_color_hex(0xc0c0c0), 0);
    lv_obj_set_style_bg_opa(ota_page, LV_OPA_100, 0);
    lv_obj_clear_flag(ota_page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(ota_page, 0, 0);
    lv_obj_set_style_outline_width(ota_page, 0, 0);
    lv_obj_center(ota_page);

    lv_obj_t * title_lable = lv_label_create(ota_page);
    lv_obj_set_style_text_font(title_lable, &GENERAL_FONT_BIG, 0);
    lv_obj_add_style(title_lable, &lv_pro_res.label_white, 0);
    lv_label_set_recolor(title_lable, true);
    lv_obj_align(title_lable, LV_ALIGN_TOP_MID, 0, 0);


    char *png_path[] = {"A:/usr/share/lv_projector/local_update.png",
                        "A:/usr/share/lv_projector/online_update.png"};
    lv_obj_t *img_obj = lv_img_create(ota_page);
    lv_obj_set_size(img_obj, 128, 128);
    lv_obj_align(img_obj, LV_ALIGN_CENTER, 0, -20);

    if (mode) {
        lv_label_set_text_fmt(title_lable, "#ffffff %s #", lv_get_string(STR_OAT_ONLINE_UPDATE));
        lv_img_set_src(img_obj, png_path[1]);
    } else {
        lv_label_set_text_fmt(title_lable, "#ffffff %s #", lv_get_string(STR_UPGRADE));
        lv_img_set_src(img_obj, png_path[0]);
    }

    lv_obj_t *slider_obj = lv_slider_create(ota_page);
    lv_obj_set_size(slider_obj, LV_PCT(65), LV_PCT(6));
    lv_slider_set_range(slider_obj, 0, 100);
    lv_slider_set_value(slider_obj, 0, LV_ANIM_OFF);
    lv_obj_align(slider_obj, LV_ALIGN_BOTTOM_MID, 0, -40);
    lv_obj_set_style_bg_color(slider_obj, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider_obj, lv_color_hex(0x00ffff), LV_PART_INDICATOR);

    lv_obj_t *value_obj = lv_label_create(slider_obj);
    lv_label_set_long_mode(value_obj, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_font(value_obj, &GENERAL_FONT_MID, 0);
    lv_obj_set_style_text_color(value_obj, lv_color_hex(0xffffff), 0);
    lv_obj_center(value_obj);

    lv_obj_t * status_lable = lv_label_create(ota_page);
    lv_obj_set_style_text_font(status_lable, &GENERAL_FONT_BIG, 0);
    lv_obj_add_style(status_lable, &lv_pro_res.label_white, 0);
    lv_label_set_recolor(status_lable, true);
    lv_label_set_text_fmt(status_lable, "#ffffff %s #", lv_get_string(STR_UPGRADE));
    lv_obj_align(status_lable, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_obj_t * part_lable = lv_label_create(ota_page);
    lv_obj_set_style_text_font(part_lable, &GENERAL_FONT_MID, 0);
    lv_obj_add_style(part_lable, &lv_pro_res.label_white, 0);
    lv_label_set_recolor(part_lable, true);
    lv_obj_align(part_lable, LV_ALIGN_BOTTOM_RIGHT, 0, -35);

    ota_set_progressbar(0);
    system("echo 3 > /proc/sys/vm/drop_caches");
    if (mode) {
        ota_set_part_obj_str("0");
        ota_set_status(STR_NET_UPG_DOWNLOAD);
        run_download();
    } else {
        ota_set_part_obj(0, 0);
        ota_set_status(STR_UPGRADE_NOT_POWER_OFF);
        run_swupdate(mode);
    }
}

static void net_ota_msgbox_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t *key = lv_event_get_param(e);
    lv_obj_t *cur_obj = lv_event_get_current_target(e);
    char *user_data = lv_event_get_user_data(e);

    if (code == LV_EVENT_KEY) {
        if (*key == LV_KEY_ENTER && user_data) {
            if(lv_msgbox_get_active_btn(cur_obj) == 0){
                disable_key_event();
                del_autosleep_timer();
                create_ota_ui(1);
            } else {
                lv_group_set_default(NULL);
                switch_page(PAGE_HOME);
            }
            lv_msgbox_close(ota_mbox);
            ota_mbox = NULL;
        } else if (*key == LV_KEY_BACK) {
            switch_page(PAGE_HOME);
            lv_msgbox_close(ota_mbox);
            ota_mbox = NULL;
        } else if (!is_global_key_go_new_channel(*key)) {
            lv_msgbox_close(ota_mbox);
            ota_mbox = NULL;
        }
    }
}

static void ota_msgbox_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t *key = lv_event_get_param(e);
    lv_obj_t *cur_obj = lv_event_get_current_target(e);
    char *user_data = lv_event_get_user_data(e);

    if (code == LV_EVENT_KEY) {
        if (*key == LV_KEY_ENTER && user_data) {
            if(lv_msgbox_get_active_btn(cur_obj) == 0){
                disable_key_event();
                del_autosleep_timer();
                create_ota_ui(0);
            } else {
                set_current_ui_group(Setting_system_group);
            }
            lv_msgbox_close(ota_mbox);
            ota_mbox = NULL;
        } else if (*key == LV_KEY_BACK) {
            set_current_ui_group(Setting_system_group);
            lv_msgbox_close(ota_mbox);
            ota_mbox = NULL;
        } else if (!is_global_key_go_new_channel(*key)) {
            lv_msgbox_close(ota_mbox);
            ota_mbox = NULL;
        }
    }
}

/*
*   id:
*   0: 本地升级
*   1：本地升级，但没有插入U盘
*   2：在线升级
*/
void create_ui_ota_msgbox(ota_mbox_type_t id)
{
    if (id == LOCAL_OTA || id == NETWORK_OTA) {
        static const char *btns[3];
        btns[0] = lv_get_string(STR_RESTORE_OK_1);
        btns[1] = lv_get_string(STR_RESTORE_CLOSE);
        btns[2] = "";
        lv_group_remove_all_objs(key_group);
        set_current_ui_group(key_group);
        ota_mbox = lv_msgbox_create(NULL, NULL, lv_get_string(STR_NETWORK_UPGRADE), btns, false);
        lv_obj_set_size(ota_mbox, LV_PCT(30), LV_PCT(30));
        if (id == LOCAL_OTA)
            lv_obj_add_event_cb(ota_mbox, ota_msgbox_event_cb, LV_EVENT_KEY, ota_mbox);
        else if (id == NETWORK_OTA)
            lv_obj_add_event_cb(ota_mbox, net_ota_msgbox_event_cb, LV_EVENT_KEY, ota_mbox);
    } else if (id == LOCAL_OTA_NO_USB) {
        static const char * btns[1];
        btns[0] = "no_use";
        lv_group_remove_all_objs(key_group);
        set_current_ui_group(key_group);
        ota_mbox = lv_msgbox_create(NULL, NULL, lv_get_string(STR_UPGRADE_NO_DEV), btns, false);
        lv_obj_set_size(ota_mbox, LV_PCT(20), LV_PCT(20));
        lv_obj_add_event_cb(ota_mbox, ota_msgbox_event_cb, LV_EVENT_KEY, NULL);
    }

    lv_event_send(lv_group_get_focused(key_group), LV_EVENT_FOCUSED, NULL);
    lv_obj_align(ota_mbox, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(ota_mbox, lv_color_hex(0xf0f0f0), 0);
    lv_obj_set_flex_align(ota_mbox, LV_FLEX_ALIGN_SPACE_AROUND , LV_FLEX_ALIGN_CENTER , LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(ota_mbox, 30, 0);
    lv_obj_set_scrollbar_mode(ota_mbox, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(ota_mbox, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *label = lv_msgbox_get_content(ota_mbox);
    lv_obj_set_size(label, LV_PCT(100), LV_PCT(35));
    lv_obj_set_style_text_font(label, &GENERAL_FONT_MID, 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0 , 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(label, lv_color_black(), 0);

    lv_obj_t *btns_obj = lv_msgbox_get_btns(ota_mbox);
    if (id == LOCAL_OTA || id == NETWORK_OTA) {
        lv_obj_set_size(btns_obj, LV_PCT(80), LV_PCT(20));
        lv_obj_set_style_text_font(btns_obj, &GENERAL_FONT_MID, 0);
        lv_obj_set_style_pad_gap(btns_obj, 60, 0);
        lv_obj_set_style_shadow_width(btns_obj, 0, LV_PART_ITEMS | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(btns_obj, 5, LV_PART_ITEMS | LV_STATE_DEFAULT);
    } else if (id == LOCAL_OTA_NO_USB) {
        lv_obj_add_flag(btns_obj, LV_OBJ_FLAG_HIDDEN);
    }

    lv_obj_t *bg = lv_obj_get_parent(ota_mbox);
    lv_obj_set_style_bg_opa(bg, LV_OPA_70, 0);
    lv_obj_set_style_bg_color(bg, lv_palette_main(LV_PALETTE_GREY), 0);

    lv_group_focus_obj(lv_msgbox_get_btns(ota_mbox));
}

static int create_network_ota(void)
{
    create_ui_ota_msgbox(NETWORK_OTA);
    return 0;
}

static int destory_network_ota(void)
{
    if (ota_mbox) {
        lv_msgbox_close(ota_mbox);
        ota_mbox = NULL;
    }
    return 0;
}

static int show_network_ota(void)
{
    return 0;
}

static int hide_network_ota(void)
{
    return 0;
}

static page_interface_t page_network_ota =
{
    .ops =
    {
        create_network_ota,
        destory_network_ota,
        show_network_ota,
        hide_network_ota,
    },
    .info =
    {
        .id = PAGE_NETWORK_OTA,
        .user_data = NULL
    }
};

void REGISTER_PAGE_NETWORK_OTA(void)
{
    reg_page(&page_network_ota);
}

static int create_local_ota(void)
{
    create_ui_ota_msgbox(LOCAL_OTA);
    return 0;
}

static int destory_local_ota(void)
{
    return 0;
}

static int show_local_ota(void)
{
    return 0;
}

static int hide_local_ota(void)
{
    return 0;
}

static page_interface_t page_local_ota =
{
    .ops =
    {
        create_local_ota,
        destory_local_ota,
        show_local_ota,
        hide_local_ota,
    },
    .info =
    {
        .id = PAGE_LOCAL_OTA,
        .user_data = NULL
    }
};

void REGISTER_PAGE_LOCAL_OTA(void)
{
    reg_page(&page_local_ota);
}
