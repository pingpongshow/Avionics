import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import "components"

ApplicationWindow {
    id: root

    width: appController.displayClass === "primary" ? 1280
           : appController.displayClass === "compact" ? 1024
           : 800
    height: appController.displayClass === "primary" ? 800
            : appController.displayClass === "compact" ? 600
            : 480
    visible: true
    color: "#071018"
    title: "Experimental MFD Demo"

    readonly property color panelColor: "#0e1824"
    readonly property color borderColor: "#203244"
    readonly property color accentColor: "#1f9fff"
    readonly property color textColor: "#f3f7fb"
    readonly property color dimTextColor: "#9db3c7"
    readonly property color cautionColor: "#f59e0b"
    readonly property color warningColor: "#ef4444"
    readonly property color okColor: "#22c55e"
    property int dismissedWarningSerial: -1
    property int dismissedCautionSerial: -1
    property int lastPlayedWarningSerial: 0
    property int lastPlayedCautionSerial: 0
    property bool warningFlashState: true

    function pageSource(pageName) {
        switch (pageName) {
        case "PFD + Map":
            return "pages/PfdMapSplitPage.qml"
        case "PFD Focus":
            return "pages/PfdFocusPage.qml"
        case "Full Map":
            return "pages/FullMapPage.qml"
        case "Engine":
            return "pages/EnginePage.qml"
        case "Traffic":
            return "pages/TrafficPage.qml"
        case "System Status":
            return "pages/SystemStatusPage.qml"
        case "Setup":
            return "pages/SetupPage.qml"
        default:
            return "pages/SystemStatusPage.qml"
        }
    }

    Shortcut {
        sequences: [StandardKey.MoveToNextPage]
        onActivated: appController.nextPage()
    }

    Shortcut {
        sequences: [StandardKey.MoveToPreviousPage]
        onActivated: appController.previousPage()
    }

    Shortcut {
        sequence: "Right"
        onActivated: appController.nextPage()
    }

    Shortcut {
        sequence: "Left"
        onActivated: appController.previousPage()
    }

    Shortcut {
        sequence: "1"
        onActivated: {
            if (appController.availablePages.indexOf("PFD + Map") >= 0) {
                appController.currentPage = "PFD + Map"
            }
        }
    }

    Shortcut {
        sequence: "2"
        onActivated: {
            if (appController.availablePages.indexOf("PFD Focus") >= 0) {
                appController.currentPage = "PFD Focus"
            }
        }
    }

    Shortcut {
        sequence: "3"
        onActivated: {
            if (appController.availablePages.indexOf("Full Map") >= 0) {
                appController.currentPage = "Full Map"
            }
        }
    }

    Shortcut {
        sequence: "4"
        onActivated: appController.currentPage = "System Status"
    }

    Shortcut {
        sequence: "5"
        onActivated: appController.currentPage = "Setup"
    }

    SoundEffect {
        id: warningSound
        source: "qrc:/ExperimentalMfd/qml/assets/audio/warning_whoop.wav"
        volume: 0.95
    }

    SoundEffect {
        id: cautionSound
        source: "qrc:/ExperimentalMfd/qml/assets/audio/caution_ding.wav"
        volume: 0.95
    }

    Timer {
        interval: 420
        repeat: true
        running: appController.warningActive && root.dismissedWarningSerial !== appController.warningEventSerial
        onTriggered: root.warningFlashState = !root.warningFlashState
    }

    Connections {
        target: appController

        function onAlertsChanged() {
            if (appController.warningEventSerial > root.lastPlayedWarningSerial && appController.warningActive) {
                root.lastPlayedWarningSerial = appController.warningEventSerial
                root.dismissedWarningSerial = -1
                warningSound.play()
            } else if (appController.cautionEventSerial > root.lastPlayedCautionSerial
                       && appController.cautionActive
                       && !appController.warningActive) {
                root.lastPlayedCautionSerial = appController.cautionEventSerial
                root.dismissedCautionSerial = -1
                cautionSound.play()
            } else if (appController.cautionEventSerial > root.lastPlayedCautionSerial && appController.cautionActive) {
                root.lastPlayedCautionSerial = appController.cautionEventSerial
                root.dismissedCautionSerial = -1
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        TopBar {
            Layout.fillWidth: true
        }

        Loader {
            id: pageLoader
            Layout.fillWidth: true
            Layout.fillHeight: true
            source: root.pageSource(appController.currentPage)
        }
    }

    Rectangle {
        id: warningPopup
        visible: appController.warningActive && root.dismissedWarningSerial !== appController.warningEventSerial
        anchors.top: parent.top
        anchors.topMargin: 62
        anchors.right: parent.right
        anchors.rightMargin: 18
        width: Math.min(parent.width * 0.42, 420)
        height: warningColumn.implicitHeight + 22
        radius: 12
        color: "#0a1119"
        border.color: root.warningFlashState ? "#ff2b2b" : "#7c1414"
        border.width: 4
        z: 10

        Column {
            id: warningColumn
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8

            Text {
                text: "WARNING"
                color: "#f8fbff"
                font.pixelSize: 20
                font.bold: true
            }

            Repeater {
                model: appController.warningMessages

                delegate: Text {
                    required property string modelData
                    text: modelData
                    color: "#f8fbff"
                    font.pixelSize: 14
                    wrapMode: Text.WordWrap
                    width: warningPopup.width - 48
                }
            }

            Row {
                width: parent.width

                Item {
                    width: parent.width - warningDismissButton.implicitWidth
                    height: 1
                }

                Button {
                    id: warningDismissButton
                    text: "Dismiss"
                    onClicked: root.dismissedWarningSerial = appController.warningEventSerial
                }
            }
        }
    }

    Rectangle {
        id: cautionPopup
        visible: !warningPopup.visible
                 && appController.cautionActive
                 && root.dismissedCautionSerial !== appController.cautionEventSerial
        anchors.top: parent.top
        anchors.topMargin: 62
        anchors.right: parent.right
        anchors.rightMargin: 18
        width: Math.min(parent.width * 0.40, 400)
        height: cautionColumn.implicitHeight + 22
        radius: 12
        color: "#0a1119"
        border.color: "#f5c66e"
        border.width: 4
        z: 9

        Column {
            id: cautionColumn
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8

            Text {
                text: "CAUTION"
                color: "#f8fbff"
                font.pixelSize: 19
                font.bold: true
            }

            Repeater {
                model: appController.cautionMessages

                delegate: Text {
                    required property string modelData
                    text: modelData
                    color: "#f8fbff"
                    font.pixelSize: 14
                    wrapMode: Text.WordWrap
                    width: cautionPopup.width - 48
                }
            }

            Row {
                width: parent.width

                Item {
                    width: parent.width - cautionDismissButton.implicitWidth
                    height: 1
                }

                Button {
                    id: cautionDismissButton
                    text: "Dismiss"
                    onClicked: root.dismissedCautionSerial = appController.cautionEventSerial
                }
            }
        }
    }
}
