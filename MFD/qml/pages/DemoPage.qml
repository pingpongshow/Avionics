import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 156
            radius: 12
            color: "#0c1622"
            border.color: "#223648"
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 10

                Text {
                    text: "Demo Scenarios"
                    color: "#f8fbff"
                    font.pixelSize: 20
                    font.bold: true
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: 8

                    Repeater {
                        model: appController.scenarios

                        delegate: Button {
                            id: scenarioButton

                            required property string modelData

                            text: modelData.replace("_", " ").toUpperCase()

                            background: Rectangle {
                                radius: 10
                                color: appController.scenario === modelData ? "#1f4fd1" : "#132234"
                                border.color: appController.scenario === modelData ? "#7cb2ff" : "#27425a"
                                border.width: 1
                            }

                            contentItem: Text {
                                text: scenarioButton.text
                                color: "#f8fbff"
                                font.pixelSize: 13
                                font.bold: true
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            onClicked: appController.scenario = modelData
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Text {
                        text: "Demo Mode"
                        color: "#f8fbff"
                        font.pixelSize: 14
                    }

                    Switch {
                        checked: appController.demoMode
                        onToggled: appController.demoMode = checked
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 12
            color: "#0c1622"
            border.color: "#223648"
            border.width: 1

            GridLayout {
                anchors.fill: parent
                anchors.margins: 12
                columns: appController.displayClass === "standby" ? 1 : 2
                rowSpacing: 10
                columnSpacing: 10

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 10
                    color: "#132234"
                    border.color: "#27425a"

                    Column {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 8

                        Text {
                            text: "Flight Snapshot"
                            color: "#f8fbff"
                            font.pixelSize: 18
                            font.bold: true
                        }

                        Text {
                            text: "IAS " + Number(appController.iasKt).toFixed(0)
                                  + "   ALT " + Number(appController.altitudeFt).toFixed(0)
                                  + "   VS " + Number(appController.verticalSpeedFpm).toFixed(0)
                            color: "#c0d0dd"
                            font.pixelSize: 14
                        }

                        Text {
                            text: "Pitch " + Number(appController.pitchDeg).toFixed(1)
                                  + "   Roll " + Number(appController.rollDeg).toFixed(1)
                                  + "   HDG " + Number(appController.headingDeg).toFixed(0)
                            color: "#c0d0dd"
                            font.pixelSize: 14
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 10
                    color: "#132234"
                    border.color: "#27425a"

                    Column {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 8

                        Text {
                            text: "Bench Notes"
                            color: "#f8fbff"
                            font.pixelSize: 18
                            font.bold: true
                        }

                        Text {
                            text: "This scaffold is ready for touch demo use now and for Pi 5 bench integration with live GPS and CAN hardware next."
                            color: "#c0d0dd"
                            wrapMode: Text.WordWrap
                            font.pixelSize: 14
                        }

                        Text {
                            text: "Keyboard shortcuts: Left/Right pages, 1-6 common pages"
                            color: "#8fb1cb"
                            font.pixelSize: 13
                        }
                    }
                }
            }
        }
    }
}
