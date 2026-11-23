// Dedicated beacon board: broadcasts a personality beacon forever. The server's
// default is a "silent" beacon (affiliation 0x0d) which keeps nearby droids awake
// and stops them reacting to each other. Swap in locationBeacon() to trigger a
// location reaction instead.
#include <Arduino.h>

#include "Beacon.h"

static astromech::Beacon beacon;

void setup() {
  Serial.begin(115200);
  astromech::initBle("beacon");
  using namespace astromech::protocol;
  auto payload = personalityBeacon(Affiliation::Silent, kChipRSeriesDefault);
  Serial.println(beacon.start(payload) ? "Broadcasting" : "Beacon start failed");
}

void loop() {
  // Watchdog: restart advertising if it ever stops.
  if (!beacon.ensureRunning()) Serial.println("Beacon not running");
  delay(5000);
}
