import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root

    // ---- State label helpers ----
    function stateName(s) {
        switch(s) {
        case 0: return qsTr("空闲")
        case 1: return qsTr("搜索中")
        case 2: return qsTr("连接中")
        case 3: return qsTr("已连接")
        case 4: return qsTr("传输中")
        case 5: return qsTr("已暂停")
        case 6: return qsTr("错误")
        default: return qsTr("未知")
        }
    }
    function stateColor(s) {
        switch(s) {
        case 3: return "#4CAF50"
        case 4: return "#2196F3"
        case 6: return "#F44336"
        default: return "#FFC107"
        }
    }

    // ---- Log model ----
    ListModel { id: logModel }

    Connections {
        target: mirrorManager
        function onStatusMessage(msg) { logModel.insert(0, { "text": "🔵 " + msg }) }
        function onError(msg)         { logModel.insert(0, { "text": "🔴 " + msg }) }
    }
    Connections {
        target: wifiManager
        function onStatusMessage(msg) { logModel.insert(0, { "text": "📶 " + msg }) }
        function onError(msg)         { logModel.insert(0, { "text": "⚠️  " + msg }) }
    }

    // ---- Layout ----
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        // ---- Top status bar ----
        Rectangle {
            Layout.fillWidth: true
            height: 64
            radius: 8
            color: "#16213E"

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 16

                // Device name
                Column {
                    spacing: 2
                    Text {
                        text: qsTr("设备名称")
                        color: "#A0A0B0"
                        font.pixelSize: 11
                    }
                    Text {
                        text: mirrorManager.deviceName
                        color: "#EAEAEA"
                        font.pixelSize: 14
                        font.bold: true
                    }
                }

                Rectangle { width: 1; height: 40; color: "#0F3460" }

                // WiFi status
                Column {
                    spacing: 2
                    Text {
                        text: qsTr("WiFi 状态")
                        color: "#A0A0B0"
                        font.pixelSize: 11
                    }
                    RowLayout {
                        spacing: 6
                        Rectangle {
                            width: 10; height: 10; radius: 5
                            color: wifiManager.isEnabled ? "#4CAF50" : "#F44336"
                        }
                        Text {
                            text: wifiManager.isEnabled
                                  ? (wifiManager.connectedSSID !== ""
                                     ? wifiManager.connectedSSID
                                     : qsTr("已开启"))
                                  : qsTr("已关闭")
                            color: "#EAEAEA"
                            font.pixelSize: 13
                        }
                    }
                }

                Rectangle { width: 1; height: 40; color: "#0F3460" }

                // Mirror state
                Column {
                    spacing: 2
                    Text {
                        text: qsTr("投屏状态")
                        color: "#A0A0B0"
                        font.pixelSize: 11
                    }
                    StatusIndicator {
                        state_: mirrorManager.state
                        label:  root.stateName(mirrorManager.state)
                        color_: root.stateColor(mirrorManager.state)
                    }
                }

                Item { Layout.fillWidth: true }
            }
        }

        // ---- Quick action buttons ----
        Text {
            text: qsTr("快速操作")
            color: "#A0A0B0"
            font.pixelSize: 12
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Button {
                text: wifiManager.isEnabled ? qsTr("关闭 WiFi") : qsTr("开启 WiFi")
                Layout.fillWidth: true
                height: 44
                background: Rectangle {
                    radius: 6
                    color: wifiManager.isEnabled ? "#E94560" : "#0F3460"
                }
                contentItem: Text {
                    text: parent.text
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 13
                }
                onClicked: {
                    if (wifiManager.isEnabled)
                        wifiManager.stopWiFi()
                    else
                        wifiManager.startWiFi()
                }
            }

            Button {
                text: qsTr("扫描 WiFi")
                Layout.fillWidth: true
                height: 44
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
                onClicked: wifiManager.scanNetworks()
            }

            Button {
                text: mirrorManager.state === 1 ? qsTr("停止搜索") : qsTr("搜索设备")
                Layout.fillWidth: true
                height: 44
                background: Rectangle {
                    radius: 6
                    color: "#0F3460"
                }
                contentItem: Text {
                    text: parent.text
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 13
                }
                onClicked: {
                    if (mirrorManager.state === 1)
                        mirrorManager.stopDiscovery()
                    else
                        mirrorManager.startDiscovery(0)
                }
            }

            Button {
                text: (mirrorManager.state === 3 || mirrorManager.state === 4)
                      ? qsTr("断开投屏") : qsTr("连接设备")
                Layout.fillWidth: true
                height: 44
                background: Rectangle {
                    radius: 6
                    color: (mirrorManager.state === 3 || mirrorManager.state === 4)
                           ? "#E94560" : "#0F3460"
                }
                contentItem: Text {
                    text: parent.text
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 13
                }
                onClicked: {
                    if (mirrorManager.state === 3 || mirrorManager.state === 4)
                        mirrorManager.disconnectDevice()
                    else
                        mirrorManager.connectDevice(0, 0)
                }
            }
        }

        // ---- Log area ----
        Text {
            text: qsTr("实时日志")
            color: "#A0A0B0"
            font.pixelSize: 12
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#0D0D1A"
            radius: 8

            ListView {
                id: logView
                anchors.fill: parent
                anchors.margins: 8
                model: logModel
                clip: true
                spacing: 4

                delegate: Text {
                    width: logView.width
                    text: model.text
                    color: "#CCCCDD"
                    font.pixelSize: 12
                    font.family: "Monospace"
                    wrapMode: Text.WordWrap
                }
            }
        }
    }
}
