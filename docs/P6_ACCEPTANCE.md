# P6 — Original Cardputer K132 hardware acceptance

**NOT PERFORMED.** All host tests/CI compilation results only certify
portable logic and firmware buildability, not real Cardputer I/O.

## Launcher and SD
- [ ] Both `DEVTOOLS` and `SYSTEM` manifests appear in factory launcher,
      alongside eight earlier app packages.
- [ ] SHA-256 and size validation precede install into OTA_0.
- [ ] Boot each app and Escape/backtick returns to factory.
- [ ] Remove SD: useful errors without a crash; no writes from system tools.
- [ ] Verify factory partition remains unchanged after app swapping.

## Developer Tools
- [ ] BASIC LET, PRINT, CLEAR with variables A..Z and operator precedence.
- [ ] Reject 1/0, invalid syntax, too many nested parentheses, overflow.
- [ ] Scientific functions return expected known values and reject invalid
      sqrt/log/tan/factorial inputs.
- [ ] Hex/ASCII codec converts known text; odd/malformed hex is refused.
- [ ] Copy small owner-authored CHIP8 ROM into `/SUITE/CHIP8`; choose/load.
- [ ] CHIP8 display, keyboard, timers and sound timer verified (sound optional
      until implemented in renderer), and invalid opcodes halt safely.
- [ ] Long-running emulation keeps heap and frame rate stable with no PSRAM.

## System Toolbox
- [ ] Battery percentage/voltage update; invalid measurements handled safely.
- [ ] Verify no unsupported battery charging-current claims.
- [ ] Free/min heap and CPU speed correspond to serial diagnostics.
- [ ] Firmware partition address/sketch/flash size reported accurately.
- [ ] Card/filesystem storage sizes shown without modifying card.
- [ ] Read-only benchmark scans bounded app data; no extra data files created.
- [ ] SHA-256 digest matches `sha256sum` of exact source .BIN on workstation.
- [ ] Uptime increments through normal operation and resets after power cycle.

## Evidence required
Board SKU K132/StampS3, source SHA, build run, UART logs,
physical screenshots, battery/SD test logs, CHIP8 test ROM hashes,
OTA factory-return proof, backup and USB recovery plan.
Keep the PR draft until these physical tests pass.
