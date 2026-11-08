# P11 physical original Cardputer acceptance — NOT PERFORMED

- [ ] Original K132, no PSRAM/Grove; back up firmware/NVS/SD.
- [ ] POCKET11 appears in factory SD launcher, installs into OTA0 and
      safely returns to factory via Escape/backtick.
- [ ] Text editor handles 1,023 ASCII/LF chars, cursor and backspace,
      explicitly saves NOTE.TXT, reloads after reboot.
- [ ] Invalid/oversized NOTE.TXT refuses overwrite and preserves source.
- [ ] Spreadsheet supports 6x6 signed integers, correct row sums and
      CSV persistence, rejects malformed cells and oversized saves.
- [ ] Flashcards load twenty owner-authored Q|A records, refuse
      malformed decks, navigate and case-insensitively search.
- [ ] QR renders scan-ready at 240x135 for multiple known ASCII strings;
      verify on real phone/camera without extra Cardputer sensors.
- [ ] Random-string output is bounded, changes between runs, not saved
      to SD, and clear on leaving app; do not claim audited password security.
- [ ] Prove .NEW/.BAK logic and recovery on spare FAT32 card; no
      automatic overwrite of malformed user files.
- [ ] Keyboard Ctrl+B / Ctrl+S and UI navigation function as described.
- [ ] 100 mode changes without growing heap or a stuck app.
- [ ] Capture actual K132 screenshots, hashes, serial logs, source SHA
      and factory OTA recovery, before marking a PR release-ready.
