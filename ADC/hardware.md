# ADC Hardware Specification

## Recommended v1 Hardware

- `MCU`: `STM32H743` / `STM32H753`
- `IMU`: `ADIS16500`-class primary sensor
- `pressure sensors`: pitot/static and optional `AOA` differential sensors
- `bus`: `CAN FD`

## Inputs

- pitot pressure
- static pressure
- optional `AOA` differential pressure
- accelerometer / gyro
- optional temperature sensor

## Notes

- keep the `ADC` and remote `MAG` separate
- provide clean vibration mounting and thermal behavior
- use deterministic sampling and explicit sensor-validity handling

