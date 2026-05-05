import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root

    readonly property bool compact: appController.displayClass !== "primary"

    color: "#000000"
    border.color: "#2c3948"
    border.width: 1
    radius: 8
    implicitHeight: compact ? 36 : 42

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: compact ? 10 : 14
        anchors.rightMargin: compact ? 10 : 14
        spacing: compact ? 14 : 18

        Repeater {
            model: [
                { label: "WPT", value: appController.activeWaypointLabel },
                { label: "BRG", value: ("000" + Number(appController.waypointBearingDeg).toFixed(0)).slice(-3) + "M" },
                { label: "DST", value: Number(appController.waypointDistanceNm).toFixed(1) + "NM" },
                { label: "ETE", value: appController.waypointEteLabel },
                { label: "GS", value: Number(appController.groundSpeedKt).toFixed(0) + "KT" },
                { label: "TRK", value: ("000" + Number(appController.groundTrackDeg).toFixed(0)).slice(-3) + "M" }
            ]

            delegate: Column {
                required property var modelData

                Layout.alignment: Qt.AlignVCenter
                spacing: 0

                Text {
                    text: modelData.label
                    color: "#ffffff"
                    opacity: 0.95
                    font.pixelSize: root.compact ? 11 : 12
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                }

                Text {
                    text: modelData.value
                    color: "#ff4ef1"
                    font.pixelSize: root.compact ? 16 : 17
                    font.bold: true
                    font.family: "Menlo"
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        Item { Layout.fillWidth: true }
    }
}
