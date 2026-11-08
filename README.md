# Cardputer Suite — original M5Stack Cardputer K132

One native, statically linked firmware for the **stock original 2023 M5Stack Cardputer (K132 / StampS3)**. No Grove unit, external sensor, soldering or expansion board. A FAT32 microSD is optional and uses the device's **built-in slot**.

**P1 status: source-only development milestone. Not flashed or hardware-certified.** This repository currently needs GitHub CI and an actual K132 acceptance run. Back up your existing firmware and card before flashing.

## Included P1 programs

| Program | P1 capability |
| --- | --- |
| Home | Six-app launcher with shared controls |
| Notes | Fixed 4 KiB ASCII document, insertion at cursor, arrows, newline, forward/backward delete, Ctrl+S; write via .NEW/.BAK |
| Calculator | Arithmetic, parentheses, operator precedence, errors |
| Files | Read-only browsing within /SUITE, subfolders and file selection |
| Text Viewer | Preview supported text files to 4 KiB; no binary or oversized file loading |
| Wi-Fi Scan | Async passive SSID/RSSI list; no connection or password harvesting |
| System Info | CPU frequency, flash, free heap, uptime and SD status |
| Settings | Brightness presets, optional note autosave; persisted to microSD |

Home, viewer, and settings are integrated in one application firmware, not separate flash images.

## Build and tests

Install [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/index.html), then:

~~~sh
bash tools/run-host-tests.sh
pio run -e cardputer_original
~~~

To flash a physical **original Cardputer** via USB-C (replaces existing firmware):

~~~sh
pio run -e cardputer_original -t upload
~~~

Do not flash until the build passes. The CI workflow runs C++ host tests and a PlatformIO compile; neither replaces physical verification.

## Controls

| Screen | Controls |
| --- | --- |
| Any app | Top-left backtick or Fn+backtick: return Home |
| Home | Fn+Up / Fn+Down or W / S; Enter launch |
| Notes | Type; Enter newline; Backspace delete behind; Fn+Backspace delete ahead; Fn+arrow cursor movement; Ctrl+S save |
| Files | Fn+Up / Fn+Down or W / S; Enter open; Backspace parent; R refresh |
| Text Viewer | Fn+arrow move around text; Backspace returns to Files |
| Calculator | Type expression; Enter evaluate; Backspace erase |
| Wi-Fi Scan | R rescan |
| Settings | Fn+Up / Fn+Down select option; Fn+Left / Fn+Right or Enter change |

The original keyboard's arrow and Escape functions are on the Fn layer; the code reads the official M5Cardputer key-state fields.

## SD layout and file safety

If a FAT32 microSD card is mounted, the suite creates /SUITE. Notes live at /SUITE/NOTES.TXT; preferences at /SUITE/CONFIG.TXT. A save first writes and checks a .NEW file, renames the previous version to .BAK, then installs the new file. On next boot, a missing primary file is recovered from .BAK if available. **FAT SD storage is not power-loss transactional:** avoid power cycling during writes and keep your own backups.

Notes or preview files larger than 4096 bytes, or containing non-ASCII content, are rejected rather than silently truncated. An invalid existing note is protected from overwrite. Without SD, edits and settings are temporary.

## Hardware restrictions

- ESP32-S3, 8 MB flash, **no PSRAM**; use bounded allocations.
- 240x135 display, physical 56-key matrix.
- SD SPI: GPIO40 SCK / 39 MISO / 14 MOSI / 12 CS.
- Built-in Wi-Fi only; no radio transmit tools, external IMU, GPS, camera or IR receiver.
- Avoid M5Stack Cardputer-ADV-specific APIs.

## Docs

- [Acceptance checklist](docs/ACCEPTANCE.md)
- [Milestone roadmap](docs/ROADMAP.md)

## Sources

- [Official original Cardputer hardware](https://docs.m5stack.com/en/core/Cardputer)
- [Official keyboard API](https://docs.m5stack.com/en/arduino/m5cardputer/keyboard)
- [M5Cardputer library](https://github.com/m5stack/M5Cardputer)
