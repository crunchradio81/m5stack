import XCTest
@testable import RemoteCore

final class RemoteCoreTests: XCTestCase {
    let secret = Array(0..<32).map(UInt8.init)
    let challenge = "0123456789abcdef0123456789abcdef"

    func testAuthenticatedMacrosAndMovement() throws {
        let m = RemoteProtocol.Message(sequence: 1, verb: "MACRO", argument: "12")
        let line = RemoteProtocol.encode(m, challenge: challenge, secret: secret)
        XCTAssertEqual(try RemoteProtocol.decode(line: line, challenge: challenge,
                         secret: secret, previousSequence: 0), m)
        XCTAssertThrowsError(try RemoteProtocol.decode(line: line, challenge: challenge,
                             secret: secret, previousSequence: 1))
        let move = RemoteProtocol.Message(sequence: 2, verb: "MOVE", argument: "-40,40")
        XCTAssertEqual(try RemoteProtocol.decode(
            line: RemoteProtocol.encode(move, challenge: challenge, secret: secret),
            challenge: challenge, secret: secret, previousSequence: 1), move)
    }

    func testRejectReplayedCrossSessionAndTampering() {
        let m = RemoteProtocol.Message(sequence: 6, verb: "CLICK", argument: "L")
        let line = RemoteProtocol.encode(m, challenge: challenge, secret: secret)
        XCTAssertThrowsError(try RemoteProtocol.decode(line: line,
            challenge: "ffeeddccbbaa99887766554433221100", secret: secret,
            previousSequence: 0))
        XCTAssertThrowsError(try RemoteProtocol.decode(
            line: line.replacingOccurrences(of: "|L|", with: "|R|"),
            challenge: challenge, secret: secret, previousSequence: 0))
    }

    func testStrictPayloads() {
        for payload in ["41", "ff", "1b", "0a", String(repeating: "41", count: 65)] {
            let m = RemoteProtocol.Message(sequence: 1, verb: "TEXT", argument: payload)
            let line = RemoteProtocol.encode(m, challenge: challenge, secret: secret)
            let valid = payload == "41"
            if valid {
                XCTAssertNoThrow(try RemoteProtocol.decode(
                    line: line, challenge: challenge, secret: secret, previousSequence: 0))
            } else {
                XCTAssertThrowsError(try RemoteProtocol.decode(
                    line: line, challenge: challenge, secret: secret, previousSequence: 0))
            }
        }
        let invalid = [
            RemoteProtocol.Message(sequence: 1, verb: "MOVE", argument: "500,1"),
            RemoteProtocol.Message(sequence: 1, verb: "SCROLL", argument: "9"),
            RemoteProtocol.Message(sequence: 1, verb: "MACRO", argument: "13"),
            RemoteProtocol.Message(sequence: 1, verb: "SHELL", argument: "rm"),
        ]
        for m in invalid {
            XCTAssertThrowsError(try RemoteProtocol.decode(
                line: RemoteProtocol.encode(m, challenge: challenge, secret: secret),
                challenge: challenge, secret: secret, previousSequence: 0))
        }
    }
    func testHexRoundTrip() {
        XCTAssertEqual(RemoteProtocol.bytes(from: "00ffAA"), [0,255,170])
        XCTAssertNil(RemoteProtocol.bytes(from: "abc"))
        XCTAssertNil(RemoteProtocol.bytes(from: "xx"))
        XCTAssertEqual(RemoteProtocol.hex([0,15,255]), "000fff")
    }
}
