// R2 unit demo: connect, play sounds while looking around, drive a little.
// Equivalent of python/src/samples/r2.py. One board <-> one droid.
#include <Arduino.h>

#include "R2Unit.h"

#ifndef DROID_MAC
#define DROID_MAC "F7:E6:74:95:E5:53"
#endif
#ifndef DROID_PERSONALITY
#define DROID_PERSONALITY "resistance_blue"
#endif

static astromech::R2Unit droid(DROID_MAC, astromech::findPersonality(DROID_PERSONALITY));

void setup() {
  Serial.begin(115200);
  astromech::initBle();
  Serial.printf("Connecting to %s...\n", DROID_MAC);
  if (!droid.connect()) {
    Serial.println("Connect failed (is the stock remote off and the droid awake?)");
    return;
  }
  Serial.println("Connected");

  droid.centerHead();
  droid.play(0, /*wait=*/false);
  droid.lookAround();
  droid.moveForward(1000);
  droid.spinClockwise(1000);
  droid.play(2, /*wait=*/true);
}

void loop() {
  // Heartbeat: ping every 30 s, reconnecting if the droid dropped us.
  delay(30000);
  Serial.println(droid.checkAlive() ? "droid alive" : "droid unavailable");
}
