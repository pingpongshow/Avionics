import QtQuick

Rectangle {
    id: root

    property string title: "CARD"
    property string value: "--"
    property string subtitle: ""

    radius: 10
    color: "#0f1b29"
    border.color: "#27425a"
    border.width: 1

    implicitWidth: 140
    implicitHeight: 82

    Column {
        anchors.centerIn: parent
        spacing: 4

        Text {
            text: root.title
            color: "#8fb1cb"
            font.pixelSize: 11
            horizontalAlignment: Text.AlignHCenter
        }

        Text {
            text: root.value
            color: "#f8fbff"
            font.pixelSize: 22
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
        }

        Text {
            visible: root.subtitle.length > 0
            text: root.subtitle
            color: "#b8c9d8"
            font.pixelSize: 11
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
