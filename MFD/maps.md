# MFD Map Sources

## Purpose

This document defines the real aviation chart sources and packaging strategy for the `MFD`.

The goal is to avoid building the MFD around a generic street-map stack or a third-party website dependency.

## Source Policy

### U.S. base charts

- use official `FAA` digital chart products as the primary U.S. source
- target `VFR` chart products first
- support `Sectional` and `TAC` first
- treat FAA edition dates and effective dates as first-class metadata

### Non-U.S. regional base charts

- use `OpenFlightMaps` where the project has regional coverage
- prefer packaged offline formats that are already close to embedded use

### Supplemental overlay data

- `openAIP` may be used as a supplemental overlay source
- use it for airports, airspace, frequencies, or other optional overlays as appropriate
- do not treat it as the primary certified navigation source
- keep the license constraints visible in the project decision record

### Open-source and noncommercial fit

- the project is intended to be released as `open source`
- the current intended use is `noncommercial`
- that makes `OpenFlightMaps` and `openAIP` easier to integrate from a project-license perspective than they would be for a commercial closed product
- `openAIP` license constraints still matter for downstream commercial reuse, so its use should remain isolated to optional overlays and clearly documented

### Sources not to depend on at runtime

- do not depend on `VFRMap` as the MFD backend chart source
- do not require an internet connection to display charts in the aircraft
- do not build the chart engine around scraping or embedding third-party websites

## First Implementation Direction

The first implementation should use offline packaged raster aviation charts.

Preferred first steps:

- import `FAA` `GeoTIFF` VFR charts into a local chart package for U.S. demo and bench use
- support `OpenFlightMaps` `MBTiles` or `GeoTIFF` packages for covered regions
- add local ownship, route, waypoint, and traffic overlays on top of the chart
- keep the renderer focused on chart display and avionics overlays, not generic GIS features

## Why This Direction

- `FAA` charts are the correct primary U.S. source for a U.S.-based project
- `OpenFlightMaps` is explicitly structured for reuse in applications and embedded products
- `openAIP` is useful for supplemental data, but should not drive the entire chart strategy
- `VFRMap` is better treated as a visual/function reference than as a data integration dependency

## Packaging Requirements

Chart packages should support:

- offline use
- region-based installation
- version/effective-date tracking
- fast startup
- fast pan/zoom in the visible map window
- simple replacement when a chart cycle changes

## Storage Direction

Chart assets should live outside the read-only root filesystem.

Suggested writable storage layout:

- `/data/charts/us/faa/`
- `/data/charts/ofm/`
- `/data/overlays/openaip/`
- `/data/config/`
- `/data/logs/`

The exact paths can change later, but chart packages must be independently updateable.

## Bench And Demo Expectations

The `Raspberry Pi 5` bench target should be able to:

- boot the same MFD application
- load local chart packages
- use live `GNSS`
- connect live modules over `CAN FD`
- keep `Demo Mode` available when no modules are attached

## Open Items

- exact `FAA` import toolchain
- exact chart package format used by the renderer
- whether the first renderer reads imported tiles, native rasters, or both
- how `openAIP` overlay refresh is handled
- how chart cycle updates are installed on the target
