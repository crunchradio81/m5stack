#!/usr/bin/env python3
"""P27 Cardputer K132 SD app pack inspector / conservative copy helper.

Standard library only. NEVER flashes firmware. ZIP is not a signature.
Only install trusted packages produced by the project's GitHub Actions.
"""
from __future__ import annotations
import argparse
import hashlib
import os
from pathlib import Path
import re
import stat
import sys
import zipfile

EXPECTED_IDS = frozenset((
    "METRONOME", "PIANO", "CHANNELS", "BLE", "ARCADE", "OFFICE",
    "INSPECT", "MUSICLAB", "DEVTOOLS", "SYSTEM", "IRTOOLS", "WIRELESS",
    "AUDIOLAB", "FILETOOLS", "NETTOOLS", "MIDITOOLS", "ARCADE2",
    "POCKET11", "ARCADE3", "MUSIC2", "DEV2", "WEBSECURE", "THEMES",
    "NOTEBOOK", "SAFEFILES", "GAMEBOY", "FIELD2", "MACREMOTE",
    "LANFILES", "GATTVIEW",
))
MAX_BIN = 0x300000
MAX_MANIFEST = 512
MAX_TOTAL = len(EXPECTED_IDS) * (MAX_BIN + MAX_MANIFEST) + 4096
HASH_CHUNK = 65536


class PackError(Exception):
    pass


def _no_symlink_dir(path: Path, *, must_exist: bool) -> bool:
    if path.is_symlink():
        raise PackError(f"Symlink directory refused: {path}")
    if path.exists():
        if not path.is_dir():
            raise PackError(f"Not a directory: {path}")
        return True
    if must_exist:
        raise PackError(f"Directory missing: {path}")
    return False


def target_tree(root: Path, *, create: bool = False) -> Path:
    """Only create SUITE/APPS inside an owner-selected existing root."""
    _no_symlink_dir(root, must_exist=True)
    resolved = root.resolve(strict=True)
    if resolved == Path("/") or resolved == Path.home().resolve() or len(resolved.parts) < 3:
        raise PackError("Choose the actual SD volume root, not / or your home directory")
    # Never touch system root /Volumes itself. Genuine /Volumes/CARD is okay.
    if str(resolved) in ("/Volumes", "/System", "/Applications", "/Library",
                         "/usr", "/etc", "/private", "/mnt", "/media"):
        raise PackError("System or volume-container directory refused")
    suite = root / "SUITE"
    apps = suite / "APPS"
    for path in (suite, apps):
        if not _no_symlink_dir(path, must_exist=False) and create:
            # Do not overwrite files and never accept a symlink parent.
            path.mkdir(exist_ok=False)
    return apps


def parse_manifest(data: bytes, expected_id: str) -> tuple[str, int, str]:
    if not (50 <= len(data) <= MAX_MANIFEST):
        raise PackError(f"{expected_id}: manifest length invalid")
    try:
        text = data.decode("ascii")
    except UnicodeDecodeError as exc:
        raise PackError(f"{expected_id}: non-ASCII manifest") from exc
    if "\r" in text or "\x00" in text or not text.endswith("\n"):
        raise PackError(f"{expected_id}: manifest must use LF lines")
    lines = text.splitlines()
    if len(lines) != 6 or lines[0] != "K132APP1":
        raise PackError(f"{expected_id}: unsupported manifest format")
    keys = ("NAME", "ID", "IMAGE", "SIZE", "SHA256")
    values = {}
    for key, line in zip(keys, lines[1:]):
        if not line.startswith(key + "="):
            raise PackError(f"{expected_id}: malformed or reordered {key}")
        values[key] = line[len(key)+1:]
    name = values["NAME"]
    if not (1 <= len(name) <= 29 and name.isascii() and name.isprintable()):
        raise PackError(f"{expected_id}: invalid display name")
    if values["ID"] != expected_id or values["IMAGE"] != f"{expected_id}.BIN":
        raise PackError(f"{expected_id}: ID and binary name mismatch")
    size_string = values["SIZE"]
    if not re.fullmatch(r"[1-9][0-9]*", size_string):
        raise PackError(f"{expected_id}: invalid canonical size")
    size = int(size_string)
    if not (64 <= size <= MAX_BIN):
        raise PackError(f"{expected_id}: binary size outside 3MiB OTA partition")
    digest = values["SHA256"]
    if not re.fullmatch(r"[0-9a-f]{64}", digest):
        raise PackError(f"{expected_id}: invalid SHA256")
    return name, size, digest


