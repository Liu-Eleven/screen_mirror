# libscreenmirror v2.0 — Allwinner H133 投屏库

高性能、模块化的投屏库。内部集成 Allwinner AWCast/USBCast 完整协议栈，对外保持统一的纯 C 公开 API。

## 功能特性

✨ **多协议支持**
- **Miracast** — AWCast `libwfd2.so` + P2P WiFi Direct（`ENABLE_MIRACAST`）
- **DLNA / UPnP** — AWCast `libawdlna.so`（`ENABLE_DLNA`）
- **AirPlay** — AWCast `libthirdparty_mirror.so` dlopen（`ENABLE_AIRPLAY`）
- **USB 有线投屏** — USBCast `WSP_*`，dlopen Android/AirPlay 协议（`ENABLE_WIRED`）
- **AWCast All-In-One** — 同时启动三种无线协议（`MIRROR_MODE_WIRELESS`）

🔌 **连接管理**
- **WiFi 管理** — 基于 `libwifimg.so`，支持 STA / P2P 并发模式（`ENABLE_WIFI`）
- **蓝牙管理** — 基于 `libbtmanager.so`，A2DP Source/Sink 模式（`ENABLE_BT`）

🎯 **核心特点**
- 公开 API 不变（`screenmirror.h` 中所有 `screenmirror_*` 函数签名保持向后兼容）
- **完全剥离 LVGL 依赖** — 纯 C/C++，可在任意嵌入式 Linux 平台编译
- 条件编译宏 — 本地（无 H133 BSP）编译自动使用 stub，不需要任何 BSP 库
- 线程安全事件系统（消息队列 + mutex）
- 严格状态机管理，防止非法状态转移

---

## 快速开始

### 本地 Host 编译（stub 模式，无需 H133）

```bash
cd screenmirror_lib
make all
```

编译产物：
```
build/
├── libscreenmirror.a                      # 静态库
├── libscreenmirror.so.2.0.0               # 动态库
├── libscreenmirror.so.2                   # 符号链接
└── libscreenmirror.so                     # 符号链接
```

### H133 交叉编译（完整 BSP 功能）

```bash
cd screenmirror_lib
make all \
    CROSS_COMPILE=arm-linux-gnueabihf- \
    H133_SYSROOT=/opt/h133-sysroot \
    ENABLE_WIFI=1 \
    ENABLE_BT=1 \
    ENABLE_MIRACAST=1 \
    ENABLE_DLNA=1 \
    ENABLE_AIRPLAY=1 \
    ENABLE_WIRED=1
```

### CMake 编译

```bash
# 本地编译
mkdir build && cd build
cmake ..
make

# H133 交叉编译
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=/path/to/arm-toolchain.cmake \
    -DH133_SYSROOT=/opt/h133-sysroot \
    -DH133_BOARD=ON \
    -DENABLE_WIFI=ON \
    -DENABLE_BT=ON \
    -DENABLE_MIRACAST=ON \
    -DENABLE_DLNA=ON \
    -DENABLE_AIRPLAY=ON \
    -DENABLE_WIRED=ON
make
```

### 编译验证

```bash
make test    # 编译并运行 examples/basic_example.c
```

---

## 编译配置宏

| 宏 | 说明 | 依赖库 |
|----|------|--------|
| `H133_BOARD` | 启用 H133 BSP 硬件支持（自动启用当 H133_SYSROOT 设置时） | — |
| `ENABLE_WIFI` | WiFi 管理（STA/AP/P2P） | `libwifimg.so` |
| `ENABLE_BT` | 蓝牙管理（A2DP Source/Sink） | `libbtmanager.so` |
| `ENABLE_MIRACAST` | Miracast P2P 投屏 | `libwfd2.so`, `libwpa_client.so` |
| `ENABLE_DLNA` | DLNA UPnP 投屏 | `libawdlna.so` |
| `ENABLE_AIRPLAY` | AirPlay 投屏 | `libthirdparty_mirror.so`（运行时 dlopen） |
| `ENABLE_WIRED` | USB 有线投屏 | `libthirdparty_mirror.so`（运行时 dlopen） |

