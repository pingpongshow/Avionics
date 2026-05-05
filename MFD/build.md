# MFD Build Strategy

## Purpose

This document defines how the `MFD` software should be built and exercised during early development.

The project now targets:

- `Installed target`: `CM5 + Yocto`
- `Bench integration target`: `Raspberry Pi 5 + Yocto`

The same `Qt 6 + C++ + QML` application should run on both.

## Build Targets

### 1. Installed target

- hardware: `Raspberry Pi CM5`
- OS: custom `Yocto` image
- runtime: fullscreen MFD application
- graphics mode: `EGLFS` first

### 2. Bench integration target

- hardware: `Raspberry Pi 5`
- OS: custom `Yocto` image
- runtime: same fullscreen MFD application
- inputs: touchscreen preferred, mouse/keyboard fallback
- interfaces: live `CAN FD`, live `GNSS`, optional mixed live/simulated sources

### 3. Optional host simulator target

- hardware: development workstation
- purpose: fast UI iteration
- notes: useful later, but not required if `Raspberry Pi 5` is the preferred live bench platform

## Build Principles

- one application codebase
- one primary embedded architecture
- as few target-specific code paths as possible
- runtime configuration chooses display class and demo/live mode
- hardware access is isolated behind interfaces

## Recommended Source Tree Direction

The exact tree can evolve, but the application should eventually separate:

- `src/app`
  main entry, startup, app controller
- `src/platform`
  hardware/platform abstractions
- `src/domain`
  normalized data model
- `src/services`
  GNSS, module manager, demo manager, logging
- `src/ui`
  QML pages, widgets, themes, layout definitions
- `assets`
  fonts, icons, demo data, map placeholders
- `cmake`
  build support

## Yocto Integration Direction

The application should be packaged as part of the Yocto image from the beginning.

### Image assumptions

- minimal userspace
- `systemd`
- Qt 6 runtime
- application autostart
- persistent writable storage for config and logs

### Recipe assumptions

- application recipe for the MFD app
- explicit Qt dependencies
- explicit fonts and asset packaging
- optional demo assets bundled with the image

## Development Workflow

### Early workflow

1. build the application
2. deploy to `Raspberry Pi 5`
3. run in `Demo Mode`
4. iterate on layouts and interaction
5. connect live modules as they become available

### Installed-target workflow later

1. build Yocto image for `CM5`
2. deploy to CM5 target hardware
3. validate display, touch or controls, GNSS, and module integration

## What Must Exist Before Coding Starts

- chosen Qt major version: `Qt 6`
- chosen language split: `C++ backend + QML UI`
- chosen graphics path: `EGLFS`
- chosen primary display baseline: `10.1" 1280x800`
- chosen bench target: `Raspberry Pi 5`
- normalized data model contract
- initial page/layout definitions

## Suggested First Milestones

### Milestone 1

- Yocto image boots on `Raspberry Pi 5`
- application starts fullscreen
- touch input works
- mouse/keyboard fallback works
- `Demo Mode` can switch between pages

### Milestone 2

- `PFD + Map Split` page
- `PFD Focus` page
- chart-source selection in setup
- packaged aviation-chart preview path
- top bar and alert strip

### Milestone 3

- data model plumbing
- module simulation toggles
- scenario presets
- stale/invalid visual states

### Milestone 4

- local GNSS integration on `Raspberry Pi 5`
- live ownship position on packaged chart layers

### Milestone 5

- first live `CAN FD` interface on `Raspberry Pi 5`
- live module discovery and status page
- mixed live and simulated source handling

### Milestone 6

- deploy the same software stack to `CM5`
- verify equivalent behavior on installed-target hardware

## Demo Mode Build Requirements

The demo build must not depend on external avionics modules.

It should work with:

- `Raspberry Pi 5`
- HDMI display or touchscreen
- mouse and keyboard if needed
- optional local GNSS

## Bench Integration Requirements

The bench integration build must also support:

- live `CAN FD` interface
- live `GNSS`
- offline chart packages stored locally
- attached external modules
- mixed live and simulated sources
- status/diagnostics pages for module testing

## Platform Abstraction Requirements

The following should be hidden behind platform/service interfaces:

- touch and pointer input
- GNSS source
- CAN interface
- persistent storage paths
- display brightness control
- service/provisioning hooks

This keeps the same UI and domain code usable on both `CM5` and `Raspberry Pi 5`.

## Build Open Items

- exact Yocto layers to adopt for the project
- whether host simulator support is worth adding immediately
- exact deployment method to `Raspberry Pi 5`
- exact offline chart-package layout on persistent storage
- whether `FAA` charts are stored as imported tiles, native rasters, or both
