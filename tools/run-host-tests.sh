#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
dir="$(mktemp -d)"
trap 'rm -rf "$dir"' EXIT
for name in calculator document paths wifi_csv midi_smf midi_synth pocket_tools sd_manifest field_tools arcade_core office_core inspector_core musiclab_core dev_core system_core p7_core net8_core midi9_core arcade10 pocket11_core launcher2_core p12_arcade music13_core dev14_core web15_core theme16_core notebook17_core safefiles17_core; do
  c++ -std=c++17 -O2 -Wall -Wextra -Werror -pedantic "tests/test_$name.cpp" -o "$dir/test_$name"
  "$dir/test_$name"
done
python3 tests/test_sd_package.py
