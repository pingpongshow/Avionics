import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 94
            radius: 12
            color: "#0c1622"
            border.color: "#223648"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 10

                Text {
                    Layout.fillWidth: true
                    text: "Bench integration status. Use this page to simulate installed or missing modules before live CAN and GNSS links are connected."
                    color: "#c5d3df"
                    wrapMode: Text.WordWrap
                    font.pixelSize: 14
                }
            }
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10
            clip: true
            model: appController.moduleModel

            delegate: Rectangle {
                required property string moduleKey
                required property string name
                required property bool present
                required property bool healthy
                required property string mode
                required property string details

                width: ListView.view.width
                height: 96
                radius: 12
                color: "#0c1622"
                border.color: healthy && present ? "#2d8d5a" : present ? "#f59e0b" : "#223648"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 12

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        Text {
                            text: name
                            color: "#f8fbff"
                            font.pixelSize: 20
                            font.bold: true
                        }

                        Text {
                            text: details
                            color: "#b9cad8"
                            font.pixelSize: 13
                        }

                        Text {
                            text: present ? (healthy ? "ONLINE / HEALTHY" : "ONLINE / DEGRADED") : "OFFLINE"
                            color: healthy && present ? "#7ef0a6" : present ? "#f7cb67" : "#8da4ba"
                            font.pixelSize: 12
                            font.bold: true
                        }
                    }

                    Rectangle {
                        Layout.preferredWidth: 110
                        Layout.preferredHeight: 48
                        radius: 10
                        color: "#132234"
                        border.color: "#27425a"

                        Text {
                            anchors.centerIn: parent
                            text: mode.toUpperCase()
                            color: "#f8fbff"
                            font.pixelSize: 13
                            font.bold: true
                        }
                    }

                    Button {
                        id: toggleButton

                        Layout.preferredWidth: 120
                        Layout.preferredHeight: 48
                        text: present ? "Disable" : "Enable"

                        background: Rectangle {
                            radius: 10
                            color: present ? "#4b1d1d" : "#123624"
                            border.color: present ? "#ef4444" : "#22c55e"
                            border.width: 1
                        }

                        contentItem: Text {
                            text: toggleButton.text
                            color: "#f8fbff"
                            font.pixelSize: 14
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        onClicked: appController.setModuleEnabled(moduleKey, !present)
                    }
                }
            }
        }
    }
}
