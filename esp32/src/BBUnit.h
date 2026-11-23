#pragma once

#include "Astromech.h"

namespace astromech {

class BBUnit : public Astromech {
 public:
  BBUnit(const char* mac, const Personality* personality = nullptr)
      : Astromech(mac, config::k_bb_defaults, personality) {}

  bool moveForward(uint32_t ms, int speed = kDefault, int ramp_ms = kDefault);
  bool moveBackward(uint32_t ms, int speed = kDefault, int ramp_ms = kDefault);
  // BB spins its body to turn the head, so these use the head defaults.
  bool turnHeadClockwise(uint32_t ms, int speed = kDefault, int ramp_ms = kDefault);
  bool turnHeadCounterClockwise(uint32_t ms, int speed = kDefault, int ramp_ms = kDefault);
};

}  // namespace astromech
