# P9 physical original Cardputer K132 acceptance — NOT PERFORMED

- [ ] Original K132 firmware, SD and private MIDI media backed up.
- [ ] Factory MIDI player still plays Type 0/1 MIDI across tempo changes.
- [ ] `.KAR` library entry detected and used, alongside .MID/.MIDI.
- [ ] Lyric FF 05 and text FF 01 meta events shown in correct playback
      synchronization, no lost MIDI notes or audio buffering regressions.
- [ ] K opens karaoke, B/K returns player, Enter pauses/resumes.
- [ ] Non-ASCII/overlong lyric meta events safely sanitized/truncated.
- [ ] SD MIDITOOLS manifest accepted, installed to OTA_0 and boots.
- [ ] All five MIDI tools open an owner-supplied song and report accurate
      note/channel/tempo counts (cross-check with desktop MIDI utility).
- [ ] Long/damaged >4MiB and 30k-event files reject or give partial notice,
      with heap remaining stable and user navigation responsive.
- [ ] Piano-roll pitch bins show expected note-on sample events.
- [ ] Lyrics catalog limited to 12 entries; Fn arrows scroll.
- [ ] Favorites file saved, reloaded after power cycle, detects invalid
      and duplicate names; no unexpected SD overwrite.
- [ ] The P9 app returns to factory using Escape/backtick; loader factory
      layout and recovery behavior verified on the stock K132.
- [ ] Document source SHA, CI build, serial logs, screenshots and audio.
- [ ] Mark no release/PR production-ready until physical gates pass.
