#!/bin/bash
set -euo pipefail
# Package a native SwiftPM executable as an installable macOS .app.
# Ad-hoc signed only: NOT Apple notarized or Developer ID distributed.
ROOT="$(cd "$(dirname "$0")" && pwd)"
SRC="$ROOT/.build/release/CardputerRemoteMac"
DEST="$ROOT/dist/CardputerRemoteMac.app"
ZIP="$ROOT/dist/CardputerRemoteMac-macOS-unsigned.zip"
STAGE="$ROOT/dist/P26-Install-Bundle"
if [[ ! -x "$SRC" ]]; then
  echo "Build first: swift build --package-path Mac -c release" >&2
  exit 2
fi
mkdir -p "$DEST/Contents/MacOS" "$DEST/Contents/Resources"
rm -rf "$STAGE"
cp "$SRC" "$DEST/Contents/MacOS/CardputerRemoteMac"
# Keep the P24 authenticated Python helper alongside the native UI.
# The UI finds this read-only resource via Bundle.main; no shell downloads.
cp "$ROOT/../Desktop/lanfile24.py" "$DEST/Contents/Resources/lanfile24.py"
test -s "$DEST/Contents/Resources/lanfile24.py"
cat > "$DEST/Contents/Info.plist" <<'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN"
  "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleIdentifier</key><string>com.crunchradio81.CardputerRemoteMac</string>
  <key>CFBundleExecutable</key><string>CardputerRemoteMac</string>
  <key>CFBundleName</key><string>Cardputer Remote Mac</string>
  <key>CFBundleDisplayName</key><string>Cardputer Remote</string>
  <key>CFBundlePackageType</key><string>APPL</string>
  <key>CFBundleShortVersionString</key><string>0.26.0</string>
  <key>CFBundleVersion</key><string>26</string>
  <key>LSMinimumSystemVersion</key><string>14.0</string>
  <key>NSHumanReadableCopyright</key>
  <string>Open-source software; personal-use remote control.</string>
  <key>NSLocalNetworkUsageDescription</key>
  <string>Accept authenticated Cardputer macro and pointer commands and explicitly initiated file transfers on your own LAN.</string>
  <key>NSHighResolutionCapable</key><true/>
</dict>
</plist>
PLIST
plutil -lint "$DEST/Contents/Info.plist"
# Ad-hoc local development signature; may need owner-approved Gatekeeper steps.
codesign --force --deep --sign - --timestamp=none "$DEST"
codesign --verify --deep --strict "$DEST"
mkdir -p "$STAGE"
/usr/bin/ditto "$DEST" "$STAGE/CardputerRemoteMac.app"
cp "$ROOT/Install-CardputerRemoteMac.command" "$STAGE/Install-CardputerRemoteMac.command"
chmod 755 "$STAGE/Install-CardputerRemoteMac.command"
/usr/bin/codesign --verify --deep --strict "$STAGE/CardputerRemoteMac.app"
/usr/bin/ditto -c -k --sequesterRsrc "$STAGE" "$ZIP"
echo "Unsigned-for-distribution development bundle: $ZIP"
