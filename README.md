# Cardputer Suite — original M5Stack Cardputer K132

One native, statically linked firmware for the **stock original 2023 M5Stack Cardputer (K132 / StampS3)**. No Grove unit, external sensor, soldering or expansion board. A FAT32 microSD is optional and uses the device's **built-in slot**.

**P2 MIDI Studio: in development.** Earlier P1/P1.1 source built in CI, but the new MIDI branch still requires a successful CI build and real K132 audio acceptance. Back up your existing firmware and card before flashing.

## Included P1 programs

| Program | P1 capability |
| --- | --- |
| Home | Seven-app launcher with shared controls |
| Notes | Fixed 4 KiB ASCII document, insertion at cursor, arrows, newline, forward/backward delete, Ctrl+S; write via .NEW/.BAK |
| Calculator | Arithmetic, parentheses, operator precedence, errors |
| Files | Read-only browsing within /SUITE, subfolders and file selection |
| Text Viewer | Preview supported text files to 4 KiB; no binary or oversized file loading |
| Wi-Fi Scan | Async SSID/RSSI list; no connection or password harvesting |
| Wi-Fi Logger | Start/Pause, repeated passive 2.4 GHz AP scans, CSV on SD, full details, saved-record browsing, confirmed Erase |
| MIDI Studio (P2 feature) | Type 0/1 Standard MIDI File streaming, 12-voice internal-speaker synth, tempo changes, track mute/solo, loop, speed and volume controls |
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
| Wi-Fi Logger | S start, P pause, V view saved detections, E erase (requires Y confirmation); Fn+Up/Fn+Down choose live AP, Enter details |
| Saved Wi-Fi Log | Fn+Up older saved record; Fn+Down newer; B/Backspace back |
| Wi-Fi Details | B/Backspace back |
| Erase Confirmation | Y erases only WIFI.CSV; N/Backspace cancels |
| MIDI Library | Put .MID or .MIDI files in /SUITE/MIDI on SD. Fn Up/Down select, Enter load, R rescan |
| MIDI Player | Enter/Space/P pause/resume; N next; Z restart; L loop; +/- speed 50–200%; [ / ] speaker volume; M tracks; H help; B library |
| MIDI Mixer | Fn Up/Down select track; Enter or M mute; S solo; B player |
| Settings | Fn+Up / Fn+Down select option; Fn+Left / Fn+Right or Enter change |

The original keyboard's arrow and Escape functions are on the Fn layer; the code reads the official M5Cardputer key-state fields.

## SD layout and file safety

If a FAT32 microSD card is mounted, the suite creates /SUITE. Notes live at /SUITE/NOTES.TXT; preferences at /SUITE/CONFIG.TXT. A save first writes and checks a .NEW file, renames the previous version to .BAK, then installs the new file. On next boot, a missing primary file is recovered from .BAK if available. **FAT SD storage is not power-loss transactional:** avoid power cycling during writes and keep your own backups.

Notes or preview files larger than 4096 bytes, or containing non-ASCII content, are rejected rather than silently truncated. An invalid existing note is protected from overwrite. Without SD, edits and settings are temporary.

## P2 MIDI Studio (first major advanced app)

MIDI Studio is built for the **original Cardputer's internal speaker**, not an external MIDI module.
Copy Standard MIDI Files (.mid / .midi) to the built-in microSD slot under /SUITE/MIDI/.
Open **MIDI Studio** from the launcher, select a file and press Enter.

The streaming parser supports **SMF Type 0 and 1**, up to **16 tracks**, PPQN timing,
running status, tempo/time signature events, program changes, note on/off, pitch bend,
volume, expression, sustain, all-notes-off and channel 10 percussion.
Tracks are merged chronologically. A **12-voice software synthesizer** renders
22.05 kHz 16-bit mono PCM for the built-in speaker.

