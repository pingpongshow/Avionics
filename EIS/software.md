# EIS Software Specification

## v1 Architecture

- deterministic sensor acquisition loop
- limit evaluation loop
- fuel totalizer state
- periodic `CAN FD` publication

## Core Functions

- convert raw inputs into engineering units
- compute tank totals and fuel used
- evaluate `white / green / yellow / red` ranges
- publish channel validity and alert state

## Bench Strategy

The firmware core in this directory is host-buildable. That lets the math and alert logic be tested before binding to STM32 HAL drivers.

