import QtQuick
import QtQuick.Layouts
import "../components"

Item {
    readonly property bool compactClass: appController.displayClass !== "primary"

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        PfdOverlayDisplay {
            Layout.fillWidth: true
            Layout.fillHeight: true
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
            compact: compactClass
        }

        EngineStrip {
            visible: appController.engineModuleAvailable && appController.displayClass === "primary"
            Layout.fillWidth: true
            Layout.preferredHeight: 110
            compact: true
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

        FlightSummaryCard {
            visible: appController.engineModuleAvailable && appController.displayClass !== "primary"
            Layout.fillWidth: true
            Layout.preferredHeight: 88
            title: "RPM / FF"
            value: Number(appController.rpm).toFixed(0)
            subtitle: Number(appController.fuelFlowGph).toFixed(1) + " GPH"
        }
    }
}
