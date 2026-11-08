# Stock Cardputer suite roadmap

Target: original K132 with built-in peripherals; **no external Grove units**.

## P0 — source proof of concept
- Statically linked shell, Notes, calculator, file list, Wi-Fi scan, system status.
- 12 host-side calculator checks.
- No hardware certification.

## P1 — core reliability (this PR)
- Bounded cursor editor, strict file-size/binary guards.
- Safer microSD file replacement with backup/recovery.
- Read-only folder browser and bounded text viewer.
- User preferences (brightness and autosave), built-in Wi-Fi scanner.
- Host unit tests plus GitHub PlatformIO build workflow.
- **Gate still pending:** physical original-Cardputer acceptance.

## P1.1 — walking Wi-Fi Logger
- Built-in 2.4 GHz AP scanning with Start, Pause, View and confirmed Erase.
- Append-only bounded CSV: repeated observations, SSID, BSSID, RSSI, channel,
  advertised security, boot-relative time, scan and session.
- Browse saved detections on-device without loading the whole CSV into RAM.
- No Grove, GPS or external hardware; no 5 GHz or client sniffing.
- **Gate pending:** hardware SD removal/recovery, passive scan and walking tests.

## P2 — MIDI Studio (source built, hardware pending)
- Streaming Standard MIDI File Type 0/1, fixed 12-voice waveform synthesizer.
- Song browser, tempo/speed, transposition, looping, mute/solo and visual meter.
- Sample-based instruments, playlists, seeking and lyrics still planned.
- **Gate pending:** physical original K132 speaker + SD + keyboard tests.

## P3 — offline pocket tools (source built, hardware pending)
- Stopwatch with eight laps, pause/resume and elapsed time measured by uptime.
- Countdown timer with HHMMSS entry, pause/resume/reset and visual completion.
- Sixteen unit-conversion directions and unsigned 32-bit programmer calculator.
- Host-side negative cases, 32-bit rollover and numeric overflow tests.
- **Gate pending:** physical original K132 acceptance in docs/P3_ACCEPTANCE.md.

## P4 — SD application launcher and external firmware programs
- Custom 8MiB factory + replaceable OTA partition (3MiB each).
- FAT32 SD app list with bounded .APP manifests, SHA256 checksum and
  Y/N confirmation for firmware installation.
- Standalone SD binaries: Metronome, Music Keyboard, Wi-Fi Channel Analyzer,
  and BLE Explorer; apps arm factory partition as next boot target.
- Parallel CI builds factory + four images and packages them for microSD.
- **Gate pending:** physical original K132 boot, recovery, Wi-Fi, BLE and audio tests.

## P5 — expanded personal tools
- Editable named text documents with explicit Save As and confirmation prompts.
- File manager copy/rename/delete only after tested backup and undo behaviors.
- TODO, calendar/date support, expanded calculator.
- Input regression fixture for all Fn/modifier combinations.

## P6 — communication and development
- Wi-Fi connection manager with local credential storage policy.
- NTP time sync, HTTP/JSON reader, local LAN messaging.
- Tiny BASIC or script interpreter with strict resource and I/O quotas.
- BLE scanner using only built-in radio.

## P7 — media and games
- Mono audio recorder/player (streaming, limited formats).
- IR transmit remote, synth/sequencer, compact retro games.
- User-friendly backup/restore and multi-firmware launcher integration only
  when the shared binary exceeds original hardware limits.

Every phase must compile for **original K132 with no PSRAM**.
Do not claim releases pass until the board-specific evidence exists.

## P5 — four SD collections with 23 new programs

- Seven Arcade games, five Pocket Office applications, six SD Inspector
  utilities and five Music Practice applications.
- Four new independent SD app images `ARCADE`, `OFFICE`, `INSPECT`,
  `MUSICLAB`; eight images total packaged by CI.
- Fixed-memory game logic, record codec, CSV/JSON bounds and Morse/tap
  timing tested by host C++ regressions.
- **Still pending:** physical original K132 flash/return-to-factory,
  SD persistence, display, keyboard, speaker and battery acceptance.
- The rest of the 94-program project backlog remains open.

## P6 — Developer Tools and System Toolbox

- Source: BASIC Mini expression REPL, scientific math, hex/ASCII codec,
  CHIP-8 VM with bounded 3584-byte ROM and 4 KiB emulator memory.
- Source: Battery, free heap/CPU, firmware/partitions, SD card info,
  read-only throughput benchmark, SHA-256 SD app digest, uptime.
- Ten known SD firmware IDs, unchanged factory/OTA flash layout.
- Expanded host regressions, firmware compilation and SD image packing.
- **Gate pending:** physical original K132 SD boot/return, CHIP-8 keyboard,
  battery readout, timing/heap and OTA recovery.

## P7 — IR, wireless field logging, audio and SD file management

- IRTOOLS: user-supplied NEC transmit profiles, media/shutter codes,
  manually triggered bounded IR macro, **no receiver/learning**.
- WIRELESS: 2.4GHz passive AP surveys, label and SD logs, appearance
  comparison, BLE broadcast logs/strength; no GPS or pairing.
- AUDIOLAB: 16kHz mono PCM WAV recorder with .NEW finalization,
  bounded PCM WAV playback, amplitude, rough tuner and coarse spectrum.
- FILETOOLS: guarded file operations under /SUITE/USER and /SUITE/BACKUP
  only, no overwrite, confirmed destructive actions.
- Four new independently built firmware images, making 14 SD packages.
- **Gate pending:** physical K132 IR carrier timing, passive radio
  survey, microphone/speaker exclusivity, SD recoverability, and
  factory boot/OTA recovery.
- Full 94-item catalog remains open, including deeper media formats,
  network clients, text tools and application quality passes.

## P8 — Networking and Internet collection (source implemented)

- Real SD-loaded, owner-controlled Wi-Fi connection with bounded config
  and password never rendered on screen.
- DNS resolution, single ICMP echo, UTC NTP clock, bounded plaintext
  HTTP GET and first-title RSS view.
- Local _http._tcp mDNS discovery, trusted plaintext MQTT subscription,
  and opt-in UDP LAN chat with strict ASCII packet framing.
- 15 SD app images + original factory build, unchanged flash layout.
- Host tests for credentials parser, invalid ports/hosts, LAN message
  framing and sanitization.
- **Not implemented:** HTTPS with CA validation, secure MQTT, full RSS,
  browser, comprehensive LAN discovery, Internet radio and WAN file serving.
- **Gate pending:** original K132 Wi-Fi network acceptance, timeout
  behavior, heap stability, genuine ping results, and OTA/factory recovery.

## P9 — Advanced MIDI Studio / MIDI Deep Tools

- Streaming SMF parser can surface bounded text and lyric meta events.
- Resident factory MIDI player adds .KAR library support and timed K
  lyric view during playback, without buffering full song in RAM.
- New MIDITOOLS SD collection: Track Analyzer, bounded Piano Roll,
  Lyrics Catalog, Favorites, Tempo Map with bounded SD playlist I/O.
- **Gate pending:** original Cardputer audio timing, lyric sync,
  .KAR media validation, SD file persistence and OTA return.
- **Not yet implemented:** high-quality GM sample banks, multitrack
  editing, full sequencer/tracker, seek and downloadable patch banks.
