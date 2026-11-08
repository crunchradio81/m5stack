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
