# ADC Module

## Purpose

The `ADC` module is the `Air Data Computer / AHRS` node.

It owns:

- pitot/static sensing
- altitude / VSI
- airspeed
- optional `AOA`
- `IMU` attitude estimation

and publishes normalized flight data over `CAN FD`.

## Directory Layout

- [hardware.md](/Users/stephen/Desktop/PFD/ADC/hardware.md)
- [software.md](/Users/stephen/Desktop/PFD/ADC/software.md)
- [interfaces.md](/Users/stephen/Desktop/PFD/ADC/interfaces.md)
- [firmware/CMakeLists.txt](/Users/stephen/Desktop/PFD/ADC/firmware/CMakeLists.txt)

