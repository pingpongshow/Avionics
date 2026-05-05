import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: root

    property string selectedSection: "system"
    property string speedUnit: "KT"
    property string altitudeUnit: "FT"
    property string temperatureUnit: "F"

    function closeSetup() {
        if (appController.availablePages.indexOf("PFD + Map") >= 0) {
            appController.currentPage = "PFD + Map"
        } else if (appController.availablePages.length > 0) {
            appController.currentPage = appController.availablePages[0]
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: 12
        color: "#1d2230"
        border.color: "#596a7e"
        border.width: 1
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        RowLayout {
            Layout.fillWidth: true

            ColumnLayout {
                spacing: 2

                Text {
                    text: "Configuration Mode"
                    color: "#f8fbff"
                    font.pixelSize: 22
                    font.bold: true
                }

                Text {
                    text: appController.setupAccessAllowed
                        ? "Installed-system settings and bench configuration"
                        : "Setup is locked while the engine is running outside Demo Mode"
                    color: appController.setupAccessAllowed ? "#c0d0dd" : "#f5c66e"
                    font.pixelSize: 13
                }
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "Close"
                onClicked: root.closeSetup()
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: appController.displayClass === "primary" ? 6 : 3
            columnSpacing: 10
            rowSpacing: 10

            Repeater {
                model: [
                    { key: "system", title: "System Info", iconSource: "qrc:/ExperimentalMfd/qml/assets/icons/system_info.svg", color: "#56d6ff" },
                    { key: "aircraft", title: "Aircraft", iconSource: "qrc:/ExperimentalMfd/qml/assets/icons/aircraft.svg", color: "#9de29b" },
                    { key: "weight", title: "Weight & Balance", iconSource: "qrc:/ExperimentalMfd/qml/assets/icons/weight_balance.svg", color: "#f5c66e" },
                    { key: "units", title: "Units", iconSource: "qrc:/ExperimentalMfd/qml/assets/icons/units.svg", color: "#d7dde4" },
                    { key: "gps", title: "GPS / Map", iconSource: "qrc:/ExperimentalMfd/qml/assets/icons/gps.svg", color: "#56d6ff" },
                    { key: "engine", title: "Engine & Airframe", iconSource: "qrc:/ExperimentalMfd/qml/assets/icons/engine.svg", color: "#ff7b7b" }
                ]

                delegate: MenuTile {
                    required property var modelData

                    Layout.fillWidth: true
                    tileHeight: 92
                    iconBoxSize: 34
                    iconSize: 20
                    titlePixelSize: 10
                    squareIconBox: true
                    title: modelData.title
                    iconSource: modelData.iconSource
                    iconColor: modelData.color
                    border.color: root.selectedSection === modelData.key ? "#56d6ff" : "#6c7d90"

                    onClicked: root.selectedSection = modelData.key
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 10
            color: "#0a1119"
            border.color: "#355069"
            border.width: 1

            Loader {
                anchors.fill: parent
                anchors.margins: 14

                sourceComponent: {
                    switch (root.selectedSection) {
                    case "aircraft": return aircraftSection
                    case "weight": return weightSection
                    case "units": return unitsSection
                    case "gps": return gpsSection
                    case "engine": return engineSection
                    default: return systemSection
                    }
                }
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: 12
        visible: !appController.setupAccessAllowed
        color: "#aa081018"
        border.color: "#ef4444"
        border.width: 2

        Column {
            anchors.centerIn: parent
            spacing: 8

            Text {
                text: "SETUP LOCKED"
                color: "#f8fbff"
                font.pixelSize: 30
                font.bold: true
            }

            Text {
                text: "Engine must be stopped unless Demo Mode is enabled."
                color: "#f5c66e"
                font.pixelSize: 15
            }
        }
    }

    Component {
        id: systemSection

        ColumnLayout {
            spacing: 12

            Text {
                text: "System Information"
                color: "#f8fbff"
                font.pixelSize: 20
                font.bold: true
            }

            Text {
                text: "Target: CM5 installed unit | Pi 5 bench integration | Yocto + Qt 6/QML/C++"
                color: "#c0d0dd"
                wrapMode: Text.WordWrap
                font.pixelSize: 14
            }

            Text {
                text: "Chart status: " + chartManager.chartStatus
                color: "#c0d0dd"
                wrapMode: Text.WordWrap
                font.pixelSize: 14
            }

            RowLayout {
                spacing: 10

                Text {
                    text: "Demo Mode"
                    color: "#f8fbff"
                    font.pixelSize: 15
                }

                Switch {
                    checked: appController.demoMode
                    onToggled: appController.demoMode = checked
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: "Setup access: " + (appController.setupAccessAllowed ? "allowed" : "locked")
                    color: appController.setupAccessAllowed ? "#9de29b" : "#f5c66e"
                    font.pixelSize: 14
                    font.bold: true
                }
            }

            RowLayout {
                spacing: 10

                Text {
                    text: "Display Class"
                    color: "#f8fbff"
                    font.pixelSize: 15
                }

                Repeater {
                    model: [
                        { key: "primary", label: "10in" },
                        { key: "compact", label: "7in" },
                        { key: "standby", label: "5in" }
                    ]

                    delegate: Button {
                        required property var modelData
                        text: modelData.label
                        highlighted: appController.displayClass === modelData.key
                        onClicked: appController.displayClass = modelData.key
                    }
                }
            }
        }
    }

    Component {
        id: aircraftSection

        ColumnLayout {
            spacing: 12

            Text {
                text: "Aircraft"
                color: "#f8fbff"
                font.pixelSize: 20
                font.bold: true
            }

            RowLayout {
                spacing: 10

                Text {
                    text: "Tail Number"
                    color: "#f8fbff"
                    font.pixelSize: 15
                }

                TextField {
                    id: tailNumberField
                    Layout.preferredWidth: 180
                    text: appController.tailNumber
                }

                Button {
                    text: "Apply"
                    onClicked: appController.tailNumber = tailNumberField.text
                }
            }

            Text {
                text: "Use this page for aircraft-specific identity and installation settings."
                color: "#c0d0dd"
                wrapMode: Text.WordWrap
                font.pixelSize: 14
            }
        }
    }

    Component {
        id: weightSection

        ColumnLayout {
            spacing: 12

            property real emptyWeightLb: 1460
            property real emptyCgIn: 97.4

            ListModel {
                id: stationModel
                ListElement { description: "Front Seat"; armIn: "98.0" }
                ListElement { description: "Rear Seat"; armIn: "138.0" }
                ListElement { description: "Fuel Tanks"; armIn: "125.0" }
                ListElement { description: "Baggage"; armIn: "178.0" }
            }

            ListModel {
                id: envelopeModel
                ListElement { weightLb: "1500"; cgIn: "94.0" }
                ListElement { weightLb: "2100"; cgIn: "96.5" }
                ListElement { weightLb: "2550"; cgIn: "102.0" }
                ListElement { weightLb: "2550"; cgIn: "108.0" }
                ListElement { weightLb: "1500"; cgIn: "106.0" }
            }

            Text {
                text: "Weight & Balance"
                color: "#f8fbff"
                font.pixelSize: 20
                font.bold: true
            }

            RowLayout {
                spacing: 14

                ColumnLayout {
                    spacing: 4
                    Text { text: "Empty Weight"; color: "#f8fbff"; font.pixelSize: 13 }
                    TextField {
                        Layout.preferredWidth: 120
                        text: Number(weightSection.emptyWeightLb).toFixed(1)
                        onEditingFinished: weightSection.emptyWeightLb = Number(text)
                    }
                }

                ColumnLayout {
                    spacing: 4
                    Text { text: "Empty CG"; color: "#f8fbff"; font.pixelSize: 13 }
                    TextField {
                        Layout.preferredWidth: 120
                        text: Number(weightSection.emptyCgIn).toFixed(1)
                        onEditingFinished: weightSection.emptyCgIn = Number(text)
                    }
                }
            }

            Text {
                text: "Stations define the arm for loading inputs used later by the fuel computer and weight/balance pages."
                color: "#c0d0dd"
                wrapMode: Text.WordWrap
                font.pixelSize: 14
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 180
                radius: 8
                color: "#08111b"
                border.color: "#355069"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Custom Stations"; color: "#f8fbff"; font.pixelSize: 16; font.bold: true }
                        Item { Layout.fillWidth: true }
                        Button {
                            text: "Add Station"
                            onClicked: stationModel.append({ description: "New Station", armIn: "0.0" })
                        }
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 6
                        model: stationModel

                        delegate: RowLayout {
                            required property int index
                            required property string description
                            required property string armIn
                            width: ListView.view.width
                            spacing: 8

                            TextField {
                                Layout.fillWidth: true
                                text: description
                                onEditingFinished: stationModel.setProperty(index, "description", text)
                            }

                            TextField {
                                Layout.preferredWidth: 110
                                text: armIn
                                placeholderText: "Arm"
                                onEditingFinished: stationModel.setProperty(index, "armIn", text)
                            }

                            Button {
                                text: "Delete"
                                onClicked: stationModel.remove(index)
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 8
                color: "#08111b"
                border.color: "#355069"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "CG Envelope"; color: "#f8fbff"; font.pixelSize: 16; font.bold: true }
                        Item { Layout.fillWidth: true }
                        Button {
                            text: "Add Point"
                            onClicked: envelopeModel.append({ weightLb: "0", cgIn: "0.0" })
                        }
                    }

                    Text {
                        text: "Enter the approved envelope points in order around the polygon."
                        color: "#c0d0dd"
                        wrapMode: Text.WordWrap
                        font.pixelSize: 13
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 6
                        model: envelopeModel

                        delegate: RowLayout {
                            required property int index
                            required property string weightLb
                            required property string cgIn
                            width: ListView.view.width
                            spacing: 8

                            TextField {
                                Layout.preferredWidth: 120
                                text: weightLb
                                placeholderText: "Weight LB"
                                onEditingFinished: envelopeModel.setProperty(index, "weightLb", text)
                            }

                            TextField {
                                Layout.preferredWidth: 120
                                text: cgIn
                                placeholderText: "CG IN"
                                onEditingFinished: envelopeModel.setProperty(index, "cgIn", text)
                            }

                            Button {
                                text: "Delete"
                                onClicked: envelopeModel.remove(index)
                            }
                        }
                    }
                }
            }
        }
    }

    Component {
        id: unitsSection

        ColumnLayout {
            spacing: 12

            Text {
                text: "Units"
                color: "#f8fbff"
                font.pixelSize: 20
                font.bold: true
            }

            RowLayout {
                spacing: 12

                ColumnLayout {
                    spacing: 4
                    Text { text: "Speed"; color: "#f8fbff"; font.pixelSize: 13 }
                    ComboBox {
                        model: ["KT", "MPH", "KM/H"]
                        currentIndex: model.indexOf(root.speedUnit)
                        onActivated: root.speedUnit = currentText
                    }
                }

                ColumnLayout {
                    spacing: 4
                    Text { text: "Altitude"; color: "#f8fbff"; font.pixelSize: 13 }
                    ComboBox {
                        model: ["FT", "M"]
                        currentIndex: model.indexOf(root.altitudeUnit)
                        onActivated: root.altitudeUnit = currentText
                    }
                }

                ColumnLayout {
                    spacing: 4
                    Text { text: "Temperature"; color: "#f8fbff"; font.pixelSize: 13 }
                    ComboBox {
                        model: ["F", "C"]
                        currentIndex: model.indexOf(root.temperatureUnit)
                        onActivated: root.temperatureUnit = currentText
                    }
                }
            }

            Text {
                text: "Selected units are a UI-layer preference in this demo build."
                color: "#c0d0dd"
                wrapMode: Text.WordWrap
                font.pixelSize: 14
            }
        }
    }

    Component {
        id: gpsSection

        ColumnLayout {
            spacing: 12

            Text {
                text: "GPS / Map"
                color: "#f8fbff"
                font.pixelSize: 20
                font.bold: true
            }

            Text {
                text: "Position: " + Number(appController.latitudeDeg).toFixed(4) + ", " + Number(appController.longitudeDeg).toFixed(4)
                color: "#c0d0dd"
                font.pixelSize: 14
            }

            Text {
                text: "Ground speed: " + Number(appController.groundSpeedKt).toFixed(0) + " KT | Track: " + ("000" + Number(appController.groundTrackDeg).toFixed(0)).slice(-3)
                color: "#c0d0dd"
                font.pixelSize: 14
            }

            RowLayout {
                spacing: 12

                Text {
                    text: "Base Chart"
                    color: "#f8fbff"
                    font.pixelSize: 14
                }

                ComboBox {
                    model: appController.mapBaseSources
                    currentIndex: model.indexOf(appController.mapBaseSource)
                    onActivated: appController.mapBaseSource = currentText
                }

                CheckBox {
                    text: "openAIP Overlay"
                    checked: appController.openAipOverlayEnabled
                    onToggled: appController.openAipOverlayEnabled = checked
                }

                CheckBox {
                    text: "Weather Overlay"
                    checked: appController.weatherOverlayEnabled
                    onToggled: appController.weatherOverlayEnabled = checked
                }
            }

            RowLayout {
                spacing: 12

                Text {
                    text: "Weather Opacity"
                    color: "#f8fbff"
                    font.pixelSize: 14
                }

                Slider {
                    Layout.preferredWidth: 240
                    from: 0.1
                    to: 1.0
                    value: appController.weatherOverlayOpacity
                    onMoved: appController.weatherOverlayOpacity = value
                }

                Text {
                    text: Number(appController.weatherOverlayOpacity).toFixed(2)
                    color: "#35d7ff"
                    font.pixelSize: 14
                    font.bold: true
                }
            }
        }
    }

    Component {
        id: engineSection

        ColumnLayout {
            spacing: 12

            Text {
                text: "Engine & Airframe"
                color: "#f8fbff"
                font.pixelSize: 20
                font.bold: true
            }

            Text {
                text: "Fuel tank configuration drives both the strip display and the fuel-computer popup. Default is two tanks, Left / Right."
                color: "#c0d0dd"
                wrapMode: Text.WordWrap
                font.pixelSize: 14
            }

            RowLayout {
                spacing: 8

                Text {
                    text: "Tanks"
                    color: "#f8fbff"
                    font.pixelSize: 14
                }

                Repeater {
                    model: [1, 2, 3, 4]

                    delegate: Button {
                        required property int modelData
                        text: modelData.toString()
                        highlighted: appController.fuelTankCount === modelData
                        onClicked: appController.fuelTankCount = modelData
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Repeater {
                    model: appController.fuelTankStates

                    delegate: Rectangle {
                        required property var modelData

                        Layout.fillWidth: true
                        Layout.preferredHeight: 110
                        color: "#070b12"
                        border.color: "#355069"
                        border.width: 1
                        radius: 8

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 8

                            TextField {
                                Layout.fillWidth: true
                                text: modelData.label
                                horizontalAlignment: Text.AlignHCenter
                                onEditingFinished: appController.setFuelTankLabel(modelData.index, text)
                            }

                            Text {
                                text: Number(modelData.quantityGal).toFixed(1) + " / " + Number(modelData.capacityGal).toFixed(1) + " GAL"
                                color: "#f8fbff"
                                font.pixelSize: 13
                                horizontalAlignment: Text.AlignHCenter
                                Layout.fillWidth: true
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                height: 10
                                radius: 5
                                color: "#e5e8ee"
                                opacity: 0.18

                                Rectangle {
                                    width: parent.width * (Number(modelData.percent) / 100.0)
                                    height: parent.height
                                    radius: 5
                                    color: modelData.lowWarning ? "#ef4444"
                                           : modelData.lowCaution ? "#f5c66e"
                                           : "#22d13d"
                                }
                            }
                        }
                    }
                }
            }

            RowLayout {
                spacing: 10

                FlightSummaryCard {
                    title: "VS"
                    value: "50"
                    subtitle: "STALL"
                }

                FlightSummaryCard {
                    title: "VX"
                    value: "72"
                    subtitle: "BEST ANGLE"
                }

                FlightSummaryCard {
                    title: "VY"
                    value: "85"
                    subtitle: "BEST RATE"
                }
            }
        }
    }
}
