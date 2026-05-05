import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    readonly property bool compactClass: appController.displayClass !== "primary"
    property string selectedCylinderSection: "CHT"

    RowLayout {
        anchors.fill: parent
        spacing: 10

        Rectangle {
            Layout.preferredWidth: compactClass ? 470 : 560
            Layout.fillHeight: true
            radius: 14
            color: "#08111b"
            border.color: "#223648"
            border.width: 1

            PfdOverlayDisplay {
                anchors.fill: parent
                anchors.margins: 10
                pitchDeg: appController.pitchDeg
                rollDeg: appController.rollDeg
                slipSkidNormalized: appController.slipSkidNormalized
                iasKt: appController.iasKt
                airspeedTrendKt: appController.airspeedTrendKt
                tasKt: appController.tasKt
                aoaNormalized: appController.aoaNormalized
                altitudeFt: appController.altitudeFt
                verticalSpeedFpm: appController.verticalSpeedFpm
                headingDeg: appController.headingDeg
                courseBearingDeg: appController.waypointBearingDeg
                groundTrackDeg: appController.groundTrackDeg
                groundSpeedKt: appController.groundSpeedKt
                baroSettingInHg: appController.baroSettingInHg
                selectedAltitudeFt: appController.selectedAltitudeFt
                trafficActive: appController.trafficModuleAvailable
                navSource: "GPS"
                courseLabel: "CRS " + ("000" + Number(appController.waypointBearingDeg).toFixed(0)).slice(-3)
                distanceLabel: Number(appController.waypointDistanceNm).toFixed(1) + "NM"
                apModeLabel: appController.autopilotMaster ? appController.autopilotLateralMode : "OFF"
                gpsModeLabel: appController.autopilotVerticalMode
                compact: compactClass
            }
        }

        EngineStrip {
            id: splitEngineStrip
            visible: appController.engineModuleAvailable && appController.displayClass === "primary"
            Layout.preferredWidth: visible ? 116 : 0
            Layout.minimumWidth: visible ? 96 : 0
            Layout.fillHeight: true
            rpm: appController.rpm
            manifoldPressureInHg: appController.manifoldPressureInHg
            oilPressurePsi: appController.oilPressurePsi
            oilTemperatureF: appController.oilTemperatureF
            fuelFlowGph: appController.fuelFlowGph
            fuelQuantityPercent: appController.fuelQuantityPercent
            fuelTankStates: appController.fuelTankStates
            busVoltage: appController.busVoltage
            chtMaxF: appController.chtMaxF
            egtMaxF: appController.egtMaxF
            compact: true

            onFuelDetailsRequested: fuelPopup.open()
            onCylinderDetailsRequested: section => {
                selectedCylinderSection = section
                cylinderPopup.open()
            }
        }

        MapPane {
            Layout.fillWidth: true
            Layout.fillHeight: true
            latitudeDeg: appController.latitudeDeg
            longitudeDeg: appController.longitudeDeg
            groundTrackDeg: appController.groundTrackDeg
            baseChartSource: appController.mapBaseSource
            openAipOverlayEnabled: appController.openAipOverlayEnabled
            weatherOverlayEnabled: appController.weatherOverlayEnabled
            weatherOverlayOpacity: appController.weatherOverlayOpacity
            mapZoomFactor: appController.mapZoomFactor
            showTraffic: appController.trafficModuleAvailable
            trafficCount: appController.trafficCount
            nearestTrafficRangeNm: appController.nearestTrafficRangeNm
            nearestTrafficRelativeAltitudeFt: appController.nearestTrafficRelativeAltitudeFt
            trafficTargets: appController.trafficTargets
            flightPlanLegs: appController.flightPlanLegs
            compact: compactClass
        }
    }

    Popup {
        id: cylinderPopup
        modal: true
        focus: true
        anchors.centerIn: Overlay.overlay
        width: compactClass ? 420 : 520
        height: compactClass ? 280 : 330
        padding: 0

        background: Rectangle {
            radius: 12
            color: "#05080d"
            border.color: "#355069"
            border.width: 1
        }

        ColumnLayout {
            id: cylinderContent
            anchors.fill: parent
            anchors.margins: 16
            spacing: 10

            readonly property var values: selectedCylinderSection === "CHT"
                ? appController.chtValues
                : appController.egtValues
            readonly property real minValue: selectedCylinderSection === "CHT" ? 200 : 1000
            readonly property real maxValue: selectedCylinderSection === "CHT" ? 470 : 1550
            readonly property real cautionValue: selectedCylinderSection === "CHT" ? 400 : 1450
            readonly property real alertValue: selectedCylinderSection === "CHT" ? 430 : 1500

            Text {
                text: selectedCylinderSection + " Detail"
                color: "#f8fbff"
                font.pixelSize: 22
                font.bold: true
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 8

                Repeater {
                    model: cylinderContent.values

                    delegate: Column {
                        required property int index
                        required property real modelData
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 6

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: Number(modelData).toFixed(0)
                            color: "#f8fbff"
                            font.pixelSize: 14
                            font.bold: true
                        }

                        Rectangle {
                            width: parent.width
                            height: cylinderPopup.height * 0.45
                            color: "#0d141d"
                            border.color: "#355069"
                            border.width: 1

                            Rectangle { x: 0; y: 6; width: parent.width; height: 4; color: "#e93939" }
                            Rectangle {
                                x: 0
                                y: 10 + (parent.height - 18) * (1.0 - (cylinderContent.cautionValue - cylinderContent.minValue) / (cylinderContent.maxValue - cylinderContent.minValue))
                                width: parent.width
                                height: 3
                                color: "#f3f556"
                            }
                            Rectangle {
                                x: parent.width * 0.2
                                y: 10 + (parent.height - 18) * (1.0 - (modelData - cylinderContent.minValue) / (cylinderContent.maxValue - cylinderContent.minValue))
                                width: parent.width * 0.6
                                height: Math.max(10, parent.height - (10 + (parent.height - 18) * (1.0 - (modelData - cylinderContent.minValue) / (cylinderContent.maxValue - cylinderContent.minValue))) - 8)
                                color: modelData >= cylinderContent.alertValue ? "#e93939" : modelData >= cylinderContent.cautionValue ? "#f3f556" : "#22d13d"
                            }
                        }

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: selectedCylinderSection + " " + (index + 1)
                            color: "#95aec2"
                            font.pixelSize: 12
                            font.bold: true
                        }
                    }
                }
            }

            Button {
                Layout.alignment: Qt.AlignRight
                text: "Close"
                onClicked: cylinderPopup.close()
            }
        }
    }

    Popup {
        id: fuelPopup
        modal: true
        focus: true
        anchors.centerIn: Overlay.overlay
        width: compactClass ? 460 : 620
        height: compactClass ? 360 : 440
        padding: 0

        background: Rectangle {
            radius: 12
            color: "#05080d"
            border.color: "#355069"
            border.width: 1
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 12

            RowLayout {
                Layout.fillWidth: true

                Text {
                    text: "Fuel"
                    color: "#f8fbff"
                    font.pixelSize: 22
                    font.bold: true
                }

                Item { Layout.fillWidth: true }

                Button {
                    text: "Close"
                    onClicked: fuelPopup.close()
                }
            }

            GridLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: compactClass ? 110 : 126
                columns: 3
                columnSpacing: 12

                Repeater {
                    model: [
                        { title: "Fuel Onboard", value: Number(appController.fuelRemainingGal).toFixed(1) + " GAL", accent: "#f8fbff" },
                        { title: "Fuel Used", value: Number(appController.fuelUsedGal).toFixed(1) + " GAL", accent: "#35d7ff" },
                        { title: "Fuel Flow", value: Number(appController.fuelFlowGph).toFixed(1) + " GPH", accent: "#35d7ff" }
                    ]

                    delegate: Rectangle {
                        required property var modelData
                        Layout.fillWidth: true
                        Layout.preferredHeight: compactClass ? 98 : 112
                        radius: 10
                        color: "#070b12"
                        border.color: "#355069"
                        border.width: 1

                        Column {
                            anchors.centerIn: parent
                            spacing: 8

                            Text {
                                text: modelData.title
                                color: "#c0d0dd"
                                font.pixelSize: 13
                            }

                            Text {
                                text: modelData.value
                                color: modelData.accent
                                font.pixelSize: compactClass ? 22 : 26
                                font.bold: true
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: compactClass ? 130 : 150
                radius: 10
                color: "#070b12"
                border.color: "#355069"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 20

                    Repeater {
                        model: appController.fuelTankStates

                        delegate: ColumnLayout {
                            required property var modelData
                            Layout.fillWidth: true
                            spacing: 8

                            Text {
                                text: modelData.label + " Tank"
                                color: "#f8fbff"
                                font.pixelSize: 14
                                font.bold: true
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 12
                                radius: 6
                                color: "#152232"
                                border.color: "#355069"
                                border.width: 1

                                Rectangle {
                                    width: parent.width * Math.max(0, Math.min(1, Number(modelData.percent || 0) / 100.0))
                                    height: parent.height
                                    radius: 6
                                    color: modelData.lowWarning ? "#e93939"
                                           : modelData.lowCaution ? "#f3f556"
                                           : "#22d13d"
                                }
                            }

                            Text {
                                text: Number(modelData.quantityGal).toFixed(1) + " GAL / " + Number(modelData.capacityGal).toFixed(1)
                                color: "#35d7ff"
                                font.pixelSize: 16
                                font.bold: true
                            }
                        }
                    }
                }
            }

            GridLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                columns: 2
                rowSpacing: 12
                columnSpacing: 12

                Repeater {
                    model: [
                        { title: "Endurance", value: appController.fuelEnduranceHours > 0 ? Number(appController.fuelEnduranceHours).toFixed(2) + " HR" : "--" },
                        { title: "Range Remaining", value: Number(appController.fuelRangeNm).toFixed(0) + " NM" },
                        { title: "Fuel Remaining", value: Number(appController.fuelRemainingGal).toFixed(1) + " GAL" },
                        { title: "Economy", value: Number(appController.groundSpeedKt / Math.max(0.1, appController.fuelFlowGph)).toFixed(1) + " NM/GAL" }
                    ]

                    delegate: Rectangle {
                        required property var modelData
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 10
                        color: "#070b12"
                        border.color: "#355069"
                        border.width: 1

                        Column {
                            anchors.centerIn: parent
                            spacing: 6

                            Text {
                                text: modelData.title
                                color: "#c0d0dd"
                                font.pixelSize: 13
                            }

                            Text {
                                text: modelData.value
                                color: "#35d7ff"
                                font.pixelSize: compactClass ? 22 : 24
                                font.bold: true
                            }
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        visible: !appController.flightDataHealthy
        color: "#77091015"
        border.color: "#ef4444"
        border.width: 2
        radius: 14

        Text {
            anchors.centerIn: parent
            text: "AHRS DATA STALE"
            color: "#f8fbff"
            font.pixelSize: 34
            font.bold: true
        }
    }
}
