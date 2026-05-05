import QtQuick
import QtQuick.Layouts
import "../components"

Item {
    TrafficRadar {
        anchors.fill: parent
        trafficCount: Math.max(1, appController.trafficCount)
        groundTrackDeg: appController.groundTrackDeg
        nearestTrafficRangeNm: appController.nearestTrafficRangeNm
        nearestTrafficRelativeAltitudeFt: appController.nearestTrafficRelativeAltitudeFt
        trafficTargets: appController.trafficTargets
    }

    Rectangle {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 12
        width: appController.displayClass === "primary" ? 240 : 200
        height: 118
        radius: 12
        color: "#b00f1823"
        border.color: "#27425a"
        border.width: 1

        Column {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 6

            Text {
                text: "TRAFFIC SUMMARY"
                color: "#f8fbff"
                font.pixelSize: 16
                font.bold: true
            }

            Text {
                text: "Targets  " + appController.trafficCount
                color: "#d0dae4"
                font.pixelSize: 13
            }

            Text {
                text: "Nearest  " + Number(appController.nearestTrafficRangeNm).toFixed(1) + " NM"
                color: "#d0dae4"
                font.pixelSize: 13
            }

            Text {
                text: "Rel Alt  " + Number(appController.nearestTrafficRelativeAltitudeFt).toFixed(0) + " FT"
                color: "#d0dae4"
                font.pixelSize: 13
            }

            Text {
                text: "Ownship  " + Number(appController.groundSpeedKt).toFixed(0) + " KT  TRK " + ("000" + Number(appController.groundTrackDeg).toFixed(0)).slice(-3)
                color: "#8fd3ff"
                font.pixelSize: 12
            }
        }
    }
}
