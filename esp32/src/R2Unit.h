#pragma once

#include "Astromech.h"

namespace astromech {

class R2Unit : public Astromech {
 public:
  R2Unit(const char* mac, const Personality* personality = nullptr)
      : Astromech(mac, config::k_r2_defaults, personality) {}

  bool rotateHead(Direction d, int speed = kDefault, int ramp_ms = kDefault, uint16_t delay_after = 0);
  bool centerHead(int speed = kDefault, bool stop_at_center = true);
  // Blocks ~3 s: left, right, then (optionally) center.
  bool lookAround(int speed = kDefault, bool stop_at_center = true);

  bool moveForward(uint32_t ms, int speed = kDefault, int ramp_ms = kDefault);
  bool moveBackward(uint32_t ms, int speed = kDefault, int ramp_ms = kDefault);
  bool spinClockwise(uint32_t ms, int speed = kDefault, int ramp_ms = kDefault);
  bool spinCounterClockwise(uint32_t ms, int speed = kDefault, int ramp_ms = kDefault);
  bool turnClockwise(uint32_t ms, int speed = kDefault, int ramp_ms = kDefault);
  bool turnCounterClockwise(uint32_t ms, int speed = kDefault, int ramp_ms = kDefault);
  bool driftClockwise(uint32_t ms, int speed = 0xa0, int ramp_ms = kDefault);
  bool driftCounterClockwise(uint32_t ms, int speed = 0xa0, int ramp_ms = kDefault);
};

}  // namespace astromech
