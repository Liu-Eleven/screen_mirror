#ifndef AW_MIRROR_INTERFACE_H
#define AW_MIRROR_INTERFACE_H

#define AIRPLAY_LIB ("/usr/lib/libthirdparty_mirror.so")
#define WIRED_LIB   ("/usr/lib/libthirdparty_mirror.so")

/*
协议厂商实现以下函数，主程序调用通过dlopen/dlsym获取该函数并调用获取 struct AirplayServiceS实例
struct AirplayServiceS *AirplayServiceGetInstance(void);

//TO
struct AirplayLineServiceS *AirplayLineServiceGetInstance(void);
struct AndroidLineServiceS *AndroidLineServiceGetInstance(void);
注意：用C函数实现
*/

typedef struct AirplayServiceS* (*AirplayServiceGetInstance)(void);
typedef struct AirplayLineServiceS* (*AirplayLineServiceGetInstance)(void);
typedef struct AndroidLineServiceS* (*AndroidLineServiceGetInstance)(void);

typedef enum{
    ANDROID_MIRROR_START,
    ANDROID_MIRROR_STOP,
    IOS_MIRROR_START,
    IOS_MIRROR_STOP,
    AIRPLAY_MIRROR_START,
    AIRPLAY_MIRROR_STOP,
    AIRPLAY_AUDIO_START,
    AIRPLAY_AUDIO_STOP,
    AIRPLAY_URL_START,
    AIRPLAY_URL_STOP
} MirrorEventE;

typedef int (*event_cb)(MirrorEventE event, void *param);

struct AirplayServiceS {
    int (*init)(char *dev_name, void *event_cb);
    int (*exit)(void);
    int (*start)(void);
    int (*stop)(void);
};

struct AndroidLineServiceS {
    int (*init)(char *dev_name, void *event_cb);
    int (*exit)(void);
    int (*start)(void);
    int (*stop)(void);
};

struct AirplayLineServiceS {
    int (*init)(char *dev_name, void *event_cb);
    int (*exit)(void);
    int (*start)(void);
    int (*stop)(void);
};


/*
*   激活服务
*/
typedef enum{
    ACTIVATE_OK,
    ACTIVATE_FAIL,
} ActivateEventE;

typedef struct ActivateServiceS* (*ActivateServiceGetInstance)(void);

typedef int (*activate_cb)(ActivateEventE code);

struct ActivateServiceS {
	int (*init)(void* activate_cb);
};


#endif
