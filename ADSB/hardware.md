# ADSB Hardware Specification

## v1 Goal

Build a dedicated `ADS-B In / FIS-B In` node that can feed:

- traffic
- FIS-B text/weather
- module heartbeat/health

to the `MFD` over `CAN FD`.

## Recommended Hardware

### Compute

- `Raspberry Pi 4B 4GB`, or
- `CM4` on a custom carrier for an installed version

Reason:

- enough CPU for `978 UAT` + `1090ES`
- broad Linux support
- easy USB/SPI/CAN integration

### RF Front End

- dual-receiver architecture preferred:
  - `978 MHz UAT`
  - `1090 MHz ES`
- use known-good SDR-compatible front ends used by the `Stratux` ecosystem, or a supported integrated receive board

### CAN Interface

- `MCP2518FD` over SPI, or
- industrial USB-to-CAN FD adapter if Linux support is clean

### GNSS

- optional local GNSS for receiver timing and ownship-quality metadata
- the `MFD` still keeps its own GNSS path

### Storage

- modest local storage for logs and temporary weather/traffic caches
- eMMC preferred on CM4
- SD acceptable for bench/early prototype

### Power

- aircraft bus input protection
- filtered DC/DC
- clean shutdown hold-up or supervised power-fail input

### Antennas / RF Notes

- separate `978` and `1090` antennas preferred
- RF layout and antenna placement matter more than code quality here
- do not package this module near noisy display DC/DC hardware if avoidable

## Installed vs Bench

### Bench

- Pi 4B
- USB SDRs
- USB or SPI CAN FD
- lab power

### Installed

- CM4 carrier
- fixed RF connectors
- enclosure shielding
- filtered power
- service USB
- CAN FD on locking connector

