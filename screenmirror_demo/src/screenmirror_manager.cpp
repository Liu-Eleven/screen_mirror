#include "screenmirror_manager.h"

#include <QDebug>
#include <QMetaObject>
#include <QThread>
#include <cstring>

/* ---------- static C callbacks (called from screenmirror lib thread) ---- */

static void s_eventCallback(const MirrorEvent *event, void *userData)
{
    auto *mgr = static_cast<ScreenMirrorManager *>(userData);
    if (mgr) {
        /* Forward to the manager on the Qt main thread */
        QMetaObject::invokeMethod(mgr, [mgr, eventCopy = *event]() mutable {
            mgr->handleEvent(&eventCopy);
        }, Qt::QueuedConnection);
    }
}

static void s_deviceListCallback(const MirrorDeviceInfo *devices,
                                  int deviceCount, void *userData)
{
    auto *mgr = static_cast<ScreenMirrorManager *>(userData);
    if (!mgr || !devices || deviceCount <= 0)
        return;

    /* Copy device list so it is safe to use across threads */
    QVector<MirrorDeviceInfo> copy;
    copy.reserve(deviceCount);
    for (int i = 0; i < deviceCount; ++i)
        copy.append(devices[i]);

    QMetaObject::invokeMethod(mgr, [mgr, copy]() {
        mgr->handleDeviceList(copy.constData(), copy.size());
    }, Qt::QueuedConnection);
}

/* ---------- ScreenMirrorManager ----------------------------------------- */

ScreenMirrorManager::ScreenMirrorManager(QObject *parent)
    : QObject(parent)
    , m_state(MIRROR_STATE_IDLE)
    , m_deviceName(QStringLiteral("H133 Device"))
    , m_audioEnabled(true)
    , m_hdcpEnabled(false)
    , m_videoQuality(0)
{
}

ScreenMirrorManager::~ScreenMirrorManager()
{
    cleanup();
}

bool ScreenMirrorManager::init()
{
    int ret = screenmirror_init();
    if (ret != 0) {
        emit error(QString("screenmirror_init failed: %1").arg(ret));
        return false;
    }

    ret = screenmirror_set_event_callback(s_eventCallback, this);
    if (ret != 0) {
        emit error(QString("screenmirror_set_event_callback failed: %1").arg(ret));
        return false;
    }

    emit statusMessage(QStringLiteral("投屏库初始化成功"));
    return true;
}

void ScreenMirrorManager::cleanup()
{
    screenmirror_exit();
    emit statusMessage(QStringLiteral("投屏库已清理"));
}

void ScreenMirrorManager::startDiscovery(int modeIndex, int timeoutMs)
{
    auto mode = static_cast<MirrorMode>(modeIndex);
    m_discoveredDevices.clear();

    int ret = screenmirror_start_discovery(mode, timeoutMs, s_deviceListCallback, this);
    if (ret != 0) {
        emit error(QString("开始搜索失败: %1").arg(ret));
        return;
    }
    emit statusMessage(QStringLiteral("正在搜索设备..."));
}

void ScreenMirrorManager::stopDiscovery()
{
    screenmirror_stop_discovery();
    emit statusMessage(QStringLiteral("已停止搜索"));
}

bool ScreenMirrorManager::connectDevice(int deviceIndex, int modeIndex)
{
    if (deviceIndex < 0 || deviceIndex >= m_discoveredDevices.size()) {
        emit error(QStringLiteral("无效的设备索引"));
        return false;
    }

    MirrorConfig cfg = buildConfig(modeIndex);
    const QVariantMap &devMap = m_discoveredDevices.at(deviceIndex);

    MirrorDeviceInfo dev{};
    QByteArray name = devMap.value(QStringLiteral("name")).toString().toUtf8();
    QByteArray ip   = devMap.value(QStringLiteral("ip")).toString().toUtf8();
    QByteArray mac  = devMap.value(QStringLiteral("mac")).toString().toUtf8();
    strncpy(dev.name,        name.constData(), sizeof(dev.name) - 1);
    strncpy(dev.ip_address,  ip.constData(),   sizeof(dev.ip_address) - 1);
    strncpy(dev.mac_address, mac.constData(),  sizeof(dev.mac_address) - 1);
    dev.signal_strength = devMap.value(QStringLiteral("signal")).toInt();
    dev.mode = static_cast<MirrorMode>(modeIndex);

    int ret = screenmirror_connect(&dev, &cfg);
    if (ret != 0) {
        emit error(QString("连接设备失败: %1").arg(ret));
        return false;
    }
    emit statusMessage(QString("正在连接: %1").arg(devMap.value(QStringLiteral("name")).toString()));
    return true;
}

void ScreenMirrorManager::disconnectDevice()
{
    screenmirror_disconnect();
    emit statusMessage(QStringLiteral("已断开连接"));
}

void ScreenMirrorManager::sendCommand(const QString &command)
{
    QByteArray ba = command.toUtf8();
    screenmirror_control(ba.constData());
}

