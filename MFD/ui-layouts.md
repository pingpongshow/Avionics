# MFD UI Layouts

## Purpose

This document defines the first layout classes and page compositions for the `MFD`.

The goal is not to copy an existing vendor display exactly. The goal is to capture proven layout patterns from the reference displays and turn them into a modular, testable UI system.

## Display Size Strategy

The MFD should support a small number of explicit display classes instead of arbitrary free scaling.

### Supported display classes

- `Primary`
  `10.1"` landscape, baseline `1280x800`
- `Compact`
  `7"` class, landscape, typically around `1024x600`
- `Standby`
  `5"` class for reduced-layout or role-specific pages

### Design rule

All first-pass layout work should start with the `Primary` class.

The `Compact` and `Standby` classes should reuse the same widget library, but they do not need to preserve every pane or every supplemental detail from the `Primary` class.

## Layout Principles

- flight-critical information gets the most stable and central region
- optional panes collapse cleanly when the source module is absent
- map, engine, and traffic are modular panes, not hard-coded assumptions
- layouts switch by `display class`, `page mode`, and `installed module set`
- touch targets must be large enough for reliable bench and demo use

## Reference Patterns From Example Displays

The provided example displays show several patterns worth supporting:

- dedicated PFD with a large central attitude view
- split-screen PFD and map
- side engine strips
- bottom engine strips
- full-page map mode
- reduced/standby attitude layouts
- popups and overlays for secondary tasks

The layout system should support these patterns as configurable compositions.

## Initial Page Set

- `PFD Focus`
- `PFD + Map Split`
- `PFD + Map + Engine`
- `Full Map`
- `Engine`
- `Traffic`
- `System Status`
- `Setup`
- `Demo Mode`

## Standard Layout Regions

- `TopBar`
  radios, nav summary, waypoint, distance, groundspeed, track, alerts, mode tags
- `LeftSidebar`
  engine strip or supplemental data region
- `CenterPrimary`
  main attitude/PFD content
- `RightPane`
  map, traffic, engine detail, or flight-plan pane
- `BottomStrip`
  status, soft-key labels, compact system data, or compact engine row
- `OverlayLayer`
  dialogs, menus, warnings, map options, module status, and demo controls

## Primary Display Class

### `PFD Focus`

- large central attitude display
- airspeed tape left
- altitude/VSI tape right
- compact HSI lower center
- top status bar
- optional compact engine strip

Use when:

- `ADC/AHRS` is present
- flight data is the primary task

### `PFD + Map Split`

- PFD on left or center-left
- map on right
- HSI integrated in the lower PFD region
- top status bar
- bottom status/soft-key strip

Use when:

- `GPS` and `ADC/AHRS` are present

This should be the first major layout implemented.

### `PFD + Map + Engine`

- PFD remains dominant
- map reduced to a narrower pane
- engine strip on left or bottom
- engine region collapses cleanly if `EIS` is absent

Use when:

- `EIS` is present
- user wants an all-data one-screen view

### `Full Map`

- map takes most of the screen
- compact PFD or nav summary overlaid or docked
- route, waypoint, alerts, and traffic remain visible

Use when:

- navigation emphasis is primary

### `Engine`

- large engine strip/bar area
- compact nav or PFD inset
- warning/caution presentation prioritized

Use when:

- `EIS` is present

### `Traffic`

- map/traffic pane dominant
- compact ownship/PFD summary
- target list or detail area

Use when:

- `ADSB` is present

## Compact Display Class

The `Compact` class should support:

- `PFD Focus`
- `PFD + Map Split`
- `Full Map`
- `Engine`

Rules:

- reduce secondary text density
- collapse sidebars sooner
- keep touch targets larger, not smaller
- avoid overly dense bottom strips

## Standby Display Class

The `Standby` class should support:

- reduced `PFD Focus`
- reduced map/repeater page only if readable

Rules:

- no clutter-heavy split layouts
- no dense engine grids
- no nonessential overlays during normal operation

## Module-Driven Layout Behavior

### `GPS` only

- `Full Map`
- `System Status`
- `Setup`
- `Demo Mode`

### `GPS + ADC/AHRS`

- enable `PFD Focus`
- enable `PFD + Map Split`

### `GPS + ADC/AHRS + EIS`

- enable `PFD + Map + Engine`
- enable `Engine`

### `GPS + ADC/AHRS + ADSB`

- enable traffic overlays
- enable `Traffic`

### Missing modules

- no empty reserved panes
- no labels for nonexistent functions
- replace missing-module panes with a cleaner expanded layout

## Input Model For Layout Work

The first layout baseline is touch-first.

### Touch behavior

- top-bar items may open detail popups
- panes should have obvious touch focus
- basic tap controls are enough for the first milestone
- map gestures can be added later if needed

### Mouse/keyboard fallback

- mouse should emulate touch selection cleanly
- keyboard shortcuts should exist for page switching, demo control, and setup during development

## Popup And Overlay System

The MFD should support overlays for:

- module status
- configuration dialogs
- map options
- traffic details
- alerts/warnings
- demo controls

Overlays should not permanently alter the base page composition.

## Demo Mode Layout Requirements

`Demo Mode` should be able to drive all major layouts without requiring live modules.

Required states:

- GPS only
- full PFD
- PFD plus map
- PFD plus map plus engine
- traffic overlay
- module-failure degraded page

`DEMO` marking must remain visible in all demo pages and overlays.

## Recommended First Implementation Order

1. `PFD + Map Split`
2. `PFD Focus`
3. `Full Map`
4. `System Status`
5. `Demo Mode controls`
6. `PFD + Map + Engine`
7. `Engine`
8. `Traffic`

## Open Layout Decisions

- exact top-bar information density
- whether the first engine strip is left-side or bottom-first
- whether the initial HSI is circular, arc, or compact hybrid
- whether the compact class uses the same page names or a reduced role-specific set
