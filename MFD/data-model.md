# MFD Data Model

## Purpose

This document defines the normalized internal data model used by the `MFD` application.

The UI should never depend directly on raw `CAN FD`, serial, or USB messages. All external inputs should be translated into stable typed application models first.

## Modeling Rules

- each model carries data plus quality/state information
- raw transport format is not exposed to the UI
- timestamps are retained for stale-data handling
- units should be explicit and not implied
- missing and invalid are not the same state

## Common Quality State

Every live data item or grouped model should expose:

- `sourceId`
- `timestamp`
- `state`
- `faultFlags`

### State values

- `valid`
- `stale`
- `invalid`
- `notInstalled`
- `faulted`
- `demo`

## Core Models

## ModuleState

Used to represent discovered modules.

Suggested fields:

- `moduleId`
- `moduleType`
- `serialNumber`
- `firmwareVersion`
- `capabilities`
- `healthState`
- `lastHeartbeat`
- `isPresent`

## OwnshipNavData

Used for GPS/GNSS-derived navigation data.

Suggested fields:

- `latitudeDeg`
- `longitudeDeg`
- `gpsAltitudeFt`
- `groundSpeedKt`
- `groundTrackDeg`
- `utcTime`
- `fixType`
- `satellitesUsed`
- `horizontalAccuracyM`
- `verticalAccuracyM`
- quality state block

## FlightData

Used for ADC/AHRS-derived flight data.

Suggested fields:

- `pitchDeg`
- `rollDeg`
- `headingDeg`
- `turnRateDegPerSec`
- `slipSkidNormalized`
- `iasKt`
- `tasKt`
- `pressureAltitudeFt`
- `baroCorrectedAltitudeFt`
- `verticalSpeedFpm`
- `baroSettingInHg`
- `oatC`
- `densityAltitudeFt`
- `aoaNormalized`
- quality state block

## EngineData

Used for EIS-derived engine and electrical data.

Suggested fields:

- `rpm`
- `manifoldPressureInHg`
- `oilPressurePsi`
- `oilTemperatureF`
- `fuelPressurePsi`
- `fuelFlowGph`
- `fuelUsedGal`
- `fuelRemainingGal`
- `busVoltage`
- `busCurrentA`
- `chtF[]`
- `egtF[]`
- quality state block

### Channel metadata

Engine channels also need per-channel metadata:

- `label`
- `currentValue`
- `state`
- `alarmState`
- `greenRange`
- `yellowRange`
- `redRange`

## TrafficTarget

Used for one ADS-B or traffic target.

Suggested fields:

- `targetId`
- `latitudeDeg`
- `longitudeDeg`
- `relativeBearingDeg`
- `relativeDistanceNm`
- `altitudeFt`
- `relativeAltitudeFt`
- `trackDeg`
- `speedKt`
- `ageSec`
- `threatState`
- quality state block

## TrafficData

Used as the aggregate traffic model.

Suggested fields:

- `targets[]`
- `hasTrafficSource`
- `selectedTargetId`
- overall quality state block

## AlertItem

Used for warnings, cautions, and informational alerts.

Suggested fields:

- `alertId`
- `severity`
- `sourceModuleId`
- `title`
- `message`
- `isAcknowledged`
- `timestamp`
- `isVisible`

### Severity values

- `info`
- `caution`
- `warning`

## DisplayConfig

Used for persistent user and installation configuration.

Suggested fields:

- `displayRole`
- `displayClass`
- `baseChartSource`
- `supplementalOverlaySources`
- `brightnessMode`
- `brightnessLevel`
- `selectedPage`
- `layoutPreset`
- `unitPreferences`
- `demoModeEnabled`

## MapState

Used to represent chart selection and runtime map presentation state.

Suggested fields:

- `baseChartSource`
- `chartPackageId`
- `chartPackageRevision`
- `chartPackageState`
- `declutterLevel`
- `orientationMode`
- `rangeNm`
- `activeOverlays`
- `selectedWaypointId`
- `mapCenterLatitudeDeg`
- `mapCenterLongitudeDeg`
- quality state block

## DemoScenarioState

Used to control simulation/demo behavior.

Suggested fields:

- `scenarioId`
- `enabledModules`
- `playbackState`
- `simulationTimeScale`
- `failureOverrides`

## ApplicationState

Top-level state exposed to the UI.

Suggested fields:

- `moduleStates`
- `ownshipNavData`
- `flightData`
- `engineData`
- `trafficData`
- `activeAlerts`
- `displayConfig`
- `mapState`
- `demoScenarioState`

## Suggested C++ Structure

The application can model these as plain typed domain structures with change notification at the model/view boundary.

Recommended split:

- domain structs/classes in `C++`
- adapter/view-model layer for `QML`
- no raw bus parsing inside `QML`

## First Models Required For Coding

The first code milestone only needs:

- `ApplicationState`
- `DisplayConfig`
- `OwnshipNavData`
- `FlightData`
- `ModuleState`
- `AlertItem`
- `DemoScenarioState`

Then add:

- `EngineData`
- `TrafficData`

## Stale Data Rules

Each major model family should define:

- update cadence expectation
- stale threshold
- invalid threshold

The first implementation can start with family-level thresholds instead of per-field thresholds.

## Demo And Bench Integration

All demo data and all live bench-test data should populate the same normalized models.

That means:

- no demo-only widgets
- no bench-test-only page logic
- only the data source changes

## Open Data Model Decisions

- exact heading source precedence
- exact engine channel schema
- route/flight-plan model shape
- map viewport state structure
- whether replay/logging uses snapshots or event streams
