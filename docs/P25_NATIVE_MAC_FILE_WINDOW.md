# P25 — Native macOS file-transfer window for Cardputer

P25 extends the **existing** Cardputer Remote Mac SwiftUI application.
Its new **LAN File Transfer** tab has actual user-driven file and inbox
choosers, send/receive, progress, results, an explicit cancel button, and
a private LAN.CFG chooser/generator. Nothing runs as a background file
server until the user clicks Receive.

## What is native, and what is reused

- The macOS window, configuration flow, file pickers and progress are native
  SwiftUI/AppKit (macOS 14+, Apple Silicon or Intel).
- The transfer engine intentionally reuses exactly the tested Python 3
  standard-library code from P24, shipped as the read-only application
  resource Contents/Resources/lanfile24.py. The Mac app launches it through
  Process without a shell or user-controlled command execution.
- A working **Python 3** is required on Mac at /opt/homebrew/bin/python3,
  /usr/local/bin/python3 or /usr/bin/python3. App checks availability and
  shows a warning; no silent installation or unknown downloads.
- P24 now emits PROGRESS:0–100 messages based on actual payload bytes sent
  or received; the UI shows a progress bar and percentage. It is not an
  invented ETA or percentage during the initial connection handshake.
- The Mac menu bar macro/keyboard/mouse app remains independent. Its
  separate 256-bit Mac macro token is **not** reused as the LAN file key.

## Pairing (one stock original Cardputer K132)

From the native Mac app:
1. Open LAN File Transfer. Select Generate LAN.CFG and choose a private
   local path for a new shared key. A file already at that location is
   protected from overwrite. Or choose an existing owner-private LAN.CFG.
2. Copy the generated key *file* to your Cardputer microSD under
   /SUITE/NET/LAN.CFG. Keep it private; don't commit it to GitHub.
3. Configure your existing /SUITE/NET/NET.CFG owner Wi-Fi profile.
4. Mac → Cardputer: on Cardputer launch LANFILES, C connects, L receives.
   In Mac native tab choose **Send to Cardputer**, type the Cardputer's
   actual private IPv4, select one small file, then click Send.
5. Cardputer → Mac: on Mac choose **Receive from Cardputer**, enter your
   Mac's own private IPv4, select an existing inbox folder, click Receive
   (35-second one-shot listener). On Cardputer place file in /SUITE/OUTBOX,
   configure NET.CFG TCP_HOST to the same Mac address, C connect, T send.
6. The Mac never overwrites existing filenames and leaves UPLOAD.PART for
   owner recovery if reception was interrupted. Cancel stops the subprocess
   and does **not** clean partial evidence.

Do not enter 0.0.0.0, loopback or public IPs for real transfers. The
P24 CLI refuses them. The TCP listener is never automatically opened.
No data goes to a cloud service.

## Security and restrictions

The desktop and Cardputer sign each header with HMAC-SHA256 using a fresh
128-bit challenge and verify payload SHA256, with a strict 256KiB bound.
**This does NOT encrypt traffic.** Only transfer non-sensitive material
on a trusted private LAN. Do not forward TCP port 38948, use untrusted
Wi-Fi, or transfer passwords/private credentials. No executable automation
is provided; file paths come from user-selected AppKit file panels.

The application package is ad-hoc signed and **not Apple notarized**.
Real original K132 hardware testing, actual LAN transfer, macOS firewall
behavior and physical recovery still remain uncertified.

## Source and CI

Run from checkout:

    python3 -m unittest discover -s Desktop -p 'test_lanfile24.py' -v
    swift test --package-path Mac
    swift build --package-path Mac -c release
    bash Mac/package-app.command

P25 CI uses macos-15 for Swift compilation, protocol regression and
packaging validation, and both macos-15 and Ubuntu for actual host loopback
socket transfers, malformed commands, wrong keys, interrupted streams and
monotone byte-progress tracking. No changes to Cardputer firmware or its
30 native SD app images; the separate GPL C64 remains unchanged.
