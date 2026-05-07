# ADSB Software Specification

## Purpose

The software stack turns raw received traffic/weather into normalized outputs for the rest of the system.

## Recommended Stack

- `OS`: Linux
- `decode source`: `Stratux`-compatible decode stream, or equivalent local receiver stack
- `bridge daemon`: Python service for v1
- `bus output`: `SocketCAN`

## Runtime Responsibilities

- connect to decoded traffic/weather feed
- consume raw `jsonio` frames and text weather
- decode `UAT` weather blocks
- maintain a target table
- maintain recent weather block cache
- publish heartbeat/health over `CAN FD`
- publish traffic/weather updates over `CAN FD`

## Why Python For v1

- easier to iterate on parsing and protocol handling
- direct access to `SocketCAN` via Python `socket`
- easy integration with websocket feeds
- good fit for a Linux bridge service

This is not the recommendation for `ADC`, `EIS`, or `MAG`. It is specific to this Linux RF/decode node.

## External Dependencies

- `websocket-client`

Everything else in the bridge uses the Python standard library.

