# P9 — MIDI Deep Tools and synchronized karaoke display

**Original stock Cardputer K132**, built-in speaker and microSD, 8 MiB
flash, **no PSRAM, Grove accessories or external MIDI interface**.

## Enhancements to the existing resident MIDI player

- Streaming SMF Type 0/1 parser now exposes standard FF 05 lyric and
  FF 01 text meta events. Events are bounded to 47 printable ASCII
  characters; longer text is truncated rather than allocating RAM.
- The MIDI player's library accepts `.MID`, `.MIDI` and `.KAR` files.
- During playback, **K** opens a synchronized lyric screen. Lyric text
  changes when the correct stream event is dispatched; **B/K** returns
  to the normal player. Play/pause is available on the lyric screen.
- **This is not a full karaoke renderer**: no scrolling word highlight,
  non-ASCII international code pages, subtitling without embedded lyrics,
  SoundFonts, GM sample bank or USB MIDI.
- The existing twelve-voice built-in-speaker waveform synth, mute/solo,
  transpose and tempo handling remain intact.

## Standalone P9 SD app: MIDITOOLS.BIN

Select **MIDI Deep Tools** from the existing factory SD App Library.
The new firmware collection has five local tools:

| Program | Implementation |
| --- | --- |
| MIDI Track Analyzer | Stream up to 30,000 MIDI events or 2 s, track note counts, format, PPQN, length in ticks, track/channel and program stats |
| Piano Roll | Graph the pitch of the latest 40 note-on events, bounded in RAM; this is an event-order pitch overview, **not** a full editor or metrically spaced DAW piano roll |
| Lyrics Catalog | Show first 12 bounded FF 05/FF 01 text/lyric events from a selected file; use arrows to scroll |
| Favorite Playlists | Mark/unmark files with F; persist up to 48 favorite filenames under `/SUITE/MIDI/FAVORITES.TXT`; **not** ordered multi-playlist editing |
| Tempo Map | Show first 16 tempo events as tick position and beats per minute |

File browser lists up to 48 safe `.MID`/`.KAR` filenames from
`/SUITE/MIDI`. No other directories are scanned. Each analysis file
is at most 4 MiB. All MIDI events stream from SD without loading songs
in RAM. Analysis might stop at a time/event cap and marks output partial.
No MIDI code, copyrighted songs, sound banks or ROMs are distributed.

## Favorite file safety

Favorites use ASCII filename entries validated against path traversal,
malformed data, duplicates and overlong names. Saves are written to
`FAVORITES.NEW`, with previous `FAVORITES.TXT` renamed to
`FAVORITES.BAK`. Invalid favorite files are kept read-only rather than
silently overwritten. FAT32 writes are not power-loss transactional:
back up your SD data.

## Packaging

`MIDITOOLS.APP` and `MIDITOOLS.BIN` extend the 15-image P8 SD
library to **16 firmware packages**. The firmware is copied from SD into
replaceable OTA_0 when launched; it is **not executed directly from SD**.
P9 also changes the factory MIDI app, so updating the factory requires
a deliberate USB firmware update/reflash (not a P9 SD app switch).
See docs/SD_LAUNCHER.md; the factory/OTA partition map itself is unchanged.

**No physical original Cardputer tests were performed.** Source tests
and PlatformIO compilation do not establish speaker/SD/keyboard behavior.
See docs/P9_ACCEPTANCE.md.
