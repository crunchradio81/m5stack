#!/usr/bin/env python3
"""P24 portable Mac/Linux LANFILES companion, standard library only.

Compatible with the *existing* P23 original K132 Cardputer LANFILES firmware.
Authenticated one-shot TCP, NOT encrypted. Use only on trusted owner LAN.
This is NOT a daemon, cloud upload, Internet service, or network scanner.
"""
from __future__ import annotations

import argparse
import hashlib
import hmac
import ipaddress
import os
from pathlib import Path
import secrets
import socket
import stat
import sys
import time

PORT = 38948
MAX_SIZE = 256 * 1024
MAX_HEADER = 250
CHALLENGE = b"K132CHAL1 "
MAGIC = "K132FILE1"
READY = b"K132READY1\n"
OK = b"K132OK1\n"
ERROR = b"K132ERR1\n"
KEY_PREFIX = b"K132LAN1\nKEY="
PARTIAL = "UPLOAD.PART"
READ_TIMEOUT = 9.0
CONNECT_TIMEOUT = 3.0
LISTEN_TIMEOUT = 35.0
RESERVED = {
    "CON", "PRN", "AUX", "NUL", "CLOCK$",
    *(f"COM{i}" for i in range(1, 10)),
    *(f"LPT{i}" for i in range(1, 10)),
}


class TransferError(Exception):
    """One transfer refused or failed safely."""


def private_ipv4(value: str, *, allow_loopback: bool = False) -> str:
    try:
        ip = ipaddress.ip_address(value)
    except ValueError as exc:
        raise TransferError("An explicit private numeric IPv4 address is required") from exc
    if not isinstance(ip, ipaddress.IPv4Address):
        raise TransferError("IPv4 required (Cardputer Wi-Fi is IPv4)")
    if allow_loopback and ip.is_loopback:
        return str(ip)
    numbers = tuple(int(x) for x in value.split("."))
    if len(numbers) != 4 or any(len(x) > 1 and x[0] == "0" for x in value.split(".")):
        raise TransferError("Use canonical dotted-decimal IPv4")
    a, b, _, d = numbers
    if d in (0, 255):
        raise TransferError("Host octet cannot be network or broadcast address")
    if not (a == 10 or (a == 172 and 16 <= b <= 31) or (a == 192 and b == 168)):
        raise TransferError("Only RFC1918 LAN peers are allowed")
    return str(ip)


def safe_name(filename: str) -> bool:
    """Exactly mirror P17 safefiles17::safeName (ASCII, reserved DOS names)."""
    if not (1 <= len(filename) < 36) or filename[0] == "." or filename[-1] == ".":
        return False
    if filename.count(".") > 1:
        return False
    if any(c not in "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_." for c in filename):
        return False
    parts = filename.split(".")
    stem = parts[0]
    if len(stem) == 0 or len(stem) > 24 or stem.upper() in RESERVED:
        return False
    if len(parts) == 2 and (len(parts[1]) == 0 or len(parts[1]) > 8):
        return False
    return True


def load_key(path: Path) -> bytes:
    raw = path.read_bytes()
    if len(raw) != len(KEY_PREFIX) + 64 + 1 or not raw.startswith(KEY_PREFIX) or not raw.endswith(b"\n"):
        raise TransferError("Expected K132LAN1 LF config with exactly one 64-hex-digit KEY")
    digits = raw[len(KEY_PREFIX):-1]
    if any(c not in b"0123456789abcdefABCDEF" for c in digits):
        raise TransferError("LAN.CFG contains non-hex key data")
    return bytes.fromhex(digits.decode("ascii"))


def generate_key(destination: Path) -> None:
    if destination.is_symlink():
        raise TransferError("Refusing to create through a symlink")
    secret = secrets.token_hex(32)
    flags = os.O_CREAT | os.O_EXCL | os.O_WRONLY
    if hasattr(os, "O_NOFOLLOW"):
        flags |= os.O_NOFOLLOW
    fd = os.open(destination, flags, 0o600)
    with os.fdopen(fd, "wb") as f:
        f.write(KEY_PREFIX + secret.encode("ascii") + b"\n")
        f.flush()
        os.fsync(f.fileno())
    print(f"Created private LAN.CFG at {destination} (key not displayed)")


def valid_challenge(line: bytes) -> str:
    # Cardputer challenge exactly 10+32 bytes plus LF, without CR.
    if not line.startswith(CHALLENGE) or len(line) != len(CHALLENGE) + 32 + 1 or line[-1:] != b"\n":
        raise TransferError("Invalid receiver handshake")
    nonce = line[len(CHALLENGE):-1]
    if any(x not in b"0123456789abcdefABCDEF" for x in nonce):
        raise TransferError("Invalid receiver nonce")
    return nonce.decode("ascii")


