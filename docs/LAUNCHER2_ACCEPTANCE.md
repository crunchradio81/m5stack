# Launcher 2.0 physical original K132 acceptance

**Not executed on a real Cardputer.**

- [ ] Verify original K132 has no PSRAM/Grove modules.
- [ ] Create a firmware/NVS/SD backup and prove USB recovery before
      installing the new factory firmware.
- [ ] Factory boots with zero, one, 18 and malformed SD application entries.
- [ ] C cycles all 9 categories; category assignment matches app IDs.
- [ ] S enters text query, Enter accepts, Backspace clears, and typing
      letters such as C/F/S does not accidentally operate launcher controls.
- [ ] F toggles favorites; file persists through reboot; 16-entry limit
      enforced without memory corruption.
- [ ] V displays most-recent app installations in correct order (last 4).
- [ ] Invalid/oversize/duplicate preferences cause read-only protection,
      not silent overwrite. .NEW and .BAK recovery tested.
- [ ] R rescans newly added/removed FAT32 app manifests correctly.
- [ ] Enter, N/Y confirmation; cancellation never changes OTA image.
- [ ] 18 known app manifests can be installed sequentially to OTA_0.
- [ ] Copy/verification failure never overwrites factory. Verify
      actual partition addresses and USB recovery after interrupted OTA
      only on an expendable bench device.
- [ ] Escape/backtick in each suite-managed app returns to factory.
- [ ] Display and keyboard scan stable at 240×135; no RAM exhaustion.
- [ ] Attach CI run, source SHA, physical photos/UART traces, SD backup,
      app checksums and explicit test record before releasing.
