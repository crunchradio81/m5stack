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
print("SD App Packager: PASS (four manifests, hashes and invalid image rejection)")
