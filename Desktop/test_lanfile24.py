#!/usr/bin/env python3
"""P24 host-side contract and loopback integration tests, no hardware needed."""
import hashlib
import hmac
import importlib.util
from pathlib import Path
import queue
import socket
import tempfile
import threading
import unittest

SCRIPT = Path(__file__).resolve().parent / "lanfile24.py"
spec = importlib.util.spec_from_file_location("lanfile24", SCRIPT)
lan = importlib.util.module_from_spec(spec)
spec.loader.exec_module(lan)

SECRET = bytes(range(32))


class DesktopProtocolTests(unittest.TestCase):
    def test_p23_config_round_trip_and_protect_existing_key(self):
        with tempfile.TemporaryDirectory() as root:
            path = Path(root) / "LAN.CFG"
            lan.generate_key(path)
            self.assertEqual(len(lan.load_key(path)), 32)
            self.assertEqual(len(path.read_bytes()), 13 + 64 + 1)
            self.assertEqual(path.read_bytes()[:13], b"K132LAN1\nKEY=")
            with self.assertRaises(FileExistsError):
                lan.generate_key(path)
            path.write_bytes(b"K132LAN1\nKEY=abc\n")
            with self.assertRaises(lan.TransferError):
                lan.load_key(path)

    def test_matches_p17_safe_name_limits(self):
        for name in ["TEST.TXT", "PHOTO1.BIN", "MIDI.MID", "FILENAME", "lower_case.abc"]:
            self.assertTrue(lan.safe_name(name), name)
        for name in ["", ".", ".HIDDEN", "A..TXT", "BAD NAME.TXT", "../file", "foo/bar",
                     "foo\\bar", "LPT1", "com4.txt", "A"*25, "x.abcdefghx",
                     "A"*35 + ".TXT", "DATA.txt/", "a.txt\n", "A:B"]:
            self.assertFalse(lan.safe_name(name), name)

    def test_private_ip_only(self):
        for ip in ["10.1.2.3", "172.16.0.1", "172.31.254.254", "192.168.1.23"]:
            self.assertEqual(lan.private_ipv4(ip), ip)
        for ip in ["8.8.8.8", "127.0.0.1", "192.168.1.0", "192.168.1.255",
                   "172.32.0.1", "192.168.1.02", "::1", "localhost"]:
            with self.assertRaises(lan.TransferError, msg=ip):
                lan.private_ipv4(ip)
        self.assertEqual(lan.private_ipv4("127.0.0.1", allow_loopback=True), "127.0.0.1")

    def test_hmac_body_interoperable_with_p23(self):
        nonce = "0123456789abcdef0123456789abcdef"
        payload = b"Test\x00Data" * 4
        header = lan.make_header("TEST.BIN", payload, nonce, SECRET)
        fields = header.decode().strip().split("|")
        self.assertEqual(fields[0], "K132FILE1")
        self.assertEqual(fields[1], "TEST.BIN")
        self.assertEqual(fields[2], str(len(payload)))
        self.assertEqual(fields[3], hashlib.sha256(payload).hexdigest())
        signed = "|".join(fields[:4] + [nonce]).encode("ascii")
        self.assertEqual(fields[4], hmac.new(SECRET, signed, hashlib.sha256).hexdigest())
        self.assertEqual(lan.parse_header(header, nonce, SECRET), (
            "TEST.BIN", len(payload), hashlib.sha256(payload).hexdigest()))
        for tampered in (
            header.replace(b"TEST.BIN", b"BEST.BIN"),
            header.replace(b"TEST.BIN", b"../S.BIN"),
            header.replace(b"TEST.BIN", b"TEST.BIN\r"),
            header.replace(f"|{len(payload)}|".encode(), f"|{len(payload)+1}|".encode()),
        ):
            with self.assertRaises(lan.TransferError):
                lan.parse_header(tampered, nonce, SECRET)
        with self.assertRaises(lan.TransferError):
            lan.parse_header(header, "f"*32, SECRET)
        with self.assertRaises(lan.TransferError):
            lan.parse_header(header, nonce, bytes([255]*32))

    def _pair(self, folder: Path, *, server_key=SECRET):
        reply = queue.Queue()
        def receiver():
            try:
                reply.put(("ok", lan.serve_once("127.0.0.1", folder, server_key,
                    allow_loopback=True, ready=lambda port: ports.put(port))))
            except Exception as exc:
                reply.put(("error", exc))
        ports = queue.Queue()
        thread = threading.Thread(target=receiver, daemon=True)
        thread.start()
        port = ports.get(timeout=5)
        return port, reply, thread

    def test_end_to_end_bytes_and_no_overwrite(self):
        with tempfile.TemporaryDirectory() as root:
            directory = Path(root)
            inbox = directory / "INBOX"
            inbox.mkdir()
            payload = b"\x00\xff\xfe" + bytes(range(256))*1023
            self.assertEqual(len(payload), 261891)
            outbound = directory / "REAL.BIN"
            outbound.write_bytes(payload)
            port, result, thread = self._pair(inbox)
            sent = lan.send_file("127.0.0.1", outbound, SECRET,
                                 port=port, allow_loopback=True)
            self.assertIn("REAL.BIN", sent)
            status, target = result.get(timeout=7)
            thread.join(timeout=2)
            self.assertEqual(status, "ok")
            self.assertEqual(Path(target).read_bytes(), payload)
            self.assertFalse((inbox / lan.PARTIAL).exists())
            # Receive the same filename again: never overwrite existing content.
            port, result, thread = self._pair(inbox)
            with self.assertRaises(lan.TransferError):
                lan.send_file("127.0.0.1", outbound, SECRET,
                              port=port, allow_loopback=True)
            status, exc = result.get(timeout=7)
            self.assertEqual(status, "error")
            self.assertIsInstance(exc, lan.TransferError)
            self.assertEqual((inbox / "REAL.BIN").read_bytes(), payload)

    def test_wrong_key_fails_before_file_creation(self):
        with tempfile.TemporaryDirectory() as root:
            folder = Path(root)
            outbound = folder / "RIGHT.TXT"
            outbound.write_bytes(b"valid data")
            inbox = folder / "INBOX"
            inbox.mkdir()
            port, result, _ = self._pair(inbox)
            with self.assertRaises(lan.TransferError):
                lan.send_file("127.0.0.1", outbound, b"x"*32,
                              port=port, allow_loopback=True)
            self.assertEqual(result.get(timeout=7)[0], "error")
            self.assertEqual(list(inbox.iterdir()), [])

    def test_interrupted_payload_preserves_part_for_owner(self):
        with tempfile.TemporaryDirectory() as root:
            inbox = Path(root) / "INBOX"
            inbox.mkdir()
            ports = queue.Queue()
            result = queue.Queue()
            def receiver():
                try:
                    lan.serve_once("127.0.0.1", inbox, SECRET,
                        allow_loopback=True, ready=ports.put)
                    result.put("unexpected success")
                except Exception as exc:
                    result.put(exc)
            thread = threading.Thread(target=receiver, daemon=True)
            thread.start()
            with socket.create_connection(("127.0.0.1", ports.get(timeout=5))) as client:
                nonce = lan.valid_challenge(lan.read_line(client, 64))
                header = lan.make_header("PART.DAT", b"x"*2048, nonce, SECRET)
                client.sendall(header)
                self.assertEqual(lan.read_line(client, 32), lan.READY)
                client.sendall(b"x"*100)
            error = result.get(timeout=7)
            self.assertIsInstance(error, lan.TransferError)
            self.assertEqual((inbox / "UPLOAD.PART").read_bytes(), b"x"*100)
            self.assertFalse((inbox / "PART.DAT").exists())

    def test_partial_or_colliding_files_are_protected(self):
        with tempfile.TemporaryDirectory() as root:
            path = Path(root)
            path.joinpath("INBOX").mkdir()
            inbox = path / "INBOX"
            inbox.joinpath("UPLOAD.PART").write_bytes(b"owner evidence")
            outbound = path / "N.BIN"
            outbound.write_bytes(b"abc123")
            port, result, _ = self._pair(inbox)
            with self.assertRaises(lan.TransferError):
                lan.send_file("127.0.0.1", outbound, SECRET,
                    port=port, allow_loopback=True)
            self.assertEqual(result.get(timeout=7)[0], "error")
            self.assertEqual((inbox / "UPLOAD.PART").read_bytes(), b"owner evidence")
            inbox.joinpath("UPLOAD.PART").unlink()
            inbox.joinpath("n.bin").write_bytes(b"another")
            port, result, _ = self._pair(inbox)
            with self.assertRaises(lan.TransferError):
                lan.send_file("127.0.0.1", outbound, SECRET,
                              port=port, allow_loopback=True)
            self.assertEqual(result.get(timeout=7)[0], "error")
            self.assertEqual(inbox.joinpath("n.bin").read_bytes(), b"another")

    def test_send_limits_and_symlink_guard(self):
        with tempfile.TemporaryDirectory() as root:
            folder = Path(root)
            path = folder / "GIANT.BIN"
            path.write_bytes(b"z"*(lan.MAX_SIZE+1))
            with self.assertRaises(lan.TransferError):
                lan.send_file("127.0.0.1", path, SECRET, allow_loopback=True)
            symlink = folder / "SYMLINK.BIN"
            symlink.symlink_to(path)
            with self.assertRaises(lan.TransferError):
                lan.send_file("127.0.0.1", symlink, SECRET, allow_loopback=True)
            with self.assertRaises(lan.TransferError):
                lan.incoming_folder(symlink)


if __name__ == "__main__":
    unittest.main(verbosity=2)
