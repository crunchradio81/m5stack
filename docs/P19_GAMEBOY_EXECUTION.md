# P19 — First Game Boy (DMG) emulator for stock K132

Status: DRAFT. No original physical K132 has yet been flashed or gameplay-tested.

- Single standalone GAMEBOY OTA_0 firmware, keeps the factory + 3MiB app partition layout.
- MIT Peanut-GB 1.3.0 vendored verbatim from deltabeard/Peanut-GB,
  commit d0bcca771c83a2638c93a9ae61f3d51f226dc905, full license header retained.
- Original monochrome DMG only. Not native Game Boy Color. Sound disabled in P19.
- ROMs: owner-supplied FAT32 files in /SUITE/ROMS/GB (not packaged or published).
- Bank cache: 2x16KiB; demand-load from SD; ROM size ceiling 8MiB.
- Frame time 17ms target, instruction budget 100,000 and 250ms time ceiling;
  observed real-device speed and power use not yet verified.
- 150x135 RGB565 framebuffer, scaled from 160x144 within stock 240x135 LCD.
- E/S/A/D move; K/L = A/B; 1/2 = Start/Select; Esc returns to factory.
- Battery SRAM: exact-length loads; on exit attempt .NEW write, byte verify,
  existing file to .BAK and final readback. Existing .BAK/.NEW are protected.
  A failed exit-time save has no safe on-device retry interface yet. FAT32 is
  not power-loss-safe; do not rely on save integrity without manual backup.

Host tests:
tests/test_gameboy19_emulator.c links the vendored core and executes an
original, synthetic 32KiB GB ROM for four bounded CPU/LCD frames. This is not
proof of physical gameplay. tools/generate-gb-test-rom.py creates the same ROM.

Hardware acceptance required before releasing:
1. Owner backup and one-time factory partition adoption approved.
2. Verify OTA app install, reboot to factory, source ROM untouched.
3. Run generated STRIPES.GB on actual original K132 and photograph display.
4. Validate keyboard holds, speed, RAM heap, microSD bank paging and hot removal.
5. Run legal DMG homebrew ROMs (ROM-only and MBC1/2/3/5); observe performance.
6. Validate battery SRAM save and restore, corrupted data and restart recovery.
7. Test USB recovery and every prior application still opens from factory.
8. Add audio and native CGB only in later tested development stages.

No Nintendo ROMs, boot code, or commercial game images are included.
