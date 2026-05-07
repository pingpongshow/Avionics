# EIS Module

## Purpose

The `EIS` module is the dedicated `Engine Information System` node.

It owns engine probes and fuel-totalizer state, then publishes normalized engine data over `CAN FD`.

## Responsibilities

- RPM
- manifold pressure
- oil pressure
- oil temperature
- fuel flow
- fuel quantity per tank
- bus voltage / current
- `CHT` per cylinder
- `EGT` per cylinder
- caution/warning evaluation against configured ranges

## Directory Layout

- [hardware.md](/Users/stephen/Desktop/PFD/EIS/hardware.md)
- [software.md](/Users/stephen/Desktop/PFD/EIS/software.md)
- [interfaces.md](/Users/stephen/Desktop/PFD/EIS/interfaces.md)
- [firmware/CMakeLists.txt](/Users/stephen/Desktop/PFD/EIS/firmware/CMakeLists.txt)

