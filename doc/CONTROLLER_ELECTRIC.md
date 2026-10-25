# Controller and electric communication

This document describes how the **controller** (cab, type `0x10`) and the **electric** locomotive module (type `0x20`) talk on Loco-CAN, and which states each side depends on. IDs and flag names match `src/can_protocol.h`.

Two different “setup” concepts exist:


| Name             | ID / state                                             | Purpose                                                      |
| ---------------- | ------------------------------------------------------ | ------------------------------------------------------------ |
| **Loco setup**   | `CAN_ID_LOCO_SETUP` `0x600`, controller status `SETUP` | Choose the master loco and flip its stored direction         |
| **Module setup** | `0x7FF` enter/leave, ACK `0x7F8`                       | Programmer session. Modules go silent except `0x700`–`0x7FF` |


Loco setup is part of driving. Module setup is programming and is blocked while a controller drive heartbeat is on the bus.

---



## Roles

```
  Cab switches / levers          Motor / relays / ADC
  ┌──────────────────┐           ┌─────────────────────┐
  │    Controller    │           │      Electric       │
  │  TYPE_CONTROLLER │           │   TYPE_ELECTRIC     │
  └────────┬─────────┘           └──────────▲──────────┘
           │  0x010 heartbeat               │
           │  0x400 drive                   │  0x500 vehicle status
           │  0x220 signal                  │  0x310 motor voltage
           │  0x410 light                   │  0x320… batteries
           │  0x600 loco setup              │  0x300 series voltage
           └────────────── CAN ─────────────┘
```

The controller is the only node that may **command** the train. The electric module **executes** `0x400` and **reports** whether the loco is ready, moving or in fault. That report drives the controller status machine (standby → ready → moving, and emergency drop to off).

Only one controller may command. A second cab that sees `0x010` or `0x400` from another UUID goes to `LOCKED`.

---



## Timing


| Frame                             | Period                     | Sender                | Lost-link effect                                                                                |
| --------------------------------- | -------------------------- | --------------------- | ----------------------------------------------------------------------------------------------- |
| `CAN_ID_DRIVE_HEARTBEAT` `0x010`  | 50 ms                      | Commanding controller | Other cabs: bus occupied for 250 ms (`5 × 50`). Does **not** refresh the electric drive timeout |
| `CAN_ID_DRIVE` `0x400`            | 100 ms                     | Commanding controller | Electric: command stale after 300 ms (`3 × 100`). Timeout is idle, not error                    |
| `CAN_ID_SIGNAL` `0x220`           | 100 ms                     | Commanding controller | Horn bits only; electric ignores this ID                                                        |
| `CAN_ID_LIGHT` `0x410`            | 100 ms                     | Commanding controller | Switch modules; idle cabs copy instrument light                                                 |
| `CAN_ID_LOCO_SETUP` `0x600`       | 100 ms while in loco setup | Commanding controller | Electric leaves loco-setup after 500 ms without frames                                          |
| `CAN_ID_VEHICLE_STATUS` `0x500`   | 100 ms                     | Electric              | Controller drops the vehicle after 2 s (`2 ×` module heartbeat)                                 |
| Sensor voltages / current         | 200 ms                     | Electric              | Controller gauges (`0x320` battery, `0x310` motor, `0x110` current). Not used for status        |
| `CAN_ID_MODULE_HEARTBEAT` `0x510` | 1000 ms                    | Every module          | Presence for the programmer. Stopped in module setup                                            |


The controller sends `0x010` / `0x400` / `0x220` / `0x410` only while `controller_is_commanding(status)` is true (everything except `OFF` and `LOCKED`).

---



## Messages



### Controller → bus



#### Drive heartbeat `0x010`

Empty frame. Means “a cab is commanding”. Used by:

- other controllers (`foreign` / `LOCKED`)
- the programmer (module setup is refused while this is fresh, 250 ms hold)

Electric receives it but does **not** treat it as a live throttle. The drive command itself must keep arriving.

