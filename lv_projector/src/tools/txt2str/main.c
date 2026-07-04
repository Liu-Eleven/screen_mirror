#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#define LINE_LENGTH         (1024 * 10)

#define LANGAGE_MAX_COUNT   (16)
#define DATA_MAX_LEN        (512)
#define MAX_LINE            (1024)

#define DEFAULT_FILE        ("lv_string.txt")
#define FILE_STRING_ID_NAME ("lv_string_id")

static char langage_name[LANGAGE_MAX_COUNT][64];
static int langage_count = 0;
static int str_line = 0;
static char *all_langage_data;

#ifdef USE_DEBUG
static void debug(char *buf)
{
    static int i = 0;
    if ((i % langage_count) == 0)
        printf("\n %d ", i);
    else {
        printf("%s  ", buf);
    }
    i++;
}
#endif

static int parser_langage_name(char *buf)
{
	char *pval = buf;
	char *seps = "\t";
    
    //printf("buf %s\n", buf);
    memset(langage_name, 0, sizeof(langage_name));
	pval = strtok(pval, seps);
	while (pval != NULL) {
        if (pval[strlen(pval) - 1] == '\n' || pval[strlen(pval) - 1] == '\r')
            strncpy(langage_name[langage_count], pval, strlen(pval) - 2);
        else
            strncpy(langage_name[langage_count], pval, strlen(pval));
        printf("langage_name[%d] = %s\n", langage_count, langage_name[langage_count]);
        langage_count++;
        pval = strtok(NULL, seps);
	}
    return 0;
}

int strtok_utf8(const char *src_buf, char *dec_buf, const char *seps)
{
    int len = 0;
    if (src_buf == NULL || seps == NULL) {
        printf("strtok_utf8 is error\n");
        return -1;
    }
    while (*(src_buf + len) != '\0') {
        if (len >= strlen(src_buf))
            break;
        else if (*(src_buf + len) == '\t')
            break;
        else if (*(src_buf + len) == '\r')
            break;
        else if (*(src_buf + len) == '\n')
            break;
        len++;
    }
    strncpy(dec_buf, src_buf, len);
    dec_buf[len + 1] = 0;
#ifdef USE_DEBUG
    debug(dec_buf);
#endif
    return len;
}

int readConfig(char *path)
{
	char *str;
	FILE *mhKeyFile;
    const char *seps = "\t";
    const char *pval;
    char (*langage_data_p)[MAX_LINE][DATA_MAX_LEN];
    int i = 0;
    int j = 0;
    int len;

	if (path == NULL) {
		printf("error input para");
		return -1;
	}

	mhKeyFile = fopen(path, "rb");
	if (mhKeyFile == 0) {
		printf("open file error %s\n", path);
		return -1;
	}
	fseek(mhKeyFile, 0L, SEEK_SET);

    str = malloc(LINE_LENGTH);
	memset(str, 0, LINE_LENGTH);
    
    if (fgets(str, LINE_LENGTH, mhKeyFile)) {
        parser_langage_name(str);
    }

    if (langage_count > 0) {
        all_langage_data = (void *)malloc(DATA_MAX_LEN * MAX_LINE * (langage_count + 1));
    }
    memset(all_langage_data, 0, (DATA_MAX_LEN * MAX_LINE * (langage_count + 1)));
    langage_data_p = (void *)all_langage_data;
	while (fgets(str, LINE_LENGTH, mhKeyFile)) {
        pval = str;
        for (i = 0; i < langage_count; i++) {
            len = strtok_utf8(pval, langage_data_p[i][j], seps);
            if (len < 0)
                break;
            pval = pval + len + 1;
        }
        j++;
        memset(str, 0, LINE_LENGTH);
	}
    str_line = j;
	fclose(mhKeyFile);
	return 0;
}

int str_to_include_h(void)
{
    FILE *myFile;
    char file_name[DATA_MAX_LEN];
    char line_data[DATA_MAX_LEN];
    char (*langage_data_p)[MAX_LINE][DATA_MAX_LEN];
    int i, j;
    int line = 0;

    langage_data_p = (void *)all_langage_data;

    for (i = 0; i < langage_count; i++) {
        memset(file_name, 0, sizeof(file_name));

        if (!strcmp(langage_name[i], FILE_STRING_ID_NAME))
            sprintf(file_name, "%s.h", langage_name[i]);
        else
            sprintf(file_name, "str_%s.h", langage_name[i]);

        myFile = fopen(file_name, "wb+");
        if (!myFile) {
            printf("error temp key file handle");
            return -1;
        }
        fseek(myFile, 0L, SEEK_SET);

        memset(line_data, 0, sizeof(line_data));
        if (!strcmp(langage_name[i], FILE_STRING_ID_NAME)) {
            sprintf(line_data, "%s\r%s\r\r", "#ifndef __STR_LANGAGE_H_", "#define __STR_LANGAGE_H_");
            fwrite(line_data, strlen(line_data), 1, myFile);
        } else {
            sprintf(line_data, "const unsigned char *strs_array_%s[] = {\r", langage_name[i]);
            fwrite(line_data, strlen(line_data), 1, myFile);    
        }

        for (j = 0; j < str_line; j++) {
            memset(line_data, 0, sizeof(line_data));
            if (!strcmp(langage_name[i], FILE_STRING_ID_NAME)) {
                sprintf(line_data, "#define %-32s       (%d)\r", langage_data_p[i][j], j);
                fwrite(line_data, strlen(line_data), 1, myFile);
            } else {
                sprintf(line_data, "    \"%s\",\r", langage_data_p[i][j]);
                fwrite(line_data, strlen(line_data), 1, myFile);
                //printf("line_data %s\n", line_data);
            }
        }
        
        memset(line_data, 0, sizeof(line_data));
        if (!strcmp(langage_name[i], FILE_STRING_ID_NAME)) {
            sprintf(line_data, "\r%s", "#endif");
            fwrite(line_data, strlen(line_data), 1, myFile);
        } else {
            sprintf(line_data, "%s", "};");
            fwrite(line_data, strlen(line_data), 1, myFile);    
        }
        fflush(myFile);
        fclose(myFile);
    }
    return 0;
}

int main(int argc, char **argv)
{
	char cwd[128] = {0};
    char file_path[128] = {0};
    
    if (argc > 1)
        snprintf(file_path, sizeof(file_path), "%s", argv[1]);
    else {
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("The current working directory is %s\n", cwd);
            sprintf(file_path, "%s/%s", cwd, DEFAULT_FILE);
        }
    }

    if (!readConfig(file_path)) {
        str_to_include_h();
    }
    free(all_langage_data);
	return 0;
}

