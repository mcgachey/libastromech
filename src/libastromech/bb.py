from __future__ import annotations

from typing import cast, Optional

from libastromech.astromech import Astromech, Direction, Motor

class BB_Unit(Astromech):
  def __init__(self, mac_address, personality):
    super().__init__('bb', mac_address, personality)

  async def __aenter__(self) -> BB_Unit:
    return cast(BB_Unit, await super().__aenter__())

  async def move_forward(self, duration_ms: int, speed: Optional[int] = None, ramp_time: Optional[int] = None):
    await self._move_wheels(Direction.FORWARD, Direction.FORWARD, duration_ms, speed, speed, ramp_time)

  async def move_backward(self, duration_ms: int, speed: Optional[int] = None, ramp_time: Optional[int] = None):
    await self._move_wheels(Direction.BACKWARD, Direction.BACKWARD, duration_ms, speed, speed, ramp_time)

  async def turn_head_clockwise(self, duration_ms: int, speed: Optional[int] = None, ramp_time: Optional[int] = None):
    speed_value = speed if speed is not None else self._head_speed
    ramp_value = ramp_time if ramp_time is not None else self._head_ramp_time
    await self._move_wheels(Direction.FORWARD, Direction.BACKWARD, duration_ms, speed_value, speed_value, ramp_value)

  async def turn_head_counter_clockwise(self, duration_ms: int, speed: Optional[int] = None, ramp_time: Optional[int] = None):
    speed_value = speed if speed is not None else self._head_speed
    ramp_value = ramp_time if ramp_time is not None else self._head_ramp_time
    await self._move_wheels(Direction.BACKWARD, Direction.FORWARD, duration_ms, speed_value, speed_value, ramp_value)
