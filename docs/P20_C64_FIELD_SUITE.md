# P20 — Existing Cardputer C64 port + Field Lab II

**All firmware is draft. No real original stock K132 has passed hardware acceptance.**

## C64: reuse upstream instead of rewriting it

The preexisting third-party GPL C64 emulator is built in an *isolated* image
using pinned upstream source. The full GPL corresponding source and generated
adapter changes are included in the C64 package. This is NOT merged into
the independently authored suite's normal 27-image pack.

Upstream: https://github.com/iele/M5Cardputer-C64-Emulator
Pinned commit: 3cd226097a5b61727dd5e5b0fd07318f4d33fd58
Open ROMs: https://github.com/MEGA65/open-roms
Pinned commit: ad178dbe4d48cd6a317737a8e0e7e662f7e33d32

Adapter script: tools/build-c64-upstream.py
- clones the pinned upstream source
- replaces embedded historical Commodore BASIC/KERNAL/CHARGEN headers with a
  matching MEGA65 Generic BASIC/KERNAL and Open CHARGEN set
- patches SD_MMC references to original K132 SPI microSD pinout
- adds an early OTA next-boot factory-return guard, refusing wrong partitions
- uses the matching 8MB stock flash and 3MiB application partition
- compiles and independently packages the GPL/Open ROM source with SHA256
- does not claim real-device emulation compatibility.

Original upstream limitations: no SID, no full disk/tape support, incomplete
VIC and unofficial CPU instructions, some games fail.

An owner can instead build with their own legally acquired ROMs via --rom-dir
without packaging or distributing the resulting privately held ROM data.
The public packager refuses explicit private-ROM builds. No historical
Commodore ROM headers are committed to the suite.

## Field Lab II — three additional catalog applications

A new FIELD2 SD firmware image includes:

1. Wi-Fi History Explorer: read-only view of existing P7 survey log located at
   /SUITE/FIELD/WIFI.CSV, capped to last 128KiB and 768 lines, with up to 16
   BSSID histories (sample count, minimum, maximum and mean RSSI). No GPS.
2. World Clock / NTP: user-initiated Wi-Fi from existing private
   /SUITE/NET/NET.CFG, manual NTP request, seven fixed timezone offsets.
   Fixed offsets do NOT automatically adjust for daylight saving time.
3. LAN Throughput Test: user-initiated Wi-Fi, user-started TCP receiver on
   port 38946, explicit generated 64KiB pattern to the owner-configured
   private RFC1918 IPv4 TCP_HOST in NET.CFG. No real user files transferred.
   Unencrypted; use only on trusted LAN. Rate measures bytes and elapsed time.

A normal FIELD2.BIN and .APP is packaged alongside the existing 26 native
suite apps. C64.BIN and .APP are distributed as a **separate** GPL package,
to keep upstream obligations and the additional acceptance steps explicit.

## Device and source acceptance still required

- Host CSV parser, invalid-row rejection, private IP guard and rate math.
- GitHub compile original K132 factory plus FIELD2 and upstream C64.
- Audit generated C64 source and Open ROM licensing/provenance.
- Verify on actual original K132: return to factory after adapted C64 starts.
- Verify C64 BASIC banner, typing, PRG/BAS from SPI SD, memory, display, speed.
- Verify two real K132 devices can perform LAN throughput and measured
  sender/receiver rates; malicious/off-LAN peers rejected; timeout/reboot.
- Confirm world clock correctness (DST is manual); no unrequested Wi-Fi.
- USB recovery, microSD remove, bad .APP, and previous 26 apps still function.

Do not merge, flash or claim hardware certification from CI alone.
