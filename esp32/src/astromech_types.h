#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace astromech {

// One clip on a personality chip: audio group, index in group, duration (ms).
struct Sound {
  uint8_t group;
  uint8_t sound;
  uint16_t ms;
};

struct Personality {
  const char* name;
  const Sound* sounds;
  size_t count;
};

// Motor defaults per droid type (from config/droids/*.yml).
struct DroidDefaults {
  uint8_t wheel_speed;
  uint16_t wheel_ramp_ms;
  uint8_t head_speed;
  uint16_t head_ramp_ms;
};

// High bit of the motor byte: 0x00 = forward/left, 0x80 = backward/right.
enum class Direction : uint8_t {
  Left = 0x00,
  Right = 0x80,
  Forward = 0x00,
  Backward = 0x80,
};

enum class Motor : uint8_t { Left = 0x00, Right = 0x01, Head = 0x02 };

}  // namespace astromech
