# P17 original Cardputer K132 physical acceptance — OPEN

**No physical device has been tested.** Source builds alone are not release evidence.

- [ ] Confirm original, stock K132 (not ADV), 8 MiB flash, no PSRAM/Grove.
- [ ] Back up the entire SD, original firmware and NVS; test USB recovery.
- [ ] Factory shows NOTEBOOK and SAFEFILES .APP manifests in the SD launcher.
- [ ] Install each into replaceable OTA_0; factory remains bootable on return.
- [ ] Create 32 notes, reject a 33rd; reject path traversal, DOS reserved names,
      duplicate names differing only by case, oversized and invalid files.
- [ ] Save 1,023-byte note, verify exact SD content, reload after power cycle.
- [ ] Unsaved edits require confirmation before discard.
- [ ] Name and contents search (mixed case, multiline), Ctrl+E opens match.
- [ ] Note backup refuses overwrite and byte-compares source/temp/final.
- [ ] Note restore produces new R1- and R2- filenames without changing
      existing primary or backup bytes.
- [ ] Corrupt note refuses editing/save; .NEW and .BAK recovery demonstrated
      on a spare SD after interrupted write.
- [ ] SafeFiles accepts user-owned files only in /SUITE/USER;
      refuses nested paths, firmware paths, invalid names and >256 KiB.
- [ ] 512-byte streaming file backup produces an exact binary copy including
      zero bytes and non-ASCII data, with no stack/heap exhaustion.
- [ ] Refuse same-name backup target; no operation deletes another user file.
- [ ] Restore creates a new R#-name; compare mismatched/missing backups.
- [ ] Remove SD mid-operation on expendable spare card and demonstrate
      correct failure without claiming automatic repair.
- [ ] Repeated navigation, 100 searches/copies, stable stack/heap, no lag/hang.
- [ ] Log original device model, CI SHA, OTA recovery, file hashes, screenshots,
      serial logs and confirmation controls before certification.
- [ ] Keep PR draft and firmware artifact labeled UNVERIFIED until accepted.
