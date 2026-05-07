# MAG Software Specification

## Core Functions

- read raw magnetic vector
- apply hard-iron bias
- apply soft-iron correction matrix
- perform tilt-compensated heading calculation with supplied pitch/roll
- publish calibrated vector + health over `CAN FD`

## v1 Strategy

The firmware core in this directory is host-buildable so calibration and heading math can be validated before hardware-driver integration.

