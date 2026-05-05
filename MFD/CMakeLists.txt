cmake_minimum_required(VERSION 3.21)

project(experimental_mfd_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 Multimedia Network WebSockets)

qt_standard_project_setup()

qt_add_executable(mfd_demo
    src/main.cpp
    src/appcontroller.cpp
    src/appcontroller.h
    src/chartpackagemanager.cpp
    src/chartpackagemanager.h
    src/modulelistmodel.cpp
    src/modulelistmodel.h
    src/navdatamanager.cpp
    src/navdatamanager.h
    src/weatherdatamanager.cpp
    src/weatherdatamanager.h
)

qt_add_qml_module(mfd_demo
    URI ExperimentalMfd
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/components/EngineStrip.qml
        qml/components/FlightSummaryCard.qml
        qml/components/HorizonView.qml
        qml/components/HsiCompass.qml
        qml/components/InstrumentTape.qml
        qml/components/MapPane.qml
        qml/components/MenuTile.qml
        qml/components/NumericKeypadPopup.qml
        qml/components/PageTabs.qml
        qml/components/PfdOverlayDisplay.qml
        qml/components/TrafficRadar.qml
        qml/components/TopBar.qml
        qml/pages/DemoPage.qml
        qml/pages/EnginePage.qml
        qml/pages/FullMapPage.qml
        qml/pages/PfdFocusPage.qml
        qml/pages/PfdMapSplitPage.qml
        qml/pages/SetupPage.qml
        qml/pages/SystemStatusPage.qml
        qml/pages/TrafficPage.qml
    RESOURCES
        qml/assets/icons/aircraft.svg
        qml/assets/icons/engine.svg
        qml/assets/icons/flight_log.svg
        qml/assets/icons/flight_plan.svg
        qml/assets/icons/gps.svg
        qml/assets/icons/setup.svg
        qml/assets/icons/status.svg
        qml/assets/icons/system_info.svg
        qml/assets/icons/timer.svg
        qml/assets/icons/tools.svg
        qml/assets/icons/track_log.svg
        qml/assets/icons/units.svg
        qml/assets/icons/vnav.svg
        qml/assets/icons/waypoint_flag.svg
        qml/assets/icons/weight_balance.svg
        qml/assets/audio/caution_ding.wav
        qml/assets/audio/warning_whoop.wav
)

set_target_properties(mfd_demo PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(mfd_demo
    PRIVATE
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::Multimedia
        Qt6::Network
        Qt6::WebSockets
)

target_compile_definitions(mfd_demo
    PRIVATE
        MFD_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

add_custom_command(TARGET mfd_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/data/reference_waypoints.json
        $<TARGET_FILE_DIR:mfd_demo>/reference_waypoints.json
)

install(TARGETS mfd_demo
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

install(FILES data/reference_waypoints.json
    DESTINATION bin
)
