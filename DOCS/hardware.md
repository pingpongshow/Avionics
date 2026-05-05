# Experimental PFD/MFD Hardware Architecture

## Goals

- Build the system as a set of independent modules instead of one monolithic box.
- Allow repair, upgrade, and experimentation by replacing one module without redesigning the others.
- Support one large split-screen display, multiple smaller displays, or a mixed system with dedicated PFD, MFD, and engine pages.
- Keep sensor acquisition in dedicated real-time modules and keep the display head focused on presentation, navigation, and user interaction.
- Support operation with optional modules added over time.

## System Architecture

The system should be organized around interchangeable modules connected over a common vehicle bus.

### Core modules

- `MFD / Primary Display`
  Main display computer. Renders flight instruments, moving map, engine data, traffic, alerts, menus, and configuration pages.
- `ADC (Air Data Computer)`
  Provides baro altitude, vertical speed, pitot/static airspeed, OAT, and optional AOA.
- `AHRS / IMU`
  Provides pitch, roll, turn rate, slip/skid, and other attitude data. This may be combined with the ADC in one module.
- `EIS (Engine Information System)`
  Provides RPM, manifold pressure, oil pressure, oil temperature, fuel flow, CHT, EGT, volts, amps, and other engine data.
- `ADSB In`
  Provides traffic targets and, if supported later, weather products.
- `Secondary Display`
  Optional second display configured as a dedicated PFD, map, standby attitude, or engine page.
- `Auxiliary / Safety`
  Optional CO sensor, fuel calculator, annunciator panel, dimmer controller, external knobs, or data logger.

### Communications

- `Primary bus`: `CAN FD`
  Fast, simple, fault-tolerant, and well suited to distributed avionics-style modules.
- `Optional secondary bus`: `Ethernet`
  Useful later for software update, large log transfer, display mirroring, terrain databases, or video-like data. Not required for the first version.

### Architectural rule

The display head should not be the primary source of attitude or air-data sensing. It should consume that data from dedicated modules. The exception is `GPS/GNSS`, which is advantageous to keep local to the MFD for moving-map responsiveness and simpler wiring.

## Display Hardware Philosophy

All display units should share one common hardware platform where practical. The role should be set by configuration, not by unique hardware.

Examples:

- A `10.1"` display can run as the main `MFD/PFD split-screen`.
- A second identical display can be configured as `full-screen map`, `engine page`, or `reversionary PFD`.
- A smaller display can be configured as a `standalone attitude display`.

This keeps the system easier to repair and lets a failed module be replaced with the same spare hardware.

## Primary Display (MFD)

### Purpose

The `Primary Display / MFD` is the main user interface node. It collects data from connected modules, renders the active page layout, manages local navigation/map data, and exposes user controls and configuration.

### Responsibilities

- Render PFD symbology: attitude, altitude, airspeed, slip/skid, bugs, trend tapes, and annunciations.
- Render moving map and ownship position from local GPS input.
- Render engine pages when an `EIS` module is present.
- Render traffic when an `ADSB In` module is present.
- Detect connected modules and adapt page layout accordingly.
- Provide system configuration, calibration entry points, status pages, and fault reporting.
- Provide a `Demo Mode` for development, bench testing, UI work, and sales/demo use.

### Non-responsibilities

- Direct acquisition of primary attitude data.
- Direct acquisition of pitot/static air-data.
- Direct acquisition of engine probes.

Those functions belong in dedicated modules.

### Recommended MFD hardware

- `Compute`
  `Raspberry Pi CM5` is the recommended first platform for the display head. It is flexible, available, and powerful enough for map rendering, split-screen layouts, and future UI growth.
- `Display`
  `10.1"` sunlight-readable IPS panel, target `1000 cd/m2` brightness, target resolution `1280x800`. This is a good size for a combined PFD/MFD layout like the reference examples.
- `Storage`
  Prefer `eMMC` or `NVMe` over microSD for reliability.
- `Controls`
  At minimum:
  - dual rotary encoders with push
  - 6 to 8 front-panel soft keys
  - dedicated `Menu`, `Back`, and brightness access
  - power/status indicator
- `GPS`
  Local GNSS receiver connected to the display head for map position, time reference, and smoother moving-map behavior.
- `Communications`
  - dual `CAN FD` interfaces preferred
  - `USB` for service, updates, and prototype ADS-B integration
  - optional `Ethernet`
  - optional serial service/debug port
- `Power`
  Designed for aircraft bus input with a protected input stage and regulated internal rails. Brownout behavior and clean shutdown need to be defined early.

