#!/bin/bash
# P26 Mac-only owner-invoked installer for the already-built native SwiftUI app.
# Never downloads software, runs sudo, overwrites an existing app, or bypasses
# Gatekeeper/quarantine; only the Mac owner decides whether to open it.
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
SOURCE="$HERE/CardputerRemoteMac.app"
APPS="$HOME/Applications"
DEST="$APPS/CardputerRemoteMac.app"
TMP="$APPS/.CardputerRemoteMac-p26-install-$$.app"

if [[ "$(uname -s)" != "Darwin" ]]; then
  echo "This installer runs only on macOS." >&2
  exit 2
fi
if [[ ! -d "$SOURCE" || ! -x "$SOURCE/Contents/MacOS/CardputerRemoteMac" ||
      ! -s "$SOURCE/Contents/Resources/lanfile24.py" ]]; then
  echo "App bundle is missing or incomplete. Re-extract the P26 ZIP." >&2
  exit 2
fi
if [[ -e "$DEST" || -L "$DEST" ]]; then
  echo "Existing $DEST found. Nothing was replaced." >&2
  echo "Manually back up or remove it before installing a new version." >&2
  exit 2
fi
if ! /usr/bin/codesign --verify --deep --strict "$SOURCE"; then
  echo "Source bundle code signature failed; refusing installation." >&2
  exit 2
fi

mkdir -p "$APPS"
trap 'rm -rf "$TMP"' EXIT
/usr/bin/ditto "$SOURCE" "$TMP"
/usr/bin/codesign --verify --deep --strict "$TMP"
if [[ -e "$DEST" || -L "$DEST" ]]; then
  echo "Another app appeared at the destination; nothing overwritten." >&2
  exit 2
fi
/bin/mv -n "$TMP" "$DEST"
if [[ ! -d "$DEST" ]]; then
  echo "Installation did not finish; source bundle remains unchanged." >&2
  exit 2
fi
echo "Installed local app: $DEST"
echo "This is an ad-hoc signed development build, NOT notarized."
echo "No quarantine or macOS security settings were modified."
echo "Open from Finder when you choose; approve macOS permissions yourself."
echo "The remote listener remains OFF until you enable it in the app."
