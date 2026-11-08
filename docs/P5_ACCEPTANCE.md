# P5 original K132 physical acceptance — NOT EXECUTED

**No physical Cardputer has been flashed or tested.** CI compiles and
host tests alone cannot establish that firmware switching, speaker
output, SD saves, BLE/Wi-Fi or keyboard controls work on real hardware.

## SD boot and recovery
- [ ] Factory launcher remains usable after each OTA package install.
- [ ] Eight manifests listed and checksum-verified; invalid binaries rejected.
- [ ] Each of four P5 collections boots and returns through Escape/backtick.
- [ ] B returns to the collection submenu without reflashing.
- [ ] Missing/removed SD, corrupted manifest and interrupted OTA leave
      a verified recovery procedure available.
- [ ] Retain owner firmware/SD backup; avoid destructive erase unless
      expressly authorized for first partition-table install.

## Arcade
- [ ] Snake growth, collision and direction reversal behavior.
- [ ] 2048 merge once per move, win and no-move conditions.
- [ ] Minesweeper safe first click, flags and flood reveal.
- [ ] Sokoban pushing, goal and reset.
- [ ] Pong/Breakout stable frame timing and working paddles.
- [ ] Word Guess letter tracking and failure limit.

## Pocket Office
- [ ] All four SD lists load/save and survive power cycle.
- [ ] N, Enter, D then Y/N and R work; expenses sum accurately.
- [ ] Malformed file is not silently overwritten, backup recoverable.
- [ ] No-SD state refuses save and remains navigable.
- [ ] Manual Calendar February/leap years display correctly.

## Inspector
- [ ] HEX exact byte offsets and file browsing under /SUITE.
- [ ] CSV quote handling; bounded JSON syntax display.
- [ ] File Search matches known names without unbounded recursion.
- [ ] ASCII Chart values, Storage Analyzer bounded totals.
- [ ] None of the six tools modifies source files.

## Music Practice
- [ ] Morse playback spaces, dot/dash timing and stop behavior.
- [ ] Tone frequencies and 16-step drum sequence produce sound.
- [ ] Ear Trainer higher/lower quiz scores expected answers.
- [ ] Tap tempo agrees with independent 60/120/180 BPM reference.
- [ ] Returning to factory leaves no stuck sound.

Record source SHA, test media, serial logs, hardware ID,
screen/speaker evidence, memory/battery observations and results.
Keep P5 as a draft until all physical gates pass.
