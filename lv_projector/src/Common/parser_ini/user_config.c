#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include "parser_ini/user_config.h"

#define FILE_MAX_SIZE       (4*1024)
#define LINE_LENGTH         (512)
#define KEY_LENGTH          (32)
#define FILE_NAME_LENGTH    (128)
#define DEAL_SPACE          (1)

pthread_mutex_t g_file_mutex = PTHREAD_MUTEX_INITIALIZER;

#define loge(fmt, arg...) fprintf(stderr, fmt "\n", ##arg)
#define logw(fmt, arg...)
#define logd(fmt, arg...)
#define logv(fmt, arg...)

static void getValue(char *line, char *value)
{
    char *ptemp = line;

    while (*ptemp && (*ptemp++ != '='));

    char *pval = ptemp;
    char *seps = " \n\r\t";
    int offset = 0;
    pval = strtok(pval, seps);
    while (pval != NULL) {
        strncpy(value + offset, pval, strlen(pval));
        offset += strlen(pval);
        pval = strtok(NULL, seps);
    }
    *(value + offset) = 0;
}

static void setValue(char *line, char *value)
{
    char temp_str[256] = {0};
    int i;

    for (i = 0; i < strlen(line); i++) {
        temp_str[i] = line[i];
        if (line[i] == '=') {
            temp_str[i  + 1] = ' ';
            temp_str[i  + 2] = '\0';
            memset(line, 0, sizeof(line));
            sprintf(line, "%s%s", temp_str, value);
            return;
        }
    }
}

static void _deal_return(char *buf)
{
    char *ptemp = buf;
    int i;

    for (i = 0; i < strlen(buf); i++) {
        if (ptemp[i] && (ptemp[i] == '\n' || ptemp[i] == '\r')) {
            ptemp[i + 1] = '\0';
            return;
        }
    }
    ptemp[i] = '\n';
    ptemp[i + 1] = '\0';
}

int readConfig(char *path, char *mainkey, char *subkey, char *value)
{
    int bRet = -1;
    char str[LINE_LENGTH];
    char tmp_mainkey[64] = {0};
    FILE *mhKeyFile;
    int find_main = 0;

    pthread_mutex_lock(&g_file_mutex);

    if (subkey == NULL || value == NULL || path == NULL || mainkey == NULL) {
        loge("error input para");
        pthread_mutex_unlock(&g_file_mutex);
        return -1;
    }

    mhKeyFile = fopen(path, "rb");
    if (mhKeyFile == 0) {
        loge("error open file handle: %s\n", path);
        pthread_mutex_unlock(&g_file_mutex);
        return -1;
    }

    fseek(mhKeyFile, 0L, SEEK_SET);

    memset(str, 0, LINE_LENGTH);
    sprintf(tmp_mainkey, "[%s]", mainkey);
    while (fgets(str, LINE_LENGTH, mhKeyFile)) {
        if (!find_main) {
            if (!strncmp(str, tmp_mainkey, strlen(tmp_mainkey))) {
                find_main = 1;
            }
            memset(str, 0, LINE_LENGTH);
            continue;
        }

        if (!strncmp("[", str, strlen("["))) {
            bRet = -1;
            break;
        }

        if (!strncmp(subkey, str, strlen(subkey))) {
            getValue(str, value);

            bRet = 0;
            break;
        }
        memset(str, 0, LINE_LENGTH);
    }

    fclose(mhKeyFile);
    pthread_mutex_unlock(&g_file_mutex);
    return bRet;
}

