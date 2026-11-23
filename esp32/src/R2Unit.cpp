#include "R2Unit.h"

namespace astromech {

bool R2Unit::rotateHead(Direction d, int speed, int ramp_ms, uint16_t delay_after) {
  return sendMotor(d, Motor::Head, speed, ramp_ms, delay_after);
}

bool R2Unit::centerHead(int speed, bool stop_at_center) {
  const uint8_t s = speed != kDefault ? speed : defaults_.head_speed;
  return send(protocol::centerHead(s, stop_at_center));
}

bool R2Unit::lookAround(int speed, bool stop_at_center) {
  bool ok = rotateHead(Direction::Left, speed);
  delay(1500);
  ok = rotateHead(Direction::Right, speed) && ok;
  delay(1500);
  if (stop_at_center) ok = centerHead(speed) && ok;
  return ok;
}

bool R2Unit::moveForward(uint32_t ms, int s, int r) {
  return moveWheels(Direction::Forward, Direction::Forward, ms, s, s, r);
}
bool R2Unit::moveBackward(uint32_t ms, int s, int r) {
  return moveWheels(Direction::Backward, Direction::Backward, ms, s, s, r);
}
bool R2Unit::spinClockwise(uint32_t ms, int s, int r) {
  return moveWheels(Direction::Forward, Direction::Backward, ms, s, s, r);
}
bool R2Unit::spinCounterClockwise(uint32_t ms, int s, int r) {
  return moveWheels(Direction::Backward, Direction::Forward, ms, s, s, r);
}
bool R2Unit::turnClockwise(uint32_t ms, int s, int r) {
  return moveWheels(Direction::Forward, Direction::Forward, ms, s, 0, r);
}
bool R2Unit::turnCounterClockwise(uint32_t ms, int s, int r) {
  return moveWheels(Direction::Forward, Direction::Forward, ms, 0, s, r);
}
bool R2Unit::driftClockwise(uint32_t ms, int s, int r) {
  return moveWheels(Direction::Forward, Direction::Forward, ms, s, s / 2, r);
}
bool R2Unit::driftCounterClockwise(uint32_t ms, int s, int r) {
  return moveWheels(Direction::Forward, Direction::Forward, ms, s / 2, s, r);
}

}  // namespace astromech
