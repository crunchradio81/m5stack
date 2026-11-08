# P10 — Expanded Offline Arcade for original Cardputer K132

**Source-stage work; not physically verified on the K132.**

One more SD firmware package `ARCADE2.APP` + `ARCADE2.BIN` contains
five playable games with fixed memory, original graphics and original text.
No Grove attachments, external display, GPS, online account or PSRAM.

| Game | What is implemented | Controls |
| --- | --- | --- |
| Falling Blocks | 10×16 tile grid, seven tetromino shapes, rotation, movement, hard drop, row clearing and score | Fn arrows / WASD, Enter/Space drop, R reset |
| Checkers | Two human players on one device; diagonal movement, forced captures, chained captures, promotion to kings | Fn arrows select squares, Enter choose/move, R reset |
| Memory Match | 16 shuffled tiles forming 8 pairs, two-card flip and delayed conceal on mismatch | Fn arrows select, Enter flip, R reset |
| Virtual Pet | Uptime-based hunger, joy and energy values that gradually drain, Feed/Play/Rest actions | F feed, P play, Z rest, R reset |
| Text Adventure | Original short branching story, two choices per scene and ending paths | 1/2 choose, R restart |

**B** returns from a game to the Arcade 2 submenu.
Escape/backtick restarts the original factory launcher after a
successful SD-app boot return guard.

Limitations: Falling Blocks is a lightweight block game, not a
pixel-perfect commercial Tetris implementation. Checkers is local
two-player only; no AI, matchmaking or online play. Virtual Pet has
**no save or RTC** yet and loses its state at reboot. Text Adventure
ships one short original scenario; no external script engine, maps or
inventory. There is no Sudoku, Solitaire, Chess or large dungeon in
this milestone; they remain in the overall catalog.

The app image installs from FAT32 SD into OTA_0, not direct code
execution from SD. P10 leaves the existing 8 MiB factory+OTA partition
table unchanged. The first installation of that custom layout still
requires an explicitly approved backup and one-time flash erase.
Read `docs/SD_LAUNCHER.md`.

Portable host tests cover game mechanics, pairing, checkers moves,
millis rollover, game resets and malformed moves. Compiler builds
are not proof that all controls fit the original K132 keyboard/display.

See `docs/P10_ACCEPTANCE.md` for physical certification gates.
