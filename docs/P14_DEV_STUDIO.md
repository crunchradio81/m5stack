# P14 — Developer Studio II, original Cardputer K132

The `DEV2.APP`/`DEV2.BIN` collection has five standalone offline
program screens (one replaceable OTA firmware package). No PSRAM or
Grove components are used.

**BASIC Program Studio** builds upon the earlier expression interpreter:
up to 64 numbered lines (1..9999), each max 80 printable ASCII chars,
sorted insert/delete/replace, commands `LET`, `PRINT`, `CLEAR`,
`REM`, `GOTO`, `IF expression < > = expression THEN line`,
and `END`. The `RUN` command executes a maximum of 1000 statements,
stopping with an explicit error on an infinite loop, missing target
or invalid statement. `SAVE`/`LOAD` manage
`/SUITE/DEV/PROGRAM.BAS` with `.NEW`/`.BAK` previous-copy protection.
This **is a bounded BASIC dialect, not full GW-BASIC/QBASIC compatibility**:
no arrays, strings, FOR/NEXT, INPUT, file/network I/O or screen graphics.

**Syntax Color Editor** is a single-file, 1,023-byte ASCII/LF code
editor saving `/SUITE/DEV/CODE.TXT`. It distinguishes uppercase
letters and digits visually. This is **character-category coloring,
not a full token-aware programming-language syntax highlighter**.

**Bounded Hex Editor** opens only `/SUITE/DEV/DATA.BIN`, up to 256
bytes, with byte-oriented editable cursor, 00–FF hex input, append,
explicit Ctrl+S save and previous `.BAK` recovery copy. It does not
modify system/OTA firmware files, the SD root or arbitrary paths.

**USB Serial Console** receives and sends bounded ASCII lines on the
board's configured USB CDC serial port (115200 configured in the suite),
without network access. Enter transmits a typed message; incoming
ASCII lines appear in a rolling five-line display. Not a USB host
terminal for peripheral devices or an unrestricted shell.

**SD Content Search** searches up to 20 ordinary local files of at
most 4KiB each under `/SUITE/USER`, displaying at most four matching
filenames. It never modifies the searched files or scans firmware
images. Queries are case-insensitive ASCII substrings.

The app adds **21 SD firmware images total** to the complete library,
without changing the factory+OTA table. All images remain on FAT32 SD
and install into OTA_0 after explicit confirmation. No real original
Cardputer has been flashed or QA-tested.

Portable host tests: `tests/test_dev14_core.cpp`. Firmware target:
`pio run -e sd_dev2`. Physical acceptance checklist:
`docs/P14_ACCEPTANCE.md`.