def make_header(filename: str, payload: bytes, nonce: str, secret: bytes) -> bytes:
    if not safe_name(filename) or not 1 <= len(payload) <= MAX_SIZE or len(secret) != 32:
        raise TransferError("Unsupported filename, payload size or key")
    if len(nonce) != 32 or any(c not in "0123456789abcdefABCDEF" for c in nonce):
        raise TransferError("Invalid receiver challenge")
    checksum = hashlib.sha256(payload).hexdigest()
    body = f"{MAGIC}|{filename}|{len(payload)}|{checksum}|{nonce}".encode("ascii")
    signature = hmac.new(secret, body, hashlib.sha256).hexdigest()
    header = f"{MAGIC}|{filename}|{len(payload)}|{checksum}|{signature}\n".encode("ascii")
    if not 145 <= len(header) < MAX_HEADER:
        raise TransferError("Header outside Cardputer protocol bounds")
    return header


def parse_header(line: bytes, nonce: str, secret: bytes) -> tuple[str, int, str]:
    if len(secret) != 32 or not (145 <= len(line) < MAX_HEADER) or not line.endswith(b"\n"):
        raise TransferError("Malformed LANFILES header")
    if any(c < 32 or c > 126 for c in line[:-1]):
        raise TransferError("Header contains non-ASCII control characters")
    try:
        magic, filename, size_text, digest, signature = line[:-1].decode("ascii").split("|")
    except ValueError as exc:
        raise TransferError("Header has incorrect field count") from exc
    if magic != MAGIC or not safe_name(filename):
        raise TransferError("Unsafe filename or unknown framing")
    if not size_text.isascii() or not size_text.isdecimal() or size_text.startswith("0"):
        raise TransferError("Invalid noncanonical file length")
    size = int(size_text)
    if not 1 <= size <= MAX_SIZE:
        raise TransferError("File exceeds 256KiB boundary")
    for x in (digest, signature):
        if len(x) != 64 or any(c not in "0123456789abcdefABCDEF" for c in x):
            raise TransferError("Invalid SHA256/HMAC hexadecimal field")
    body = f"{MAGIC}|{filename}|{size}|{digest}|{nonce}".encode("ascii")
    expected = hmac.new(secret, body, hashlib.sha256).hexdigest()
    if not hmac.compare_digest(bytes.fromhex(expected), bytes.fromhex(signature)):
        raise TransferError("HMAC authentication failed")
    return filename, size, digest


def read_line(peer: socket.socket, max_length: int) -> bytes:
    """No buffering past line end, so a following payload cannot be lost."""
    answer = bytearray()
    while len(answer) <= max_length:
        part = peer.recv(1)
        if not part:
            raise TransferError("Peer closed connection while awaiting response")
        answer.extend(part)
        if part == b"\n":
            if len(answer) > max_length:
                break
            return bytes(answer)
    raise TransferError("Overlong or unterminated protocol line")


def existing_name_casefold(folder: Path, requested: str) -> bool:
    return any(entry.name.upper() == requested.upper() for entry in folder.iterdir())


def incoming_folder(folder: Path) -> Path:
    if folder.is_symlink() or not folder.is_dir():
        raise TransferError("INBOX must already exist and must not be a symlink")
    return folder.resolve(strict=True)


def handle_receive(peer: socket.socket, inbox: Path, secret: bytes,
                   *, allow_loopback: bool = False) -> str:
    """Exactly one connection, one file, no overwrite or temporary deletion."""
    if len(secret) != 32:
        raise TransferError("Expected 32-byte key")
    peer.settimeout(READ_TIMEOUT)
    remote = peer.getpeername()[0]
    private_ipv4(remote, allow_loopback=allow_loopback)
    folder = incoming_folder(inbox)
    nonce = secrets.token_hex(16)
    peer.sendall(CHALLENGE + nonce.encode("ascii") + b"\n")
    pending_created = False
    try:
        line = read_line(peer, MAX_HEADER - 1)
        filename, size, expected = parse_header(line, nonce, secret)
        partial = folder / PARTIAL
        final = folder / filename
        if existing_name_casefold(folder, filename) or partial.exists() or partial.is_symlink():
            raise TransferError("Destination or UPLOAD.PART exists; manual recovery required")
        flags = os.O_CREAT | os.O_EXCL | os.O_WRONLY
        if hasattr(os, "O_NOFOLLOW"):
            flags |= os.O_NOFOLLOW
        fd = os.open(partial, flags, 0o600)
        pending_created = True
        with os.fdopen(fd, "wb") as file:
            peer.sendall(READY)
            hashed = hashlib.sha256()
            remaining = size
            while remaining:
                block = peer.recv(min(4096, remaining))
                if not block:
                    raise TransferError("Transfer interrupted; UPLOAD.PART preserved")
                file.write(block)
                hashed.update(block)
                remaining -= len(block)
            file.flush()
            os.fsync(file.fileno())
        if not hmac.compare_digest(hashed.hexdigest(), expected):
            raise TransferError("Payload SHA256 mismatch; UPLOAD.PART preserved")
        if partial.stat().st_size != size:
            raise TransferError("File length mismatch; UPLOAD.PART preserved")
        if existing_name_casefold(folder, filename):
            raise TransferError("Destination appeared before commit; PART preserved")
        # Atomic no-overwrite create on macOS/Linux; no os.replace() allowed.
        try:
            os.link(partial, final, follow_symlinks=False)
        except OSError as exc:
            raise TransferError(f"Cannot publish final file; PART preserved: {exc}") from exc
        try:
            partial.unlink()
        except OSError:
            # Data safely exists under final name, but leave a detectable PART.
            pass
        peer.sendall(OK)
        return str(final)
    except (TransferError, OSError):
        try:
            peer.sendall(ERROR)
        except OSError:
            pass
        # Any partially written transfer is deliberately kept for review.
        raise


