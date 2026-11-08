# P6 — Developer Tools and System Toolbox

**Source-stage implementation, not verified on physical original Cardputer K132.**
Adds two independent SD firmware packages containing **11 additional program modes**.
The factory launcher and 8 MiB factory/OTA partition layout are unchanged.

## Developer Tools (`DEVTOOLS.BIN`, four modes)

| Mode | Function / limits |
| --- | --- |
| BASIC Mini REPL | `LET A=2+3*4`, `PRINT A/7`, `? A^2`, `CLEAR`. One-line expressions, 26 variables A–Z, parentheses, + - * / ^, division-by-zero/domain checking. **Not** a full line-numbered BASIC interpreter: no loops, file I/O, commands or saved programs yet. |
| Scientific Calculator | Square root, square, reciprocal, sin/cos/tan in degrees, ln, log10, exp, integer factorial 0–20. Rejects invalid numeric domains and overflow. Fn Up/Down chooses operation; Enter evaluates numeric input. |
| Hex/ASCII Codec | Converts printable typed text into hexadecimal or pairs of hex digits to ASCII. Nonprintable decoded bytes render as `.`. Max 48 input bytes, not a general-purpose file hex editor. |
| CHIP-8 Interpreter | Loads user-supplied `.CH8` ROMs from `/SUITE/CHIP8` (1–3584 bytes), runs a bounded 4 KiB-memory, 64×32 monochrome, 16-key interpreter. Standard 1970s opcodes, 60 Hz timer updates, bounded CPU instructions. Not SuperChip/XO-CHIP or a commercial ROM collection. |

**Developer menu:** Fn Up/Down choose mode, Enter open; **Ctrl+B** returns
to the Developer menu. **Ctrl+C** clears BASIC/Scientific/Hex input.
For the emulator, move among up to 16 available `.CH8` files using
Fn Up/Down, Enter boots; **R** rescans after adding ROMs; CHIP-8 keys map
to the `1234 / QWER / ASDF / ZXCV` physical keyboard rows.
The emulator is intentionally limited to stop on unsupported/invalid opcodes,
instead of executing arbitrary memory.

## System Toolbox (`SYSTEM.BIN`, seven modes)

| Mode | Actual behavior |
| --- | --- |
| Battery Monitor | Approximate percentage and reported millivolts using M5Cardputer Power API; rolling 16-sample voltage average/trend. **Charging status/current are not measurable reliably** on this hardware. |
| Heap & CPU | Current/free/min heap, largest allocation and CPU frequency. |
| Firmware Info | CPU model, ESP SDK version, flash size, sketch size, running partition name/address. Read only, no flashing. |
| SD Card Info | Card size and filesystem total/used space, provided by the SD library. Read only. |
| SD Read Benchmark | Reads up to 1 MiB from a packaged `.BIN` on the SD card, times transfer and estimates KiB/s. **Never writes** benchmark data to card. |
| SHA-256 File Digest | Select a `.BIN` in `/SUITE/APPS`, stream it to SHA-256, view 64-character digest. Hashing detects corruption, does not authenticate an app author. |
| Uptime Clock | Days/hours/minutes/seconds since device boot using the monotonic ESP timer. **Not a wall clock** and not persisted across reboot. |

**System menu:** Fn Up/Down, Enter. Inside System Toolbox, **B**
returns to the collection menu, **S** refreshes battery or starts the SD
read test, Fn Up/Down selects files in hash mode, Enter computes a hash.
No external modules, GPS, PSRAM or cloud service required.

## SD and memory layout

The factory launcher now recognizes **10 IDs**:
`METRONOME, PIANO, CHANNELS, BLE, ARCADE, OFFICE, INSPECT, MUSICLAB,
DEVTOOLS, SYSTEM`. The complete `SUITE/APPS` ZIP contains 10 matching
`.APP` manifests and 10 `.BIN` images; two new collection images host
11 programs. Only **one** image occupies the replaceable OTA_0 flash slot
at a time. App switching is an SD-to-OTA install, **not direct code execution
from SD**, and the flash's write cycles are finite.

**Safety:** A one-time custom-partition install erases existing flash and
requires backup and explicit approval. Never flash standalone `.BIN` images
over the factory partition. SHA256 checks content integrity, not authenticity.

See `docs/SD_LAUNCHER.md`, `docs/P6_ACCEPTANCE.md`, the P5 catalog,
and GitHub issue #3 for the rest of the unfinished 94-app plan.
