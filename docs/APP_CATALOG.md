# Original Cardputer — complete application-suite backlog

**Owner direction:** build the full catalog, with **MIDI Studio first**. All apps
must remain compatible with the **original K132**, without Grove units, external
sensors, PSRAM, a camera, GPS, 5/6 GHz Wi-Fi or a headphone socket.

Statuses: **Built source** means source code exists, not necessarily merged or hardware verified;
**Active** means the app is under implementation; **Planned** means no claim of code.

## P1 platform (built source, hardware acceptance pending)

Home launcher, Notes editor, Calculator, read-only File Browser, Text Viewer,
Wi-Fi Scanner, Wi-Fi Logger, System Info, Settings (9 components).

## P2 media — first major subsystem

1. **MIDI Studio** — **Built source, hardware pending:** Standard MIDI Files Type 0/1, PPQN,
   multitrack timing, song library, synth, playback, mute/solo, loop, speed
   control. Full GM sample-based accuracy is not currently supported.
2. MIDI Lyric/Karaoke Viewer — **Planned:** text and synchronized lyric events
   in MIDI/KAR where present.
3. Advanced Instrument Library — **Planned:** storage-efficient instrument
   patches, not PSRAM-only SF2 loading.
4. MIDI Track Analyzer — **Planned:** notes, tempo map, channel programs,
   event browser and instrument activity.
5. MIDI Playlist Manager — **Planned:** playlists/favorites, sorting, resume.
6. Voice Recorder — **Planned:** microphone audio to SD; no simultaneous capture/play.
7. WAV Player — **Planned:** mono streaming.
8. Music Tracker — **Planned:** patterns and sample-lite instruments.
9. Piano — **Planned:** Cardputer keyboard synthesizer.
10. Drum Machine — **Planned:** pattern sequencing.
11. Metronome — **Planned:** accurate timing/beats.
12. Audio Level Meter — **Planned:** microphone RMS/peak.
13. Guitar Tuner — **Planned:** microphone pitch estimator.
14. Morse Trainer — **Planned:** generate/decode practice tones.
15. Audio Spectrum Viewer — **Planned:** FFT of microphone samples.

## P3 wireless, networking and Internet (15)

16. Wi-Fi Signal Graph — **Planned**
17. Wi-Fi Channel Analyzer — **Planned**
18. Wi-Fi Survey Notebook with manually named survey points — **Planned**
19. Wi-Fi History Explorer / comparisons — **Planned**
20. Wi-Fi Appearance/Disappearance Monitor — **Planned**
21. BLE Advertisement Logger — **Planned**
22. BLE Signal Analyzer — **Planned**
23. BLE GATT Explorer — **Planned**, user-consented device connections only
24. Ping / latency diagnostics — **Planned**
25. DNS Lookup — **Planned**
26. LAN Service Discovery (mDNS/SSDP) — **Planned**
27. LAN Throughput Test — **Planned**
28. MQTT Monitor — **Planned**
29. Internet Radio — **Planned**, codec/bitrate constrained
30. Local LAN Chat — **Planned**
31. Weather Dashboard — **Planned**, provider/API required
32. World Clock / NTP — **Planned**, time unavailable before sync
33. RSS Reader — **Planned**
34. Wikipedia Text Reader — **Planned**
35. Lightweight Web Text Browser — **Planned**
36. HTTP / REST Client — **Planned**
37. LAN File Transfer Server — **Planned**
38. Smart-home MQTT Controller — **Planned**

## P4 productivity (14)

39. Full Text Editor — **Planned**
40. Searchable Notebook — **Planned**
41. To-Do Manager — **Planned**
42. Calendar — **Planned** (needs clock sync/manual date)
43. Contacts — **Planned**
44. Scientific Calculator — **Planned**
45. Programmer Calculator — **Built source, hardware pending**
46. Spreadsheet Mini — **Planned**
47. Expenses / Budget Tracker — **Planned**
48. Password Generator — **Planned** (system RNG)
49. Encrypted Password Vault — **Planned** (security review required)
50. Personal Journal — **Planned**
51. Stopwatch / Lap Timer — **Built source, hardware pending**
52. Countdown Timers — **Built source, hardware pending**
53. Unit Converter — **Built source, hardware pending**

## P5 programming and diagnostics (12)

54. BASIC Studio — **Planned**
55. Bounded Lua Interpreter — **Planned**, only if memory permits
56. Syntax-highlighted Code Editor — **Planned**
57. Hex Viewer / Editor — **Planned**, backup-required writes
58. USB Serial Console — **Planned**, available USB roles only
59. ASCII / Unicode Table — **Planned**
60. Binary/Octal/Decimal/Hex Converter — **Partially in Programmer Calculator (source)**
61. Logic and Bit Calculator — **Partially in Programmer Calculator (source)**
62. JSON Inspector — **Planned**
63. QR Code Generator — **Planned** (display-only)
64. CHIP-8 Interpreter — **Planned**
65. SD Benchmark — **Planned**

## P6 infrared tools (5)

66. IR Universal Remote — **Planned** (transmit only)
67. IR Device Profiles — **Planned**
68. IR Media Center Controller — **Planned**
69. IR Camera Shutter — **Planned**, supported camera models
70. IR Macro/Scheduler — **Planned** (no receiver; cannot learn signals)

## P7 arcade, puzzles and interactive (15)

71. Snake — **Planned**
72. Falling Blocks — **Planned**
73. Dungeon Crawler — **Planned**
74. Minesweeper — **Planned**
75. Chess — **Planned**
76. Checkers — **Planned**
77. Solitaire — **Planned**
78. Sokoban — **Planned**
79. 2048 — **Planned**
80. Pong — **Planned**
81. Breakout — **Planned**
82. Word Guess — **Planned**
83. Text Adventure Interpreter — **Planned**
84. Virtual Pet — **Planned**
85. Retro Arcade Frontend with saves / high scores — **Planned**

## P8 core system utilities (9)

86. Full SD File Manager — **Planned**, safe operations and confirmation
87. File Search / Content Search — **Planned**
88. Storage Analyzer — **Planned**
89. Backup / Restore Manager — **Planned**
90. Battery Monitor — **Planned**
91. Firmware Information / Recovery Guide — **Planned**
92. Theme Editor — **Planned**
93. System Diagnostics — **Planned**
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