未定义上述宏时，各协议模块编译为空 stub，本地测试正常通过。

---

## 基本使用

### 1. 初始化和事件监听

```c
#include <screenmirror.h>
#include <stdio.h>
#include <unistd.h>

void on_mirror_event(const MirrorEvent *event, void *user_data)
{
    switch (event->type) {
    case MIRROR_EVENT_CONNECTED:
        printf("[投屏] 已连接设备\n");
        break;
    case MIRROR_EVENT_DISCONNECTED:
        printf("[投屏] 连接断开\n");
        break;
    case MIRROR_EVENT_ERROR:
        printf("[投屏] 错误: %s (code=%d)\n",
               event->error_msg, event->error_code);
        break;
    case MIRROR_EVENT_STATE_CHANGED:
        printf("[投屏] 状态变化: %d\n",
               screenmirror_get_state());
        break;
    default:
        break;
    }
}

int main(void)
{
    /* 初始化库 */
    if (screenmirror_init() < 0) {
        fprintf(stderr, "screenmirror_init() 失败\n");
        return -1;
    }

    /* 注册事件回调 */
    screenmirror_set_event_callback(on_mirror_event, NULL);

    printf("库版本: %s\n", screenmirror_get_version());
    /* 输出: 库版本: libscreenmirror v2.0.0 */

    /* ... 业务逻辑 ... */

    screenmirror_exit();
    return 0;
}
```

### 2. Miracast 设备发现与连接

```c
void on_devices_found(const MirrorDeviceInfo *devices,
                      int count, void *user_data)
{
    printf("发现 %d 台设备:\n", count);
    for (int i = 0; i < count; i++) {
        printf("  [%d] %s  MAC: %s  信号: %d%%\n",
               i, devices[i].name,
               devices[i].mac_address,
               devices[i].signal_strength);
    }
}

/* 开始 Miracast 设备发现（超时 5 秒） */
screenmirror_start_discovery(MIRROR_MODE_MIRACAST, 5000,
                             on_devices_found, NULL);
sleep(6);
screenmirror_stop_discovery();

/* 选择设备并连接 */
MirrorDeviceInfo dev = {0};
strcpy(dev.name, "Living Room TV");
strcpy(dev.mac_address, "AA:BB:CC:DD:EE:FF");
dev.mode = MIRROR_MODE_MIRACAST;

MirrorConfig cfg = {0};
cfg.mode             = MIRROR_MODE_MIRACAST;
cfg.max_bitrate      = 10000;    /* 10 Mbps */
cfg.resolution_width = 1920;
cfg.resolution_height = 1080;
cfg.refresh_rate     = 60;
cfg.enable_audio     = true;
cfg.enable_hdcp      = true;     /* 需要 /data/miracast.dat HDCP key */
cfg.connect_timeout_ms = 10000;

screenmirror_connect(&dev, &cfg);
/* 等待 MIRROR_EVENT_CONNECTED 回调 */
sleep(5);

screenmirror_disconnect();
```

### 3. 同时启动所有无线协议（AWCast All-In-One）

```c
/* MIRROR_MODE_WIRELESS 同时启动 Miracast + DLNA + AirPlay */
MirrorDeviceInfo dev = {0};
MirrorConfig cfg = {0};
cfg.mode = MIRROR_MODE_WIRELESS;

screenmirror_connect(&dev, &cfg);
/* 设备现在可被手机以 Miracast、DLNA 或 AirPlay 三种方式投屏 */
```

### 4. USB 有线投屏

```c
MirrorDeviceInfo dev = {0};
strcpy(dev.name, "USB Device");
dev.mode = MIRROR_MODE_WIRED;

MirrorConfig cfg = {0};
cfg.mode = MIRROR_MODE_WIRED;

screenmirror_connect(&dev, &cfg);
/* 等待手机 USB 连接 */
```

### 5. WiFi 管理（需要 `ENABLE_WIFI`）

