# Original Cardputer — complete application-suite backlog

**Owner direction:** build the full catalog, with **MIDI Studio first**. All apps
must remain compatible with the **original K132**, without Grove units, external
sensors, PSRAM, a camera, GPS, 5/6 GHz Wi-Fi or a headphone socket.

Statuses: **Built source** means source code exists, not necessarily merged or hardware verified;
**Active** means the app is under implementation; **Planned** means no claim of code.

## P4 executable SD library (built source, hardware acceptance pending)

A persistent factory menu launches eighteen separately compiled SD firmware
images into a replaceable OTA slot, using owner-controlled manifests and
SHA256 integrity checks. Only suite-managed app IDs are allowed; this is
not authenticated firmware or guaranteed hardware recovery. See docs/SD_LAUNCHER.md.

## P1 platform (built source, hardware acceptance pending)

Home launcher, Notes editor, Calculator, read-only File Browser, Text Viewer,
Wi-Fi Scanner, Wi-Fi Logger, System Info, Settings (9 components).

## P2 media — first major subsystem

1. **MIDI Studio** — **Built source, hardware pending:** Standard MIDI Files Type 0/1, PPQN,
   multitrack timing, song library, synth, playback, mute/solo, loop, speed
   control. Full GM sample-based accuracy is not currently supported.
2. MIDI Lyric/Karaoke Viewer — **P9 source: bounded FF 05/FF 01 lyrics synchronized during playback, hardware pending:** text and synchronized lyric events
   in MIDI/KAR where present.
3. Advanced Instrument Library — **Planned:** storage-efficient instrument
   patches, not PSRAM-only SF2 loading.
4. MIDI Track Analyzer — **P9 source: bounded note/channel/tempo statistics, hardware pending:** notes, tempo map, channel programs,
   event browser and instrument activity.
5. MIDI Playlist Manager — **P9 partial: saved favorites only; ordered playlists/resume planned:** playlists/favorites, sorting, resume.
6. Voice Recorder — **P7 source; hardware pending:** microphone audio to SD; no simultaneous capture/play.
7. WAV Player — **P7 source; 8/16k mono PCM16 only, hardware pending:** mono streaming.
8. Music Tracker — **Planned:** patterns and sample-lite instruments.
9. Piano — **Built source as SD Music Keyboard:** real hardware pending.
10. Drum Machine — **Planned:** pattern sequencing.
11. Metronome — **Built source as SD app:** physical timing tests pending.
12. Audio Level Meter — **P7 source: approximate amplitude, not calibrated RMS/SPL:** microphone RMS/peak.
13. Guitar Tuner — **P7 partial: rough pitch estimator, accuracy pending:** microphone pitch estimator.
14. Morse Trainer — **Planned:** generate/decode practice tones.
15. Audio Spectrum Viewer — **P7 partial: eight coarse bands, not a full FFT:** FFT of microphone samples.

## P3 wireless, networking and Internet (15)

16. Wi-Fi Signal Graph — **P7 source, 16-sample graph; hardware pending**
17. Wi-Fi Channel Analyzer — **Built source as SD app; hardware pending**
18. Wi-Fi Survey Notebook with manually named survey points — **P7 source; boot-relative CSV log**
19. Wi-Fi History Explorer / comparisons — **P7 partial: live 16-sample signal window; full history remains planned**
20. Wi-Fi Appearance/Disappearance Monitor — **P7 source; scan-to-scan estimate, hardware pending**
21. BLE Advertisement Logger — **P7 source, passive SD CSV with boot-relative time**
22. BLE Signal Analyzer — **P7 extends passive RSSI viewer; history/graphs still planned**
23. BLE GATT Explorer — **Planned**, user-consented device connections only
24. Ping / latency diagnostics — **P8 source: single ICMP echo, hardware pending**
25. DNS Lookup — **P8 source: bounded IPv4 lookup, hardware pending**
26. LAN Service Discovery (mDNS/SSDP) — **P8 partial: mDNS HTTP-service query, SSDP planned**
27. LAN Throughput Test — **Planned**
28. MQTT Monitor — **P8 partial: trusted plaintext LAN subscriber only, no TLS or publishing**
29. Internet Radio — **Planned**, codec/bitrate constrained
30. Local LAN Chat — **P8 source: unauthenticated ASCII UDP broadcast on LAN**
31. Weather Dashboard — **Planned**, provider/API required
32. World Clock / NTP — **P8 partial: UTC NTP only; multi-zone clock planned**
33. RSS Reader — **P8 partial: first XML title from bounded HTTP response, full reader planned**
34. Wikipedia Text Reader — **Planned**
35. Lightweight Web Text Browser — **Planned**
36. HTTP / REST Client — **P8 partial: HTTP GET only, no TLS or REST methods**
37. LAN File Transfer Server — **Planned**
38. Smart-home MQTT Controller — **Planned**

