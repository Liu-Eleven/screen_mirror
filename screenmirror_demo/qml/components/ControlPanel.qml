import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// ControlPanel – audio / video / HDCP / pause controls
Rectangle {
    id: root
    color: "#16213E"
    radius: 8

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 14

        Text {
            text: qsTr("控制面板")
            color: "#EAEAEA"
            font.pixelSize: 14
            font.bold: true
        }

        // ---- Audio ----
        RowLayout {
            Layout.fillWidth: true
            Text { text: qsTr("音频"); color: "#A0A0B0"; font.pixelSize: 13; Layout.fillWidth: true }
            Switch {
                id: audioSwitch
                checked: mirrorManager.audioEnabled
                onCheckedChanged: mirrorManager.audioEnabled = checked
                indicator: Rectangle {
                    implicitWidth: 44; implicitHeight: 22; radius: 11
                    color: audioSwitch.checked ? "#4CAF50" : "#555577"
                    Rectangle {
                        x: audioSwitch.checked ? parent.width - width - 2 : 2
                        anchors.verticalCenter: parent.verticalCenter
                        width: 18; height: 18; radius: 9
                        color: "white"
                        Behavior on x { NumberAnimation { duration: 150 } }
                    }
                }
            }
        }

        // ---- HDCP ----
        RowLayout {
            Layout.fillWidth: true
            Text { text: qsTr("HDCP"); color: "#A0A0B0"; font.pixelSize: 13; Layout.fillWidth: true }
            Switch {
                id: hdcpSwitch
                checked: mirrorManager.hdcpEnabled
                onCheckedChanged: mirrorManager.hdcpEnabled = checked
                indicator: Rectangle {
                    implicitWidth: 44; implicitHeight: 22; radius: 11
                    color: hdcpSwitch.checked ? "#4CAF50" : "#555577"
                    Rectangle {
                        x: hdcpSwitch.checked ? parent.width - width - 2 : 2
                        anchors.verticalCenter: parent.verticalCenter
                        width: 18; height: 18; radius: 9
                        color: "white"
                        Behavior on x { NumberAnimation { duration: 150 } }
                    }
                }
            }
        }

        // ---- Video quality ----
        Column {
            Layout.fillWidth: true
            spacing: 6

            Text { text: qsTr("视频质量"); color: "#A0A0B0"; font.pixelSize: 13 }

            ComboBox {
                id: qualityCombo
                width: parent.width
                model: [qsTr("720p (默认)"), qsTr("1080p"), qsTr("480p")]
                currentIndex: mirrorManager.videoQuality
                onCurrentIndexChanged: mirrorManager.videoQuality = currentIndex

                contentItem: Text {
                    leftPadding: 8
                    text: qualityCombo.displayText
                    color: "#EAEAEA"
                    font.pixelSize: 12
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: "#0F3460"; radius: 6
                    border.color: qualityCombo.pressed ? "#E94560" : "#1A2B4A"
                }
                popup: Popup {
                    y: qualityCombo.height
                    width: qualityCombo.width
                    padding: 1
                    contentItem: ListView {
                        clip: true; model: qualityCombo.delegateModel
                        implicitHeight: contentHeight
                        ScrollIndicator.vertical: ScrollIndicator {}
                    }
                    background: Rectangle { color: "#16213E"; radius: 6; border.color: "#0F3460" }
                }
                delegate: ItemDelegate {
                    width: qualityCombo.width
                    contentItem: Text {
                        text: modelData
                        color: qualityCombo.currentIndex === index ? "#E94560" : "#EAEAEA"
                        font.pixelSize: 12
                        leftPadding: 10
                    }
                    background: Rectangle { color: hovered ? "#0F3460" : "transparent" }
                }
            }
        }

        // ---- Spacer ----
        Item { Layout.fillHeight: true }

        // ---- Pause / Resume ----
        Button {
            Layout.fillWidth: true
            height: 40
            text: mirrorManager.state === 5 ? qsTr("继续投屏") : qsTr("暂停投屏")
            enabled: mirrorManager.state === 4 || mirrorManager.state === 5
            background: Rectangle {
                radius: 6
                color: parent.enabled
                       ? (mirrorManager.state === 5 ? "#1B5E20" : "#7B3F00")
                       : "#2A2A3E"
            }
            contentItem: Text {
                text: parent.text
                color: parent.enabled ? "white" : "#666"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 13
            }
            onClicked: {
                if (mirrorManager.state === 5)
                    mirrorManager.resumeMirroring()
                else
                    mirrorManager.pauseMirroring()
            }
        }

        // ---- Disconnect ----
        Button {
            Layout.fillWidth: true
            height: 40
            text: qsTr("断开连接")
            enabled: mirrorManager.state !== 0 && mirrorManager.state !== 1
            background: Rectangle {
                radius: 6
                color: parent.enabled ? "#E94560" : "#2A2A3E"
            }
            contentItem: Text {
                text: parent.text
                color: parent.enabled ? "white" : "#666"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 13
            }
            onClicked: mirrorManager.disconnectDevice()
        }
    }
}
