# P7 acceptance — four SD collections on original Cardputer K132

**Hardware acceptance: NOT RUN.** Compiler and host-test success alone cannot
validate real SD writes, ADC/audio timing, radio coexistence, infrared reception
at a target device, ESP boot partition recovery, or battery safety.

## Cross-package, flash, safety
- [ ] Confirm stock **original K132 / ESP32-S3FN8 / 8 MB / no PSRAM**.
- [ ] Preserve firmware/NVS/SD backups before any destructive firmware update.
- [ ] The existing factory + OTA0 table remains unchanged; old ten SD images
      still boot, and four P7 app images are listed with correct SHA256/size.
- [ ] All four new SD apps boot on K132 and escape/backtick returns to factory.
- [ ] Confirm no accidental writes to factory, /SUITE/APPS or previous logs.
- [ ] Verify failure with absent/corrupt SD is recoverable without a crash.
- [ ] USB recovery from unknown OTA boot problem demonstrated.

## IR transmit collection
- [ ] GPIO44 sends NEC 38kHz carrier at measured timing/length.
- [ ] Load two real, *verified owner device* IR profiles from microSD.
- [ ] Confirm address/command payload matches receiver's known NEC codes.
- [ ] NEC Remote / Media Controller / Shutter modes send only selected code.
- [ ] Manual three-step macro order and 500ms spacing are correct.
- [ ] Escape/B cancels a pending macro; no IR input/learning is asserted.
- [ ] Untrusted malformed profiles are rejected and never sent.

## Wi-Fi/BLE Field Lab
- [ ] Wi-Fi asynchronous scan lists known 2.4GHz SSID/BSSID/RSSI/channel.
- [ ] S auto/paused behavior, R one-shot and selected signal graph work.
- [ ] Survey label manually entered, stored; site not interpreted as GPS.
- [ ] AP appearance/disappearance reflects scan-to-scan changes, including
      uncertainty when an AP was merely missed.
- [ ] BLE passive advertisements scanned (no connecting or pairing).
- [ ] Wi-Fi and BLE CSV under /SUITE/FIELD survive reboot; correct quotes,
      row counts, capped 1 MiB file and explicit erase confirmation.
- [ ] No unexpected radio coexistence failure or heap leak on repeated scans.
- [ ] Time in logs is boot-relative only; no real calendar time asserted.
- [ ] Confirm privacy-aware handling of BSSID/BLE MAC identifiers.

## Audio Lab
- [ ] Mic and speaker are never enabled together on original K132.
- [ ] Record 1, 10, 60 and 110 seconds of sound at 16k mono 16-bit.
- [ ] Standard WAV header byte counts match actual data exactly.
- [ ] Verify .NEW safe finalization and sound survives normal SD eject/power.
- [ ] Recorder refuses missing SD, stale .NEW, and 4 MiB size overflow.
- [ ] WAV player accepts valid 8/16k mono PCM16 and refuses invalid headers.
- [ ] No queued buffer corruption, popping, stuck speaker or playback hangs.
- [ ] Compare level/pitch estimate against known 440Hz and 82Hz source.
- [ ] Eight spectral bars respond plausibly to tone changes, not a calibrated FFT.
- [ ] Audio quality, timing, microphone clipping and battery usage documented.

## Safe SD File Tools
- [ ] USER-only file listing, preview, case-insensitive search and byte counts.
- [ ] Create empty .TXT, rename and delete all require expected controls.
- [ ] Delete and rename cannot occur without a second Y confirmation.
- [ ] Backup -> BACKUP and Restore -> USER verify byte-for-byte hashes.
- [ ] Refuse overwrite, traversal, leading dot, >1MiB sources, missing SD.
- [ ] Interrupt a copy only on recoverable spare media, ensure .NEW handling.
- [ ] No ability to modify /SUITE/APPS, previous suite config or firmware.
- [ ] 40-file list limit and nonrecursive directory semantics shown accurately.

## Evidence
Board serial/SKU, code SHA, Actions build, UART trace, SD media sample hashes,
M5 microphone audio samples, IR receiver measurements if available,
photos/screenshots, start/stop logs, memory/flash utilization and proof
of safe return-to-factory required. PR remains draft until actual tests pass.