void ScreenMirrorManager::pauseMirroring()
{
    sendCommand(QStringLiteral("pause"));
    emit statusMessage(QStringLiteral("投屏已暂停"));
}

void ScreenMirrorManager::resumeMirroring()
{
    sendCommand(QStringLiteral("resume"));
    emit statusMessage(QStringLiteral("投屏已继续"));
}

/* ---------- Property accessors ------------------------------------------ */

int ScreenMirrorManager::getState() const
{
    return m_state;
}

QString ScreenMirrorManager::getDeviceName() const
{
    return m_deviceName;
}

QList<QVariantMap> ScreenMirrorManager::getDiscoveredDevices() const
{
    return m_discoveredDevices;
}

bool ScreenMirrorManager::getAudioEnabled() const { return m_audioEnabled; }
void ScreenMirrorManager::setAudioEnabled(bool enabled)
{
    if (m_audioEnabled == enabled) return;
    m_audioEnabled = enabled;
    emit audioEnabledChanged();
    sendCommand(enabled ? QStringLiteral("audio_on") : QStringLiteral("audio_off"));
}

bool ScreenMirrorManager::getHdcpEnabled() const { return m_hdcpEnabled; }
void ScreenMirrorManager::setHdcpEnabled(bool enabled)
{
    if (m_hdcpEnabled == enabled) return;
    m_hdcpEnabled = enabled;
    emit hdcpEnabledChanged();
}

int ScreenMirrorManager::getVideoQuality() const { return m_videoQuality; }
void ScreenMirrorManager::setVideoQuality(int quality)
{
    if (m_videoQuality == quality) return;
    m_videoQuality = quality;
    emit videoQualityChanged();
}

/* ---------- Event handlers (called on Qt main thread) ------------------- */

void ScreenMirrorManager::handleEvent(const MirrorEvent *event)
{
    if (!event) return;

    switch (event->type) {
    case MIRROR_EVENT_CONNECTED:
        m_state = MIRROR_STATE_CONNECTED;
        emit stateChanged();
        emit statusMessage(QStringLiteral("连接成功，开始投屏"));
        break;

    case MIRROR_EVENT_DISCONNECTED:
        m_state = MIRROR_STATE_IDLE;
        emit stateChanged();
        emit statusMessage(QStringLiteral("已断开连接"));
        break;

    case MIRROR_EVENT_ERROR:
        m_state = MIRROR_STATE_ERROR;
        emit stateChanged();
        emit error(event->error_msg
                   ? QString::fromUtf8(event->error_msg)
                   : QStringLiteral("未知错误"));
        break;

    case MIRROR_EVENT_STATE_CHANGED: {
        MirrorState newState = screenmirror_get_state();
        m_state = static_cast<int>(newState);
        emit stateChanged();
        break;
    }

    case MIRROR_EVENT_DEVICE_FOUND:
        emit statusMessage(QStringLiteral("发现新设备"));
        break;

    case MIRROR_EVENT_DISCOVERY_FINISHED:
        emit statusMessage(QString("搜索完成，共发现 %1 个设备")
                           .arg(m_discoveredDevices.size()));
        break;

    default:
        break;
    }
}

void ScreenMirrorManager::handleDeviceList(const MirrorDeviceInfo *devices, int count)
{
    m_discoveredDevices.clear();
    for (int i = 0; i < count; ++i) {
        const MirrorDeviceInfo &d = devices[i];
        QVariantMap map;
        map[QStringLiteral("name")]   = QString::fromUtf8(d.name);
        map[QStringLiteral("mac")]    = QString::fromUtf8(d.mac_address);
        map[QStringLiteral("ip")]     = QString::fromUtf8(d.ip_address);
        map[QStringLiteral("signal")] = d.signal_strength;
        map[QStringLiteral("mode")]   = static_cast<int>(d.mode);
        m_discoveredDevices.append(map);
    }
    emit devicesUpdated(m_discoveredDevices);
}

/* ---------- Private helpers --------------------------------------------- */

MirrorConfig ScreenMirrorManager::buildConfig(int modeIndex) const
{
    MirrorConfig cfg{};
    cfg.mode         = static_cast<MirrorMode>(modeIndex);
    cfg.enable_audio = m_audioEnabled;
    cfg.enable_hdcp  = m_hdcpEnabled;
    cfg.connect_timeout_ms = 10000;

    switch (m_videoQuality) {
    case 1: /* 1080p */
        cfg.resolution_width  = 1920;
        cfg.resolution_height = 1080;
        cfg.max_bitrate       = 8000;
        cfg.refresh_rate      = 30;
        break;
    case 2: /* 480p */
        cfg.resolution_width  = 854;
        cfg.resolution_height = 480;
        cfg.max_bitrate       = 2000;
        cfg.refresh_rate      = 30;
        break;
    default: /* 720p */
        cfg.resolution_width  = 1280;
        cfg.resolution_height = 720;
        cfg.max_bitrate       = 4000;
        cfg.refresh_rate      = 30;
        break;
    }
    return cfg;
}
