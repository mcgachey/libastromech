#include "BBUnit.h"

namespace astromech {

bool BBUnit::moveForward(uint32_t ms, int s, int r) {
  return moveWheels(Direction::Forward, Direction::Forward, ms, s, s, r);
}
bool BBUnit::moveBackward(uint32_t ms, int s, int r) {
  return moveWheels(Direction::Backward, Direction::Backward, ms, s, s, r);
}
bool BBUnit::turnHeadClockwise(uint32_t ms, int s, int r) {
  if (s == kDefault) s = defaults_.head_speed;
  if (r == kDefault) r = defaults_.head_ramp_ms;
  return moveWheels(Direction::Forward, Direction::Backward, ms, s, s, r);
}
bool BBUnit::turnHeadCounterClockwise(uint32_t ms, int s, int r) {
  if (s == kDefault) s = defaults_.head_speed;
  if (r == kDefault) r = defaults_.head_ramp_ms;
  return moveWheels(Direction::Backward, Direction::Forward, ms, s, s, r);
}

}  // namespace astromech
