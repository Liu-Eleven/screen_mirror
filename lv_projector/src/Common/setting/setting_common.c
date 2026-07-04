#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include "parser_ini/user_config.h"
#include "setting_common.h"

#define UPDATE_DELAY        (5)

static char update_flag;

static void* update_ini_thread(void *arg)
{
    sleep(UPDATE_DELAY);
    int mhKeyfd = open(USER_SETTING_FILE_PATH, O_RDWR);
    if (mhKeyfd < 0) {
        loge("error open file handle: %s\n", USER_SETTING_FILE_PATH);
        update_flag = 0;
        return NULL;
    }
    fsync(mhKeyfd);
    close(mhKeyfd);

    update_flag = 0;
    return NULL;
}

void do_update_ini(void)
{
    int ret = 0;
    pthread_t wait_pthread;

    if (update_flag)
        return;
    update_flag = 1;

    ret = pthread_create(&wait_pthread, NULL, &update_ini_thread, (void *)NULL);
    if (ret < 0)
        printf("update ini pthread_create error\n");
}

int setting_config_read(char *mainkey, char *subkey, char *value)
{
    int ret;
    ret = readConfig(USER_SETTING_FILE_PATH, mainkey, subkey, value);
    if (ret)
        ret = readConfig(DEFAULT_SETTING_FILE_PATH, mainkey, subkey, value);
    return ret;
}

int setting_config_write(char *mainkey, char *subkey, char *value)
{
    if (!writeConfig(USER_SETTING_FILE_PATH, mainkey, subkey, value, 0)) {
        do_update_ini();
        return 0;
    }
    return -1;
}