def _sha_file(path: Path) -> tuple[str, int, int]:
    sha = hashlib.sha256()
    n = 0
    first = -1
    with path.open("rb") as f:
        while chunk := f.read(HASH_CHUNK):
            if first == -1:
                first = chunk[0]
            n += len(chunk)
            sha.update(chunk)
    return sha.hexdigest(), n, first


def _scan_info(zf: zipfile.ZipFile, info: zipfile.ZipInfo,
               maximum: int) -> tuple[str, int, int]:
    if not (0 < info.file_size <= maximum):
        raise PackError(f"Invalid ZIP uncompressed size: {info.filename}")
    if info.flag_bits & 1:
        raise PackError("Encrypted ZIP entries refused")
    if info.compress_type not in (zipfile.ZIP_STORED, zipfile.ZIP_DEFLATED):
        raise PackError("Unsupported ZIP compression")
    mode = (info.external_attr >> 16) & 0xFFFF
    if stat.S_IFMT(mode) == stat.S_IFLNK or info.is_dir():
        raise PackError("ZIP links/directories refused")
    sha = hashlib.sha256()
    seen = 0
    first = -1
    try:
        with zf.open(info, "r") as f:
            while part := f.read(HASH_CHUNK):
                if first == -1:
                    first = part[0]
                seen += len(part)
                if seen > maximum:
                    raise PackError("ZIP file exceeds size cap while streaming")
                sha.update(part)
    except (OSError, RuntimeError, zipfile.BadZipFile, EOFError) as exc:
        raise PackError(f"ZIP stream integrity error: {info.filename}") from exc
    if seen != info.file_size:
        raise PackError(f"ZIP length mismatch: {info.filename}")
    return sha.hexdigest(), seen, first


def inspect_pack(archive: Path) -> dict[str, tuple[int, str]]:
    """Verify *every* file before returning a plan; no SD writes."""
    if archive.is_symlink() or not archive.is_file() or archive.stat().st_size > 200 * 1024 * 1024:
        raise PackError("Select an existing, bounded local ZIP (not a symlink)")
    results: dict[str, tuple[int, str]] = {}
    try:
        with zipfile.ZipFile(archive) as zf:
            records = zf.infolist()
            if len(records) != 1 + len(EXPECTED_IDS) * 2:
                raise PackError("Expected a complete 30-app K132 archive (60 files + README)")
            names = [x.filename for x in records]
            if len(set(n.lower() for n in names)) != len(names):
                raise PackError("Duplicate or case-colliding ZIP paths refused")
            allowed = {"README.txt"}
            for app in EXPECTED_IDS:
                allowed.add(f"SUITE/APPS/{app}.BIN")
                allowed.add(f"SUITE/APPS/{app}.APP")
            if set(names) != allowed:
                raise PackError("Unexpected ZIP path, missing K132 image or nested file")
            if sum(x.file_size for x in records) > MAX_TOTAL:
                raise PackError("ZIP uncompressed total exceeds bounded K132 pack")
            by_name = {x.filename: x for x in records}
            _scan_info(zf, by_name["README.txt"], 4096)
            for app in sorted(EXPECTED_IDS):
                manifest_info = by_name[f"SUITE/APPS/{app}.APP"]
                _scan_info(zf, manifest_info, MAX_MANIFEST)
                with zf.open(manifest_info) as stream:
                    _, size, declared_sha = parse_manifest(stream.read(MAX_MANIFEST + 1), app)
                digest, actual, first = _scan_info(zf, by_name[f"SUITE/APPS/{app}.BIN"], MAX_BIN)
                if digest != declared_sha or actual != size or first != 0xE9:
                    raise PackError(f"{app}: binary or manifest SHA256/size/header mismatch")
                results[app] = (size, digest)
    except (zipfile.BadZipFile, OSError, ValueError, EOFError) as exc:
        raise PackError(f"Unreadable / corrupt ZIP archive: {exc}") from exc
    return results


