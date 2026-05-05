import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root

    property real rpm: 0
    property real manifoldPressureInHg: 0
    property real oilPressurePsi: 0
    property real oilTemperatureF: 0
    property real fuelFlowGph: 0
    property real fuelQuantityPercent: 0
    property var fuelTankStates: []
    property real busVoltage: 0
    property real chtMaxF: 0
    property real egtMaxF: 0
    property bool compact: width < 130

    signal fuelDetailsRequested()
    signal cylinderDetailsRequested(string section)

    function clampValue(value, minimum, maximum) {
        return Math.max(minimum, Math.min(maximum, value))
    }

    function normalized(value, minimum, maximum) {
        if (maximum <= minimum) {
            return 0
        }
        return clampValue((value - minimum) / (maximum - minimum), 0, 1)
    }

    readonly property var chtValues: [
        Math.max(chtMaxF - 18, 0),
        Math.max(chtMaxF - 5, 0),
        Math.max(chtMaxF + 8, 0),
        Math.max(chtMaxF - 12, 0)
    ]
    readonly property var egtValues: [
        Math.max(egtMaxF - 42, 0),
        Math.max(egtMaxF - 8, 0),
        Math.max(egtMaxF + 16, 0),
        Math.max(egtMaxF - 24, 0)
    ]

    radius: 14
    color: "#04070c"
    border.color: "#223648"
    border.width: 1

    component ArcGauge: Item {
        id: gauge

        required property string title
        required property string valueLabel
        required property real value
        required property real minValue
        required property real maxValue
        property real greenStart: 0.08
        property real greenEnd: 0.84
        property real cautionStart: 0.84
        property real cautionEnd: 0.94
        property real alertStart: 0.94
        property real lineWidth: root.compact ? 8 : 10

        Layout.fillWidth: true
        Layout.preferredHeight: root.compact ? 92 : 112

        onValueChanged: gaugeCanvas.requestPaint()
        onWidthChanged: gaugeCanvas.requestPaint()
        onHeightChanged: gaugeCanvas.requestPaint()

        Canvas {
            id: gaugeCanvas
            anchors.fill: parent

            onPaint: {
                const ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)

                const cx = width / 2
                const cy = height * 0.58
                const radius = Math.min(width, height * 1.05) * 0.34
                const startAngle = Math.PI * 0.84
                const endAngle = Math.PI * 0.16

                function drawArc(fromFraction, toFraction, color) {
                    const fromAngle = startAngle + (endAngle - startAngle) * fromFraction
                    const toAngle = startAngle + (endAngle - startAngle) * toFraction
                    ctx.beginPath()
                    ctx.strokeStyle = color
                    ctx.lineWidth = gauge.lineWidth
                    ctx.lineCap = "round"
                    ctx.arc(cx, cy, radius, fromAngle, toAngle, true)
                    ctx.stroke()
                }

                drawArc(0.0, 1.0, "#d7dbe3")
                drawArc(gauge.greenStart, gauge.greenEnd, "#22d13d")
                drawArc(gauge.cautionStart, gauge.cautionEnd, "#f3f556")
                drawArc(gauge.alertStart, 1.0, "#e93939")

                const ratio = root.normalized(gauge.value, gauge.minValue, gauge.maxValue)
                const needleAngle = startAngle + (endAngle - startAngle) * ratio
                const tipX = cx + Math.cos(needleAngle) * (radius - 2)
                const tipY = cy + Math.sin(needleAngle) * (radius - 2)

                ctx.beginPath()
                ctx.strokeStyle = "#f8fbff"
                ctx.lineWidth = 4
                ctx.lineCap = "round"
                ctx.moveTo(cx, cy)
                ctx.lineTo(tipX, tipY)
                ctx.stroke()

                ctx.beginPath()
                ctx.fillStyle = "#f8fbff"
                ctx.arc(cx, cy, 4.5, 0, Math.PI * 2)
                ctx.fill()
            }
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 2
            text: gauge.title
            color: "#f8fbff"
            font.pixelSize: root.compact ? 10 : 11
            font.bold: true
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: root.compact ? 6 : 8
            text: gauge.valueLabel
            color: "#f8fbff"
            font.pixelSize: root.compact ? 9 : 10
            font.bold: true
        }
    }

    component LinearGauge: Item {
        id: gauge

        required property string title
        required property string valueLabel
        required property real value
        required property real minValue
        required property real maxValue
        property real lowWhiteFraction: 0.0
        property real lowCautionFraction: 0.12
        property real highCautionFraction: 0.88
        property real highAlertFraction: 0.96

        Layout.fillWidth: true
        Layout.preferredHeight: root.compact ? 36 : 40

        onValueChanged: markerCanvas.requestPaint()
        onWidthChanged: markerCanvas.requestPaint()
        onHeightChanged: markerCanvas.requestPaint()

        Column {
            anchors.fill: parent
            spacing: 1

            Row {
                width: parent.width

                Text {
                    id: titleText
                    text: gauge.title
                    color: "#f8fbff"
                    font.pixelSize: root.compact ? 9 : 10
                    font.bold: true
                }

                Item {
                    width: Math.max(0, parent.width - titleText.width - valueText.width - 8)
                    height: 1
                }

                Text {
                    id: valueText
                    text: gauge.valueLabel
                    color: "#f8fbff"
                    font.pixelSize: root.compact ? 9 : 10
                    font.bold: true
                }
            }

            Item {
                width: parent.width
                height: root.compact ? 16 : 18

                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width
                    height: 6
                    radius: 3
                    color: "#e5e8ee"
                    opacity: 0.18
                }

                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    x: 0
                    width: parent.width * gauge.lowWhiteFraction
                    height: 6
                    radius: 3
                    color: "#e93939"
                }

                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    x: parent.width * gauge.lowWhiteFraction
                    width: parent.width * Math.max(0, gauge.lowCautionFraction - gauge.lowWhiteFraction)
                    height: 6
                    color: "#f8fbff"
                }

                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    x: parent.width * gauge.lowCautionFraction
                    width: parent.width * Math.max(0, gauge.highCautionFraction - gauge.lowCautionFraction)
                    height: 6
                    color: "#22d13d"
                }

                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    x: parent.width * gauge.highCautionFraction
                    width: parent.width * Math.max(0, gauge.highAlertFraction - gauge.highCautionFraction)
                    height: 6
                    color: "#f3f556"
                }

                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    x: parent.width * gauge.highAlertFraction
                    width: parent.width * Math.max(0, 1.0 - gauge.highAlertFraction)
                    height: 6
                    radius: 3
                    color: "#e93939"
                }

                Canvas {
                    id: markerCanvas
                    anchors.fill: parent

                    onPaint: {
                        const ctx = getContext("2d")
                        ctx.clearRect(0, 0, width, height)

                        const ratio = root.normalized(gauge.value, gauge.minValue, gauge.maxValue)
                        const x = ratio * width

                        ctx.beginPath()
                        ctx.fillStyle = "#f8fbff"
                        ctx.moveTo(x, 0)
                        ctx.lineTo(x - 6, 9)
                        ctx.lineTo(x + 6, 9)
                        ctx.closePath()
                        ctx.fill()
                    }
                }
            }
        }
    }

    component CylinderGroup: Item {
        id: group

        required property string title
        required property string valueLabel
        required property var values
        required property real minValue
        required property real maxValue
        property real cautionValue: 0
        property real alertValue: 0

        Layout.fillWidth: true
        Layout.preferredHeight: root.compact ? 72 : 84

        Column {
            anchors.fill: parent
            spacing: 2

            Row {
                width: parent.width

                Text {
                    id: groupTitleText
                    text: group.title
                    color: "#f8fbff"
                    font.pixelSize: root.compact ? 9 : 10
                    font.bold: true
                }

                Item {
                    width: Math.max(0, parent.width - groupTitleText.width - valueLabelText.width - 8)
                    height: 1
                }

                Text {
                    id: valueLabelText
                    text: group.valueLabel
                    color: "#f8fbff"
                    font.pixelSize: root.compact ? 9 : 10
                    font.bold: true
                }
            }

            Row {
                width: parent.width
                height: root.compact ? 56 : 66
                spacing: 5

                Repeater {
                    model: group.values

                    delegate: Item {
                        required property real modelData

                        width: (parent.width - (group.values.length - 1) * parent.spacing) / group.values.length
                        height: parent.height

                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            height: parent.height
                            radius: 2
                            color: "#0c1622"
                            border.color: "#27425a"
                        }

                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            height: parent.height * root.normalized(modelData, group.minValue, group.maxValue)
                            radius: 2
                            color: modelData >= group.alertValue
                                ? "#e93939"
                                : modelData >= group.cautionValue
                                    ? "#f3f556"
                                    : "#22d13d"
                        }
                    }
                }
            }
        }
    }

    component TankGroup: Item {
        id: tankGroup

        required property var tanks

        Layout.fillWidth: true
        Layout.preferredHeight: root.compact
            ? (18 + Math.max(1, tankGroup.tanks.length) * 20)
            : (22 + Math.max(1, tankGroup.tanks.length) * 22)

        Column {
            anchors.fill: parent
            spacing: root.compact ? 2 : 3

            Text {
                text: "FUEL QTY"
                color: "#f8fbff"
                font.pixelSize: root.compact ? 9 : 10
                font.bold: true
            }

            Column {
                width: parent.width
                spacing: root.compact ? 1 : 2
                Repeater {
                    model: tankGroup.tanks

                    delegate: LinearGauge {
                        required property var modelData
                        width: parent.width
                        title: String(modelData.label || "") + " GAL"
                        value: Number(modelData.gallons || 0)
                        minValue: 0
                        maxValue: Math.max(1, Number(modelData.capacityGal || 1))
                        valueLabel: Number(modelData.gallons || 0).toFixed(1)
                        lowWhiteFraction: 0.0
                        lowCautionFraction: 0.10
                        highCautionFraction: 0.92
                        highAlertFraction: 0.98
                    }
                }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.compact ? 8 : 10
        spacing: root.compact ? 6 : 8

        ArcGauge {
            title: "MAN IN"
            value: root.manifoldPressureInHg
            minValue: 10
            maxValue: 30
            valueLabel: Number(root.manifoldPressureInHg).toFixed(0)
            greenStart: 0.12
            greenEnd: 0.82
            cautionStart: 0.82
            cautionEnd: 0.92
            alertStart: 0.92
        }

        ArcGauge {
            title: "RPM"
            value: root.rpm
            minValue: 0
            maxValue: 2800
            valueLabel: Number(root.rpm).toFixed(0)
            greenStart: 0.16
            greenEnd: 0.84
            cautionStart: 0.84
            cautionEnd: 0.92
            alertStart: 0.92
        }

        LinearGauge {
            title: "GPH"
            value: root.fuelFlowGph
            minValue: 0
            maxValue: 16
            valueLabel: Number(root.fuelFlowGph).toFixed(1)
            lowWhiteFraction: 0.0
            lowCautionFraction: 0.06
            highCautionFraction: 0.84
            highAlertFraction: 0.93

            MouseArea {
                anchors.fill: parent
                onClicked: root.fuelDetailsRequested()
            }
        }

        LinearGauge {
            title: "OIL PSI"
            value: root.oilPressurePsi
            minValue: 0
            maxValue: 100
            valueLabel: Number(root.oilPressurePsi).toFixed(0)
            lowWhiteFraction: 0.12
            lowCautionFraction: 0.18
            highCautionFraction: 0.82
            highAlertFraction: 0.94
        }

        LinearGauge {
            title: "OIL F"
            value: root.oilTemperatureF
            minValue: 100
            maxValue: 260
            valueLabel: Number(root.oilTemperatureF).toFixed(0)
            lowWhiteFraction: 0.0
            lowCautionFraction: 0.08
            highCautionFraction: 0.72
            highAlertFraction: 0.88
        }

        TankGroup {
            tanks: root.fuelTankStates

            MouseArea {
                anchors.fill: parent
                onClicked: root.fuelDetailsRequested()
            }
        }

        CylinderGroup {
            title: "CHT F"
            valueLabel: Number(root.chtMaxF).toFixed(0)
            values: root.chtValues
            minValue: 200
            maxValue: 470
            cautionValue: 400
            alertValue: 430

            MouseArea {
                anchors.fill: parent
                onClicked: root.cylinderDetailsRequested("CHT")
            }
        }

        CylinderGroup {
            title: "EGT F"
            valueLabel: Number(root.egtMaxF).toFixed(0)
            values: root.egtValues
            minValue: 1000
            maxValue: 1550
            cautionValue: 1450
            alertValue: 1500

            MouseArea {
                anchors.fill: parent
                onClicked: root.cylinderDetailsRequested("EGT")
            }
        }

        Item {
            Layout.fillHeight: true
            Layout.minimumHeight: 1
        }

        LinearGauge {
            title: "BUS V"
            value: root.busVoltage
            minValue: 20
            maxValue: 32
            valueLabel: Number(root.busVoltage).toFixed(1)
            lowWhiteFraction: 0.0
            lowCautionFraction: 0.18
            highCautionFraction: 0.82
            highAlertFraction: 0.94
        }
    }
}