```c
#include "connectivity/wifi_core.h"

void on_wifi_event(WifiEvent event, const WifiEventData *data,
                  void *user_data)
{
    switch (event) {
    case WIFI_EVENT_SCAN_RESULT:
        printf("扫描到 %d 个 AP\n", data->scan.ap_count);
        for (int i = 0; i < data->scan.ap_count; i++) {
            printf("  SSID: %-32s  信号: %d dBm\n",
                   data->scan.aps[i].ssid,
                   data->scan.aps[i].rssi);
        }
        break;
    case WIFI_EVENT_CONNECTED:
        printf("WiFi 已连接: %s  IP: %s\n",
               data->conn.ssid, data->conn.ip_addr);
        break;
    case WIFI_EVENT_DISCONNECTED:
        printf("WiFi 断开\n");
        break;
    default:
        break;
    }
}

wifi_core_init(on_wifi_event, NULL);

/* 开启 WiFi（STA + P2P 并发，支持 Miracast） */
wifi_core_on();

/* 扫描 */
wifi_core_scan();

/* 连接 */
wifi_core_connect("MySSID", "MyPassword");

/* 开启 AP 模式（SSID: allwinner-ap，密码: Aa123456，信道: 6） */
/* wifi_core_ap_on("allwinner-ap", "Aa123456", 6); */

wifi_core_off();
wifi_core_deinit();
```

### 6. 蓝牙管理（需要 `ENABLE_BT`）

```c
#include "connectivity/bt_core.h"

void on_bt_event(BtEvent event, const BtEventData *data,
                void *user_data)
{
    switch (event) {
    case BT_EVENT_SCAN_RESULT:
        printf("BT 扫描到 %d 台设备\n", data->scan.device_count);
        for (int i = 0; i < data->scan.device_count; i++) {
            printf("  %s  %s\n",
                   data->scan.devices[i].name,
                   data->scan.devices[i].addr);
        }
        break;
    case BT_EVENT_CONNECTED:
        printf("BT 已连接: %s\n", data->conn.device_name);
        break;
    case BT_EVENT_DISCONNECTED:
        printf("BT 断开\n");
        break;
    default:
        break;
    }
}

/* sink_mode=false: A2DP Source（投影仪输出音频到 BT 音箱） */
/* sink_mode=true:  A2DP Sink（手机推送音频到投影仪）       */
bt_core_init(on_bt_event, NULL, /*sink_mode=*/false);

bt_core_on();
bt_core_scan();

/* 连接配对设备 */
bt_core_connect("AA:BB:CC:DD:EE:FF");

bt_core_off();
bt_core_deinit();
```

---

## API 参考

### 核心初始化

| 函数 | 说明 |
|------|------|
| `screenmirror_init()` | 初始化库，分配内部资源 |
| `screenmirror_exit()` | 释放所有资源，停止所有协议 |
| `screenmirror_get_version()` | 返回版本字符串（`"libscreenmirror v2.0.0"`） |

### 事件

| 函数 | 说明 |
|------|------|
| `screenmirror_set_event_callback(cb, data)` | 注册全局事件回调 |

事件类型（`MirrorEventType`）：

| 值 | 含义 |
|----|------|
| `MIRROR_EVENT_STATE_CHANGED` | 状态机状态变化 |
| `MIRROR_EVENT_DEVICE_FOUND` | 发现新设备 |
| `MIRROR_EVENT_CONNECTED` | 投屏连接建立 |
| `MIRROR_EVENT_DISCONNECTED` | 投屏连接断开 |
| `MIRROR_EVENT_ERROR` | 错误发生 |
| `MIRROR_EVENT_STREAM_STARTED` | 流传输开始 |
| `MIRROR_EVENT_STREAM_STOPPED` | 流传输停止 |

### 设备发现

| 函数 | 说明 |
|------|------|
| `screenmirror_start_discovery(mode, timeout_ms, cb, data)` | 启动协议对应的设备发现 |
| `screenmirror_stop_discovery()` | 停止设备发现 |

### 连接管理

| 函数 | 说明 |
|------|------|
| `screenmirror_connect(device, config)` | 连接指定设备（或以服务器模式等待） |
| `screenmirror_disconnect()` | 断开当前连接 |

### 状态查询

