#include <stdlib.h>
#include "../widget/lv_pro_res.h"
#include "lv_pro_launcher.h"
#include "lv_pro_source.h"
#include "lv_common.h"
#include "Common/language/string/lv_string_id.h"
#include "lv_pro_set_btn_style1.h"
#include "page.h"
#include "key_event.h"

#define min(a, b) ((a)<(b) ? (a) : (b))
#define max(a, b) ((a)<(b) ? (b) : (a))

lv_obj_t *SignalIn_activity;
lv_group_t *SignalIn_group;
lv_obj_t *no_signal_cont;
lv_obj_t *source_menu;

bool s_signalIn_flag;
static lv_timer_t *timer;
static bool up_dir = true;
static bool left_dir = true;


#if ENABLE_TEST_HDMI
static lv_timer_t *hdmi_test_timer = NULL;
#endif

static int random_in_range(int low, int high) {
    return low + random() % (high - low + 1);
}

lv_obj_t *g_signalin_info_box;
static lv_timer_t *signalin_info_show_time;

static void SignalinInfo_Timer(lv_timer_t *timer)
{
    lv_obj_add_flag(g_signalin_info_box,LV_OBJ_FLAG_HIDDEN);
    lv_timer_pause(signalin_info_show_time);
}

void hdmi_show_video(void)
{
    if (no_signal_cont)
        lv_obj_add_flag(no_signal_cont,LV_OBJ_FLAG_HIDDEN);

    if (g_signalin_info_box)
        lv_obj_clear_flag(g_signalin_info_box,LV_OBJ_FLAG_HIDDEN);

    lv_obj_invalidate(lv_scr_act());
    s_signalIn_flag = 1;
}

void hdmi_no_signal(void)
{
    s_signalIn_flag = 0;
    if (g_signalin_info_box) {
        lv_obj_add_flag(g_signalin_info_box,LV_OBJ_FLAG_HIDDEN);
        BeginSingalInfo_Timer();
    }

    if (no_signal_cont)
        lv_obj_clear_flag(no_signal_cont,LV_OBJ_FLAG_HIDDEN);

    lv_obj_invalidate(lv_scr_act());
}

void hdmi_update_info(int width, int height, int b_interlace, int frame_rate)
{
    if (g_signalin_info_box) {
        s_signalIn_flag = 1;
        if (no_signal_cont)
            lv_obj_add_flag(no_signal_cont,LV_OBJ_FLAG_HIDDEN);

        lv_obj_clear_flag(g_signalin_info_box,LV_OBJ_FLAG_HIDDEN);
        lv_obj_invalidate(lv_scr_act());
        lv_label_set_text_fmt(lv_obj_get_child(g_signalin_info_box, NULL),"%dx%d%c@%dHz",
            width, height, b_interlace ? 'I' : 'P', frame_rate);

       BeginSingalInfo_Timer();
   }
}

void HdmiInfo_init(void)
{
    hdmi_select_source(hdmi_get_source());

    if (g_signalin_info_box != NULL) {
        lv_obj_del(g_signalin_info_box);
        g_signalin_info_box = NULL;
    }

    g_signalin_info_box = lv_obj_create(SignalIn_activity);
    lv_obj_set_style_bg_color(g_signalin_info_box, lv_color_hex(0x0000FF), 0);
    lv_obj_t *label = lv_label_create(g_signalin_info_box);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF),0);
    lv_obj_set_style_text_font(label, &GENERAL_FONT_MID, 0);
    lv_obj_center(label);
    lv_obj_set_size(g_signalin_info_box, LV_PCT(20), LV_PCT(15));
    lv_obj_set_pos(g_signalin_info_box, LV_PCT(3), LV_PCT(3));
    lv_label_set_text_fmt(label, "%s%d", "HDMI", hdmi_get_source_to_map());
    signalin_info_show_time = lv_timer_create(SignalinInfo_Timer,3000,NULL);
    BeginSingalInfo_Timer();
}

void HdmiInfo_deinit(void)
{
    if (signalin_info_show_time) {
        lv_timer_del(signalin_info_show_time);
        signalin_info_show_time = NULL;
    }

    if (g_signalin_info_box) {
        lv_obj_t *label = lv_obj_get_child(g_signalin_info_box, NULL);

        if (label)
            lv_obj_del(label);
        lv_obj_del(g_signalin_info_box);
        g_signalin_info_box = NULL;
    }
}

