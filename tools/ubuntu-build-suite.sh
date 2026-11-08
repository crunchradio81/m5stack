#!/usr/bin/env bash
# Original M5Stack Cardputer K132 - local Ubuntu build, NEVER a flash/upload.
set -Eeuo pipefail
umask 077
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
VENV="$HOME/.venvs/cardputer-pio"
PIO="$VENV/bin/pio"
REPORTS="$HOME/cardputer-build-reports"
MODE="help"
if (( $# > 0 )); then MODE="$1"; fi

die() { echo "ERROR: $*" >&2; exit 2; }
need() { command -v "$1" >/dev/null || die "Missing '$1'. Install: sudo apt install git python3 python3-venv build-essential zip"; }
help_text() {
cat <<'HELP'
P29: original stock M5Stack Cardputer (K132), Ubuntu source build.
  bash tools/ubuntu-build-suite.sh setup   # install isolated PlatformIO venv
  bash tools/ubuntu-build-suite.sh test    # portable regressions only
  bash tools/ubuntu-build-suite.sh smoke   # regressions + factory + MIDI tools
  bash tools/ubuntu-build-suite.sh full    # all 31 firmware builds + 30-app SD ZIP
  bash tools/ubuntu-build-suite.sh pack    # package 30 previously built apps
  bash tools/ubuntu-build-suite.sh list    # list local firmware environments
No Arduino IDE, sudo firmware commands, USB, erase, upload, or flashing.
Reports and binaries: ~/cardputer-build-reports/<UTC-stamp>-<commit>/
HELP
}
setup() {
  need python3; need git; need c++; need cc; need zip
  if [[ ! -x "$VENV/bin/python" ]]; then
    python3 -m venv "$VENV" || die "Install Python venv: sudo apt install python3-venv"
  fi
  "$VENV/bin/python" -m pip install --disable-pip-version-check 'platformio>=6.1,<7'
  "$PIO" --version
}
require_pio() {
  [[ -x "$PIO" ]] || die "PlatformIO missing. Run: bash tools/ubuntu-build-suite.sh setup"
  "$PIO" --version
}
apps() {
  python3 - "$ROOT" <<'PY'
from pathlib import Path
import runpy, sys
list_ = runpy.run_path(str(Path(sys.argv[1])/"tools/package-sd-apps.py"))["APPS"]
assert len(list_) == len({item[0] for item in list_}) == 30
for env, name, label in list_:
    assert env.startswith("sd_") and name and label
    print(env)
PY
}
make_report() {
  need git
  local sha now
  sha="$(git -C "$ROOT" rev-parse --short=12 HEAD)" || die "Not a Git repository"
  now="$(date -u +%Y%m%dT%H%M%SZ)"
  RUN="$REPORTS/$now-$sha"
  mkdir -p "$RUN"
  {
    echo "SourceCommit=$(git -C "$ROOT" rev-parse HEAD)"
    echo "SourceBranch=$(git -C "$ROOT" branch --show-current)"
    echo "SourceDirty=$(test -z "$(git -C "$ROOT" status --porcelain)" && echo no || echo yes)"
    echo "Mode=$MODE"
    echo "Host=$(uname -srm)"
    echo "ArduinoIDERequired=no"
    echo "FlashOrUSBActivity=none"
  } > "$RUN/summary.txt"
  echo "Reports: $RUN"
}
host_tests() {
  echo "HOST TESTS"
  (cd "$ROOT" && bash tools/run-host-tests.sh) 2>&1 | tee "$RUN/host-tests.log"
  echo "PASS host-tests" >> "$RUN/summary.txt"
}
build() {
  local target="$1" binary
  echo "BUILD $target"
  "$PIO" run -d "$ROOT" -e "$target" 2>&1 | tee "$RUN/$target.log"
  binary="$ROOT/.pio/build/$target/firmware.bin"
  python3 - "$target" "$binary" <<'PY' | tee "$RUN/$target.sha256.txt"
import hashlib
import sys
from pathlib import Path
name, file = sys.argv[1], Path(sys.argv[2])
data = file.read_bytes()
assert 64 <= len(data) <= 0x300000, (name, len(data))
assert data[0] == 0xe9, f"{name}: invalid ESP32 image header"
print(f"{name}: bytes={len(data)} sha256={hashlib.sha256(data).hexdigest()}")
PY
  echo "PASS $target" >> "$RUN/summary.txt"
}
copy_factory() {
  cp "$ROOT/.pio/build/cardputer_original/firmware.bin" "$RUN/factory-firmware.bin"
  sha256sum "$RUN/factory-firmware.bin" >> "$RUN/summary.txt"
}
package_apps() {
  echo "PACKAGING 30 SD APPS"
  python3 "$ROOT/tools/package-sd-apps.py" \
    --build-root "$ROOT/.pio/build" \
    --out "$RUN/Cardputer-K132-SD-App-Pack.zip" 2>&1 | tee "$RUN/sd-package.log"
  python3 - "$ROOT" "$RUN/Cardputer-K132-SD-App-Pack.zip" <<'PY' | tee "$RUN/sd-verify.log"
from pathlib import Path
import runpy, sys
root, archive = (Path(x) for x in sys.argv[1:])
verify = runpy.run_path(str(root/"Desktop/sdpack27.py"))["inspect_pack"]
checks = verify(archive)
assert len(checks) == 30
print("PASS 30 firmware images, 30 ASCII manifests, SHA256, size, ESP32 header")
PY
  sha256sum "$RUN/Cardputer-K132-SD-App-Pack.zip" >> "$RUN/summary.txt"
  echo "PASS SD package" >> "$RUN/summary.txt"
}
fail() {
  code=$?
  if [[ -n "$RUN" ]]; then
    echo "FAILED exit=$code (see logs in $RUN)" | tee -a "$RUN/summary.txt" >&2
  fi
  exit "$code"
}
RUN=""
trap fail ERR
case "$MODE" in
  help|-h|--help) help_text ;;
  setup) setup ;;
  list) echo cardputer_original; apps ;;
  test)
    need python3; need c++; need cc
    make_report; host_tests ;;
  smoke)
    need python3; need c++; need cc; require_pio
    make_report; host_tests
    build cardputer_original; build sd_miditools; copy_factory
    echo "SMOKE PASS. Zero flashing." | tee -a "$RUN/summary.txt" ;;
  full)
    need python3; need c++; need cc; require_pio
    make_report; host_tests; build cardputer_original
    while IFS= read -r app; do build "$app"; done < <(apps)
    copy_factory; package_apps
    echo "FULL PASS: 31 images + 30-app ZIP, no flashing." | tee -a "$RUN/summary.txt" ;;
  pack)
    need python3; make_report; package_apps ;;
  *) help_text; die "Unknown command: $MODE" ;;
esac