| 函数 | 说明 |
|------|------|
| `screenmirror_get_state()` | 返回当前 `MirrorState` |
| `screenmirror_get_device_info()` | 返回已连接设备信息（NULL 表示未连接） |
| `screenmirror_get_config()` | 返回当前连接配置（NULL 表示未连接） |

### 数据传输

| 函数 | 说明 |
|------|------|
| `screenmirror_send_video_frame(data, size)` | 推送视频帧数据 |
| `screenmirror_send_audio_frame(data, size)` | 推送音频帧数据 |
| `screenmirror_control(command)` | 发送控制命令（`"pause"`, `"resume"`, `"stop"`, `"reset"`） |

---

## 项目结构

```
screenmirror_lib/
├── Makefile                           # 编译系统（支持交叉编译和 BSP 特性开关）
├── CMakeLists.txt                     # CMake 构建（同等特性支持）
├── README.md                          # 本文件
├── include/
│   ├── screenmirror.h                 # 公开 API（不变）
│   ├── screenmirror_internal.h        # 内部接口（ProtocolOps 等）
│   ├── bsp/
│   │   └── awcast_api.h               # AWCast/USBCast BSP 符号声明（H133_BOARD）
│   ├── connectivity/
│   │   ├── wifi_core.h                # WiFi 管理 API
│   │   └── bt_core.h                  # 蓝牙管理 API
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
│   │   ├── mirror_engine.c            # 协议分发、生命周期管理
│   │   ├── event_system.c             # 线程安全事件系统
│   │   └── state_machine.c            # 状态机
│   ├── connectivity/
│   │   ├── wifi_core.c                # WiFi（libwifimg，无 LVGL）
│   │   └── bt_core.c                  # 蓝牙（libbtmanager，无 LVGL）
│   ├── protocols/
│   │   ├── miracast/miracast.c        # AWCast Miracast（libwfd2）
│   │   ├── airplay/airplay.c          # AirPlay stub（由 AWCast 统一管理）
│   │   ├── dlna/dlna.c                # AWCast DLNA（libawdlna）
│   │   ├── wired/wired.c              # USBCast 有线投屏
│   │   └── wireless/wireless.c        # AWCast All-In-One 无线投屏
│   ├── network/
│   │   ├── discovery.c
│   │   └── transport.c
│   └── platform/
│       └── cedarx_platform.c
├── examples/
│   └── basic_example.c                # 验证示例
└── build/                             # 编译产物（gitignore）
    ├── libscreenmirror.a
    ├── libscreenmirror.so.2.0.0
    ├── libscreenmirror.so.2
    └── libscreenmirror.so
```

---

## 集成到你的项目

### Makefile 方式

```makefile
SCREENMIRROR_DIR = path/to/screenmirror_lib
CFLAGS  += -I$(SCREENMIRROR_DIR)/include
LDFLAGS += -L$(SCREENMIRROR_DIR)/build -lscreenmirror -pthread -ldl

myapp: myapp.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
```

### CMake 方式

```cmake
add_subdirectory(screenmirror_lib)
target_link_libraries(myapp screenmirror_static)
```

---

## H133 开发注意事项

1. **HDCP Key** — Miracast HDCP 加密需要 `/data/miracast.dat`（902 字节），  
   若文件不存在则以无 HDCP 模式运行（依然功能正常，部分内容保护内容不可用）

2. **WiFi 模式** — `wifi_core_on()` 使用 `WIFI_STATION_P2P` 并发模式，  
   同时支持 STA（上网）和 P2P（Miracast 发现）

3. **蓝牙模式** — 默认 A2DP Source（投影仪输出音频到 BT 音箱）；  
   设置 `sink_mode=true` 改为 A2DP Sink（接收手机音频）

4. **运行时 dlopen** — AirPlay 和 Android 有线协议在运行时动态加载，  
   库路径：`/usr/lib/libthirdparty_mirror.so`

5. **线程安全** — 所有公开 API 内部使用 mutex 保护，可从多个线程调用

---

## 许可证

Apache License 2.0

## 联系

- 作者：Liu-Eleven
- GitHub：https://github.com/Liu-Eleven/screen_mirror

---

**最后更新：2026-07-06**

