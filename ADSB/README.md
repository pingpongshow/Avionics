# ADSB Module

## Purpose

The `ADSB` module is an `ADS-B In / FIS-B In` receiver and bridge node for the modular avionics system.

Its job is to:

- receive `1090ES` and `978 UAT` traffic
- receive `FIS-B` weather/text products
- normalize traffic/weather into an internal model
- publish module health plus traffic/weather over `CAN FD` to the `MFD`

This module is intentionally `ADS-B In only`.

`ADS-B Out` is excluded from this open experimental module path because installed `ADS-B Out` equipment and configuration are subject to FAA performance and installation requirements under `14 CFR 91.225 / 91.227`, and that should be treated as a separate certified/compliance-driven workstream.

## Recommended Architecture

Use a `Linux SBC` for the ADSB module, not a small MCU.

Reason:

- SDR demodulation and weather/traffic decode are Linux-class workloads
- existing open-source stacks already solve much of the RF and decode problem
- the module can still present a clean `CAN FD` interface to the rest of the aircraft system

## Recommended v1 Stack

- `Compute`: `Raspberry Pi 4B` or `CM4`
- `Decode stack`: `Stratux`-style open source pipeline or equivalent
- `Bridge daemon`: local process that reads decoded traffic/weather and publishes `CAN FD`
- `Bus`: `CAN FD`

## Directory Layout

- [hardware.md](/Users/stephen/Desktop/PFD/ADSB/hardware.md)
- [software.md](/Users/stephen/Desktop/PFD/ADSB/software.md)
- [interfaces.md](/Users/stephen/Desktop/PFD/ADSB/interfaces.md)
- [bridge/bridge.py](/Users/stephen/Desktop/PFD/ADSB/bridge/bridge.py)

