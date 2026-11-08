#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
dir="$(mktemp -d)"
trap 'rm -rf "$dir"' EXIT
for name in calculator document paths wifi_csv midi_smf midi_synth pocket_tools; do
  c++ -std=c++17 -O2 -Wall -Wextra -Werror -pedantic "tests/test_$name.cpp" -o "$dir/test_$name"
  "$dir/test_$name"
done
