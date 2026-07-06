#ifndef SCREENMIRROR_MANAGER_H
#define SCREENMIRROR_MANAGER_H

#include <QObject>
#include <QString>
#include <QList>
#include <QVariantMap>
#include <QTimer>

extern "C" {
#include "screenmirror.h"
}

class ScreenMirrorManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int state READ getState NOTIFY stateChanged)
    Q_PROPERTY(QString deviceName READ getDeviceName NOTIFY deviceNameChanged)
    Q_PROPERTY(bool audioEnabled READ getAudioEnabled WRITE setAudioEnabled NOTIFY audioEnabledChanged)
    Q_PROPERTY(bool hdcpEnabled READ getHdcpEnabled WRITE setHdcpEnabled NOTIFY hdcpEnabledChanged)
    Q_PROPERTY(int videoQuality READ getVideoQuality WRITE setVideoQuality NOTIFY videoQualityChanged)

public:
    explicit ScreenMirrorManager(QObject *parent = nullptr);
    ~ScreenMirrorManager();

    Q_INVOKABLE bool init();
    Q_INVOKABLE void cleanup();

    Q_INVOKABLE void startDiscovery(int modeIndex, int timeoutMs = 10000);
    Q_INVOKABLE void stopDiscovery();

    Q_INVOKABLE bool connectDevice(int deviceIndex, int modeIndex);
    Q_INVOKABLE void disconnectDevice();

    Q_INVOKABLE void sendCommand(const QString &command);
    Q_INVOKABLE void pauseMirroring();
    Q_INVOKABLE void resumeMirroring();

    int getState() const;
    QString getDeviceName() const;
    QList<QVariantMap> getDiscoveredDevices() const;

    bool getAudioEnabled() const;
    void setAudioEnabled(bool enabled);

    bool getHdcpEnabled() const;
    void setHdcpEnabled(bool enabled);

    int getVideoQuality() const;
    void setVideoQuality(int quality);

    /* Called from C callback – posts event to Qt main thread */
    void handleEvent(const MirrorEvent *event);
    void handleDeviceList(const MirrorDeviceInfo *devices, int count);

signals:
    void stateChanged();
    void deviceNameChanged();
    void audioEnabledChanged();
    void hdcpEnabledChanged();
    void videoQualityChanged();
    void devicesUpdated(QList<QVariantMap> devices);
    void error(QString message);
    void statusMessage(QString message);

private:
    MirrorConfig buildConfig(int modeIndex) const;

    int              m_state;
    QString          m_deviceName;
    bool             m_audioEnabled;
    bool             m_hdcpEnabled;
    int              m_videoQuality;   /* 0=720p, 1=1080p, 2=480p */
    QList<QVariantMap> m_discoveredDevices;
};

#endif // SCREENMIRROR_MANAGER_H
