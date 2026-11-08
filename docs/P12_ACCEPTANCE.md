# P12 stock original Cardputer hardware acceptance — OPEN

- [ ] Original M5Stack Cardputer K132 confirmed; 8MB flash/no PSRAM.
- [ ] Back up firmware/NVS and SD; verify USB recovery before new flash.
- [ ] ARCADE3 appears with SHA checksum and enters/exits factory correctly.
- [ ] Chess UI: all pieces visible, arrow targeting, both players' moves,
      castling, en passant, check/mate/stalemate and rejection of illegal moves.
- [ ] Solitaire UI: draw through stock, foundation movement, tableau
      sequence movement, card flipping, no duplicate/lost cards.
- [ ] Sudoku: digits 1–9/0 entry, fixed clues reject, conflict colors,
      three-hint limit, solved indication.
- [ ] Dungeon: seed produces reachable key/stairs, combat resolves, health
      and treasure tracked, all three floors accessible.
- [ ] No broken keyboard combinations or accidental Home/Escape flash writes.
- [ ] 100 game resets, memory stable and no out-of-bounds display writes.
- [ ] Battery/sleep behavior tested; factory OTA return and USB recovery
      shown with photos and UART evidence.
- [ ] Draft remains unmerged until actual device evidence exists.
