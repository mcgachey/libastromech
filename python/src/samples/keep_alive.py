from __future__ import annotations

import asyncio

from libastromech import Astromech, R2_Unit

def _heartbeat_failure(droid: Astromech):
  print(f"Lost connection to {droid}")

def _heartbeat_success(droid: Astromech):
  print(f"Successfully pinged {droid}")

async def test_r2():
  droid = R2_Unit("F7:E6:74:95:E5:53")
  await droid.keep_alive(
    heartbeat_success=_heartbeat_success,
    heartbeat_failure=_heartbeat_failure,
    sleep_secs=3,
  )


if __name__ == '__main__':
  asyncio.run(test_r2())
