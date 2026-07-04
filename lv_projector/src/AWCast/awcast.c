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
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include<dlfcn.h>
#define CONFIG_LOG_LEVEL LOG_LEVEL_DEBUG
#include <cdx_log.h>

#include "iscrctl_draw.h"
#include <irender.h>
#include <aw_dlna.h>
#include "awcast.h"
#include "miracast2.h"
#include "lv_common.h"
#include "widget/lv_pro_res.h"
#include "../WirelessSP/lv_pro_wirelesssp.h"

#include "aw_mirror_interface.h"

#define TIMEOUT_SEC 2
#define KEYSETSIZE 912

enum {
	AWCAST_STATE_INVALID,
	AWCAST_STATE_LISTENING,
	AWCAST_STATE_MIRACAST,
	AWCAST_STATE_DLNA,
	AWCAST_STATE_AIRPLAY
};

struct AWCastS {
    char device_name[64];
    char uuid[64];
    SrcctlDrawT *scd;
    RenderT *render;
    AWDlnaT *awd;
    int state;
    pthread_mutex_t lock;
    char miraStop;
    char dlnaStop;
    char airplayStop;
};

struct AWCastS *g_awc = NULL;

#define WIFI_WLAN0_ADDRESS "/sys/class/net/wlan0/address"

extern void lv_pro_dlna_set_render(RenderT *render);

struct AirplayServiceS *airplay_dev_t = NULL;
void *airplay_libFd = NULL;
char Airplay_init_flag;
char Miracast_init_flag;
char Dlna_init_flag;
char* __Get_AWCast_device_name(void)
{
    return g_awc->device_name;
}

static int get_hdcp_buf()
{
    //客户去读取,example:
    int fd;
    ssize_t nRead;
    unsigned char key_buf[1024] = {0};
    fd = open("/data/miracast.dat", O_RDONLY);
    if(fd == -1) {
        CDX_LOGE("Devicekeyset:open confidential failed.");
        return -1;
    }
    nRead = read(fd, key_buf, sizeof(key_buf));
    if (nRead != KEYSETSIZE-10) {
        CDX_LOGE("get key buf size abnormal");
        close(fd);
        return -1;
    }

    close(fd);
    printf("get key succeed.");
    Miracast_SetKey(key_buf);
    return 0;
}

int __Airplay_Event_CB(MirrorEventE enEvent, void *param)
{
    CDX_LOGD("AIRPLAY event: '%d'\n", enEvent);
    g_awc->state = AWCAST_STATE_AIRPLAY;

    switch (enEvent) {
        case AIRPLAY_MIRROR_START:
            AWCast_remove();
            AWCastEntryShow();
            break;

        case AIRPLAY_MIRROR_STOP:
            if (get_awcast_show_flag()) {
                if (!Miracast_init_flag || g_awc->miraStop == 1) {
                    AWCastExitShow();
                    AWCast_reset();
                }
            }
            break;

        default:
            CDX_LOGD("__AIRPLAY_Notify: unknow event: '%d'", enEvent);
            break;
    }
    return 0;
}

static int airplay_register(void)
{
    if (airplay_dev_t)
        return 0;

    airplay_libFd = dlopen(AIRPLAY_LIB, RTLD_LAZY | RTLD_GLOBAL);
    if (!airplay_libFd) {
        CDX_LOGE("dlopen airplay lib %s, error: %s\n", AIRPLAY_LIB, dlerror());
        return -1;
    }

    AirplayServiceGetInstance airplay = (AirplayServiceGetInstance)dlsym(airplay_libFd, "AirplayServiceGetInstance");
    if (airplay) {
        airplay_dev_t = airplay();
        return 0;
    }

    return -1;
}

static int airplay_unregister(void)
{
    if (airplay_libFd)
        dlclose(airplay_libFd);

    airplay_dev_t = NULL;
    return 0;
}

long long GetNowUs(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return (long long)tv.tv_usec + tv.tv_sec * 1000000ll;
}

