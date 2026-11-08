import Foundation
import CryptoKit

/// Authenticated, session-bound local TCP commands. No shell execution.
public enum RemoteProtocol {
    public static let port: UInt16 = 38947
    public static let maxLine = 512
    public static let prefix = "K132R1"

    public struct Message: Equatable, Sendable {
        public let sequence: UInt64
        public let verb: String
        public let argument: String
        public init(sequence: UInt64, verb: String, argument: String) {
            self.sequence = sequence
            self.verb = verb
            self.argument = argument
        }
    }
    public enum Error: Swift.Error {
        case invalidFormat, invalidAuthentication, invalidSequence
        case invalidArgument, invalidCommand
    }
    public static func bytes(from hex: String) -> [UInt8]? {
        guard hex.count.isMultiple(of: 2), !hex.isEmpty else { return nil }
        var result = [UInt8]()
        result.reserveCapacity(hex.count / 2)
        let chars = Array(hex.utf8)
        for i in stride(from: 0, to: chars.count, by: 2) {
            func digit(_ x: UInt8) -> UInt8? {
                switch x {
                case 48...57: return x - 48
                case 65...70: return x - 65 + 10
                case 97...102: return x - 97 + 10
                default: return nil
                }
            }
            guard let hi = digit(chars[i]), let lo = digit(chars[i + 1]) else { return nil }
            result.append((hi << 4) | lo)
        }
        return result
    }
    public static func hex(_ bytes: [UInt8]) -> String {
        let alphabet = Array("0123456789abcdef".utf8)
        var output = [UInt8]()
        output.reserveCapacity(bytes.count * 2)
        for b in bytes {
            output.append(alphabet[Int(b >> 4)])
            output.append(alphabet[Int(b & 15)])
        }
        return String(decoding: output, as: UTF8.self)
    }
    public static func proof(for content: String, secret: [UInt8]) -> String {
        let digest = HMAC<SHA256>.authenticationCode(
            for: Data(content.utf8), using: SymmetricKey(data: Data(secret)))
        return hex(Array(digest))
    }
    /// Reject replay across connections: server creates unpredictable per-connection challenge.
    public static func decode(line: String, challenge: String, secret: [UInt8],
                              previousSequence: UInt64) throws -> Message {
        guard line.utf8.count < maxLine, line.unicodeScalars.allSatisfy({
            $0.value >= 32 && $0.value <= 126
        }) else { throw Error.invalidFormat }
        let fields = line.split(separator: "|", omittingEmptySubsequences: false).map(String.init)
        guard fields.count == 6,
              fields[0] == prefix, fields[1] == challenge,
              challenge.count == 32, bytes(from: challenge)?.count == 16,
              secret.count == 32,
              let seq = UInt64(fields[2]), seq > previousSequence,
              fields[5].count == 64,
              let signature = bytes(from: fields[5]) else {
            throw Error.invalidFormat
        }
        let content = fields[0...4].joined(separator: "|")
        let expect = bytes(from: proof(for: content, secret: secret))!
        // Constant-work comparison, not normal Swift string equality.
        var difference: UInt8 = 0
        for i in 0..<32 { difference |= signature[i] ^ expect[i] }
        guard difference == 0 else { throw Error.invalidAuthentication }
        let verb = fields[3], arg = fields[4]
        switch verb {
        case "MACRO":
            guard let n = Int(arg), (1...12).contains(n),
                  String(n) == arg else { throw Error.invalidArgument }
        case "MOVE":
            let v = arg.split(separator: ",", omittingEmptySubsequences: false)
            guard v.count == 2,
                  let dx = Int(v[0]), let dy = Int(v[1]),
                  abs(dx) <= 40, abs(dy) <= 40,
                  String(dx) == v[0], String(dy) == v[1] else {
                throw Error.invalidArgument
            }
        case "CLICK":
            guard ["L", "R"].contains(arg) else { throw Error.invalidArgument }
        case "SCROLL":
            guard let n = Int(arg), (-8...8).contains(n),
                  String(n) == arg else { throw Error.invalidArgument }
        case "TEXT":
            guard arg.count <= 128, arg.count.isMultiple(of: 2),
                  let decoded = bytes(from: arg), decoded.count <= 64,
                  let text = String(bytes: decoded, encoding: .utf8),
                  !text.unicodeScalars.contains(where: {
                      $0.value < 32 || $0.value == 127
                  }) else { throw Error.invalidArgument }
        default: throw Error.invalidCommand
        }
        return Message(sequence: seq, verb: verb, argument: arg)
    }
    public static func encode(_ message: Message, challenge: String,
                              secret: [UInt8]) -> String {
        let text = [prefix, challenge, String(message.sequence),
                    message.verb, message.argument].joined(separator: "|")
        return text + "|" + proof(for: text, secret: secret)
    }
}
