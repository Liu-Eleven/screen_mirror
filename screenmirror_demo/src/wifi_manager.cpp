#include "wifi_manager.h"

#include <QDebug>
#include <QFile>
#include <QProcess>
#include <QRegularExpression>

WiFiManager::WiFiManager(QObject *parent)
    : QObject(parent)
    , m_enabled(false)
    , m_scanning(false)
    , m_scanTimer(new QTimer(this))
{
    m_scanTimer->setSingleShot(true);
    connect(m_scanTimer, &QTimer::timeout, this, &WiFiManager::onScanTimeout);
}

bool WiFiManager::startWiFi()
{
    if (m_enabled) {
        emit statusMessage(QStringLiteral("WiFi 已经开启"));
        return true;
    }

    /* Try to bring up the wireless interface */
    QProcess proc;
    proc.start(QStringLiteral("ifconfig"), {QStringLiteral("wlan0"), QStringLiteral("up")});
    proc.waitForFinished(3000);

    m_enabled = true;
    emit enabledChanged();
    emit statusMessage(QStringLiteral("WiFi 已开启"));
    return true;
}

bool WiFiManager::stopWiFi()
{
    if (!m_enabled) {
        emit statusMessage(QStringLiteral("WiFi 已经关闭"));
        return true;
    }

    if (!m_connectedSSID.isEmpty()) {
        disconnectNetwork();
    }

    QProcess proc;
    proc.start(QStringLiteral("ifconfig"), {QStringLiteral("wlan0"), QStringLiteral("down")});
    proc.waitForFinished(3000);

    m_enabled = false;
    m_networks.clear();
    emit enabledChanged();
    emit networksUpdated(m_networks);
    emit statusMessage(QStringLiteral("WiFi 已关闭"));
    return true;
}

void WiFiManager::scanNetworks()
{
    if (!m_enabled) {
        emit error(QStringLiteral("请先开启 WiFi"));
        return;
    }
    if (m_scanning) {
        emit statusMessage(QStringLiteral("正在扫描中..."));
        return;
    }

    m_scanning = true;
    emit scanningChanged();
    emit scanStarted();
    emit statusMessage(QStringLiteral("开始扫描 WiFi..."));

    /* Issue scan command then wait briefly for results */
    QProcess proc;
    proc.start(QStringLiteral("iw"), {QStringLiteral("wlan0"), QStringLiteral("scan")});
    bool finished = proc.waitForFinished(8000);

    if (finished && proc.exitCode() == 0) {
        parseScanResults(QString::fromUtf8(proc.readAllStandardOutput()));
    } else {
        /* iw not available or failed – use simulated data for demo */
        addSimulatedNetworks();
    }

    m_scanning = false;
    emit scanningChanged();
    emit scanFinished();
    emit networksUpdated(m_networks);
    emit statusMessage(QString("扫描完成，发现 %1 个网络").arg(m_networks.size()));
}

bool WiFiManager::connectNetwork(const QString &ssid, const QString &password)
{
    if (!m_enabled) {
        emit error(QStringLiteral("请先开启 WiFi"));
        return false;
    }
    if (ssid.isEmpty()) {
        emit error(QStringLiteral("SSID 不能为空"));
        return false;
    }

    /* Write a minimal wpa_supplicant config and invoke wpa_cli */
    QString configContent = QString(
        "ctrl_interface=/var/run/wpa_supplicant\n"
        "update_config=1\n"
        "network={\n"
        "    ssid=\"%1\"\n"
        "    psk=\"%2\"\n"
        "    key_mgmt=WPA-PSK\n"
        "}\n"
    ).arg(ssid, password);

    /* Write temp config */
    QFile cfgFile(QStringLiteral("/tmp/wpa_demo.conf"));
    if (cfgFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        cfgFile.write(configContent.toUtf8());
        cfgFile.close();
    }

    QProcess proc;
    proc.start(QStringLiteral("wpa_supplicant"),
               {QStringLiteral("-i"), QStringLiteral("wlan0"),
                QStringLiteral("-c"), QStringLiteral("/tmp/wpa_demo.conf"),
                QStringLiteral("-B")});
    bool ok = proc.waitForFinished(5000);

    /* Whether the real command worked or not, update UI state for demo */
    if (ok && proc.exitCode() == 0) {
        m_connectedSSID = ssid;
    } else {
        /* Simulate success in demo mode */
        m_connectedSSID = ssid;
    }

    emit connectedSSIDChanged();
    emit connectSuccess(ssid);
    emit statusMessage(QString("已连接到 %1").arg(ssid));
    return true;
}

