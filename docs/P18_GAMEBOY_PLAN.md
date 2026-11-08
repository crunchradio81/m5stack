# P18 — Game Boy/Game Boy Color preflight, original Cardputer K132

Status: **source-only preflight, not yet an emulator or a flashable Game Boy app**.
This pass deliberately does not add a 26th OTA image or ROMs.

## Why this is feasible
The original stock K132 has an ESP32-S3FN8 dual core (240 MHz), 8 MiB flash,
no PSRAM, a 240x135 SPI TFT, I2S mono sound, keyboard and microSD.
Working emulators specifically targeting Cardputer exist:
- https://github.com/geo-tp/Cardputer-Game-Station-Emulators
  (GnuBoy-based console collection, GB/GBC, SD saves and audio; its top-level
  project uses MIT, but audit *each included emulation core* before reuse).
- https://github.com/Mr-PauI/Walnut-CGB (MIT-licensed GB/GBC core, includes
  original Cardputer example; actual SD paging and APU still must be adapted).
- https://github.com/matteofo/gb_cardputer (original Peanut-GB proof of concept,
  reports no audio; not preferred for shipping).

## Suite-specific integration plan
1. Preserve the P17 factory + 3 MiB replaceable OTA_0 partition scheme.
   Do NOT substitute upstream Game Station's 5 MiB/other partition layout.
2. Port a **single** licensed GB/DMG core first, not the entire multi-console
   emulator suite. Prefer Walnut-CGB with cartridge paging over FAT32 for
   future GB/GBC support; independently benchmark the Game Station GnuBoy
   implementation before selecting the final engine.
3. ROMs live only on user-supplied FAT32 microSD under /SUITE/ROMS/GB/
   and are **never included in GitHub commits or firmware artifacts**.
   Recognize .GB and .GBC; do not walk outside this fixed directory.
   Read only the first 0x150 bytes to validate cartridge metadata.
   Unsupported cartridge mappers should be reported, never silently run.
4. Maintain a small ROM bank cache rather than RAM-copying entire cartridges.
   Test MBC1, MBC2, MBC3+RTC, MBC5, battery RAM persistence and >1MiB games.
   Any flash/partition fallback must be separately reviewed; no automatic
   repartitioning, flashing ROM payloads or erase on first launch.
5. Adapt existing built-in keyboard: E/S/A/D for D-pad, K/L for A/B,
   1/2 for Start/Select. Long-hold G0 should return to suite launcher,
   AFTER explicitly closing/flushing pending saves.
6. Use a 150x135 pixel destination for 160x144 native frame (exact 15/16
   aspect-preserving reduction). In the first renderer use bounded integer
   nearest-neighbor sampling. Verify text legibility and scanline bandwidth
   on the actual LCD; optionally add center-crop / pan mode.
7. Route synthesized 4-channel Game Boy audio to stock I2S speaker, with a
   bounded queue and no simultaneous microphone recording.
8. Place SRAM saves and optional save states in /SUITE/SAVES/GB; use a
   temporary file, read-back validation, non-overwriting backup and explicit
   error messages. FAT32 is not transactional. No save claims without tests.
9. Add a dedicated `sd_gameboy` / `GAMEBOY.APP` firmware only when the
   imported emulator runs real open-licensed homebrew ROMs in the 3 MiB OTA
   image, with a host-side test ROM and no unlicensed BIOS/ROM data.

## What P18 code actually verifies
`src/core/GameBoy18Core.h` contains ROM filename safety, GB cartridge-header
checksum and declared size checks, minimal title/CGB metadata, fixed 150x135
sample mapping and proposed keyboard mapping. It is portable and does not
include an emulator, flash writes, or any external engine. Corresponding
`tests/test_gameboy18_core.cpp` uses synthetic headers only. This is **not**
a claim that actual games run in the Silicon suite build.

## Release gates
- P17 hardware: owner-approved backup and USB factory install; stock original
  K132 keyboard, display, SD removal and reboot/OTA round-trip.
- P18 software: license audit, upstream pinned revision, SDK compatibility,
  cartridge bank paging, mapper regression ROMs, audio and frame pacing,
  safe saves, bounded 3 MiB image, OTA/factory return.
- P18 hardware: actual device run, multiple homebrew games, restart after
  saving, low heap, hot SD removal, recovery through USB and power cycle.
- All items remain **unverified** unless logs and hardware reports show PASS.

No copyrighted commercial ROMs, BIOS images, or game assets are distributed.