int checkNetConnection(const char* host, int port)
{
    struct sockaddr_in sa;
    int sock = 0;
    struct hostent *host_entry;
    fd_set writefds;
    struct timeval tv;
    int result;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        logw("Could not create socket");
        return -1;
    }

    FD_ZERO(&writefds);
    FD_SET(sock, &writefds);
    tv.tv_sec = TIMEOUT_SEC;
    tv.tv_usec = 0;

    host_entry = gethostbyname(host);
    if (host_entry == NULL)
    {
        logw("Could not resolve hostname");
        close(sock);
        return -1;
    }

    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    memcpy(&sa.sin_addr, host_entry->h_addr_list[0], host_entry->h_length);

    result = connect(sock, (struct sockaddr *)&sa, sizeof(sa));
    if (result == -1)
    {
        if(errno == EINPROGRESS)
        {
            if (select(sock + 1, NULL, &writefds, NULL, &tv) > 0)
            {
                int so_error;
                socklen_t len = sizeof(so_error);
                result = getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len);
                if(result < 0 || so_error)
                {
                    if(so_error)
                    {
                        errno = so_error;
                    }
                    logw("connect err(%d)", errno);
                    close(sock);
                    return -1;
                }
            }
        }
    }

    close(sock);
    return result == 0 ? 0 : -1;
}

#if 0
int getSsid()
{
    FILE* fp=NULL;
    char command[] = "iw dev wlan0 link";
    char buf[1024] = {0};
    char *q = NULL;
    char ssid[256];
    int ret = -1;

    fp = popen(command, "r");
    if(fp == NULL){
        CDX_LOGW("%s:%d: popen failed");
        return ret;
    }

    while (fgets(buf, sizeof(buf), fp) != NULL)
    {
        q = strstr(buf, "SSID:");
        if (q != NULL)
        {
            memset(ssid, 0, sizeof(ssid));
            strncpy(ssid, q + 6, strlen(buf) - 1 - (q + 6 - buf));  // -1 exclude the symbol of end-of-line
            CDX_LOGD("get_ssid:%s", ssid);
            ret = 0;
        }
    }

    pclose(fp);
    return ret;
}
#endif

static int getWifiAddr(char *str)
{
    unsigned char file[] = WIFI_WLAN0_ADDRESS;
    unsigned char wifi_addr[32] = {0};
    unsigned char mac[13] = {0};
    int fd = 0;
    int ret = 0, err = 0;

    fd = open(file, O_RDONLY);
    if(fd < 0) {
        CDX_LOGE("err: awcast_get_wifi_addr: could not open %s, %s", file, strerror(errno));
        return -1;
    }

    ret = read(fd, (void *)&wifi_addr, sizeof(wifi_addr));
    if(ret <= 0) {
        CDX_LOGE("awcast_get_wifi_addr: read failed, %s", strerror(errno));
        err = -2;
        goto end;
    }
    ret = 0;

    sscanf(wifi_addr, "%c%c:%c%c:%c%c:%c%c:%c%c:%c%c", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5],
                     &mac[6], &mac[7], &mac[8], &mac[9], &mac[10], &mac[11]);

    sprintf(str, "%s", mac);

    CDX_LOGI("mac str is '%s,'.", str);

end:
    close(fd);

    return err;
}

static int __Miracast_Event_CB(MIRACAST_EVENT_CALLBACK_E enEvent, void* pvPrivateData)
{
    CDX_LOGD("event '%d'", enEvent);

    g_awc->state = AWCAST_STATE_MIRACAST;

    switch (enEvent)
    {
        case MIRACAST_CBK_P2P_FOUND:
        {
            CDX_LOGD("MIRACAST_CBK_P2P_FOUND...");
            break;
        }
        case MIRACAST_CBK_P2P_GO_NEG_REQUEST:
        {
            CDX_LOGD("MIRACAST_CBK_P2P_GO_NEG_REQUEST...");
            break;
        }
        case MIRACAST_CBK_P2P_INVITATION_ACCEPTED:
        {
            CDX_LOGD("MIRACAST_CBK_P2P_INVITATION_ACCEPTED...");
            break;
        }
        case MIRACAST_CBK_P2P_CONNECTED:
        {
            CDX_LOGD("Connected...");
            AWCast_remove();
            break;
        }
        case MIRACAST_CBK_P2P_DISCONNECTED:
        {
            CDX_LOGD("disconneted...");
            if (get_awcast_show_flag()) {
                if (!Airplay_init_flag || g_awc->airplayStop == 1) {
                    AWCastExitShow();
                    AWCast_reset();
                }
            }
            break;
        }
        case MIRACAST_CHK_P2P_NEGOTIATION_ERROR:
        case MIRACAST_CHK_P2P_FORMATION_ERROR:
        case MIRACAST_CHK_P2P_TIMEOUT_ERROR:
        case MIRACAST_CHK_P2P_OVERLAP_ERROR:
        {
            CDX_LOGD("MIRACAST_CBK_P2P_xxx... error, start ap again if network not connected");

            break;
        }
        case MIRACAST_CBK_PLAYER_FIRST_SHOW:
        {
            CDX_LOGD("MIRACAST_CBK_PLAYER_FIRST_SHOW");
            AWCastEntryShow();
            break;
        }
        default:
            CDX_LOGW("__Miracast_Event_CB: unknow event: '%d'", enEvent);
        break;
    }
    return 0;
}


