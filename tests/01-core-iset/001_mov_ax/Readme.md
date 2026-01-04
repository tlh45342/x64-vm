You’re **executing zeros** (`00`) at `0000:1000`, which almost always means:

- your binary **did not get loaded at 0x1000**, and/or
- your CPU **did not start at CS:IP = 0000:1000** (it’s still using whatever default/reset you have, likely