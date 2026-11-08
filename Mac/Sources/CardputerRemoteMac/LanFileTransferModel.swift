import AppKit
import Foundation
import SwiftUI

/// Native Mac UI state for the exact P24/P23 LANFILES protocol.
/// The only subprocess is our bundled, hash-tested standard-library Python
/// helper. No shell, no arbitrary command strings, no background daemon.
@MainActor
final class LanFileTransferModel: ObservableObject {
    enum Direction: String, CaseIterable, Identifiable {
        case send = "Send to Cardputer"
        case receive = "Receive from Cardputer"
        var id: String { rawValue }
    }

    @Published var direction: Direction = .send
    @Published var cardputerIP = ""
    @Published var macIP = ""
    @Published var keyFile: URL?
    @Published var outgoingFile: URL?
    @Published var inbox: URL?
    @Published var activity = "Transfer is off. Select a key and a destination."
    @Published var progress = 0.0
    @Published var working = false
    @Published var lastSuccess = false

    private var process: Process?
    private var outputPipe: Pipe?
    private var lineBuffer = Data()
    private var generationDestination: URL?
    private var lastDetail = ""

    var selectedKeyName: String { keyFile?.lastPathComponent ?? "Choose LAN.CFG" }
    var selectedFileName: String { outgoingFile?.lastPathComponent ?? "Choose a file (up to 256 KiB)" }
    var inboxDisplay: String { inbox?.path ?? "Choose a receive folder" }
    var keyReady: Bool { keyFile != nil }
    var hasPython: Bool { pythonExecutable() != nil }

    var canRun: Bool {
        guard !working, keyReady, hasPython else { return false }
        if direction == .send {
            return outgoingFile != nil && !cardputerIP.isEmpty
        }
        return inbox != nil && !macIP.isEmpty
    }

    func chooseKey() {
        guard !working else { return }
        let panel = NSOpenPanel()
        panel.title = "Select your private K132 LAN.CFG"
        panel.message = "Only select the LAN.CFG shared with your own Cardputer."
        panel.canChooseFiles = true
        panel.canChooseDirectories = false
        panel.allowsMultipleSelection = false
        if panel.runModal() == .OK, let url = panel.url {
            keyFile = url
            activity = "Selected private LAN.CFG (contents hidden)"
        }
    }

    func createKey() {
        guard !working else { return }
        let panel = NSSavePanel()
        panel.title = "Create a new Cardputer LAN pairing key"
        panel.nameFieldStringValue = "LAN.CFG"
        panel.message = "Create a private key file; copy it to /SUITE/NET/LAN.CFG on your Cardputer SD."
        if panel.runModal() == .OK, let url = panel.url {
            // P24 generator uses O_EXCL and never overwrites an existing key.
            generationDestination = url
            launch(arguments: ["generate-key", "--out", url.path],
                   operation: "Generating a private LAN key", validatingKey: false)
        }
    }

    func chooseFile() {
        guard !working else { return }
        let panel = NSOpenPanel()
        panel.title = "Choose one file for Cardputer"
        panel.canChooseFiles = true
        panel.canChooseDirectories = false
        panel.allowsMultipleSelection = false
        if panel.runModal() == .OK, let url = panel.url {
            outgoingFile = url
            activity = "Selected \(url.lastPathComponent) (not yet sent)"
        }
    }

    func chooseInbox() {
        guard !working else { return }
        let panel = NSOpenPanel()
        panel.title = "Choose the Mac receive folder"
        panel.message = "Received files are never allowed to overwrite an existing file."
        panel.canChooseFiles = false
        panel.canChooseDirectories = true
        panel.canCreateDirectories = true
        panel.allowsMultipleSelection = false
        if panel.runModal() == .OK, let url = panel.url {
            inbox = url
            activity = "Selected inbox. Nothing is listening until you press Receive."
        }
    }

    func start() {
        guard canRun, let key = keyFile else { return }
        if direction == .send {
            guard let path = outgoingFile else { return }
            launch(arguments: [
                "send", "--peer", cardputerIP.trimmingCharacters(in: .whitespaces),
                "--file", path.path, "--key-file", key.path, "--progress"
            ], operation: "Sending \(path.lastPathComponent)", validatingKey: true)
        } else {
            guard let folder = inbox else { return }
            launch(arguments: [
                "receive", "--bind", macIP.trimmingCharacters(in: .whitespaces),
                "--inbox", folder.path, "--key-file", key.path, "--progress"
            ], operation: "Listening once for a Cardputer file (35 seconds)",
            validatingKey: true)
        }
    }

