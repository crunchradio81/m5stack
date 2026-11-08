#!/usr/bin/env python3
"""Package K132 suite-managed SD firmware binaries with bounded manifests.

Run after the standalone PlatformIO environments compile:
  python3 tools/package-sd-apps.py --build-root .pio/build --out Cardputer-SD-Apps.zip

Place the ZIP's SUITE/APPS directory at the root of a FAT32 microSD card.
The factory launcher firmware is NOT included; it must first be flashed over
USB once with its matching custom partition table. Manifest SHA256 protects
against accidental corruption, not a maliciously replaced image.
"""
import argparse
import hashlib
from pathlib import Path
import zipfile

APPS = (
    ("sd_metronome", "METRONOME", "Metronome"),
    ("sd_piano", "PIANO", "Music Keyboard"),
    ("sd_channels", "CHANNELS", "Wi-Fi Channel Analyzer"),
    ("sd_ble", "BLE", "BLE Explorer"),
    ("sd_arcade", "ARCADE", "Retro Arcade Collection"),
    ("sd_office", "OFFICE", "Pocket Office Collection"),
    ("sd_inspect", "INSPECT", "SD Inspector Collection"),
    ("sd_musiclab", "MUSICLAB", "Music Practice Collection"),
    ("sd_devtools", "DEVTOOLS", "Developer Tools"),
    ("sd_system", "SYSTEM", "System Toolbox"),
    ("sd_irtools", "IRTOOLS", "Infrared Remote Tools"),
    ("sd_wireless", "WIRELESS", "Wireless Field Lab"),
    ("sd_audiolab", "AUDIOLAB", "Audio Recorder and Lab"),
    ("sd_filetools", "FILETOOLS", "Safe SD File Tools"),
    ("sd_nettools", "NETTOOLS", "Network and Internet"),
    ("sd_miditools", "MIDITOOLS", "MIDI Deep Tools"),
    ("sd_arcade2", "ARCADE2", "Expanded Arcade"),
    ("sd_pocket11", "POCKET11", "Pocket Productivity"),
    ("sd_arcade3", "ARCADE3", "Arcade Classics III"),
    ("sd_music2", "MUSIC2", "Music Production Studio"),
)
MAX_APP_BYTES = 0x300000


def package(root: Path, destination: Path, artifact_layout: bool = False):
    output = []
    for environment, app_id, label in APPS:
        base = (root / f"cardputer-k132-{environment}-unverified"
                if artifact_layout else root / environment)
        data = (base / "firmware.bin").read_bytes()
        if len(data) < 64 or len(data) > MAX_APP_BYTES or data[0] != 0xE9:
            raise ValueError(f"{environment} is not a bounded ESP32 app image")
        digest = hashlib.sha256(data).hexdigest()
        manifest = (
            "K132APP1\n"
            f"NAME={label}\n"
            f"ID={app_id}\n"
            f"IMAGE={app_id}.BIN\n"
            f"SIZE={len(data)}\n"
            f"SHA256={digest}\n"
        ).encode("ascii")
        output.append((f"SUITE/APPS/{app_id}.BIN", data))
        output.append((f"SUITE/APPS/{app_id}.APP", manifest))
    readme = (
        "Original M5Stack Cardputer K132 SD App Pack\n"
        "------------------------------------------\n"
        "Copy the SUITE directory to the root of a FAT32 microSD card.\n"
        "Do not flash these images directly over the factory launcher.\n"
        "Install the matching factory launcher FIRST over USB, following\n"
        "docs/SD_LAUNCHER.md. Menu: SD App Library > Enter > Y.\n"
        "After app boot, the next restart returns to the factory launcher.\n"
        "Physical boot/SD recovery on real K132 is NOT YET CERTIFIED.\n"
    ).encode("ascii")
    with zipfile.ZipFile(destination, "w", zipfile.ZIP_DEFLATED) as archive:
        archive.writestr("README.txt", readme)
        for name, payload in output:
            archive.writestr(name, payload)
    return output


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--build-root", default=".pio/build")
    p.add_argument("--out", default="Cardputer-SD-Apps.zip")
    p.add_argument("--artifact-layout", action="store_true")
    args = p.parse_args()
    paths = package(Path(args.build_root), Path(args.out), args.artifact_layout)
    print(f"Created {args.out} with {len(paths)//2} K132 SD apps")


if __name__ == "__main__":
    main()
