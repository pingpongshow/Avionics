# MFD Interface Requirements

## Purpose

This document defines what the `MFD` expects from the rest of the system.

It is not the final bus protocol. It is the contract checklist the `ADC`, `EIS`, `ADSB`, and future modules need to satisfy so the MFD can consume their data reliably.

## Bus Assumption

- `Primary inter-module bus`: `CAN FD`
- `Optional service/prototype paths`: USB, UART, Ethernet

## General Rules For All External Data

Every data source consumed by the MFD should provide:

- source module ID
- source module type
- timestamp
- validity state
- stale timeout expectation
- units
- value
- fault code or reason if invalid

## Module Discovery Requirements

The MFD needs each module to report:

- module type
- module instance ID
- serial number or unique ID
- firmware version
- capability set
- heartbeat period
- health status

## Required MFD-Consumed Data By Module

## ADC/AHRS

### Required values

- pitch
- roll
- yaw/turn rate
- slip/skid
- IAS
- altitude
- vertical speed
- baro setting
- OAT

### Optional values

- TAS
- density altitude
- AOA
- pressure altitude separate from corrected altitude

### Required metadata

- alignment/calibration state
- sensor valid flag
- stale timeout
- source fault flags

## GPS/GNSS

The MFD may own GNSS locally, but if GNSS data is ever shared externally the same data contract should apply.

### Required values

- latitude
- longitude
- ground speed
- ground track
- UTC time

### Required metadata

- fix type
- satellites used
- estimated position accuracy
- valid flag

## EIS

### Required values for first engine page

- RPM
- oil pressure
- oil temperature
- fuel flow
- volts
- amps if available
- CHT per cylinder
- EGT per cylinder

### Optional values

- manifold pressure
- fuel pressure
- fuel quantity if provided by EIS
- outside engine-bay temperatures

### Required metadata

- red/yellow/green limits or state
- probe/channel fault flags
- stale timeout

## ADSB

### Required values

- target identifier
- target latitude/longitude or relative position
- target altitude
- target track
- target speed

### Required metadata

- age of target
- confidence/validity
- threat class if available

### Optional later

- weather products
- TIS/FIS style source typing

## System-Level Inputs

### Heartbeats

The MFD needs periodic heartbeat messages from all installed modules.

Required heartbeat contents:

- module ID
- health summary
- current mode
- internal fault bitmask

### Capability advertisement

Each module should declare what it supports so the MFD can enable or hide UI features.

Examples:

- AOA installed
- manifold pressure installed
- traffic available
- weather available
- CO sensor installed

## Commands The MFD May Need To Send

The MFD is not just a passive display. It may need to send configuration and control messages.

### ADC/AHRS commands

- set baro reference
- request calibration page action
- request zeroing procedure

### EIS commands

- acknowledge alarms
- reset fuel used
- select display units if distributed settings are used

### System commands

- identify module
- request version info
- request health snapshot
- reboot or service mode later if supported

## Data Freshness Requirements

The final protocol needs agreed timeouts by data family.

Suggested classes:

- `fast flight data`
- `navigation data`
- `engine data`
- `traffic data`
- `module health`

Each class should define:

- normal update rate
- stale threshold
- invalid threshold

## Layout-Relevant Capability Flags

The MFD needs these capability flags to decide what layout to show:

- `has_adc`
- `has_ahrs`
- `has_gps`
- `has_eis`
- `has_adsb`
- `has_aoa`
- `has_co_sensor`
- `has_fuel_totalizer`
- `has_secondary_display`

## Demo Mode Interface Expectations

The MFD should be able to replace any live module input with simulated data internally. That means the UI must depend on the normalized data model, not directly on raw bus traffic.

## Open Interface Decisions

- final CAN FD frame format
- whether messages are fixed binary structs or field-tagged
- time synchronization method
- redundancy / source-priority method
- whether any high-bandwidth data should bypass CAN FD