void BeginSingalInfo_Timer(void)
{
    lv_timer_reset(signalin_info_show_time);
    lv_timer_resume(signalin_info_show_time);
}

static void no_signal_timer(lv_timer_t *timer) {
    if (s_signalIn_flag == 0) {
        lv_obj_clear_flag(no_signal_cont,LV_OBJ_FLAG_HIDDEN);
        if (no_signal_cont) {
            lv_coord_t p_h = lv_obj_get_x(no_signal_cont);
            lv_coord_t p_v = lv_obj_get_y(no_signal_cont);
            if(up_dir) {
                p_v = max(0, p_v-20);
                if(p_v == 0) {
                    up_dir = false;
                }
            } else {
                p_v = min((lv_disp_get_ver_res(lv_disp_get_default()))/6*5-100, p_v+20);
                if(p_v == ((lv_disp_get_ver_res(lv_disp_get_default()))/6*5 - 100)) {
                    up_dir = true;
                }
            }
            if(left_dir) {
                p_h = max(0, p_h - 30);
                if(p_h == 0){
                    left_dir = false;
                }
            } else {
                p_h = min(lv_disp_get_hor_res(lv_disp_get_default())/5*4, p_h + 30);
                if(p_h == lv_disp_get_hor_res(lv_disp_get_default())/5*4) {
                    left_dir = true;
                }
            }
            lv_obj_set_pos(no_signal_cont, p_h, p_v);
            lv_obj_invalidate(lv_scr_act());
        }
    }
    else
    {
        lv_obj_add_flag(no_signal_cont,LV_OBJ_FLAG_HIDDEN);  /*hidden no_signal*/
        lv_obj_invalidate(lv_scr_act());
    }
}

static void signalin_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t *key = lv_event_get_param(e);
    struct _lv_obj_t * cur_obj = lv_event_get_target(e);

    if (code == LV_EVENT_KEY) {
        if (*key == LV_KEY_BACK) {
            switch_page(PAGE_HOME);
        } else if (*key == U_KEY_SOURCE) {
#if (HDMI_MAX_PORT_NUM > 1)
            creat_source_menu();
#endif
        } else if (*key == LV_KEY_UP || *key == LV_KEY_DOWN ||
                   *key == LV_KEY_LEFT || *key == LV_KEY_RIGHT ||
                   *key == LV_KEY_ENTER) {
            hdmi_cec_send_key(*key);
        }
    }
}

void lv_pro_signalin_init(void) {

    SignalIn_activity = lv_obj_create(NULL);
    SignalIn_group = lv_group_create();

    lv_obj_set_size(SignalIn_activity, lv_pct(100), lv_pct(100));
    lv_obj_add_style(SignalIn_activity, &lv_pro_res.set_bg_black, 0);
    lv_obj_clear_flag(SignalIn_activity, LV_OBJ_FLAG_SCROLLABLE);
    lv_disp_get_default()->driver->screen_transp = 1;
    lv_disp_set_bg_opa(lv_disp_get_default(), LV_OPA_TRANSP);
    /* Empty the buffer, not emptying will cause the UI to be opaque */
    lv_memset_00(lv_disp_get_default()->driver->draw_buf->buf_act,
                lv_disp_get_default()->driver->draw_buf->size * sizeof(lv_color32_t));
    lv_obj_set_style_bg_opa(SignalIn_activity, LV_OPA_TRANSP, 0);
}

void lv_pro_signalin_exit(void)
{
    if (!SignalIn_activity)
        return;

    lv_group_remove_all_objs(SignalIn_group);
    lv_obj_del(SignalIn_activity);
    lv_group_del(SignalIn_group);
    SignalIn_group = NULL;
    SignalIn_activity = NULL;
}

