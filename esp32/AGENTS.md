# AGENTS.md: libastromech ESP32 implementation

Guidance for agents and developers working in `esp32/`. For the repo-wide rules (config source of truth, make targets, deploy safety) see the top-level `CLAUDE.md`. For user-facing usage see `README.md`. This file covers how the library is built and what to watch out for.

## Purpose and deployment model

An Arduino/PlatformIO port of the Python library in `../python`, for controlling Star Wars: Galaxy's Edge astromech droids over BLE.

- **One board connects to one droid.** There is no BLE multiplexing on a board, and the code assumes this (see "Single-droid assumption").
- **A separate board runs the beacon.** It never connects to anything.
- The intended consumer is a firmware per droid plus one beacon firmware, replacing the Python `astromech_server` (a Flask app on a Raspberry Pi). That firmware (Wi-Fi, HTTP, Home Assistant reporting) is **not** part of this library. The library only supplies the droid and beacon primitives it needs.

The Python code and the protocol notes are the reference for behaviour. When they disagree with this port, the Python code is the working implementation.

## Layout

```
src/
  astromech_types.h      Sound, Personality, DroidDefaults, Direction, Motor (no deps)
  astromech_protocol.*   PURE wire encoding + beacon payloads (no Arduino/BLE deps)
  astromech_config.h     GENERATED. Droid defaults and personality sound banks
  astromech_ble.h        initBle(): NimBLEDevice::init wrapper shared by both roles
  Astromech.*            Connection, retry, audio, motor primitives (base class)
  R2Unit.*, BBUnit.*     Droid-type-specific movement built on Astromech
  Beacon.*               Non-connectable manufacturer-data advertiser
config/                  Synced copy of ../config (do not edit)
tools/gen_config.py      YAML -> src/astromech_config.h
test/test_protocol/      Host-side Unity tests
examples/                One main.cpp per example env
library.json             For consumers using this as a PlatformIO library
platformio.ini           Project root is the library itself (see below)
```

The split between `astromech_protocol` and the BLE classes is deliberate: everything that can be tested without hardware lives in the protocol layer and is covered by `pio test -e native`. Keep new encoding logic there, not inline in `Astromech.cpp`.

## Build and test

```bash
make esp32-test    # host tests: pio test -e native (no hardware, no ESP toolchain)
make esp32-build   # builds all default example envs for the board
pio run -e <env> -t upload      # from esp32/, flash one example
```

`platformio.ini` sets `src_dir = .` and gives each example env a `build_src_filter` of `src/*.cpp` plus one `examples/<name>/` directory. This avoids the usual "library inside a project" recursion. To add an example, create `examples/<name>/main.cpp` and add an `[env:<name>]` that extends `esp32_common`. Include `<Arduino.h>` in example sources, since they are `.cpp`, not `.ino`.

The code is built as `gnu++17` with `-Wall -Wextra` and currently compiles warning-free. Keep it that way.

## Config pipeline

- `../config/` is the only place to edit droid and personality YAML.
- `make sync-config` copies it into `esp32/config/` and runs `tools/gen_config.py`, which writes `src/astromech_config.h`.
- The generated header **is committed**, so firmware builds need neither Python nor PyYAML. The generator is a developer-time tool only.
- `make check-config` fails if the YAML copy has drifted or the generated header is stale (`gen_config.py --check`). Never hand-edit `astromech_config.h`; the next sync overwrites it.
- Personalities are looked up by their YAML file stem with `findPersonality("resistance_blue")`, which returns `nullptr` for an unknown name.

## How the droid side works

Wire format (same as Python `_command`): `[0x1f + 4 + n, 0x42, command_id, 0x40 + n, data...]`, with `n <= 31`.

- Audio is command `0x0f` with data `[0x44, 0x00, cmd, param]` (`0x1f` = set group, `0x18` = play clip N from the current group, `0x0e` = volume).
- Motor is command `0x05` with data `[direction|motor, speed, ramp_hi, ramp_lo, delay_hi, delay_lo]`.
- Head centering is command `0x0f` with data `[0x44, 0x01, speed, stop_at_center ? 0 : 1]`.
- Login is the raw 3-byte frame `22 20 01`, **not** wrapped by `encode()`.
- Ping is command `0x02` with data `[0, 0]`.

Connection sequence (`Astromech::doConnect`): connect to the MAC with a **random** address type, find the droid service, get the command and notify characteristics **by UUID**, subscribe to notifications, wait 500 ms, then write the login frame **twice**. The droid disconnects within seconds if it doesn't see the double login. The delay and the double write mirror Python.

Sends (`Astromech::send`) take the mutex and try up to 3 times. A failed or disconnected attempt tears the client down and reconnects from scratch, then waits 1 s. `checkAlive()` is just a ping, since `send()` already handles reconnection. It is the building block for a heartbeat. There is deliberately no built-in `keep_alive()` loop. The caller owns task scheduling.

Writes use write-with-response, so a successful return means the droid's BLE stack acknowledged the write, not that the droid acted on it.