### Recommended MFD inputs

- `Local`
  - GPS / GNSS position
  - time source
  - front-panel buttons and encoders
  - display dimming input or ambient light input
- `From ADC/AHRS`
  - pitch / roll / yaw-rate
  - slip/skid
  - indicated airspeed
  - altitude
  - vertical speed
  - baro setting state
  - OAT
  - optional AOA
- `From EIS`
  - all engine and fuel measurements
- `From ADSB`
  - traffic tracks
  - optional weather later
- `From system modules`
  - heartbeat / health / faults
  - module capabilities
  - firmware / configuration versions

## Adaptive Layout System

The display layout should change based on detected modules and selected operating mode.

### Layout principles

- The same display hardware should support multiple page compositions.
- Missing modules should remove or collapse related panes rather than leaving dead space.
- Critical flight data should remain readable even when optional panes are enabled.
- The UI should support both portrait-like instrument emphasis and wide split-screen map emphasis.

### Reference style

The attached example displays show the main layout ideas worth supporting:

- large central attitude view
- split-screen PFD and map
- side or bottom engine strips
- overlay traffic and annunciations
- full-page map mode
- smaller dedicated backup/standby display mode

The project should use these as functional references, not exact copies.

### Initial layout modes

- `PFD Focus`
  Large attitude area, airspeed and altitude tapes, compact HSI, minimal map inset.
- `PFD + Map Split`
  Main flight instruments on one side, moving map on the other side.
- `PFD + Map + Engine`
  Adds engine strip or engine block when `EIS` is present.
- `Full Map / MFD`
  Moving map takes most of the screen with compact flight data overlay.
- `Engine Page`
  Full engine presentation with compact navigation data.
- `Reversionary`
  If a second display fails or modules are missing, collapse to the minimum critical set.
- `Standby Display`
  Reduced attitude / airspeed / altitude layout for a small secondary display.

### Module-driven behavior

- If `GPS` is present and no other modules are connected, the unit should still operate as a map/navigation display.
- If `ADC/AHRS` is connected, enable full PFD layouts.
- If `EIS` is connected, add engine pages and engine strips.
- If `ADSB In` is connected, add traffic overlays and traffic pages.
- If `CO sensor` is connected, add alerting and status widgets.

### Layout engine requirements

- Support saved user presets.
- Support automatic defaults based on detected module set.
- Support per-display role selection:
  - `Primary MFD`
  - `Primary PFD`
  - `Map repeater`
  - `Engine display`
  - `Standby attitude`

## Demo Mode

The system should include a `Demo Mode`.

### Demo Mode goals

- Allow UI and layout development without connected hardware modules.
- Allow bench testing of the display head by itself.
- Allow simulation of optional modules before they exist.
- Allow quick customer/demo presentation of layouts and pages.

### Demo Mode behavior

- Present synthetic flight, engine, and traffic data.
- Allow individual module simulation to be toggled on/off:
  - `ADC/AHRS`
  - `GPS`
  - `EIS`
  - `ADSB In`
  - `CO sensor`
- Provide multiple scenarios:
  - power-on idle
  - taxi
  - takeoff / climb
  - cruise
  - approach / landing
  - engine warning / alarm
  - traffic near ownship
  - sensor/module failure
- Clearly mark the screen as `DEMO` so it cannot be confused with live flight data.

## Upgradeability and Repair

- Each module should be replaceable independently.
- The display head should be usable even if some optional modules are absent.
- Shared hardware across display types should be preferred over unique one-off units.
- Front-panel controls and display should be serviceable without redesigning sensor modules.
- The bus protocol should support module discovery and capability reporting so new modules can be added without rewriting the entire display stack.

## Recommended First Implementation

Start with these three modules:

1. `MFD / Primary Display`
   CM5-based display head with local GPS and front-panel controls.
2. `ADC/AHRS`
   Dedicated air-data and attitude module on `CAN FD`.
3. `EIS`
   Dedicated engine monitoring module on `CAN FD`.

`ADSB In` should be added next as a separate module or USB-connected prototype source.

## Next Hardware Work

- Define the `CAN FD` message/discovery scheme.
- Finalize the first MFD screen size and bezel/control layout.
- Select the exact GNSS receiver for the MFD.
- Define power-input and shutdown behavior for the display head.
- Write separate hardware specs for:
  - `ADC/AHRS`
  - `EIS`
  - `ADSB In`
  - `Secondary Display`
