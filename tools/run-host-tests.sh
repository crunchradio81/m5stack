#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
dir="$(mktemp -d)"
trap 'rm -rf "$dir"' EXIT
for name in calculator document paths wifi_csv midi_smf midi_synth pocket_tools sd_manifest field_tools arcade_core office_core inspector_core musiclab_core dev_core system_core p7_core net8_core midi9_core arcade10 pocket11_core launcher2_core p12_arcade music13_core dev14_core web15_core theme16_core notebook17_core safefiles17_core gameboy18_core field20_core playlist21_core rss21_core ssdp21_core macremote22_core lan23_core gatt23_core; do
  c++ -std=c++17 -O2 -Wall -Wextra -Werror -pedantic "tests/test_$name.cpp" -o "$dir/test_$name"
  "$dir/test_$name"
done
cc -std=gnu99 -O2 -Wall -Wextra -Werror tests/test_gameboy19_emulator.c -o "$dir/test_gameboy19_emulator"
"$dir/test_gameboy19_emulator"
python3 tests/test_sd_package.py
