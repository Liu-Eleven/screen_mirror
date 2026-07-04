#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include "ota/ota_update.h"
#include "lv_common.h"
#include "network_ota.h"
#include "lv_pro_launcher.h"

static uint8_t already_check;

struct download_param {
    char *url;
    char *dest_path;
};

void update_ota_version(void)
{
    char online_version[32] = {0};

    if (readConfig(NET_OTA_CONFIG_PATH, MAINKEY_OTA_NAME, SUBKEY_VERSION_NAME, online_version))
        return -1;

    writeConfig(LOCAL_OTA_CONFIG, MAINKEY_OTA_NAME, SUBKEY_VERSION_NAME, online_version, 1);
}

int check_ota_md5(char *fw_patch, char *config_path)
{
    char online_md5[64] = {0};
    char loacal_md5[64] = {0};
    char command[128] = {0};
    FILE   *stream;
    uint8_t md5_is_ok = 0;
    char *p;

    /* check md5 */
    if (!readConfig(config_path, MAINKEY_OTA_NAME, SUBKEY_CHECK_NAME, online_md5)) {
        /* don't need check */
        if (!strncmp(online_md5, "0", strlen(online_md5)))
            return 0;

        sprintf(command, "md5sum %s", fw_patch);
        stream = popen(command, "r");
        if(!stream) {
            return -1;
        }

        memset(command, 0, sizeof(command));
        if (fgets(command, sizeof(command), stream)) {
            strncpy(loacal_md5, strtok_r(command, " ", &p), sizeof(loacal_md5));
            printf("ota md5: local %s, online %s\n", loacal_md5, online_md5);
            if (!strncmp(online_md5, loacal_md5, strlen(loacal_md5))) {
                md5_is_ok = 1;
            }
        }
        pclose(stream);

        if (md5_is_ok)
            return 0;
        else
            return -1;
    }

    /* don't need check */
    return 0;
}

static int check_ota_product(char *path)
{
    char local_product[32] = {0};
    char online_product[32] = {0};

    /* check product */
    if (readConfig(LOCAL_OTA_CONFIG, MAINKEY_OTA_NAME, SUBKEY_PRODUCT_NAME, local_product))
        return -1;

    if (readConfig(path, MAINKEY_OTA_NAME, SUBKEY_PRODUCT_NAME, online_product))
        return -1;

    //printf("ota product: local %s, online %s\n", local_product, online_product);
    if (strncmp(local_product, online_product, strlen(online_product))) {
        printf("ota: Can't update, %s %s\n", local_product, online_product);
        return -1;
    }
    return 0;
}

static int check_ota_version(char *path)
{
    char local_version[32] = {0};
    char online_version[32] = {0};
    char test_mode[16] = {0};

    /* test mode */
    if (!readConfig(LOCAL_OTA_CONFIG, MAINKEY_OTA_NAME, SUBKEY_TEST_NAME, test_mode)) {
        if (!strncmp(test_mode, "1", strlen(test_mode)))
            return 0;
    }

    /* check product */
    if (check_ota_product(path))
        return -1;

    /* check version */
    if (readConfig(LOCAL_OTA_CONFIG, MAINKEY_OTA_NAME, SUBKEY_VERSION_NAME, local_version))
        return -1;

    if (readConfig(path, MAINKEY_OTA_NAME, SUBKEY_VERSION_NAME, online_version))
        return -1;

    //printf("ota version: local %s, online %s\n", local_version, online_version);

    /* format: 00:00:00 */
    char local_v0[8] = {0};
    char local_v1[8] = {0};
    char local_v2[8] = {0};
    char online_v0[8] = {0};
    char online_v1[8] = {0};
    char online_v2[8] = {0};
    uint32_t local_data;
    uint32_t online_data;
    char *tmp_str;
    char *p;

    tmp_str = strtok_r(local_version, ".", &p);
    if (tmp_str)
        strncpy(local_v0, tmp_str, 2);

    tmp_str = strtok_r(NULL, ".", &p);
    if (tmp_str)
        strncpy(local_v1, tmp_str, 2);

    tmp_str = strtok_r(NULL, ".", &p);
    if (tmp_str)
        strncpy(local_v2, tmp_str, 2);

    tmp_str = strtok_r(online_version, ".", &p);
    if (tmp_str)
        strncpy(online_v0, tmp_str, 2);

    tmp_str = strtok_r(NULL, ".", &p);
    if (tmp_str)
        strncpy(online_v1, tmp_str, 2);

    tmp_str = strtok_r(NULL, ".", &p);
    if (tmp_str)
        strncpy(online_v2, tmp_str, 2);

    //printf("local_v %s:%s:%s\n", local_v0, local_v1, local_v2);
    //printf("online_v %s:%s:%s\n", online_v0, online_v1, online_v2);

    if (strlen(online_v0)) {
        if (!strlen(local_v0))
            return 0;

        online_data = atoi(online_v0);
        local_data = atoi(local_v0);
        if (online_data > local_data)
            return 0;
        else if (online_data == local_data) {
            if (strlen(online_v1)) {
                if (!strlen(local_v1))
                    return 0;

                online_data = atoi(online_v1);
                local_data = atoi(local_v1);
                if (online_data > local_data)
                    return 0;
                else if (online_data == local_data) {
                    if (strlen(online_v2)) {
                        if (!strlen(local_v2))
                            return 0;
                    }
                    online_data = atoi(online_v2);
                    local_data = atoi(local_v2);
                    if (online_data > local_data)
                        return 0;
                }
            }
        }
    }

    return -1;
}

