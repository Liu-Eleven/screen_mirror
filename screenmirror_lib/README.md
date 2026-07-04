# libscreenmirror - 高效易用的投屏库

一个模块化、高性能的投屏库，支持多种投屏协议和传输方式，可轻松集成到任何项目中。

## 功能特性

✨ **多协议支持**
- Miracast (WiFi Direct)
- AirPlay
- DLNA
- 有线投屏 (USB)
- 无线投屏 (WiFi)

🎯 **核心特点**
- 清晰的分层架构，应用层与投屏库完全解耦
- 统一的 API 接口，支持任意投屏协议无缝切换
- 观察者模式事件系统，实时反馈投屏状态
- 严格的状态机管理，防止非法状态转移
- 线程安全的实现，支持多线程并发调用
- 易于扩展，新增协议只需实现固定接口

⚡ **性能优化**
- 最小化内存占用
- 高效的事件分发机制
- 异步设备发现
- 支持流式数据传输

🔧 **平台支持**
- Allwinner Cedarx 平台优化
- 通用 Linux 系统兼容
- 易于移植到其他平台

## 快速开始

### 编译库

```bash
cd screenmirror_lib
make all                 # 编译静态库和动态库
make test                # 编译并运行 test/test_main.c
make install-local       # 安装到 ./install 目录
```

> H133 交叉编译请显式指定：`make SYSROOT=/home/share_hp/h133/tina/out/h133/p2/openwrt/staging_dir/target`。

编译产物：
```
build/
├── libscreenmirror.a                    # 静态库
├── libscreenmirror.so.1.0.0            # 动态库
├── libscreenmirror.so.1                # 符号链接
└── libscreenmirror.so                  # 符号链接
```

### 基本使用

```c
#include <screenmirror.h>
#include <stdio.h>
#include <unistd.h>

/* 事件回调函数 */
void mirror_event_callback(const MirrorEvent *event, void *user_data)
{
    switch (event->type) {
        case MIRROR_EVENT_CONNECTED:
            printf("Connected to device\n");
            break;
        case MIRROR_EVENT_DISCONNECTED:
            printf("Disconnected from device\n");
            break;
        case MIRROR_EVENT_ERROR:
            printf("Error: %s\n", event->error_msg);
            break;
        case MIRROR_EVENT_STATE_CHANGED:
            printf("State changed\n");
            break;
        default:
            break;
    }
}

/* 设备发现回调函数 */
void device_discovery_callback(const MirrorDeviceInfo *devices,
                              int device_count, void *user_data)
{
    printf("Found %d devices:\n", device_count);
    for (int i = 0; i < device_count; i++) {
        printf("  [%d] %s (%s) - Signal: %d%%\n",
               i + 1,
               devices[i].name,
               devices[i].mac_address,
               devices[i].signal_strength);
    }
}

int main(int argc, char *argv[])
{
    int ret;

    /* 初始化投屏库 */
    ret = screenmirror_init();
    if (ret < 0) {
        printf("Failed to initialize screenmirror library\n");
        return -1;
    }

    /* 设置事件回调 */
    screenmirror_set_event_callback(mirror_event_callback, NULL);

    /* 开始设备发现 */
    printf("Discovering devices...\n");
    screenmirror_start_discovery(MIRROR_MODE_MIRACAST, 5000,
                                device_discovery_callback, NULL);

    /* 等待发现完成 */
    sleep(6);

    /* 停止发现 */
    screenmirror_stop_discovery();

    /* 模拟选择设备并连接 */
    MirrorDeviceInfo device;
    strcpy(device.name, "Test Device 1");
    strcpy(device.mac_address, "00:11:22:33:44:55");
    strcpy(device.ip_address, "192.168.1.100");
    device.signal_strength = 80;
    device.mode = MIRROR_MODE_MIRACAST;

    /* 配置连接参数 */
    MirrorConfig config;
    config.mode = MIRROR_MODE_MIRACAST;
    config.max_bitrate = 10000;     /* 10 Mbps */
    config.resolution_width = 1920;
    config.resolution_height = 1080;
    config.refresh_rate = 60;
    config.enable_audio = true;
    config.enable_hdcp = true;
    config.connect_timeout_ms = 10000;

    /* 连接设备 */
    printf("Connecting to device: %s\n", device.name);
    ret = screenmirror_connect(&device, &config);
    if (ret < 0) {
        printf("Failed to connect device\n");
    }

    /* 等待连接完成 */
    sleep(3);

    /* 检查当前状态 */
    MirrorState state = screenmirror_get_state();
    printf("Current state: %d\n", state);

    /* 断开连接 */
    printf("Disconnecting...\n");
    screenmirror_disconnect();

    /* 等待断开完成 */
    sleep(1);

    /* 反初始化 */
    screenmirror_exit();

    printf("Done\n");
    return 0;
}
```

