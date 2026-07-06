# ScreenMirror Demo

基于 **Qt 5.15** 的投屏 Demo 程序，集成 `libscreenmirror` 库，提供完整的 WiFi 管理和投屏功能。

## 项目结构

```
screenmirror_demo/
├── CMakeLists.txt                 # CMake 配置（Qt 5.15）
├── README.md                      # 使用说明
├── src/
│   ├── main.cpp                   # 应用入口
│   ├── screenmirror_manager.h     # 投屏管理 C++ 后端
│   ├── screenmirror_manager.cpp
│   ├── wifi_manager.h             # WiFi 管理 C++ 后端
│   └── wifi_manager.cpp
├── qml/
│   ├── main.qml                   # 主窗口（标签页导航）
│   ├── pages/
│   │   ├── MainPage.qml           # 主页（状态 + 快捷操作 + 日志）
│   │   ├── WiFiPage.qml           # WiFi 管理页
│   │   └── MirrorPage.qml         # 投屏管理页
│   └── components/
│       ├── WiFiListView.qml       # WiFi 列表组件
│       ├── DeviceListView.qml     # 投屏设备列表组件
│       ├── StatusIndicator.qml    # 状态指示器
│       └── ControlPanel.qml       # 控制面板（音频/视频/HDCP/暂停）
└── resources/
    └── qml.qrc                    # Qt 资源文件
```

## 功能概览

| 页面 | 功能 |
|------|------|
| 主页 | 设备名/WiFi/投屏状态显示、快速操作按钮、实时日志 |
| WiFi 管理 | 开启/关闭 WiFi、扫描网络、连接/断开 |
| 投屏管理 | 协议选择（Miracast/AirPlay/DLNA）、设备发现、连接/断开、控制面板 |

## 编译

### 前置条件

- CMake ≥ 3.14
- Qt 5.15（含 QtQuick、QtQml 模块）
- 已编译好的 `screenmirror_lib`（位于 `../screenmirror_lib/`）

### 本机（x86/Linux）调试编译

```bash
cd screenmirror_demo
mkdir -p build && cd build
cmake ..
make -j$(nproc)
./screenmirror_demo
```

> 没有 H133 硬件时，投屏库以 **demo 模式** 运行（函数调用均有日志输出，但不操作真实硬件）。
> WiFi 扫描在没有 `iw` 工具时自动返回模拟网络列表。

### H133 Tina 交叉编译

```bash
# 确保 H133 工具链已激活（source envsetup.sh / lunch）
export H133_SYSROOT=/home/share_hp/h133/tina/out/h133/p2/openwrt/staging_dir/target

# 先编译 libscreenmirror（如未编译）
cd ../screenmirror_lib
make SYSROOT=$H133_SYSROOT all
cd ../screenmirror_demo

# 配置并编译 Demo
mkdir -p build_h133 && cd build_h133
cmake \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/h133-toolchain.cmake \
  -DH133_SYSROOT=$H133_SYSROOT \
  ..
make -j$(nproc)
```

#### 工具链文件示例 (`h133-toolchain.cmake`)

```cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CROSS_PREFIX arm-openwrt-linux-gnueabi-)
set(CMAKE_C_COMPILER   ${CROSS_PREFIX}gcc)
set(CMAKE_CXX_COMPILER ${CROSS_PREFIX}g++)
set(CMAKE_SYSROOT $ENV{H133_SYSROOT})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
```

### 部署到 H133 设备

```bash
# 复制可执行文件
scp build_h133/screenmirror_demo root@<device-ip>:/usr/bin/

# 复制 libscreenmirror 动态库（如未集成到 rootfs）
scp ../screenmirror_lib/build/libscreenmirror.so* root@<device-ip>:/usr/lib/

# 在设备上运行
ssh root@<device-ip>
export QT_QPA_PLATFORM=linuxfb   # 或 eglfs
export LD_LIBRARY_PATH=/usr/lib:$LD_LIBRARY_PATH
screenmirror_demo
```

## UI 使用流程

1. **开启 WiFi** → 切换到「WiFi 管理」页，点击「开启 WiFi」
2. **扫描网络** → 点击「扫描网络」，选择目标 AP 点击「连接」，输入密码
3. **搜索投屏设备** → 切换到「投屏管理」页，选择协议，点击「搜索设备」
4. **连接设备** → 在设备列表中点击「连接」
5. **手机投屏** → 手机开启 Miracast/AirPlay/DLNA，选择本设备即可投屏
6. **控制** → 在右侧控制面板调整音频、视频质量、HDCP，或暂停/继续投屏

## API 说明

### ScreenMirrorManager（QML 属性）

| 属性/方法 | 类型 | 说明 |
|-----------|------|------|
| `state` | `int` | 投屏状态（0=空闲…6=错误） |
| `deviceName` | `string` | 本机设备名称 |
| `audioEnabled` | `bool` | 音频开关 |
| `hdcpEnabled` | `bool` | HDCP 开关 |
| `videoQuality` | `int` | 视频质量（0=720p,1=1080p,2=480p） |
| `init()` | 方法 | 初始化投屏库 |
| `startDiscovery(mode, timeout)` | 方法 | 开始设备发现 |
| `connectDevice(idx, mode)` | 方法 | 连接设备 |
| `disconnectDevice()` | 方法 | 断开连接 |
| `pauseMirroring()` | 方法 | 暂停投屏 |
| `resumeMirroring()` | 方法 | 继续投屏 |

### WiFiManager（QML 属性）

| 属性/方法 | 类型 | 说明 |
|-----------|------|------|
| `isEnabled` | `bool` | WiFi 是否已开启 |
| `connectedSSID` | `string` | 当前已连接的 SSID |
| `isScanning` | `bool` | 是否正在扫描 |
| `startWiFi()` | 方法 | 开启 WiFi |
| `stopWiFi()` | 方法 | 关闭 WiFi |
| `scanNetworks()` | 方法 | 扫描附近网络 |
| `connectNetwork(ssid, pwd)` | 方法 | 连接指定网络 |
| `disconnectNetwork()` | 方法 | 断开当前网络 |
