import QtQuick

Rectangle {
    id: root

    property real pitchDeg: 0
    property real rollDeg: 0
    property real slipSkidNormalized: 0

    radius: 14
    color: "#0b1320"
    border.color: "#23384b"
    border.width: 1
    clip: true

    Item {
        id: world
        width: root.width * 3
        height: root.height * 3
        anchors.centerIn: parent
        rotation: -root.rollDeg
        y: root.pitchDeg * (root.height / 28)

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: parent.height / 2
            color: "#3f85ff"
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: parent.height / 2
            color: "#73512b"
        }

        Rectangle {
            width: parent.width
            height: 4
            anchors.centerIn: parent
            color: "#f0c34d"
        }

        Repeater {
            model: [-20, -10, 10, 20]

            delegate: Rectangle {
                required property int index
                required property var modelData

                width: Math.abs(modelData) === 20 ? world.width * 0.18 : world.width * 0.28
                height: 2
                x: (world.width - width) / 2
                y: world.height / 2 - modelData * (root.height / 40)
                color: "white"
                opacity: 0.85
            }
        }
    }

    Repeater {
        model: 5

        delegate: Rectangle {
            required property int index

            width: index === 2 ? 40 : 12
            height: index === 2 ? 4 : 10
            radius: 2
            color: "#f8fbff"
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 18
            rotation: -24 + index * 12
            transformOrigin: Item.Bottom
            y: 16
        }
    }

    Rectangle {
        width: root.width * 0.16
        height: 4
        radius: 2
        color: "#f8fbff"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenterOffset: -root.width * 0.11
    }

    Rectangle {
        width: root.width * 0.16
        height: 4
        radius: 2
        color: "#f8fbff"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenterOffset: root.width * 0.11
    }

    Rectangle {
        width: root.width * 0.06
        height: 4
        radius: 2
        color: "#f0c34d"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
    }

    Rectangle {
        width: 120
        height: 16
        radius: 8
        color: "#0d1623"
        border.color: "#27425a"
        border.width: 1
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 12

        Rectangle {
            width: 10
            height: 10
            radius: 5
            color: "#f0c34d"
            anchors.centerIn: parent
            anchors.horizontalCenterOffset: root.slipSkidNormalized * 34
        }
    }
}
