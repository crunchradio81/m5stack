# MIDI Studio P2.0 — original Cardputer hardware acceptance

Current status: **NOT HARDWARE-TESTED**. Compiler/host-test results do not certify speaker or keyboard operation.

## Preparation
- [ ] Confirm device is original K132/StampS3 (not ADV/V1.1) without PSRAM.
- [ ] Back up current firmware, user SD media and settings.
- [ ] Install test FAT32 SD; place self-created or permitted MIDI files in /SUITE/MIDI.
- [ ] Start at conservative speaker volume; use USB serial logging.

## MIDI file compatibility
- [ ] Type 0/1 files open, with correct track counts and PPQN.
- [ ] Test running status, velocity-zero note-off, overlapping chords, MIDI channel 10 drums.
- [ ] Test tempo-change and time-signature events at nonzero tick.
- [ ] Test instrument program selection (approximate waveform family, not GM sound samples).
- [ ] Test pitch bend, volume, expression, sustain pedal and All Sound Off.
- [ ] Reject Type 2, SMPTE division, malformed/truncated tracks, more than 16 tracks and >4MiB.
- [ ] Verify a 4 MiB MIDI can stream without RAM exhaustion or hangs.

## Player UI, sound and library
- [ ] Copy .MID/.MIDI files; list filters invalid extension, supports lowercase and uppercase.
- [ ] R refresh; Fn Up/Down and Enter select/load a song.
- [ ] Playback visibly advances; notes/chords produce audible sound with acceptable latency.
- [ ] Pause silences promptly, resume does not jump ahead after long pause.
- [ ] Z restart, L loop and N next work; B returns to library.
- [ ] +/- change speed (50-200%) without accidental extreme timeouts.
- [ ] [ / ] adjust speaker volume, moderate output avoids distortion.
- [ ] M mixer; individual track mute, solo and 12-voice cap function without stuck notes.
- [ ] Exit to Home stops playback and frees SD/speaker resources.
- [ ] After playback, Wi-Fi scan and Wi-Fi logger still work.
- [ ] Repeated 1-hour playlist test: monitor heap fragmentation, stalls, resets, speaker crackles.

## Fault injection and documentation
- [ ] SD removed during play: no crash, audio stopped, readable error.
- [ ] Damaged MIDI files cause bounded failure and UI remains navigable.
- [ ] Capture firmware SHA, CI run, board model, audio logs and pass/fail table.
- [ ] Clearly label synthesized instruments as approximations, not an SF2 / full General MIDI bank.

Do not merge the MIDI branch or call the release hardware-verified until checks pass.