static int __DLNA_Notify(int event, void *param)
{
    CDX_LOGD("DLNA event: '%d'", event);

    g_awc->state = AWCAST_STATE_DLNA;
    AWCast_remove();

    switch (event)
    {
        case AWD_EVENT_ENTRY:
        {
            CDX_LOGD("------AWD_EVENT_ENTRY------");
            break;
        }
        case AWD_EVENT_QUIT:
        {
            CDX_LOGD("------AWD_EVENT_QUIT------");
            SrcctlDraw_Control(g_awc->scd, DLNA_EVENT_QUIT, NULL);
            break;
        }

        default:
            CDX_LOGW("__DLNA_Notify: unknow event: '%d'", event);
            break;
        }
        return 0;
}

static struct AWD_LinstenerS dlna_linstener =
{
    .notify = __DLNA_Notify
};

static int awcast_get_rand(char *str, int cnt)
{
    int i = 0, n = 0;
    time_t t;
    int temp = 0;

    //srand((unsigned) time(&t));
    temp = rand();
    snprintf(str, (cnt+1), "%02x", temp);

    CDX_LOGI("rand str is '%s'.", str);

    return 0;
}

/* eg: 0b6741ac-7f21-c0c2-2aa0-7ad4f5d40441 */
static int awcast_get_uuid(char *uuid)
{
    int len = 0;
    char *temp_str = 0;

    temp_str = uuid;
    awcast_get_rand(temp_str, 8);
    temp_str += 8;

    strcat(temp_str, "-");
    temp_str += 1;

    awcast_get_rand(temp_str, 4);
    temp_str += 4;

    strcat(temp_str, "-");
    temp_str += 1;

    awcast_get_rand(temp_str, 4);
    temp_str += 4;

    strcat(temp_str, "-");
    temp_str += 1;

    awcast_get_rand(temp_str, 4);
    temp_str += 4;

    strcat(temp_str, "-");
    temp_str += 1;

    awcast_get_rand(temp_str, 12);

    CDX_LOGI("uuid str is '%s'.", uuid);

    return 0;
}

static void* awcast_remove_thread(void *arg)
{
    int id = *((int *)arg);

    if (id < AWCAST_STATE_MIRACAST || id > AWCAST_STATE_AIRPLAY)
        return NULL;

    if (id != AWCAST_STATE_MIRACAST) {
        if (Miracast_init_flag) {
            if (g_awc->miraStop == 0) {
                Miracast_Stop();
                Miracast_DeInit();
                g_awc->miraStop = 1;
            }
        }
    }
    if (id != AWCAST_STATE_DLNA) {
        if (Dlna_init_flag) {
            if (g_awc->dlnaStop == 0) {
                g_awc->dlnaStop = 1;
                if (g_awc->render != NULL)
                    AWD_Render_Stop();
                if (g_awc->awd != NULL) {
                    AWD_Destroy(g_awc->awd);
                    free(g_awc->awd);
                    g_awc->awd = NULL;
                }

                if (g_awc->render != NULL) {
                    RenderDestroy(g_awc->render);
                    g_awc->render = NULL;
                }
            }
        }
    }
    if (id != AWCAST_STATE_AIRPLAY) {
        if (g_awc->airplayStop == 0) {
            if (Airplay_init_flag && airplay_dev_t->stop)
                airplay_dev_t->stop();

            g_awc->airplayStop = 1;
        }
    }
    return NULL;
}

