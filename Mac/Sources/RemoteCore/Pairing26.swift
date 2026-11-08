import Darwin
import Foundation

/// The exact K132MAC1 owner-private SD pairing format consumed by
/// src/core/MacRemote22Core.h on the original stock Cardputer.
/// Remote access remains OFF until the Mac owner explicitly enables it.
enum Pairing26 {
    enum Error: Swift.Error, LocalizedError {
        case invalidAddress, invalidKey, invalidDirectory, exists, writeFailed
        var errorDescription: String? {
            switch self {
            case .invalidAddress:
                return "Enter this Mac's private IPv4 address (10.x, 172.16–31.x, or 192.168.x)."
            case .invalidKey:
                return "Mac pairing token is not a 256-bit key."
            case .invalidDirectory:
                return "Choose an existing folder for REMOTE.CFG."
            case .exists:
                return "REMOTE.CFG already exists. It was not replaced; review it before choosing a new location."
            case .writeFailed:
                return "Could not safely write REMOTE.CFG. No pairing token was printed."
            }
        }
    }

    /// Rejects hostnames, leading-zero octets, wildcard IPs, public IPs
    /// and host-octet 0/255 exactly like the K132 source parser.
    static func validPrivateIPv4(_ address: String) -> Bool {
        let parts = address.split(separator: ".", omittingEmptySubsequences: false)
        guard parts.count == 4 else { return false }
        var numbers: [Int] = []
        for part in parts {
            guard part.count > 0, part.count <= 3,
                  !(part.count > 1 && part.first == "0"),
                  part.utf8.allSatisfy({ (48...57).contains($0) }),
                  let number = Int(part), (0...255).contains(number) else {
                return false
            }
            numbers.append(number)
        }
        guard numbers[3] != 0, numbers[3] != 255 else { return false }
        return numbers[0] == 10 ||
            (numbers[0] == 172 && (16...31).contains(numbers[1])) ||
            (numbers[0] == 192 && numbers[1] == 168)
    }

    static func encode(address: String, secret: [UInt8]) throws -> Data {
        guard validPrivateIPv4(address) else { throw Error.invalidAddress }
        guard secret.count == 32 else { throw Error.invalidKey }
        let alphabet = Array("0123456789abcdef".utf8)
        var hex = [UInt8]()
        hex.reserveCapacity(64)
        for byte in secret {
            hex.append(alphabet[Int(byte >> 4)])
            hex.append(alphabet[Int(byte & 15)])
        }
        // Exactly three LF-terminated lines. No comments, CRLF or keys.
        let token = String(decoding: hex, as: UTF8.self)
        let format = "K132MAC1\nHOST=\(address)\nTOKEN=\(token)\n"
        guard let value = format.data(using: .ascii), value.count <= 160 else {
            throw Error.invalidKey
        }
        return value
    }

    /// Owner explicitly chooses a folder; write is exclusive and 0600 on
    /// APFS. A FAT32 SD card does not preserve Unix file permissions.
    /// Never replace/rename an existing pairing config or follow a file symlink.
    @discardableResult
    static func saveNew(in folder: URL, address: String,
                        secret: [UInt8]) throws -> URL {
        let value = try encode(address: address, secret: secret)
        var isDirectory: ObjCBool = false
        guard folder.isFileURL,
              FileManager.default.fileExists(atPath: folder.path,
                                             isDirectory: &isDirectory),
              isDirectory.boolValue else {
            throw Error.invalidDirectory
        }
        let destination = folder.appendingPathComponent("REMOTE.CFG", isDirectory: false)
        let fd = Darwin.open(destination.path,
                             O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0o600)
        guard fd >= 0 else {
            if errno == EEXIST { throw Error.exists }
            throw Error.writeFailed
        }
        defer { Darwin.close(fd) }
        let success = value.withUnsafeBytes { bytes -> Bool in
            guard let base = bytes.baseAddress else { return false }
            var offset = 0
            while offset < bytes.count {
                let written = Darwin.write(fd, base.advanced(by: offset),
                                           bytes.count - offset)
                if written <= 0 { return false }
                offset += written
            }
            return Darwin.fsync(fd) == 0
        }
        if !success {
            _ = Darwin.unlink(destination.path)
            throw Error.writeFailed
        }
        return destination
    }
}
