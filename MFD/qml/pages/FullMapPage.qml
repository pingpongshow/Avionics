import QtQuick
import "../components"

Item {
    MapPane {
        anchors.fill: parent
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
        compact: appController.displayClass !== "primary"
    }
}
