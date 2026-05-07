# ADC Software Specification

## Core Functions

- convert pressure to `IAS`
- compute pressure altitude
- compute `VSI`
- compute optional `AOA`
- estimate pitch / roll from `IMU`
- publish fast flight data on `CAN FD`

## v1 Strategy

The firmware core in this directory is host-buildable and contains:

- pressure/airspeed math
- barometric altitude math
- complementary-filter attitude estimator
- `AOA` normalization

Hardware driver binding comes later.

