#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
bash tools/run-host-tests.sh
if ! command -v pio >/dev/null 2>&1; then
  echo "PlatformIO Core is not installed; cannot compile firmware on this host." >&2
  exit 2
fi
pio run -e cardputer_original
