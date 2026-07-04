/*

Copyright (c) 2008-2019 Allwinner Technology Co. Ltd.. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include<dlfcn.h>

#define CONFIG_LOG_LEVEL LOG_LEVEL_DEBUG
#include <cdx_log.h>
#include "aw_mirror_interface.h"
#include "usbcast.h"
#include "widget/lv_pro_res.h"
#include "lv_pro_wiredsp.h"

enum {
	USBCAST_STATE_INVALID,
	USBCAST_STATE_LISTENING,
	USBCAST_STATE_ANDROID,
	USBCAST_STATE_AIRPLAY
};

struct AWLine_dev {
    char device_name[64];
    int state;
    pthread_mutex_t lock;
    char AndroidLine_Stop;
    char AirplayLine_Stop;
};

struct AWLine_dev *g_usbcast = NULL;

struct AndroidLineServiceS *android_line_dev_t = NULL;
struct AirplayLineServiceS *airplay_line_dev_t = NULL;
void *Wired_libFd = NULL;
char AndroidLine_init_flag;
char AirplayLine_init_flag;

int usbcast_sp_Event_CB(MirrorEventE enEvent, void *param)
{
    CDX_LOGD("Wired_sp event: '%d'\n", enEvent);

    switch (enEvent) {
        case ANDROID_MIRROR_START:
            g_usbcast->state = USBCAST_STATE_ANDROID;
            USBCast_remove();
            USBCastEntryShow();
            break;
        case ANDROID_MIRROR_STOP:
            if (!get_usbcast_key_flag()) {
                if (!AirplayLine_init_flag || g_usbcast->AirplayLine_Stop == 1) {
                    USBCastExitShow();
                    set_current_ui_group(WiredSP_group);
                    USBCast_reset();
                }
            }
            break;
        case IOS_MIRROR_START:
            g_usbcast->state = USBCAST_STATE_AIRPLAY;
            USBCast_remove();
            USBCastEntryShow();
            break;

        case IOS_MIRROR_STOP:
            if (!get_usbcast_key_flag()) {
                if (!AndroidLine_init_flag || g_usbcast->AndroidLine_Stop == 1) {
                    USBCastExitShow();
                    set_current_ui_group(WiredSP_group);
                    USBCast_reset();
                }
            }
            break;
        default:
            CDX_LOGD("__Wired_sp_Event_CB: unknow event: '%d'", enEvent);
            break;
    }
    return 0;
}

static int Wired_protocol_register(void)
{
    if (android_line_dev_t || airplay_line_dev_t)
        return 0;

    Wired_libFd = dlopen(WIRED_LIB, RTLD_LAZY | RTLD_GLOBAL);
    if (!Wired_libFd) {
        CDX_LOGE("dlopen Wired protocol lib %s, error: %s\n", WIRED_LIB, dlerror());
        return -1;
    }

    AirplayLineServiceGetInstance airplayline = (AirplayLineServiceGetInstance)dlsym(Wired_libFd, "AirplayLineServiceGetInstance");
    AndroidLineServiceGetInstance androidline = (AndroidLineServiceGetInstance)dlsym(Wired_libFd, "AndroidLineServiceGetInstance");

    android_line_dev_t = androidline();
    airplay_line_dev_t = airplayline();

    return 0;
}

static int Wired_protocol_unregister(void)
{
    if (Wired_libFd)
        dlclose(Wired_libFd);

    android_line_dev_t = NULL;
    airplay_line_dev_t = NULL;

    return 0;
}

static void* usbcast_remove_thread(void *arg)
{
    int id = *((int *)arg);

    if (id != USBCAST_STATE_AIRPLAY) {
        printf("yan USBCAST_STATE_AIRPLAY\n");
        if (AirplayLine_init_flag) {
            if (g_usbcast->AirplayLine_Stop == 0 && airplay_line_dev_t->stop) {
                airplay_line_dev_t->stop();
                g_usbcast->AirplayLine_Stop = 1;
            }
        }
    }
    if (id != USBCAST_STATE_ANDROID) {
        printf("yan USBCAST_STATE_ANDROID\n");
        if (AndroidLine_init_flag) {
            if (g_usbcast->AndroidLine_Stop == 0 && android_line_dev_t->stop) {
                android_line_dev_t->stop();
                g_usbcast->AndroidLine_Stop = 1;
            }
        }
    }
    return NULL;
}

int USBCast_remove(void)
{
    int ret = 0;
    pthread_t wait_pthread;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&wait_pthread, &attr, &usbcast_remove_thread, (void *)&g_usbcast->state);
    if (ret < 0)
        printf("awcast remove pthread_create error\n");

    return 0;
}

static void* usbcast_reset_thread(void *arg)
{
    USBCast_StopService();
    USBCast_exit();
    USBCast_init();
    USBCast_StartService();

    return NULL;
}

int USBCast_reset(void)
{
    int ret = 0;
    pthread_t wait_pthread;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&wait_pthread, &attr, &usbcast_reset_thread, (void *)NULL);
    if (ret < 0)
        printf("usbcast reset pthread_create error\n");

    return 0;
}

int USBCast_StartService(void)
{
	int ret;

    g_usbcast->state = USBCAST_STATE_LISTENING;

    if (g_usbcast->AndroidLine_Stop == 0)
    {
        if (AndroidLine_init_flag && android_line_dev_t->start) {
            android_line_dev_t->start();
        }
    }

    if (g_usbcast->AirplayLine_Stop == 0)
    {
        if (AirplayLine_init_flag && airplay_line_dev_t->start) {
            airplay_line_dev_t->start();
        }
    }

    CDX_LOGI("USBCast_StartService finish....");

    return 0;
}

int USBCast_StopService(void)
{
    int ret;

    if (g_usbcast->AndroidLine_Stop == 0) {
        if (AndroidLine_init_flag && android_line_dev_t->stop) {
            android_line_dev_t->stop();
            g_usbcast->AndroidLine_Stop = 1;
        }
    }

    if (g_usbcast->AirplayLine_Stop == 0) {
        if (AirplayLine_init_flag && airplay_line_dev_t->stop) {
            airplay_line_dev_t->stop();
            g_usbcast->AirplayLine_Stop = 1;
        }
    }

    g_usbcast->state = USBCAST_STATE_INVALID;
    return 0;
}

int USBCast_init(void)
{
    cdx_log_set_level(LOG_LEVEL_DEBUG);

    if (g_usbcast == NULL)
    {
        g_usbcast = malloc(sizeof(*g_usbcast));
        if (g_usbcast == NULL)
        {
            CDX_LOGE("g_usbcast malloc fail.");
            return -1;
        }
    }
    memset(g_usbcast, 0x00, sizeof(*g_usbcast));

    Wired_protocol_register();

    AndroidLine_init_flag = 0;
    if (android_line_dev_t && android_line_dev_t->init) {
        if (!android_line_dev_t->init(NULL, (void *)usbcast_sp_Event_CB))
            AndroidLine_init_flag = 1;
    }

    AirplayLine_init_flag = 0;
    if (airplay_line_dev_t && airplay_line_dev_t->init) {
        if (!airplay_line_dev_t->init(NULL, (void *)usbcast_sp_Event_CB))
            AirplayLine_init_flag = 1;
    }

    g_usbcast->state = USBCAST_STATE_INVALID;
    CDX_LOGI("USBCast_init finish....");
    return 0;
}

int USBCast_exit()
{
    CDX_LOGI("USBCast_exit");

    if (AndroidLine_init_flag && android_line_dev_t->exit) {
        android_line_dev_t->exit();
    }

    if (AirplayLine_init_flag && airplay_line_dev_t->exit) {
        airplay_line_dev_t->exit();
    }

    AirplayLine_init_flag = 0;
    AndroidLine_init_flag = 0;
    memset(g_usbcast, 0x00, sizeof(*g_usbcast));

    return 0;
}

