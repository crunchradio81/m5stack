# P29 — Compile the original Cardputer suite locally on Ubuntu

Yes: the K132 firmware already uses the Arduino ESP32 framework via
PlatformIO Core. The project explicitly pins espressif32@6.7.0 and the
original stock Cardputer K132 ESP32-S3FN8 / 8 MB flash / no PSRAM.
Arduino IDE is optional, not required to compile this source.

## 1. Ubuntu prerequisites

    sudo apt update
    sudo apt install -y git python3 python3-venv python3-pip build-essential zip ca-certificates

No system-wide Python package installation is required. P29 creates
a local isolated PlatformIO virtual environment under ~/.venvs.

## 2. Clone the latest P29 branch (new directory)

    mkdir -p "$HOME/shared"
    git clone --branch feature/cardputer-k132-p29-ubuntu-local-build \
      https://github.com/crunchradio81/m5stack.git \
      "$HOME/shared/m5stack-p29"
    cd "$HOME/shared/m5stack-p29"

Do not overwrite an existing directory. If m5stack-p29 is already
a Git checkout, use git fetch/checkout instead of repeating git clone.

## 3. First build: factory and MIDI tools

    bash tools/ubuntu-build-suite.sh setup
    bash tools/ubuntu-build-suite.sh smoke

The setup step installs PlatformIO Core inside the isolated Python
virtual environment. The smoke step runs all source-owned portable
host regressions, compiles the factory firmware, then compiles
sd_miditools. It validates ESP32 magic 0xE9 and the 3 MiB slot size.

No Cardputer, USB cable, root privilege, erase, upload or physical
flashing is involved. Internet access is needed for first-time ESP32
toolchain and library downloads.

## 4. Full source build: 31 firmware images plus SD ZIP

    bash tools/ubuntu-build-suite.sh full

This builds:
- The permanent factory launcher: cardputer_original.
- All 30 standalone SD application firmware targets, using the names
  in tools/package-sd-apps.py as the single source of truth.
- Cardputer-K132-SD-App-Pack.zip containing all 30 binaries and
  30 ASCII manifests. The archive is validated with the P28
  SHA-256, image-header and manifest inspector.

Output location:

    ~/cardputer-build-reports/<UTC-date>-<commit>/

The directory contains summary.txt, one console log per build,
a separate factory-firmware.bin and the complete 30-app SD ZIP.

Helpful other options:

    bash tools/ubuntu-build-suite.sh list
    bash tools/ubuntu-build-suite.sh test
    bash tools/ubuntu-build-suite.sh pack

Pack uses existing binaries in .pio/build; run full beforehand to
ensure the 30 app binaries match your current Git branch.

## 5. Optional graphical Arduino IDE

The official Arduino IDE 2 can be installed from:
https://www.arduino.cc/en/software/

For an Ubuntu x86-64 desktop, the official Linux AppImage can be
downloaded, made executable and opened. Some distributions require
FUSE 2 compatibility for AppImages.

However Arduino IDE does not directly build all the separate
platformio.ini environments as configured. Prefer PlatformIO CLI,
or VS Code with its PlatformIO extension, if you want build buttons.

## Separate hardware phase — DO NOT FLASH YET

The initial real Cardputer installation would erase/replace
existing internal firmware and use the custom 8 MB partition
layout. A successful Ubuntu build does not authorize that.
Back up firmware/data and obtain explicit owner approval before
any future factory flash, erase or USB upload.

The separate GPL C64 emulator is built/distributed independently;
it is not part of the 30-app SD firmware ZIP.
The native SwiftUI macOS application requires macOS to build;
only its Cardputer-side firmware is compiled on Ubuntu.

PlatformIO and Arduino IDE installation references:
https://docs.platformio.org/en/latest/core/installation/
https://support.arduino.cc/hc/en-us/articles/360019833020-Download-and-install-Arduino-IDE
