# P13 music production physical original K132 acceptance — OPEN

- [ ] Original stock K132 identified, not ADV, no PSRAM/Grove devices.
- [ ] Owner firmware/NVS/SD backups and USB recovery validated.
- [ ] MUSIC2 image appears in factory library and boots to OTA0.
- [ ] Track 16-step changes, 4-track arpeggio/audio/mute/program settings,
      tempo range, UI selection and stop work without hung I2S.
- [ ] Drum machine sequence produces approximate kicks/snare/hat/tom;
      no claim of commercial sampled sound.
- [ ] Playback loops reliably for 30 min; no buffer reuse corruption,
      pops, heap leak or battery overheating.
- [ ] Chord quality root calculations and voice audio compared against
      known C Major, A Minor 7, G7, etc.
- [ ] Arpeggiator up/down/up-down notes and timing validated.
- [ ] Morse quiz: random groups, correct dot/dash timing, reveal/hide.
- [ ] Instrument preset audition uses on-board speaker without mic conflicts.
- [ ] Save/reload TRACK.SEQ and DRUMS.SEQ verified bit-for-bit after reboot.
- [ ] Corrupt pattern file protected; simulate interrupted .NEW on spare SD.
- [ ] Escape/backtick returns to factory launcher; USB recovery tested.
- [ ] Record board model, code SHA, CI artifacts, UART logs, SD hashes,
      photo and audio samples before release.
