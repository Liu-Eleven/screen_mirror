#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDebug>

#include "screenmirror_manager.h"
#include "wifi_manager.h"

int main(int argc, char *argv[])
{
    /* Enable High DPI scaling for embedded displays */
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("ScreenMirror Demo"));
    app.setApplicationVersion(QStringLiteral("1.0.0"));
    app.setOrganizationName(QStringLiteral("H133 Demo"));

    /* Instantiate C++ backend objects */
    ScreenMirrorManager mirrorManager;
    WiFiManager         wifiManager;

    /* Expose to QML */
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("mirrorManager"), &mirrorManager);
    engine.rootContext()->setContextProperty(QStringLiteral("wifiManager"),   &wifiManager);

    /* Load main QML file */
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load QML root";
        return -1;
    }

    /* Initialise the screenmirror library after the UI is ready */
    if (!mirrorManager.init()) {
        qWarning() << "ScreenMirrorManager::init() failed – running in demo mode";
    }

    return app.exec();
}
