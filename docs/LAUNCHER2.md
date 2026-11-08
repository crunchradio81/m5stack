# Launcher 2.0 — original stock Cardputer K132

**Development-stage factory firmware. Hardware field testing is still required.**

The launcher remains in the dedicated factory partition of the original
K132's 8 MiB flash. Executable application `.BIN` files are stored on
FAT32 SD card with `.APP` manifests and SHA-256 integrity checks.
Choosing an app installs it into the separate replaceable OTA_0 slot,
then reboots. **ESP32 application binaries cannot execute directly
from the SD card.**

## New user interface

- **C** cycles categories: ALL, MUSIC, WIRELESS, GAMES, OFFICE, TOOLS,
  INFRARED, FAVORITES and RECENT. Groups are based on known suite app IDs;
  unrecognized app IDs remain rejected by the manifest allowlist.
- **Fn Up/Down** navigates filtered results.
- **F** toggles a favorite on the highlighted app (maximum 16).
- **V** switches to RECENT (last four successfully installed apps).
- **S** enters search; type up to 16 ASCII letters/numbers/spaces/hyphens.
  Search matches app ID and display name, case-insensitively. Enter ends
  search; Backspace in normal browsing clears an existing query.
- **R** rescans SD manifests and reloads preferences.
- **Enter** opens an install confirmation and **Y/N** confirms/cancels.
  This preserves the explicit user approval for copying firmware from SD
  into the replaceable OTA partition. No automatic installation starts
  from simply inserting a card.

The 240×135 UI shows the current category, visible count, favorite
markers and up to six results at a time.

## SD preferences and recovery

Favorites and recent app IDs live in:

~~~text
/SUITE/APPS/LAUNCHER.CFG
K132MENU1
F=ARCADE
F=MIDITOOLS
R=MIDITOOLS
R=ARCADE
~~~

This is an example, not a default file. The application itself writes
the file when you save a favorite or launch an app.

The on-device parser has a strict 512-byte cap, known app-ID
allowlist, duplicate checks, 16 favorite limit and 4 recent entries.
The launcher saves to `LAUNCHER.NEW` first, keeps the previous file as
`LAUNCHER.BAK`, and refuses to overwrite malformed saved preferences.
Do not assume FAT32 writes are power-loss transactional; keep SD backups.

## Build, firmware and safety

The Launcher 2.0 source is in `src/SDAppLauncher.h`, the deterministic
filter/preferences model in `src/core/Launcher2Core.h`, and host tests
in `tests/test_launcher2_core.cpp`.

**Important:** Launcher 2.0 modifies the factory firmware, not just a
standalone SD app. Upgrading from the earlier launcher requires an
intentional USB firmware update after preserving existing factory/NVS
settings. It does not change the established factory+OTA partition
layout, but initial adoption of that layout still requires a
user-approved backup and one-time destructive flash erase.

The existing on-device OTA installer still checks image magic, size,
SHA-256 and the expected running/target partitions before writing.
A digest only checks accidental corruption, not code authenticity,
and the factory partition is not hardware write-protected.

**Physical hardware acceptance and recovery remain unverified.**
See `docs/LAUNCHER2_ACCEPTANCE.md`.