#### Drive `0x400` (8 bytes)

Packed by `controller_pack_drive()`, unpacked by `electric_unpack_drive()`.

```
byte 0:  7 error | 6 aux-1 | 5 aux | 4 down | 3 up | 2 dir | 1 motor | 0 mains
byte 1:  unused
byte 2:  bit7 multi | bits6-2 loco UUID 15..11 | bits1-0 drive 9..8
byte 3:  drive 7..0
byte 4:  bits7-2 loco UUID 10..6 | bits1-0 power 9..8
byte 5:  power 7..0
byte 6:  bits7-2 loco UUID 5..0 | bits1-0 brake 9..8
byte 7:  brake 7..0
```


| Field             | Controller source                        | Electric use                                              |
| ----------------- | ---------------------------------------- | --------------------------------------------------------- |
| `mains`           | Main switch not off, and commanding      | Must be set or outputs stay off                           |
| `motor`           | Status `ON` / `READY` / `MOVING`         | Pantograph / “drive enabled”                              |
| `dir`             | Latched reverse while moving             | XOR with EEPROM `reversed`                                |
| `up` / `down`     | Horn / horn2                             | Not used by electric (also on `0x220`)                    |
| `error`           | Fault frame on drop to off               | Immediate cut, status error                               |
| `drive` / `brake` | Levers, 10-bit, only in `READY`/`MOVING` | PWM; effective drive is `drive × power / 1023`            |
| `power`           | Power pot, or 1023 if drive > 0          | Current / throttle limit                                  |
| `multi`           | More than one `0x500` UUID               | Echoed in vehicle status                                  |
| `loco`            | Selected / first vehicle UUID            | Carried in the command; **not** used to mute slave motors |


On a controller fault frame, `error` is set, speed is zero and brake is 1023.

#### Signal `0x220`

Horn high / horn low bits. Switch and sound modules. Electric ignores it.

#### Light `0x410`

One byte of `LIGHT_*` bits from the two light switches. Switch modules map outputs to these bits. A non-commanding cab copies `LIGHT_INSTR` to its instrument lamp.

#### Loco setup `0x600` (3 bytes)

```
byte 0: bit0 status (0 = select master, 1 = reverse direction)
        bit1 disable (end of loco setup)
byte 1: loco UUID LSB
byte 2: loco UUID MSB
```

Sent every drive period while the cab is in `CONTROLLER_STATUS_SETUP`, and once with `disable` when leaving that state.

### Electric → bus



#### Vehicle status `0x500` (3 bytes)

```
byte 0:  7 error | 6 ready | 5 moving | 4 multi | 3 reverse-logic | 2 dir | 1 drive | 0 mains
byte 1:  UUID LSB
byte 2:  UUID MSB
```

`electric_vehicle_status()` builds byte 0 from the last drive command, the plugin output and the motor voltage.


| Bit               | Meaning for the controller                                                  |
| ----------------- | --------------------------------------------------------------------------- |
| `error`           | `0x000` emergency or leftover link fault. Cab fault drive is not copied.    |
| `moving`          | PWM active or |motor voltage| ≥ `voltage_min` → cab may enter `MOVING`      |
| `ready`           | Mains + motor, direction settled, no fault                                  |
| `mains` / `drive` | Echo of the command (cleared on error)                                      |
| `reverse-logic`   | EEPROM direction flip from loco setup                                       |
| `dir`             | Applied plugin direction is reverse                                         |
| `multi`           | Echo of the drive multi bit                                                 |


During loco setup, a **non-selected** loco reports `mains + drive + ready` and holds the motor off (`setup_idle`). That keeps the cab’s vehicle list alive without moving the consist.

#### Sensor frames (4 bytes each, every 200 ms)

Little-endian reading plus ADC full-scale reference.


| ID                             | Content                             |
| ------------------------------ | ----------------------------------- |
| `0x310` `CAN_ID_MOTOR_VOLTAGE` | Signed motor voltage (plus − minus) |
| `0x320` / `0x321` / `0x322`    | Battery cells 0…2                   |
| `0x300` `CAN_ID_VOLTAGE`       | Sum of the configured cells         |


