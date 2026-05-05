import QtQuick
import QtQuick.Layouts
import "../components"

Item {
    RowLayout {
        anchors.fill: parent
        spacing: 10

        EngineStrip {
            Layout.preferredWidth: appController.displayClass === "primary" ? 280 : 220
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
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 110
                spacing: 10

                FlightSummaryCard {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    title: "RPM"
                    value: Number(appController.rpm).toFixed(0)
                    subtitle: "PROP / ENGINE"
                }

                FlightSummaryCard {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    title: "OIL"
                    value: Number(appController.oilTemperatureF).toFixed(0) + " F"
                    subtitle: Number(appController.oilPressurePsi).toFixed(0) + " PSI"
                }

                FlightSummaryCard {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    title: "ELEC"
                    value: Number(appController.busVoltage).toFixed(1) + " V"
                    subtitle: "Bus voltage"
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
                showTraffic: false
                trafficCount: 0
                trafficTargets: []
                flightPlanLegs: appController.flightPlanLegs
                compact: true
            }
        }
    }
}
