import QtQuick

Rectangle {
    id: root

    property real headingDeg: 0
    property real groundTrackDeg: 0
    property string navSource: "GPS"
    property string courseLabel: "CRS 043"

    radius: 14
    color: "#09131d"
    border.color: "#223648"
    border.width: 1

    Canvas {
        id: rose
        anchors.fill: parent

        function polar(radius, angleDeg) {
            const radians = (angleDeg - 90) * Math.PI / 180.0
            return {
                x: width / 2 + Math.cos(radians) * radius,
                y: height / 2 + Math.sin(radians) * radius
            }
        }

        onPaint: {
            const ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)

            const cx = width / 2
            const cy = height / 2 + height * 0.02
            const radius = Math.min(width, height) * 0.42

            ctx.save()
            ctx.translate(cx, cy)
            ctx.strokeStyle = "#d8e4ef"
            ctx.lineWidth = 2
            ctx.beginPath()
            ctx.arc(0, 0, radius, 0, Math.PI * 2)
            ctx.stroke()

            for (let deg = 0; deg < 360; deg += 5) {
                const rel = deg - root.headingDeg
                const isMajor = deg % 30 === 0
                const isMedium = deg % 10 === 0
                const tickLength = isMajor ? 16 : (isMedium ? 10 : 5)
                const angle = (rel - 90) * Math.PI / 180.0
                const x1 = Math.cos(angle) * (radius - tickLength)
                const y1 = Math.sin(angle) * (radius - tickLength)
                const x2 = Math.cos(angle) * radius
                const y2 = Math.sin(angle) * radius
                ctx.strokeStyle = isMajor ? "#f5f8fb" : "#aebdca"
                ctx.lineWidth = isMajor ? 2 : 1
                ctx.beginPath()
                ctx.moveTo(x1, y1)
                ctx.lineTo(x2, y2)
                ctx.stroke()
            }

            const labels = [
                { deg: 0, text: "N" },
                { deg: 30, text: "3" },
                { deg: 60, text: "6" },
                { deg: 90, text: "E" },
                { deg: 120, text: "12" },
                { deg: 150, text: "15" },
                { deg: 180, text: "S" },
                { deg: 210, text: "21" },
                { deg: 240, text: "24" },
                { deg: 270, text: "W" },
                { deg: 300, text: "30" },
                { deg: 330, text: "33" }
            ]

            ctx.font = "bold 20px Arial"
            ctx.fillStyle = "#f5f8fb"
            ctx.textAlign = "center"
            ctx.textBaseline = "middle"
            for (let i = 0; i < labels.length; ++i) {
                const rel = labels[i].deg - root.headingDeg
                const angle = (rel - 90) * Math.PI / 180.0
                const x = Math.cos(angle) * (radius - 30)
                const y = Math.sin(angle) * (radius - 30)
                ctx.fillText(labels[i].text, x, y)
            }

            ctx.restore()

            ctx.strokeStyle = "#ff40df"
            ctx.lineWidth = 4
            ctx.beginPath()
            ctx.moveTo(cx, cy + radius * 0.75)
            ctx.lineTo(cx, cy - radius * 0.82)
            ctx.stroke()

            ctx.fillStyle = "#ff40df"
            ctx.beginPath()
            ctx.moveTo(cx, cy - radius * 0.82)
            ctx.lineTo(cx - 10, cy - radius * 0.63)
            ctx.lineTo(cx + 10, cy - radius * 0.63)
            ctx.closePath()
            ctx.fill()

            ctx.strokeStyle = "#38d0ff"
            ctx.lineWidth = 3
            const trackAngle = (root.groundTrackDeg - root.headingDeg - 90) * Math.PI / 180.0
            ctx.beginPath()
            ctx.moveTo(cx, cy)
            ctx.lineTo(cx + Math.cos(trackAngle) * radius * 0.72,
                       cy + Math.sin(trackAngle) * radius * 0.72)
            ctx.stroke()

            ctx.fillStyle = "#ffffff"
            ctx.beginPath()
            ctx.moveTo(cx, cy - 18)
            ctx.lineTo(cx + 12, cy + 16)
            ctx.lineTo(cx, cy + 8)
            ctx.lineTo(cx - 12, cy + 16)
            ctx.closePath()
            ctx.fill()
        }
    }

    onWidthChanged: rose.requestPaint()
    onHeightChanged: rose.requestPaint()
    onHeadingDegChanged: rose.requestPaint()
    onGroundTrackDegChanged: rose.requestPaint()

    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 10
        width: 92
        height: 34
        radius: 8
        color: "#0f1823"
        border.color: "#27425a"

        Text {
            anchors.centerIn: parent
            text: ("000" + Number(root.headingDeg).toFixed(0)).slice(-3)
            color: "#f8fbff"
            font.pixelSize: 20
            font.bold: true
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 12
        width: 92
        height: 34
        radius: 8
        color: "#0f1823"
        border.color: "#27425a"

        Text {
            anchors.centerIn: parent
            text: root.navSource
            color: "#ff40df"
            font.pixelSize: 15
            font.bold: true
        }
    }

    Rectangle {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 12
        width: 110
        height: 34
        radius: 8
        color: "#0f1823"
        border.color: "#27425a"

        Text {
            anchors.centerIn: parent
            text: root.courseLabel
            color: "#38d0ff"
            font.pixelSize: 15
            font.bold: true
        }
    }
}