`R2Unit`/`BBUnit` movement follows Python `_move_wheels`: send left motor, send right motor, wait `duration`, then send two zero-speed stops. Note that `stop()` is attempted even if a start command failed, so a half-started move still gets stopped.

### Concurrency model

The API is blocking and exception-free: methods return `bool`. All BLE writes go through a `std::recursive_mutex`, so several FreeRTOS tasks may share one droid object. This is how Python's `asyncio.gather(play, look_around)` is reproduced (see `examples/r2t2_beeps`). Rules that matter:

- **Never hold the mutex while sleeping.** `play(wait=true)` holds it only for the group-select and play pair (so concurrent plays can't interleave) and releases it before waiting out the clip. Holding it across a `delay()` blocks every other task and silently serialises what should be concurrent.
- `lookAround()` blocks for about 3 s by design, so run it in its own task if you need it concurrent with something else.
- BLE calls shouldn't be made from NimBLE callbacks.

### Single-droid assumption

`Astromech::notifyTrampoline` has no context argument, so notifications are routed through one file-scope `g_notify_target`. With more than one `Astromech` in a process, the last one to connect receives all notifications. That is fine for the one-board-one-droid model. Fix it with a per-characteristic lambda or a small registry if multi-droid ever becomes a goal.

## Beacon side

`Beacon` advertises manufacturer-specific data: company ID `0x0183` (little-endian) followed by the payload, using non-connectable advertising at 100-150 ms intervals. `ensureRunning()` restarts advertising if it has stopped, and is meant to be called from `loop()` as a watchdog. It is the analogue of the server's restart-on-crash loop.

Payload builders (`personalityBeacon`, `locationBeacon`) are pure and tested. The Python server's keep-droids-awake beacon is `personalityBeacon(Affiliation::Silent, kChipRSeriesDefault)`. Droids ignore affiliations `0x0C`-`0x0F` but still reset their sleep timer.

**Status-byte discrepancy:** the notes' example beacon tables show status `0x81`, but their own formula and the Python code produce `0x01` when not paired with a remote and `0x81` when paired. This port follows the formula and Python. The tests cover both forms. If beacons ever fail to affect droids, check this byte first.

## NimBLE API pin

The code targets **NimBLE-Arduino 2.5.x** (`~2.5.2` in both `platformio.ini` and `library.json`). The 1.x and 2.x APIs differ, and 1.x snippets from the internet will not compile. Examples:
- `setConnectableMode()` replaces `setAdvertisementType()`.
- `NimBLEClient::setConnectTimeout()` takes **milliseconds**.
- `subscribe()` is on the remote characteristic.
- Scan results are returned by `getResults(duration_ms, is_continue)`.

If you bump the version, rebuild every env and re-verify on hardware, because the host tests don't touch the BLE layer.

## Verification status (what is and isn't proven)

Verified on a classic ESP32 (ESP32-D0WD-V3) against a real R-series droid:
- Connect, service discovery, notification subscribe and double login succeed.
- Audio group select and play work (droid audibly beeped).
- The full demo (center head, concurrent look-around and sounds, center head) runs to completion with every write acknowledged.

**Not** verified on hardware: wheel motion, BB-series behaviour, `checkAlive()` reconnect after a real dropout, `scanForDroids()`, volume, the beacon (nobody has confirmed a droid reacting to it), and boards other than the classic ESP32. Physical head movement was not explicitly confirmed by an observer. Treat these as compiled-and-unit-tested only.

The host tests cover encoding only. They can't catch BLE timing, connection or concurrency problems.

## Gotchas

- The droid refuses BLE connections while its stock remote is connected. The remote must be off, and the droid must be awake. A failed connect is usually this.
- Droids auto-sleep after about 5 minutes without commands or beacon traffic.
- The notes call byte 1 `0x22 + (total - 1)` in one place and `0x1f + 4 + n` in another. They are the same value, and the code uses the latter. It also always sets byte 2 to `0x42`, as Python does, even though the notes say `0x00` for non-`0x0f` commands. The droid accepts it. Don't "fix" this without testing on a droid.
- `Sound.ms` durations come from the YAML, measured by the `discover_sounds` Python sample. They are only used for `wait=true`.
- `delay()` is used for waits, so a task that calls blocking methods yields to the scheduler but is unavailable for other work meanwhile.
- Speed values below about `0x60` won't move the motors (see the notes). `0` is used as "stop".

## Conventions

- 2-space indentation, matching the Python tree.
- Namespace everything in `astromech`. Pure code goes in `astromech::protocol`.
- No exceptions or RTTI assumptions; return `bool` for fallible operations.
- Prefer small, explicit examples over framework-level abstractions in the library. Network, Home Assistant and HTTP concerns belong to the application firmware, not here.
- When adding protocol behaviour, add the encoding to `astromech_protocol.*` first with a test whose expected bytes come from the protocol notes or Python output, not from the new encoder itself.