def build_plan(archive: Path, root: Path) -> tuple[dict[str, tuple[int, str]], list[str], list[str]]:
    validated = inspect_pack(archive)
    apps = target_tree(root)
    files = []
    for app in sorted(EXPECTED_IDS):
        files.extend([f"{app}.BIN", f"{app}.APP"])
    if apps.exists():
        names = {p.name.lower(): p for p in apps.iterdir()}
        # Reject any symlink (even if the name isn't in this archive).
        if any(p.is_symlink() for p in names.values()):
            raise PackError("SUITE/APPS contains a symlink; refusing SD writes")
    else:
        names = {}
    new, same, conflicts = [], [], []
    with zipfile.ZipFile(archive) as zf:
        for filename in files:
            old = names.get(filename.lower())
            if old is None:
                new.append(filename)
                continue
            if old.name != filename or not old.is_file():
                conflicts.append(filename)
                continue
            expected = zf.getinfo(f"SUITE/APPS/{filename}")
            # Existing files count as safe only when byte-for-byte identical.
            sha, n, _ = _sha_file(old)
            if n == expected.file_size:
                digest, _, _ = _scan_info(zf, expected, MAX_BIN if filename.endswith(".BIN") else MAX_MANIFEST)
                if sha == digest:
                    same.append(filename)
                    continue
            conflicts.append(filename)
    if conflicts:
        raise PackError("Existing card files differ; none overwritten: " + ", ".join(conflicts[:12]))
    return validated, new, same


def install(archive: Path, root: Path) -> tuple[int, int]:
    valid, new, same = build_plan(archive, root)
    # Re-check tree after validation; no write until full package verified.
    apps = target_tree(root, create=True)
    copied = 0
    # Always publish binaries before their manifest. On interruption a
    # partial .BIN without its matching .APP cannot launch in factory.
    ordered = sorted(new, key=lambda x: (x.endswith(".APP"), x))
    try:
        with zipfile.ZipFile(archive) as zf:
            for filename in ordered:
                target = apps / filename
                flags = os.O_WRONLY | os.O_CREAT | os.O_EXCL
                if hasattr(os, "O_NOFOLLOW"):
                    flags |= os.O_NOFOLLOW
                fd = os.open(target, flags, 0o644)
                sha = hashlib.sha256()
                size = 0
                try:
                    with os.fdopen(fd, "wb") as f, zf.open(f"SUITE/APPS/{filename}") as stream:
                        while block := stream.read(HASH_CHUNK):
                            f.write(block)
                            sha.update(block)
                            size += len(block)
                        f.flush()
                        os.fsync(f.fileno())
                    expected = zf.getinfo(f"SUITE/APPS/{filename}")
                    if size != expected.file_size:
                        raise PackError("Copied SD file has incorrect size: " + filename)
                    on_disk, n, _ = _sha_file(target)
                    if on_disk != sha.hexdigest() or n != size:
                        raise PackError("SD readback failed: " + filename)
                    copied += 1
                except Exception:
                    # Never erase partial owner data silently. It must be
                    # inspected/recovered manually before retrying.
                    raise
    except (OSError, zipfile.BadZipFile) as exc:
        raise PackError(f"SD copy stopped. Partial files may remain; inspect card: {exc}") from exc
    return copied, len(same)


def main(argv=None) -> int:
    p = argparse.ArgumentParser(description="Owner-selected 30-app Cardputer K132 SD installer (never flashes)")
    p.add_argument("mode", choices=("plan", "install"))
    p.add_argument("--archive", type=Path, required=True)
    p.add_argument("--target", type=Path, required=True, help="Existing SD volume root, NOT SUITE/APPS")
    p.add_argument("--confirm-copy", action="store_true", help="Explicitly permit create-only SD file copying")
    args = p.parse_args(argv)
    try:
        validated, new, same = build_plan(args.archive, args.target)
        print(f"PLAN:APPS={len(validated)}|NEW={len(new)}|IDENTICAL={len(same)}", flush=True)
        if args.mode == "plan":
            print("READY: validated package and selected target; no changes made", flush=True)
        elif not args.confirm_copy:
            raise PackError("Copy requires --confirm-copy; no SD files were written")
        else:
            count, unchanged = install(args.archive, args.target)
            print(f"INSTALLED:NEW={count}|IDENTICAL={unchanged}", flush=True)
        return 0
    except (PackError, OSError) as exc:
        print(f"SDPACK27 ERROR: {exc}", file=sys.stderr, flush=True)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
