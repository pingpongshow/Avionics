# ADSB Interface Specification

## Upstream Inputs

### Decoder / Receiver Feed

The ADSB module consumes:

- traffic updates
- raw `FIS-B` weather frames
- decoded weather text messages
- optional receiver status/health

## Downstream Outputs To MFD

### Heartbeat

- module type: `ADSB`
- firmware version
- receiver status
- `1090` path present/healthy
- `978` path present/healthy
- traffic count
- weather cache block count

### Traffic Messages

Each traffic target should provide:

- ICAO / target ID
- latitude / longitude
- altitude
- ground track
- ground speed
- vertical trend if available
- age
- alert / threat classification if available

### Weather Messages

For v1, publish normalized weather blocks:

- product ID
- block number
- scale factor
- north/west anchor
- block size
- intensity payload
- timestamp / age

This keeps the `MFD` independent of raw SDR formats.

## CAN FD Note

Weather payloads are too large for a single classic CAN frame. Use `CAN FD` segmentation for block payload transport.

