# P14 stock original Cardputer acceptance — not performed

- [ ] Original K132 (8MB flash, no PSRAM/Grove) identified; firmware/NVS/SD backed up.
- [ ] DEV2 appears in launcher, verified manifest, boots OTA_0 and returns to factory.
- [ ] BASIC: numbered line order, LET/PRINT, GOTO, IF/THEN and END; bounded
      infinite-loop and missing-target tests; Save/Load persists data.
- [ ] Syntax editor: 1,023-character limits, newline/Backspace/arrow navigation,
      Ctrl+S save and protected malformed original file.
- [ ] Hex editor: no writes outside DATA.BIN, 256-byte maximum, correct byte
      modification with backup and verification after reboot.
- [ ] USB CDC: two-way ASCII over original board's USB port, no lockups.
- [ ] SD content search: case-insensitive name/body matches, no traversal,
      20-file/4KiB bounds and no SD file mutation.
- [ ] Repeated mode exits stable, no I2S/radio/heap regressions.
- [ ] Source SHA, original board photos, UART, SD SHA and factory recovery proof attached.
- [ ] Keep draft until physical acceptance succeeds.
