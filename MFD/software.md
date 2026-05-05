# MFD Software Specification

## Purpose

The `MFD` software is the user-facing avionics application stack running on the display head.

It must:

- render the selected display layout
- host the moving map
- integrate all external module data
- handle faults and stale data correctly
- support simulation and `Demo Mode`

## Official Software Target

- `OS`: `custom Yocto Linux image`
- `init / service manager`: `systemd`
- `UI`: `Qt 6 / QML`
- `backend`: `C++`
- `graphics`: GPU-accelerated Qt scenegraph
- `primary platform plugin`: `EGLFS`
- `windowing upgrade path`: `Wayland` only if multi-process UI becomes necessary
- `device interfaces`: Linux CAN, UART, USB, filesystem, GPIO/evdev
- `build model`: cross-compiled from the Yocto SDK/toolchain

This project officially targets a `Yocto-first` production-style embedded stack rather than a `Raspberry Pi OS Lite` prototype-first stack.

## Supported Runtime Targets

- `Installed target`: `CM5`
- `Bench integration target`: `Raspberry Pi 5`

The same `Qt 6 + C++ + QML` application should run on both targets.

## Software Architecture Rules

- the MFD should boot directly into the avionics application
- the MFD should not depend on a desktop environment
- the MFD should not depend on Raspberry Pi desktop packages or ad hoc distro services
- the MFD application should be one dedicated fullscreen embedded app in v1
- the `Qt 6 + C++ + QML` codebase should remain portable across Linux images by using standard Linux and Qt APIs
- the software should support both live module inputs and simulated inputs on either target

## Why Yocto-first

Using `Yocto` from the beginning is the preferred path for this project because it gives:

- controlled package contents
- reproducible builds
- cleaner boot-to-app behavior
- easier read-only rootfs design
- easier future A/B update strategy
- better long-term ownership of the deployed image

It also avoids having to migrate the product from one Linux distribution model to another after core application behavior is already established.

## Software Layers

### 1. Platform layer

Responsibilities:

- boot
- boot directly to the MFD application
- watchdog
- power-fail handling
- storage mounting
- service mode entry
- software version reporting
- persistent config handling
- image/recovery state reporting

Preferred platform characteristics:

- read-only or mostly read-only rootfs strategy
- writable persistent partition for config/logs
- minimal userspace
- deterministic startup ordering

### 2. Hardware abstraction layer

Responsibilities:

- CAN FD interface
- GNSS interface
- button and encoder input
- backlight control
- buzzer or alert output if present
- log writing

### 3. Data integration layer

Responsibilities:

- module discovery
- message parsing
- timestamp handling
- stale-data detection
- validity state propagation
- unit normalization
- source capability tracking

### 4. Domain model

Responsibilities:

- ownship state
- flight state
- engine state
- traffic state
- system health state
- user settings

This layer should present the UI with stable typed values instead of raw bus messages.

### 5. UI/layout layer

Responsibilities:

- page routing
- adaptive layouts
- widget composition
- color themes
- annunciations
- alert stack

### 6. Demo/simulation layer

Responsibilities:

- synthetic sources
- scenario presets
- module-presence simulation
- playback / log replay later

## Core Runtime Features

### Application model

The first production-oriented software model should be:

- one dedicated fullscreen `Qt 6` application
- `C++` for backend logic and hardware/data integration
- `QML` for UI composition and layout behavior

Do not split into multiple UI processes unless a clear requirement appears. If that requirement appears later, the migration path is:

- `Wayland`
- optional `Qt Application Manager`

### Adaptive layout engine

The software must:

- determine available modules
- select a default layout based on module set
- allow the user to override with saved presets
- degrade gracefully when a module disappears or goes stale

### Map/navigation engine

The software must:

- render the map locally
- use real aviation chart sources rather than a generic road/street map
- overlay ownship
- support pan/zoom
- support route and waypoint overlays
- support traffic overlays
- support declutter levels

The first implementation should prefer packaged offline raster aviation charts over a large custom GIS stack.

Recommended source split:

- U.S. base charts from official `FAA` VFR chart packages
- regional base charts from `OpenFlightMaps` where coverage exists
- optional `openAIP` overlay data for airports, airspace, and points of interest where licensing fits the intended use
- no dependency on `VFRMap` as a runtime data source

The initial map renderer should be built around a chart-package abstraction rather than a web-service abstraction.

Recommended first package formats:

