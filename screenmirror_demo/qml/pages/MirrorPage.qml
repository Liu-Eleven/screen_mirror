import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root

    // Devices list (populated by mirrorManager)
    property var devices: []

    Connections {
        target: mirrorManager
        function onDevicesUpdated(list) { root.devices = list }
    }

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

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        // ---- Mode selector + action bar ----
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Text {
                text: qsTr("协议:")
                color: "#A0A0B0"
                font.pixelSize: 13
                verticalAlignment: Text.AlignVCenter
            }

            ComboBox {
                id: modeCombo
                model: [qsTr("Miracast"), qsTr("AirPlay"), qsTr("DLNA"),
                        qsTr("有线 USB"), qsTr("无线 WiFi")]
                width: 140
                contentItem: Text {
                    leftPadding: 8
                    text: modeCombo.displayText
                    color: "#EAEAEA"
                    font.pixelSize: 13
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: "#0F3460"
                    radius: 6
                    border.color: modeCombo.pressed ? "#E94560" : "#1A2B4A"
                }
                popup: Popup {
                    y: modeCombo.height
                    width: modeCombo.width
                    padding: 1
                    contentItem: ListView {
                        clip: true
                        model: modeCombo.delegateModel
                        implicitHeight: contentHeight
                        ScrollIndicator.vertical: ScrollIndicator {}
                    }
                    background: Rectangle { color: "#16213E"; radius: 6; border.color: "#0F3460" }
                }
                delegate: ItemDelegate {
                    width: modeCombo.width
                    contentItem: Text {
                        text: modelData
                        color: modeCombo.currentIndex === index ? "#E94560" : "#EAEAEA"
                        font.pixelSize: 13
                        leftPadding: 10
                    }
                    background: Rectangle { color: hovered ? "#0F3460" : "transparent" }
                }
            }

            Button {
                text: mirrorManager.state === 1 ? qsTr("停止搜索") : qsTr("搜索设备")
                height: 40
                width: 100
                background: Rectangle {
                    radius: 6
                    color: mirrorManager.state === 1 ? "#E94560" : "#0F3460"
                }
                contentItem: Text {
                    text: parent.text; color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 13
                }
                onClicked: {
                    if (mirrorManager.state === 1)
                        mirrorManager.stopDiscovery()
                    else
                        mirrorManager.startDiscovery(modeCombo.currentIndex)
                }
            }

            Item { Layout.fillWidth: true }

            // State badge
            Rectangle {
                height: 30; width: stateLabel.implicitWidth + 20; radius: 15
                color: {
                    switch(mirrorManager.state) {
                    case 3: return "#1B5E20"
                    case 4: return "#0D47A1"
                    case 6: return "#B71C1C"
                    default: return "#1A2B4A"
                    }
                }
                Text {
                    id: stateLabel
                    anchors.centerIn: parent
                    text: root.stateName(mirrorManager.state)
                    color: "#EAEAEA"
                    font.pixelSize: 12
                }
            }
        }

        // ---- Device list + control panel ----
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10

            // Device list
            DeviceListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                devices: root.devices
                onConnectRequested: function(idx) {
                    mirrorManager.connectDevice(idx, modeCombo.currentIndex)
                }
                onDisconnectRequested: mirrorManager.disconnectDevice()
            }

            // Control panel (right side)
            ControlPanel {
                Layout.preferredWidth: 220
                Layout.fillHeight: true
            }
        }
    }
}
