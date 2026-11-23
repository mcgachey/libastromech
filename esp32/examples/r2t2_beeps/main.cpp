// Bring-up test driver: on every power-on, connect to R2-T2 and run the same
// sequence as run_demo() in astromech_server/src/server.py, then disconnect and
// idle. Configuration is hard-coded on purpose.
//
//   run_demo:  center_head
//              gather(look_around, play first 7 sounds with wait=True)
//              center_head
//
// Python's asyncio.gather becomes a second FreeRTOS task for the head movement;
// the library's mutex serialises the BLE writes between the two tasks.
//
// Progress is printed on the serial port (115200) so you can watch with
// `pio device monitor`. The board's LED (GPIO 2 on most dev boards) is on while
// working, blinks fast on failure, and is off when done successfully.
#include <Arduino.h>

#include "R2Unit.h"

static const char* const kMac = "F7:E6:74:95:E5:53";  // R2-T2
static const char* const kPersonality = "resistance_blue";
static const size_t kDemoSounds = 7;  // personality.sounds[0:7] in run_demo
static const int kConnectAttempts = 5;
static const uint8_t kLedPin = 2;

static astromech::R2Unit droid(kMac, astromech::findPersonality(kPersonality));
static SemaphoreHandle_t head_done;
static volatile bool head_ok = false;

static void fail(const char* why) {
  Serial.printf("FAILED: %s\n", why);
  for (;;) {  // fast blink forever; reset the board to retry
    digitalWrite(kLedPin, !digitalRead(kLedPin));
    delay(100);
  }
}

// droid.look_around(): left, 1.5 s, right, 1.5 s, center.
static void headTask(void*) {
  Serial.println("[head] look_around");
  head_ok = droid.lookAround();
  Serial.printf("[head] look_around %s\n", head_ok ? "done" : "FAILED");
  xSemaphoreGive(head_done);
  vTaskDelete(nullptr);
}

// play_sounds(): first 7 sounds, each waited out.
static bool playSounds() {
  const size_t n = min(kDemoSounds, droid.personality()->count);
  for (size_t i = 0; i < n; i++) {
    const astromech::Sound& s = droid.personality()->sounds[i];
    Serial.printf("[audio] %u/%u: group %u sound %u (%u ms)\n", (unsigned)(i + 1), (unsigned)n,
                  s.group, s.sound, s.ms);
    if (!droid.play(s, /*wait=*/true)) return false;
  }
  return true;
}

static bool runDemo() {
  Serial.println("center_head");
  if (!droid.centerHead()) return false;

  head_done = xSemaphoreCreateBinary();
  xTaskCreate(headTask, "head", 8192, nullptr, 1, nullptr);
  bool audio_ok = playSounds();
  xSemaphoreTake(head_done, portMAX_DELAY);  // gather: wait for both
  vSemaphoreDelete(head_done);
  if (!audio_ok) fail("audio failed");
  if (!head_ok) fail("look_around failed");

  Serial.println("center_head");
  return droid.centerHead();
}

void setup() {
  Serial.begin(115200);
  delay(500);
  pinMode(kLedPin, OUTPUT);
  digitalWrite(kLedPin, HIGH);
  Serial.printf("\nR2-T2 demo: %s (%s)\n", kMac, kPersonality);

  if (!droid.personality()) fail("personality not found");
  astromech::initBle("r2t2-test");

  bool connected = false;
  for (int i = 1; i <= kConnectAttempts && !connected; i++) {
    Serial.printf("Connecting (attempt %d/%d)...\n", i, kConnectAttempts);
    connected = droid.connect(15000);
    if (!connected) delay(2000);
  }
  if (!connected) fail("could not connect (droid on? stock remote off? in range?)");
  Serial.println("Connected");

  if (!runDemo()) fail("demo failed");

  droid.disconnect();
  Serial.println("Done. Reset the board to run again.");
  digitalWrite(kLedPin, LOW);
}

void loop() { delay(1000); }
