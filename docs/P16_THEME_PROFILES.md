# P16 — Theme Editor & Settings Profiles (original Cardputer K132)

This adds one more independent SD firmware image, \`THEMES.APP\` /
\`THEMES.BIN\`, and updates the **existing factory launcher and SD app
UIs** to use the same shared appearance palette.

The original Cardputer K132 has **no PSRAM**, Grove device, GPS, camera,
or 5 GHz radio requirement. All configurations are bounded, offline and
stored on the built-in FAT32 microSD.

## Six screens in the THEMES firmware

1. **Theme Editor** — Fn Left/Right preview six curated screen-color
   presets; Enter explicitly saves the selected theme for the active
   profile. The preview demonstrates the current background, header,
   foreground, selection, accents, status and warning colors.
2. **Switch Profile** — Fn Up/Down selects one of three saved profiles;
   Enter activates it. Reboot or relaunch apps to refresh the palette.
3. **Rename Profile** — Edit a name with the built-in keyboard, Enter
   validates and saves. Allowed: 1–16 ASCII letters/digits, spaces,
   underscores and hyphens. Duplicate names are refused.
4. **Brightness & Volume** — Fn Up/Down chooses a field;
   Fn Left/Right adjusts brightness in four steps or volume 0–100.
   Enter saves.
5. **Note Autosave** — Enter toggles the active profile's Notes
   autosave preference and saves. The resident factory Notes app
   applies this setting on its next boot.
6. **Backup Recovery** — R requests recovery from a validated prior
   configuration, **Y** confirms and **N** cancels. A valid current
   file is never overwritten by the recovery feature.

**B** returns from a tool to the theme submenu (when not typing a
profile name); **Ctrl+B** works when typing. Escape/backtick returns
to the resident factory launcher via the established suite OTA guard.

## Color presets

- Classic Blue — familiar original Cardputer blue/silver suite colors
- Slate — subdued gray-blue surfaces
- Terminal Green — black/green retro terminal look
- Amber — warm CRT-style gold and brown
- Violet — purple accent palette
- High Contrast — white background, black text and dark accents

The shared rendering palette replaces the original fixed colors in
the resident factory launcher, the built-in MIDI Studio screen and
all existing collection UIs. Some game-specific graphics, special
meter colors and hard-coded accent pixels remain fixed. This is a
**curated preset editor**, not an arbitrary RGB designer or WCAG
certification. Very fine font legibility needs physical 240×135 checks.

## Profiles and persistence

Exactly **three named profiles** are stored in:

\`/SUITE/CONFIG/PROFILES.CFG\`

Each profile contains a name, theme index, brightness level, speaker
volume preference and note autosave toggle. Example *structure*:

~~~text
K132PREF2
ACTIVE=0
P0=DEFAULT|0|2|60|1
P1=NIGHT|1|1|35|1
P2=CONTRAST|5|3|75|1
HASH=XXXXXXXX
~~~

The final HASH is an 8-digit FNV32 checksum calculated by the firmware;
\`XXXXXXXX\` is a placeholder, so this example is **not** a usable
configuration file. Always let the app generate the real file.

The parser accepts a maximum of 384 bytes and rejects unknown keys,
truncated files, invalid checksums, duplicate names, illegal themes or
out-of-range values. A bad configuration is never silently overwritten.

Writes go to \`PROFILES.NEW\` first, then rename the old file to
\`PROFILES.BAK\`. Corrupt primary-file recovery preserves the bad
primary as \`PROFILES.BAD\` and restores a **verified** backup only
with explicit confirmation. The old P1 \`/SUITE/CONFIG.TXT\` is
retained as fallback for brightness/Notes autosave and synchronized
when a valid P16 profile exists.

**Important:** FAT32 rename sequences are not power-loss transactions.
Back up important card contents before updating; do not pull the card
during a write. The checksum is not authentication or encryption.
No network passwords, Wi-Fi credentials, secret keys or tokens live in
these profiles.

Audio apps initialize their built-in speaker volume from the active
profile; their existing on-device volume controls still work. Theme
files are read once per application session, **not on every frame**.

## Firmware and acceptance

- Original factory firmware: shared palette, profile-aware legacy
  brightness and Notes autosave, existing factory SD app launcher.
- SD images: **23** after P16, one app installed at a time into
  the existing replaceable OTA_0 partition from removable FAT32 SD.
- No bootloader, factory/OTA partition table or underlying hardware
  changes in this pass.
- First installation of the *already-custom* factory/OTA layout still
  requires owner-approved backups and a one-time destructive erase.
- Automated host regressions: \`tests/test_theme16_core.cpp\`.
- **No physical original Cardputer has been flashed or tested.**
  Compilation does not certify SD reliability or safe USB recovery.

Physical acceptance checklist: \`docs/P16_ACCEPTANCE.md\`.