## P4 productivity (14)

39. Full Text Editor — **P11 partial: bounded 1023-byte ASCII editor, multi-file editor planned**
40. Searchable Notebook — **Planned**
41. To-Do Manager — **P5 source; hardware pending**
42. Calendar — **P5 source; hardware pending** (needs clock sync/manual date)
43. Contacts — **P5 source; hardware pending**
44. Scientific Calculator — **P6 source; hardware pending**
45. Programmer Calculator — **Built source, hardware pending**
46. Spreadsheet Mini — **P11 source: 6x6 integer cells and row sums, formulas planned**
47. Expenses / Budget Tracker — **P5 source; hardware pending**
48. Password Generator — **P11 partial: non-audited random-string tool, no vault**
49. Encrypted Password Vault — **Planned** (security review required)
50. Personal Journal — **P5 source; hardware pending**
51. Stopwatch / Lap Timer — **Built source, hardware pending**
52. Countdown Timers — **Built source, hardware pending**
53. Unit Converter — **Built source, hardware pending**

## P5 programming and diagnostics (12)

54. BASIC Studio — **P6 partial: BASIC expression REPL, full Studio planned**
55. Bounded Lua Interpreter — **Planned**, only if memory permits
56. Syntax-highlighted Code Editor — **Planned**
57. Hex Viewer / Editor — **P5 source; hardware pending**, backup-required writes
58. USB Serial Console — **Planned**, available USB roles only
59. ASCII / Unicode Table — **P5 source; hardware pending**
60. Binary/Octal/Decimal/Hex Converter — **Partially in Programmer Calculator (source)**
61. Logic and Bit Calculator — **Partially in Programmer Calculator (source)**
62. JSON Inspector — **P5 source; hardware pending**
63. QR Code Generator — **P11 source: display-only QR, physical scan pending**
64. CHIP-8 Interpreter — **P6 source; physical keyboard/ROM test pending**
65. SD Benchmark — **P6 source: read-only benchmark; hardware pending**

## P6 infrared tools (5)

66. IR Universal Remote — **P7 partial: user-configured NEC codes only; no universal DB**
67. IR Device Profiles — **P7 source: user-supplied bounded .IRP files**
68. IR Media Center Controller — **P7 source: commands from loaded owner profile**
69. IR Camera Shutter — **P7 partial: owner-defined NEC shutter code, no model guarantee**
70. IR Macro/Scheduler — **P7 partial: manual three-command macro, no scheduling/receiver**

## P7 arcade, puzzles and interactive (15)

71. Snake — **P5 source; hardware pending**
72. Falling Blocks — **P10 source; hardware pending**
73. Dungeon Crawler — **Planned**
74. Minesweeper — **P5 source; hardware pending**
75. Chess — **Planned**
76. Checkers — **P10 two-player source; physical hardware pending**
77. Solitaire — **Planned**
78. Sokoban — **P5 source; hardware pending**
79. 2048 — **P5 source; hardware pending**
80. Pong — **P5 source; hardware pending**
81. Breakout — **P5 source; hardware pending**
82. Word Guess — **P5 source; hardware pending**
83. Text Adventure Interpreter — **P10 partial: one original branching story, interpreter planned**
84. Virtual Pet — **P10 uptime-based source; persistent saves planned**
85. Retro Arcade Frontend with saves / high scores — **P5 source; hardware pending**