/*
*   set config
*
*   [main key]
*   subkey = value
*/
int writeConfig(char *path, char *mainkey, char *subkey, char *value, int save)
{
    int replaceLineLen = 0;
    FILE *mhKeyFile = NULL;
    char tmp_file[FILE_NAME_LENGTH] = {0};
    char tmp_mainkey[KEY_LENGTH] = {0};
    char tmp_subkey[KEY_LENGTH] = {0};
    char str[LINE_LENGTH];
    char tmp_note[LINE_LENGTH] = {0};
    char *file_buf = NULL;
    int find_main = 0;
    int find_sub = 0;

    pthread_mutex_lock(&g_file_mutex);

    if (subkey == NULL || value == NULL || path == NULL) {
        loge("error input para\n");
        pthread_mutex_unlock(&g_file_mutex);
        return -1;
    }
    //printf("************** writeConfig %s %s %s %s\n", path, mainkey, subkey, value);

    mhKeyFile = fopen(path, "rb+");
    if (!mhKeyFile) {
        loge("error open file handle: %s\n", path);
        pthread_mutex_unlock(&g_file_mutex);
        return -1;
    }
    fseek(mhKeyFile, 0L, SEEK_SET);

    file_buf = malloc(FILE_MAX_SIZE);
    if (!file_buf) {
        loge("writeConfig: malloc error\n");
        fclose(mhKeyFile);
        pthread_mutex_unlock(&g_file_mutex);
        return -1;
    }

    memset(file_buf, 0, FILE_MAX_SIZE);
    memset(str, 0, LINE_LENGTH);
    sprintf(tmp_mainkey, "[%s]", mainkey);
    while (fgets(str, LINE_LENGTH, mhKeyFile)) {
        /* find main key */
        if (!find_main) {
            if (!strncmp(str, tmp_mainkey, strlen(tmp_mainkey))) {
                find_main = 1;
            }
            /* deal prev note # */
            if (strlen(tmp_note)) {
                strcat(file_buf, tmp_note);
                memset(tmp_note, 0, LINE_LENGTH);
            }
            if (str[0] == '#' || (str[0] == ' ' && str[1] == '#') ||
                    str[0] == ';' || (str[0] == ' ' && str[1] == ';')) {
                memset(tmp_note, 0, LINE_LENGTH);
                strncpy(tmp_note, str, strlen(str) + 1);
            } else {
                strcat(file_buf, str);
            }
            memset(str, 0, LINE_LENGTH);
            continue;
        }
        /* find next main key */
        if (!strncmp("[", str, strlen("["))) {
            /* add new sub key */
            memset(tmp_subkey, 0, KEY_LENGTH);
            sprintf(tmp_subkey, "%s = %s", subkey, value);
            replaceLineLen = strlen(tmp_subkey);
            tmp_subkey[replaceLineLen] = '\n';
            tmp_subkey[replaceLineLen+1] = '\n';
            strcat(file_buf, tmp_subkey);

            /* deal prev note # */
            if (strlen(tmp_note)) {
                strcat(file_buf, tmp_note);
                memset(tmp_note, 0, LINE_LENGTH);
            }
            strcat(file_buf, str);
            memset(str, 0, LINE_LENGTH);
            find_sub = 1;
            break;
        }
        /* find sub key */
        if (!strncmp(subkey, str, strlen(subkey))) {
            /* deal prev note # */
            if (strlen(tmp_note)) {
                strcat(file_buf, tmp_note);
                memset(tmp_note, 0, LINE_LENGTH);
            }

            /* update subkey */
            setValue(str, value);
            _deal_return(str);

            strcat(file_buf, str);
            memset(str, 0, LINE_LENGTH);
            find_sub = 1;
            break;
        }

#if DEAL_SPACE
        if (str[0] != '\n' && str[0] != '\t' && str[0] != '\r') {
#endif
            /* deal prev note # */
            if (strlen(tmp_note)) {
                strcat(file_buf, tmp_note);
                memset(tmp_note, 0, LINE_LENGTH);
            }
            if (str[0] == '#' || (str[0] == ' ' && str[1] == '#') ||
                    str[0] == ';' || (str[0] == ' ' && str[1] == ';')) {
                memset(tmp_note, 0, LINE_LENGTH);
                strncpy(tmp_note, str, strlen(str) + 1);
            } else {
                strcat(file_buf, str);
            }
#if DEAL_SPACE
        }
#endif
        memset(str, 0, LINE_LENGTH);
    }

    if (!find_main && !find_sub) {
        memset(str, 0, LINE_LENGTH);
        str[0] = '\n';
        sprintf(&str[1], "%s", tmp_mainkey);
        replaceLineLen = strlen(str);
        str[replaceLineLen] = '\n';
        strcat(file_buf, str);

        memset(str, 0, LINE_LENGTH);
        sprintf(str, "%s = %s", subkey, value);
        replaceLineLen = strlen(str);
        str[replaceLineLen] = '\n';
        strcat(file_buf, str);
    } else if (find_main && !find_sub){
        memset(str, 0, LINE_LENGTH);
        sprintf(str, "%s = %s", subkey, value);
        replaceLineLen = strlen(str);
        str[replaceLineLen] = '\n';
        strcat(file_buf, str);
    }

    memset(str, 0, LINE_LENGTH);
    while (fgets(str, LINE_LENGTH, mhKeyFile)) {
        strcat(file_buf, str);
        memset(str, 0, LINE_LENGTH);
    }
    fclose(mhKeyFile);

    int mhKeyfd = open(path, O_RDWR | O_CREAT,0777);
    if (mhKeyfd < 0) {
        loge("error open file handle: %s\n", path);
        pthread_mutex_unlock(&g_file_mutex);
        free(file_buf);
        return -1;
    }
    write(mhKeyfd, file_buf, strlen(file_buf));
    if (save)
        fsync(mhKeyfd);
    close(mhKeyfd);
    free(file_buf);

    pthread_mutex_unlock(&g_file_mutex);
    return 0;
}
