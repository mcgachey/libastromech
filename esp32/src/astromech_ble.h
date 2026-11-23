#pragma once

#include <NimBLEDevice.h>

namespace astromech {

// Initialise the BLE stack once at startup (before Astromech::connect() or
// Beacon::start()).
inline void initBle(const char* device_name = "") { NimBLEDevice::init(device_name); }

}  // namespace astromech
