#ifndef SCREENMIRROR_H
#define SCREENMIRROR_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 事件定义 ========== */
typedef enum {
    MIRROR_EVENT_CONNECTED,         /* 连接成功 */
    MIRROR_EVENT_DISCONNECTED,      /* 断开连接 */
    MIRROR_EVENT_ERROR,             /* 错误 */
    MIRROR_EVENT_STATE_CHANGED,     /* 状态变化 */
    MIRROR_EVENT_DEVICE_FOUND,      /* 发现设备 */
    MIRROR_EVENT_DISCOVERY_FINISHED,/* 发现完成 */
} MirrorEventType;

typedef struct {
    MirrorEventType type;
    int error_code;                 /* 错误码（仅在 ERROR 时有效） */
    const char *error_msg;          /* 错误信息 */
    void *data;                     /* 事件数据 */
} MirrorEvent;

/* ========== 投屏模式定义 ========== */
typedef enum {
    MIRROR_MODE_MIRACAST,           /* Miracast（WiFi Direct） */
    MIRROR_MODE_AIRPLAY,            /* AirPlay */
    MIRROR_MODE_DLNA,               /* DLNA */
    MIRROR_MODE_WIRED,              /* USB 有线投屏 */
    MIRROR_MODE_WIRELESS,           /* WiFi 无线投屏 */
} MirrorMode;

/* ========== 投屏状态定义 ========== */
typedef enum {
    MIRROR_STATE_IDLE,              /* 空闲 */
    MIRROR_STATE_DISCOVERING,       /* 发现中 */
    MIRROR_STATE_CONNECTING,        /* 连接中 */
    MIRROR_STATE_CONNECTED,         /* 已连接 */
    MIRROR_STATE_STREAMING,         /* 传输中 */
    MIRROR_STATE_PAUSED,            /* 已暂停 */
    MIRROR_STATE_ERROR,             /* 错误 */
} MirrorState;

/* ========== 设备信息结构体 ========== */
typedef struct {
    char name[128];                 /* 设备名称 */
    char mac_address[32];           /* MAC 地址 */
    char ip_address[16];            /* IP 地址 */
    int signal_strength;            /* 信号强度 0-100 */
    MirrorMode mode;                /* 投屏模式 */
    uint32_t model_id;              /* 型号 ID */
    void *platform_data;            /* 平台特定数据指针 */
} MirrorDeviceInfo;

/* ========== 投屏配置结构体 ========== */
typedef struct {
    MirrorMode mode;                /* 投屏模式 */
    int max_bitrate;                /* 最大码率 (Kbps) */
    int resolution_width;           /* 分辨率宽度 */
    int resolution_height;          /* 分辨率高度 */
    int refresh_rate;               /* 刷新率 (Hz) */
    bool enable_audio;              /* 是否启用音频 */
    bool enable_hdcp;               /* 是否启用 HDCP */
    int connect_timeout_ms;         /* 连接超时时间 (ms) */
    void *extra_config;             /* 扩展配置指针 */
} MirrorConfig;

/* ========== 回调函数定义 ========== */

/**
 * 投屏事件回调函数
 * @param event: 事件对象指针
 * @param user_data: 用户自定义数据
 */
typedef void (*MirrorEventCallback)(const MirrorEvent *event, void *user_data);

/**
 * 设备列表回调函数
 * @param devices: 设备列表数组
 * @param device_count: 设备数量
 * @param user_data: 用户自定义数据
 */
typedef void (*MirrorDeviceListCallback)(const MirrorDeviceInfo *devices,
                                        int device_count, void *user_data);

/* ========== 核心 API ========== */

/**
 * 初始化投屏库
 * @return: 0 成功，< 0 失败
 */
int screenmirror_init(void);

/**
 * 反初始化投屏库
 * @return: 0 成功，< 0 失败
 */
int screenmirror_exit(void);

/**
 * 设置事件回调
 * @param callback: 回调函数
 * @param user_data: 用户自定义数据
 * @return: 0 成功，< 0 失败
 */
int screenmirror_set_event_callback(MirrorEventCallback callback,
                                   void *user_data);

/**
 * 开始设备发现
 * @param mode: 投屏模式
 * @param timeout_ms: 发现超时时间 (毫秒)
 * @param callback: 设备列表回调
 * @param user_data: 用户自定义数据
 * @return: 0 成功，< 0 失败
 */
int screenmirror_start_discovery(MirrorMode mode, int timeout_ms,
                                MirrorDeviceListCallback callback,
                                void *user_data);

/**
 * 停止设备发现
 * @return: 0 成功，< 0 失败
 */
int screenmirror_stop_discovery(void);

/**
 * 连接指定设备
 * @param device: 设备信息
 * @param config: 连接配置
 * @return: 0 成功，< 0 失败
 */
int screenmirror_connect(const MirrorDeviceInfo *device,
                        const MirrorConfig *config);

/**
 * 断开连接
 * @return: 0 成功，< 0 失败
 */
int screenmirror_disconnect(void);

/**
 * 获取当前状态
 * @return: 当前投屏状态
 */
MirrorState screenmirror_get_state(void);

/**
 * 获取当前连接的设备信息
 * @return: 设备信息指针，如果未连接返回 NULL
 */
const MirrorDeviceInfo* screenmirror_get_device_info(void);

/**
 * 获取当前投屏配置
 * @return: 配置指针
 */
const MirrorConfig* screenmirror_get_config(void);

/**
 * 发送视频数据帧
 * @param data: 数据指针
 * @param size: 数据大小 (字节)
 * @return: 已发送字节数，< 0 表示失败
 */
int screenmirror_send_video_frame(const uint8_t *data, int size);

/**
 * 发送音频数据帧
 * @param data: 数据指针
 * @param size: 数据大小 (字节)
 * @return: 已发送字节数，< 0 表示失败
 */
int screenmirror_send_audio_frame(const uint8_t *data, int size);

/**
 * 投屏控制命令
 * @param command: 控制命令字符串 ("pause", "resume", "stop" 等)
 * @return: 0 成功，< 0 失败
 */
int screenmirror_control(const char *command);

/**
 * 获取库版本
 * @return: 版本字符串
 */
const char* screenmirror_get_version(void);

#ifdef __cplusplus
}
#endif

#endif /* SCREENMIRROR_H */
