from __future__ import annotations

import asyncio
from datetime import datetime
import yaml
import sys

from libastromech import R2_Unit, Personality, Sound

R2T2 = "F7:E6:74:95:E5:53"

async def discover_sounds(output_file: str):
  print(f"Writing sound data to {output_file}")
  sounds = []
  async with R2_Unit(R2T2, Personality('navy_blue')) as droid:
    for group in range(10):
      await droid.set_audio_group(group)
      for sound in range(100):
        print(f"Playing {group}:{sound}")
        await droid.play_sound_from_current_group(sound)
        played = input('Did a sound play? [Yn] ')
        if played and played.lower() == 'n':
          break
        else:
          complete = False
          while not complete:
            input(f"Measuring timing for {group}:{sound}. Hit [Enter] to start, then again once the sound ends.")
            start = datetime.now()
            await droid.play_sound_from_current_group(sound)
            input(f"Hit [Enter] when the sound ends.")
            end = datetime.now()
            diff = (end - start)
            time = (diff.seconds * 1000) + round(diff.microseconds / 1000)
            success = input(f"Sound {group}:{sound} played for {time}ms. Write sound data? [Yn] ")
            complete = not (success and success.lower() == 'n')
          sounds.append(Sound(group, sound, time))
          with open(output_file, 'w') as f:
            yaml.safe_dump({'sounds': [{'group': x.group, 'sound': x.sound, 'ms': x.ms} for x in sounds]}, f)

async def playback_sounds(config_file: str):
  with open(config_file) as f:
    sounds = [Sound(x['group'], x['sound'], x['ms']) for x in yaml.safe_load(f)['sounds']]
  async with R2_Unit(R2T2, Personality('navy_blue')) as droid:
    for idx, sound in enumerate(sounds):
      print(f"Playing sound {idx} [{sound.group}:{sound.sound}]")
      await droid.play(sound, wait=True)

if __name__ == '__main__':
  asyncio.run(discover_sounds(sys.argv[1]))
  asyncio.run(playback_sounds(sys.argv[1]))
