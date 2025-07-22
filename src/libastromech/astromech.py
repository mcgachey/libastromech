from __future__ import annotations

import asyncio
import enum
from pathlib import Path
from typing import Callable, List, Optional
import yaml
from bleak import BleakScanner, BleakClient
from bleak.backends.characteristic import BleakGATTCharacteristic
from bleak.exc import BleakDeviceNotFoundError

class Personality(object):
  def __init__(self, name: str):
    with open(Path(__file__).parent.parent / 'config' / 'personalities' / f"{name}.yml") as f:
      config = yaml.safe_load(f)
    self.sounds = [Sound(x['group'], x['sound'], x['ms']) for x in config['sounds']]

class Sound(object):
  def __init__(self, group: int, sound: int, ms: int):
    self.group = group
    self.sound = sound
    self.ms = ms

class DiscoveredDevice(object):
  def __init__(self, mac_address: str, name: str):
    self.mac_address = mac_address
    self.name = name

  def __str__(self):
    return f"{self.name}: {self.mac_address}"

async def scan(bt_names: Optional[List[str]] = None) -> List[DiscoveredDevice]:
  devices: List[DiscoveredDevice] = []
  names = bt_names if bt_names else ['DROID']
  for d in await BleakScanner.discover():
    if d.name in names:
      print(f"Found droid {d.name}: {d.details}")
      devices.append(DiscoveredDevice(mac_address=d.details['props']['Address'], name=d.name))
  return devices

class Direction(enum.Enum):
  LEFT = 0x00
  RIGHT = 0x80
  FORWARD = 0x00
  BACKWARD = 0x80

class Motor(enum.Enum):
  LEFT = 0x00
  RIGHT = 0x01
  HEAD = 0x02

class Astromech(object):
  def __init__(
      self, 
      droid_type: str,
      mac_address: str, 
      personality: Optional[Personality]
    ):
    with open(Path(__file__).parent.parent / 'config' / 'droids' / f"{droid_type}.yml") as f:
      config = yaml.safe_load(f)
    self._wheel_ramp_time = config['wheels']['motor_ramp_time']
    self._wheel_speed = config['wheels']['speed']
    self._head_ramp_time = config['head']['motor_ramp_time']
    self._head_speed = config['head']['speed']
    self.mac_address = mac_address
    self.personality = personality
    self._client: BleakClient
    self._notification_listeners = []

  async def __aenter__(self) -> Astromech:
    self._client = BleakClient(self.mac_address)
    await self._client.connect()
    await self._client.start_notify(
      self._client.services.characteristics[10], 
      self._notification_callback
    )
    await self._execute(bytearray([0x22, 0x20, 0x01]))
    await self._execute(bytearray([0x22, 0x20, 0x01]))
    return self

  async def __aexit__(self, exception_type, exception_value, exception_traceback):
    if self._client and self._client.is_connected:
      await self._client.disconnect()

  def _notification_callback(self, sender: BleakGATTCharacteristic, data: bytearray):
    for c in self._notification_listeners:
      c(data)

  def listen_for_notifications(self, callback):
    self._notification_listeners.append(callback)

  async def keep_alive(
      self, 
      heartbeat_success: Optional[Callable[[], Astromech]],
      heartbeat_failure: Optional[Callable[[], Astromech]],
      sleep_secs: int = 30
    ):
    while True:
      try:
        async with self:
          await self.ping()
          heartbeat_success(self)
      except Exception as e:
        if type(e) == BleakDeviceNotFoundError:
          print(f"Droid offline")
          heartbeat_failure(self)
        else:
          print(f"Error: {e}, {type(e)}")
      await asyncio.sleep(sleep_secs)

  async def set_audio_group(self, group_id: int):
    return await self._execute(self._audio_command(0x1f, group_id))

  async def play_sound_from_current_group(self, sound_id: int):
    return await self._execute(self._audio_command(0x18, sound_id))

  async def play(self, sound: Sound, wait: bool = False):
    await self.set_audio_group(sound.group)
    await self.play_sound_from_current_group(sound.sound)
    if wait:
      await asyncio.sleep(float(sound.ms) / 1000.0)

  async def ping(self):
    await self._execute(self._command(0x02, bytearray([0, 0])))

  async def _move_wheels(
      self, 
      left_direction: Direction, 
      right_direction: Direction,
      duration_ms: int,
      left_speed: Optional[int], 
      right_speed: Optional[int],
      ramp_time: Optional[int],
    ):
    print(f"Left speed: {left_speed}, right speed: {right_speed}")
    await self._execute(self._motor_command(left_direction, Motor.LEFT, left_speed, ramp_time))
    await self._execute(self._motor_command(right_direction, Motor.RIGHT, right_speed, ramp_time))
    await self.stop(delay_ms=duration_ms)

  async def stop(self, delay_ms: int = 0):
    await asyncio.sleep(delay_ms / 1000)
    await self._execute(self._motor_command(Direction.FORWARD, Motor.LEFT, 0))
    await self._execute(self._motor_command(Direction.FORWARD, Motor.RIGHT, 0))

  def _audio_command(self, cmd: int, param: Optional[int] = None):
    command_data = bytearray([0x44, 0x00, cmd])
    if param is not None:
      command_data.append(param)
    return self._command(0x0f, command_data)

  def _motor_command(
      self,
      direction: Direction, 
      motor: Motor, 
      speed: int | None = None, 
      ramp_time: int | None = None,
      delay_after: int = 0,
    ):
    speed_value = speed if speed is not None else self._head_speed if motor == Motor.HEAD else self._wheel_speed
    ramp_value = ramp_time if ramp_time is not None else self._head_ramp_time if motor == Motor.HEAD else self._wheel_ramp_time
    command_data = bytearray([direction.value | motor.value, speed_value])
    command_data += _int_to_bytes(ramp_value)
    command_data += _int_to_bytes(delay_after)
    return self._command(0x05, command_data)

  def _command(self, command_id: int, command_data: Optional[bytearray]=None):
    cmd_len = len(command_data) if command_data else 0
    data = bytearray()
    data.append(0x1f + 4 + cmd_len)
    data.append(0x42)
    data.append(command_id)
    data.append(0x40 + cmd_len)
    if command_data:
      data += command_data  
    return data

  async def _execute(self, command: bytearray):
    print(f"Sending {_dump_bytes(command)}")
    response = await self._client.write_gatt_char(
      self._client.services.characteristics[13], command, 
      response=True,
    )
    if response:
      print(f"Response: {response}")
    return response

def _dump_bytes(data: bytearray):
  return data.hex(bytes_per_sep=1, sep=' ')

def _int_to_bytes(i: int):
  return i.to_bytes(2, 'big')
