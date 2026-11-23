// BB unit demo. Equivalent of python/src/samples/bb.py.
#include <Arduino.h>

#include "BBUnit.h"

#ifndef DROID_MAC
#define DROID_MAC "E6:1C:86:B3:BD:AE"
#endif
#ifndef DROID_PERSONALITY
#define DROID_PERSONALITY "smuggler_blue"
#endif

static astromech::BBUnit droid(DROID_MAC, astromech::findPersonality(DROID_PERSONALITY));

void setup() {
  Serial.begin(115200);
  astromech::initBle();
  if (!droid.connect()) {
    Serial.println("Connect failed");
    return;
  }
  droid.play(0, false);
  droid.turnHeadClockwise(1000);
  droid.turnHeadCounterClockwise(1000);
  droid.play(1, true);
}

void loop() {
  delay(30000);
  Serial.println(droid.checkAlive() ? "droid alive" : "droid unavailable");
}
