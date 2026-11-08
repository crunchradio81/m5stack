# P16 theme/profile acceptance — stock original Cardputer K132

**NOT PERFORMED ON PHYSICAL HARDWARE**

- [ ] Confirm original K132, 8 MiB flash and no PSRAM/Grove dependencies.
- [ ] Back up factory flash, NVS and SD; verify USB restore before updates.
- [ ] Factory launcher plus all 23 SD app builds compile for stock K132.
- [ ] THEMES.APP manifest hash, OTA_0 installation and return to factory work.
- [ ] All six presets render legibly on 240×135, especially High Contrast.
- [ ] Each SD app header/background/selection adopts the active palette
      when newly launched; original MIDI player adopts theme after reboot.
- [ ] Profile activation changes theme and brightness on subsequent boot.
- [ ] Three profile names survive reboot, reject invalid/duplicate names.
- [ ] Speaker volume preference applies in Piano, Metronome, Music Lab,
      WAV player, MIDI Studio and Music Production without I2S faults.
- [ ] Factory Notes autosave respects the selected profile and preserves
      prior P1 settings if no profile configuration is installed.
- [ ] Corrupt checksum, duplicate keys/names, invalid profile values and
      oversized config are rejected without clobbering the original file.
- [ ] Save creates .NEW/.BAK; interrupted writes can be manually recovered
      from a spare test card without data loss beyond FAT32 limitations.
- [ ] Backup Restore requires R then Y; never overwrites a verified
      current file or consumes an invalid backup.
- [ ] 100 profile switches/SD-app relaunches without heap leak or hang.
- [ ] Remove SD and verify graceful fallback to original palette.
- [ ] Attach photos, original K132 hardware ID, CI SHA, UART logs,
      SD file hashes, real OTA boot/return and USB-recovery evidence.
- [ ] Keep PR draft and firmware artifact labeled UNVERIFIED until
      physical acceptance is complete.
