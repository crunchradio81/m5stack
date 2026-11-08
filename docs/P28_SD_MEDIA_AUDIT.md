# P28 — Actual FAT32 removable-media guard and full SD integrity audit

P28 improves the native Mac Cardputer SD App Installer from P27.
It does not add ESP32 firmware or change the original K132 8 MB partition layout, 30 SD app images, or the standalone legacy emulator.

## Rationale

P27 could copy a valid 30-app package into an arbitrary writable folder if the wrong path was selected. P28 requires independently verified removable FAT32 volume metadata before a single file write.

- macOS: exact mounted /Volumes/<SD-CARD> root, verified by diskutil info -plist as MS-DOS/FAT32, external/removable and writable. An APFS or internal volume, an ordinary folder, network share or stale mount is rejected.
- Ubuntu/Linux: exact mounted vfat root of a real /dev/ block device, verified by findmnt. ext4, test folders, read-only mounts and network shares are rejected.
- Device identity is rechecked between file writes. Removing a card during copying halts the operation rather than redirecting subsequent files into the computer's internal folder.
- New .BIN images are created exclusively and verified by SHA-256 readback before .APP manifests.
- After copying, the installer rereads ALL 30 images and 30 manifests. It claims success only when all 60 match the selected archive.

## Mac GUI

The **Install SD Apps** tab has read-only **Inspect** and **Audit all 60 files already on SD** controls. Inspect checks the actual removable FAT32 card and the full trusted archive, then previews newly required/identical files. Audit reports any missing or changed image/manifest pair without deleting, repairing or replacing data.

After an explicit confirmation, Copy verifies each write and runs a full 60-file post-copy audit. Always eject safely in Finder. If a card is unplugged or an incomplete new file is left behind, recover it deliberately using an SD reader; there is no automatic cleanup.

## CLI

On an actual mounted FAT32 card (replace sample paths):

    python3 Desktop/sdpack27.py plan --archive /path/to/Cardputer-SD-Apps.zip --target /Volumes/MY_CARD
    python3 Desktop/sdpack27.py audit --archive /path/to/Cardputer-SD-Apps.zip --target /Volumes/MY_CARD
    python3 Desktop/sdpack27.py install --archive /path/to/Cardputer-SD-Apps.zip --target /Volumes/MY_CARD --confirm-copy

The application doesn't flash factory firmware, modify partitions, format cards, download resources or change unrelated data. App-manifest SHA-256 detects corruption, not authenticity. Install only known trusted build archives.

## Verification and outstanding acceptance

Mac/Ubuntu CI simulates diskutil/findmnt metadata and tests exact mount/FAT32 rejection, dangerous internal/wrong devices, unplugging, corrupt images, modified/case-colliding files, partial transfers, idempotence and successful whole-card audits. macOS SwiftUI build and ad-hoc app ZIP checks remain active.

Real Cardputer hardware, real SD card reader, macOS filesystem metadata variants, eject/reconnect, original factory launcher and USB recovery remain **unverified**. Preserve the draft PR pending those tests.
