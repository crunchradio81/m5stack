# P25 — Tempo-aware, incremental MIDI seeking
Target: original stock Cardputer K132 (no PSRAM/Grove; unchanged flash layout).

**J / F** in MIDI Studio or Karaoke rewinds/advances ten seconds of MIDI
musical timeline, following PPQN and embedded tempo changes rather than
a fixed number of ticks. Current song seconds appear on the player screen.
The speaker stops during seeking and the existing SD MIDI is re-read in
chunks of at most 128 merged events per UI poll.

The seek scan rebuilds program, expression, volume, sustain, pitch-bend,
time-signature and lyric state. It deliberately *does not retrigger notes
starting before the destination*, so long-held notes will resume only on
their next attack. This is not sampled-GM or perfect state restoration.
While scanning press J/F again to adjust destination, P/Enter to select
play-on-completion versus paused, or B to cancel into the library.

Maximum 200,000 skipped events per seek; goal is bounded to first 24
hours of song time. Attempts beyond the guard stop with visible status.
Manual seek past the last event leaves player at the end and suppresses
automatic loop/next-song behavior.

Host-testable scanner and time arithmetic live in
`src/midi/Seek25Core.h`; `tests/test_seek25_core.cpp` covers tempo changes,
bounds, speed and paused timing. GitHub Actions must compile original K132
factory and all 30 SD firmware images.

**Physical K132 acceptance pending:** J/F with fast/dense MIDI, tempo
changes, pauses and consecutive seeks, near-end manual seek, karaoke
lyrics, silent note restoration, UI responsiveness, SD removals, speaker
stuck-note check, OTA return and USB recovery. No physical flashing in CI.
