from __future__ import annotations

import asyncio
from datetime import datetime
import yaml
import sys

from libastromech import R2_Unit, Personality, Sound

R2T2 = "F7:E6:74:95:E5:53"

async def playback_sounds():
  async with R2_Unit(R2T2, Personality('resistance_blue')) as droid:
    for idx, sound in enumerate(droid.personality.sounds):
      print(f"Playing sound {idx} [{sound.group}:{sound.sound}]")
      await droid.play(sound, wait=True)

if __name__ == '__main__':
  asyncio.run(playback_sounds())
