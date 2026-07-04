#include "lvgl/lvgl.h"
#include "lv_drivers/display/sunxifb.h"
#include "lv_drivers/indev/evdev.h"
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "lv_pro_launcher.h"
#include "widget/lv_pro_res.h"
#include "System/system_api.h"
#include "sys_param.h"
#include "lv_common.h"
#include "awcast.h"
#include <signal.h>
#include "page.h"
#include <ucontext.h>
#include <dlfcn.h>

lv_indev_t *evdev_indev;
lv_indev_drv_t indev_drv;

static void disable_bootlogo(void)
{
    system("./usr/bin/kill_yuview.sh");
}

static void keypad_int()
{
    evdev_init();
    lv_indev_drv_init(&indev_drv); /*Basic initialization*/
    indev_drv.type = LV_INDEV_TYPE_KEYPAD; /*See below.*/
    indev_drv.read_cb = evdev_read_ex; /*See below.*/
    /*Register the driver in LVGL and save the created input device object*/
    evdev_indev = lv_indev_drv_register(&indev_drv);

    register_global_key(do_global_event);
}

void print_stack(ucontext_t *uc) {
    if (!uc) {
        printf("No valid context provided.\n");
        return;
    }
    /*
    // Extract program counter (PC) and stack pointer (SP) from context
    void *pc = (void *)uc->uc_mcontext.__gregs[REG_PC];
    void *sp = (void *)uc->uc_mcontext.__gregs[REG_SP];

    printf("Program counter: %p\n", pc);
    printf("Stack pointer: %p\n", sp);

    // Traverse the stack and print addresses
    void **stack = (void **)sp;
    for (int i = 0; i < 10 && stack; i++) {
        Dl_info info;
        void *addr = stack[i];

        // Resolve symbol and library info
        if (dladdr(addr, &info)) {
            printf("Frame[%d]: Address: %p, Symbol: %s, Library: %s, Offset: %p\n",
                    i,
                    addr,
                    info.dli_sname ? info.dli_sname : "Unknown",
                    info.dli_fname ? info.dli_fname : "Unknown",
                    (void *)((uintptr_t)addr - (uintptr_t)info.dli_fbase));
        } else {
            printf("Frame[%d]: Address: %p, Unknown symbol\n", i, addr);
        }
    }
    */
}


void signal_handler(int sig, siginfo_t *si, void *arg) {
    ucontext_t *uc = (ucontext_t *)arg;
    printf("Caught signal %d\n", sig);
    print_stack(uc);
    exit(1);
}

static void install_sig_handler(void)
{
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = signal_handler;
    sigemptyset(&sa.sa_mask);

    int signals[] = {
        SIGBUS,
        SIGFPE,
        SIGHUP,
        SIGILL,
        SIGINT,
        SIGIOT,
        SIGPIPE,
        SIGQUIT,
        SIGSEGV,
        SIGSYS,
        SIGTERM,
        SIGTRAP,
        SIGUSR1,
        SIGUSR2
    };
    size_t num_signals = sizeof(signals) / sizeof(signals[0]);

    for (size_t i = 0; i < num_signals; i++) {
        if (sigaction(signals[i], &sa, NULL) == -1) {
            printf("Failed to register handler for signal %d\n", signals[i]);
            exit(EXIT_FAILURE);
        }
    }
}

void reg_all_page(void)
{
    REG_PAGE(PAGE_HOME);
    REG_PAGE(PAGE_WIRED_SP);
    REG_PAGE(PAGE_WIRELESS_SP);
    REG_PAGE(PAGE_SETTING);
    REG_PAGE(PAGE_SOURCE);
    REG_PAGE(PAGE_MEDIA);
    REG_PAGE(PAGE_DISK);
    REG_PAGE(PAGE_FILE);
    REG_PAGE(PAGE_MOVIE);
    REG_PAGE(PAGE_MUSIC);
    REG_PAGE(PAGE_PICTURE);
    REG_PAGE(PAGE_EBOOK);
#if ENABLE_WIFI
    REG_PAGE(PAGE_WIFI);
#endif
    REG_PAGE(PAGE_NETWORK_OTA);
#if ENABLE_BT
    REG_PAGE(PAGE_BT);
#endif
    REG_PAGE(PAGE_FACTORY_TEST);
    REG_PAGE(PAGE_MEDIA_STRESS);
}

