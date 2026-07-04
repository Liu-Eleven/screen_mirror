#ifndef SCREENMIRROR_INTERNAL_H
#define SCREENMIRROR_INTERNAL_H

#include "screenmirror.h"
#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>

/* ========== 错误码定义 ========== */
#define MIRROR_ERR_SUCCESS           0
#define MIRROR_ERR_ALREADY_INIT     -1
#define MIRROR_ERR_NOT_INIT         -2
#define MIRROR_ERR_INVALID_PARAM    -3
#define MIRROR_ERR_OUT_OF_MEMORY    -4
#define MIRROR_ERR_CONNECT_FAILED   -5
#define MIRROR_ERR_TIMEOUT          -6
#define MIRROR_ERR_DEVICE_BUSY      -7
#define MIRROR_ERR_UNKNOWN          -99

/* ========== 事件系统 ========== */
#define MAX_OBSERVERS 10

typedef struct {
    MirrorEventCallback callback;
    void *user_data;
} EventObserver;

typedef struct {
    EventObserver observers[MAX_OBSERVERS];
    int observer_count;
    pthread_mutex_t lock;
} EventSystem;

EventSystem* event_system_create(void);
void event_system_destroy(EventSystem *sys);
int event_system_subscribe(EventSystem *sys, MirrorEventCallback callback,
                          void *user_data);
void event_system_emit(EventSystem *sys, const MirrorEvent *event);

/* ========== 状态机 ========== */
typedef struct {
    MirrorState current_state;
    pthread_mutex_t lock;
} StateMachine;

StateMachine* state_machine_create(void);
void state_machine_destroy(StateMachine *sm);
int state_machine_transition(StateMachine *sm, MirrorState new_state);
MirrorState state_machine_get_state(StateMachine *sm);
bool state_machine_can_transition(StateMachine *sm, MirrorState new_state);

/* ========== 协议接口 ========== */

/* 协议操作函数指针 */
typedef struct {
    /* 初始化协议 */
    int (*init)(void);
    
    /* 反初始化协议 */
    void (*exit)(void);
    
    /* 开始设备发现 */
    int (*start_discovery)(int timeout_ms);
    
    /* 停止设备发现 */
    void (*stop_discovery)(void);
    
    /* 连接设备 */
    int (*connect)(const MirrorDeviceInfo *device, const MirrorConfig *config);
    
    /* 断开连接 */
    void (*disconnect)(void);
    
    /* 发送视频帧 */
    int (*send_video)(const uint8_t *data, int size);
    
    /* 发送音频帧 */
    int (*send_audio)(const uint8_t *data, int size);
    
    /* 控制命令 */
    int (*control)(const char *command);
    
    /* 获取状态 */
    MirrorState (*get_state)(void);
} ProtocolOps;

/* ========== 核心引擎 ========== */

typedef struct {
    bool initialized;
    MirrorState state;
    MirrorDeviceInfo current_device;
    MirrorConfig config;
    
    EventSystem *event_sys;
    StateMachine *state_machine;
    
    /* 当前活跃的协议 */
    MirrorMode current_mode;
    ProtocolOps *protocol_ops;
    void *protocol_handle;  /* 具体协议实例指针 */
    
    /* 线程管理 */
    pthread_mutex_t lock;
    pthread_t discovery_thread;
    bool discovery_running;
    
    /* 用户回调 */
    MirrorEventCallback user_event_callback;
    void *user_callback_data;
} MirrorEngine;

/* 全局引擎实例 */
extern MirrorEngine *g_mirror_engine;

/* 内部 API */
int mirror_engine_create(void);
void mirror_engine_destroy(void);

#endif /* SCREENMIRROR_INTERNAL_H */
