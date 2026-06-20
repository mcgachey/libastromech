# libastromech

Python library for controlling Star Wars: Galaxy's Edge astromech droids over Bluetooth Low Energy (BLE).

## Architecture

- `src/libastromech/astromech.py` - Base `Astromech` class with BLE connection management, command protocol encoding, motor control, and audio playback. Uses `bleak` for BLE communication.
- `src/libastromech/r2.py` - `R2_Unit` subclass with R-Series-specific methods: head rotation, center head, look around, differential drive (forward, backward, spin, turn, drift).
- `src/libastromech/bb.py` - `BB_Unit` subclass with BB-Series-specific methods: forward/backward movement and head rotation via body spin.
- `src/libastromech/beacon.py` - BLE beacon broadcasting via `hcitool`/`hciconfig` subprocess calls. Supports location and personality beacon construction, start/stop, and a long-running `run_beacon()` async loop.
- `src/libastromech/__init__.py` - Public API re-exports: `Astromech`, `Direction`, `Personality`, `Sound`, `R2_Unit`, `BB_Unit`, `location_beacon_payload`, `personality_beacon_payload`, `run_beacon`, `start_beacon`, `stop_beacon`.

## Config

- `src/config/droids/r2.yml` and `bb.yml` - Motor speed and ramp time defaults per droid type.
- `src/config/personalities/*.yml` - Sound banks (group, sound index, duration in ms). Named by personality chip color + affiliation.

## Key Concepts

- Droids connect as async context managers (`async with R2_Unit(...) as droid:`). The `__aenter__` handles BLE connect, notification subscription, and the double-login handshake (`0x222001` x2).
- Commands are written to GATT characteristic handle 13 (Command Characteristic `0x000e`). Notifications come from handle 10 (Notify Characteristic `0x000b`).
- Command encoding: `[0x1f + 4 + data_len, 0x42, command_id, 0x40 + data_len, ...data]`. Audio commands use command ID `0x0f` with sub-function byte `0x00`. Motor commands use command ID `0x05`.
- `Sound` objects store group, sound index, and duration (ms). `Personality` loads these from YAML config files.
- `keep_alive()` periodically reconnects and pings, calling success/failure callbacks.

## Samples

- `src/samples/r2.py` - R2 unit movement + sound demo
- `src/samples/bb.py` - BB unit head turn + sound demo
- `src/samples/beep.py` - Play a single sound by index
- `src/samples/discover_sounds.py` - Interactive tool to catalog sound durations for a personality chip
- `src/samples/play_all_sounds.py` - Play all sounds from a personality sequentially
- `src/samples/keep_alive.py` - Heartbeat monitor demo

## Build & Install

```bash
pip install -e .
```

Package is built with setuptools via `pyproject.toml`. Requires Python >= 3.9, `bleak>=0.22.3`, `PyYAML>=6.0.2`.

## Known Droid MAC Addresses

- R2-T2: `F7:E6:74:95:E5:53`
- BB-12: `E6:1C:86:B3:BD:AE`
