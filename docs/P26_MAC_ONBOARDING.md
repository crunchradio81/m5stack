# P26 — Native Mac companion pairing, safer install, and recovery gates

**Target:** original stock M5Stack Cardputer K132 and native macOS 14+
Cardputer Remote app. Source/CI only: no real hardware or Mac owner
Accessibility/firewall acceptance has been performed.

P26 changes the Mac app only, not its 30 independently buildable Cardputer
SD firmware apps or the separate GPL C64 port.

## Guided keyboard/mouse pairing

The existing **Macros & Mouse** tab now has a guided remote pairing section:

1. Find your Mac's actual private LAN IPv4 under System Settings → Network
   (Wi-Fi or Ethernet connection details). Do not use a VPN exit IP,
   hostname, public IP, loopback or a 0/255 host octet.
2. Enter that private IPv4 in the new Mac pairing field.
3. Insert the owner's private Cardputer FAT32 microSD. Create the folder
   /SUITE/MAC if needed, then choose that exact folder when pressing
   **Export REMOTE.CFG** in the Mac app.
4. The app writes **exactly** the original Cardputer K132MAC1 format:

       K132MAC1
       HOST=192.168.1.22
       TOKEN=<64-character private token>

   The address above is an example only. The actual token is taken from
   the current Mac's Keychain pairing secret. It is not printed into logs
   or exposed by the export status. No password is written or inferred.
5. Independently prepare /SUITE/NET/NET.CFG with your own Wi-Fi
   credentials, as in P22. That file is not auto-generated or modified.
6. On the Mac, grant Accessibility permission to the actual installed
   native app and explicitly enable its macro/keyboard listener.
   On Cardputer launch MACREMOTE, C connect, then Fn+2 macros, Fn+3 mouse,
   or Fn+4 type. Esc returns to the factory launcher.

Files are written only after a deliberate folder selection, with O_EXCL
(no overwrite), O_NOFOLLOW (no symlink target), and 0600 permissions on
APFS. A file already called REMOTE.CFG is **never silently replaced**.
FAT32 does not preserve Unix access controls; treat the microSD as secret.
The remote listener remains **OFF** until explicitly enabled, and now
requires Accessibility permission before opening.

The two pairing credentials are **distinct**: this REMOTE.CFG uses the
Keychain macro/mouse secret; LAN File Transfer uses /SUITE/NET/LAN.CFG
and must continue to be configured separately. Do not substitute one
for the other.

## Downloadable optional installer

The macOS CI ZIP now contains:
- CardputerRemoteMac.app (ad-hoc signed native SwiftUI app, including the
  P24 Python standard-library LANFILES helper)
- Install-CardputerRemoteMac.command, a user-run optional shell installer

Extract both files, and run Install-CardputerRemoteMac.command on your Mac.
It validates the app signature, copies the app to ~/Applications via an
intermediate path, verifies the copied code signature, and refuses to
overwrite an existing ~/Applications/CardputerRemoteMac.app. It does not
require sudo, launch the app, download dependencies, modify Finder
settings, remove quarantine flags or bypass macOS Gatekeeper. The Mac
owner must approve a development build according to macOS security
controls; no notarization is claimed.

If you already installed a previous version, back it up or deliberately
remove it yourself before using the installer. P26 intentionally does
not perform unattended application upgrades or permission migrations.

## Transfer protection improvements

The native Mac LANFILES tab now refuses invalid or public/wildcard
private IPv4 input *before* starting the Python process, in addition
to the strict P24 protocol validation. Both P25 send/receive, file
selection, measured progress, explicit cancel and preserved UPLOAD.PART
recovery remain unchanged.

Communication for macro/mouse (TCP 38947) and LANFILES (TCP 38948)
uses per-message/transfer HMAC authentication but **is not encrypted**.
Use only on a trusted LAN; do not send passwords, credentials or private
files. Do not forward those ports to the Internet.

## Verification gates

Automated macOS CI:
- swift test --package-path Mac: 256-bit Keychain-token format, strict
  original Cardputer private IPv4, owner-only exclusive write,
  symlink refusal and no-overwrite tests
- release build, ad-hoc signing, ZIP creation and extraction
- installer simulation with isolated HOME, signature verification,
  repeat-install refusal (no changes to the real runner's home)
- Mac+Ubuntu P24 authenticated loopback file-transfer regressions

Hardware still required:
- actual Cardputer MACREMOTE reads new generated REMOTE.CFG
- first-run macOS Accessibility permission, real keyboard/mouse events
  and Unicode macro typing
- LANFILES two-way communication and actual SD file integrity
- app upgrade/rollback and Gatekeeper behavior on owner's Mac
- original K132 factory-return guard, SD hot swap and USB recovery.

Don't merge this draft until those checks are performed by the owner.
