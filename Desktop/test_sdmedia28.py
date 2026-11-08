#!/usr/bin/env python3
"""P28: mounted-FAT32 policy and readback audit; no real device writes."""
import importlib.util
import hashlib
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import zipfile

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("sdpack28", HERE / "sdpack27.py")
sd = importlib.util.module_from_spec(spec)
spec.loader.exec_module(sd)


def create_fixture(archive):
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as z:
        z.writestr("README.txt", b"source-generated QA image, not genuine ESP32")
        for app in sorted(sd.EXPECTED_IDS):
            data = b"\xe9" + app.encode("ascii") + b"\0" * 90
            digest = hashlib.sha256(data).hexdigest()
            manifest = (f"K132APP1\nNAME={app}\nID={app}\nIMAGE={app}.BIN\n"
                        f"SIZE={len(data)}\nSHA256={digest}\n").encode("ascii")
            z.writestr(f"SUITE/APPS/{app}.BIN", data)
            z.writestr(f"SUITE/APPS/{app}.APP", manifest)


class SDMedia28Tests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        self.card = self.root / "OwnerCard"
        self.card.mkdir()
        self.archive = self.root / "K132.zip"
        create_fixture(self.archive)

    def tearDown(self):
        self.tmp.cleanup()

    def test_macos_fat32_only_and_external_only(self):
        source = {"MountPoint": "/Volumes/K132", "FilesystemType": "msdos",
                  "FileSystemPersonality": "MS-DOS FAT32",
                  "Internal": False, "Writable": True}
        sd.validate_macos_volume(source, Path("/Volumes/K132"))
        for changed in (
            {"MountPoint": "/Volumes/OTHER"},
            {"FilesystemType": "apfs", "FileSystemPersonality": "APFS"},
            {"Internal": True},
            {"Writable": False},
        ):
            altered = dict(source)
            altered.update(changed)
            with self.assertRaises(sd.PackError, msg=str(changed)):
                sd.validate_macos_volume(altered, Path("/Volumes/K132"))
        with self.assertRaises(sd.PackError):
            sd.validate_macos_volume({"MountPoint": "/Volumes/K132"}, Path("/Volumes/K132"))

    def test_linux_requires_exact_vfat_block_mount(self):
        good = {"filesystems": [{"target": "/media/user/K132",
              "source": "/dev/sdb1", "fstype": "vfat", "options": "rw,relatime"}]}
        sd.validate_linux_volume(good, Path("/media/user/K132"))
        for change in ({"target": "/media/user"},
                       {"source": "tmpfs"},
                       {"fstype": "ext4"},
                       {"options": "ro,relatime"}):
            bad = {"filesystems": [dict(good["filesystems"][0], **change)]}
            with self.assertRaises(sd.PackError):
                sd.validate_linux_volume(bad, Path("/media/user/K132"))

    def test_real_directory_is_not_a_mount_and_cannot_be_written(self):
        with self.assertRaises(sd.PackError):
            sd.install(self.archive, self.card)
        self.assertEqual(list(self.card.iterdir()), [])
        self.assertEqual(sd.main(["plan", "--archive", str(self.archive),
                                  "--target", str(self.card)]), 2)
        self.assertEqual(list(self.card.iterdir()), [])

    def test_simulated_linux_mount_is_authorized_only_with_fat32(self):
        import subprocess
        import json
        data = {"filesystems":[{"target":str(self.card.resolve()),
                                "source":"/dev/sdb1","fstype":"vfat","options":"rw"}]}
        with patch.object(sd.os.path, "ismount", return_value=True), \
             patch.object(sd.platform, "system", return_value="Linux"), \
             patch.object(sd.subprocess, "run",
                          return_value=subprocess.CompletedProcess(
                              args=[], returncode=0,
                              stdout=json.dumps(data).encode(), stderr=b"")):
            self.assertEqual(sd.require_card_media(self.card),
                             self.card.stat().st_dev)
        data["filesystems"][0]["fstype"] = "ext4"
        with patch.object(sd.os.path, "ismount", return_value=True), \
             patch.object(sd.platform, "system", return_value="Linux"), \
             patch.object(sd.subprocess, "run",
                          return_value=subprocess.CompletedProcess(
                              args=[], returncode=0,
                              stdout=json.dumps(data).encode(), stderr=b"")):
            with self.assertRaises(sd.PackError):
                sd.require_card_media(self.card)

    def test_full_read_only_audit_missing_changed_and_complete(self):
        good, missing, changed, details = sd.audit_card(self.archive, self.card)
        self.assertEqual((good, missing, changed), (0,60,0))
        self.assertIn("SUITE/APPS directory is missing", details)
        with patch.object(sd, "require_card_media", return_value=123), \
             patch.object(sd, "check_card_still_mounted"):
            self.assertEqual(sd.install(self.archive, self.card), (60,0))
        self.assertEqual(sd.audit_card(self.archive, self.card)[:3], (60,0,0))
        apps = self.card / "SUITE" / "APPS"
        (apps / "PIANO.APP").unlink()
        (apps / "GAMEBOY.BIN").write_bytes(b"\xe9malformed")
        good, missing, changed, details = sd.audit_card(self.archive, self.card)
        self.assertEqual((good, missing, changed), (58,1,1))
        self.assertIn("MISSING:PIANO.APP", details)
        self.assertIn("CHANGED:GAMEBOY.BIN", details)
        self.assertFalse((apps / "PIANO.APP").exists())
        self.assertEqual((apps / "GAMEBOY.BIN").read_bytes(), b"\xe9malformed")

    def test_reject_case_collisions_even_when_one_file_matches(self):
        apps = self.card / "SUITE" / "APPS"
        apps.mkdir(parents=True)
        with zipfile.ZipFile(self.archive) as z:
            data = z.read("SUITE/APPS/PIANO.BIN")
        (apps / "PIANO.BIN").write_bytes(data)
        (apps / "piano.bin").write_bytes(b"other")
        # Default macOS APFS is case-insensitive and cannot represent both
        # distinct names. Ubuntu case-sensitive filesystems exercise the
        # actual duplicate-names guard; on macOS test its case-only mismatch.
        if len(list(apps.iterdir())) < 2:
            with self.assertRaises(sd.PackError):
                sd.build_plan(self.archive, self.card)
            return
        with self.assertRaises(sd.PackError):
            sd.build_plan(self.archive, self.card)
        with self.assertRaises(sd.PackError):
            sd.audit_card(self.archive, self.card)

    def test_card_removal_between_files_blocks_new_writes(self):
        times = [0]
        def removed(root, identity):
            times[0] += 1
            if times[0] == 3:
                raise sd.PackError("Card disconnected, stop")
        with patch.object(sd, "require_card_media", return_value=991), \
             patch.object(sd, "check_card_still_mounted", side_effect=removed):
            with self.assertRaises(sd.PackError):
                sd.install(self.archive, self.card)
        apps = self.card / "SUITE" / "APPS"
        self.assertTrue(apps.is_dir())
        self.assertEqual(len(list(apps.iterdir())), 1)
        self.assertTrue(next(apps.iterdir()).name.endswith(".BIN"))

    def test_no_mount_security_bypass_in_command_interface(self):
        with self.assertRaises(sd.PackError):
            sd.require_card_media(self.card)
        # --confirm-copy cannot bypass the real mount guard.
        self.assertEqual(sd.main(["install", "--archive", str(self.archive),
                                   "--target", str(self.card),
                                   "--confirm-copy"]), 2)
        self.assertEqual(list(self.card.iterdir()), [])


if __name__ == "__main__":
    unittest.main(verbosity=2)
