# Cardputer Suite — original M5Stack Cardputer K132

The stock original 2023 **M5Stack Cardputer K132 / StampS3** suite has a resident factory launcher and an expanding collection of **separately compiled application firmware files stored on microSD**. No Grove unit, PSRAM, external sensor, soldering or expansion board is required.

**P1–P4 source compiled in CI; P5 adds 23 further programs across four SD collections. All are still unverified on a physical original Cardputer.** Back up your firmware and SD files before flashing. This P4 release changes the flash partition table and requires an explicitly approved one-time erase.

## Resident suite and SD application collections

| Program | Capability |
| --- | --- |
| Home | Scrollable 13-entry factory launcher with shared keyboard controls |
| Notes | Fixed 4 KiB ASCII document, insertion at cursor, arrows, newline, forward/backward delete, Ctrl+S; write via .NEW/.BAK |
| Calculator | Arithmetic, parentheses, operator precedence, errors |
| Files | Read-only browsing within /SUITE, subfolders and file selection |
| Text Viewer | Preview supported text files to 4 KiB; no binary or oversized file loading |
| Wi-Fi Scan | Async SSID/RSSI list; no connection or password harvesting |
| Wi-Fi Logger | Start/Pause, repeated passive 2.4 GHz AP scans, CSV on SD, full details, saved-record browsing, confirmed Erase |
| MIDI Studio (P2 feature) | Type 0/1 Standard MIDI File streaming, 12-voice internal-speaker synth, tempo changes, track mute/solo, loop, speed and volume controls |
| Stopwatch (P3) | Pause/resume, eight lap records, centisecond display, uptime-based timing |
| Countdown (P3) | Editable HHMMSS duration, pause/resume/reset, visual completion notice |
| Unit Converter (P3) | Sixteen unit conversions across five measurement categories |
| Programmer Calculator (P3) | Binary/octal/decimal/hex values and 32-bit AND, OR, XOR, NOT and shifts |
| SD App Library (P4) | Read manifests and install selected app from SD into separate OTA partition |
| Metronome (SD firmware) | 30–260 BPM, accents, 2–12 beats and subdivisions using built-in speaker |
| Music Keyboard (SD firmware) | Play 13-note layout on built-in speaker, octave and volume controls |
| Wi-Fi Channel Analyzer (SD firmware) | Passive 2.4 GHz survey, per-channel counts, peak RSSI and heuristic recommendation |
| BLE Explorer (SD firmware) | Passive 3-second BLE scan and up to 24 nearby advertiser summaries |
| Retro Arcade collection | Snake, 2048, Minesweeper, Sokoban, Pong, Breakout, Word Guess |
| Pocket Office collection | To-do, Contacts, Expenses, Journal, manual Calendar |
| SD Inspector collection | Hex, CSV, bounded JSON, File Search, ASCII, Storage Analyzer |
| Music Practice collection | Morse, Tone Generator, Drum Sequencer, Ear Trainer, Tap Tempo |
| System Info | CPU frequency, flash, free heap, uptime and SD status |
| Settings | Brightness presets, optional note autosave; persisted to microSD |

The first 12 entries remain built into the factory suite, including its SD library menu. P4 has four standalone SD firmware images; P5 adds four more SD firmware images, each hosting a category submenu. Selecting an image installs it into the replaceable OTA slot.

## Build and tests

Install [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/index.html), then:

~~~sh
bash tools/run-host-tests.sh
pio run -e cardputer_original -e sd_metronome -e sd_piano -e sd_channels -e sd_ble \\
        -e sd_arcade -e sd_office -e sd_inspect -e sd_musiclab
python3 tools/package-sd-apps.py --build-root .pio/build --out Cardputer-K132-SD-App-Pack.zip
~~~

The ZIP contains eight manifest/binary pairs: METRONOME, PIANO, CHANNELS, BLE, ARCADE, OFFICE, INSPECT and MUSICLAB. The P5 collections provide 23 additional programs. See [collection controls](docs/P5_COLLECTIONS.md).
Copy its `SUITE` directory to the **root of a FAT32 SD card**. Do not
flash the SD-app images directly over the factory launcher.

**Critical:** This new branch uses a custom factory+OTA partition layout.
Its first physical installation is destructive to previous internal
firmware/configuration and must use a one-time full flash erase. Do not
execute that without a backup and consent. [Read the exact installation
and recovery instructions](docs/SD_LAUNCHER.md).

