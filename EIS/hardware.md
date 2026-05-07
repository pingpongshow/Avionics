# EIS Hardware Specification

## Recommended v1 Hardware

- `MCU`: `STM32H743` / `STM32H753`
- `bus`: `CAN FD`
- `thermocouple front-end`: `MAX31856`
- `analog inputs`: oil pressure, manifold pressure, tank senders, voltage sense
- `frequency / pulse inputs`: RPM, fuel flow

## Probe Inputs

- `CHT`: 4 to 8 channels
- `EGT`: 4 to 8 channels
- `fuel flow`: hall / turbine pulse input
- `RPM`: tach pulse input
- `oil pressure`
- `oil temperature`
- `manifold pressure`
- `bus voltage`
- `alternator / shunt current`
- `fuel quantity`: `L/R` default, configurable

## Notes

- thermocouple wiring and grounding discipline matter
- provide sensor-fault detection per channel
- use isolated or carefully-filtered analog front ends where required