int check_local_ota_log(void)
{
    if (access(NET_OTA_UPDATE_LOG, F_OK) == 0) {
        return 0;
    }
    return -1;
}

int get_download_info(char *step, char *time)
{
    char str[512] = {0};
    char tmp_str[512] = {0};
    uint8_t find = 0;
    FILE * myFile;
    char *p;

    myFile = fopen(NET_OTA_UPDATE_LOG, "r");
    if (myFile == 0) {
        printf("error open file handle: %s\n", NET_OTA_UPDATE_LOG);
        return -1;
    }

    fseek(myFile, 0L, SEEK_SET);

    memset(str, 0, sizeof(str));
    while (fgets(str, 512, myFile)) {
        if (strstr(str, "update.swu") && strstr(str, "ETA")) {
            memset(tmp_str, 0, sizeof(tmp_str));
            memcpy(tmp_str, str, strlen(str));
            find = 1;
        }
        memset(str, 0, sizeof(str));
    }
    fclose(myFile);

    if (find) {
        /* format: update.swu            24% |*******                         | 3553k  0:05:39 ETA */

        strtok_r(tmp_str, " ", &p);
        strncpy(step, strtok_r(NULL, " ", &p), 4);
        strtok_r(NULL, "|", &p);
        strtok_r(NULL, " ", &p);
        strncpy(time, strtok_r(NULL, " ", &p), 8);

        step[strlen(step) - 1] = '\0';   // del %
        time[strlen(time)] = '\0';
    }

    return 0;
}

static int network_is_ok(char *ip)
{
    char command[512] = {0};
    FILE   *stream;

    sprintf(command, "ping %s -c 4", ip);
    stream = popen(command, "r");
    if(!stream) {
        return -1;
    }
    while (fgets(command, sizeof(command), stream)) {
        if (strstr(command, "100% packet loss") || strstr(command, "Network unreachable")) {
            pclose(stream);
            return -1;
        }
        memset(command, 0, sizeof(command));
    }

    pclose(stream);
    return 0;
}

static void * download(struct download_param *param)
{
    char command[256] = {0};

    sprintf(command, "wget -c -O %s %s > %s 2>&1", param->dest_path, param->url, NET_OTA_UPDATE_LOG);

    system(command);

    return 0;
}


static void *download_thread(void *arg) {
    int is_ota = 0;
    struct download_param *param = (struct download_param *)arg;

    download(param);

    pthread_exit(NULL);
}

void download_from_url(const char *url, const char *dest_path, uint8_t wait)
{
    struct download_param *param = NULL;

    param = malloc(sizeof(struct download_param));
    memset(param, 0x00, sizeof(struct download_param));

    param->url = url;
    param->dest_path = dest_path;

    if (!wait) {
        int ret = 0;
        pthread_t wait_pthread;
        ret = pthread_create(&wait_pthread, NULL, &download_thread, (void *)param);
    } else {
        download(param);
    }
}

void network_download_ota(void)
{
    download_from_url(OTA_FW_URL_0, NET_OTA_FW_PATH, 0);
}

static void* ota_check_thread(void *arg)
{
    char str[64] = {0};
    char *url = NULL;

    /* delay some times, that go to check */
    sleep(3);

    if (!network_is_ok(OTA_CHECK_0_IP)) {
        download_from_url(OTA_CONFIG_URL_0, NET_OTA_CONFIG_PATH, 1);
        url = OTA_FW_URL_0;
    } else if (!network_is_ok(OTA_CHECK_1_IP)) {
        download_from_url(OTA_CONFIG_URL_1, NET_OTA_CONFIG_PATH, 1);
        url = OTA_FW_URL_1;
    } else {
        printf("ota service not connect\n");
        goto _exit;
    }

    if (check_ota_version(NET_OTA_CONFIG_PATH))
        goto _exit;

    /* only on launcher page, create ota message*/
    if (get_current_page_id() == PAGE_HOME)
        switch_page(PAGE_NETWORK_OTA);

_exit:
    return NULL;
}

void network_ota_check(void)
{
    int ret = 0;
    pthread_t wait_pthread;

    /* only first network connect */
    if (already_check)
        return;
    already_check = 1;

    ret = pthread_create(&wait_pthread, NULL, &ota_check_thread, (void *)NULL);
    if (ret < 0)
        printf("net ota pthread_create error\n");
}