GitHub Actions compiles the factory launcher and four SD apps as five
separate jobs, then packages their binary images and manifests for SD.
CI and host tests cannot certify real keyboard, audio, Wi-Fi, BLE or
flash switching operation. Use [P4 hardware acceptance](docs/P4_ACCEPTANCE.md).

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
| MIDI Player | Enter/Space/P pause/resume; N next; Z restart; L loop; A auto-next; +/- speed 50–200%; , / . transpose; [ / ] volume; M mixer; H help; B library |
| MIDI Mixer | Fn Up/Down select track; Enter or M mute; S solo; B player |
| Stopwatch | S/Enter start or pause, L lap, R reset while paused |
| Countdown | Type HHMMSS (0130 means 1m30s), Enter set, S start/pause, R reset |
| Unit Converter | Fn Up/Down change conversion, type number, Backspace edit, C clear |
| Programmer Calculator | H base, O operation, T select operand, type digits, Backspace edit, R clear |
| SD App Library | Fn Up/Down choose, R rescan, Enter confirm, Y flash selected app, N cancel |
| Metronome (SD) | S start/stop; Fn Up/Down change BPM; M meter; D subdivisions |
| Music Keyboard (SD) | A S D F G H J K white keys, W E T Y U sharps, Fn Up/Down octave, +/- volume |
| Wi-Fi Channels (SD) | S scan, Fn Left/Right select channel, view AP count and strongest RSSI |
| BLE Explorer (SD) | S scan, Fn Up/Down choose advertisement, Enter detail, Backspace list |
| Any SD app | Escape/Home key restarts into factory launcher, only when return guard worked |
| Settings | Fn+Up / Fn+Down select option; Fn+Left / Fn+Right or Enter change |

The original keyboard's arrow and Escape functions are on the Fn layer; the code reads the official M5Cardputer key-state fields.

## SD layout and file safety

If a FAT32 microSD card is mounted, the suite creates /SUITE. Notes live at /SUITE/NOTES.TXT; preferences at /SUITE/CONFIG.TXT. A save first writes and checks a .NEW file, renames the previous version to .BAK, then installs the new file. On next boot, a missing primary file is recovered from .BAK if available. **FAT SD storage is not power-loss transactional:** avoid power cycling during writes and keep your own backups.

Notes or preview files larger than 4096 bytes, or containing non-ASCII content, are rejected rather than silently truncated. An invalid existing note is protected from overwrite. Without SD, edits and settings are temporary.

## P3 offline pocket tools (four more apps)

The P3 branch adds four standalone applications to the scrollable launcher:
**Stopwatch**, **Countdown**, **Unit Converter** and **Programmer Calculator**.
They require no SD, network, GPS, external RTC or Grove accessories.

The stopwatch offers elapsed time and eight laps. The countdown accepts
one to six digits interpreted as HHMMSS, with explicit valid minute/second
ranges and a **visual-only** completion indication; it is not a wake-up alarm
and does not persist through power-off. Neither makes up wall-clock time.

The converter includes 16 measurement directions across distance, mass,
speed, temperature and US liquid volume. The programmer calculator supports
32-bit **unsigned** values, binary/octal/decimal/hex input, bitwise operators
and bounded 0–31-place shifts. See
[the P3 hardware acceptance checklist](docs/P3_ACCEPTANCE.md).

**P3 code is unverified on actual hardware until acceptance is recorded.**

## P2 MIDI Studio (first major advanced app)

MIDI Studio is built for the **original Cardputer's internal speaker**, not an external MIDI module.
Copy Standard MIDI Files (.mid / .midi) to the built-in microSD slot under /SUITE/MIDI/.
Open **MIDI Studio** from the launcher, select a file and press Enter.

The player includes a live 12-note pitch-class activity meter and a bounded
three-buffer audio queue. The streaming parser supports **SMF Type 0 and 1**, up to **16 tracks**, PPQN timing,
running status, tempo/time signature events, program changes, note on/off, pitch bend,
volume, expression, sustain, all-notes-off and channel 10 percussion.
Tracks are merged chronologically. A **12-voice software synthesizer** renders
22.05 kHz 16-bit mono PCM for the built-in speaker.

Player keys: Enter/Space/P pause or resume, B browse, N next, Z restart,
L loop, A auto-next-song (loop takes priority), +/- speed 50–200%,
comma/period transpose by semitone (-12 to +12), brackets volume, M mixer, H help.
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
- [SD firmware launcher design, installation and recovery](docs/SD_LAUNCHER.md)
- [P4 physical acceptance checklist](docs/P4_ACCEPTANCE.md)

## Sources

- [Official original Cardputer hardware](https://docs.m5stack.com/en/core/Cardputer)
- [Official keyboard API](https://docs.m5stack.com/en/arduino/m5cardputer/keyboard)
- [M5Cardputer library](https://github.com/m5stack/M5Cardputer)


## P5 collection controls and limitations

The eight SD packages live in FAT32 `/SUITE/APPS`. Open **SD App Library**
from the factory suite, choose a package, press Enter and confirm with Y.
The device installs that one package to OTA_0, then boots it; **B** goes
back to the package's submenu and the top-left backtick/Escape restarts
the factory launcher. Flash switching is not direct SD execution.

**Retro Arcade:** Fn arrows/WASD move; B returns to menu, R restarts.
**Pocket Office:** N adds, D deletes with confirmation, arrows select,
Enter toggles a to-do; files saved under `/SUITE/OFFICE`.
**SD Inspector:** browse read-only files under `/SUITE`, view bounded
hex/CSV/JSON, search filenames and analyze storage.
**Music Practice:** built-in speaker tones, Morse messages, drum pattern,
interval quizzes and tap tempo. This is not a sampled instrument bank.

Detailed controls, limits and tests:
[program inventory](docs/P5_COLLECTIONS.md) and
[hardware acceptance](docs/P5_ACCEPTANCE.md).
