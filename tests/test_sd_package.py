#!/usr/bin/env python3
"""Host-side SD packager regression; uses generated dummy images, not firmware."""
import hashlib
import importlib.util
from pathlib import Path
import tempfile
import zipfile

spec = importlib.util.spec_from_file_location(
    "sd_packer", Path(__file__).resolve().parents[1] / "tools" / "package-sd-apps.py")
packer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(packer)

with tempfile.TemporaryDirectory() as directory:
    tmp = Path(directory)
    build = tmp / "build"
    for n, (env, app_id, label) in enumerate(packer.APPS):
        dest = build / env
        dest.mkdir(parents=True)
        (dest / "firmware.bin").write_bytes(b"\xe9" + bytes([n])*127)
    archive = tmp / "apps.zip"
    # Regression: launcher cannot parse NAME of 30+ bytes.
    assert len(packer.APPS) <= 64
    assert all(0 < len(label) < 30 for _, _, label in packer.APPS)
    original = packer.APPS
    env, app_id, _ = original[0]
    try:
        packer.APPS = ((env, app_id, "OVERLONG_" + "X"*25),) + original[1:]
        try:
            packer.package(build, tmp / "invalid-name.zip")
            raise AssertionError("Overlong launcher NAME was accepted")
        except ValueError:
            pass
    finally:
        packer.APPS = original
    # Reject conflicting IDs and oversized catalogs before touching the
    # output archive (no accidental partial SD package).
    try:
        packer.APPS = original + (original[0],)
        try:
            packer.package(build, tmp / "duplicate.zip")
            raise AssertionError("Duplicate app ID accepted")
        except ValueError:
            assert not (tmp / "duplicate.zip").exists()
        packer.APPS = original * 3
        try:
            packer.package(build, tmp / "over-capacity.zip")
            raise AssertionError("More than 64 firmware entries accepted")
        except ValueError:
            assert not (tmp / "over-capacity.zip").exists()
    finally:
        packer.APPS = original
    files = packer.package(build, archive)
    assert len(files) == 2 * len(packer.APPS)
    with zipfile.ZipFile(archive) as z:
        names = z.namelist()
        assert len(names) == 1 + 2 * len(packer.APPS) and "README.txt" in names
        for env, app_id, label in packer.APPS:
            blob = z.read(f"SUITE/APPS/{app_id}.BIN")
            meta = z.read(f"SUITE/APPS/{app_id}.APP").decode("ascii")
            assert meta.startswith("K132APP1\n")
            assert f"NAME={label}\n" in meta
            assert f"ID={app_id}\n" in meta
            assert f"SIZE={len(blob)}\n" in meta
            assert f"SHA256={hashlib.sha256(blob).hexdigest()}\n" in meta
    (build / packer.APPS[0][0] / "firmware.bin").write_bytes(b"\x00"*128)
    try:
        packer.package(build, tmp / "should_not_exist.zip")
        raise AssertionError("Bad ESP image was incorrectly accepted")
    except ValueError:
        pass
print("SD App Packager: PASS (all manifests, hashes and invalid image rejection)")
