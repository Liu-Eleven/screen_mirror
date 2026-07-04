/*
 * lv_pro_ebook_activity.c
 *
 *  Created on: 2024/12/10
 *      Author: hongjiasen
 */

#include "lv_pro_ebook_activity.h"

#include "lv_pro_ebook_list_activity.h"
#include "../middle_ware/lv_pro_res_ebook_player_int.h"
#define BARBTN_ITEM 7

extern lv_obj_t *File_activity;
extern lv_group_t *File_right_group;

lv_obj_t *lv_pro_ebook_activity;
lv_group_t *lv_pro_ebook_group;
lv_obj_t *ui_ebook_label_page;
lv_obj_t *ui_ebook_label;
lv_obj_t *ebook_label;

static lv_obj_t *ctrlbar_obj = NULL;
static lv_obj_t *ctrlbarbtn[BARBTN_ITEM] = {NULL};
static lv_timer_t *ctrlbar_show_timer = NULL;

static char curPath[FILE_PATH_MAXT_LEN];
static char curFd[FILE_NAME_MAXT_LEN];

static bool playBarIsShow = false;

static void show_player_ctrlbar(bool show);

static void ctrlbar_show_timer_cb(lv_timer_t *timer);
static void ebook_keyinput_event_cb(lv_event_t *e);
static void ebook_activity_event_handler(lv_event_t *e);
static int lv_pro_ebook_quit(void);

static void show_player_ctrlbar(bool show)
{
    EBOOK_DBG("is show:%d\n", show);
    if (show) {
        lv_obj_clear_flag(ctrlbar_obj, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(ctrlbar_obj, LV_OBJ_FLAG_HIDDEN);
    }
    playBarIsShow = show;
}

void lv_pro_ebook_ctrlbar_timer_en(bool en)
{
    // EBOOK_DBG("is timer en:%d\n", en);
    if (en) {
        show_player_ctrlbar(true);
        lv_timer_reset(ctrlbar_show_timer);
        lv_timer_resume(ctrlbar_show_timer);
    } else {
        lv_timer_pause(ctrlbar_show_timer);
    }
}

static void ctrlbar_show_timer_cb(lv_timer_t *timer)
{
    show_player_ctrlbar(false);
    lv_pro_ebook_ctrlbar_timer_en(false);
}

static void ebook_keyinput_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    const uint32_t *key_param = lv_event_get_param(e);
    uint32_t key = (key_param != NULL) ? *key_param : 0;
    char *user_data = (char *)lv_event_get_user_data(e);

    if (code == LV_EVENT_KEY) {
        if (!is_global_key_go_new_channel(key) || key == LV_KEY_BACK) {
            switch_page_show_destory(PAGE_FILE);
            return;
        }

        if (!playBarIsShow) {
            if (key == LV_KEY_UP || key == LV_KEY_DOWN)
                lv_pro_ebook_res_change_ebook_txt_info(key, 0);
            else
                lv_pro_ebook_ctrlbar_timer_en(true);
            return;
        }

        if (key == LV_KEY_ENTER) {
            int ret = 0;
            if (!strcmp(user_data, "up")) {
                lv_pro_ebook_res_change_ebook_txt_info(LV_KEY_UP, 0);
            } else if (!strcmp(user_data, "down")) {
                lv_pro_ebook_res_change_ebook_txt_info(LV_KEY_DOWN, 0);
            } else if (!strcmp(user_data, "pre")) {
                // lv_pro_ebook_res_play_mode(0, 0);
                ret = lv_pro_ebook_res_trySwitch_ebook(0, 0);
                if (ret == -1)
                    lv_pro_msgbox_msg_open_on_top(&IDB_Icon_unsupported,
                                                  lv_get_string(STR_FILE_FAIL), 500);
            } else if (!strcmp(user_data, "next")) {
                // lv_pro_ebook_res_play_mode(1, 0);
                ret = lv_pro_ebook_res_trySwitch_ebook(1, 0);
                if (ret == -1)
                    lv_pro_msgbox_msg_open_on_top(&IDB_Icon_unsupported,
                                                  lv_get_string(STR_FILE_FAIL), 500);
            } else if (!strcmp(user_data, "stop")) {
                switch_page_show_destory(PAGE_FILE);
                return;
            } else if (!strcmp(user_data, "info")) {
                lv_pro_ebook_ctrlbar_timer_en(false);
                lv_pro_ebook_info_init();
            } else if (!strcmp(user_data, "list")) {
                lv_pro_ebook_ctrlbar_timer_en(false);
                lv_pro_ebook_list_init();
            }
        }
    }
    lv_timer_reset(ctrlbar_show_timer);
}

