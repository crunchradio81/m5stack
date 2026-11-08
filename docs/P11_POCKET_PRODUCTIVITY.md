# P11 — Five offline Pocket Productivity SD applications

Target: **original stock M5Stack Cardputer K132**, 8MB flash,
no PSRAM, no Grove modules or external keyboard. All app data is
owner-controlled on the built-in FAT32 SD card.

`POCKET11.APP` / `POCKET11.BIN` is the new SD collection. Its
five program screens are:

1. **SD Text Editor** — edits a single `/SUITE/POCKET/NOTE.TXT` file
   as bounded printable ASCII plus LF. Maximum 1023 bytes in RAM.
   Left/Right move cursor, Backspace deletes, Enter inserts newline,
   **Ctrl+S** saves and **Ctrl+B** returns to menu. This is not yet a
   multi-file rich-text document editor.
2. **Spreadsheet Mini** — six rows and six columns of signed integers
   between -99,999 and 99,999. Arrow keys choose a cell, type an
   integer, Enter commits, Ctrl+S saves; row sum is shown. CSV is
   stored at `/SUITE/POCKET/SHEET.CSV` with `SHEET1` header.
   This is not a formula or macro engine and never executes cells.
3. **Flashcard Trainer** — reads up to 20 question/answer pairs from
   `/SUITE/POCKET/CARDS.TXT`; each line is `Question|Answer` with
   max 41 ASCII chars per side. Enter reveals answer, Fn arrows
   navigate, slash begins case-insensitive question search, R reloads.
   Decks are read-only; author files on another computer.
4. **QR Code Generator** — encodes up to 42 ASCII characters in a
   version 3 QR code using ricmoo's low-memory QRCode library;
   rendered to the original 240×135 display, no network required.
   This is a static display, not a QR scanner or camera app. Scan
   reliability needs real device/phone acceptance testing.
5. **Random String Maker** — generates an 8–32 character random
   alphanumeric string using the ESP random interface with bounded
   rejection sampling. **Not an audited cryptographic password
   manager**; random quality depends on hardware entropy state.
   No password vault/storage/copy/transmission is implemented.
   The string is cleared when leaving the collection.

### SD save behavior

Text and spreadsheet saves are explicit, write to a new `.NEW`
file first and retain the previous file as `.BAK`. Invalid saved
content is protected against overwriting until corrected on another
computer. FAT32 is not guaranteed power-loss safe: always back up
important files; do not remove the card during writes.

### Compilation and launch

P11 adds the `POCKET11` ID to the existing SD application
manifest allowlist. The factory launcher will present **18**
independent SD firmware packages after this branch is built. P11
does not change the factory/OTA partition table: one chosen SD binary
is installed into the replaceable OTA_0 slot, never run directly from
SD. First installation of this custom partition scheme still requires
an owner-approved backup and destructive flash erase.

No physical Cardputer has been tested. See `docs/P11_ACCEPTANCE.md`.
