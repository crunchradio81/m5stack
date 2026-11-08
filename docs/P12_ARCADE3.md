# P12 — Arcade Classics III (original Cardputer K132)

One SD firmware package `ARCADE3.APP` + `ARCADE3.BIN` contains four
offline games using the original built-in 240×135 display/keyboard.
No PSRAM, Grove peripherals, networks or copyrighted media needed.

| Game | Source feature coverage | Deliberate limits |
| --- | --- | --- |
| Local Chess | Two human players; normal piece movement, pawn double steps, captures, check/king safety, castling, en passant, promotion to queen, checkmate/stalemate detection | No computer opponent, clock, PGN/FEN import, repetition/50-move draw, underpromotion or online play |
| Klondike Solitaire | Real shuffled 52-card draw-one Klondike, 7 tableau columns, stock/waste, 4 suit foundations, alternating-color build, move face-up sequences, reveal covered cards and win check | One recycle, no undo/hints/scoring or foundation-to-tableau move |
| Sudoku | Owner-playable 9×9 Sudoku; fixed clues, digits/clear, row/column/3×3 conflict detection, up to 3 hints, rotated number variants and win check | One source puzzle rotated by digits, no uniqueness solver/editor or difficulty levels |
| Dungeon Crawler | Seeded 19×10 dungeon maps, three floors, corridor connectivity, key-locked stairs, hit points, a blocking monster, health potion and treasure, combat and ending | Simple original adventure only; no inventory, party, dialogue trees or savegame |

**Controls:** Fn arrow keys select the highlighted square or move in dungeon;
Enter selects chess piece/destination, opens a selected solitaire sequence,
or enters a Sudoku cell value via digits 1–9 and 0 to clear. Chess, Solitaire,
Sudoku and Dungeon reset with **R**; **B** returns to Arcade 3 submenu.

Solitaire: **D** draws stock; **F** places waste/selected tableau top on
foundation where legal; **G** places waste on the highlighted tableau;
Enter first chooses the sequence source at the selected face-up card, then
chooses destination. **Z** cancels the selection. The board view shows only
a compact window of up to six visible pile cards.

Sudoku: **H** reveals one solution digit on the selected non-given square,
up to three hints; conflicting entries are drawn red.

Dungeon: arrow keys move; moving into the monster attacks it, possibly
taking damage. The key is required to descend the stair tile.

## Engineering and hardware status

- Fixed RAM game logic: `src/core/Chess12.h`, `Solitaire12.h`,
  `P12Puzzles.h`; UI `src/Arcade12Collection.h`.
- Negative-case and rules tests: `tests/test_p12_arcade.cpp`.
- Compiled through `pio run -e sd_arcade3`; integrated into full K132
  SD manifest allowlist, packaging and CI.
- Extends the SD firmware library from 18 to **19** images, each stored
  on FAT32 SD and installed only into OTA_0 after confirmation. No
  direct ELF/binary execution from SD.
- Chess engine checks standard checkmate/stalemate in its supported rules,
  but is not a tournament-compliant referee due to missing draw variants.
- The actual original Cardputer K132 has **NOT** been flashed or physically
  exercised. Do not consider compiled artifacts production-safe.
- First adoption of factory/OTA flash layout requires owner-approved
  backup and one-time destructive erase. P12 does not change partitions.

Physical acceptance: `docs/P12_ACCEPTANCE.md`.
