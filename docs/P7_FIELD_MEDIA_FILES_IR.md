# P7 — Four more SD application collections (20 program screens)

**Hardware target:** original M5Stack Cardputer K132, ESP32-S3FN8,
8 MiB flash, no PSRAM. The P7 branch adds four **separate SD firmware
packages**, taking the recognized SD app library from 10 packages to 14.
Each new firmware image contains five submenu programs.

**Status:** Compiled-source development. No physical original K132 was
flashed or used for acceptance. See `docs/P7_ACCEPTANCE.md` before treating
these as working field instruments or a safe permanent launcher.

## 1. IR Remote Tools — IRTOOLS.APP / IRTOOLS.BIN

The original K132 has a **transmit-only IR LED on GPIO44**. IR hardware
cannot receive/learn a remote's button codes. The five modes are:

| Mode | Current functionality |
| --- | --- |
| NEC Remote | Send user-defined NEC address/command values |
| Device Profiles | List and load up to 8 locally supplied `.IRP` files |
| Media Controller | Power, up/down, mute and play codes from loaded profile |
| Camera Shutter | Sends the profile's PLAY code on Enter; a camera must support that exact IR format/code |
| Manual IR Macro | User-triggered sequence POWER → PLAY → MUTE, ~500 ms spacing; S cancels |

**No ready-made universal code database, brand compatibility claim or
IR receive/learning capability.** These are NEC only, not Sony/RC5/etc.
Never point an unverified macro at critical hardware.

To create a profile on microSD:

~~~text
/SUITE/IR/LIVING.IRP
IRP1,Living Room,0000,45,46,47,48,49
~~~

Fields: `IRP1,NAME,AAAA,PP,UU,DD,MM,LL`, where AAAA is 16-bit hex
NEC address and the five two-digit hex bytes are POWER, UP, DOWN, MUTE,
PLAY. **These numbers are only example format values**, not verified
commands for any TV or camera. Copy correct codes from your device's
manufacturer or a verified owner-controlled reference.

## 2. Wireless Field Lab — WIRELESS.APP / WIRELESS.BIN

Five tools: Wi-Fi Signal Graph, Wi-Fi Survey Notebook, AP Appearance
Monitor, BLE Advertisement Logger and BLE Signal Viewer.

- Only **2.4 GHz** Wi-Fi is supported. Scanning passively detects nearby
  access points and their advertised SSID, BSSID, channel and RSSI.
- Signal graph keeps the last 16 selected AP strength readings. A single
  fixed-interval Wi-Fi scan is asynchronous; S toggles automatic scanning,
  R scans once; Fn arrows select entries.
- Survey Notebook lets you type a **manual site label** with N. No GPS,
  coordinates, precise location or real-world timestamp is inferred.
- Appearance Monitor compares recent scan result sets (bounded to 24 APs)
  and reports observed changes; disappearance can also mean missed scans.
- BLE tools collect **advertising broadcasts** with name/address/RSSI;
  no Bluetooth Classic, pairing, unauthorized GATT connections or
  identifying a person. S/R initiates a two-second scan.
- Survey and BLE app logs append on FAT32 SD as
  `/SUITE/FIELD/WIFI.CSV` and `/SUITE/FIELD/BLE.CSV` (1 MiB cap).
  Time is **seconds since boot**, not local calendar time. The records
  include broadcast device addresses; handle/sharing is your choice.
- E requests **Y/N confirmation** before removing the current category
  log; there is no automatic deletion. No Wi-Fi association or traffic
  interception occurs.

## 3. Audio Lab — AUDIOLAB.APP / AUDIOLAB.BIN

Five programs: Voice Recorder, Mono WAV Player, Audio Level Meter,
Guitar Pitch Estimator and an 8-Band Spectrum Viewer.

- Original K132 built-in SPM1423 microphone and speaker **cannot operate
  simultaneously** because they share GPIO43. The app explicitly stops
  speaker output before starting the mic, and closes the mic before
  playback. No monitoring mic audio through the speaker while recording.
- Recorder uses 16 kHz, mono, 16-bit PCM. S/Enter starts/stops.
  Files saved under `/SUITE/AUDIO/REC000.WAV` etc. (up to 4,000,000
  data bytes per clip); intermediate recording uses `.WAV.NEW` until
  a finalized 44-byte WAV header is written. Do not rename unfinished
  files without repairing their headers.
- Player accepts **only exact standard 44-byte mono PCM16 WAV files**
  at 8 or 16 kHz, at most 4 MiB data; it does not decode MP3/AAC/FLAC
  or arbitrary WAV chunks and cannot stream radio.
- Audio meter reports absolute sample amplitude, not calibrated SPL.
  Pitch estimator is a limited 125–1000 Hz correlation estimate, not
  an accurate guitar tuner until external accuracy tests pass.
  Spectrum uses eight approximate energy bands, not a full 1024-point FFT.
- Long recordings can use substantial SD space. Use a sufficiently
  charged battery and do not remove the card while recording.

## 4. SD File Tools — FILETOOLS.APP / FILETOOLS.BIN

Five programs: Safe File Manager, Backup Files, Restore Files,
Filename Search and Storage Inventory.

The **only** editable data root is `/SUITE/USER` and the backup root
is `/SUITE/BACKUP`. This is deliberately not an unrestricted filesystem
editor: it cannot alter the launcher, `/SUITE/APPS` firmware images,
MIDI files or system configuration.

- Manager lists up to 40 ordinary files. M renames, D deletes, N creates
  a new empty `.TXT` file, V previews the first 28 bytes; destructive
  actions require an explicit Y/N confirmation.
- Backup copies selected USER file into BACKUP; Restore copies it back.
  Source/target are guarded by safe relative filenames, a 1 MiB per-file
  limit, and **no overwrite** even if the names match an existing file.
- A copy goes first to `DEST.NEW`, then is renamed only after all data
  was written. Failure preserves the partial `.NEW` for inspection.
  It does **not** provide strong power-loss transactional guarantees.
- Filename Search matches local filenames in USER, case-insensitive.
  Storage Inventory counts visible USER files and their bytes;
  neither is a recursive full-SD scanner.
- No folder deletion, formatting, mass erase, unrestricted root access
  or firmware flashing is supported.

## Packaging and safety

To build the four new app images:

~~~sh
pio run -e sd_irtools -e sd_wireless -e sd_audiolab -e sd_filetools
~~~

The full matrix also builds 10 prior SD images plus the resident factory
launcher. GitHub CI packages all 14 firmware images with bounded `.APP`
manifests and SHA-256 checksums into
`Cardputer-K132-SD-App-Pack.zip`; copy `SUITE/APPS` to FAT32 SD root.

**Each SD app runs from the replaceable OTA flash slot after selection.
It does not execute code directly from SD.** Changing from ordinary
Cardputer firmware to the custom factory+OTA partition layout for the
first time requires an owner's explicit backup and one-time flash erase.
No physical K132 has yet been tested for OTA recovery, mic/IR/SD behavior,
or battery endurance. Hashes are integrity checks, not authentication.

Full catalog progress: `docs/APP_CATALOG.md`; acceptance:
`docs/P7_ACCEPTANCE.md`.