def serve_once(bind: str, inbox: Path, secret: bytes,
               *, allow_loopback: bool = False, ready=None) -> str:
    bind = private_ipv4(bind, allow_loopback=allow_loopback)
    incoming_folder(inbox)
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as listener:
        listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        listener.bind((bind, PORT if not allow_loopback else 0))
        listener.listen(1)
        listener.settimeout(LISTEN_TIMEOUT)
        if ready is not None:
            ready(listener.getsockname()[1])
        print(f"One-shot LANFILES receiver on {bind}:{listener.getsockname()[1]} (35s)")
        try:
            connection, _ = listener.accept()
        except socket.timeout as exc:
            raise TransferError("Receiver listen window expired") from exc
        with connection:
            return handle_receive(connection, inbox, secret, allow_loopback=allow_loopback)


def send_file(peer_address: str, file_path: Path, secret: bytes,
              *, port: int = PORT, allow_loopback: bool = False) -> str:
    peer_address = private_ipv4(peer_address, allow_loopback=allow_loopback)
    if file_path.is_symlink() or not file_path.is_file() or not safe_name(file_path.name):
        raise TransferError("Send only a regular file with a Cardputer-safe basename")
    size = file_path.stat().st_size
    if not (1 <= size <= MAX_SIZE):
        raise TransferError("File must be between 1 and 262144 bytes")
    # Snapshot the bounded file to prevent size/hash changes after negotiation.
    content = file_path.read_bytes()
    if len(content) != size:
        raise TransferError("File changed while being loaded")
    with socket.create_connection((peer_address, port), CONNECT_TIMEOUT) as link:
        link.settimeout(READ_TIMEOUT)
        nonce = valid_challenge(read_line(link, 64))
        header = make_header(file_path.name, content, nonce, secret)
        link.sendall(header)
        if read_line(link, 32) != READY:
            raise TransferError("Receiver refused header (key, filename, collision or stale PART)")
        link.sendall(content)
        if read_line(link, 32) != OK:
            raise TransferError("Receiver could not verify or publish file")
    return f"Sent {file_path.name} ({size} bytes) to {peer_address}"


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(
        description="Cardputer LANFILES Mac/Ubuntu companion; unencrypted trusted LAN only")
    sub = parser.add_subparsers(dest="action", required=True)
    setup = sub.add_parser("generate-key", help="Generate owner-private LAN.CFG (never overwrite)")
    setup.add_argument("--out", type=Path, required=True)
    sender = sub.add_parser("send", help="Send one file to a Cardputer with LANFILES L enabled")
    sender.add_argument("--peer", required=True, help="Cardputer private IPv4")
    sender.add_argument("--file", type=Path, required=True)
    sender.add_argument("--key-file", type=Path, required=True)
    receiver = sub.add_parser("receive", help="Receive one Cardputer LANFILES T file")
    receiver.add_argument("--bind", required=True, help="This Mac/Ubuntu machine's private IPv4")
    receiver.add_argument("--inbox", type=Path, required=True, help="Existing destination directory")
    receiver.add_argument("--key-file", type=Path, required=True)
    args = parser.parse_args(argv)
    try:
        if args.action == "generate-key":
            generate_key(args.out)
        else:
            secret = load_key(args.key_file)
            if args.action == "send":
                print(send_file(args.peer, args.file, secret))
            else:
                print("Received at", serve_once(args.bind, args.inbox, secret))
        return 0
    except (TransferError, OSError) as exc:
        print(f"LANFILES: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
