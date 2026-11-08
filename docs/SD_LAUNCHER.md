# K132 Suite — SD Application Library and Factory Launcher

**This is a development-stage architecture, NOT a hardware-certified installer.**

## Purpose

The original M5Stack Cardputer K132 (8 MiB flash, no PSRAM) cannot
execute ESP32 application binaries directly from FAT32 microSD. This
project uses an **installed factory launcher** and a **replaceable OTA_0
application partition**. The fifteen separately compiled app packages are
kept on SD and copied into the OTA slot only when selected.

This differs from simply showing data files on SD or compiling every
program into the factory firmware. It allows a growing app catalog
without linking every application into the resident executable.

## Fixed 8 MiB partition table

| Partition | Address | Size | Use |
| --- | --- | --- | --- |
| nvs | 0x9000 | 0x5000 | Internal settings |
| otadata | 0xE000 | 0x2000 | Next-boot selection |
| factory | 0x10000 | 0x300000 (3 MiB) | Built-in suite, main launcher, app installer |
| sdapp (OTA_0) | 0x310000 | 0x300000 (3 MiB) | Latest installed SD application |
| spiffs | 0x610000 | 0x1F0000 | Reserved flash filesystem |

The resident factory code does **not** update itself through the
SD installer. It writes only to OTA_0 via the ESP32 Arduino Update API.

## On-card structure

After unpacking the CI SD app pack at the FAT32 card root:

~~~text
/SUITE/
  APPS/
    METRONOME.APP
    METRONOME.BIN
    PIANO.APP
    PIANO.BIN
    CHANNELS.APP
    CHANNELS.BIN
    BLE.APP
    BLE.BIN
    ARCADE.APP
    ARCADE.BIN
    OFFICE.APP
    OFFICE.BIN
    INSPECT.APP
    INSPECT.BIN
    MUSICLAB.APP
    MUSICLAB.BIN
    DEVTOOLS.APP
    DEVTOOLS.BIN
    SYSTEM.APP
    SYSTEM.BIN
    IRTOOLS.APP
    IRTOOLS.BIN
    WIRELESS.APP
    WIRELESS.BIN
    AUDIOLAB.APP
    AUDIOLAB.BIN
    FILETOOLS.APP
    FILETOOLS.BIN
    NETTOOLS.APP
    NETTOOLS.BIN
  NET/
    NET.CFG  (user-owned plaintext Wi-Fi credentials)
  IR/
    (your validated .IRP NEC profiles)
  FIELD/
    WIFI.CSV
    BLE.CSV
  AUDIO/
    REC000.WAV
  USER/
    (personal files safely managed by FILETOOLS)
  BACKUP/
    (file copies only)
  CHIP8/
    (optional owner-provided .CH8 ROMs)
  OFFICE/
    (created by the Pocket Office collection)
  MIDI/
    (your original .MID files)
  WIFI/
    WIFI.CSV
~~~

The `.APP` is a bounded ASCII manifest (name, known ID, image name,
byte size and SHA-256). The `.BIN` is the **app-only** ESP32-S3 image,
not a merged factory/bootloader binary. Only fifteen known suite app IDs
are accepted in this first milestone. Any malformed manifest, wrong
size, invalid image magic, or SHA mismatch cancels installation.

SHA-256 protects against accidental corruption **but is not a digital
signature or a trust guarantee**. Only use images you built or trust.

## How to build / install (developer instructions)

**Back up your existing firmware and internal settings first.**
**Changing partition tables is destructive**, and the first physical
install must be done over USB using a full flash erase. The SD data
remains on the removable card; preserve a separate backup anyway.

~~~sh
# From the source branch in this repository
pio run -e cardputer_original
pio run -e sd_metronome -e sd_piano -e sd_channels -e sd_ble \\
        -e sd_arcade -e sd_office -e sd_inspect -e sd_musiclab \\
        -e sd_devtools -e sd_system \\
        -e sd_irtools -e sd_wireless -e sd_audiolab -e sd_filetools \\
        -e sd_nettools

# Builds on host, creates portable SD library:
python3 tools/package-sd-apps.py \
  --build-root .pio/build --out Cardputer-K132-SD-App-Pack.zip
~~~

On the original device with the **owner's explicit approval to erase
its flash**, and after backing up data:

~~~sh
pio run -e cardputer_original -t erase
pio run -e cardputer_original -t upload
~~~

This first install erases previous internal data/firmware so that an
old OTA boot selection cannot conflict with the new factory layout.
Do not execute that erase command merely to test a source branch.

Alternatively, download the sixteen CI artifacts for **this same
commit**, including the SD app ZIP, from GitHub Actions. Prebuilt
binaries are **unverified** until physical board acceptance.

## On-device

1. Copy the ZIP's `SUITE/APPS` tree to the FAT32 SD card and insert it.
2. Boot the **factory suite launcher**.
3. Select **SD App Library** using W/S or Fn arrows, Enter to open it.
4. Choose among Metronome, Music Keyboard, Wi-Fi Channels, BLE Explorer,
   Retro Arcade, Pocket Office, SD Inspector, Music Practice,
   Developer Tools, System Toolbox, IR Remote Tools, Wireless Field Lab,
   Audio Lab, Safe SD File Tools, Networking & Internet.
5. Enter opens a confirmation screen. Y verifies and installs; N cancels.
6. After complete SHA check and write, the app boots from OTA_0.
7. **Home / Escape** in a suite-managed app restarts into the factory
   launcher. The app also arms the factory partition as its *next* boot
   before starting its drivers, so a normal subsequent reset returns
   to the launcher.

At present, exactly **one** SD firmware image is installed in the OTA
slot at a time. All fifteen binary images remain stored on SD. Changing apps
requires reflashing that one OTA slot, not the factory launcher.
This is normal, but flash erase/write cycles are finite: avoid using
it as a rapid background app switcher.

## Safety, failure and recovery

- **No GPIO modules, Grove attachments or external chips** required.
- Factory stays in its own partition when using this install path.
- A failed checksum, SD read, image validation or OTA install should
  leave the running factory image alone and report an error.
- A mid-write power failure can leave the OTA slot unusable. The
  factory should remain bootable; USB recovery can still be required.
- If the SD application fails **before its startup return guard runs**,
  it might reboot back into the failing app. Physical USB reflash can
  be required. Do not describe this as guaranteed self-recovery.
- `factory` is logically reserved, **not write-protected by hardware**.
- There is no app authentication / signed image enforcement.
- Do not install arbitrary third-party `.bin` images: they do not
  include the common return-to-launcher contract and may use different
  partitions or hardware.
- Battery must be sufficiently charged or USB powered during flash
  installation. **Never deliberately power off during an OTA write.**
- Factory and SD app binaries **must use the same partition table**.

## Future steps

A recovery-tested universal firmware installer (or interoperability
with the mature open-source [M5Launcher](
https://github.com/bmorcelli/Launcher)) can be added later.
For now this design deliberately accepts only suite-managed firmware
generated by the matching build profiles.

See `docs/P4_ACCEPTANCE.md` for mandatory physical verification.

P5 controls and physical tests: docs/P5_COLLECTIONS.md and docs/P5_ACCEPTANCE.md.

P6 controls: docs/P6_DEV_SYSTEM.md; hardware gates: docs/P6_ACCEPTANCE.md.

P7 app instructions: docs/P7_FIELD_MEDIA_FILES_IR.md;
physical gates: docs/P7_ACCEPTANCE.md.

P8 network configuration and security: docs/P8_NETWORKING.md.
Original board validation checklist: docs/P8_ACCEPTANCE.md.
