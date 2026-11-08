# P17 — Notebook + Safe SD File Library, original Cardputer K132

Two *separate* replaceable SD firmware packages extend the resident
factory-launched suite: **NOTEBOOK** and **SAFEFILES**. They increase
the known packaged SD image count from **23 to 25**. Neither runs
executables directly from SD, changes partitions, or touches the factory
flash image. The stock K132's 240×135 display and built-in keyboard
are the only input/output requirements; no PSRAM, Grove peripheral,
Wi-Fi, cloud account or external server is needed.

## 1. NOTEBOOK.BIN — 5 program screens

- **Multi-file Notes:** create up to **32** note files, each containing
  no more than **1,023 bytes** of ASCII text/newlines, under
  \`/SUITE/NOTES17/\`. Filename stems are 1–18 ASCII letters,
  digits, underscore or hyphen; DOS device names and traversal are
  rejected. Names end in \`.TXT\`. Existing notes can be opened and
  edited; saves require **Ctrl+S**, not an automatic overwrite.
- **Content Search:** a case-insensitive substring search over the
  bounded note titles and bodies, showing up to 32 results. Use
  **Ctrl+E** to open the highlighted result.
- **Backup Note:** copies one note to \`/SUITE/BACKUP/NOTES17/\` only
  after a **Y/N confirmation** and read-after-write verification.
  Existing backups are never overwritten by this operation.
- **Restore Safe Copy:** restores a verified note backup as a *new*
  \`R1-...\` through \`R9-...\` filename under the notes directory.
  The current primary note is never overwritten.
- **Note Integrity:** displays the selected note's byte count and
  FNV-32 checksum. The hash is diagnostic only, *not* authenticity or
  data-integrity security against deliberate modification.

**Keyboard:** Fn Up/Down selects a note, Enter opens it. **N** starts
a note with a new safe name. Inside the editor, Left/Right moves
the cursor, Backspace deletes, Enter inserts newline, **Ctrl+S**
explicitly saves, **Ctrl+B** asks before discarding unsaved edits.
**B** returns to the notebook submenu. In backup/restore modes,
Enter requests a copy and **Y** confirms; any other key cancels.

On save, the application creates \`NAME.TXT.NEW\`, reads back and
compares the bytes, preserves the prior note as \`NAME.TXT.BAK\`,
then renames the verified temporary file and compares the final
stored contents. Existing malformed primary note data is protected
against overwrite.

## 2. SAFEFILES.BIN — 5 program screens

- **Browse USER Files:** bounded list of user-owned files in
  \`/SUITE/USER/\`, with filename and size.
- **Browse Backups:** bounded view of user-file copies under
  \`/SUITE/BACKUP/USER/\`.
- **Verified Backup:** after Y/N confirmation, copy the highlighted
  USER file into BACKUP/USER with a 512-byte streaming buffer.
  Refuse to overwrite a pre-existing backup.
- **Safe Restore Copy:** restore a selected backup into the USER
  directory with a new non-overwriting \`R1-\`…\`R9-\` filename,
  preserving both the previous USER version and the backup.
- **Compare Source and Backup:** stream both files and compare exact
  bytes, reporting MATCH or missing/mismatch.

Limits: **32 displayed files** per directory, at most **128 scanned
directory entries**, up to **256 KiB per file**, safe 1–24-character
stems and optional extension up to 8 characters, and no directory
recursion. Copy performs a source/temp comparison before the rename
and a source/final comparison afterward. Broken/partial \`.NEW\`
files are preserved for manual recovery rather than silently erased.

Only the exact two whitelisted data folders above are touched.
**There is no delete, destructive rename, format, arbitrary root
browser, firmware writer, automated scheduler, external drive support,
or cloud sync.** Files in \`/SUITE/APPS\`, firmware/OTA images, Wi-Fi
profiles, keys and private suite configuration are deliberately outside
this tool's writable scope. The previous P7 restricted File Tools
remain separately available.

## Failure handling and limitations

- FAT32 lacks fully atomic transactional rename; unexpected power
  loss or removal during a copy can leave \`.NEW\` or \`.BAK\`. Make
  computer-side backups of important data first.
- Existing same-name destination files are always refused, not
  replaced. Manually resolve filename collisions on a PC.
- The notebook is **not** a full-text, Unicode, rich-text or
  multi-megabyte document editor; no indexing database is stored.
- The SafeFiles utility does not certify that a removed SD card is
  trustworthy. It detects byte mismatch, not malicious replacement
  of all copies. Do not store secrets on the unencrypted FAT32 card
  expecting confidentiality.
- The factory K132 launcher installs the selected .BIN into the
  replaceable OTA_0 slot after a manifest SHA256 check and explicit
  confirmation. SHA256 here is a corruption check, *not* authenticated
  firmware signature verification.
- An initial owner-approved backup and one-time destructive flash
  erase are still necessary for first adoption of the existing
  custom factory/OTA partition scheme. P17 does not modify it.

## Engineering

Portable core:
\`src/core/Notebook17Core.h\`,
\`src/core/SafeFiles17Core.h\`.
Device UIs:
\`src/Notebook17Collection.h\`,
\`src/SafeFiles17Collection.h\`.
Portable regressions:
\`tests/test_notebook17_core.cpp\`,
\`tests/test_safefiles17_core.cpp\`.
Firmware targets:
\`pio run -e sd_notebook -e sd_safefiles\`.

**No actual original K132 was flashed, erased, or physically tested.**
Source builds, GitHub Actions status, hardware/OTA recovery and SD
failure simulations are separate requirements. See
\`docs/P17_ACCEPTANCE.md\`.