The controller does not consume these for its status machine.

---



## Controller status and electric feedback

```
                    horn + mains, bus not moving
   OFF ---------------------------------------------> SETUP
    | mains on, bus free (no foreign 0x010/0x400)        |
    v                                                    | mains off
 STANDBY <---- motor off ---- ON                         v
    | motor on, lever nulled   | dir selected           OFF
    v                          v
  NULLED -- dir --> READY -- 0x500 moving --> MOVING
                      ^                            |
                      +---- not moving, dir set ---+
```

Inputs that come from the **electric** side (via `0x500`):

- `bus_moving` — any remembered vehicle has `MOVING`
- `vehicle_error` — any remembered vehicle has `ERROR` (cuts motor-on states only; standby keeps sending mains)
- `emergency` — `0x000` or both horn buttons (cuts every commanding state)

Inputs that come from the **bus / other cab**:

- `foreign` — another UUID sent `0x010` or `0x400`
- `bus_free` — no foreign frame for 250 ms

The cab remembers up to six vehicles. A UUID expires 2 s after the last `0x500`. That list is also the loco-setup roster and the multi-traction count.

`READY` does not mean “lever off the stop”. The cab enters `READY` when mains is motor and direction is not neutral. It enters `MOVING` only when an electric module reports `moving`.

---



## Link supervision

```
Controller                         Electric
    │ 0x400 every 100 ms                │
    │──────────────────────────────────►│  _drive_timeout = 300 ms
    │                                   │  stale drive → idle, not error
    │ 0x500 error                       │
    │◄──────────────────────────────────│  outputs off, status error
    │ motor-on → OFF, standby stays     │
    │ 0x400 error only on motor cut     │
    │──────────────────────────────────►│  command.error → cut
```

A lone `0x010` does not keep the motor alive. If `0x400` stops, the electric module drops to idle and clears `link_fault` and a latched `0x000`. A quiet bus is not an error.

The next valid `0x400` also clears `link_fault`. A cab fault drive (`error` bit) cuts outputs but is **not** copied into `0x500` error — otherwise the cab sees its own cut, drops to `OFF`, sends another error frame, and never publishes mains again. After a horn / `0x000` cut the cab stays `OFF` until mains is switched off. After that cycle it returns to standby even if `0x500` still shows error, and sends a clean mains-off then mains-on drive so the locomotive can recover.

`CAN_ID_EMERGENCY` `0x000` stays until a drive frame with mains off and no error bit, or until the drive timeout.

---



## Loco setup (consist pairing)

This is **not** the programmer session.

1. Mains off, train not moving, press horn, then switch mains on → controller `SETUP`.
2. Cab sends `0x600` with the UUID of the vehicle at `_setup_index` (first `0x500` seen is index 0).
3. Every electric module enters loco-setup (`_setup_active`). The matching UUID is `_setup_selected`.
4. Short horn steps `_setup_index` to the next vehicle.
5. Direction lever ≠ neutral sets `0x600` bit 0. The **selected** loco toggles EEPROM `reversed` on the rising edge and stores it.
6. Mains off: cab leaves `SETUP`, sends `0x600` with `disable`. Electric modules leave loco-setup (or after 500 ms silence).

While loco-setup is active, non-selected locos stay `setup_idle`: motor outputs forced off, status still looks “ready” so the cab keeps seeing them.

The selected UUID is also written into later `0x400` frames as `loco` when more than one vehicle is present (`multi`).

---



## Electric execution path

```
0x400 ── unpack ──► ELECTRIC_COMMAND
                         │
                         ▼
              plugin (direct / 4QD / Curtis)
                         │
                         ▼
              ELECTRIC_OUTPUT ── PWM, relays, optional power cut
                         │
                         ▼
              0x500 vehicle status
```

