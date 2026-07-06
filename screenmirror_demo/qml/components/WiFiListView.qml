import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// WiFiListView – shows scanned networks from wifiManager
Rectangle {
    id: root
    color: "#0D0D1A"
    radius: 8

    signal connectRequested(string ssid)

    property var networkModel: []

    Connections {
        target: wifiManager
        function onNetworksUpdated(networks) { root.networkModel = networks }
    }

    // Header
    Rectangle {
        id: header
        anchors { top: parent.top; left: parent.left; right: parent.right }
        height: 36
        color: "#16213E"
        radius: 8

        RowLayout {
            anchors { fill: parent; leftMargin: 12; rightMargin: 12 }
            Text { text: qsTr("SSID");        color: "#A0A0B0"; font.pixelSize: 12; Layout.preferredWidth: 200 }
            Text { text: qsTr("信号强度");    color: "#A0A0B0"; font.pixelSize: 12; Layout.preferredWidth: 90 }
            Text { text: qsTr("加密方式");    color: "#A0A0B0"; font.pixelSize: 12; Layout.fillWidth: true }
            Text { text: qsTr("操作");        color: "#A0A0B0"; font.pixelSize: 12; Layout.preferredWidth: 80 }
        }
    }

    // Empty placeholder
    Text {
        anchors.centerIn: parent
        visible: root.networkModel.length === 0
        text: wifiManager.isScanning
              ? qsTr("正在扫描...")
              : qsTr("暂无网络，请先开启 WiFi 并扫描")
        color: "#555577"
        font.pixelSize: 14
    }

    // List
    ListView {
        anchors { top: header.bottom; left: parent.left; right: parent.right; bottom: parent.bottom; topMargin: 4 }
        model: root.networkModel
        clip: true
        spacing: 2

        delegate: Rectangle {
            width: ListView.view.width
            height: 44
            color: mouseArea.containsMouse ? "#1A2B4A" : "transparent"
            radius: 6

            required property var modelData
            required property int index

            RowLayout {
                anchors { fill: parent; leftMargin: 12; rightMargin: 12 }
                spacing: 8

                // SSID
                Text {
                    text: modelData.ssid ?? ""
                    color: "#EAEAEA"
                    font.pixelSize: 13
                    elide: Text.ElideRight
                    Layout.preferredWidth: 200
                }

                // Signal bar
                RowLayout {
                    spacing: 3
                    Layout.preferredWidth: 90

                    Repeater {
                        model: 4
                        Rectangle {
                            width: 6
                            height: 8 + index * 4
                            radius: 2
                            color: {
                                var sig = modelData.signal ?? 0
                                var threshold = [25, 50, 75, 95]
                                return sig >= threshold[index] ? "#4CAF50" : "#333355"
                            }
                            anchors.bottom: parent.bottom
                        }
                    }

                    Text {
                        text: (modelData.signal ?? 0) + "%"
                        color: "#A0A0B0"
                        font.pixelSize: 11
                        leftPadding: 4
                    }
                }

                // Encryption
                Text {
                    text: modelData.encryption ?? "Open"
                    color: (modelData.encryption === "Open") ? "#FFC107" : "#4CAF50"
                    font.pixelSize: 12
                    Layout.fillWidth: true
                }

                // Connect button
                Button {
                    text: (wifiManager.connectedSSID === modelData.ssid)
                          ? qsTr("已连接") : qsTr("连接")
                    width: 72
                    height: 30
                    enabled: wifiManager.isEnabled
                             && wifiManager.connectedSSID !== modelData.ssid
                    background: Rectangle {
                        radius: 6
                        color: parent.enabled ? "#0F3460" : "#2A2A3E"
                        border.color: wifiManager.connectedSSID === modelData.ssid
                                      ? "#4CAF50" : "transparent"
                    }
                    contentItem: Text {
                        text: parent.text; color: parent.enabled ? "#EAEAEA" : "#666"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 12
                    }
                    onClicked: root.connectRequested(modelData.ssid)
                }
            }

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                hoverEnabled: true
                propagateComposedEvents: true
                onDoubleClicked: root.connectRequested(modelData.ssid)
            }
        }
    }
}