int lv_pro_ebook_init(char *cur_path, char *fn)
{
    if (lv_pro_ebook_activity) {
        EBOOK_ERR("ebook activity already init!\n");
        return -1;
    }

    // 主框
    EBOOK_DBG("\n");

    if (curPath != cur_path) {
        memset(curPath, '\0', sizeof(curPath));
        snprintf(curPath, sizeof(curPath), "%s", cur_path);
    }

    if (curFd != fn) {
        memset(curFd, '\0', sizeof(curFd));
        snprintf(curFd, sizeof(curFd), "%s", fn);
    }

    lv_pro_ebook_activity = lv_obj_create(NULL);
    lv_obj_set_size(lv_pro_ebook_activity, lv_pct(100), lv_pct(100));
    lv_obj_add_style(lv_pro_ebook_activity, &lv_pro_res.set_bg_black, 0);

    lv_pro_ebook_group = lv_group_create();

    // 文字框
    ui_ebook_label = lv_label_create(lv_pro_ebook_activity);
    lv_obj_set_size(ui_ebook_label, lv_pct(100), lv_pct(93));
    lv_obj_align(ui_ebook_label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_long_mode(ui_ebook_label, LV_LABEL_LONG_WRAP);
    lv_label_set_recolor(ui_ebook_label, true);
    lv_obj_set_style_text_color(ui_ebook_label, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_pad_hor(ui_ebook_label, EBOOK_LAB_PADHOR, 0);
    lv_obj_set_style_text_font(ui_ebook_label, &GENERAL_FONT_MID, 0);
    lv_obj_set_style_text_letter_space(ui_ebook_label, 1, 0);
    lv_obj_set_style_text_line_space(ui_ebook_label, EBOOK_LINE_SPACE, 0);
    lv_label_set_text(ui_ebook_label, "");

    // 页面信息
    ui_ebook_label_page = lv_label_create(lv_pro_ebook_activity);
    lv_obj_set_size(ui_ebook_label_page, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align_to(ui_ebook_label_page, ui_ebook_label, LV_ALIGN_OUT_BOTTOM_RIGHT,
                    EBOOK_PAGELAB_PADHOR, 0);
    lv_label_set_long_mode(ui_ebook_label_page, LV_LABEL_LONG_WRAP);
    lv_label_set_recolor(ui_ebook_label_page, true);
    lv_obj_set_style_text_color(ui_ebook_label_page, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(ui_ebook_label_page, &GENERAL_FONT_NORMAL, 0);
    lv_label_set_text(ui_ebook_label_page, "");

    // 按钮框
    // add a ctrl bar win
    ctrlbar_obj = lv_obj_create(lv_pro_ebook_activity);
    lv_obj_set_size(ctrlbar_obj, lv_pct(100), lv_pct(26));
    lv_obj_align(ctrlbar_obj, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_style(ctrlbar_obj, &lv_pro_res.set_bg_black, 0);
    lv_obj_set_style_bg_opa(ctrlbar_obj, LV_OPA_70, 0);

    lv_obj_t *btn_cont = lv_obj_create(ctrlbar_obj);
    lv_obj_set_size(btn_cont, lv_pct(90), lv_pct(62));
    lv_obj_align(btn_cont, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_style(btn_cont, &lv_pro_res.set_bg_transp, 0);

    lv_obj_set_style_pad_column(btn_cont, CTRLBAR_BTN_COL, 0);
    lv_obj_set_style_pad_all(btn_cont, 5, 0);
    lv_obj_set_scrollbar_mode(btn_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(btn_cont, LV_FLEX_FLOW_ROW);

    /*Create image button*/
    const lv_img_dsc_t *icon_path[BARBTN_ITEM] = {
        &player_backward, &player_forward, &player_back, &player_next,
        &player_stop,     &player_info,    &player_list};

    int imgbtn_label[BARBTN_ITEM] = {STR_EBOOK_UP,   STR_EBOOK_DOWN, STR_EBOOK_PREV,
                                     STR_EBOOK_NEXT, STR_STOP,       STR_INFO,
                                     STR_LIST};

    char *imgbtn_name[BARBTN_ITEM] = {"up", "down", "pre", "next", "stop", "info", "list"};

    for (int i = 0; i < BARBTN_ITEM; i++) {
        ctrlbarbtn[i] = lv_pro_ctrlbar_imgbtn_create(btn_cont);
        lv_obj_set_size(ctrlbarbtn[i], CTRLBAR_BTN_WIDTH, CTRLBAR_BTN_HEIGHT);
        lv_pro_ctrlbar_imgbtn_set_src(ctrlbarbtn[i], icon_path[i],
                                      lv_get_string(imgbtn_label[i]), &GENERAL_FONT_MID,
                                      &lv_pro_res.set_bg_transp);
        lv_obj_t *ctrlbar_imgbtn_btn = lv_pro_ctrlbar_imgbtn_get_btn(ctrlbarbtn[i]);
        lv_obj_add_event_cb(ctrlbar_imgbtn_btn, ebook_keyinput_event_cb, LV_EVENT_KEY,
                            imgbtn_name[i]);
        lv_group_add_obj(lv_pro_ebook_group, ctrlbar_imgbtn_btn);
    }

    lv_obj_t *label_cont = lv_obj_create(ctrlbar_obj);
    lv_obj_set_size(label_cont, lv_pct(90), lv_pct(38));
    lv_obj_add_style(label_cont, &lv_pro_res.set_bg_transp, 0);
    lv_obj_align_to(label_cont, btn_cont, LV_ALIGN_OUT_TOP_MID, 0, 0);

    char ebookPath[512];
    lv_pro_ebook_res_list_init(curPath, curFd);
    snprintf(ebookPath, sizeof(ebookPath), "%s/%s", curPath, curFd);
    int ret = lv_pro_ebook_res_open(ebookPath);

    ebook_label = lv_label_create(label_cont);
    lv_label_set_long_mode(ebook_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(ebook_label, lv_pct(40));
    lv_label_set_recolor(ebook_label, true);
    lv_obj_set_style_text_font(ebook_label, &GENERAL_FONT_MID, 0);
    if (curFd) {
        lv_label_set_text_fmt(ebook_label, "%s", curFd);
    } else {
        lv_label_set_text_fmt(ebook_label, "%s", "Not Ebook Play");
    }
    lv_obj_set_style_text_color(ebook_label, lv_color_white(), 0);
    lv_obj_align(ebook_label, LV_ALIGN_TOP_LEFT, lv_pct(10), STATE_ALIGN);

    if (ret != 0) {
        EBOOK_ERR("ebook open fail!\n");
        lv_pro_ebook_quit();
        return -1;
    }

    // group will fouce on last obj,so disable scroll bar
    set_current_ui_group(lv_pro_ebook_group);
    lv_group_focus_obj(ctrlbarbtn[0]);

    show_player_ctrlbar(true);
    ctrlbar_show_timer = lv_timer_create(ctrlbar_show_timer_cb, 5000, NULL);

    lv_obj_update_layout(lv_pro_ebook_activity);

    EBOOK_DBG("finish!\n");
    return 0;
}

static int lv_pro_ebook_quit()
{
    lv_pro_ebook_res_close();
    lv_pro_ebook_res_list_deinit();
    lv_pro_ebook_deinit();
}

int lv_pro_ebook_deinit()
{
    EBOOK_DBG("\n");
    if (ctrlbar_show_timer) lv_timer_del(ctrlbar_show_timer);

    if (lv_pro_ebook_group) {
        lv_group_remove_all_objs(lv_pro_ebook_group);
        lv_group_del(lv_pro_ebook_group);
    }
    if (lv_pro_ebook_activity) lv_obj_del_async(lv_pro_ebook_activity);

    for (int i = 0; i < BARBTN_ITEM; i++) {
        ctrlbarbtn[i] = NULL;
    }
    ctrlbar_show_timer = NULL;
    ctrlbar_obj = NULL;
    ui_ebook_label_page = NULL;
    ui_ebook_label = NULL;
    lv_pro_ebook_activity = NULL;
    lv_pro_ebook_group = NULL;
}

static int create_ebook_ui(void)
{
    EBOOK_DBG("\n");
    lv_obj_set_parent(lv_pro_ebook_activity, File_activity);
    load_current_channel(lv_pro_ebook_activity, lv_pro_ebook_group);

    return 0;
}

static int destory_ebook_ui(void)
{
    EBOOK_DBG("\n");
    lv_pro_ebook_quit();
    return 0;
}

/*When entering the menu page, hide is triggered, and when the menu page returns, show is triggered.
 * Resources need to be released and reloaded */
static int show_ebook_ui(void)
{
    EBOOK_DBG("\n");
    lv_pro_ebook_init(curPath, curFd);
    create_ebook_ui();
    return 0;
}

static int hide_ebook_ui(void)
{
    EBOOK_DBG("\n");
    lv_pro_ebook_quit();
    return 0;
}

static page_interface_t page_ebook_ui = {.ops =
                                             {
                                                 create_ebook_ui,
                                                 destory_ebook_ui,
                                                 show_ebook_ui,
                                                 hide_ebook_ui,
                                             },
                                         .info = {.id = PAGE_EBOOK, .user_data = NULL}};

void REGISTER_PAGE_EBOOK(void) { reg_page(&page_ebook_ui); }
