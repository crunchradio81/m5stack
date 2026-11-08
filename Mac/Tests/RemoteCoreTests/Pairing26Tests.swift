import Foundation
import XCTest
@testable import RemoteCore

final class Pairing26Tests: XCTestCase {
    private let secret = Array(0..<32).map(UInt8.init)

    func testExactCardputerFormat() throws {
        let raw = try Pairing26.encode(address: "192.168.1.22", secret: secret)
        let value = try XCTUnwrap(String(data: raw, encoding: .ascii))
        let lines = value.split(separator: "\n")
        XCTAssertEqual(lines.count, 3)
        XCTAssertEqual(lines[0], "K132MAC1")
        XCTAssertEqual(lines[1], "HOST=192.168.1.22")
        XCTAssertEqual(lines[2], "TOKEN=000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f")
        XCTAssertTrue(value.hasSuffix("\n"))
        XCTAssertFalse(value.contains("\r"))
        XCTAssertLessThanOrEqual(raw.count, 160)
    }

    func testPrivateIPv4Policy() {
        for good in ["10.0.0.1", "10.255.255.254", "172.16.0.2",
                     "172.31.255.254", "192.168.1.22", "192.168.0.5"] {
            XCTAssertTrue(Pairing26.validPrivateIPv4(good), good)
        }
        for bad in ["", "192.168.01.2", "192.168.1.0", "192.168.1.255",
                    "192.168.1.999", "192.168.1.2\nTOKEN=AAAA", "0.0.0.0",
                    "127.0.0.1", "8.8.8.8", "172.15.1.2", "172.32.1.2",
                    "100.64.0.1", "::1", "mac.local", "192.168.1", "192.168..1"] {
            XCTAssertFalse(Pairing26.validPrivateIPv4(bad), bad)
        }
    }

    func testRejectedKeyAndAddressNeverProduceFile() throws {
        let root = FileManager.default.temporaryDirectory
            .appendingPathComponent(UUID().uuidString, isDirectory: true)
        try FileManager.default.createDirectory(at: root, withIntermediateDirectories: false)
        defer { try? FileManager.default.removeItem(at: root) }
        XCTAssertThrowsError(try Pairing26.saveNew(in: root, address: "8.8.8.8", secret: secret))
        XCTAssertThrowsError(try Pairing26.saveNew(in: root, address: "192.168.1.2", secret: [1,2]))
        XCTAssertFalse(FileManager.default.fileExists(atPath: root.appendingPathComponent("REMOTE.CFG").path))
    }

    func testOwnerOnlyWriteAndExistingFileNeverOverwritten() throws {
        let root = FileManager.default.temporaryDirectory
            .appendingPathComponent(UUID().uuidString, isDirectory: true)
        try FileManager.default.createDirectory(at: root, withIntermediateDirectories: false)
        defer { try? FileManager.default.removeItem(at: root) }
        let target = try Pairing26.saveNew(in: root,
                                             address: "10.23.4.5", secret: secret)
        let saved = try Data(contentsOf: target)
        XCTAssertEqual(saved, try Pairing26.encode(address: "10.23.4.5", secret: secret))
        let permissions = try FileManager.default.attributesOfItem(atPath: target.path)
        let mode = try XCTUnwrap(permissions[.posixPermissions] as? NSNumber).intValue
        XCTAssertEqual(mode & 0o777, 0o600)
        XCTAssertThrowsError(try Pairing26.saveNew(in: root,
                                                   address: "10.23.4.6", secret: secret))
        XCTAssertEqual(try Data(contentsOf: target), saved)
    }

    func testSymlinkCannotReplaceOwnerPairingFile() throws {
        let root = FileManager.default.temporaryDirectory
            .appendingPathComponent(UUID().uuidString, isDirectory: true)
        try FileManager.default.createDirectory(at: root, withIntermediateDirectories: false)
        defer { try? FileManager.default.removeItem(at: root) }
        let outside = root.appendingPathComponent("old.cfg")
        try Data("owner data".utf8).write(to: outside)
        try FileManager.default.createSymbolicLink(
            at: root.appendingPathComponent("REMOTE.CFG"), withDestinationURL: outside)
        XCTAssertThrowsError(try Pairing26.saveNew(in: root,
                                                   address: "10.2.3.4", secret: secret))
        XCTAssertEqual(try String(contentsOf: outside, encoding: .utf8), "owner data")
    }
}
