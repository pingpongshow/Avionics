import QtQuick

Rectangle {
    id: root

    property int trafficCount: 0
    property real groundTrackDeg: 0
    property real nearestTrafficRangeNm: 0
    property real nearestTrafficRelativeAltitudeFt: 0
    property var trafficTargets: []

    radius: 14
    color: "black"
    border.color: "#223648"
    border.width: 1

    Canvas {
        id: radarCanvas
        anchors.fill: parent

        function drawDiamond(ctx, x, y, size, color, filled) {
            ctx.save()
            ctx.translate(x, y)
            ctx.rotate(Math.PI / 4)
            if (filled) {
                ctx.fillStyle = color
                ctx.fillRect(-size / 2, -size / 2, size, size)
            } else {
                ctx.strokeStyle = color
                ctx.lineWidth = 2
                ctx.strokeRect(-size / 2, -size / 2, size, size)
            }
            ctx.restore()
        }

        onPaint: {
            const ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)

            const cx = width / 2
            const cy = height * 0.58
            const outer = Math.min(width, height) * 0.43
            const inner = outer * 0.34

            ctx.strokeStyle = "#d7dde4"
            ctx.lineWidth = 2
            ctx.beginPath()
            ctx.arc(cx, cy, outer, 0, Math.PI * 2)
            ctx.stroke()
            ctx.beginPath()
            ctx.arc(cx, cy, inner, 0, Math.PI * 2)
            ctx.stroke()

            ctx.strokeStyle = "#9ba9b8"
            ctx.lineWidth = 1
            ctx.beginPath()
            ctx.moveTo(cx, cy - outer)
            ctx.lineTo(cx, cy + outer)
            ctx.stroke()

            ctx.strokeStyle = "#ff40df"
            ctx.lineWidth = 4
            ctx.beginPath()
            ctx.moveTo(cx, cy + outer)
            ctx.lineTo(cx, cy - outer * 0.05)
            ctx.stroke()

            ctx.fillStyle = "#ffffff"
            ctx.beginPath()
            ctx.moveTo(cx, cy - 18)
            ctx.lineTo(cx + 11, cy + 14)
            ctx.lineTo(cx, cy + 7)
            ctx.lineTo(cx - 11, cy + 14)
            ctx.closePath()
            ctx.fill()

            ctx.fillStyle = "#e5edf5"
            ctx.font = "bold 15px Arial"
            ctx.textAlign = "center"
            ctx.fillText("35", cx, cy - outer + 16)

            ctx.textAlign = "left"
            ctx.fillStyle = "#f3f7fb"
            ctx.fillText("2NM", cx + inner + 12, cy + 2)
            ctx.fillText("6NM", cx + outer * 0.78, cy - outer * 0.78)

            for (let i = 0; i < Math.min(root.trafficCount, root.trafficTargets.length); ++i) {
                const model = root.trafficTargets[i]
                const relativeAngle = (Number(model.bearingDeg || 0) - root.groundTrackDeg - 90) * Math.PI / 180.0
                const rangeNm = Number(model.rangeNm || 0)
                const targetRadius = Math.min(outer, (rangeNm / 6.0) * outer)
                const target = {
                    x: cx + Math.cos(relativeAngle) * targetRadius,
                    y: cy + Math.sin(relativeAngle) * targetRadius,
                    alt: (Number(model.relativeAltitudeFt || 0) >= 0 ? "+" : "") + Math.round(Number(model.relativeAltitudeFt || 0) / 100),
                    tag: model.callsign ? String(model.callsign).slice(0, 2) : "",
                    style: model.style === "filled" ? "filledYellow" : (Number(model.verticalTrend || 0) > 0 ? "openWhiteUp" : "openWhite")
                }

                if (target.style === "filledYellow") {
                    ctx.fillStyle = "#f6f219"
                    ctx.beginPath()
                    ctx.arc(target.x, target.y, 12, 0, Math.PI * 2)
                    ctx.fill()
                    ctx.strokeStyle = "#f6f219"
                    ctx.lineWidth = 2
                    ctx.stroke()
                } else {
                    this.drawDiamond(ctx, target.x, target.y, 16, "#ffffff", false)
                }

                if (target.style === "openWhiteUp") {
                    ctx.strokeStyle = "#ffffff"
                    ctx.lineWidth = 2
                    ctx.beginPath()
                    ctx.moveTo(target.x, target.y - 20)
                    ctx.lineTo(target.x, target.y - 7)
                    ctx.stroke()
                    ctx.beginPath()
                    ctx.moveTo(target.x, target.y - 20)
                    ctx.lineTo(target.x - 5, target.y - 14)
                    ctx.lineTo(target.x + 5, target.y - 14)
                    ctx.closePath()
                    ctx.fillStyle = "#ffffff"
                    ctx.fill()
                }

                ctx.fillStyle = "#ffffff"
                ctx.font = "bold 18px Arial"
                ctx.textAlign = "center"
                ctx.fillText(target.alt, target.x, target.y - 24)

                if (target.tag.length > 0) {
                    ctx.fillStyle = "#d0d62d"
                    ctx.font = "bold 16px Arial"
                    ctx.fillText(target.alt, target.x, target.y - 24)
                    ctx.fillStyle = "#0f0f16"
                    ctx.fillRect(target.x + 6, target.y - 14, 24, 18)
                    ctx.strokeStyle = "#b44cd8"
                    ctx.lineWidth = 1
                    ctx.strokeRect(target.x + 6, target.y - 14, 24, 18)
                    ctx.fillStyle = "#ff40df"
                    ctx.font = "bold 12px Arial"
                    ctx.textAlign = "center"
                    ctx.fillText(target.tag, target.x + 18, target.y - 1)
                }
            }

            ctx.fillStyle = "#ffffff"
            ctx.font = "bold 18px Arial"
            ctx.textAlign = "center"
            ctx.fillText("JWKGD", cx, cy + outer * 0.82)
            ctx.fillStyle = "#38d0ff"
            ctx.beginPath()
            ctx.moveTo(cx, cy + outer * 0.75)
            ctx.lineTo(cx - 5, cy + outer * 0.81)
            ctx.lineTo(cx + 5, cy + outer * 0.81)
            ctx.closePath()
            ctx.fill()
        }
    }

    onWidthChanged: radarCanvas.requestPaint()
    onHeightChanged: radarCanvas.requestPaint()
    onTrafficCountChanged: radarCanvas.requestPaint()
    onGroundTrackDegChanged: radarCanvas.requestPaint()
    onTrafficTargetsChanged: radarCanvas.requestPaint()

    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 8
        width: 124
        height: 40
        color: "transparent"
        border.color: "#6ed3ff"
        border.width: 1

        Text {
            anchors.centerIn: parent
            text: "OPERATING"
            color: "#6ed3ff"
            font.pixelSize: 15
            font.bold: true
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: 8
        anchors.topMargin: 52
        width: 124
        height: 40
        color: "transparent"
        border.color: "#6ed3ff"
        border.width: 1

        Text {
            anchors.centerIn: parent
            text: "UNRESTRICTED"
            color: "#6ed3ff"
            font.pixelSize: 14
            font.bold: true
        }
    }

    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 8
        width: 148
        height: 36
        color: "#1d1d24"
        border.color: "#70747d"
        border.width: 1

        Row {
            anchors.centerIn: parent
            spacing: 12

            Text {
                text: "6KT"
                color: "#f3f7fb"
                font.pixelSize: 14
                font.bold: true
            }

            Text {
                text: "HDG UP"
                color: "#f3f7fb"
                font.pixelSize: 14
                font.bold: true
            }
        }
    }
}