### 编译示例

```bash
# 使用静态库
gcc -o example example.c -L./build -lscreenmirror -I./include -pthread

# 或使用动态库
gcc -o example example.c -L./build -lscreenmirror -I./include -pthread
export LD_LIBRARY_PATH=./build:$LD_LIBRARY_PATH
./example
```

## API 文档

### 初始化与清理

#### `int screenmirror_init(void)`
初始化投屏库。必须在使用其他函数前调用。

**返回值：**
- `0` - 成功
- `< 0` - 失败（错误码见 screenmirror_internal.h）

#### `int screenmirror_exit(void)`
反初始化投屏库。需要释放所有资源。

**返回值：**
- `0` - 成功
- `< 0` - 失败

### 事件管理

#### `int screenmirror_set_event_callback(MirrorEventCallback callback, void *user_data)`
设置全局事件回调函数。投屏库将通过此回调报告所有事件。

**参数：**
- `callback` - 事件回调函数指针
- `user_data` - 用户自定义数据，会在回调时传递

**返回值：**
- `0` - 成功
- `< 0` - 失败

### 设备发现

#### `int screenmirror_start_discovery(MirrorMode mode, int timeout_ms, MirrorDeviceListCallback callback, void *user_data)`
启动设备发现。

**参数：**
- `mode` - 投屏模式（MIRROR_MODE_MIRACAST 等）
- `timeout_ms` - 发现超时时间（毫秒）
- `callback` - 发现结果回调函数
- `user_data` - 用户自定义数据

**返回值：**
- `0` - 成功
- `< 0` - 失败

#### `int screenmirror_stop_discovery(void)`
停止设备发现。

**返回值：**
- `0` - 成功
- `< 0` - 失败

### 连接管理

#### `int screenmirror_connect(const MirrorDeviceInfo *device, const MirrorConfig *config)`
连接到指定设备。

**参数：**
- `device` - 目标设备信息
- `config` - 连接配置参数

**返回值：**
- `0` - 成功
- `< 0` - 失败

#### `int screenmirror_disconnect(void)`
断开当前连接。

**返回值：**
- `0` - 成功
- `< 0` - 失败

### 状态查询

#### `MirrorState screenmirror_get_state(void)`
获取当前投屏状态。

**返回值：**
- `MIRROR_STATE_IDLE` - 空闲
- `MIRROR_STATE_DISCOVERING` - 发现中
- `MIRROR_STATE_CONNECTING` - 连接中
- `MIRROR_STATE_CONNECTED` - 已连接
- `MIRROR_STATE_STREAMING` - 传输中
- `MIRROR_STATE_PAUSED` - 已暂停
- `MIRROR_STATE_ERROR` - 错误

#### `const MirrorDeviceInfo* screenmirror_get_device_info(void)`
获取当前连接设备的信息。

**返回值：**
- 设备信息指针，如果未连接返回 NULL

#### `const MirrorConfig* screenmirror_get_config(void)`
获取当前连接的配置参数。