int main(int argc, char *argv[]) {
    lv_disp_drv_t disp_drv;
    lv_disp_draw_buf_t disp_buf;
    uint32_t rotated = LV_DISP_ROT_NONE;

    install_sig_handler();

    lv_disp_drv_init(&disp_drv);

    if (argc >= 2 && atoi(argv[1]) >= 0 && atoi(argv[1]) <= 6) {
        rotated = atoi(argv[1]);
#ifndef USE_SUNXIFB_G2D_ROTATE
        if (rotated != LV_DISP_ROT_NONE)
            disp_drv.sw_rotate = 1;
#endif
    }

#if ENABLE_FASTBOOT
    disable_bootlogo();
#endif

    /*LittlevGL init*/
    lv_init();

    /*Linux frame buffer device init*/
    sunxifb_init(rotated);

    /*A buffer for LittlevGL to draw the screen's content*/
    static uint32_t width, height;
    sunxifb_get_sizes(&width, &height);

    static lv_color_t *buf;
#ifdef USE_SUNXIFB_DIRECT_MODE
    buf = (lv_color_t*) sunxifb_get_buf();
#else
    buf = (lv_color_t*) sunxifb_alloc(width * height * sizeof(lv_color_t),
            "lv_projector");
#endif

    if (buf == NULL) {
        sunxifb_exit();
        printf("malloc draw buffer fail\n");
        return 0;
    }

    /*Initialize a descriptor for the buffer*/
    lv_disp_draw_buf_init(&disp_buf, buf, NULL, width * height);

    /*Initialize and register a display driver*/
    disp_drv.draw_buf = &disp_buf;
    disp_drv.flush_cb = sunxifb_flush;
    disp_drv.hor_res = width;
    disp_drv.ver_res = height;
    if (rotated > 3)        //mirroring function does not perform special processing for UI rendering
        disp_drv.rotated    = 0;
    else
        disp_drv.rotated = rotated;
    disp_drv.screen_transp = 0;
#ifdef USE_SUNXIFB_DIRECT_MODE
    disp_drv.direct_mode = 1;
    disp_drv.full_refresh = 1;
#endif
    lv_disp_drv_register(&disp_drv);
    keypad_int();
    system_init_early();
    lv_pro_res_init();
    lv_projector_ui_init();
    system_init_late();

    /*Handle LitlevGL tasks (tickless mode)*/
    while (1) {

        system_message_process();

        pthread_mutex_lock(&lvgl_mutex);
        lv_task_handler();
        pthread_mutex_unlock(&lvgl_mutex);
        usleep(10000);
    }

    /*sunxifb_free((void**) &buf, "lv_g2d_test");*/
    /*sunxifb_exit();*/
    return 0;
}

/*Set in lv_conf.h as `LV_TICK_CUSTOM_SYS_TIME_EXPR`*/
uint32_t custom_tick_get(void) {
    static uint64_t start_ms = 0;
    if (start_ms == 0) {
        struct timeval tv_start;
        gettimeofday(&tv_start, NULL);
        start_ms = ((uint64_t) tv_start.tv_sec * 1000000
                + (uint64_t) tv_start.tv_usec) / 1000;
    }

    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    uint64_t now_ms;
    now_ms = ((uint64_t) tv_now.tv_sec * 1000000 + (uint64_t) tv_now.tv_usec)
            / 1000;

    uint32_t time_ms = now_ms - start_ms;
    return time_ms;
}