Direction changes wait until `|motor voltage| < voltage_min` (EEPROM), then open both direction relays for 30 ms before engaging the new side. While blocked, throttle PWM is zero.


| Plugin | `DRIVE_PWM`  | `DRIVE_BREAK`        | `DRIVE_FORW`   | `DRIVE_REV`    | `DRIVE_POWER`                                          |
| ------ | ------------ | -------------------- | -------------- | -------------- | ------------------------------------------------------ |
| Direct | Drive PWM    | Brake PWM            | Forward relay  | Reverse relay  | Series contactor (optional)                            |
| 4QD    | Throttle PWM | (unused / brake PWM) | Enable         | Reverse switch | Ignition (drops during direction wait)                 |
| Curtis | Throttle PWM | (unused / brake PWM) | Forward switch | Reverse switch | Keyswitch; HPD locks throttle until it returns to zero |


Effective throttle is `drive × power / 1023`. Brake either subtracts from drive (drive-only PWM) or uses a second PWM channel.

---



## What each side depends on



### Electric depends on the controller for

- Live `0x400` (mains, motor, direction, 10-bit drive / power / brake, fault)
- Periodic `0x400` within 300 ms, otherwise idle (not error)
- `0x000` emergency
- `0x600` to select the loco and persist reverse
- Absence of module-setup (`loco_setup.active`): in that mode the motor is forced off and only EEPROM GET/SET run

Electric does **not** need `0x010` to stay enabled. It does not follow `0x220` or `0x410`.

### Controller depends on the electric module for

- `0x500` so vehicles appear at all (standby LED “no loco” when the list is empty)
- `0x500` `moving` to leave `READY` for `MOVING`
- `0x500` `error` to cut motor-on states; standby keeps commanding mains
- At least one `0x500` UUID for loco-setup and the `multi` / `loco` fields on `0x400`

The controller does **not** use electric voltage frames for interlocks. Motor-voltage gating of direction lives entirely on the electric module.

### Shared / bus-wide dependencies

- **One commanding cab.** Foreign `0x010`/`0x400` locks others.
- **Module setup vs drive heartbeat.** Programmer enter is refused for 250 ms after `0x010`. A heartbeat during an open session aborts it. Both modules then stop autonomous TX (no `0x010`, `0x400`, `0x500`, sensors, or `0x510`) until leave or 30 s idle.
- **Switch modules** sit beside this pair: they consume `0x410` / `0x220` / `0x400` from the cab. They are not required for the motor loop.

---



## EEPROM that affects the loop

**Controller** (`src/module/controller/params.h`): analog-switch points for mains, direction and lights; drive/brake lever zero and full. Live ADC GET indices 45…55 are not stored; the programmer copies them into the point bytes.

**Electric** (`src/module/electric/settings.h`): plugin type, battery channel count, PWM mode, `reversed` (loco setup), `voltage_min` for direction changes.

A cab flashed with different lever points than the physical pots will publish a wrong 10-bit drive. An electric module with the wrong plugin or `voltage_min` will refuse to settle direction or will wire the power stage incorrectly. Those are configuration dependencies, not CAN IDs.

---



## Source map


| Topic                                  | Files                                                              |
| -------------------------------------- | ------------------------------------------------------------------ |
| IDs and drive/status bit layout        | `src/can_protocol.h`                                               |
| Cab state machine                      | `src/module/controller/status.cpp`                                 |
| Pack `0x400`                           | `src/module/controller/drive_codec.h`                              |
| Cab TX/RX, vehicle table, loco setup   | `src/module/controller/main.cpp`                                   |
| Unpack `0x400`, plugins, `0x500` bits  | `src/module/electric/plugin.cpp`                                   |
| Electric timeouts, loco setup, sensors | `src/module/electric/main.cpp`                                     |
| Voltage frames                         | `src/module/electric/sensors.cpp`                                  |
| Module setup session                   | `src/core/setup/setup_mode.h`                                      |
| Host checks                            | `host/controller_logic_check.cpp`, `host/electric_logic_check.cpp` |