**返回值：**
- 配置参数指针，如果未连接返回 NULL

### 数据传输

#### `int screenmirror_send_video_frame(const uint8_t *data, int size)`
发送视频数据帧。

**参数：**
- `data` - 视频数据指针
- `size` - 数据大小（字节）

**返回值：**
- 已发送的字节数，< 0 表示失败

#### `int screenmirror_send_audio_frame(const uint8_t *data, int size)`
发送音频数据帧。

**参数：**
- `data` - 音频数据指针
- `size` - 数据大小（字节）

**返回值：**
- 已发送的字节数，< 0 表示失败

### 控制命令

#### `int screenmirror_control(const char *command)`
发送控制命令。

**参数：**
- `command` - 命令字符串（如 "pause", "resume", "stop"）

**返回值：**
- `0` - 成功
- `< 0` - 失败

### 版本信息

#### `const char* screenmirror_get_version(void)`
获取库版本号。

**返回值：**
- 版本字符串（如 "libscreenmirror v1.0.0"）

## 数据结构

### MirrorMode（投屏模式）
```c
typedef enum {
    MIRROR_MODE_MIRACAST,    /* Miracast (WiFi Direct) */
    MIRROR_MODE_AIRPLAY,     /* AirPlay */
    MIRROR_MODE_DLNA,        /* DLNA */
    MIRROR_MODE_WIRED,       /* USB 有线投屏 */
    MIRROR_MODE_WIRELESS,    /* WiFi 无线投屏 */
} MirrorMode;
```

### MirrorState（投屏状态）
```c
typedef enum {
    MIRROR_STATE_IDLE,           /* 空闲 */
    MIRROR_STATE_DISCOVERING,    /* 发现中 */
    MIRROR_STATE_CONNECTING,     /* 连接中 */
    MIRROR_STATE_CONNECTED,      /* 已连接 */
    MIRROR_STATE_STREAMING,      /* 传输中 */
    MIRROR_STATE_PAUSED,         /* 已暂停 */
    MIRROR_STATE_ERROR,          /* 错误 */
} MirrorState;
```

### MirrorEvent（事件结构）
```c
typedef struct {
    MirrorEventType type;      /* 事件类型 */
    int error_code;            /* 错误码 */
    const char *error_msg;     /* 错误信息 */
    void *data;                /* 事件数据 */
} MirrorEvent;
```

### MirrorDeviceInfo（设备信息）
```c
typedef struct {
    char name[128];            /* 设备名称 */
    char mac_address[32];      /* MAC 地址 */
    char ip_address[16];       /* IP 地址 */
    int signal_strength;       /* 信号强度 0-100 */
    MirrorMode mode;           /* 投屏模式 */
    uint32_t model_id;         /* 型号 ID */
    void *platform_data;       /* 平台特定数据 */
} MirrorDeviceInfo;
```

### MirrorConfig（连接配置）
```c
typedef struct {
    MirrorMode mode;           /* 投屏模式 */
    int max_bitrate;           /* 最大码率 (Kbps) */
    int resolution_width;      /* 分辨率宽度 */
    int resolution_height;     /* 分辨率高度 */
    int refresh_rate;          /* 刷新率 (Hz) */
    bool enable_audio;         /* 是否启用音频 */
    bool enable_hdcp;          /* 是否启用 HDCP */
    int connect_timeout_ms;    /* 连接超时时间 (ms) */
    void *extra_config;        /* 扩展配置指针 */
} MirrorConfig;
```

## 错误码