int AWCast_remove(void)
{
    int ret = 0;
    pthread_t wait_pthread;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&wait_pthread, &attr, &awcast_remove_thread, (void *)&g_awc->state);
    if (ret < 0)
        printf("awcast remove pthread_create error\n");

    return 0;
}

static void* awcast_reset_thread(void *arg)
{
    AWCast_StopService();
    AWCast_exit();
    AWCast_init();
    AWCast_StartService();

    return NULL;
}

int AWCast_reset(void)
{
    int ret = 0;
    pthread_t wait_pthread;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&wait_pthread, &attr, &awcast_reset_thread, (void *)NULL);
    if (ret < 0)
        printf("awcast reset pthread_create error\n");

    return 0;
}

int AWCast_StartService()
{
    g_awc->state = AWCAST_STATE_LISTENING;

    if (Miracast_init_flag)
        Miracast_Start(g_awc->device_name, __Miracast_Event_CB);

    if (Dlna_init_flag) {
        if (g_awc->awd != NULL)
        {
            AWD_Start(g_awc->awd);
        }
    }

    if (Airplay_init_flag && airplay_dev_t->start)
        airplay_dev_t->start();

    CDX_LOGI("AWCast_StartService finish....");

    return 0;
}

int AWCast_StopService()
{
    if (Miracast_init_flag && !g_awc->miraStop) {
        Miracast_Stop();
        g_awc->miraStop = 1;
    }

    if (Dlna_init_flag && g_awc->awd != NULL)
    {
        AWD_Stop(g_awc->awd);
    }

    if (Airplay_init_flag && !airplay_dev_t->stop) {
        airplay_dev_t->stop();
        airplay_dev_t->stop = 1;
    }

    g_awc->state = AWCAST_STATE_INVALID;
    return 0;
}

int AWCast_init()
{
    char wifi_addr[32] = {0};
    int ret;
    cdx_log_set_level(LOG_LEVEL_DEBUG);

    if (g_awc == NULL)
    {
        g_awc = malloc(sizeof(*g_awc));
        if (g_awc == NULL)
        {
            CDX_LOGE("g_awc malloc fail.");
            return -1;
        }
    }
    memset(g_awc, 0x00, sizeof(*g_awc));

    ret = getWifiAddr(wifi_addr);
    if (ret == 0)
    {
        sprintf(g_awc->device_name, "AWCast-%s", &wifi_addr[8]);
    }
	else
    {
        sprintf(g_awc->device_name, "AWCast-abcd");
    }

    /* init Miracast */
    Miracast_init_flag = 0;

#if ENABLE_MIRACAST
    int is_enable_hdcp = 0;

    if (get_hdcp_buf() == 0)
    {
        is_enable_hdcp = 1;
        if (!Miracast_Init(is_enable_hdcp))
            Miracast_init_flag = 1;
    }
    else
    {
        if (!Miracast_Init(is_enable_hdcp))
            Miracast_init_flag = 1;
    }
#endif

    /* Init Airplay */
    Airplay_init_flag = 0;
#if ENABLE_AIRPLAY
    airplay_register();
    if (airplay_dev_t && airplay_dev_t->init) {
        if (!airplay_dev_t->init(g_awc->device_name, (void *)__Airplay_Event_CB))
            Airplay_init_flag = 1;
    }
#endif

    Dlna_init_flag = 0;
#if ENABLE_DLNA
    if (checkNetConnection("8.8.8.8", 53) == 0)
    {
        /* init DLNA */
        g_awc->scd = LV_SrcctlDraw_Instance();
        g_awc->render = CedarRender_Instance(g_awc->scd);
        if (g_awc->render == NULL)
        {
            CDX_LOGE("CedarRender_Instance fail.");
            ret = -1;
            free(g_awc);
            g_awc = NULL;
            goto out;
        }
        lv_pro_dlna_set_render(g_awc->render);
        awcast_get_uuid(g_awc->uuid);
        g_awc->awd = AWD_Instance(g_awc->device_name, g_awc->uuid, &dlna_linstener, g_awc->render);
        if (g_awc->awd == NULL)
        {
            CDX_LOGE("AWD_Instance fail");
            ret = -1;
            RenderDestroy(g_awc->render);
            g_awc->render = NULL;
            goto out;
        }
        Dlna_init_flag = 1;
    }
    else
    {
        logw("Unable to connect to the network.");
    }
#endif
    g_awc->state = AWCAST_STATE_INVALID;

    CDX_LOGI("AWCast_init finish....");

out:
    return ret;
}

