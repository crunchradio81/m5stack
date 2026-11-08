# P24 — SD launcher capacity, manifest identity, bounded scan

**Target:** original stock M5Stack Cardputer K132 (ESP32-S3FN8,
8 MiB flash, no PSRAM, no Grove attachments).

P24 hardens the permanent factory launcher before expanding the 30
native SD-app packages shipped by P23 (C64 is an optional separately
packaged GPL image). It DOES NOT change partitions, provide 64 OTA slots,
or automatically install apps. One selected firmware at a time is copied
into the existing 3 MiB OTA_0 partition with explicit Y confirmation.

## Changes

- Catalog capacity rises from 32 to **64** manifest entries; IDs remain
  restricted to the exact known-suite allowlist. This is reserved space
  for future approved IDs, not an open firmware execution feature.
- Scan at most **256 FAT32 directory entries** per explicit refresh. If
  there are more entries, the UI reports an incomplete list rather than
  indefinitely walking a cluttered or malformed SD directory.
- A valid manifest is accepted only when its **basename equals
  `<ID>.APP` exactly**; content claiming another app ID is ignored.
  Duplicate declared app IDs are rejected; the first accepted
  occurrence wins. Files are then sorted alphabetically by app ID.
- SD packager rejects a duplicate environment/ID and more than 64
  entries before opening images or producing a ZIP.
- Existing SHA256 verification, allowlist, image limits, explicit
  confirmation, factory-to-OTA checks, favorites and recent lists remain.

The normal package has around 60 .APP/.BIN directory entries, so it
fits the 256-entry scan limit. A cluttered directory is not silently
treated as fully discovered; place only managed app images here.
No new third-party firmware trust is implied by any of these checks.

## Automated regression gates

Run `bash tools/run-host-tests.sh`. The SD manifest tests cover exact
basename matching, rejecting a mismatched ID, duplicate ID protection,
and 64-entry bounds. Launcher filtering is exercised at 64 entries.
The packager tests reject over-capacity and duplicate catalogs without
creating incomplete output ZIPs. GitHub Actions must compile the
original factory, all 30 SD targets and one combined ZIP.

## Original K132 hardware acceptance: NOT DONE

Before using the new factory image, owner must back up firmware and
microSD, confirm actual partition layout, and have a verified USB
recovery path. Test with the P23 app ZIP and optional C64 manifest:

1. Read the full app list and bottom entry; verify sorting, favorites,
   recent apps, category navigation and filtering at 240x135 pixels.
2. Add a separately named .APP whose contents claim an already present
   allowed ID; ensure it is rejected rather than displayed twice.
3. Create over 256 benign directory entries, then confirm that the
   launcher reports an incomplete list and remains responsive.
4. Explicitly install one known safe SD app and confirm OTA reboot,
   immediate return-to-factory guard and recovery over USB.
5. Power-cycle and reinsert SD; verify settings and saves. Do not claim
   FAT32 writes are transactional or SHA256 signatures authenticate code.

**No flashing or physical board acceptance performed in CI.**
No release or merge approval is implied by a passing source build.
