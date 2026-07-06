import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// StatusIndicator – small coloured dot + label
RowLayout {
    spacing: 6

    property int    state_: 0
    property string label:  ""
    property color  color_: "#FFC107"

    Rectangle {
        width: 10; height: 10; radius: 5
        color: parent.color_

        SequentialAnimation on opacity {
            running: parent.parent.state_ === 1 || parent.parent.state_ === 2
            loops: Animation.Infinite
            NumberAnimation { to: 0.2; duration: 600 }
            NumberAnimation { to: 1.0; duration: 600 }
        }
    }

    Text {
        text: parent.label
        color: "#EAEAEA"
        font.pixelSize: 13
    }
}
