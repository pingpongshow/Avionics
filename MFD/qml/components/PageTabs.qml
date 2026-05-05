import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    color: "#0c1722"
    border.color: "#223648"
    border.width: 1
    radius: 10
    implicitHeight: 72

    RowLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8

        Repeater {
            model: appController.availablePages

            delegate: Button {
                id: pageButton

                required property string modelData

                Layout.fillWidth: true
                Layout.fillHeight: true
                text: modelData

                background: Rectangle {
                    radius: 8
                    color: appController.currentPage === modelData ? "#1f4fd1" : "#142231"
                    border.color: appController.currentPage === modelData ? "#7cb2ff" : "#27425a"
                    border.width: 1
                }

                contentItem: Text {
                    text: pageButton.text
                    color: "#f8fbff"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: appController.displayClass === "standby" ? 12 : 14
                    font.bold: appController.currentPage === modelData
                    elide: Text.ElideRight
                }

                onClicked: appController.currentPage = modelData
            }
        }
    }
}
