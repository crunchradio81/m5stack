import AppKit
import Foundation
import SwiftUI

/// P27 owner-driven SD installer. Never flashes OTA, never overwrites files,
/// and never copies without an explicit archive check and owner confirmation.
/// Uses the bundled P27 standard-library helper, not a network downloader.
@MainActor
final class SDPack27Model: ObservableObject {
    @Published var archive: URL?
    @Published var cardRoot: URL?
    @Published var status = "No SD files have been changed. Select a trusted suite ZIP and SD volume root."
    @Published var busy = false
    @Published var planValid = false
    @Published var confirmed = false
    @Published var done = false
    @Published var newFiles = 0
    @Published var identicalFiles = 0

    private var child: Process?
    private var pendingAction = ""

    var selectedArchive: String { archive?.lastPathComponent ?? "Choose Cardputer SD Apps ZIP" }
    var selectedCard: String { cardRoot?.path ?? "Choose the mounted microSD volume root" }
    var canInspect: Bool { archive != nil && cardRoot != nil && !busy }
    var canCopy: Bool { canInspect && planValid && confirmed }

    func chooseArchive() {
        guard !busy else { return }
        let p = NSOpenPanel()
        p.title = "Choose an original K132 30-app suite archive"
        p.message = "Use the trusted Cardputer-SD-Apps ZIP from your GitHub Actions run."
        p.canChooseFiles = true
        p.canChooseDirectories = false
        p.allowsMultipleSelection = false
        p.allowedContentTypes = [.zip]
        if p.runModal() == .OK, let selected = p.url {
            archive = selected
            resetPlan()
            status = "Selected archive. Choose the SD volume root, then inspect it."
        }
    }

    func chooseCardRoot() {
        guard !busy else { return }
        let p = NSOpenPanel()
        p.title = "Select the root of the Cardputer microSD card"
        p.message = "Choose the mounted FAT32 volume itself (not the SUITE or APPS folder). No files will be copied yet."
        p.canChooseFiles = false
        p.canChooseDirectories = true
        p.canCreateDirectories = false
        p.allowsMultipleSelection = false
        if p.runModal() == .OK, let selected = p.url {
            cardRoot = selected
            resetPlan()
            status = "SD destination selected. Inspect will not modify the card."
        }
    }

    func resetPlan() {
        planValid = false
        confirmed = false
        done = false
        newFiles = 0
        identicalFiles = 0
    }

    func inspect() {
        guard canInspect else { return }
        resetPlan()
        run(mode: "plan")
    }

    func copy() {
        guard canCopy else { return }
        run(mode: "install")
    }

    func cancel() {
        guard busy, let child, child.isRunning else { return }
        child.terminate()
        status = "Cancelled. Some newly created SD files may be incomplete; inspect and recover before retrying."
        confirmed = false
        planValid = false
    }

    private func pythonExecutable() -> String? {
        ["/opt/homebrew/bin/python3", "/usr/local/bin/python3", "/usr/bin/python3"]
            .first(where: { FileManager.default.isExecutableFile(atPath: $0) })
    }

    private func scriptURL() -> URL? {
        if let bundle = Bundle.main.url(forResource: "sdpack27", withExtension: "py") {
            return bundle
        }
        let location = URL(fileURLWithPath: #filePath)
            .deletingLastPathComponent()
            .deletingLastPathComponent()
            .deletingLastPathComponent()
            .deletingLastPathComponent()
        let source = location.appendingPathComponent("Desktop/sdpack27.py")
        return FileManager.default.fileExists(atPath: source.path) ? source : nil
    }

    private func run(mode: String) {
        guard !busy, let archive, let cardRoot else { return }
        guard let python = pythonExecutable(), let script = scriptURL() else {
            status = "Python 3 or bundled SD installer helper is missing."
            resetPlan()
            return
        }
        let task = Process()
        task.executableURL = URL(fileURLWithPath: python)
        task.arguments = [
            script.path, mode, "--archive", archive.path,
            "--target", cardRoot.path
        ] + (mode == "install" ? ["--confirm-copy"] : [])
        let pipe = Pipe()
        task.standardOutput = pipe
        task.standardError = pipe
        task.currentDirectoryURL = FileManager.default.homeDirectoryForCurrentUser
        task.terminationHandler = { [weak self] ended in
            let code = ended.terminationStatus
            let result = pipe.fileHandleForReading.readDataToEndOfFile()
            let report = String(decoding: result, as: UTF8.self)
            Task { @MainActor [weak self] in
                self?.finished(mode: mode, status: code, report: report)
            }
        }
        do {
            try task.run()
            child = task
            pendingAction = mode
            busy = true
            done = false
            status = mode == "plan" ? "Validating all 30 manifests and SHA-256 hashes…"
                                    : "Copying new files. Do not remove the SD card."
        } catch {
            status = "Could not run SD inspector: \(error.localizedDescription)"
            resetPlan()
        }
    }

    private func finished(mode: String, status code: Int32, report: String) {
        busy = false
        child = nil
        pendingAction = ""
        let lines = report.split(separator: "\n").map(String.init)
        if code != 0 {
            let errorLine = lines.last(where: { $0.hasPrefix("SDPACK27 ERROR:") })
            status = errorLine ?? "SD pack validation/copy failed (status \(code)). No existing files are overwritten; inspect partial new files."
            planValid = false
            confirmed = false
            return
        }
        let plans = lines.filter({ $0.hasPrefix("PLAN:APPS=") })
        if let line = plans.last, let fields = parsePlan(line), fields.apps == 30 {
            newFiles = fields.new
            identicalFiles = fields.identical
        }
        if mode == "plan" && lines.contains(where: { $0.hasPrefix("READY:") }) {
            planValid = true
            confirmed = false
            status = "Verified all 30 app pairs. \(newFiles) new files; \(identicalFiles) byte-identical files already present. No changes made."
        } else if mode == "install", let finished = lines.first(where: { $0.hasPrefix("INSTALLED:") }) {
            status = "SD installation finished. \(finished). Eject the card safely in Finder."
            done = true
            planValid = false
            confirmed = false
        } else {
            resetPlan()
            status = "Unexpected SD installer response; no success claimed."
        }
    }

    private func parsePlan(_ line: String) -> (apps: Int, new: Int, identical: Int)? {
        let chunks = line.split(separator: "|").map(String.init)
        guard chunks.count == 3,
              chunks[0].hasPrefix("PLAN:APPS="),
              chunks[1].hasPrefix("NEW="),
              chunks[2].hasPrefix("IDENTICAL="),
              let apps = Int(chunks[0].dropFirst("PLAN:APPS=".count)),
              let new = Int(chunks[1].dropFirst("NEW=".count)),
              let identical = Int(chunks[2].dropFirst("IDENTICAL=".count)),
              apps == 30, new >= 0, identical >= 0, new + identical == 60
        else { return nil }
        return (apps, new, identical)
    }
}
