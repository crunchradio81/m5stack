# P22 — Cardputer Remote native macOS backend

This work targets stock original M5Stack Cardputer K132 with no PSRAM,
no Grove modules and its existing 8 MB factory plus replaceable OTA_0 layout.
**Source and CI only**: nothing here establishes tested physical operation.

## Native macOS app
- Mac/ is a Swift Package for macOS 14+ with a SwiftUI control window,
  menu-bar enable/disable switch and twelve editable macro slots.
- Each Mac macro is a bounded Unicode Text action or a named Shortcut
  (command/shift/option/control plus one allowlisted key). No shell commands,
  scripts, arbitrary program execution or file-system access.
- Quartz CGEvents implement actual mouse movement, click, wheel scroll,
  layout-independent Unicode text (where the focused app honors it),
  and virtual-keyboard shortcut events.
- User explicitly authorizes Accessibility permission and enables LAN access.
  Mac app starts with remote controls OFF.
- Pairing secret is 256 random bits stored in macOS Keychain; hidden by
  default, reveal/copy only while provisioning the owner's own Cardputer.
- Authentication is per-connection nonce plus monotonically increasing
  sequence and HMAC SHA-256. Commands/inputs are bounded and allowlisted.
- This is *plaintext TCP*, even when messages are authenticated. Typing is
  visible to an observer on the network; it is not suitable for passwords,
  secrets or untrusted Wi-Fi. Do not forward TCP port 38947 to the Internet.

## Cardputer remote app
The new independent MACREMOTE firmware is one of the suite SD installable
apps, in the same 3 MiB OTA_0 slot. Factory firmware retains its next-boot
return guard and Esc-to-reboot behavior. No re-partitioning is added.

Create /SUITE/MAC/REMOTE.CFG on your private FAT32 SD card:

    K132MAC1
    HOST=192.168.1.22
    TOKEN=0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef

These HOST/TOKEN values are **examples only**. HOST must be the actual
Mac's IPv4 address on a private RFC1918 LAN, not a public address.
TOKEN must be copied from the running Mac app's pairing control.
Also configure the existing /SUITE/NET/NET.CFG with the local Wi-Fi profile.
Never commit real pairing tokens or passwords.

Cardputer key bindings:
- Fn+1: Setup mode; C connects, R reloads configs, D disconnects.
- Fn+2: Macro mode; 1 to 9, 0, minus and equals fire Mac macro slots 1-12.
- Fn+3: Mouse mode; I/J/K/L move, Space left-click, R right-click,
  U/N scroll. Motion is intentionally bounded to 12 pixels per key event.
- Fn+4: Type mode; printable ASCII is sent to the Mac as text.
  Full Unicode is available in the Mac's own configured Text macros
  (including non-English scripts); Mac receiving apps may differ.
- Esc returns to the factory launcher.

The current implementation requires both units on the same trusted LAN
and explicit Mac Accessibility authorization. No screen streaming,
remote shell, automatic pairing, background surveillance or password
transmission has been implemented.

## Checks and hardware acceptance
Mac:
  swift test --package-path Mac
  swift build --package-path Mac -c release
Cardputer:
  bash tools/run-host-tests.sh
  pio run -e sd_macremote
  pio run -e cardputer_original

Physical tests still required: actual pairing, invalid token refusal,
network disconnect/reconnect, macOS permission denial, 12 macros,
non-English Unicode, genuine mouse and scrolling, SD configuration guards,
memory use, Mac lock state, clean reboot to factory, USB recovery.

The older LAN-transfer/BLE-GATT P22 concept remains separate and is not
claimed as implemented by this macOS companion branch.
