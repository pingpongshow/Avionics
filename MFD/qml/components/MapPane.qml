import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property real latitudeDeg: 0
    property real longitudeDeg: 0
    property real groundTrackDeg: 0
    property string baseChartSource: "demo"
    property bool showTraffic: false
    property int trafficCount: 0
    property bool compact: false
    property real nearestTrafficRangeNm: 0
    property real nearestTrafficRelativeAltitudeFt: 0
    property var trafficTargets: []
    property var flightPlanLegs: []
    property bool openAipOverlayEnabled: false
    property bool weatherOverlayEnabled: false
    property real weatherOverlayOpacity: 0.35
    property real mapZoomFactor: 1.0

    property string mapPanelMode: "map"
    property string recordsMode: ""
    property string selectedStoredPlanId: ""
    property string selectedUserWaypointIdent: ""
    property var visibleReferenceWaypoints: []
    property real panLatitudeDeg: 0
    property real panLongitudeDeg: 0
    property real lastOverlayCenterLatitudeDeg: Number.NaN
    property real lastOverlayCenterLongitudeDeg: Number.NaN
    property real lastOverlayLatSpanDeg: 0
    property real lastOverlayLonSpanDeg: 0
    property int selectedFlightPlanIndex: -1

    readonly property real chartCenterLatitudeDeg: latitudeDeg + panLatitudeDeg
    readonly property real chartCenterLongitudeDeg: longitudeDeg + panLongitudeDeg
    readonly property bool mapCenteredOnOwnship: Math.abs(panLatitudeDeg) < 0.00001 && Math.abs(panLongitudeDeg) < 0.00001

    function updateChartViewport() {
        if (typeof chartManager === "undefined") {
            return
        }
        chartManager.updateViewport(width, height, chartCenterLatitudeDeg, chartCenterLongitudeDeg, mapZoomFactor)
    }

    function shouldRefreshReferenceOverlay(bounds) {
        const latSpan = Math.abs(bounds.northLat - bounds.southLat)
        const lonSpan = Math.abs(bounds.eastLon - bounds.westLon)
        if (!Number.isFinite(lastOverlayCenterLatitudeDeg) || !Number.isFinite(lastOverlayCenterLongitudeDeg)) {
            return true
        }

        const latMove = Math.abs(chartCenterLatitudeDeg - lastOverlayCenterLatitudeDeg)
        const lonMove = Math.abs(chartCenterLongitudeDeg - lastOverlayCenterLongitudeDeg)
        const latThreshold = Math.max(0.0025, lastOverlayLatSpanDeg * 0.18)
        const lonThreshold = Math.max(0.0025, lastOverlayLonSpanDeg * 0.18)
        const latScaleChanged = Math.abs(latSpan - lastOverlayLatSpanDeg) > Math.max(0.0025, lastOverlayLatSpanDeg * 0.12)
        const lonScaleChanged = Math.abs(lonSpan - lastOverlayLonSpanDeg) > Math.max(0.0025, lastOverlayLonSpanDeg * 0.12)
        return latMove > latThreshold || lonMove > lonThreshold || latScaleChanged || lonScaleChanged
    }

    function queueOverlayRefresh(forceRefresh) {
        overlayRefreshTimer.forceRefresh = overlayRefreshTimer.forceRefresh || !!forceRefresh
        overlayRefreshTimer.restart()
    }

    function updateLiveOverlays() {
        if (typeof chartManager === "undefined") {
            waypointCanvas.requestPaint()
            return
        }

        const bounds = chartManager.viewportBounds()
        if (typeof navDataManager !== "undefined"
            && (overlayRefreshTimer.forceRefresh || shouldRefreshReferenceOverlay(bounds))) {
                visibleReferenceWaypoints = navDataManager.referenceWaypointsInView(
                    bounds.southLat,
                    bounds.westLon,
                    bounds.northLat,
                    bounds.eastLon,
                    compact ? 80 : 140
                )
            lastOverlayCenterLatitudeDeg = chartCenterLatitudeDeg
            lastOverlayCenterLongitudeDeg = chartCenterLongitudeDeg
            lastOverlayLatSpanDeg = Math.abs(bounds.northLat - bounds.southLat)
            lastOverlayLonSpanDeg = Math.abs(bounds.eastLon - bounds.westLon)
        }
        if (typeof weatherDataManager !== "undefined") {
            weatherDataManager.updateViewport(
                bounds.southLat,
                bounds.westLon,
                bounds.northLat,
                bounds.eastLon,
                Math.max(320, Math.round(width)),
                Math.max(320, Math.round(height))
            )
        }
        overlayRefreshTimer.forceRefresh = false
        waypointCanvas.requestPaint()
    }

    function setInteractiveZoom(nextZoom) {
        const clamped = Math.max(0.6, Math.min(3.0, nextZoom))
        if (typeof appController !== "undefined") {
            appController.mapZoomFactor = clamped
        } else {
            root.mapZoomFactor = clamped
        }
    }

    function currentFlightPlanIds() {
        const identifiers = []
        for (let i = 0; i < root.flightPlanLegs.length; ++i) {
            identifiers.push(String(root.flightPlanLegs[i].ident || ""))
        }
        return identifiers
    }

    function moveFlightPlanWaypoint(fromIndex, toIndex) {
        const identifiers = currentFlightPlanIds()
        if (fromIndex < 0 || fromIndex >= identifiers.length || toIndex < 0 || toIndex >= identifiers.length || fromIndex === toIndex) {
            return
        }
        const movedIdent = identifiers.splice(fromIndex, 1)[0]
        identifiers.splice(toIndex, 0, movedIdent)
        appController.setFlightPlanWaypoints(identifiers)
        selectedFlightPlanIndex = toIndex
    }

    function deleteFlightPlanWaypoint(index) {
        const identifiers = currentFlightPlanIds()
        if (index < 0 || index >= identifiers.length) {
            return
        }
        identifiers.splice(index, 1)
        if (identifiers.length === 0) {
            appController.clearFlightPlan()
        } else {
            appController.setFlightPlanWaypoints(identifiers)
        }
        selectedFlightPlanIndex = -1
    }

    function reverseFlightPlan() {
        const identifiers = currentFlightPlanIds()
        identifiers.reverse()
        appController.setFlightPlanWaypoints(identifiers)
    }

    function openFlightPlanWaypointActions(index) {
        selectedFlightPlanIndex = index
        flightPlanActionPopup.open()
    }

    function currentNearestAirports() {
        if (typeof appController === "undefined") {
            return []
        }
        return appController.nearestAirports(16)
    }

    function showMapPanel() {
        root.mapPanelMode = "map"
    }

    function mapPointForCoordinate(mapLatitudeDeg, mapLongitudeDeg) {
        if (typeof chartManager !== "undefined") {
            return chartManager.projectCoordinateToViewport(mapLatitudeDeg, mapLongitudeDeg)
        }
        return {
            valid: true,
            inside: true,
            x: width * 0.5 + (mapLongitudeDeg - chartCenterLongitudeDeg) * 220 * mapZoomFactor,
            y: height * 0.55 - (mapLatitudeDeg - chartCenterLatitudeDeg) * 260 * mapZoomFactor
        }
    }

    function searchResults(queryText, limit) {
        if (typeof appController === "undefined") {
            return []
        }
        return appController.searchWaypoints(queryText, limit)
    }

    function openRecords(mode) {
        recordsMode = mode
        recordsPopup.open()
    }

    function storedPlanIds() {
        return navDataManager.storedFlightPlans
    }

    function recenterOnOwnship() {
        panLatitudeDeg = 0
        panLongitudeDeg = 0
        updateChartViewport()
        queueOverlayRefresh(true)
        routeCanvas.requestPaint()
        ownshipCanvas.requestPaint()
    }

    radius: 14
    color: "#536f2e"
    border.color: "#23384b"
    border.width: 1
    clip: true

    Component.onCompleted: {
        updateChartViewport()
        queueOverlayRefresh(true)
    }
    onWidthChanged: {
        updateChartViewport()
        queueOverlayRefresh(true)
        routeCanvas.requestPaint()
    }
    onHeightChanged: {
        updateChartViewport()
        queueOverlayRefresh(true)
        routeCanvas.requestPaint()
    }
    onLatitudeDegChanged: {
        updateChartViewport()
        queueOverlayRefresh(false)
        routeCanvas.requestPaint()
        ownshipCanvas.requestPaint()
    }
    onLongitudeDegChanged: {
        updateChartViewport()
        queueOverlayRefresh(false)
        routeCanvas.requestPaint()
        ownshipCanvas.requestPaint()
    }
    onPanLatitudeDegChanged: {
        updateChartViewport()
        queueOverlayRefresh(false)
        routeCanvas.requestPaint()
        ownshipCanvas.requestPaint()
    }
    onPanLongitudeDegChanged: {
        updateChartViewport()
        queueOverlayRefresh(false)
        routeCanvas.requestPaint()
        ownshipCanvas.requestPaint()
    }
    onMapZoomFactorChanged: {
        updateChartViewport()
        queueOverlayRefresh(true)
        routeCanvas.requestPaint()
        ownshipCanvas.requestPaint()
    }
    onBaseChartSourceChanged: {
        updateChartViewport()
        queueOverlayRefresh(true)
        previewCanvas.requestPaint()
        routeCanvas.requestPaint()
    }
    onWeatherOverlayEnabledChanged: queueOverlayRefresh(true)
    onWeatherOverlayOpacityChanged: queueOverlayRefresh(false)
    onTrafficTargetsChanged: ownshipCanvas.requestPaint()
    onFlightPlanLegsChanged: {
        routeCanvas.requestPaint()
        waypointCanvas.requestPaint()
    }
    onVisibleReferenceWaypointsChanged: waypointCanvas.requestPaint()

    Timer {
        id: overlayRefreshTimer
        interval: 140
        repeat: false
        property bool forceRefresh: false
        onTriggered: root.updateLiveOverlays()
    }

    Item {
        anchors.fill: parent
        visible: root.mapPanelMode === "map"
        clip: true

        Repeater {
            model: chartManager.visibleTiles

            delegate: Image {
                required property var modelData

                source: modelData.sourceUrl
                x: modelData.x
                y: modelData.y
                width: modelData.width
                height: modelData.height
                fillMode: Image.Stretch
                asynchronous: true
                cache: true
                visible: chartManager.chartLoaded
            }
        }

        Canvas {
            id: previewCanvas
            anchors.fill: parent
            visible: !chartManager.chartLoaded

            onPaint: {
                const ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)

                const landColor = root.baseChartSource === "faa_vfr"
                    ? "#d7c78d"
                    : root.baseChartSource === "openflightmaps"
                        ? "#d4d0a5"
                        : "#7f9f47"
                const waterColor = root.baseChartSource === "faa_vfr"
                    ? "#6e97d1"
                    : root.baseChartSource === "openflightmaps"
                        ? "#5d8fcd"
                        : "#274a94"

                ctx.fillStyle = landColor
                ctx.fillRect(0, 0, width, height)

                ctx.fillStyle = waterColor
                ctx.beginPath()
                ctx.moveTo(width * 0.52, 0)
                ctx.lineTo(width, 0)
                ctx.lineTo(width, height * 0.68)
                ctx.bezierCurveTo(width * 0.84, height * 0.54, width * 0.72, height * 0.38, width * 0.60, height * 0.24)
                ctx.lineTo(width * 0.48, height * 0.16)
                ctx.closePath()
                ctx.fill()

                ctx.strokeStyle = "#bfae76"
                ctx.lineWidth = 1
                for (let x = 0; x < width; x += 48) {
                    ctx.beginPath()
                    ctx.moveTo(x, 0)
                    ctx.lineTo(x, height)
                    ctx.stroke()
                }
                for (let y = 0; y < height; y += 48) {
                    ctx.beginPath()
                    ctx.moveTo(0, y)
                    ctx.lineTo(width, y)
                    ctx.stroke()
                }

                ctx.strokeStyle = "#2c5ea8"
                ctx.lineWidth = 8
                ctx.beginPath()
                ctx.moveTo(width * 0.18, height * 0.12)
                ctx.bezierCurveTo(width * 0.34, height * 0.22, width * 0.52, height * 0.62, width * 0.68, height * 0.9)
                ctx.stroke()

                ctx.strokeStyle = "#d31fe3"
                ctx.lineWidth = 2
                const airspaces = [
                    [0.10, 0.24, 0.18, 0.13, 0.31, 0.18, 0.24, 0.28],
                    [0.72, 0.10, 0.90, 0.07, 0.96, 0.20, 0.82, 0.24],
                    [0.78, 0.74, 0.93, 0.70, 0.98, 0.88, 0.82, 0.92]
                ]
                for (let p = 0; p < airspaces.length; ++p) {
                    const poly = airspaces[p]
                    ctx.beginPath()
                    ctx.moveTo(width * poly[0], height * poly[1])
                    for (let i = 2; i < poly.length; i += 2) {
                        ctx.lineTo(width * poly[i], height * poly[i + 1])
                    }
                    ctx.closePath()
                    ctx.stroke()
                }
            }
        }

        Canvas {
            id: routeCanvas
            anchors.fill: parent

            onPaint: {
                const ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)

                if (root.flightPlanLegs.length === 0) {
                    return
                }

                ctx.strokeStyle = "#ff33ef"
                ctx.lineWidth = 4
                ctx.beginPath()
                const ownshipPoint = root.mapPointForCoordinate(root.latitudeDeg, root.longitudeDeg)
                if (!ownshipPoint.valid) {
                    return
                }
                ctx.moveTo(ownshipPoint.x, ownshipPoint.y)

                for (let i = 0; i < root.flightPlanLegs.length; ++i) {
                    const leg = root.flightPlanLegs[i]
                    if (!leg.validPosition) {
                        continue
                    }
                    const point = root.mapPointForCoordinate(Number(leg.latitudeDeg || 0), Number(leg.longitudeDeg || 0))
                    if (point.valid) {
                        ctx.lineTo(point.x, point.y)
                    }
                }
                ctx.stroke()

                ctx.fillStyle = "#35d7ff"
                ctx.font = "bold 12px Menlo"
                ctx.textAlign = "center"
                for (let i = 0; i < root.flightPlanLegs.length; ++i) {
                    const leg = root.flightPlanLegs[i]
                    if (!leg.validPosition) {
                        continue
                    }
                    const point = root.mapPointForCoordinate(Number(leg.latitudeDeg || 0), Number(leg.longitudeDeg || 0))
                    if (!point.valid || !point.inside) {
                        continue
                    }

                    ctx.beginPath()
                    ctx.moveTo(point.x, point.y - 10)
                    ctx.lineTo(point.x - 7, point.y + 3)
                    ctx.lineTo(point.x + 7, point.y + 3)
                    ctx.closePath()
                    ctx.fill()

                    ctx.fillText(String(leg.ident || ""), point.x, point.y - 16)
                }
            }
        }

        Canvas {
            id: waypointCanvas
            anchors.fill: parent

            onPaint: {
                const ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)

                if (root.visibleReferenceWaypoints.length === 0) {
                    return
                }

                ctx.font = "bold 11px Menlo"
                ctx.textAlign = "center"
                ctx.lineWidth = 1.5

                function drawAirport(x, y) {
                    ctx.beginPath()
                    ctx.fillStyle = "#49d9ff"
                    ctx.moveTo(x, y - 7)
                    ctx.lineTo(x - 6, y + 5)
                    ctx.lineTo(x + 6, y + 5)
                    ctx.closePath()
                    ctx.fill()
                }

                function drawNavaid(x, y) {
                    ctx.save()
                    ctx.translate(x, y)
                    ctx.rotate(Math.PI / 4)
                    ctx.strokeStyle = "#ff7efb"
                    ctx.strokeRect(-5, -5, 10, 10)
                    ctx.restore()
                }

                function drawFix(x, y) {
                    ctx.strokeStyle = "#f8fbff"
                    ctx.beginPath()
                    ctx.arc(x, y, 4, 0, Math.PI * 2)
                    ctx.stroke()
                }

                for (let i = 0; i < root.visibleReferenceWaypoints.length; ++i) {
                    const item = root.visibleReferenceWaypoints[i]
                    const point = root.mapPointForCoordinate(Number(item.latitudeDeg || 0), Number(item.longitudeDeg || 0))
                    if (!point.valid || !point.inside) {
                        continue
                    }

                    const symbolType = String(item.symbolType || item.type || "").toLowerCase()
                    if (symbolType === "airport") {
                        drawAirport(point.x, point.y)
                    } else if (symbolType === "navaid") {
                        drawNavaid(point.x, point.y)
                    } else {
                        drawFix(point.x, point.y)
                    }

                    ctx.fillStyle = symbolType === "navaid" ? "#ff7efb" : "#49d9ff"
                    ctx.fillText(String(item.ident || ""), point.x, point.y - 12)
                }
            }
        }

        Image {
            id: weatherOverlayImage
            anchors.fill: parent
            visible: root.weatherOverlayEnabled
                && root.mapPanelMode === "map"
                && typeof weatherDataManager !== "undefined"
                && weatherDataManager.radarOverlayUrl !== ""
            opacity: root.weatherOverlayOpacity
            source: typeof weatherDataManager !== "undefined" ? weatherDataManager.radarOverlayUrl : ""
            fillMode: Image.Stretch
            asynchronous: true
            cache: false
            smooth: true
        }

        Canvas {
            id: ownshipCanvas
            anchors.fill: parent

            onPaint: {
                const ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)

                const ownshipPoint = root.mapPointForCoordinate(root.latitudeDeg, root.longitudeDeg)
                const cx = ownshipPoint.valid ? ownshipPoint.x : width * 0.5
                const cy = ownshipPoint.valid ? ownshipPoint.y : height * 0.55
                ctx.save()
                ctx.translate(cx, cy)
                ctx.rotate(root.groundTrackDeg * Math.PI / 180)
                ctx.strokeStyle = "#ffffff"
                ctx.lineWidth = 3
                ctx.beginPath()
                ctx.moveTo(0, -20)
                ctx.lineTo(12, 16)
                ctx.lineTo(0, 8)
                ctx.lineTo(-12, 16)
                ctx.closePath()
                ctx.stroke()
                ctx.restore()

                if (root.showTraffic && root.trafficCount > 0 && root.trafficTargets.length > 0) {
                    for (let i = 0; i < Math.min(root.trafficCount, root.trafficTargets.length); ++i) {
                        const item = root.trafficTargets[i]
                        const relAlt = Number(item.relativeAltitudeFt || 0)
                        const style = item.style || "open"
                        const point = item.latitudeDeg !== undefined && item.longitudeDeg !== undefined
                            ? root.mapPointForCoordinate(Number(item.latitudeDeg), Number(item.longitudeDeg))
                            : { valid: false, inside: false, x: cx, y: cy }
                        const targetX = point.x
                        const targetY = point.y

                        if ((point.valid && !point.inside) || targetX < -24 || targetX > width + 24 || targetY < -24 || targetY > height + 24) {
                            continue
                        }

                        ctx.save()
                        ctx.translate(targetX, targetY)
                        ctx.rotate(Math.PI / 4)
                        if (style === "filled") {
                            ctx.fillStyle = "#facc15"
                            ctx.fillRect(-8, -8, 16, 16)
                        } else {
                            ctx.strokeStyle = "#ffffff"
                            ctx.lineWidth = 2
                            ctx.strokeRect(-8, -8, 16, 16)
                        }
                        ctx.restore()

                        ctx.fillStyle = "#ffffff"
                        ctx.font = "bold 12px Arial"
                        ctx.textAlign = "center"
                        ctx.fillText((relAlt >= 0 ? "+" : "") + Math.round(relAlt / 100), targetX, targetY - 14)
                    }
                }
            }
        }

        MouseArea {
            id: dragArea
            anchors.fill: parent
            enabled: root.mapPanelMode === "map"
            acceptedButtons: Qt.LeftButton
            preventStealing: true
            hoverEnabled: false
            propagateComposedEvents: false

            property real pressX: 0
            property real pressY: 0
            property real startPanLatitudeDeg: 0
            property real startPanLongitudeDeg: 0
            property real startLatSpanDeg: 0
            property real startLonSpanDeg: 0

            onPressed: mouse => {
                const bounds = chartManager.viewportBounds()
                pressX = mouse.x
                pressY = mouse.y
                startPanLatitudeDeg = root.panLatitudeDeg
                startPanLongitudeDeg = root.panLongitudeDeg
                startLatSpanDeg = Math.abs(bounds.northLat - bounds.southLat)
                startLonSpanDeg = Math.abs(bounds.eastLon - bounds.westLon)
            }

            onPositionChanged: mouse => {
                if (!(mouse.buttons & Qt.LeftButton)) {
                    return
                }
                const dx = mouse.x - pressX
                const dy = mouse.y - pressY
                root.panLongitudeDeg = startPanLongitudeDeg - (dx / Math.max(1, width)) * startLonSpanDeg
                root.panLatitudeDeg = startPanLatitudeDeg + (dy / Math.max(1, height)) * startLatSpanDeg
            }

            onDoubleClicked: root.recenterOnOwnship()
        }

        Button {
            visible: root.mapPanelMode === "map" && !root.mapCenteredOnOwnship
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.topMargin: 10
            anchors.rightMargin: 10
            width: compact ? 42 : 48
            height: compact ? 42 : 48
            text: "CTR"
            z: 5

            background: Rectangle {
                radius: width / 2
                color: "#0d141d"
                border.color: "#c0c7d1"
                border.width: 1
            }

            contentItem: Text {
                text: parent.text
                color: "#f8fbff"
                font.pixelSize: compact ? 10 : 11
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            onClicked: root.recenterOnOwnship()
        }
    }

    TrafficRadar {
        anchors.fill: parent
        visible: root.mapPanelMode === "trf"
        trafficCount: Math.max(1, root.trafficCount)
        groundTrackDeg: root.groundTrackDeg
        nearestTrafficRangeNm: root.nearestTrafficRangeNm
        nearestTrafficRelativeAltitudeFt: root.nearestTrafficRelativeAltitudeFt
        trafficTargets: root.trafficTargets
    }

    Rectangle {
        anchors.fill: parent
        visible: root.mapPanelMode === "eng"
        color: "#04070c"

        RowLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 12

            EngineStrip {
                Layout.preferredWidth: root.compact ? 120 : 154
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
            }

            Rectangle {
                id: engineTabsPanel
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#060c13"
                border.color: "#223648"
                border.width: 1
                radius: 12

                property int selectedTab: 0

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 12

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Repeater {
                            model: [
                                { label: "Electrical", index: 0 },
                                { label: "Fuel", index: 1 }
                            ]

                            delegate: Button {
                                required property var modelData
                                Layout.preferredWidth: root.compact ? 116 : 132
                                text: modelData.label

                                background: Rectangle {
                                    radius: 6
                                    color: parent.down || parent.highlighted ? "#1f4fd1" : "#10171f"
                                    border.color: parent.highlighted ? "#7cb2ff" : "#355069"
                                    border.width: 1
                                }

                                contentItem: Text {
                                    text: parent.text
                                    color: "#f8fbff"
                                    font.pixelSize: 13
                                    font.bold: true
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }

                                highlighted: engineTabsPanel.selectedTab === modelData.index
                                onClicked: engineTabsPanel.selectedTab = modelData.index
                            }
                        }
                    }

                    StackLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        currentIndex: engineTabsPanel.selectedTab

                        ColumnLayout {
                            spacing: 12

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 72
                                radius: 10
                                color: "#0c1622"
                                border.color: "#355069"
                                border.width: 1

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 14
                                    spacing: 16

                                    Repeater {
                                        model: [
                                            { title: "BUS", value: Number(appController.busVoltage).toFixed(1) + " V" },
                                            { title: "LOAD", value: Number(Math.max(3.0, appController.busVoltage * 1.6 - 18.0)).toFixed(1) + " A" },
                                            { title: "FIELD", value: appController.engineModuleAvailable ? "ONLINE" : "OFFLINE" }
                                        ]

                                        delegate: ColumnLayout {
                                            required property var modelData
                                            Layout.fillWidth: true
                                            spacing: 4

                                            Text {
                                                text: modelData.title
                                                color: "#9db3c7"
                                                font.pixelSize: 12
                                                font.bold: true
                                            }

                                            Text {
                                                text: modelData.value
                                                color: "#f8fbff"
                                                font.pixelSize: 22
                                                font.bold: true
                                            }
                                        }
                                    }
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                radius: 10
                                color: "#0c1622"
                                border.color: "#355069"
                                border.width: 1

                                GridLayout {
                                    anchors.fill: parent
                                    anchors.margins: 16
                                    columns: 2
                                    rowSpacing: 12
                                    columnSpacing: 20

                                    Repeater {
                                        model: [
                                            { title: "Alternator", value: Number(Math.max(12.0, appController.busVoltage * 2.15)).toFixed(1) + " A" },
                                            { title: "Battery", value: Number(Math.max(2.5, appController.busVoltage * 0.44)).toFixed(1) + " A" },
                                            { title: "Main Bus", value: Number(appController.busVoltage).toFixed(1) + " V" },
                                            { title: "Avionics Bus", value: Number(Math.max(0.0, appController.busVoltage - 0.2)).toFixed(1) + " V" }
                                        ]

                                        delegate: ColumnLayout {
                                            required property var modelData
                                            Layout.fillWidth: true
                                            spacing: 3

                                            Text {
                                                text: modelData.title
                                                color: "#9db3c7"
                                                font.pixelSize: 12
                                            }

                                            Text {
                                                text: modelData.value
                                                color: "#35d7ff"
                                                font.pixelSize: 20
                                                font.bold: true
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        ColumnLayout {
                            spacing: 12

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 84
                                radius: 10
                                color: "#0c1622"
                                border.color: "#355069"
                                border.width: 1

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 14
                                    spacing: 16

                                    Repeater {
                                        model: appController.fuelTankStates

                                        delegate: ColumnLayout {
                                            required property var modelData
                                            Layout.fillWidth: true
                                            spacing: 4

                                            Text {
                                                text: modelData.label
                                                color: "#9db3c7"
                                                font.pixelSize: 12
                                                font.bold: true
                                            }

                                            Text {
                                                text: Number(modelData.quantityGal).toFixed(1) + " GAL"
                                                color: "#f8fbff"
                                                font.pixelSize: 22
                                                font.bold: true
                                            }
                                        }
                                    }
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                radius: 10
                                color: "#0c1622"
                                border.color: "#355069"
                                border.width: 1

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 16
                                    spacing: 12

                                    GridLayout {
                                        Layout.fillWidth: true
                                        columns: 2
                                        rowSpacing: 12
                                        columnSpacing: 20

                                        Repeater {
                                            model: [
                                                { title: "Fuel Flow", value: Number(appController.fuelFlowGph).toFixed(1) + " GPH" },
                                                { title: "Remaining", value: Number(appController.fuelRemainingGal).toFixed(1) + " GAL" },
                                                { title: "Endurance", value: Number(appController.fuelEnduranceHours).toFixed(2) + " HR" },
                                                { title: "Range", value: Number(appController.fuelRangeNm).toFixed(0) + " NM" }
                                            ]

                                            delegate: ColumnLayout {
                                                required property var modelData
                                                Layout.fillWidth: true
                                                spacing: 3

                                                Text {
                                                    text: modelData.title
                                                    color: "#9db3c7"
                                                    font.pixelSize: 12
                                                }

                                                Text {
                                                    text: modelData.value
                                                    color: "#35d7ff"
                                                    font.pixelSize: 20
                                                    font.bold: true
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        anchors.right: parent.right
        anchors.bottom: toolbar.top
        anchors.rightMargin: 12
        anchors.bottomMargin: 12
        width: compact ? 42 : 46
        height: compact ? 116 : 132
        radius: 10
        color: "#990f1823"
        border.color: "#27425a"

        Column {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 6

            Repeater {
                model: [
                    { label: "+", value: root.mapZoomFactor + 0.2 },
                    { label: "-", value: root.mapZoomFactor - 0.2 },
                    { label: "1:1", value: 1.0 }
                ]

                delegate: Rectangle {
                    required property int index
                    required property var modelData

                    width: parent.width
                    height: index === 2 ? (compact ? 24 : 26) : (compact ? 28 : 32)
                    radius: height / 2
                    color: "#0d141d"
                    border.color: "#c0c7d1"
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: modelData.label
                        color: index === 2 ? "#9de29b" : "#f8fbff"
                        font.pixelSize: compact ? (index === 2 ? 9 : 18) : (index === 2 ? 10 : 20)
                        font.bold: true
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: root.setInteractiveZoom(modelData.value)
                    }
                }
            }

            Text {
                width: parent.width
                text: Number(root.mapZoomFactor).toFixed(1) + "x"
                color: "#f8fbff"
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: compact ? 10 : 11
                font.bold: true
            }
        }
    }

    Rectangle {
        id: toolbar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: compact ? 38 : 44
        color: "#000000"
        border.color: "#2d3744"
        border.width: 1

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8
            spacing: 8

            Repeater {
                model: [
                    { key: "map", label: "Map", enabledTile: true },
                    { key: "wpt", label: "Wpt", enabledTile: true },
                    { key: "fpl", label: "FPL", enabledTile: true },
                    { key: "wx", label: "WX", enabledTile: true },
                    { key: "trf", label: "Trf", enabledTile: root.showTraffic },
                    { key: "eng", label: "Eng", enabledTile: appController.engineModuleAvailable },
                    { key: "ap", label: "AP", enabledTile: true },
                    { key: "mnu", label: "Mnu", enabledTile: true }
                ]

                delegate: Button {
                    id: softKey

                    required property var modelData

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    enabled: modelData.enabledTile
                    text: modelData.label

                    readonly property bool selected: (modelData.key === "map" && root.mapPanelMode === "map" && !root.weatherOverlayEnabled)
                        || (modelData.key === "wx" && root.weatherOverlayEnabled && root.mapPanelMode === "map")
                        || (modelData.key === "trf" && root.mapPanelMode === "trf")
                        || (modelData.key === "eng" && root.mapPanelMode === "eng")
                        || (modelData.key === "ap" && appController.autopilotMaster)

                    background: Rectangle {
                        radius: 5
                        color: softKey.selected ? "#1f4fd1" : "#10171f"
                        border.color: softKey.selected ? "#7cb2ff" : "#355069"
                        border.width: 1
                        opacity: softKey.enabled ? 1.0 : 0.45
                    }

                    contentItem: Text {
                        text: softKey.text
                        color: "#f8fbff"
                        font.pixelSize: compact ? 12 : 13
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: {
                        if (modelData.key === "map") {
                            root.mapPanelMode = "map"
                            if (typeof appController !== "undefined") {
                                appController.weatherOverlayEnabled = false
                            }
                        } else if (modelData.key === "wpt") {
                            root.mapPanelMode = "map"
                            waypointPopup.open()
                        } else if (modelData.key === "fpl") {
                            root.mapPanelMode = "map"
                            flightPlanPopup.open()
                        } else if (modelData.key === "wx") {
                            root.mapPanelMode = "map"
                            if (typeof appController !== "undefined") {
                                appController.weatherOverlayEnabled = !appController.weatherOverlayEnabled
                            }
                        } else if (modelData.key === "trf") {
                            root.mapPanelMode = "trf"
                        } else if (modelData.key === "eng") {
                            root.mapPanelMode = "eng"
                        } else if (modelData.key === "ap") {
                            autopilotPopup.open()
                        } else if (modelData.key === "mnu") {
                            menuPopup.open()
                        }
                    }
                }
            }
        }
    }

    Popup {
        id: waypointPopup
        modal: true
        focus: true
        anchors.centerIn: Overlay.overlay
        width: compact ? Math.min(root.width - 24, 540) : Math.min(root.width - 36, 680)
        height: compact ? 360 : 430
        padding: 0
        onOpened: {
            waypointField.forceActiveFocus()
            waypointField.selectAll()
        }

        background: Rectangle {
            radius: 12
            color: "#08111b"
            border.color: "#355069"
            border.width: 1
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 10

            Text {
                text: "Direct-To Waypoint"
                color: "#f8fbff"
                font.pixelSize: 22
                font.bold: true
            }

            TextField {
                id: waypointField
                Layout.fillWidth: true
                placeholderText: "Enter waypoint ident"
                text: appController.activeWaypointLabel
                font.pixelSize: 18
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#04070c"
                border.color: "#355069"
                border.width: 1
                radius: 8

                ListView {
                    anchors.fill: parent
                    anchors.margins: 8
                    clip: true
                    spacing: 6
                    model: root.searchResults(waypointField.text, 10)

                    delegate: Rectangle {
                        required property var modelData

                        width: ListView.view.width
                        height: 54
                        radius: 6
                        color: "#0d141d"
                        border.color: "#355069"
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10

                            ColumnLayout {
                                Layout.fillWidth: true

                                Text {
                                    text: modelData.ident
                                    color: "#56d6ff"
                                    font.pixelSize: 17
                                    font.bold: true
                                }

                                Text {
                                    text: modelData.name + "  |  " + modelData.type
                                    color: "#c0d0dd"
                                    font.pixelSize: 12
                                    elide: Text.ElideRight
                                }
                            }

                            Text {
                                text: Number(modelData.latitudeDeg).toFixed(3) + ", " + Number(modelData.longitudeDeg).toFixed(3)
                                color: "#9db3c7"
                                font.pixelSize: 11
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: waypointField.text = modelData.ident
                        }
                    }
                }
            }

            Text {
                text: "Sets the active waypoint, magenta course line, and route guidance source."
                color: "#b8c9d8"
                wrapMode: Text.WordWrap
                font.pixelSize: 13
            }

            RowLayout {
                Layout.fillWidth: true

                Button {
                    Layout.fillWidth: true
                    text: "Cancel"
                    onClicked: waypointPopup.close()
                }

                Button {
                    Layout.fillWidth: true
                    text: "Nrst"
                    onClicked: nearestPopup.open()
                }

                Button {
                    Layout.fillWidth: true
                    text: "Direct-To"
                    onClicked: {
                        appController.setDirectWaypoint(waypointField.text)
                        waypointPopup.close()
                    }
                }
            }
        }
    }

    Popup {
        id: nearestPopup
        modal: true
        focus: true
        anchors.centerIn: Overlay.overlay
        width: compact ? Math.min(root.width - 32, 520) : Math.min(root.width - 48, 620)
        height: compact ? 320 : 380
        padding: 0

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
                text: "Nearest Airports"
                color: "#f8fbff"
                font.pixelSize: 20
                font.bold: true
            }

            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 6
                clip: true
                model: root.currentNearestAirports()

                delegate: Rectangle {
                    required property var modelData

                    width: ListView.view.width
                    height: 56
                    radius: 8
                    color: "#0d141d"
                    border.color: "#355069"
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        spacing: 10

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            Text {
                                text: modelData.ident
                                color: "#56d6ff"
                                font.pixelSize: 16
                                font.bold: true
                            }

                            Text {
                                text: modelData.name
                                color: "#c0d0dd"
                                font.pixelSize: 12
                                elide: Text.ElideRight
                            }
                        }

                        Text {
                            text: Number(modelData.distanceNm || 0).toFixed(1) + " NM"
                            color: "#f8fbff"
                            font.pixelSize: 13
                            font.bold: true
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            waypointField.text = modelData.ident
                            nearestPopup.close()
                        }
                    }
                }
            }

            Button {
                Layout.alignment: Qt.AlignRight
                text: "Close"
                onClicked: nearestPopup.close()
            }
        }
    }

    Popup {
        id: flightPlanPopup
        modal: true
        focus: true
        anchors.centerIn: Overlay.overlay
        width: compact ? Math.min(root.width - 24, 620) : Math.min(root.width - 36, 760)
        height: compact ? 480 : 560
        padding: 0

        background: Rectangle {
            radius: 12
            color: "#08111b"
            border.color: "#355069"
            border.width: 1
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 10

            Text {
                text: "Flight Plan"
                color: "#f8fbff"
                font.pixelSize: 22
                font.bold: true
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 30
                color: "#000000"
                border.color: "#355069"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10

                    Text { text: "Waypoint"; color: "#f8fbff"; Layout.fillWidth: true; font.bold: true }
                    Text { text: "DTK"; color: "#f8fbff"; Layout.preferredWidth: 70; font.bold: true; horizontalAlignment: Text.AlignHCenter }
                    Text { text: "DIS"; color: "#f8fbff"; Layout.preferredWidth: 90; font.bold: true; horizontalAlignment: Text.AlignHCenter }
                    Text { text: "CUM"; color: "#f8fbff"; Layout.preferredWidth: 90; font.bold: true; horizontalAlignment: Text.AlignHCenter }
                }
            }

            ListView {
                Layout.fillWidth: true
                Layout.preferredHeight: compact ? 180 : 220
                spacing: 8
                clip: true
                model: root.flightPlanLegs

                delegate: Rectangle {
                    required property int index
                    required property var modelData

                    width: ListView.view.width
                    height: 62
                    radius: 8
                    color: "#05080d"
                    border.color: "#4b5f77"
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10

                        Column {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignVCenter

                            Text {
                                text: modelData.ident
                                color: index === 0 ? "#ff33ef" : "#35d7ff"
                                font.pixelSize: 18
                                font.bold: true
                            }

                            Text {
                                text: modelData.name
                                color: "#b8c9d8"
                                font.pixelSize: 11
                            }
                        }

                        Text {
                            Layout.preferredWidth: 70
                            text: ("000" + Number(modelData.dtkDeg).toFixed(0)).slice(-3) + "°"
                            color: "#f8fbff"
                            font.pixelSize: 16
                            horizontalAlignment: Text.AlignHCenter
                        }

                        Text {
                            Layout.preferredWidth: 90
                            text: Number(modelData.distanceNm).toFixed(1) + " NM"
                            color: "#ff33ef"
                            font.pixelSize: 16
                            horizontalAlignment: Text.AlignHCenter
                        }

                        Text {
                            Layout.preferredWidth: 90
                            text: Number(modelData.cumulativeNm).toFixed(1) + " NM"
                            color: "#f8fbff"
                            font.pixelSize: 16
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: root.openFlightPlanWaypointActions(index)
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                TextField {
                    id: flightPlanField
                    Layout.fillWidth: true
                    placeholderText: "Add waypoint ident"
                }

                Button {
                    text: "Add"
                    onClicked: {
                        const currentIds = root.currentFlightPlanIds()
                        if (flightPlanField.text.trim().length > 0) {
                            currentIds.push(flightPlanField.text.trim().toUpperCase())
                            appController.setFlightPlanWaypoints(currentIds)
                            flightPlanField.text = ""
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#04070c"
                border.color: "#355069"
                border.width: 1
                radius: 8

                ListView {
                    anchors.fill: parent
                    anchors.margins: 8
                    clip: true
                    spacing: 6
                    model: root.searchResults(flightPlanField.text, 8)

                    delegate: Rectangle {
                        required property var modelData

                        width: ListView.view.width
                        height: 46
                        radius: 6
                        color: "#0d141d"
                        border.color: "#355069"
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10

                            Text {
                                text: modelData.ident
                                color: "#56d6ff"
                                font.pixelSize: 16
                                font.bold: true
                            }

                            Text {
                                Layout.fillWidth: true
                                text: modelData.name
                                color: "#c0d0dd"
                                font.pixelSize: 12
                                elide: Text.ElideRight
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: flightPlanField.text = modelData.ident
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                TextField {
                    id: flightPlanNameField
                    Layout.fillWidth: true
                    placeholderText: "Stored flight plan name"
                }

                Button {
                    text: "Save"
                    onClicked: appController.saveCurrentFlightPlan(flightPlanNameField.text)
                }

                Button {
                    text: "Reverse"
                    enabled: root.flightPlanLegs.length > 1
                    onClicked: root.reverseFlightPlan()
                }

                Button {
                    text: "Clear"
                    onClicked: appController.clearFlightPlan()
                }

                Button {
                    text: "Close"
                    onClicked: flightPlanPopup.close()
                }
            }
        }
    }

    Popup {
        id: flightPlanActionPopup
        modal: true
        focus: true
        anchors.centerIn: Overlay.overlay
        width: compact ? 280 : 320
        height: compact ? 220 : 248
        padding: 0

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
                text: selectedFlightPlanIndex >= 0 && selectedFlightPlanIndex < root.flightPlanLegs.length
                    ? String(root.flightPlanLegs[selectedFlightPlanIndex].ident || "")
                    : "Flight Plan Waypoint"
                color: "#f8fbff"
                font.pixelSize: 20
                font.bold: true
            }

            Button {
                Layout.fillWidth: true
                text: "Move Up"
                enabled: selectedFlightPlanIndex > 0
                onClicked: root.moveFlightPlanWaypoint(selectedFlightPlanIndex, selectedFlightPlanIndex - 1)
            }

            Button {
                Layout.fillWidth: true
                text: "Move Down"
                enabled: selectedFlightPlanIndex >= 0 && selectedFlightPlanIndex < root.flightPlanLegs.length - 1
                onClicked: root.moveFlightPlanWaypoint(selectedFlightPlanIndex, selectedFlightPlanIndex + 1)
            }

            Button {
                Layout.fillWidth: true
                text: "Delete"
                enabled: selectedFlightPlanIndex >= 0
                onClicked: {
                    root.deleteFlightPlanWaypoint(selectedFlightPlanIndex)
                    flightPlanActionPopup.close()
                }
            }

            Button {
                Layout.alignment: Qt.AlignRight
                text: "Close"
                onClicked: flightPlanActionPopup.close()
            }
        }
    }

    Popup {
        id: storedPlansPopup
        modal: true
        focus: true
        anchors.centerIn: Overlay.overlay
        width: compact ? Math.min(root.width - 24, 620) : Math.min(root.width - 40, 780)
        height: compact ? 420 : 500
        padding: 0

        background: Rectangle {
            radius: 12
            color: "#08111b"
            border.color: "#355069"
            border.width: 1
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 10

            Text {
                text: "Stored Flight Plans"
                color: "#f8fbff"
                font.pixelSize: 22
                font.bold: true
            }

            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 8
                clip: true
                model: navDataManager.storedFlightPlans

                delegate: Rectangle {
                    required property var modelData

                    width: ListView.view.width
                    height: 68
                    radius: 8
                    color: root.selectedStoredPlanId === modelData.id ? "#12324d" : "#05080d"
                    border.color: root.selectedStoredPlanId === modelData.id ? "#56d6ff" : "#4b5f77"
                    border.width: 1

                    Column {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        spacing: 4

                        Text {
                            text: modelData.name
                            color: "#f8fbff"
                            font.pixelSize: 18
                            font.bold: true
                        }

                        Text {
                            text: String((modelData.waypoints || []).join(" → ")) + "  |  " + modelData.createdAt
                            color: "#c0d0dd"
                            font.pixelSize: 12
                            elide: Text.ElideRight
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: root.selectedStoredPlanId = modelData.id
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Button {
                    Layout.fillWidth: true
                    text: "Open Editor"
                    onClicked: {
                        storedPlansPopup.close()
                        flightPlanPopup.open()
                    }
                }

                Button {
                    Layout.fillWidth: true
                    text: "Load"
                    enabled: root.selectedStoredPlanId !== ""
                    onClicked: {
                        appController.loadStoredFlightPlan(root.selectedStoredPlanId)
                        storedPlansPopup.close()
                        flightPlanPopup.open()
                    }
                }

                Button {
                    Layout.fillWidth: true
                    text: "Delete"
                    enabled: root.selectedStoredPlanId !== ""
                    onClicked: {
                        navDataManager.deleteStoredFlightPlan(root.selectedStoredPlanId)
                        root.selectedStoredPlanId = ""
                    }
                }

                Button {
                    Layout.fillWidth: true
                    text: "Close"
                    onClicked: storedPlansPopup.close()
                }
            }
        }
    }

    Popup {
        id: userWaypointsPopup
        modal: true
        focus: true
        anchors.centerIn: Overlay.overlay
        width: compact ? Math.min(root.width - 24, 640) : Math.min(root.width - 40, 840)
        height: compact ? 460 : 520
        padding: 0

        background: Rectangle {
            radius: 12
            color: "#08111b"
            border.color: "#355069"
            border.width: 1
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 10

            Text {
                text: "User Waypoints"
                color: "#f8fbff"
                font.pixelSize: 22
                font.bold: true
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 12

                ListView {
                    id: userWaypointsList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: compact ? 240 : 300
                    spacing: 6
                    clip: true
                    model: navDataManager.userWaypoints

                    delegate: Rectangle {
                        required property var modelData

                        width: ListView.view.width
                        height: 56
                        radius: 8
                        color: root.selectedUserWaypointIdent === modelData.ident ? "#12324d" : "#05080d"
                        border.color: root.selectedUserWaypointIdent === modelData.ident ? "#56d6ff" : "#4b5f77"
                        border.width: 1

                        Column {
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.leftMargin: 10
                            spacing: 2

                            Text {
                                text: modelData.ident
                                color: "#56d6ff"
                                font.pixelSize: 16
                                font.bold: true
                            }

                            Text {
                                text: modelData.name
                                color: "#c0d0dd"
                                font.pixelSize: 12
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                root.selectedUserWaypointIdent = modelData.ident
                                userIdentField.text = modelData.ident
                                userNameField.text = modelData.name
                                userLatField.text = Number(modelData.latitudeDeg).toFixed(6)
                                userLonField.text = Number(modelData.longitudeDeg).toFixed(6)
                                userNotesField.text = modelData.notes || ""
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "#04070c"
                    border.color: "#355069"
                    border.width: 1
                    radius: 8

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 8

                        TextField { id: userIdentField; Layout.fillWidth: true; placeholderText: "IDENT" }
                        TextField { id: userNameField; Layout.fillWidth: true; placeholderText: "Name" }
                        TextField { id: userLatField; Layout.fillWidth: true; placeholderText: "Latitude" }
                        TextField { id: userLonField; Layout.fillWidth: true; placeholderText: "Longitude" }
                        TextField { id: userNotesField; Layout.fillWidth: true; placeholderText: "Notes" }

                        Item { Layout.fillHeight: true }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Button {
                                Layout.fillWidth: true
                                text: "New"
                                onClicked: {
                                    root.selectedUserWaypointIdent = ""
                                    userIdentField.text = ""
                                    userNameField.text = ""
                                    userLatField.text = ""
                                    userLonField.text = ""
                                    userNotesField.text = ""
                                }
                            }

                            Button {
                                Layout.fillWidth: true
                                text: "Save"
                                onClicked: {
                                    navDataManager.addOrUpdateUserWaypoint(
                                        userIdentField.text,
                                        userNameField.text,
                                        Number(userLatField.text),
                                        Number(userLonField.text),
                                        userNotesField.text)
                                    root.selectedUserWaypointIdent = userIdentField.text.trim().toUpperCase()
                                }
                            }

                            Button {
                                Layout.fillWidth: true
                                text: "Delete"
                                enabled: userIdentField.text.trim().length > 0
                                onClicked: navDataManager.removeUserWaypoint(userIdentField.text)
                            }

                            Button {
                                Layout.fillWidth: true
                                text: "Direct-To"
                                enabled: userIdentField.text.trim().length > 0
                                onClicked: {
                                    appController.setDirectWaypoint(userIdentField.text)
                                    userWaypointsPopup.close()
                                }
                            }
                        }
                    }
                }
            }

            Button {
                Layout.alignment: Qt.AlignRight
                text: "Close"
                onClicked: userWaypointsPopup.close()
            }
        }
    }

    Popup {
        id: recordsPopup
        modal: true
        focus: true
        anchors.centerIn: Overlay.overlay
        width: compact ? Math.min(root.width - 24, 600) : Math.min(root.width - 40, 760)
        height: compact ? 380 : 460
        padding: 0

        readonly property var activeModel: recordsMode === "flight_logs"
            ? navDataManager.flightLogs
            : navDataManager.trackLogs
        readonly property string titleText: recordsMode === "flight_logs" ? "Flight Logs" : "Track Logs"

        background: Rectangle {
            radius: 12
            color: "#08111b"
            border.color: "#355069"
            border.width: 1
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 10

            Text {
                text: recordsPopup.titleText
                color: "#f8fbff"
                font.pixelSize: 22
                font.bold: true
            }

            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 8
                clip: true
                model: recordsPopup.activeModel

                delegate: Rectangle {
                    required property var modelData

                    width: ListView.view.width
                    height: 68
                    radius: 8
                    color: "#05080d"
                    border.color: "#4b5f77"
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        spacing: 10

                        ColumnLayout {
                            Layout.fillWidth: true

                            Text {
                                text: modelData.name
                                color: "#f8fbff"
                                font.pixelSize: 17
                                font.bold: true
                            }

                            Text {
                                text: recordsMode === "flight_logs"
                                    ? (modelData.startedAt + "  |  " + modelData.duration + "  |  " + modelData.route)
                                    : (modelData.recordedAt + "  |  " + modelData.duration + "  |  " + modelData.distance)
                                color: "#c0d0dd"
                                font.pixelSize: 12
                                elide: Text.ElideRight
                            }
                        }

                        Button {
                            text: "Delete"
                            onClicked: {
                                if (recordsMode === "flight_logs") {
                                    navDataManager.deleteFlightLog(modelData.id)
                                } else {
                                    navDataManager.deleteTrackLog(modelData.id)
                                }
                            }
                        }
                    }
                }
            }

            Button {
                Layout.alignment: Qt.AlignRight
                text: "Close"
                onClicked: recordsPopup.close()
            }
        }
    }

    Popup {
        id: statusPopup
        modal: true
        focus: true
        anchors.centerIn: Overlay.overlay
        width: compact ? Math.min(root.width - 24, 620) : Math.min(root.width - 40, 840)
        height: compact ? 420 : 520
        padding: 0

        background: Rectangle {
            radius: 12
            color: "#08111b"
            border.color: "#355069"
            border.width: 1
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 10

            Text {
                text: "System Status"
                color: "#f8fbff"
                font.pixelSize: 22
                font.bold: true
            }

            Text {
                Layout.fillWidth: true
                text: chartManager.chartStatus
                color: "#c0d0dd"
                wrapMode: Text.WordWrap
                font.pixelSize: 13
            }

            Text {
                Layout.fillWidth: true
                text: typeof weatherDataManager !== "undefined"
                    ? weatherDataManager.radarOverlayStatus
                        + "  |  FIS-B msgs: "
                        + weatherDataManager.fisbMessages.length
                    : "Weather source unavailable"
                color: "#95aec2"
                wrapMode: Text.WordWrap
                font.pixelSize: 12
            }

            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 8
                clip: true
                model: appController.moduleModel

                delegate: Rectangle {
                    required property string name
                    required property bool present
                    required property bool healthy
                    required property string details

                    width: ListView.view.width
                    height: 64
                    radius: 8
                    color: "#05080d"
                    border.color: healthy && present ? "#2d8d5a" : present ? "#f59e0b" : "#355069"
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12

                        ColumnLayout {
                            Layout.fillWidth: true

                            Text {
                                text: name
                                color: "#f8fbff"
                                font.pixelSize: 16
                                font.bold: true
                            }

                            Text {
                                text: details
                                color: "#c0d0dd"
                                font.pixelSize: 12
                            }
                        }

                        Text {
                            text: present ? (healthy ? "ONLINE" : "DEGRADED") : "OFFLINE"
                            color: healthy && present ? "#7ef0a6" : present ? "#f7cb67" : "#8da4ba"
                            font.pixelSize: 12
                            font.bold: true
                        }
                    }
                }
            }

            Button {
                Layout.alignment: Qt.AlignRight
                text: "Close"
                onClicked: statusPopup.close()
            }
        }
    }

    Popup {
        id: autopilotPopup
        modal: true
        focus: true
        anchors.centerIn: Overlay.overlay
        width: compact ? 360 : 430
        height: compact ? 350 : 420
        padding: 0

        background: Rectangle {
            radius: 12
            color: "#08111b"
            border.color: "#355069"
            border.width: 1
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 12

            Text {
                text: "Automatic Flight Control"
                color: "#f8fbff"
                font.pixelSize: 20
                font.bold: true
            }

            GridLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                columns: 4
                columnSpacing: 10
                rowSpacing: 10

                Repeater {
                    model: [
                        { label: "AP", active: appController.autopilotMaster, action: function() { appController.autopilotMaster = !appController.autopilotMaster } },
                        { label: "FD", active: appController.flightDirectorEnabled, action: function() { appController.flightDirectorEnabled = !appController.flightDirectorEnabled } },
                        { label: "LVL", active: appController.levelModeEnabled, action: function() { appController.levelModeEnabled = !appController.levelModeEnabled } },
                        { label: "HDG", active: appController.autopilotLateralMode === "HDG", action: function() { appController.autopilotLateralMode = "HDG" } },
                        { label: "TRK", active: appController.autopilotLateralMode === "TRK", action: function() { appController.autopilotLateralMode = "TRK" } },
                        { label: "NAV", active: appController.autopilotLateralMode === "NAV", action: function() { appController.autopilotLateralMode = "NAV" } },
                        { label: "APR", active: appController.autopilotLateralMode === "APR", action: function() { appController.autopilotLateralMode = "APR" } },
                        { label: "ALT", active: appController.autopilotVerticalMode === "ALT", action: function() { appController.autopilotVerticalMode = "ALT" } },
                        { label: "VS", active: appController.autopilotVerticalMode === "VS", action: function() { appController.autopilotVerticalMode = "VS" } },
                        { label: "VNAV", active: appController.autopilotVerticalMode === "VNAV", action: function() { appController.autopilotVerticalMode = "VNAV" } }
                    ]

                    delegate: Button {
                        id: apModeButton
                        required property var modelData
                        Layout.fillWidth: true
                        Layout.preferredHeight: 54
                        text: modelData.label

                        background: Rectangle {
                            radius: 8
                            color: modelData.active ? "#143445" : "#10171f"
                            border.color: modelData.active ? "#56d6ff" : "#355069"
                            border.width: 1
                        }

                        contentItem: Text {
                            text: apModeButton.text
                            color: "#f8fbff"
                            font.pixelSize: 15
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        onClicked: modelData.action()
                    }
                }
            }

            Button {
                Layout.alignment: Qt.AlignRight
                text: "Close"
                onClicked: autopilotPopup.close()
            }
        }
    }

    Popup {
        id: infoPopup
        modal: true
        focus: true
        anchors.centerIn: Overlay.overlay
        width: compact ? 300 : 360
        height: compact ? 170 : 200
        padding: 0

        property string titleText: ""
        property string bodyText: ""

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
                text: infoPopup.titleText
                color: "#f8fbff"
                font.pixelSize: 20
                font.bold: true
            }

            Text {
                Layout.fillWidth: true
                text: infoPopup.bodyText
                color: "#c0d0dd"
                wrapMode: Text.WordWrap
                font.pixelSize: 14
            }

            Item { Layout.fillHeight: true }

            Button {
                Layout.alignment: Qt.AlignRight
                text: "Close"
                onClicked: infoPopup.close()
            }
        }
    }

    Popup {
        id: menuPopup
        modal: true
        focus: true
        anchors.centerIn: Overlay.overlay
        width: compact ? 620 : 760
        height: compact ? 460 : 520
        padding: 0

        background: Rectangle {
            color: "#1d2230"
            border.color: "#596a7e"
            border.width: 1
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 14

            Text {
                text: "Menu"
                color: "#f8fbff"
                font.pixelSize: 22
                font.bold: true
            }

            Text {
                visible: !appController.setupAccessAllowed
                text: "Setup is locked while the engine is running."
                color: "#f5c66e"
                font.pixelSize: 13
            }

            GridLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                columns: compact ? 3 : 4
                columnSpacing: 14
                rowSpacing: 14

                Repeater {
                    model: [
                        { key: "flight_log", title: "Flight Log", iconSource: "qrc:/ExperimentalMfd/qml/assets/icons/flight_log.svg", color: "#7fe39e" },
                        { key: "flight_plan", title: "Flight Plan List", iconSource: "qrc:/ExperimentalMfd/qml/assets/icons/flight_plan.svg", color: "#56d6ff" },
                        { key: "vnav", title: "VNAV", iconSource: "qrc:/ExperimentalMfd/qml/assets/icons/vnav.svg", color: "#7cb2ff" },
                        { key: "track_log", title: "Track Log", iconSource: "qrc:/ExperimentalMfd/qml/assets/icons/track_log.svg", color: "#56d6ff" },
                        { key: "user_timer", title: "User Timer", iconSource: "qrc:/ExperimentalMfd/qml/assets/icons/timer.svg", color: "#f5c66e" },
                        { key: "user_waypoints", title: "User Waypoints", iconSource: "qrc:/ExperimentalMfd/qml/assets/icons/waypoint_flag.svg", color: "#35d7ff" },
                        { key: "tools", title: "Tools", iconSource: "qrc:/ExperimentalMfd/qml/assets/icons/tools.svg", color: "#d7dde4" },
                        { key: "setup", title: "Setup", iconSource: "qrc:/ExperimentalMfd/qml/assets/icons/setup.svg", color: "#7cb2ff", enabledTile: appController.setupAccessAllowed },
                        { key: "status", title: "Status", iconSource: "qrc:/ExperimentalMfd/qml/assets/icons/status.svg", color: "#9de29b" }
                    ]

                    delegate: MenuTile {
                        required property var modelData

                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        title: modelData.title
                        iconSource: modelData.iconSource
                        iconColor: modelData.color
                        enabledTile: modelData.enabledTile === undefined ? true : modelData.enabledTile

                        onClicked: {
                            if (modelData.key === "flight_plan") {
                                menuPopup.close()
                                storedPlansPopup.open()
                            } else if (modelData.key === "user_waypoints") {
                                menuPopup.close()
                                userWaypointsPopup.open()
                            } else if (modelData.key === "flight_log") {
                                menuPopup.close()
                                root.openRecords("flight_logs")
                            } else if (modelData.key === "track_log") {
                                menuPopup.close()
                                root.openRecords("track_logs")
                            } else if (modelData.key === "setup") {
                                menuPopup.close()
                                appController.currentPage = "Setup"
                            } else if (modelData.key === "status") {
                                menuPopup.close()
                                statusPopup.open()
                            } else {
                                infoPopup.titleText = modelData.title
                                infoPopup.bodyText = modelData.title + " is scaffolded for the next implementation pass."
                                menuPopup.close()
                                infoPopup.open()
                            }
                        }
                    }
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.NoButton
        hoverEnabled: true

        onWheel: event => {
            root.setInteractiveZoom(root.mapZoomFactor + (event.angleDelta.y > 0 ? 0.2 : -0.2))
            event.accepted = true
        }
    }
}
