# P1 physical acceptance — original Cardputer K132 only

Status: **NOT EXECUTED** until tests are performed on a real stock K132.

## Pre-flight
- [ ] Confirm original (not v1.1 / ADV) 8 MB no-PSRAM board.
- [ ] Back up currently running firmware and all important microSD files.
- [ ] Confirm a FAT32 microSD card with non-critical test data; optional no-SD test.
- [ ] Complete host tests and PlatformIO compile without errors.
- [ ] Keep USB serial console attached, capture boot log.

## Keyboard/display
- [ ] Landscape screen renders without clipping or excessive flicker.
- [ ] Home W/S and Fn+Up/Fn+Down navigate, Enter launches.
- [ ] Top-left backtick and Fn+backtick return to Home.
- [ ] Typing several keys in succession captures each char once.
- [ ] Note insertion, cursor movement, CR/LF, backspace, forward delete pass.
- [ ] Ctrl+S saves; leaving editor with autosave enabled saves.
- [ ] 4096-byte limit does not corrupt text or crash.

## Storage
- [ ] Boots and works without SD; clearly warns that text is volatile.
- [ ] With SD, creates /SUITE and saves/loads NOTES.TXT on reboot.
- [ ] A 4097-byte note is rejected without overwrite.
- [ ] Binary/UTF-8 note is rejected without overwrite.
- [ ] Folder navigation within /SUITE works, parent does not escape scope.
- [ ] Text preview opens under-4KiB ASCII and rejects other formats/oversize.
- [ ] After controlled interruption with only NOTES.TXT.BAK left, reboot restores it.
- [ ] If a save fails, NOTES.TXT or .BAK retains the last good copy.

## Wireless, system and settings
- [ ] Wi-Fi scan works without joining any network.
- [ ] Scan can be cancelled and repeated without excessive heap loss.
- [ ] Reported heap, uptime and flash values look plausible.
- [ ] Brightness changes and persists over power cycle with SD.
- [ ] Autosave preference persists; no-SD changes remain volatile.

## Wi-Fi Logger field acceptance
- [ ] No SD: Start refuses to record, explains why, and does not silently switch to RAM.
- [ ] FAT32 SD: Start creates /SUITE/WIFI/WIFI.CSV with exactly one CSV header.
- [ ] Station remains disconnected; passive scans are used (no joining networks).
- [ ] Nearby 2.4 GHz access points appear with SSID or (hidden), BSSID, dBm, channel, security.
- [ ] Running repeated scans appends rows for a known BSSID (not deduplicated); observe changing RSSI while walking.
- [ ] Pause stops scanning and appending; resume continues in the same session.
- [ ] Enter on a live AP shows detail; V opens saved record viewer; Fn+Up/Down walks older/newer records.
- [ ] Power cycle preserves WIFI.CSV; after reset, new session ID and boot-relative timestamps are distinguishable.
- [ ] CSV quoting works for commas, quotes, empty/hidden names, and spreadsheet-formula-looking SSIDs.
- [ ] Remove or fill SD during recording: app pauses and displays an error; do not claim a record was stored if it wasn't.
- [ ] Interrupted write: incomplete trailing row is ignored in viewer and separated before the next append.
- [ ] Erase, N: CSV remains unchanged; Erase, Y: only WIFI.CSV is deleted, NOTES.TXT and CONFIG.TXT survive.
- [ ] Leave logger via Home: recording pauses, and subsequent normal Wi-Fi Scan still operates.
- [ ] Test long survey and verify RSSI display, heap stability, battery usage, no crash or stuck scan.
- [ ] No client sniffing, 5/6 GHz, GPS coordinates, or wall-clock timestamps claimed.

## Evidence to collect
Record actual hardware ID, PlatformIO toolchain/library versions, CI run,
serial logs, memory delta, SD test media, pass/fail per gate, and any firmware
recovery needed. No real-hardware PASS until those artifacts exist.