## P8 core system utilities (9)

86. Full SD File Manager — **P7 partial: USER-only files, safe copy/rename/delete; full manager planned**
87. File Search / Content Search — **P5 source; hardware pending**
88. Storage Analyzer — **P5 source; hardware pending**
89. Backup / Restore Manager — **P7 partial: copy USER↔BACKUP, no overwrites or whole-SD backup**
90. Battery Monitor — **P6 source: voltage and estimated level; hardware pending**
91. Firmware Information / Recovery Guide — **P6 partial: firmware info only**
92. Theme Editor — **Planned**
93. System Diagnostics — **P6 source: heap/CPU/partitions; hardware pending**
94. Config / Settings Profiles — **Planned**

## Development and acceptance rules

- Every app must build on original K132 and have at least one negative-case test.
- Native applications should share the launcher, keyboard, SD APIs and settings.
- Hardware timing/audio/wireless behaviors require **physical original K132 tests**;
  CI compilation is not evidence of real-world operation.
- Avoid storing large media, event lists or graphics in RAM. Stream from SD.
- For MIDI specifically, prefer exact SMF timing and safe polyphony before
  developing optional instrument banks, lyrics and visualization.
- Never assert that GPS coordinates, Wi-Fi clients, headphones, IR receive or
  multi-GHz wireless exist on the stock Cardputer.

## P5 application collection inventory (23 programs in four packages)

- ARCADE: Snake, 2048, Minesweeper, Sokoban (one level), Pong,
  Breakout and Word Guess (seven).
- OFFICE: To-do, Contacts, Expense Tracker, Journal and Manual
  Calendar (five). ASCII-only fixed-size SD records.
- INSPECT: Hex Viewer, CSV Viewer, JSON syntax viewer, File Search,
  ASCII Chart and bounded Storage Analyzer (six). Read-only.
- MUSICLAB: Morse Player, Tone Generator, Drum Sequencer, Interval
  Ear Trainer and Tap Tempo (five). Built-in speaker only.

These are source implementations, **not** physical hardware certification.
Remaining numbered catalog entries are still planned. GitHub epic #3
tracks the full 94-program goal; the P5 branch does not finish all 94.

## P6 developer/system collection inventory (11 more programs in two images)

- `DEVTOOLS`: BASIC Mini REPL, Scientific Calculator, ASCII/Hex Codec,
  CHIP-8 Interpreter loading owner-provided ROM files from SD.
- `SYSTEM`: Battery Monitor, Heap/CPU, Firmware Info, SD Card Info,
  read-only SD benchmark, streamed SHA-256 digest, Uptime Clock.
- Two source-built SD firmware packages extend the prior eight to **ten**.
  This is ten installable packages, not eleven new flash partitions.
- BASIC is a bounded expression REPL, not full BASIC Studio; battery
  current/charging state is not claimed; uptime is not a wall clock.
- Full suite's 94 numbered catalog items remain **in progress**.

See docs/P6_DEV_SYSTEM.md and docs/P6_ACCEPTANCE.md.

## P7 four additional SD firmware collections (20 app screens)

- IRTOOLS (5): NEC Remote, Device Profiles, Media Controller,
  user-profile Camera Shutter command, manual three-step Macro.
- WIRELESS (5): Signal Graph, Survey Notebook, AP Change Monitor,
  passive BLE Advertisement Logger, BLE RSSI Viewer.
- AUDIOLAB (5): mono PCM WAV Recorder, WAV Player, microphone
  amplitude Level Meter, approximate Guitar Pitch Estimator, eight-band
  Spectrum Viewer; microphone/speaker cannot operate simultaneously.
- FILETOOLS (5): restricted USER File Manager, Backup to BACKUP,
  Restore from BACKUP, Filename Search, Storage Inventory.