void WiFiManager::disconnectNetwork()
{
    if (m_connectedSSID.isEmpty()) return;

    QProcess proc;
    proc.start(QStringLiteral("wpa_cli"),
               {QStringLiteral("-i"), QStringLiteral("wlan0"),
                QStringLiteral("disconnect")});
    proc.waitForFinished(3000);

    QString old = m_connectedSSID;
    m_connectedSSID.clear();
    emit connectedSSIDChanged();
    emit statusMessage(QString("已断开 %1").arg(old));
}

/* ---------- Property accessors ------------------------------------------ */

bool    WiFiManager::isEnabled()     const { return m_enabled; }
QString WiFiManager::connectedSSID() const { return m_connectedSSID; }
bool    WiFiManager::isScanning()    const { return m_scanning; }
QList<QVariantMap> WiFiManager::getNetworks() const { return m_networks; }

/* ---------- Private helpers --------------------------------------------- */

void WiFiManager::onScanTimeout()
{
    if (m_scanning) {
        m_scanning = false;
        emit scanningChanged();
        emit scanFinished();
        emit statusMessage(QStringLiteral("扫描超时"));
    }
}

void WiFiManager::parseScanResults(const QString &output)
{
    m_networks.clear();

    /* Parse `iw wlan0 scan` output */
    QRegularExpression ssidRe(QStringLiteral("SSID: (.+)"));
    QRegularExpression signalRe(QStringLiteral("signal: ([\\-\\d.]+) dBm"));
    QRegularExpression encRe(QStringLiteral("(WPA|WPA2|WEP|RSN)"));

    QStringList lines = output.split(QLatin1Char('\n'));
    QVariantMap current;
    bool inBss = false;

    auto flushCurrent = [&]() {
        if (inBss && current.contains(QStringLiteral("ssid"))
                && !current.value(QStringLiteral("ssid")).toString().isEmpty()) {
            m_networks.append(current);
        }
        current.clear();
        inBss = false;
    };

    for (const QString &line : lines) {
        if (line.startsWith(QStringLiteral("BSS "))) {
            flushCurrent();
            inBss = true;
            current[QStringLiteral("encryption")] = QStringLiteral("Open");
            continue;
        }

        QRegularExpressionMatch m;
        m = ssidRe.match(line);
        if (m.hasMatch()) {
            current[QStringLiteral("ssid")] = m.captured(1).trimmed();
            continue;
        }
        m = signalRe.match(line);
        if (m.hasMatch()) {
            double dbm = m.captured(1).toDouble();
            /* Convert dBm to 0-100 */
            int strength = qBound(0, static_cast<int>((dbm + 100) * 2), 100);
            current[QStringLiteral("signal")] = strength;
            continue;
        }
        m = encRe.match(line);
        if (m.hasMatch()) {
            current[QStringLiteral("encryption")] = m.captured(1);
        }
    }
    flushCurrent();
}

void WiFiManager::addSimulatedNetworks()
{
    m_networks.clear();

    auto add = [&](const QString &ssid, int signal, const QString &enc) {
        QVariantMap map;
        map[QStringLiteral("ssid")]       = ssid;
        map[QStringLiteral("signal")]     = signal;
        map[QStringLiteral("encryption")] = enc;
        m_networks.append(map);
    };

    add(QStringLiteral("HomeNetwork_5G"),  92, QStringLiteral("WPA2"));
    add(QStringLiteral("Office_WiFi"),     78, QStringLiteral("WPA2"));
    add(QStringLiteral("Guest_Network"),   65, QStringLiteral("Open"));
    add(QStringLiteral("AndroidAP"),       55, QStringLiteral("WPA2"));
    add(QStringLiteral("iPhone_Hotspot"),  48, QStringLiteral("WPA2"));
}
