from __future__ import annotations

import asyncio
from typing import cast, Optional

from libastromech.astromech import Astromech, Direction, Motor, Personality

class R2_Unit(Astromech):
  def __init__(self, mac_address, personality: Optional[Personality]=None):
    super().__init__('r2', mac_address, personality)

  async def __aenter__(self) -> R2_Unit:
    return cast(R2_Unit, await super().__aenter__())

  async def rotate_head(
      self, 
      direction: Direction, 
      speed: Optional[int] = None, 
      ramp_time: Optional[int] = None,
      delay_after: int = 0,
    ):
    return await self._execute(self._motor_command(direction, Motor.HEAD, speed, ramp_time, delay_after))

  async def center_head(
      self, 
      speed: Optional[int] = None, 
      stop_at_center: bool = True,
    ):
    speed_value = speed if speed is not None else self._head_speed
    return await self._execute(self._command(
      0x0f, 
      bytearray([0x44, 0x01, speed_value, 0x00 if stop_at_center else 0x01]))
    )

  async def look_around(self, speed: Optional[int] = None, stop_at_center: bool = True,):
    await self.rotate_head(Direction.LEFT, speed)
    await asyncio.sleep(1.5)
    await self.rotate_head(Direction.RIGHT, speed)
    await asyncio.sleep(1.5)
    if stop_at_center:
      await self.center_head(speed)

  async def move_forward(self, duration_ms: int, speed: Optional[int] = None, ramp_time: Optional[int] = None):
    await self._move_wheels(Direction.FORWARD, Direction.FORWARD, duration_ms, speed, speed, ramp_time)

  async def move_backward(self, duration_ms: int, speed: Optional[int] = None, ramp_time: Optional[int] = None):
    await self._move_wheels(Direction.BACKWARD, Direction.BACKWARD, duration_ms, speed, speed, ramp_time)

  async def spin_clockwise(self, duration_ms: int, speed: Optional[int] = None, ramp_time: Optional[int] = None):
    await self._move_wheels(Direction.FORWARD, Direction.BACKWARD, duration_ms, speed, speed, ramp_time)

  async def spin_counter_clockwise(self, duration_ms: int, speed: Optional[int] = None, ramp_time: Optional[int] = None):
    await self._move_wheels(Direction.BACKWARD, Direction.FORWARD, duration_ms, speed, speed, ramp_time)

  async def turn_clockwise(self, duration_ms: int, speed: Optional[int] = None, ramp_time: Optional[int] = None):
    await self._move_wheels(Direction.FORWARD, Direction.FORWARD, duration_ms, speed, 0, ramp_time)

  async def turn_counter_clockwise(self, duration_ms: int, speed: Optional[int] = None, ramp_time: Optional[int] = None):
    await self._move_wheels(Direction.FORWARD, Direction.FORWARD, duration_ms, 0, speed, ramp_time)

  async def drift_clockwise(self, duration_ms: int, speed: int = 0xa0, ramp_time: Optional[int] = None):
    await self._move_wheels(Direction.FORWARD, Direction.FORWARD, duration_ms, speed, int(float(speed)/2.0), ramp_time)

  async def drift_counter_clockwise(self, duration_ms: int, speed: int = 0xa0, ramp_time: Optional[int] = None):
    await self._move_wheels(Direction.FORWARD, Direction.FORWARD, duration_ms, int(float(speed)/2.0), speed, ramp_time)
