# libastromech for ESP32

Arduino/PlatformIO port of libastromech using [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino) 2.5.x.

**Deployment model:** one board talks to one droid. A separate board runs the beacon. There is no BLE multiplexing on a board, so build one firmware per droid plus one beacon firmware.

| Class | Role |
|---|---|
| `R2Unit`, `BBUnit` (`Astromech` base) | Connect (random address, double login), audio, motors, heads, ping/heartbeat |
| `Beacon` | Non-connectable manufacturer-data (`0x0183`) personality/location beacon, with a restart watchdog |
| `astromech_protocol.h` | Pure command/beacon encoding (no Arduino deps; unit-tested on the host) |
| `astromech_config.h` | **Generated** droid defaults and personality sound banks |

The API is blocking and exception-free (methods return `bool`). Calls are mutex-serialised, so a heartbeat task and an HTTP task can share one droid object. Failed sends retry up to 3 times, reconnecting between attempts, as in the Python library.

```cpp
astromech::initBle();
astromech::R2Unit droid("F7:E6:74:95:E5:53", astromech::findPersonality("resistance_blue"));
if (droid.connect()) { droid.play(2, true); droid.moveForward(1000); }
// periodically, e.g. every 30-60 s:
bool available = droid.checkAlive();   // pings; reconnects on failure
```

## Build and test

```bash
make esp32-build   # builds the r2_demo, bb_demo and beacon examples
make esp32-test    # host-side protocol tests (pio test -e native), no hardware
cd esp32 && pio run -e r2_demo -t upload
```

Set the droid address with `-DDROID_MAC=\"XX:XX:..\"` in `platformio.ini`.

## Config

`esp32/config/` is a synced copy of the top-level `config/`. `tools/gen_config.py` turns it into `src/astromech_config.h`, which is committed, so building needs no Python. Edit `config/`, then run `make sync-config`. `make check-config` fails if either the copy or the header is stale.

## Differences from the Python library

- No `keep_alive()` loop. Call `checkAlive()` from your own task or `loop()`.
- `scanForDroids()` finds devices named `DROID`; you normally connect by MAC.
- Notifications use a single global target, which is fine with one droid per board.
- Not yet hardware-tested: the code compiles and the encoding is unit-tested against the protocol notes, but nothing has run against a real droid.
