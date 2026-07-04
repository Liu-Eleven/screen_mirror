# lv_projector 投屏适配层集成指南

## 目录结构

新增的适配层目录：

```text
lv_projector/src/ProjectorAdapter/
├── projector_adapter.h
├── projector_adapter_internal.h
├── projector_adapter.c
├── ui_event_handler.c
├── mirror_event_handler.c
├── examples/basic_integration.c
└── INTEGRATION_GUIDE.md

screenmirror_lib/tests/
└── projector_adapter_test.c
```

## Makefile 修改点

`lv_projector/src/Makefile` 已完成以下接入：

1. 新增 `ProjectorAdapter` 源目录到 `SRCDIRS`
2. 新增 `ProjectorAdapter` 和 `screenmirror_lib/include` 头文件路径
3. 新增 `SCREENMIRROR_DIR`，链接 `libscreenmirror`
4. 在最终链接前自动执行 `$(MAKE) -C $(SCREENMIRROR_DIR)`

## 编译和链接

建议先编译底层库：

```bash
cd screenmirror_lib
make
```

然后在 `lv_projector/src` 中编译：

```bash
make
```

> 当前仓库缺少 `lv_drivers` 等外部依赖时，`lv_projector` 整体工程无法在本沙箱完整链接；适配层本身可通过 `screenmirror_lib` 的 `make test` 独立验证。

## 线程安全说明

- `projector_adapter.c` 使用全局 `pthread_mutex_t` 保护配置、状态和当前设备。
- `mirror_event_handler.c` 仅做状态映射，不直接操作 LVGL 控件。
- `lv_projector` 的 UI 回调中继续通过 `lvgl_mutex` 更新界面，避免跨线程直接调用 LVGL API。

## UI 接入建议

无线投屏页可按以下流程接入：

1. 页面创建时调用 `projector_adapter_init`
2. 注册 `ProjectorAdapterStateCallback` / `ProjectorAdapterDeviceCallback`
3. 在 `projector_ui_on_screen_loaded()` 中启动默认协议发现
4. 用户切换协议时调用 `projector_ui_on_protocol_selected()`
5. 用户选择设备时调用 `projector_ui_on_device_selected()`
6. 页面销毁时调用 `projector_ui_on_screen_unloaded()` 和 `projector_adapter_deinit()`

## 事件流

### UI -> 库

```text
用户点击协议/设备
  -> projector_ui_on_protocol_selected / projector_ui_on_device_selected
  -> projector_adapter_on_protocol_selected / projector_adapter_on_device_selected
  -> screenmirror_start_discovery / screenmirror_connect
```

### 库 -> UI

```text
screenmirror 回调
  -> projector_adapter_handle_mirror_event
  -> ProjectorAdapterStateCallback
  -> LVGL 标签/页面状态更新
```

## 配置构建

`ProjectorAdapterConfig` 负责收集 UI 侧配置：

- 协议类型
- 分辨率 / 刷新率
- 码率
- 音频 / HDCP 开关
- 连接超时
- UI 展示用设备名

适配层在内部自动转换为 `MirrorConfig`，避免 UI 直接依赖底层库结构。
