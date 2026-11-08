# P13 — Music Production Studio on original Cardputer K132

`MUSIC2.APP`/`MUSIC2.BIN` is one more offline SD firmware collection,
containing six program screens. It uses only the original built-in
speaker, keyboard, display and FAT32 microSD, with **no PSRAM, Grove
module or external MIDI sound module**.

| App | Source implementation | Limit |
| --- | --- | --- |
| 4-Track Step Tracker | Four independent 16-step tracks, MIDI-note pitches, 40–240 BPM, per-track muting/program family, sound from bounded existing 12-voice synth, explicit SD saves | No timeline of many patterns, automation lanes, sample loading or song arrangement |
| 16-Step Drum Machine | Preset kick/snare/hat/tom gates with toggled steps, 16th-note playback, standard synth noise approximation on MIDI channel 10 | Approximate drums, not sampled acoustic kit; no pads/recording |
| Arpeggiator | Root/quality selection, up/down/up-down arpeggio notes generated in time | Limited to one chord at a time |
| Chord Dictionary | Eight common chord qualities for each of twelve roots; preview notes and audition via internal speaker | No audio chord recognition or figured-bass analysis |
| Morse Quiz Trainer | Reproducible five-letter quiz, generated new prompt, audible dots/dashes, answer reveal | No microphone decoding or calibrated certification |
| Instrument Presets | Audition 128 program-family indexes with the existing waveform synth | **NOT** GM sample-bank/SF2 fidelity or custom editable patches |

## Controls

Inside the submenu, use Fn Up/Down and Enter to open.
**B** returns to the submenu. Escape/backtick exits the SD firmware to
the resident launcher.

Tracker and Drum Machine: Fn Left/Right selects step, Fn Up/Down selects
track, Enter toggles step; **P** play/stop, **S** save to SD, **L** load,
**M** mute track, **Z** remove step; tracker has +/- semitone adjustment
and **V** next program family. Visual indicator shows step activity.

Arpeggiator: Fn Left/Right root, Fn Up/Down chord quality, **T**
cycle direction, **P** play/stop. Chord Dictionary shares root/quality
selection; Enter auditions the selected notes. Instrument Presets
uses Fn Up/Down program number, Fn Left/Right pitch and Enter audition.

Morse Trainer: **N** new five-letter phrase, **P** sound it, Enter
reveals/hides the answer.

## SD data, size and safety

Save paths are `/SUITE/MUSIC/TRACK.SEQ` and
`/SUITE/MUSIC/DRUMS.SEQ`; both are **88-byte, versioned binary
formats**, holding 4×16 pitch/step cells, four instrument programs,
tempo, four mute states and an FNV32 corruption guard. This checksum
is not authentication. Saves create a `.NEW` temp and keep the
previous file as `.BAK`. Invalid stored patterns are not overwritten;
fix/remove corrupt files on another computer first. FAT32 is not a
power-loss-safe database.

Synth playback is mono PCM on the built-in speaker, using three reused
256-frame buffers with at most two queued chunks on channel zero. The
original microphone and speaker share resources: P13 does **not**
use the microphone, and stops it before speaker operation.

P13 extends the SD image count to **20**; each SD package still
installs to OTA_0 after explicit user consent. The permanent factory
launcher and partition map are unchanged. The first factory+OTA
installation still requires backups and a deliberate one-time flash erase.

Automated C++ tests verify the sequence binary codec, corrupted data
rejection, basic note/chord generation and arpeggiator order. **No
physical original Cardputer K132 has been tested** for speaker latency,
timing, button behavior, persistence or recovery. See P13_ACCEPTANCE.md.
