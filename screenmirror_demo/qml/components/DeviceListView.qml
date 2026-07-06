import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// DeviceListView – shows discovered screenmirror devices
Rectangle {
    id: root
    color: "#0D0D1A"
    radius: 8

    signal connectRequested(int index)
    signal disconnectRequested()

    property var devices: []

    // Protocols label
    function modeName(m) {
        switch(m) {
        case 0: return "Miracast"
        case 1: return "AirPlay"
        case 2: return "DLNA"
        case 3: return "USB"
        case 4: return "WiFi"
        default: return "Unknown"
        }
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
            Text { text: qsTr("设备名称");  color: "#A0A0B0"; font.pixelSize: 12; Layout.fillWidth: true }
            Text { text: qsTr("协议");      color: "#A0A0B0"; font.pixelSize: 12; Layout.preferredWidth: 80 }
            Text { text: qsTr("信号");      color: "#A0A0B0"; font.pixelSize: 12; Layout.preferredWidth: 60 }
            Text { text: qsTr("操作");      color: "#A0A0B0"; font.pixelSize: 12; Layout.preferredWidth: 80 }
        }
    }

    // Empty placeholder
    Text {
        anchors.centerIn: parent
        visible: root.devices.length === 0
        text: mirrorManager.state === 1
              ? qsTr("正在搜索设备...")
              : qsTr("暂无设备，请点击「搜索设备」")
        color: "#555577"
        font.pixelSize: 14
    }

    ListView {
        anchors { top: header.bottom; left: parent.left; right: parent.right; bottom: parent.bottom; topMargin: 4 }
        model: root.devices
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

                // Device name + IP
                Column {
                    Layout.fillWidth: true
                    spacing: 2
                    Text {
                        text: modelData.name ?? ""
                        color: "#EAEAEA"
                        font.pixelSize: 13
                        elide: Text.ElideRight
                        width: parent.width
                    }
                    Text {
                        text: modelData.ip ?? ""
                        color: "#777799"
                        font.pixelSize: 10
                    }
                }

                // Protocol
                Rectangle {
                    width: 70; height: 22; radius: 4
                    color: "#0F3460"
                    Text {
                        anchors.centerIn: parent
                        text: root.modeName(modelData.mode ?? 0)
                        color: "#88BBFF"
                        font.pixelSize: 11
                    }
                }

                // Signal
                Text {
                    text: (modelData.signal ?? 0) + "%"
                    color: "#A0A0B0"
                    font.pixelSize: 12
                    Layout.preferredWidth: 50
                    horizontalAlignment: Text.AlignRight
                }

                // Action button
                Button {
                    text: (mirrorManager.state === 3 || mirrorManager.state === 4)
                          ? qsTr("断开") : qsTr("连接")
                    width: 64
                    height: 30
                    background: Rectangle {
                        radius: 6
                        color: (mirrorManager.state === 3 || mirrorManager.state === 4)
                               ? "#7B1818" : "#0F3460"
                    }
                    contentItem: Text {
                        text: parent.text; color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 12
                    }
                    onClicked: {
                        if (mirrorManager.state === 3 || mirrorManager.state === 4)
                            root.disconnectRequested()
                        else
                            root.connectRequested(index)
                    }
                }
            }

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                hoverEnabled: true
                propagateComposedEvents: true
            }
        }
    }
}
