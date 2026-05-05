import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root

    property string titleText: "ENTER VALUE"
    property string unitText: ""
    property bool allowDecimal: false
    property string valueText: ""

    signal acceptedValue(string valueText)

    function openForValue(title, initialText, decimalAllowed, units) {
        titleText = title
        valueText = initialText
        allowDecimal = decimalAllowed
        unitText = units || ""
        open()
    }

    modal: true
    focus: true
    anchors.centerIn: Overlay.overlay
    width: 320
    height: 420
    padding: 0
    closePolicy: Popup.NoAutoClose

    background: Rectangle {
        radius: 12
        color: "#08111b"
        border.color: "#355069"
        border.width: 1
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 10

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: root.titleText
            color: "#f8fbff"
            font.pixelSize: 18
            font.bold: true
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 58
            radius: 8
            color: "#000000"
            border.color: "#5f6e83"
            border.width: 1

            Row {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                spacing: 6

                Text {
                    text: root.valueText.length > 0 ? root.valueText : "0"
                    color: "#f8fbff"
                    font.pixelSize: 28
                    font.family: "Menlo"
                    font.bold: true
                }

                Text {
                    visible: root.unitText.length > 0
                    text: root.unitText
                    color: "#8fb1cb"
                    font.pixelSize: 16
                    font.bold: true
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            columns: 3
            columnSpacing: 8
            rowSpacing: 8

            Repeater {
                model: ["1", "2", "3", "4", "5", "6", "7", "8", "9", ".", "0", "BKSP"]

                delegate: Button {
                    required property string modelData

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    enabled: modelData !== "." || root.allowDecimal

                    background: Rectangle {
                        radius: 8
                        color: parent.down ? "#1f4fd1" : "#132234"
                        border.color: parent.enabled ? "#355069" : "#223648"
                        border.width: 1
                        opacity: parent.enabled ? 1.0 : 0.4
                    }

                    contentItem: Text {
                        text: modelData
                        color: "#f8fbff"
                        font.pixelSize: 20
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: {
                        if (modelData === "BKSP") {
                            root.valueText = root.valueText.slice(0, -1)
                            return
                        }
                        if (modelData === ".") {
                            if (!root.allowDecimal || root.valueText.indexOf(".") >= 0) {
                                return
                            }
                        }
                        root.valueText += modelData
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                Layout.fillWidth: true
                text: "CLR"

                background: Rectangle {
                    radius: 8
                    color: "#132234"
                    border.color: "#355069"
                    border.width: 1
                }

                contentItem: Text {
                    text: parent.text
                    color: "#f8fbff"
                    font.pixelSize: 16
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: root.valueText = ""
            }

            Button {
                Layout.fillWidth: true
                text: "CANCEL"

                background: Rectangle {
                    radius: 8
                    color: "#21131b"
                    border.color: "#6a4254"
                    border.width: 1
                }

                contentItem: Text {
                    text: parent.text
                    color: "#f8fbff"
                    font.pixelSize: 16
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: root.close()
            }

            Button {
                Layout.fillWidth: true
                text: "ENTER"

                background: Rectangle {
                    radius: 8
                    color: "#163ca7"
                    border.color: "#56d6ff"
                    border.width: 1
                }

                contentItem: Text {
                    text: parent.text
                    color: "#f8fbff"
                    font.pixelSize: 16
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    root.acceptedValue(root.valueText)
                    root.close()
                }
            }
        }
    }
}
