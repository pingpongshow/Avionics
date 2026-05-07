# MAG Hardware Specification

## Recommended v1 Hardware

- `MCU`: low-power `STM32G4` or `STM32H7` family member
- `sensor`: `RM3100`-class remote magnetometer
- `bus`: `CAN FD`

## Mounting Goal

Mount the module remotely from:

- displays
- DC/DC converters
- alternator/starter cabling
- ferrous structure where avoidable

## Inputs / Outputs

- magnetic field vector
- temperature optional
- calibrated heading vector output to `ADC/MFD`

