# P5 — 23 new programs across four SD app collections

**Status:** Source implemented for the **original K132** (8 MiB flash,
no PSRAM, no Grove modules), but **not tested on a physical board**.
There are four additional firmware images in /SUITE/APPS; each is
selected through the factory SD launcher and contains a sub-menu.

## ARCADE.BIN — seven games

| Game | Function | Input |
| --- | --- | --- |
| Snake | Eat food, avoid border/body, score | Fn arrows/WASD; R restart |
| 2048 | Slide/merge numbered tiles once per move | Fn arrows/WASD; R restart |
| Minesweeper | Reveal or flag hidden cells (first click safe) | Arrows, Enter reveal, F flag |
| Sokoban | Push box onto goal; one level | Arrows; R restart |
| Pong | Left paddle against simple computer | Fn arrows/WASD |
| Breakout | Bounce ball, break bricks | Fn arrows/WASD |
| Word Guess | Guess word in up to six wrong letters | Type letters |

B goes back to collection menu; top-left backtick/Escape reboots
into the factory suite. Arcade game data is currently volatile:
high scores and persistent saves are still planned.

## OFFICE.BIN — five offline personal tools

To-do Manager, Contacts, Expense Tracker, Journal and Manual Calendar.
Office records are stored under /SUITE/OFFICE on a writable FAT32 card.
Lists use ASCII entries of at most 51 characters and 40 records each.

Use **N** to add a record, Enter to save, Fn arrows to select, **D** to
request deletion (Y confirms, N cancels), **R** reload. To-do Enter
toggles completion. Expense lines must end in a USD amount like
`Groceries 15.25`. Journal entries are single-line and do not
automatically have real dates. Calendar navigates months/years
manually; the original Cardputer has no reliable wall clock until
synchronized.

File writes use a temporary .NEW file and retain .BAK backup; never
assume a FAT32 microSD is power-loss transactional. Invalid/oversized
files are protected against silent overwrite.

## INSPECT.BIN — six read-only technical tools

Hex Viewer (8-byte rows and offsets), CSV Viewer (bounded fields),
ASCII-only JSON syntax validation/display up to 4 KiB, case-insensitive
filename Search under /SUITE (256-entry/depth-three cap), ASCII
character chart 0–127, and a bounded Storage Analyzer.

These programs do not edit, rename or delete files. They are not a
full Unicode JSON editor or complete microSD forensic scanner.

## MUSICLAB.BIN — five music-practice tools

- Morse Player: N enter letters/digits and Enter play encoded tones,
  R replay; no receive/decoding microphone claim.
- Tone Generator: 100–3000 Hz; Fn arrows adjust and S/Enter play/stop.
- Drum Sequencer: 16 steps, three tone voices, Fn arrows select
  row/column, Enter toggle, S run/stop, R reset.
- Interval Ear Trainer: compare two pitched tones, H higher/L lower;
  Enter replay and N new pair.
- Tap Tempo: Tap Enter/T/Space; last eight timings averaged, R clear.

These use the built-in speaker. Tone timbre/latency/battery usage
and keyboard matrix behavior must be verified on the physical K132.

## Building and SD installation

Eight targets: `sd_metronome`, `sd_piano`, `sd_channels`, `sd_ble`,
`sd_arcade`, `sd_office`, `sd_inspect`, `sd_musiclab`.
GitHub CI compiles all eight with the original K132 profile and
packages `Cardputer-K132-SD-App-Pack.zip` with matching SHA-256
`.APP` manifests. Copy the ZIP's `SUITE/APPS` to card root.

**Do not flash the SD `.BIN` images over the factory launcher**.
Read docs/SD_LAUNCHER.md for the custom flash partitions, backup and
one-time destructive initial install. Images are unverified hardware
firmware; SD integrity hashes are not digital signatures.