Player keys: Enter/Space/P pause or resume, B browse, N next, Z restart,
L loop, +/- speed 50–200%, brackets volume, M mixer, H help.
Mixer keys: Fn Up/Down select track, Enter/M mute, S solo, B back.
Home/escape stops audio and closes the MIDI file.

**Limitations:** Instruments are light waveform approximations, not a sampled General MIDI
SoundFont. MIDI files over 4 MiB, SMF Type 2, SMPTE divisions and over 16 tracks
are intentionally rejected. Lyrics/KAR, sampled instruments, playlists and seeking
remain on the roadmap. Speaker output, keyboard and timing await physical K132 tests.
Start at low volume.

## Walking Wi-Fi logger (no add-ons)

The **Wi-Fi Logger** app uses the original Cardputer's built-in 2.4 GHz Wi-Fi radio and microSD slot. Insert a writable FAT32 microSD, choose **Wi-Fi Logger**, then press **S** to start. The device repeats passive access-point scans approximately every 8 seconds (scanning time plus a five-second gap). Walk with the logger screen open; press **P** to pause. Enter shows the selected access point's details. Press **V** while paused or running to pause and inspect saved entries; Fn+Up browses older saved readings and Fn+Down newer ones. Press **E**, then **Y**, to delete the Wi-Fi CSV only. **N** cancels. Leaving the logger pauses recording.

The log is saved directly as **`/SUITE/WIFI/WIFI.CSV`** and remains available after reboot. Every detection produces a row, including repeats of the same BSSID as signal strength changes:

```csv
session,uptime_ms,scan,bssid,ssid,rssi_dbm,channel,security,hidden
A12B34C5,155000,22,02:11:22:33:44:55,"ExampleNet",-63,6,WPA2,0
```

Fields: session ID (new on boot), milliseconds since boot, scan sequence, access point BSSID/MAC, broadcast SSID, RSSI (dBm), radio channel, advertised security and hidden-SSID flag. **-45 dBm is stronger than -80 dBm**; the bars are approximate only. Repeated observations are useful for tracking relative signal strength, not for measuring physical distance. The record viewer reads the CSV on demand, so large logs do not have to fit in device RAM. The log stops at **32 MiB** instead of overwriting old detections; when SD writes fail, collection pauses and reports the problem. There is no automatic network upload.

**Limitations:** this detects nearby **Wi-Fi access points**, not individual phones or Wi-Fi clients; the original Cardputer cannot receive 5/6 GHz Wi-Fi. It has no GPS and no always-accurate wall clock, so the log contains no location and no verified date/time. This application does not connect to networks or capture passwords. The CSV includes nearby access point MAC addresses, so keep it private unless you intentionally choose to share it.

**Data safety:** the logger appends and flushes rows after each scan. A sudden power loss can leave the final row incomplete; on reopening, it separates that incomplete row before appending new ones. The viewer skips malformed rows. Logs are not fully power-loss transactional, and you should back up long survey logs. Erase is limited to WIFI.CSV, not the rest of the SD card.

## Hardware restrictions

- ESP32-S3, 8 MB flash, **no PSRAM**; use bounded allocations.
- 240x135 display, physical 56-key matrix.
- SD SPI: GPIO40 SCK / 39 MISO / 14 MOSI / 12 CS.
- Built-in Wi-Fi only; no radio transmit tools, external IMU, GPS, camera or IR receiver.
- Avoid M5Stack Cardputer-ADV-specific APIs.

## Docs

- [Acceptance checklist](docs/ACCEPTANCE.md)
- [Milestone roadmap](docs/ROADMAP.md)
- [Complete 94-program application backlog](docs/APP_CATALOG.md)

## Sources

- [Official original Cardputer hardware](https://docs.m5stack.com/en/core/Cardputer)
- [Official keyboard API](https://docs.m5stack.com/en/arduino/m5cardputer/keyboard)
- [M5Cardputer library](https://github.com/m5stack/M5Cardputer)
