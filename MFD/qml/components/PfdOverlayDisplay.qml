import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

    property real pitchDeg: 0
    property real rollDeg: 0
    property real slipSkidNormalized: 0
    property real iasKt: 0
    property real airspeedTrendKt: 0
    property real tasKt: 0
    property real aoaNormalized: 0
    property real altitudeFt: 0
    property real verticalSpeedFpm: 0
    property real headingDeg: 0
    property real courseBearingDeg: 0
    property real groundTrackDeg: 0
    property real groundSpeedKt: 0
    property real baroSettingInHg: 29.92
    property real selectedAltitudeFt: 0
    property bool trafficActive: false
    property string navSource: "GPS"
    property string courseLabel: "CRS 043"
    property string distanceLabel: "5.0NM"
    property string apModeLabel: "ON"
    property string gpsModeLabel: "NAV"
    property bool compact: false

    property string activeEntryMode: ""

    readonly property real airspeedRounded: Math.round(iasKt / 10) * 10
    readonly property real altitudeRounded: Math.round(altitudeFt / 100) * 100
    readonly property real airspeedOffsetRatio: (iasKt - airspeedRounded) / 10.0
    readonly property real altitudeOffsetRatio: (altitudeFt - altitudeRounded) / 100.0
    readonly property real selectedAltitudeDisplay: selectedAltitudeFt > 0 ? selectedAltitudeFt : Math.round(altitudeFt / 100.0) * 100.0
    readonly property real tapeShadowTop: root.height * 0.06
    readonly property real tapeShadowHeight: root.height * 0.76
    readonly property real tapeLabelSize: compact ? 16 : 18
    readonly property real airspeedSpacing: root.height * 0.105
    readonly property real altitudeSpacing: root.height * 0.105
    readonly property real centerAirspeedY: root.height * 0.5
    readonly property real centerAltitudeY: root.height * 0.5
    readonly property real airspeedPixelsPerKt: airspeedSpacing / 10.0
    readonly property real altitudePixelsPerFt: altitudeSpacing / 100.0
    readonly property real predictedAirspeedY: centerAirspeedY - airspeedTrendKt * airspeedPixelsPerKt
    readonly property real predictedAltitudeY: centerAltitudeY - (verticalSpeedFpm / 1000.0) * altitudeSpacing
    readonly property real vsoKt: 49
    readonly property real vs1Kt: 55
    readonly property real vfeKt: 85
    readonly property real vnoKt: 129
    readonly property real vneKt: 150

    function clampValue(value, minimum, maximum) {
        return Math.max(minimum, Math.min(maximum, value))
    }

    function airspeedYForValue(valueKt) {
        return centerAirspeedY + (iasKt - valueKt) * airspeedPixelsPerKt
    }

    function altitudeYForValue(valueFt) {
        return centerAltitudeY + (altitudeFt - valueFt) * altitudePixelsPerFt
    }

    function openBaroKeypad() {
        activeEntryMode = "baro"
        keypad.openForValue("BARO SET", Number(baroSettingInHg).toFixed(2), true, "IN HG")
    }

    function openAltitudeKeypad() {
        activeEntryMode = "altitude"
        keypad.openForValue("SELECT ALT", Number(selectedAltitudeDisplay).toFixed(0), false, "FT")
    }

    radius: 14
    color: "#05090f"
    border.color: "#223648"
    border.width: 1
    clip: true

    Item {
        id: world
        width: root.width * 3
        height: root.height * 3
        anchors.centerIn: parent
        rotation: -root.rollDeg
        y: root.pitchDeg * (root.height / 32)

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: parent.height / 2
            color: "#3658d8"
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: parent.height / 2
            color: "#6a6a2d"
        }

        Rectangle {
            width: parent.width
            height: 3
            anchors.centerIn: parent
            color: "#d7d9df"
        }

        Repeater {
            model: [-20, -15, -10, -5, 5, 10, 15, 20]

            delegate: Item {
                required property var modelData

                width: world.width
                height: 24
                y: world.height / 2 - modelData * (root.height / 34) - height / 2

                Rectangle {
                    width: Math.abs(modelData) % 10 === 0 ? world.width * 0.045 : world.width * 0.026
                    height: 2
                    color: "#f3f7fb"
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    visible: Math.abs(modelData) % 10 === 0
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.horizontalCenterOffset: -world.width * 0.040
                    text: Number(Math.abs(modelData)).toFixed(0)
                    color: "#f3f7fb"
                    font.pixelSize: compact ? 12 : 13
                    font.family: "Menlo"
                }

                Text {
                    visible: Math.abs(modelData) % 10 === 0
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.horizontalCenterOffset: world.width * 0.040
                    text: Number(Math.abs(modelData)).toFixed(0)
                    color: "#f3f7fb"
                    font.pixelSize: compact ? 12 : 13
                    font.family: "Menlo"
                }
            }
        }
    }

    Canvas {
        id: overlayCanvas
        anchors.fill: parent

        onPaint: {
            const ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)

            const cx = width / 2
            const cy = height * 0.47
            const bankRadius = Math.min(width, height) * 0.24

            ctx.strokeStyle = "#f2f5f8"
            ctx.lineWidth = 3
            ctx.beginPath()
            ctx.arc(cx, cy - height * 0.08, bankRadius, Math.PI * 0.15, Math.PI * 0.85)
            ctx.stroke()

            const bankTicks = [-60, -45, -30, -20, -10, 10, 20, 30, 45, 60]
            for (let i = 0; i < bankTicks.length; ++i) {
                const angle = (bankTicks[i] - 90) * Math.PI / 180.0
                const inner = bankTicks[i] % 30 === 0 ? bankRadius - 14 : bankRadius - 8
                const outer = bankRadius + 2
                const centerY = cy - height * 0.08
                const x1 = cx + Math.cos(angle) * inner
                const y1 = centerY + Math.sin(angle) * inner
                const x2 = cx + Math.cos(angle) * outer
                const y2 = centerY + Math.sin(angle) * outer
                ctx.beginPath()
                ctx.moveTo(x1, y1)
                ctx.lineTo(x2, y2)
                ctx.stroke()
            }

            ctx.fillStyle = "#f8fbff"
            ctx.beginPath()
            ctx.moveTo(cx, cy - height * 0.08 - bankRadius - 10)
            ctx.lineTo(cx - 8, cy - height * 0.08 - bankRadius + 4)
            ctx.lineTo(cx + 8, cy - height * 0.08 - bankRadius + 4)
            ctx.closePath()
            ctx.fill()

            const rollAngle = (root.rollDeg - 90) * Math.PI / 180.0
            const rollPointerX = cx + Math.cos(rollAngle) * bankRadius
            const rollPointerY = cy - height * 0.08 + Math.sin(rollAngle) * bankRadius
            ctx.beginPath()
            ctx.moveTo(rollPointerX, rollPointerY)
            ctx.lineTo(rollPointerX - Math.sin(rollAngle) * 6 - Math.cos(rollAngle) * 9,
                       rollPointerY + Math.cos(rollAngle) * 6 - Math.sin(rollAngle) * 9)
            ctx.lineTo(rollPointerX + Math.sin(rollAngle) * 6 - Math.cos(rollAngle) * 9,
                       rollPointerY - Math.cos(rollAngle) * 6 - Math.sin(rollAngle) * 9)
            ctx.closePath()
            ctx.fill()

            ctx.fillStyle = "#ffe11f"
            ctx.beginPath()
            ctx.arc(cx - width * 0.16, cy - height * 0.02, 12, 0, Math.PI * 2)
            ctx.fill()

            ctx.fillStyle = "#ff2edc"
            ctx.beginPath()
            ctx.moveTo(cx - width * 0.13, cy + 8)
            ctx.lineTo(cx - width * 0.03, cy + 22)
            ctx.lineTo(cx - width * 0.01, cy + 10)
            ctx.lineTo(cx - width * 0.09, cy + 2)
            ctx.closePath()
            ctx.fill()

            ctx.fillStyle = "#ff2edc"
            ctx.beginPath()
            ctx.moveTo(cx + width * 0.13, cy + 8)
            ctx.lineTo(cx + width * 0.03, cy + 22)
            ctx.lineTo(cx + width * 0.01, cy + 10)
            ctx.lineTo(cx + width * 0.09, cy + 2)
            ctx.closePath()
            ctx.fill()

            ctx.fillStyle = "#ffe11f"
            ctx.beginPath()
            ctx.moveTo(cx - width * 0.07, cy + 10)
            ctx.lineTo(cx - width * 0.01, cy + 17)
            ctx.lineTo(cx + width * 0.01, cy + 10)
            ctx.lineTo(cx - width * 0.04, cy + 5)
            ctx.closePath()
            ctx.fill()

            ctx.fillStyle = "#ffe11f"
            ctx.beginPath()
            ctx.moveTo(cx + width * 0.07, cy + 10)
            ctx.lineTo(cx + width * 0.01, cy + 17)
            ctx.lineTo(cx - width * 0.01, cy + 10)
            ctx.lineTo(cx + width * 0.04, cy + 5)
            ctx.closePath()
            ctx.fill()

            ctx.strokeStyle = "#21c95b"
            ctx.lineWidth = 4
            ctx.beginPath()
            ctx.moveTo(cx - width * 0.07, cy + 14)
            ctx.lineTo(cx + width * 0.07, cy + 14)
            ctx.stroke()

            ctx.strokeStyle = "#f8fbff"
            ctx.lineWidth = 5
            ctx.beginPath()
            ctx.moveTo(cx - width * 0.09, cy + 14)
            ctx.lineTo(cx - width * 0.025, cy + 14)
            ctx.moveTo(cx + width * 0.025, cy + 14)
            ctx.lineTo(cx + width * 0.09, cy + 14)
            ctx.stroke()

            ctx.lineWidth = 4
            ctx.beginPath()
            ctx.arc(cx, cy + 14, width * 0.020, 0, Math.PI * 2)
            ctx.stroke()
        }
    }

    Item {
        id: topModes
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 6
        width: Math.min(parent.width - 170, compact ? 260 : 320)
        height: compact ? 22 : 24

        Row {
            anchors.centerIn: parent
            spacing: 4

            Repeater {
                model: [
                    { label: "AP", value: root.apModeLabel, active: root.apModeLabel !== "OFF", color: "#51d2ff", editable: false },
                    { label: "GPS", value: root.gpsModeLabel, active: root.gpsModeLabel.length > 0, color: "#74ff61", editable: false },
                    { label: "TRAFFIC", value: "", active: root.trafficActive, color: root.trafficActive ? "#39d353" : "#ef4444", editable: false }
                ]

                delegate: Rectangle {
                    required property var modelData

                    width: compact ? 62 : 74
                    height: compact ? 18 : 20
                    radius: 3
                    color: modelData.label === "TRAFFIC"
                        ? (modelData.active ? "#143c1c" : "#3a1013")
                        : (modelData.active ? "#1f3e84" : "#2b2f38")
                    border.color: modelData.active ? modelData.color : "#5a6572"
                    border.width: 1

                    Row {
                        anchors.centerIn: parent
                        spacing: 3

                        Text {
                            text: modelData.label
                            color: modelData.color
                            font.pixelSize: compact ? 10 : 11
                            font.bold: true
                        }

                        Text {
                            visible: modelData.value.length > 0
                            text: modelData.value
                            color: "#f8fbff"
                            font.pixelSize: compact ? 10 : 11
                            font.bold: true
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        enabled: modelData.editable
                        onClicked: root.openAltitudeKeypad()
                    }
                }
            }
        }
    }

    Item {
        id: airspeedTape
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: compact ? 88 : 96

        Rectangle {
            x: 0
            y: root.tapeShadowTop
            width: compact ? 74 : 82
            height: root.tapeShadowHeight
            radius: 3
            color: "#69485159"
            border.color: "#7a8a99"
            border.width: 1
        }

        Rectangle {
            x: 6
            y: root.airspeedYForValue(root.vnoKt)
            width: 10
            height: Math.max(4, root.airspeedYForValue(root.vs1Kt) - root.airspeedYForValue(root.vnoKt))
            color: "#21d43d"
        }

        Rectangle {
            x: 6
            y: root.airspeedYForValue(root.vneKt)
            width: 10
            height: Math.max(5, root.airspeedYForValue(root.vnoKt) - root.airspeedYForValue(root.vneKt))
            color: "#ef233c"
        }

        Rectangle {
            x: 6
            y: root.airspeedYForValue(root.vfeKt)
            width: 10
            height: Math.max(5, root.airspeedYForValue(root.vsoKt) - root.airspeedYForValue(root.vfeKt))
            color: "#f3e432"
        }

        Rectangle {
            x: 0
            y: root.airspeedYForValue(root.vsoKt)
            width: 5
            height: Math.max(10, root.airspeedYForValue(root.vfeKt) - root.airspeedYForValue(root.vsoKt))
            color: "#f04de3"
        }

        Rectangle {
            x: compact ? 66 : 72
            width: 5
            y: Math.min(root.centerAirspeedY, root.predictedAirspeedY)
            height: Math.max(3, Math.abs(root.predictedAirspeedY - root.centerAirspeedY))
            color: "#f04de3"
        }

        Rectangle {
            x: 4
            y: 12
            width: compact ? 54 : 60
            height: compact ? 20 : 22
            color: "#0a1020"
            border.color: "#5f6e83"
            border.width: 1

            Text {
                anchors.centerIn: parent
                text: "TAS " + Number(root.tasKt).toFixed(0) + "KT"
                color: "#f5f8fb"
                font.pixelSize: compact ? 10 : 11
                font.family: "Menlo"
                font.bold: true
            }
        }

        Repeater {
            model: 7

            delegate: Item {
                required property int index

                readonly property int offset: index - 3
                readonly property real tickValue: root.airspeedRounded - offset * 10
                y: root.airspeedYForValue(tickValue) - height / 2
                width: parent.width
                height: 28

                Rectangle {
                    x: compact ? 52 : 58
                    width: compact ? 10 : 12
                    height: 2
                    anchors.verticalCenter: parent.verticalCenter
                    color: "#d9e0e9"
                }

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 22
                    anchors.verticalCenter: parent.verticalCenter
                    text: Number(tickValue).toFixed(0)
                    color: "#f5f8fb"
                    font.pixelSize: root.tapeLabelSize
                    font.family: "Menlo"
                }
            }
        }

        Rectangle {
            width: compact ? 54 : 60
            height: compact ? 38 : 42
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 18
            color: "black"
            border.color: "#f5f8fb"
            border.width: 1

            Text {
                anchors.centerIn: parent
                text: Number(root.iasKt).toFixed(0)
                color: "#f5f8fb"
                font.pixelSize: root.tapeLabelSize
                font.family: "Menlo"
                font.bold: true
            }
        }

        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 12
            width: 10
            height: 14
            color: "#21d43d"

            transform: Rotation {
                origin.x: 5
                origin.y: 7
                angle: 45
            }
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 6
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 18
            text: "GS"
            color: "#f5f8fb"
            font.pixelSize: compact ? 10 : 11
            font.family: "Menlo"
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 24
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 12
            text: Number(root.groundSpeedKt).toFixed(0) + "KT"
            color: "#f04de3"
            font.pixelSize: compact ? 11 : 12
            font.family: "Menlo"
            font.bold: true
        }
    }

    Item {
        id: aoaIndicator
        x: compact ? 82 : 92
        y: root.height * 0.57
        width: compact ? 28 : 30
        height: compact ? 96 : 112

        readonly property int activeSegments: Math.round(root.clampValue(root.aoaNormalized, 0, 1) * 10)

        Rectangle {
            anchors.fill: parent
            color: "transparent"
            border.color: "#ffe326"
            border.width: 1
        }

        Repeater {
            model: 10

            delegate: Rectangle {
                required property int index

                readonly property bool active: index < aoaIndicator.activeSegments
                readonly property bool redZone: index >= 7
                readonly property bool yellowZone: index >= 4 && index < 7
                readonly property color segmentColor: redZone ? "#ef233c" : yellowZone ? "#f6df37" : "#2dea4f"

                x: yellowZone || redZone ? 3 : 4
                y: parent.height - 10 - (index * 9)
                width: yellowZone || redZone ? parent.width - 6 : parent.width - 8
                height: yellowZone || redZone ? 4 : 5
                color: active ? segmentColor : "#00000000"
                opacity: active ? 1.0 : 0.28
                rotation: yellowZone || redZone ? -26 : 0
            }
        }
    }

    Item {
        id: altitudeTape
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: compact ? 100 : 110

        Rectangle {
            x: compact ? 14 : 16
            y: root.tapeShadowTop
            width: compact ? 72 : 80
            height: root.tapeShadowHeight
            radius: 3
            color: "#69485159"
            border.color: "#7a8a99"
            border.width: 1
        }

        Rectangle {
            id: selectedAltitudeBox
            anchors.top: parent.top
            anchors.topMargin: 12
            anchors.right: parent.right
            anchors.rightMargin: 26
            width: compact ? 56 : 62
            height: compact ? 24 : 26
            color: "#163ca7"
            border.color: "#56d6ff"
            border.width: 1

            Text {
                anchors.centerIn: parent
                text: Number(root.selectedAltitudeDisplay).toFixed(0) + "FT"
                color: "#56d6ff"
                font.pixelSize: compact ? 11 : 12
                font.family: "Menlo"
                font.bold: true
            }

            MouseArea {
                anchors.fill: parent
                onClicked: root.openAltitudeKeypad()
            }
        }

        Rectangle {
            x: width - 25
            width: 5
            y: Math.min(root.centerAltitudeY, root.predictedAltitudeY)
            height: Math.max(3, Math.abs(root.predictedAltitudeY - root.centerAltitudeY))
            color: "#f8fbff"
        }

        Repeater {
            model: 7

            delegate: Item {
                required property int index

                readonly property int offset: index - 3
                readonly property real tickValue: root.altitudeRounded - offset * 100
                y: root.altitudeYForValue(tickValue) - height / 2
                width: parent.width
                height: 28

                Rectangle {
                    x: compact ? 22 : 26
                    width: compact ? 10 : 12
                    height: 2
                    anchors.verticalCenter: parent.verticalCenter
                    color: "#d9e0e9"
                }

                Text {
                    anchors.right: parent.right
                    anchors.rightMargin: 32
                    anchors.verticalCenter: parent.verticalCenter
                    text: Number(tickValue).toFixed(0)
                    color: "#f5f8fb"
                    font.pixelSize: root.tapeLabelSize
                    font.family: "Menlo"
                }
            }
        }

        Rectangle {
            width: compact ? 54 : 60
            height: compact ? 38 : 42
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: 22
            color: "black"
            border.color: "#f5f8fb"
            border.width: 1

            Text {
                anchors.centerIn: parent
                text: Number(root.altitudeFt).toFixed(0)
                color: "#f5f8fb"
                font.pixelSize: root.tapeLabelSize
                font.family: "Menlo"
                font.bold: true
            }
        }

        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: 18
            width: 10
            height: 14
            color: "#f5f8fb"

            transform: Rotation {
                origin.x: 5
                origin.y: 7
                angle: 45
            }
        }

        Item {
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: compact ? 24 : 28

            Repeater {
                model: 5

                delegate: Item {
                    required property int index

                    readonly property int label: 2 - index
                    y: root.height * 0.18 + index * (root.height * 0.14)
                    width: parent.width
                    height: 22

                    Text {
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        text: label === 0 ? ".5" : label.toString()
                        color: "#d7e0e9"
                        font.pixelSize: compact ? 10 : 11
                        font.family: "Menlo"
                    }
                }
            }

            Rectangle {
                anchors.left: parent.left
                anchors.leftMargin: compact ? 12 : 14
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.topMargin: root.height * 0.13
                anchors.bottomMargin: root.height * 0.16
                width: 2
                color: "#f5f8fb"
            }

            Rectangle {
                width: 14
                height: 6
                color: "#f5f8fb"
                anchors.left: parent.left
                anchors.leftMargin: compact ? 9 : 11
                y: Math.max(root.height * 0.16,
                            Math.min(root.height * 0.80,
                                     root.height * 0.47 - (root.verticalSpeedFpm / 2000.0) * (root.height * 0.28)))

                transform: Rotation {
                    origin.x: 2
                    origin.y: 3
                    angle: 25
                }
            }
        }

        Rectangle {
            id: baroBox
            anchors.right: parent.right
            anchors.rightMargin: 22
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 12
            width: compact ? 56 : 62
            height: compact ? 24 : 26
            color: "black"
            border.color: "#f5f8fb"
            border.width: 1

            Text {
                anchors.centerIn: parent
                text: Number(root.baroSettingInHg).toFixed(2) + "in"
                color: "#f5f8fb"
                font.pixelSize: compact ? 11 : 12
                font.family: "Menlo"
                font.bold: true
            }

            MouseArea {
                anchors.fill: parent
                onClicked: root.openBaroKeypad()
            }
        }
    }

    Item {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: root.height * 0.17
        width: compact ? 86 : 102
        height: compact ? 22 : 24

        Rectangle {
            anchors.fill: parent
            radius: height / 2
            color: "#3a4618"
            border.color: "#263214"
            border.width: 1
        }

        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 24
            width: 4
            height: 16
            radius: 2
            color: "#0b0f14"
        }

        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: 24
            width: 4
            height: 16
            radius: 2
            color: "#0b0f14"
        }

        Rectangle {
            width: compact ? 14 : 16
            height: width
            radius: width / 2
            color: "#ffffff"
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.horizontalCenterOffset: Math.max(-30, Math.min(30, root.slipSkidNormalized * 34))
        }
    }

    Item {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 4
        width: compact ? root.width * 0.46 : root.width * 0.48
        height: compact ? root.height * 0.28 : root.height * 0.30

        Canvas {
            id: hsiCanvas
            anchors.fill: parent

            onPaint: {
                const ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)

                const cx = width / 2
                const cy = height * 0.58
                const radius = Math.min(width, height) * 0.42

                ctx.strokeStyle = "#d8e4ef"
                ctx.lineWidth = 2
                ctx.beginPath()
                ctx.arc(cx, cy, radius, 0, Math.PI * 2)
                ctx.stroke()
                ctx.strokeStyle = "#72839a"
                ctx.lineWidth = 1
                ctx.beginPath()
                ctx.arc(cx, cy, radius * 0.56, 0, Math.PI * 2)
                ctx.stroke()

                for (let deg = 0; deg < 360; deg += 5) {
                    const rel = deg - root.headingDeg
                    const isMajor = deg % 30 === 0
                    const isMedium = deg % 10 === 0
                    const tickLength = isMajor ? 14 : (isMedium ? 9 : 5)
                    const angle = (rel - 90) * Math.PI / 180.0
                    const x1 = cx + Math.cos(angle) * (radius - tickLength)
                    const y1 = cy + Math.sin(angle) * (radius - tickLength)
                    const x2 = cx + Math.cos(angle) * radius
                    const y2 = cy + Math.sin(angle) * radius
                    ctx.strokeStyle = isMajor ? "#f5f8fb" : "#aebdca"
                    ctx.lineWidth = isMajor ? 2 : 1
                    ctx.beginPath()
                    ctx.moveTo(x1, y1)
                    ctx.lineTo(x2, y2)
                    ctx.stroke()
                }

                const labels = [
                    { deg: 0, text: "N" }, { deg: 30, text: "3" }, { deg: 60, text: "6" },
                    { deg: 90, text: "E" }, { deg: 120, text: "12" }, { deg: 150, text: "15" },
                    { deg: 180, text: "S" }, { deg: 210, text: "21" }, { deg: 240, text: "24" },
                    { deg: 270, text: "W" }, { deg: 300, text: "30" }, { deg: 330, text: "33" }
                ]

                ctx.font = "bold " + (root.compact ? "14px" : "17px") + " Arial"
                ctx.fillStyle = "#f5f8fb"
                ctx.textAlign = "center"
                ctx.textBaseline = "middle"
                for (let i = 0; i < labels.length; ++i) {
                    const rel = labels[i].deg - root.headingDeg
                    const angle = (rel - 90) * Math.PI / 180.0
                    const x = cx + Math.cos(angle) * (radius - 24)
                    const y = cy + Math.sin(angle) * (radius - 24)
                    ctx.fillText(labels[i].text, x, y)
                }

                const courseAngle = (root.courseBearingDeg - root.headingDeg - 90) * Math.PI / 180.0
                const courseCos = Math.cos(courseAngle)
                const courseSin = Math.sin(courseAngle)
                const courseStartX = cx - courseCos * radius * 0.72
                const courseStartY = cy - courseSin * radius * 0.72
                const courseEndX = cx + courseCos * radius * 0.84
                const courseEndY = cy + courseSin * radius * 0.84

                ctx.strokeStyle = "#ff40df"
                ctx.lineWidth = 3
                ctx.beginPath()
                ctx.moveTo(courseStartX, courseStartY)
                ctx.lineTo(courseEndX, courseEndY)
                ctx.stroke()

                const arrowBaseX = cx + courseCos * radius * 0.60
                const arrowBaseY = cy + courseSin * radius * 0.60
                const perpendicularX = -courseSin
                const perpendicularY = courseCos
                ctx.fillStyle = "#ff40df"
                ctx.beginPath()
                ctx.moveTo(courseEndX, courseEndY)
                ctx.lineTo(arrowBaseX + perpendicularX * 8, arrowBaseY + perpendicularY * 8)
                ctx.lineTo(arrowBaseX - perpendicularX * 8, arrowBaseY - perpendicularY * 8)
                ctx.closePath()
                ctx.fill()

                const trackAngle = (root.groundTrackDeg - root.headingDeg - 90) * Math.PI / 180.0
                const tx = cx + Math.cos(trackAngle) * radius * 0.82
                const ty = cy + Math.sin(trackAngle) * radius * 0.82
                ctx.fillStyle = "#38d0ff"
                ctx.beginPath()
                ctx.moveTo(tx, ty)
                ctx.lineTo(tx - 6, ty + 12)
                ctx.lineTo(tx + 6, ty + 12)
                ctx.closePath()
                ctx.fill()

                ctx.fillStyle = "#ffffff"
                ctx.beginPath()
                ctx.moveTo(cx, cy - 16)
                ctx.lineTo(cx + 11, cy + 14)
                ctx.lineTo(cx, cy + 7)
                ctx.lineTo(cx - 11, cy + 14)
                ctx.closePath()
                ctx.fill()
            }
        }

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            width: compact ? 58 : 64
            height: compact ? 32 : 36
            color: "black"
            border.color: "#f5f8fb"
            border.width: 1

            Text {
                anchors.centerIn: parent
                text: ("000" + Number(root.headingDeg).toFixed(0)).slice(-3)
                color: "#f5f8fb"
                font.pixelSize: compact ? 20 : 22
                font.family: "Menlo"
                font.bold: true
            }
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: compact ? 8 : 12
            anchors.top: parent.top
            anchors.topMargin: compact ? 18 : 22
            text: navSource
            color: "#ff40df"
            font.pixelSize: compact ? 12 : 14
            font.bold: true
        }

        Rectangle {
            anchors.left: parent.left
            anchors.leftMargin: compact ? 8 : 12
            anchors.top: parent.top
            anchors.topMargin: compact ? 44 : 48
            width: compact ? 52 : 58
            height: compact ? 22 : 24
            color: "#111821"
            border.color: "#5f6e83"
            border.width: 1

            Text {
                anchors.centerIn: parent
                text: "HDG " + ("000" + Number(root.headingDeg).toFixed(0)).slice(-3)
                color: "#b9c7d4"
                font.pixelSize: compact ? 10 : 11
                font.family: "Menlo"
                font.bold: true
            }
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: compact ? 42 : 46
            text: distanceLabel
            color: "#ff40df"
            font.pixelSize: compact ? 12 : 14
            font.bold: true
        }

        Rectangle {
            anchors.right: parent.right
            anchors.rightMargin: compact ? 8 : 12
            anchors.top: parent.top
            anchors.topMargin: compact ? 44 : 48
            width: compact ? 60 : 66
            height: compact ? 22 : 24
            color: "#111821"
            border.color: "#5f6e83"
            border.width: 1

            Text {
                anchors.centerIn: parent
                text: root.courseLabel
                color: "#ff40df"
                font.pixelSize: compact ? 10 : 11
                font.family: "Menlo"
                font.bold: true
            }
        }
    }

    NumericKeypadPopup {
        id: keypad

        onAcceptedValue: value => {
            if (root.activeEntryMode === "baro") {
                const parsedBaro = Number(value)
                if (!Number.isNaN(parsedBaro) && typeof appController !== "undefined") {
                    appController.baroSettingInHg = parsedBaro
                }
            } else if (root.activeEntryMode === "altitude") {
                const parsedAltitude = Number(value)
                if (!Number.isNaN(parsedAltitude) && typeof appController !== "undefined") {
                    appController.selectedAltitudeFt = parsedAltitude
                }
            }
            root.activeEntryMode = ""
        }
    }

    onHeadingDegChanged: hsiCanvas.requestPaint()
    onCourseBearingDegChanged: hsiCanvas.requestPaint()
    onGroundTrackDegChanged: hsiCanvas.requestPaint()
    onWidthChanged: hsiCanvas.requestPaint()
    onHeightChanged: hsiCanvas.requestPaint()
}