    func cancel() {
        guard working, let process, process.isRunning else { return }
        // The child is a one-shot process. Termination leaves any PART file
        // intact for owner inspection (no "cleanup" after interruption).
        process.terminate()
        activity = "Cancelled. Check UPLOAD.PART before retrying."
        lastSuccess = false
    }

    private func pythonExecutable() -> String? {
        let paths = ["/opt/homebrew/bin/python3", "/usr/local/bin/python3", "/usr/bin/python3"]
        return paths.first(where: { FileManager.default.isExecutableFile(atPath: $0) })
    }

    private func scriptURL() -> URL? {
        if let bundled = Bundle.main.url(forResource: "lanfile24", withExtension: "py") {
            return bundled
        }
        // Development-only fallback for swift run from the repository checkout.
        let here = URL(fileURLWithPath: #filePath)
        let sourceRoot = here.deletingLastPathComponent() // CardputerRemoteMac/
            .deletingLastPathComponent() // Sources/
            .deletingLastPathComponent() // Mac/
            .deletingLastPathComponent() // repository/
        let source = sourceRoot.appendingPathComponent("Desktop/lanfile24.py")
        return FileManager.default.fileExists(atPath: source.path) ? source : nil
    }

    private func launch(arguments: [String], operation: String, validatingKey: Bool) {
        guard !working else { return }
        guard let python = pythonExecutable(), let script = scriptURL() else {
            activity = "Python 3 or bundled LANFILES helper missing. Install Python 3."
            lastSuccess = false
            return
        }
        if validatingKey {
            guard let keyFile, FileManager.default.isReadableFile(atPath: keyFile.path) else {
                activity = "Choose a readable private LAN.CFG"
                lastSuccess = false
                return
            }
        }
        let child = Process()
        child.executableURL = URL(fileURLWithPath: python)
        // No bash -c, commands are generated only by the UI's fixed actions.
        child.arguments = [script.path] + arguments
        let pipe = Pipe()
        child.standardOutput = pipe
        child.standardError = pipe
        child.currentDirectoryURL = FileManager.default.homeDirectoryForCurrentUser
        child.terminationHandler = { [weak self] ended in
            let status = ended.terminationStatus
            Task { @MainActor [weak self] in
                self?.complete(status: status)
            }
        }
        pipe.fileHandleForReading.readabilityHandler = { [weak self] handle in
            let chunk = handle.availableData
            if !chunk.isEmpty {
                Task { @MainActor [weak self] in self?.ingest(chunk) }
            }
        }
        do {
            try child.run()
            process = child
            outputPipe = pipe
            lineBuffer = Data()
            lastDetail = ""
            progress = 0
            working = true
            lastSuccess = false
            activity = operation
        } catch {
            pipe.fileHandleForReading.readabilityHandler = nil
            outputPipe = nil
            generationDestination = nil
            activity = "Cannot start Python helper: \(error.localizedDescription)"
        }
    }

    private func ingest(_ bytes: Data) {
        lineBuffer.append(bytes)
        if lineBuffer.count > 8192 {
            lineBuffer.removeAll()
            activity = "Unexpected excessive helper output"
            return
        }
        while let newline = lineBuffer.firstIndex(of: 10) {
            let line = lineBuffer.prefix(upTo: newline)
            lineBuffer.removeSubrange(...newline)
            guard let text = String(data: Data(line), encoding: .utf8) else { continue }
            if text.hasPrefix("PROGRESS:"),
               let n = Int(text.dropFirst("PROGRESS:".count)), (0...100).contains(n) {
                progress = Double(n) / 100
                activity = direction == .send
                    ? "Sending… \(n)%"
                    : "Receiving… \(n)%"
            } else if !text.isEmpty {
                // Never display raw key file contents. The helper only emits
                // progress, the bound address, and specific error summaries.
                lastDetail = String(text.prefix(240))
                activity = lastDetail
            }
        }
    }

    private func complete(status code: Int32) {
        outputPipe?.fileHandleForReading.readabilityHandler = nil
        // Drain any final bytes after process termination.
        if let trailing = outputPipe?.fileHandleForReading.readDataToEndOfFile(),
           !trailing.isEmpty {
            ingest(trailing)
        }
        outputPipe = nil
        process = nil
        working = false
        lastSuccess = (code == 0)
        if code == 0 {
            if let generated = generationDestination {
                keyFile = generated
                activity = "Created private LAN.CFG. Copy it to the Cardputer SD."
            } else {
                progress = 1
                activity = lastDetail.isEmpty ? "Transfer completed" : lastDetail
            }
        } else if code != 0 {
            activity = lastDetail.isEmpty
                ? "Transfer failed or cancelled (exit \(code)). Check recovery files."
                : "Transfer failed: \(lastDetail)"
        }
        generationDestination = nil
    }
}
