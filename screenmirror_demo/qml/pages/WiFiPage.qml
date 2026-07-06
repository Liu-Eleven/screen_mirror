import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root

    // ---- Connect dialog ----
    property string pendingSSID: ""

    Dialog {
        id: connectDialog
        title: qsTr("连接 WiFi")
        modal: true
        anchors.centerIn: parent
        width: Math.min(parent.width - 40, 360)

        background: Rectangle {
            color: "#16213E"
            radius: 10
            border.color: "#0F3460"
            border.width: 1
        }

        header: Item {
            height: 48
            Text {
                anchors.centerIn: parent
                text: connectDialog.title
                color: "#EAEAEA"
                font.pixelSize: 16
                font.bold: true
            }
        }

        contentItem: ColumnLayout {
            spacing: 12
            anchors.margins: 16

            Text {
                text: qsTr("SSID")
                color: "#A0A0B0"
                font.pixelSize: 12
            }
            TextField {
                id: ssidField
                Layout.fillWidth: true
                placeholderText: qsTr("网络名称")
                text: root.pendingSSID
                color: "#EAEAEA"
                placeholderTextColor: "#666"
                font.pixelSize: 13
                background: Rectangle {
                    color: "#0F3460"
                    radius: 6
                    border.color: ssidField.activeFocus ? "#E94560" : "#1A2B4A"
                }
                leftPadding: 10; rightPadding: 10
            }

            Text {
                text: qsTr("密码")
                color: "#A0A0B0"
                font.pixelSize: 12
            }
            TextField {
                id: passwordField
                Layout.fillWidth: true
                placeholderText: qsTr("WiFi 密码")
                echoMode: TextInput.Password
                color: "#EAEAEA"
                placeholderTextColor: "#666"
                font.pixelSize: 13
                background: Rectangle {
                    color: "#0F3460"
                    radius: 6
                    border.color: passwordField.activeFocus ? "#E94560" : "#1A2B4A"
                }
                leftPadding: 10; rightPadding: 10
            }
        }

        footer: RowLayout {
            spacing: 10
            anchors { left: parent.left; right: parent.right; margins: 16; bottom: parent.bottom; bottomMargin: 12 }

            Button {
                text: qsTr("取消")
                Layout.fillWidth: true
                height: 38
                background: Rectangle { radius: 6; color: "#2A2A3E" }
                contentItem: Text { text: parent.text; color: "#EAEAEA"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; font.pixelSize: 13 }
                onClicked: connectDialog.close()
            }

            Button {
                text: qsTr("连接")
                Layout.fillWidth: true
                height: 38
                background: Rectangle { radius: 6; color: "#E94560" }
                contentItem: Text { text: parent.text; color: "white"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; font.pixelSize: 13 }
                onClicked: {
                    wifiManager.connectNetwork(ssidField.text, passwordField.text)
                    connectDialog.close()
                }
            }
        }
    }

    // ---- Layout ----
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        // ---- Control row ----
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Button {
                text: wifiManager.isEnabled ? qsTr("关闭 WiFi") : qsTr("开启 WiFi")
                height: 44
                width: 120
                background: Rectangle {
                    radius: 6
                    color: wifiManager.isEnabled ? "#E94560" : "#0F3460"
                }
                contentItem: Text {
                    text: parent.text; color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 13
                }
                onClicked: {
                    if (wifiManager.isEnabled) wifiManager.stopWiFi()
                    else                       wifiManager.startWiFi()
                }
            }

            Button {
                text: wifiManager.isScanning ? qsTr("扫描中...") : qsTr("扫描网络")
                height: 44
                width: 110
                enabled: wifiManager.isEnabled && !wifiManager.isScanning
                background: Rectangle {
                    radius: 6
                    color: parent.enabled ? "#0F3460" : "#2A2A3E"
                }
                contentItem: Text {
                    text: parent.text
                    color: parent.enabled ? "white" : "#666"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 13
                }
                onClicked: wifiManager.scanNetworks()
            }

            Button {
                text: qsTr("手动连接")
                height: 44
                width: 100
                enabled: wifiManager.isEnabled
                background: Rectangle {
                    radius: 6
                    color: parent.enabled ? "#0F3460" : "#2A2A3E"
                }
                contentItem: Text {
                    text: parent.text
                    color: parent.enabled ? "white" : "#666"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 13
                }
                onClicked: {
                    root.pendingSSID = ""
                    passwordField.text = ""
                    connectDialog.open()
                }
            }

            Item { Layout.fillWidth: true }

            // Current SSID indicator
            RowLayout {
                spacing: 6
                Rectangle {
                    width: 10; height: 10; radius: 5
                    color: wifiManager.isEnabled
                           ? (wifiManager.connectedSSID !== "" ? "#4CAF50" : "#FFC107")
                           : "#F44336"
                }
                Text {
                    text: wifiManager.isEnabled
                          ? (wifiManager.connectedSSID !== ""
                             ? qsTr("已连接: ") + wifiManager.connectedSSID
                             : qsTr("未连接"))
                          : qsTr("WiFi 已关闭")
                    color: "#EAEAEA"
                    font.pixelSize: 13
                }
            }
        }

        // ---- WiFi list ----
        WiFiListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            onConnectRequested: function(ssid) {
                root.pendingSSID = ssid
                passwordField.text = ""
                ssidField.text = ssid
                connectDialog.open()
            }
        }
    }
}
