#pragma once

// BLE beacon broadcaster. Runs on its own board (no connections, so it never
// competes with a droid client for the radio). Replaces beacon.py's BlueZ
// advertisement: manufacturer data 0x0183 + payload, non-connectable.

#include <Arduino.h>
#include <NimBLEDevice.h>

#include "astromech_ble.h"
#include "astromech_protocol.h"

namespace astromech {

class Beacon {
 public:
  // Call astromech::initBle() first. Starts advertising immediately.
  bool start(const protocol::BeaconPayload& payload);
  void stop();
  bool isAdvertising() const;
  // Restart advertising if it has stopped for any reason. Returns isAdvertising().
  // Cheap; call from loop() as a watchdog (analogue of the server's restart loop).
  bool ensureRunning();
  // Time since start()/last restart, for a /health-style report.
  uint32_t msSinceStarted() const { return millis() - started_ms_; }

 private:
  protocol::BeaconPayload payload_ = {{0}, 0};
  bool configured_ = false;
  uint32_t started_ms_ = 0;
};

}  // namespace astromech