- `GeoTIFF` import pipeline for `FAA` charts
- `MBTiles` and `GeoTIFF` support for `OpenFlightMaps`
- local cached overlay datasets for optional `openAIP` supplemental layers

### Alerting

Required behaviors:

- separate caution from warning states
- visually identify stale or invalid data
- keep alerts visible across page changes
- allow acknowledgement where appropriate

### Logging

The software should log:

- module discovery
- module loss
- GNSS fix state
- warnings/cautions
- configuration changes
- major user actions

### Deployment/runtime requirements

The deployed image should:

- start the MFD application from `systemd`
- restart the application if it exits unexpectedly
- preserve logs and configuration across image replacement
- keep development/debug tooling separate from the production runtime as much as practical

On the `Raspberry Pi 5` bench target, the same application model should support:

- fullscreen demo mode
- local touch or pointer input
- live `CAN FD` interfaces
- live GNSS interfaces
- attachment of real external modules for bench testing

## Input Handling

The first software baseline should support touch-first interaction with pointer/keyboard fallback.

### Inputs

- touchscreen events
- mouse/pointer events
- keyboard shortcuts for bench testing
- later encoder events
- later soft-key events

### Interaction rules

- all demo and bench-critical functions must be accessible by touch
- all demo and bench-critical functions must also be accessible by mouse/keyboard when touch is unavailable
- touch targets must be intentionally sized for cockpit-style use
- focus/cursor state must always be obvious on screen
- later physical controls should map onto the same command/action layer rather than page-specific code

## Page Set

The first MFD software should support these pages:

- `PFD Focus`
- `PFD + Map Split`
- `Full Map`
- `Engine`
- `Traffic`
- `System Status`
- `Setup`
- `Demo Mode`

## Fault Handling

### Required stale-data behavior

- every major data family needs a stale timeout
- stale air-data must not look identical to valid air-data
- missing GPS must not invalidate working PFD data
- missing ADC/AHRS must disable attitude-derived pages
- missing EIS must collapse engine regions cleanly

### Required state classes

- `valid`
- `stale`
- `invalid`
- `not installed`
- `faulted`

## Demo Mode

`Demo Mode` should exist from the first software milestone.

### Required capabilities

- synthetic GPS-only map mode
- synthetic PFD data
- synthetic engine data
- synthetic traffic
- manual module enable/disable
- scenario presets
- visible `DEMO` banner or watermark

### Suggested scenarios

- cold boot idle
- taxi
- takeoff/climb
- cruise
- descent/approach
- engine caution
- traffic conflict
- module failure

## Configuration And Persistence

The software should persist:

- display role
- brightness settings
- last page or default startup page
- user layout presets
- unit preferences
- demo mode preferences

Persistent application data should live in a dedicated writable location, not inside the root filesystem.

## Suggested Internal Modules

- `AppController`
- `ModuleManager`
- `CanBusInterface`
- `GnssInterface`
- `OwnshipModel`
- `FlightDataModel`
- `EngineDataModel`
- `TrafficModel`
- `AlertManager`
- `LayoutManager`
- `MapViewModel`
- `DemoManager`
- `SettingsStore`

## Initial Development Milestones

### Milestone 1

- Yocto image boots
- application starts from `systemd`
- display renders
- touch input works
- mouse/keyboard fallback works
- `Demo Mode` basic pages

### Milestone 2

- local GNSS ingest
- full map page
- ownship display

### Milestone 3

- `ADC/AHRS` data ingest
- PFD widgets
- stale-data handling

### Milestone 4

- `EIS` integration
- engine page and engine strip

### Milestone 5

- `ADSB` integration
- traffic page and overlays

### Milestone 6

- role switching
- secondary display behavior
- reversionary layouts

## Build And Integration Strategy

The application should be developed as a normal `Qt 6 + C++ + QML` codebase, but the official target environment should be the `Yocto` cross-compiled runtime from the beginning.

Practical rules:

- do not rely on `apt` packages as part of the product definition
- keep third-party dependencies explicit in Yocto recipes/layers
- keep all hardware/service assumptions documented in the image build
- test on the same graphics/runtime model intended for the product target

## Software Open Items

- exact chart renderer implementation
- `FAA GeoTIFF` import pipeline details
- `OpenFlightMaps` package import path details
- whether logs are plain text, binary, or both
- whether app is single-process or split into backend/UI processes
- exact startup-time target
