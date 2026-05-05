import QtQuick

Rectangle {
    id: root

    property string title: "IAS"
    property string unitLabel: "KT"
    property real value: 0
    property real step: 10
    property int decimals: 0
    property color accentColor: "#42c7ff"

    readonly property real roundedValue: Math.round(value / step) * step
    readonly property real offsetRatio: (value - roundedValue) / step

    radius: 12
    color: "#0c1622"
    border.color: "#223648"
    border.width: 1
    clip: true

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 36
        color: "#12202e"

        Text {
            anchors.centerIn: parent
            text: root.title
            color: "#9eb6c8"
            font.pixelSize: 13
            font.bold: true
        }
    }

    Repeater {
        model: 13

        delegate: Item {
            required property int index

            readonly property int offset: index - 6
            readonly property real spacing: (root.height - 60) / 8
            readonly property real tickValue: root.roundedValue + offset * root.step

            width: parent.width
            height: 20
            y: 44 + (parent.height - 60) / 2 + offset * spacing - root.offsetRatio * spacing - height / 2

            visible: y + height > 40 && y < parent.height

            Rectangle {
                width: offset === 0 ? parent.width * 0.28 : parent.width * 0.18
                height: 2
                anchors.right: parent.right
                anchors.rightMargin: 14
                anchors.verticalCenter: parent.verticalCenter
                color: offset === 0 ? root.accentColor : "#b9c8d7"
            }

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                text: tickValue.toFixed(root.decimals)
                color: offset === 0 ? "#f8fbff" : "#bfd1e0"
                font.pixelSize: offset === 0 ? 18 : 13
                font.bold: offset === 0
            }
        }
    }

    Rectangle {
        width: parent.width
        height: 48
        anchors.verticalCenter: parent.verticalCenter
        color: "#08131d"
        border.color: root.accentColor
        border.width: 1
        opacity: 0.95

        Text {
            anchors.centerIn: parent
            text: Number(root.value).toFixed(root.decimals)
            color: "#f8fbff"
            font.pixelSize: 28
            font.bold: true
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 28
        color: "#12202e"

        Text {
            anchors.centerIn: parent
            text: root.unitLabel
            color: "#9eb6c8"
            font.pixelSize: 12
            font.letterSpacing: 1.0
        }
    }
}
