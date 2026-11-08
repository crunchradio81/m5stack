# P23 — Authenticated LAN File Transfer and Read-only BLE GATT

Target: **original 2023 stock M5Stack Cardputer K132**, 8 MB flash,
no PSRAM, no Grove attachments. The two applications use the unchanged
factory + 3 MiB OTA_0 slot and have **not** yet been tested on hardware.

## LANFILES — owner-to-owner microSD file transfer

LANFILES is a new standalone SD app, packaged as LANFILES.BIN and
LANFILES.APP. No Internet exposure, no cloud service or auto-start.

**Source folder:** /SUITE/OUTBOX, at most 24 eligible file entries.
**Destination:** /SUITE/INBOX (receiver only).
**Size:** nonempty files no larger than 256 KiB, filenames restricted by
the existing P17 SafeFiles rules (simple basename only, no traversal).
Any preexisting destination file is protected from overwrite.
A fixed receiver temp file /SUITE/INBOX/UPLOAD.PART is **never automatically
deleted or overwritten**; recover/discard it manually after a failure.
A successful and verified receive renames that file to its requested name.

### Configuration — user-private microSD on BOTH Cardputers

Use existing /SUITE/NET/NET.CFG with K132NET1, owner SSID/password and
TCP_HOST=192.168.1.22 (receiver's current **private IPv4** address).
Create /SUITE/NET/LAN.CFG with:

    K132LAN1
    KEY=0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef

**The key above is a deliberately public EXAMPLE and is not secure.
Generate a unique 32-byte random key and put the SAME key in LAN.CFG on
both owned devices. Do not upload it to GitHub or screenshots.** The
device does not print your key.

### Controls and protocol

- C: explicitly associate with the owner Wi-Fi from NET.CFG.
- L: start a **single** 35-second receive window on TCP port 38948.
- T: send the selected existing OUTBOX file to private TCP_HOST only.
- Up/Down: select a local OUTBOX file; R: reload; X: stop listener.
- Esc: return to factory (and stop connections).
- Receiver sends a 128-bit random per-connection challenge.
- Sender signs (name,size,SHA256,challenge) with HMAC-SHA256 using LAN.CFG.
- Receiver validates the MAC before opening UPLOAD.PART.
- Sender transmits precisely the declared bytes; receiver hashes them and
  verifies the SHA256 before renaming, with no overwrite.
- Acknowledge success only after the file-length/hash checks and rename.
- Receive activity times out after 9 seconds idle; sends have bounded limits.

**Security:** authenticated but **unencrypted** TCP. A LAN observer can read
every transferred byte. Use exclusively on a trusted isolated home LAN,
never expose port 38948 to the Internet, never transfer passwords,
sensitive files, or unlicensed copyrighted content. Existing/partial
files are kept for manual recovery. Authentication does not provide
confidentiality or a defense against denial of service.

Initial P23 sender/receiver are Cardputer to Cardputer. The existing P22
Mac companion is unrelated and does not receive this file protocol.
A desktop sender can be implemented in a separate stage, if needed.

## GATTVIEW — manually initiated BLE service metadata browsing

The second standalone app, GATTVIEW.BIN/APP:
- Press S for one 3-second **passive** BLE advertiser scan.
- Choose one of the first 16 advertisers, then **Enter** to explicitly
  connect for service discovery. Does not auto-connect on scan.
- Inspect up to 30 service/characteristic UUID records and basic readable,
  writable, notify and indicate properties. Arrow keys browse results.
- Press D or B to disconnect; S disconnects and rescans.
- No characteristic value reads, GATT writes, notifications/subscriptions,
  pairing automation, firmware updates, or control operations.
- It is a view of announced GATT metadata only; connecting to another
  device may still be visible and should be done only with permission.
- Some devices require pairing/security or use random/private addresses,
  and some services may exceed the bounded list. Those are not guaranteed.

## Test and release gates

Automated: host tests for strict file headers, HMAC input framing,
challenge and key parsing, path and size rejection, GATT UUID/properties
and bounded records; build factory and all native SD images; checksum
package. A compiled image does **not** prove working BLE or LAN I/O.

Physical: both original K132 devices, real SD read/write and existing
filename collision, interrupted transfer .PART manual recovery,
byte-for-byte verify against reference, intentionally corrupted header
and wrong pairing key, timeout and network loss, no unprompted Wi-Fi.
For BLE: scan, connected metadata, uncooperative devices, manual
disconnect and memory/heap checks. Verify original factory return and
USB recovery before marking either app ready.

Do not merge or physically flash without owner-approved recovery and
actual original Cardputer tests.
