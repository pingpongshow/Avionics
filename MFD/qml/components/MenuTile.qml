import QtQuick

Rectangle {
    id: root

    property string title: "Tile"
    property string iconText: "?"
    property url iconSource: ""
    property color iconColor: "#56d6ff"
    property bool enabledTile: true
    property int tileWidth: 118
    property int tileHeight: 118
    property int iconBoxSize: 58
    property int iconSize: 38
    property int titlePixelSize: 11
    property bool squareIconBox: false

    signal clicked()

    implicitWidth: tileWidth
    implicitHeight: tileHeight
    radius: 10
    color: enabledTile ? "#05080d" : "#3a3d40"
    border.color: enabledTile ? "#6c7d90" : "#7c8791"
    border.width: 1
    opacity: enabledTile ? 1.0 : 0.55

    Rectangle {
        anchors.fill: parent
        anchors.margins: 2
        radius: 8
        gradient: Gradient {
            GradientStop { position: 0.0; color: enabledTile ? "#2a2e34" : "#55585c" }
            GradientStop { position: 0.2; color: enabledTile ? "#14181e" : "#4a4d50" }
            GradientStop { position: 1.0; color: enabledTile ? "#030507" : "#414448" }
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: 8

        Rectangle {
            width: root.iconBoxSize
            height: root.iconBoxSize
            radius: root.squareIconBox ? 6 : root.iconBoxSize / 2
            color: "#111821"
            border.color: Qt.lighter(root.iconColor, 1.2)
            border.width: 1

            Image {
                anchors.centerIn: parent
                width: root.iconSize
                height: root.iconSize
                source: root.iconSource
                fillMode: Image.PreserveAspectFit
                visible: root.iconSource !== ""
                smooth: true
            }

            Text {
                anchors.centerIn: parent
                text: root.iconText
                color: root.iconColor
                font.pixelSize: Math.max(14, root.iconSize - 16)
                font.bold: true
                font.family: "Menlo"
                visible: root.iconSource === ""
            }
        }

        Text {
            width: parent.width + 30
            text: root.title
            color: "#f8fbff"
            font.pixelSize: root.titlePixelSize
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: root.enabledTile
        onClicked: root.clicked()
    }
}
