# P24 — Mac + Ubuntu desktop companion for Cardputer LANFILES

**Purpose:** send or receive files with a single original stock M5Stack Cardputer
K132 using the unchanged P23 LANFILES firmware (256KiB per file).

The standalone Mac/Ubuntu host tool uses Python 3 **standard library only**.
There are no pip dependencies, no automatic background receiver, no cloud
upload and no changes to the original factory/OTA_0 partition layout.

**SECURITY:** HMAC-SHA256 with per-session 128-bit random challenge and 256-bit
shared secret authenticates each one-shot transfer. Actual file contents
travel over **unencrypted/plaintext TCP**. Only use on a trusted isolated
home LAN, never send passwords or sensitive files, and never forward port
38948 through your router.

## Private pairing key (both devices must share exactly the same key)

On Mac or Ubuntu, generate a new unpredictable owner-private key file:

    python3 Desktop/lanfile24.py generate-key --out "$HOME/LAN.CFG"

The generator creates owner-only 0600 permissions on Unix, does not reveal
the key, and refuses to overwrite an existing file. Do not commit the
result to GitHub. Copy this private LAN.CFG to the FAT32 microSD path
/SUITE/NET/LAN.CFG on the Cardputer. FAT32 lacks Unix file permissions;
keep that card private. Configure existing /SUITE/NET/NET.CFG with a valid
K132NET1 Wi-Fi profile and suitable private TCP_HOST.

## Desktop sends TO Cardputer

Launch LANFILES on Cardputer, press C to connect to Wi-Fi and L to
open one receive window (35 seconds); note the Cardputer's IPv4 address.
On Mac/Ubuntu, explicitly select a regular file from 1 to 262144 bytes
with a simple DOS-safe name, for example TEST.TXT:

    python3 Desktop/lanfile24.py send \
      --peer 192.168.1.50 \
      --file "$HOME/Documents/TEST.TXT" \
      --key-file "$HOME/LAN.CFG"

Replace the sample peer IP with the **actual Cardputer private IP**.
A successful receive appears on the Cardputer SD under /SUITE/INBOX.
The Cardputer verifies SHA256 and refuses existing destinations.
If a failed transfer left /SUITE/INBOX/UPLOAD.PART, inspect/recover it
manually before trying again; the receiver intentionally does not erase
the file on your behalf.

## Cardputer sends TO Mac/Ubuntu

Make an explicit desktop destination directory:

    mkdir -p "$HOME/Cardputer-Inbox"

On Mac/Ubuntu, start a **one-shot**, 35-second private-interface receiver:

    python3 Desktop/lanfile24.py receive \
      --bind 192.168.1.22 \
      --inbox "$HOME/Cardputer-Inbox" \
      --key-file "$HOME/LAN.CFG"

Replace the example bind address with the actual Mac/Ubuntu private IPv4
address. On Cardputer, place your file in /SUITE/OUTBOX, configure
TCP_HOST=192.168.1.22 in NET.CFG, connect Wi-Fi with C, choose the file
and press T.

The desktop receiver validates the HMAC before creating temporary data.
It never overwrites an existing file, including case-insensitive
collisions, and preserves interrupted UPLOAD.PART for manual recovery.
It uses POSIX atomic link/rename publishing after SHA256+size checks.
The receiver refuses public/Internet and wildcard bind addresses.
macOS may request a local-network Firewall exception for Python.

## Host verification

    python3 -m unittest discover -s Desktop -p 'test_lanfile24.py' -v

CI executes the same Python sender/receiver integration tests on macOS
and Ubuntu, including good transfers, wrong keys, header tampering,
wrong sizes, path traversal, collisions and interrupted streams.

**Hardware acceptance still pending:** original stock K132 not flashed
or tested. Two-way real Cardputer ↔ Mac/Ubuntu transfers, Wi-Fi timing,
SD FAT32 behavior, Mac firewall prompts, power loss and recovery all
need owner-device verification. Compiling/tests do not certify hardware.

The earlier P22 native SwiftUI Mac macro/keyboard/mouse app is separate.
P24 is a cross-platform CLI, not yet a native Mac file-transfer UI.
