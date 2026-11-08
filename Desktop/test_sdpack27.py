#!/usr/bin/env python3
"""P27 K132 SD installer regression: real ZIP bytes, dry run and safe copy."""
from __future__ import annotations
import hashlib
import importlib.util
from pathlib import Path
import tempfile
import unittest
import zipfile

SCRIPT = Path(__file__).resolve().parent / "sdpack27.py"
spec = importlib.util.spec_from_file_location("sdpack27", SCRIPT)
suite = importlib.util.module_from_spec(spec)
spec.loader.exec_module(suite)


def app_content(app: str):
    firmware = b"\xE9" + app.encode("ascii") + b"\x00" * 90
    hash_ = hashlib.sha256(firmware).hexdigest()
    manifest = (
        f"K132APP1\nNAME={app}\nID={app}\nIMAGE={app}.BIN\n"
        f"SIZE={len(firmware)}\nSHA256={hash_}\n"
    ).encode("ascii")
    return firmware, manifest


def make_archive(file: Path, *, modify=None):
    with zipfile.ZipFile(file, "w", zipfile.ZIP_DEFLATED) as z:
        z.writestr("README.txt", "Test K132 archive, no production firmware")
        for app in sorted(suite.EXPECTED_IDS):
            firmware, manifest = app_content(app)
            if modify is not None:
                firmware, manifest = modify(app, firmware, manifest)
            z.writestr(f"SUITE/APPS/{app}.BIN", firmware)
            z.writestr(f"SUITE/APPS/{app}.APP", manifest)


class P27Tests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.card = self.root / "CardSD"
        self.card.mkdir()
        self.zip = self.root / "firmware.zip"
        make_archive(self.zip)

    def tearDown(self):
        self.temp.cleanup()

    def test_validate_complete_archive_without_touching_card(self):
        valid, new, same = suite.build_plan(self.zip, self.card)
        self.assertEqual(len(valid), 30)
        self.assertEqual(len(new), 60)
        self.assertEqual(same, [])
        self.assertEqual(list(self.card.iterdir()), [])
        self.assertEqual(suite.main([
            "plan", "--archive", str(self.zip), "--target", str(self.card)
        ]), 0)
        self.assertFalse((self.card / "SUITE").exists())

    def test_user_confirmation_is_required_for_any_copy(self):
        self.assertEqual(suite.main([
            "install", "--archive", str(self.zip), "--target", str(self.card)
        ]), 2)
        self.assertFalse((self.card / "SUITE").exists())

    def test_copy_all_then_skip_identical_files_without_replacing(self):
        self.assertEqual(suite.install(self.zip, self.card), (60, 0))
        apps = self.card / "SUITE" / "APPS"
        self.assertEqual(len(list(apps.iterdir())), 60)
        for app in suite.EXPECTED_IDS:
            self.assertEqual((apps / f"{app}.BIN").read_bytes(), app_content(app)[0])
            self.assertEqual((apps / f"{app}.APP").read_bytes(), app_content(app)[1])
        self.assertEqual(suite.build_plan(self.zip, self.card)[1:], ([], sorted(
            (f"{app}.{ext}" for app in suite.EXPECTED_IDS for ext in ("BIN", "APP")),
            key=lambda x: (x.split(".")[0], x.endswith(".APP"))
        )))
        self.assertEqual(suite.install(self.zip, self.card), (0, 60))

    def test_refuse_existing_different_or_case_collision(self):
        apps = self.card / "SUITE" / "APPS"
        apps.mkdir(parents=True)
        other = apps / "PIANO.BIN"
        other.write_bytes(b"original owner file")
        with self.assertRaises(suite.PackError):
            suite.install(self.zip, self.card)
        self.assertEqual(other.read_bytes(), b"original owner file")
        self.assertEqual(len(list(apps.iterdir())), 1)
        other.rename(apps / "piano.bin")
        with self.assertRaises(suite.PackError):
            suite.install(self.zip, self.card)
        self.assertFalse((apps / "PIANO.APP").exists())

    def test_refuse_zip_payload_modified_without_manifest_hash(self):
        make_archive(self.zip, modify=lambda app, binary, manifest: (
            binary + (b"X" if app == "PIANO" else b""), manifest
        ))
        with self.assertRaises(suite.PackError):
            suite.install(self.zip, self.card)
        self.assertEqual(list(self.card.iterdir()), [])

    def test_refuse_manifest_wrong_name_and_unexpected_zip_paths(self):
        for invalid in ["K132APP2", "NAME=bad\nFAKE=x", "SHA256="]:
            def modify(app, firmware, manifest):
                if app == "PIANO":
                    return firmware, manifest.replace(
                        b"K132APP1" if invalid == "K132APP2" else
                        b"NAME=PIANO" if "FAKE" in invalid else b"SHA256=",
                        invalid.encode("ascii")
                    )
                return firmware, manifest
            make_archive(self.zip, modify=modify)
            with self.assertRaises(suite.PackError):
                suite.inspect_pack(self.zip)
        make_archive(self.zip)
        with zipfile.ZipFile(self.zip, "a") as z:
            z.writestr("../../OUTSIDE.TXT", "Never extract me")
        with self.assertRaises(suite.PackError):
            suite.install(self.zip, self.card)
        self.assertFalse((self.root / "OUTSIDE.TXT").exists())

    def test_refuse_archive_symlinks_and_tree_symlinks(self):
        with zipfile.ZipFile(self.zip, "a") as z:
            x = zipfile.ZipInfo("SUITE/APPS/FAKE.LNK")
            x.create_system = 3
            x.external_attr = (0o120777 << 16)
            z.writestr(x, "bad")
        with self.assertRaises(suite.PackError):
            suite.inspect_pack(self.zip)
        make_archive(self.zip)
        (self.card / "SUITE").symlink_to(self.root)
        with self.assertRaises(suite.PackError):
            suite.install(self.zip, self.card)

    def test_refuse_target_root_or_home(self):
        with self.assertRaises(suite.PackError):
            suite.target_tree(Path("/"))
        with self.assertRaises(suite.PackError):
            suite.target_tree(Path.home())

    def test_partial_binary_or_missing_manifest_blocks_unattended_recovery(self):
        apps = self.card / "SUITE" / "APPS"
        apps.mkdir(parents=True)
        (apps / "PIANO.BIN").write_bytes(b"\xe9partial")
        with self.assertRaises(suite.PackError):
            suite.install(self.zip, self.card)
        self.assertEqual((apps / "PIANO.BIN").read_bytes(), b"\xe9partial")
        self.assertFalse((apps / "PIANO.APP").exists())


if __name__ == "__main__":
    unittest.main(verbosity=2)
