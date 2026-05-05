# MFD Hardware Specification

## Module Purpose

The `MFD` hardware is the display-compute node for the system.

It should:

- host the main screen and front-panel controls
- run the map and display software
- accept local `GPS/GNSS`
- communicate with other modules over `CAN FD`
- remain reusable as a primary display, map repeater, engine display, or standby display

## Official Platform Target

### Compute

- `Primary recommendation`: `Raspberry Pi CM5`
- `Supported bench integration target`: `Raspberry Pi 5`
- `Official OS target`: `custom Yocto Linux image`
- `RAM target`: `8 GB` minimum
- `Preferred storage`: eMMC or NVMe
- `Installed-use rule`: do not depend on microSD as the primary installed system storage

Reason:

- enough CPU/GPU headroom for split-screen avionics UI
- same platform can be reused across multiple display roles
- supports a production-oriented software path from the beginning
- avoids a later distro migration becoming part of the core product risk

### Bench integration target

`Raspberry Pi 5` is not only a demo target. It should also be treated as a supported bench integration target for:

- UI development
- touchscreen testing
- local GNSS testing
- `CAN FD` module integration
- bench testing with `ADC/AHRS`, `EIS`, and future modules

This means the MFD software and interfaces should be designed so the same application can run on both `CM5` and `Raspberry Pi 5` with minimal target-specific differences.

## Display

### Supported display strategy

The MFD does not need to be fixed to one physical display size, but it should support a defined set of display classes instead of arbitrary scaling.

Recommended classes:

- `Primary`: `10.1"` landscape, baseline `1280x800`
- `Compact`: `7"` class, landscape
- `Standby`: `5"` class for reduced or role-specific pages

The initial layout design baseline should still be:

- `10.1"` landscape
- `1280x800`
- split `PFD + map` capable

### Recommended main display target

- size: `10.1"`
- resolution: `1280x800`
- brightness target: `1000 cd/m2`
- technology: `IPS`
- viewing: wide angle

### Additional supported targets

- `5"` to `7"` for standby or dedicated attitude displays

### Display-size rule

Support a small number of explicitly tested size classes. Do not try to make v1 work on arbitrary displays with no layout-class boundaries.

## Front Panel Controls

### Demo and bench input baseline

- touchscreen first
- mouse fallback
- keyboard fallback

This is the correct baseline for early software development and bench testing.

### Installed hardware controls later

- `left rotary encoder` with push
- `right rotary encoder` with push
- `6 to 8 soft keys`
- `Menu`
- `Back`
- `brightness access`

### Control architecture rule

The software should route all user actions through a common command/action layer so touchscreen, mouse, keyboard, and later physical controls all trigger the same underlying actions.

## Required Hardware Blocks

### 1. Compute carrier

The MFD needs a carrier around the CM5 with:

- dual `CAN FD` interfaces preferred
- UART for GNSS
- USB service path
- optional Ethernet
- display output
- front-panel GPIO/encoder input
- power supervision input
- service/provisioning access for image loading and recovery

For bench work on `Raspberry Pi 5`, equivalent access can initially be provided by:

- HDMI display
- USB touchscreen
- USB keyboard/mouse
- USB, HAT, or SPI/UART-connected `CAN FD` interface
- USB or UART-connected GNSS

### 2. GNSS receiver

Recommended first target:

- `u-blox NEO-F10N` class receiver

Required features:

- local position
- UTC time
- fix quality
- serial interface to the display head

### 3. Display interface

The first version can use:

- HDMI display path on both `CM5` and `Raspberry Pi 5`

Later versions may move to:

- MIPI DSI or LVDS

HDMI is acceptable for the first MFD prototype because it reduces risk and shortens bring-up time.

### 4. Power input stage

The MFD should accept aircraft electrical system input through a protected front-end.

Required functions:

- wide input tolerance
- reverse polarity protection
- input transient suppression
- input filtering
- regulated system rails
- power-fail detect

Because the project is `Yocto-first`, the power architecture should also assume:

- clean boot to the application without a desktop session
- controlled shutdown behavior
- future support for read-only rootfs and supervised recovery

### 5. Thermal path

Required:

- heat spreader or heatsink path for the SoM
- internal temperature monitoring
- enclosure path for passive dissipation

### 6. Nonvolatile storage

Required storage classes:

- OS / application storage
- config storage
- log storage
- map database storage

Preferred:

- OS/app on eMMC
- logs/maps on NVMe or large eMMC partition

### Recommended storage layout

Because the software target is `Yocto-first`, storage planning should support a product-style image layout from the beginning.

Recommended logical partitions:

- boot
- rootfs A
- rootfs B reserved for future update strategy
- persistent config
- logs
- map database

This does not mean full A/B updates must exist in v1, but the hardware should not prevent them later.

### 7. Provisioning and recovery path

The MFD hardware should provide a straightforward way to:

- flash the base image to eMMC or NVMe
- recover a failed image
- enter service/provisioning mode without disassembling the panel

Preferred:

- exposed service USB path
- accessible boot/recovery control method
- documented field/service procedure

## Recommended External Connectors

### Rear or side connectors

- aircraft power input
- CAN FD A
- CAN FD B
- GNSS antenna input
- USB service
- optional Ethernet service
- optional serial maintenance/debug
- optional dimming input

### Optional expansion

- external panel control input
- external annunciator output
- external audio output for alerts

## Suggested Connector Groups

### Power

- input power
- ground
- ignition/power enable if used
- power-fail sense if separated

### Data bus

- CAN_H
- CAN_L
- bus shield/drain if used

### GNSS

- antenna connector
- UART or USB interface to module

### Controls

- encoder A/B/push
- soft key matrix or direct GPIO
- brightness potentiometer or dim input if used

## Hardware Partitioning Rules

- the MFD must not directly terminate engine thermocouples, pitot/static sensors, or primary attitude sensors
- the MFD may host its own GNSS receiver
- the MFD may host temporary prototype ADSB interfaces during development
- the same MFD hardware should be able to run different software roles by configuration

## First Prototype BOM Categories

### Essential

- CM5 module
- CM5 carrier or custom carrier
- 10.1" sunlight-readable display
- GNSS receiver
- GNSS antenna
- CAN FD transceivers
- front-panel encoders
- buttons / soft keys
- power supply and protection stage
- enclosure / bezel
- cooling hardware
- provisioning/recovery access path

### Useful from the start

- NVMe storage
- RTC backup
- service USB access
- ambient light sensor
- buzzer or audio alert device

### Can wait until later

- touch panel
- Ethernet
- audio output amplifier
- second CAN bus if the first prototype uses only one

## Mechanical Targets

The first MFD mechanical design should define:

- panel cutout size
- bezel size
- display active area
- mounting hole pattern
- minimum panel depth
- connector clearance
- cooling path clearance

## Bench Bring-Up Hardware Requirements

For bench development, the MFD should be able to run with only:

- power
- display
- touchscreen or mouse/keyboard input
- local GNSS optional
- no external modules

This is required so `Demo Mode` and the UI can be developed before `ADC`, `EIS`, or `ADSB` are complete.

The bench platform should still boot the same class of `Yocto` image used for the product target, not a separate desktop-style development OS.

For integration bench testing, the same software stack should also support:

- live `CAN FD` interface
- live `GNSS`
- attached external modules
- mixed live and simulated sources during development

## Hardware Open Items

- exact CM5 SKU
- eMMC vs NVMe decision
- final display vendor and optical stack
- one or two CAN FD buses in v1
- final power input range
- touch or non-touch first panel
- exact bezel control count
