# P27 — Native Mac SD App Installer, 30-app archive preflight

Target: the original stock M5Stack Cardputer K132 (ESP32-S3FN8, 8 MB
flash, no PSRAM or Grove devices) and its native macOS companion.

**P27 does not change Cardputer firmware.** Its 30 native SD app images and
separate GPL C64 image are unchanged. Physical original K132 and Mac
installation have not been performed or certified.

## Native Mac tab

Open **Install SD Apps** in Cardputer Remote Mac. The app now has three tabs:
Macros & Mouse, LAN File Transfer, and Install SD Apps.

1. On a trusted GitHub Actions run, download the **complete 30-app
   Cardputer SD Apps archive** (not an individual firmware image and not
   the separate GPL C64 source package).
2. Connect/mount your own FAT32 microSD card on the Mac. Select the
   downloaded ZIP in the native file chooser, then select the **SD volume
   root**, not the SUITE or APPS folder. Do not select the Mac startup
   volume, your home folder, an unrelated USB drive or a network share.
3. Click **Inspect archive and SD card (read-only)**. The P27 validator
   checks all 30 known IDs, 30 .BIN and 30 .APP files, exact filenames,
   strict manifest fields, SHA-256 hashes, image ESP32 magic 0xE9,
   individual 3 MiB cap, and ZIP path/size limits. It then looks for
   existing files under SUITE/APPS without writing anything.
4. If every existing matching file is byte-identical, it is marked
   **already installed**. If any existing matching file differs,
   including case-only filenames, the entire copy is **refused**
   before writing. Existing SD owner files are never overwritten.
5. Check the explicit confirmation that you selected the correct
   FAT32 SD root. Then click **Copy verified apps to SD**. The helper
   creates SUITE/APPS only if missing and creates *only new* files
   using exclusive O_EXCL creation. Each new file is verified by a
   second readback; the .BIN image is published before its .APP
   manifest.
6. Eject the SD card safely using Finder before removing it.

**Cancellation / power loss:** already copied files remain. Any partial
new file also remains for explicit owner investigation. No cleanup script
deletes anything on the card automatically. Run Inspect again to detect
conflicts; recover or replace bad files yourself using an SD reader.

The app copies **SD application data only**. It never writes raw flash,
changes partitions, flashes the factory firmware, or launches any guest
binary. The matching original K132 factory launcher must have been
flashed separately over USB following docs/SD_LAUNCHER.md; do not use
this SD tool as a replacement for that step.

## Portable CLI

The exact same signed-app-bundled Python 3 standard-library helper is
usable on macOS or Ubuntu without additional packages:

    python3 Desktop/sdpack27.py plan \
      --archive /path/to/Cardputer-SD-Apps.zip \
      --target /Volumes/MY_CARD

After reviewing the read-only plan and confirming the path is your SD
volume root:

    python3 Desktop/sdpack27.py install \
      --archive /path/to/Cardputer-SD-Apps.zip \
      --target /Volumes/MY_CARD \
      --confirm-copy

Paths above are EXAMPLES; supply your actual downloaded archive and SD
volume mount. No sudo or root access is required for a writable card.

A complete package is expected to have 30 known K132 IDs and exactly
61 ZIP members (README plus 60 app files). An incomplete, mixed-version,
modified or third-party app collection is refused. The separate C64
package is not part of this bundle and is not automatically copied.

## Security limitations

The SHA-256 in an .APP checks **data corruption**, not the identity of
a software publisher. An adversary can replace a .BIN and generate a
matching .APP checksum. Only install firmware archives from a
repository/source you trust. P27 is not a firmware signature service,
antivirus scanner, notarization tool or permission bypass.

P27 explicitly rejects nested/extra ZIP paths, duplicate entries,
symlink ZIP entries, oversized images, wrong hashes, invalid app IDs,
bad ESP32 image magic and differing existing destination files. It
does not decompress arbitrary pathnames into the selected folder: only
a fixed whitelist of K132 firmware/manifest filenames is written.

## Verification gates

Automated macOS and Ubuntu tests:
- inspect complete generated 30-app archives without touching SD
- create-only installation and idempotent second copy
- rejection of changed images, corrupt manifests, unexpected ZIP paths
  and symlink entry/tree redirection
- no overwrite for differing files, case-collisions or partial binaries
- rejection of root/home destination or missing confirmation
- native SwiftUI app build, helper inclusion in ad-hoc-signed app,
  ZIP extraction, first installer run and repeat-install refusal
- all previous P26 Mac pairing and P24 LANFILE network tests remain green

Physical gates still needed:
- FAT32 card mount and native file panels on the owner's Mac
- readback via actual SD card, correct free-space behavior, unplug,
  actual factory launcher selection/OTA return and USB recovery.
- Installer cannot certify board hardware without those tests.

Keep P27 as a draft until the owner approves real-device acceptance.
