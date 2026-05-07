# CAN FD Protocol

This document defines the first concrete inter-module bus contract for the project.

- Bus: `CAN FD`
- Frame byte order: `little-endian`
- Default transport: Linux `SocketCAN`
- Frame format: fixed binary payloads, one message type per CAN ID
- Scope: `ADC`, optional external `GNSS`, `EIS`, `MAG`, `ADSB`, and `MFD`

## Message IDs

- `0x101` `ADC` heartbeat
- `0x102` external `GNSS` heartbeat
- `0x103` `EIS` heartbeat
- `0x104` `MAG` heartbeat
- `0x105` `ADSB` heartbeat
- `0x110` `ADC` flight / air-data frame
- `0x150` external `GNSS` nav frame
- `0x210` `EIS` summary frame
- `0x211` `EIS` cylinder frame
- `0x212` `EIS` fuel tank frame
- `0x310` `MAG` heading frame
- `0x510` `ADSB` traffic target frame
- `0x520` `ADSB` weather block header
- `0x521` `ADSB` weather block payload chunk

## Heartbeat

Payload layout:

- `u8 moduleType`
- `u8 instanceId`
- `u8 health`
- `u8 mode`
- `u32 capabilityFlags`
- `u32 counterA`
- `u32 counterB`

Definitions:

- `moduleType`: `1 ADC`, `2 GNSS`, `3 EIS`, `4 MAG`, `5 ADSB`
- `health`: `0 unknown`, `1 healthy`, `2 degraded`, `3 failed`
- `mode`: `0 offline`, `1 boot`, `2 demo`, `3 live`

Recommended `counterA/counterB` uses:

- `ADC`: alignment state, fault bitmask
- `EIS`: channel fault bitmask, active caution/warning count
- `ADSB`: traffic target count, weather block count
- `MAG`: calibration state, saturation/fault bitmask

## ADC Flight Frame `0x110`

- `float pitchDeg`
- `float rollDeg`
- `float headingDeg`
- `float turnRateDegPerSec`
- `float slipSkidNormalized`
- `float iasKt`
- `float tasKt`
- `float aoaNormalized`
- `float altitudeFt`
- `float verticalSpeedFpm`
- `float baroSettingInHg`

## External GNSS Nav Frame `0x150`

- `s32 latitudeE6`
- `s32 longitudeE6`
- `u16 groundSpeedTenthKt`
- `u16 groundTrackTenthDeg`
- `u32 utcSecondsOfDay`
- `u8 fixType`
- `u8 satellitesUsed`
- `u16 horizontalAccuracyTenthMeters`

## EIS Summary Frame `0x210`

- `u16 rpm`
- `u16 manifoldPressureTenthInHg`
- `u16 oilPressureTenthPsi`
- `u16 oilTemperatureF`
- `u16 fuelFlowTenthGph`
- `u16 busVoltageTenthV`
- `s16 busCurrentTenthA`
- `u16 fuelRemainingTenthGal`
- `u16 fuelUsedTenthGal`
- `u16 chtMaxF`
- `u16 egtMaxF`

## EIS Cylinder Frame `0x211`

- `u16 chtF[6]`
- `u16 egtF[6]`
- `u8 cylinderCount`

For engines with fewer than `6` cylinders, unused entries are `0`.

## EIS Fuel Tank Frame `0x212`

- `u8 tankIndex`
- `u8 tankCount`
- `u16 quantityTenthGal`
- `u16 capacityTenthGal`
- `u8 lowCaution`
- `u8 lowWarning`
- `char label[8]`

One frame is sent per tank.

## MAG Heading Frame `0x310`

- `float magneticHeadingDeg`
- `float trueHeadingDeg`
- `float declinationDeg`
- `u8 calibrated`

## ADSB Traffic Frame `0x510`

- `char identifier[8]`
- `s32 latitudeE6`
- `s32 longitudeE6`
- `s32 altitudeFt`
- `u16 trackTenthDeg`
- `u16 groundSpeedKt`
- `s16 verticalSpeedFpm`
- `u8 alertLevel`
- `u8 ageSeconds`

## ADSB Weather Transport

Raw `FIS-B` NEXRAD blocks are segmented over CAN FD.

### Header `0x520`

- `u8 productId`
- `u8 scaleFactor`
- `u32 blockNumber`
- `s16 latNorthHundredthsDeg`
- `s16 lonWestHundredthsDeg`
- `s16 latHeightHundredthsDeg`
- `s16 lonWidthHundredthsDeg`
- `u16 intensityCount`

### Payload `0x521`

- `u32 blockNumber`
- `u16 offset`
- `u8 intensity[58]`

Rules:

- A header starts a new block assembly.
- Payload chunks may arrive in multiple frames.
- Payloads are matched by `blockNumber`.
- A block is complete when `intensityCount` bytes have been received.

## Freshness

- `ADC` flight data: nominal `20-50 Hz`, stale at `250 ms`, invalid at `1000 ms`
- `GNSS` nav: nominal `5-10 Hz`, stale at `1000 ms`, invalid at `3000 ms`
- `EIS` summary: nominal `5-10 Hz`, stale at `1000 ms`, invalid at `3000 ms`
- `ADSB` traffic: use `ageSeconds` from source and remove stale targets after `30 s`
- `ADSB` weather: age by product TTL in the receiver / MFD cache

## v1 Limits

- Single logical instance per module type is assumed by the current `MFD`
- `ADSB Out` is intentionally out of scope
- High-bandwidth raster chart data does not travel over `CAN FD`
