# P4 acceptance — SD app launcher and four standalone programs

**STATUS: NOT EXECUTED ON THE ORIGINAL K132.** Only declare a PASS
after an actual stock M5Stack Cardputer is flashed and exercised.

## Pre-flight and critical recovery

- [ ] Confirm hardware is **original K132 / ESP32-S3FN8 / 8MB flash / no PSRAM**.
- [ ] Preserve firmware, internal NVS and SD backups. Do not casually erase.
- [ ] USB serial flashing recovery tested on this particular board.
- [ ] Verify all 5 PlatformIO profiles compile; use same commit for factory
      and 4 standalone OTA apps.
- [ ] Confirm custom partition table exactly matches:
      factory at 0x10000 size 0x300000, OTA_0 at 0x310000 size 0x300000.
- [ ] Perform first-time flash erase **only with owner approval**.
- [ ] Factory launcher boots after installation and after power loss.
- [ ] Boot without SD shows a useful error without crashing.

## SD library and installer

- [ ] Put CI ZIP's SUITE/APPS directory on FAT32 SD root.
- [ ] Home shows SD App Library and can browse all four manifests.
- [ ] R rescan discovers newly copied files.
- [ ] Enter opens install confirmation. N/backspace cancels without flashing.
- [ ] Y initiates SHA256 read, image validation and OTA write, with visible status.
- [ ] Missing BIN, wrong size, wrong SHA, unknown ID, bad/overlong manifest and
      invalid image magic are rejected with no factory overwrite.
- [ ] Confirm the factory partition contents stay unchanged (hash before/after).
- [ ] Detect missing/failing SD and insufficient OTA space, remain in factory.
- [ ] After install, device boots the intended standalone program.
- [ ] Return via Home/Escape, confirm factory launcher comes back.
- [ ] With an app installed, ordinary device reset returns to factory.
- [ ] Sequentially install all four apps, verifying replacement works.
- [ ] Intentionally interrupt an OTA write only on a recoverable bench and
      confirm firmware/USB recovery (not mandatory on precious production device).
- [ ] Do not install arbitrary external binaries; hash alone is not a signature.

## Metronome

- [ ] S/Enter play/pause, accent on first beat, accurate 30–260 BPM.
- [ ] Fn arrows adjust tempo, M changes 2–12 meter and D subdivisions.
- [ ] Verify audio clicks use built-in speaker, no peripherals.
- [ ] Check 30, 120 and 260 BPM with independent timing measurement.
- [ ] Exit returns to factory without stuck sound.

## Music Keyboard

- [ ] A S D F G H J K play white keys; W E T Y U accidentals.
- [ ] Simultaneous/rapid note presses do not exhaust speaker channels.
- [ ] Fn Up/Down transposes octaves within -2..+2.
- [ ] +/- adjust volume and exit releases speaker.
- [ ] Verify approximate pitches independently; no external MIDI hardware.

## Wi-Fi Channel Analyzer

- [ ] S initiates passive 2.4GHz scan, no joining networks.
- [ ] All valid channels 1–14 displayed with AP counts and peak dBm.
- [ ] Fn Left/Right select channel; displayed summary matches known APs.
- [ ] Suggested 1–11 channel marked a **heuristic**, not regulatory guarantee.
- [ ] Scan/exit repeated 30 times without stale radio or leak.
- [ ] Never imply 5/6 GHz scanning on the original K132.

## Bluetooth LE Explorer

- [ ] BLE passive discovery lists name/address/RSSI for up to 24 advertisements.
- [ ] S rescans; Fn Up/Down selects; Enter views detail.
- [ ] No unsolicited GATT connection, pairing or client tracking.
- [ ] Tested with at least two nearby BLE advertising devices.
- [ ] Repeated scans checked for memory exhaustion or UI hangs.
- [ ] No claim of Bluetooth Classic compatibility or precise distance.

## Final evidence

Attach source SHA, CI run, hardware boot/serial logs, snapshots of
each program, return-to-factory proof, USB recovery proof, and exact
memory/flash profile. Until then, the ZIP and factory build are
**unverified experimental firmware**, not a supported public release.