```c
#define MIRROR_ERR_SUCCESS           0
#define MIRROR_ERR_ALREADY_INIT     -1    /* 已初始化 */
#define MIRROR_ERR_NOT_INIT         -2    /* 未初始化 */
#define MIRROR_ERR_INVALID_PARAM    -3    /* 参数无效 */
#define MIRROR_ERR_OUT_OF_MEMORY    -4    /* 内存不足 */
#define MIRROR_ERR_CONNECT_FAILED   -5    /* 连接失败 */
#define MIRROR_ERR_TIMEOUT          -6    /* 超时 */
#define MIRROR_ERR_DEVICE_BUSY      -7    /* 设备忙 */
#define MIRROR_ERR_UNKNOWN          -99   /* 未知错误 */
```

## 项目结构

```
screenmirror_lib/
├── Makefile                      # 编译配置
├── README.md                     # 项目文档（本文件）
├── include/
│   ├── screenmirror.h           # 公开 API
│   ├── screenmirror_internal.h  # 内部接口
│   ├── protocols/
│   │   ├── miracast.h
│   │   ├── airplay.h
│   │   ├── dlna.h
│   │   ├── wired.h
│   │   └── wireless.h
│   ├── network/
│   │   ├── discovery.h
│   │   └── transport.h
│   └── platform/
│       └── cedarx_platform.h
├── src/
│   ├── core/
│   │   ├── mirror_engine.c      # 核心引擎
│   │   ├── event_system.c       # 事件系统
│   │   └── state_machine.c      # 状态机
│   ├── protocols/
│   │   ├── miracast/
│   │   ├── airplay/
│   │   ├── dlna/
│   │   ├── wired/
│   │   └── wireless/
│   ├── network/
│   │   ├── discovery.c
│   │   └── transport.c
│   └── platform/
│       └── cedarx_platform.c
└── build/                        # 编译输出目录
    ├── libscreenmirror.a
    ├── libscreenmirror.so.1.0.0
    ├── libscreenmirror.so.1
    └── libscreenmirror.so
```

## 集成到现有项目

### 方式1：使用编译好的库文件

```bash
# 编译库
cd screenmirror_lib
make all

# 安装到系统或项目路径
cp build/libscreenmirror.a /path/to/your/project/lib/
cp build/libscreenmirror.so* /path/to/your/project/lib/
cp include/screenmirror.h /path/to/your/project/include/
```

然后在项目的 Makefile 中：

```makefile
CFLAGS = -I/path/to/include
LDFLAGS = -L/path/to/lib -lscreenmirror -pthread

# 编译
gcc $(CFLAGS) -o myapp myapp.c $(LDFLAGS)
```

### 方式2：直接编译源码

在你的项目 Makefile 中添加投屏库源文件：

```makefile
MIRROR_SRCS = \
    screenmirror_lib/src/core/mirror_engine.c \
    screenmirror_lib/src/core/event_system.c \
    screenmirror_lib/src/core/state_machine.c \
    # ... 其他源文件

CFLAGS = -I./screenmirror_lib/include
SRCS += $(MIRROR_SRCS)
```

## 扩展协议支持

### 添加新的投屏协议

1. 创建新的协议模块文件：
   ```
   screenmirror_lib/include/protocols/my_protocol.h
   screenmirror_lib/src/protocols/my_protocol/my_protocol.c
   ```

2. 实现固定的协议接口（参考 miracast.c）：
   ```c
   typedef struct {
       int (*init)(void);
       void (*exit)(void);
       int (*start_discovery)(int timeout_ms);
       void (*stop_discovery)(void);
       int (*connect)(const MirrorDeviceInfo *device, const MirrorConfig *config);
       void (*disconnect)(void);
       int (*send_video)(const uint8_t *data, int size);
       int (*send_audio)(const uint8_t *data, int size);
       int (*control)(const char *command);
       MirrorState (*get_state)(void);
   } ProtocolOps;
   ```

3. 在 Makefile 中添加新协议的源文件

4. 重新编译库

## 许可证

Apache License 2.0

## 贡献

欢迎提交 Pull Request 或 Issue！

## 联系方式

- 作者：Liu-Eleven
- GitHub：https://github.com/Liu-Eleven/screen_mirror

---

**最后更新：2026-07-04**
