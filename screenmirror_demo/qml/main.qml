import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ApplicationWindow {
    id: root
    visible: true
    width: 800
    height: 480
    title: qsTr("ScreenMirror Demo")

    // Global colour palette
    readonly property color colorBg:       "#1A1A2E"
    readonly property color colorPanel:    "#16213E"
    readonly property color colorAccent:   "#0F3460"
    readonly property color colorHighlight:"#E94560"
    readonly property color colorText:     "#EAEAEA"
    readonly property color colorSubText:  "#A0A0B0"

    background: Rectangle { color: root.colorBg }

    // -------- Tab bar --------
    header: TabBar {
        id: tabBar
        background: Rectangle { color: root.colorPanel }

        TabButton {
            text: qsTr("主页")
            width: implicitWidth
            contentItem: Text {
                text: parent.text
                color: tabBar.currentIndex === 0 ? root.colorHighlight : root.colorSubText
                font.pixelSize: 14
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                color: "transparent"
                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width; height: 2
                    color: tabBar.currentIndex === 0 ? root.colorHighlight : "transparent"
                }
            }
        }

        TabButton {
            text: qsTr("WiFi 管理")
            width: implicitWidth
            contentItem: Text {
                text: parent.text
                color: tabBar.currentIndex === 1 ? root.colorHighlight : root.colorSubText
                font.pixelSize: 14
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                color: "transparent"
                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width; height: 2
                    color: tabBar.currentIndex === 1 ? root.colorHighlight : "transparent"
                }
            }
        }

        TabButton {
            text: qsTr("投屏管理")
            width: implicitWidth
            contentItem: Text {
                text: parent.text
                color: tabBar.currentIndex === 2 ? root.colorHighlight : root.colorSubText
                font.pixelSize: 14
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                color: "transparent"
                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width; height: 2
                    color: tabBar.currentIndex === 2 ? root.colorHighlight : "transparent"
                }
            }
        }
    }

    // -------- Page stack --------
    StackLayout {
        anchors.fill: parent
        currentIndex: tabBar.currentIndex

        MainPage  { id: mainPage }
        WiFiPage  { id: wifiPage }
        MirrorPage{ id: mirrorPage }
    }
}
