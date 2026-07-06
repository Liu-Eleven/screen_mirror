#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <QObject>
#include <QString>
#include <QList>
#include <QVariantMap>
#include <QProcess>
#include <QTimer>

/**
 * WiFiManager – manages WiFi on the H133 / Linux platform.
 *
 * On H133 (OpenWrt/Tina) the underlying implementation delegates to
 * `wpa_supplicant` / `hostapd` via shell commands.  On desktop Linux the
 * same shell commands are used; they will simply fail gracefully when the
 * tools are not present, allowing the UI to work in a simulated mode.
 */
class WiFiManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isEnabled READ isEnabled NOTIFY enabledChanged)
    Q_PROPERTY(QString connectedSSID READ connectedSSID NOTIFY connectedSSIDChanged)
    Q_PROPERTY(bool isScanning READ isScanning NOTIFY scanningChanged)

public:
    explicit WiFiManager(QObject *parent = nullptr);
    ~WiFiManager() override = default;

    Q_INVOKABLE bool startWiFi();
    Q_INVOKABLE bool stopWiFi();
    Q_INVOKABLE void scanNetworks();
    Q_INVOKABLE bool connectNetwork(const QString &ssid, const QString &password);
    Q_INVOKABLE void disconnectNetwork();

    bool    isEnabled()      const;
    QString connectedSSID()  const;
    bool    isScanning()     const;
    QList<QVariantMap> getNetworks() const;

signals:
    void enabledChanged();
    void connectedSSIDChanged();
    void scanningChanged();
    void networksUpdated(QList<QVariantMap> networks);
    void error(QString message);
    void scanStarted();
    void scanFinished();
    void connectSuccess(QString ssid);
    void statusMessage(QString message);

private slots:
    void onScanTimeout();

private:
    void runCommand(const QString &cmd, bool async = false);
    void parseScanResults(const QString &output);
    void addSimulatedNetworks();

    bool    m_enabled;
    bool    m_scanning;
    QString m_connectedSSID;
    QList<QVariantMap> m_networks;
    QTimer  *m_scanTimer;
};

#endif // WIFI_MANAGER_H