int AWCast_exit()
{
    CDX_LOGI("AWCast_exit");
    long long t0, t1, t2, t3, t4;

    if (Miracast_init_flag)
    {
        CDX_LOGI("Miracast exit start");
        t0 = GetNowUs();
        if (!g_awc->miraStop)
            Miracast_Stop();
        t1 = GetNowUs();
        Miracast_DeInit();
        t2 = GetNowUs();
        CDX_LOGD("miraStop cost, Miracast_Stop(%d ms), Miracast_DeInit(%d ms)",
            (t1-t0)/1000, (t2-t1)/1000);
        CDX_LOGI("Miracast exit end");
    }

    if (Dlna_init_flag) {
        CDX_LOGI("Dlna exit start");
        CDX_LOGI("g_awc->dlnaStop = %d",g_awc->dlnaStop);
        if (g_awc->dlnaStop == 0)
        {
            t0 = GetNowUs();
            if (g_awc->render != NULL && g_awc->awd != NULL)
                AWD_Stop(g_awc->awd); //player and awd both stop prevent again set url
            t1 = GetNowUs();
            if (g_awc->awd != NULL)
            {
                AWD_Destroy(g_awc->awd);
                free(g_awc->awd);
                g_awc->awd = NULL;
            }
            t2 = GetNowUs();
            if (g_awc->render != NULL)
            {
                RenderDestroy(g_awc->render);
                g_awc->render = NULL;
            }
            t3 = GetNowUs();

            CDX_LOGD("dlnaStop cost, AWD_Render_Stop(%d ms), AWD_Destroy(%d ms), RenderDestroy(%d ms)",
                (t1-t0)/1000, (t2-t1)/1000, (t3-t2)/1000);
        }
        CDX_LOGI("Dlna exit end");
    }


    if (Airplay_init_flag) {
        CDX_LOGI("Airplay exit start");
        if (!airplay_dev_t->stop)
            airplay_dev_t->stop();

        if (airplay_dev_t->exit)
            airplay_dev_t->exit();

        //airplay_unregister();
        CDX_LOGI("Airplay exit end");
    }

    Airplay_init_flag = 0;
    Miracast_init_flag = 0;
    Dlna_init_flag = 0;
    memset(g_awc, 0x00, sizeof(*g_awc));

    return 0;
}

void AWCast_restart_MiracastAirplay(void)
{
    g_awc->airplayStop = 0;
    if (g_awc->miraStop == 1)
    {
        g_awc->miraStop = 0;
        Miracast_init_flag = 0;

        #if ENABLE_MIRACAST
        int is_enable_hdcp = 0;

        if (get_hdcp_buf() == 0)
        {
            is_enable_hdcp = 1;
            if (!Miracast_Init(is_enable_hdcp))
                Miracast_init_flag = 1;
        }
        else
        {
            if (!Miracast_Init(is_enable_hdcp))
                Miracast_init_flag = 1;
        }
        #endif
    }

    if (g_awc->airplayStop == 1)
    {
        g_awc->airplayStop = 0;
        Airplay_init_flag = 0;
        #if ENABLE_AIRPLAY
            airplay_register();
            if (airplay_dev_t && airplay_dev_t->init) {
                if (!airplay_dev_t->init(g_awc->device_name, (void *)__Airplay_Event_CB))
                    Airplay_init_flag = 1;
            }
        #endif
    }
    g_awc->state = AWCAST_STATE_LISTENING;

    if (Miracast_init_flag)
        Miracast_Start(g_awc->device_name, __Miracast_Event_CB);

    if (Airplay_init_flag && airplay_dev_t->start)
        airplay_dev_t->start();
}