- Total **14** recognized SD firmware IDs; four P7 images each contain
  five modes. It remains **one installed OTA app at a time**, not
  direct SD executable loading.
- No physical original K132 has passed acceptance. No GPS, IR receiver,
  Bluetooth Classic, 5 GHz Wi-Fi, arbitrary file system root or audio
  codec hardware has been assumed.
- **Full 94-program backlog is not completed.** Remaining advanced
  apps and feature expansions are tracked in issue #3.

Details: docs/P7_FIELD_MEDIA_FILES_IR.md and docs/P7_ACCEPTANCE.md.

## P8 Networking & Internet collection (9 modes, 1 new image)

- `NETTOOLS`: Wi-Fi Connect, DNS Lookup, genuine ICMP Ping, UTC NTP
  Clock, plaintext HTTP GET, local mDNS HTTP discovery, local MQTT
  subscriber, unauthenticated UDP LAN chat and minimal RSS XML title reader.
- Requires a user-created `/SUITE/NET/NET.CFG`. The file stores
  Wi-Fi credentials **in plaintext on SD** and is never committed to Git.
- HTTP, MQTT and chat are plaintext and limited to non-sensitive tests on
  trusted networks; authenticated HTTPS and TLS MQTT remain planned.
- App binaries in SD library: **15 total**. The P8 image hosts nine modes,
  not nine separately flashable app binaries.
- Hardware original K132 network operations, ICMP timeouts, SD and OTA
  return are **NOT VERIFIED**. See docs/P8_NETWORKING.md / P8_ACCEPTANCE.md.
- The complete original 94-item suite and advanced music features remain
  unfinished and tracked in GitHub issue #3.

## P9 MIDI expansion — five submenu tools in new MIDITOOLS firmware image

- Track Analyzer, recent-event pitch Piano Roll, first 12 Lyrics/Text
  events, validated SD Favorites, first 16-event Tempo Map.
- The resident MIDI player adds .KAR files and a timed lyric view
  driven by streaming MIDI meta events; no new hardware required.
- 16 known installable SD image IDs with MIDITOOLS, unchanged factory/OTA
  partition layout. Firmware source and P9 host tests are subject to CI.
- Source is **not** original-board certified; details at
  docs/P9_MIDI_EXPANSION.md and docs/P9_ACCEPTANCE.md.
- **Still planned:** sample GM patches, advanced tracker, MIDI seeking,
  chord labels, full playlist editor and rich karaoke graphics.

## P10 arcade + P11 productivity + Launcher 2.0 progress

- P10 `ARCADE2` source: Falling Blocks, 2-player Checkers, Memory Match,
  uptime-based Virtual Pet and short original Text Adventure. **Five
  program modes in one SD image**, host and firmware CI passed.
- P11 `POCKET11` source: 1023-byte text editor, 6×6 spreadsheet,
  flashcard trainer, display-only QR code generator, bounded random
  alphanumeric string maker. **Five modes in one SD image**, CI passed.
- Launcher 2.0 resident factory browser: categories, bounded
  case-insensitive search, favorites, four-item recent history,
  preferences safely stored on SD, explicit Y/N install confirmation.
- Recognized **18 installable SD firmware images** with .APP/.BIN pairs.
  Launcher 2.0 is a factory change, **not a 19th SD image**.
- Original physical K132 OTA/recovery/audio/keyboard/SD acceptance
  **NOT PERFORMED**. Do not interpret build success as release certification.
- Still unfinished in original 94 catalog: Chess, Solitaire, Sudoku,
  Dungeon Crawler, sampled GM patches, full BASIC Studio, encrypted
  vault, rich text, secure HTTPS, full RSS and other planned entries.

P10 details: docs/P10_ARCADE2.md. P11: docs/P11_POCKET_PRODUCTIVITY.md.
Launcher: docs/LAUNCHER2.md and docs/LAUNCHER2_ACCEPTANCE.md.
