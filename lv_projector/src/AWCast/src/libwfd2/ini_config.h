#ifndef INI_CONFIG_H
#define INI_CONFIG_H

#ifdef  __cplusplus
extern "C" {
#endif

#define LINE_CONTENT_MAX_LEN  256

int read_string_value(const char *key, char *value,const char *file);
int read_int_value(const char *key, int *value,const char *file);
int write_string_value(const char *key, char *value,const char *file);
int write_int_value(const char *key, int value,const char *file);

#ifdef  __cplusplus
}
#endif

#endif //INI_CONFIG_H