void source_menu_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t *key = lv_event_get_param(e);
    struct _lv_obj_t * cur_obj = lv_event_get_target(e);

    if (code == LV_EVENT_KEY) {
        if (*key == LV_KEY_BACK) {
            destory_source_menu();
            set_current_ui_group(SignalIn_group);
        } else if (*key == LV_KEY_ENTER) {
            int id = lv_group_get_obj_id(key_group, cur_obj);
            if (id == 0) {
                #if CONFIG_PACKAGE_libhdmiapi
                hdmi_select_source(kSourceId_HDMI_1);
                #endif
            } else if (id == 1) {
                #if CONFIG_PACKAGE_libhdmiapi
                hdmi_select_source(kSourceId_HDMI_2);
                #endif
            }

            destory_source_menu();
            set_current_ui_group(SignalIn_group);
        }
    }
}

void destory_source_menu(void)
{
    if (source_menu) {
        lv_group_remove_all_objs(key_group);
        lv_obj_del(source_menu);
        source_menu = NULL;
    }
}

void creat_source_menu(void)
{
    char *png_path[] = { "A:/usr/share/lv_projector/hdmi.png"};
    lv_pro_set_btn_style1_t *btn;

    if (source_menu == NULL)
        source_menu = lv_obj_create(SignalIn_activity);

    lv_obj_set_size(source_menu, lv_pct(40), lv_pct(28));
    lv_obj_set_style_bg_color(source_menu, lv_color_hex(0xababab), 0);
    lv_obj_set_style_bg_opa(source_menu, LV_OPA_80, 0);
    lv_obj_clear_flag(source_menu, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_align(source_menu, LV_ALIGN_BOTTOM_MID);

    lv_group_remove_all_objs(key_group);

    lv_obj_t *obj1 = lv_pro_set_btn_style1_create(source_menu);
    lv_obj_set_size(obj1, lv_pct(30), lv_pct(100));
    lv_obj_set_align(obj1, LV_ALIGN_LEFT_MID);
    lv_pro_set_btn_style1_src(obj1, &GENERAL_FONT_MID, lv_color_hex(0xffffff),
                        LV_OPA_TRANSP, png_path[0], "HDMI1");
    btn = (lv_pro_set_btn_style1_t *)obj1;
    lv_obj_align_to(btn->name, btn->img, LV_ALIGN_OUT_BOTTOM_MID, 0, -5);
    lv_group_add_obj(key_group, lv_pro_set_btn_style1_get_btn(obj1));
    lv_obj_add_event_cb(obj1, source_menu_event_handler, LV_EVENT_KEY, NULL);

    lv_obj_t *obj2 = lv_pro_set_btn_style1_create(source_menu);
    lv_obj_set_size(obj2, lv_pct(30), lv_pct(100));
    lv_obj_align_to(obj2, obj1, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    lv_pro_set_btn_style1_src(obj2, &GENERAL_FONT_MID, lv_color_hex(0xffffff),
                        LV_OPA_TRANSP, png_path[0], "HDMI2");
    btn = (lv_pro_set_btn_style1_t *)obj2;
    lv_obj_align_to(btn->name, btn->img, LV_ALIGN_OUT_BOTTOM_MID, 0, -5);

    lv_group_add_obj(key_group, lv_pro_set_btn_style1_get_btn(obj2));
    lv_obj_add_event_cb(obj2, source_menu_event_handler, LV_EVENT_KEY, NULL);

    lv_group_focus_obj_for_id(lv_group_get_head_obj_ex(key_group), (hdmi_get_source() - 1), true);
    set_current_ui_group(key_group);
}

void no_signal_cont_init(void)
{
    HdmiInfo_init();
    if (no_signal_cont != NULL) {
        lv_obj_del(no_signal_cont);
        no_signal_cont = NULL;
    }
    no_signal_cont = lv_obj_create(SignalIn_activity);
    lv_obj_set_size(no_signal_cont, 240, 100);
    lv_obj_set_style_border_width(no_signal_cont, 0, 0);
    lv_obj_set_style_radius(no_signal_cont, 0, 0);
    lv_obj_set_style_bg_color(no_signal_cont, lv_color_hex(0xe0e0e0), 0);
    lv_obj_set_style_bg_opa(no_signal_cont, LV_OPA_70, 0);
    lv_obj_clear_flag(no_signal_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *label = lv_label_create(no_signal_cont);
    lv_obj_set_style_text_font(label, &GENERAL_FONT_MID, 0);
    lv_label_set_recolor(label, true);
    lv_label_set_text_fmt(label, "#0x404040 %s #", lv_get_string(STR_NO_SIGNAL));
    lv_obj_center(label);
    timer = lv_timer_create(no_signal_timer, 2000, NULL);
    lv_obj_add_event_cb(label, signalin_event_handler, LV_EVENT_KEY, NULL);
    lv_group_add_obj(SignalIn_group, label);
    lv_obj_update_layout(SignalIn_activity);
    lv_obj_add_flag(no_signal_cont, LV_OBJ_FLAG_HIDDEN);
}

void no_signal_cont_deinit(void)
{
    if (timer) {
        lv_timer_del(timer);
        timer = NULL;
    }

    if (no_signal_cont) {
        lv_obj_t *label = lv_obj_get_child(no_signal_cont, NULL);

        if (label) {
            lv_obj_remove_event_cb(label, signalin_event_handler);
            lv_obj_del(label);
        }
        lv_obj_del(no_signal_cont);
        no_signal_cont = NULL;
    }
    HdmiInfo_deinit();
}

#if ENABLE_TEST_HDMI
static void hdmi_test_timer_handle(lv_timer_t *timer_)
{
    static uint32_t times = 0;
    printf("HDMI Test times: %d\n", times);
    if ((times % 2) == 0) {
        hdmi_select_source(kSourceId_HDMI_1);
    } else{
        hdmi_select_source(kSourceId_HDMI_2);
    }
    times = times + 1;
    lv_timer_reset(hdmi_test_timer);
}
#endif

static int create_signalin(void)
{
    no_signal_cont_init();
    lv_obj_set_parent(SignalIn_activity, launcher_activity);
    lv_obj_clear_flag(SignalIn_activity,LV_OBJ_FLAG_HIDDEN);
    load_current_channel(SignalIn_activity, SignalIn_group);
    lv_group_focus_obj(lv_group_get_head_obj_ex(SignalIn_group));

    //lv_memset_00 draw buf,prevent last show
    lv_disp_get_default()->driver->screen_transp = 1;
    lv_disp_set_bg_opa(lv_disp_get_default(), LV_OPA_TRANSP);
    lv_memset_00(lv_disp_get_default()->driver->draw_buf->buf_act,
                lv_disp_get_default()->driver->draw_buf->size * sizeof(lv_color32_t));
    lv_obj_set_style_bg_opa(SignalIn_activity, LV_OPA_TRANSP, 0);

#if ENABLE_TEST_HDMI
    if (!hdmi_test_timer)
        hdmi_test_timer = lv_timer_create(hdmi_test_timer_handle, 8 * 1000, NULL);
    lv_timer_set_repeat_count(hdmi_test_timer, -1);
#endif
    return 0;
}

static int destory_signalin(void)
{
    //lv_pro_signalin_exit();
    lv_obj_add_flag(SignalIn_activity, LV_OBJ_FLAG_HIDDEN);
#if ENABLE_HDMIRX
    hdmirx_app_select_source(kSourceId_VideoDec);
#endif
    no_signal_cont_deinit();
    destory_source_menu();

    lv_disp_get_default()->driver->screen_transp = 0;
    lv_disp_set_bg_opa(lv_disp_get_default(), LV_OPA_COVER);
    lv_memset_00(lv_disp_get_default()->driver->draw_buf->buf_act,
                 lv_disp_get_default()->driver->draw_buf->size * sizeof(lv_color32_t));

#if ENABLE_TEST_HDMI
    if (hdmi_test_timer) {
        lv_timer_del(hdmi_test_timer);
        hdmi_test_timer = NULL;
    }
#endif
    return 0;
}

static int show_signalin(void)
{
    lv_obj_clear_flag(SignalIn_activity, LV_OBJ_FLAG_HIDDEN);
    set_current_ui_group(SignalIn_group);
    return 0;
}

static int hide_signalin(void)
{
    lv_obj_add_flag(SignalIn_activity, LV_OBJ_FLAG_HIDDEN);
    destory_source_menu();
    return 0;
}

static page_interface_t page_signalin =
{
    .ops =
    {
        create_signalin,
        destory_signalin,
        show_signalin,
        hide_signalin,
    },
    .info =
    {
        .id = PAGE_SOURCE,
        .user_data = NULL
    }
};

void REGISTER_PAGE_SOURCE(void)
{
    reg_page(&page_signalin);
}
