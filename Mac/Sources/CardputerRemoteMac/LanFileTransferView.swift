import AppKit
import SwiftUI

/// P25: a genuine SwiftUI file-transfer window inside the existing native
/// Cardputer Remote Mac app. The P24 Python helper is bundled as an auditable
/// subprocess, not reimplemented and not an automatically running server.
struct LanFileTransferView: View {
    @StateObject private var transfer = LanFileTransferModel()

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 17) {
                HStack {
                    Label("Cardputer File Transfer", systemImage: "arrow.left.arrow.right.square")
                        .font(.title2.bold())
                    Spacer()
                    Text(transfer.working ? "ACTIVE" : "OFF")
                        .foregroundStyle(transfer.working ? Color.orange : Color.secondary)
                }
                Text("Send or receive one small file with the original K132 LANFILES app. No background service.")
                    .foregroundStyle(.secondary)

                GroupBox("Pairing — separate from Mac keyboard/mouse macros") {
                    VStack(alignment: .leading, spacing: 9) {
                        Text("LANFILES uses its own LAN.CFG, not the Keychain token for macro controls.")
                            .font(.caption)
                        HStack {
                            Button("Choose LAN.CFG") { transfer.chooseKey() }
                            Button("Generate LAN.CFG…") { transfer.createKey() }
                            Spacer()
                        }
                        .disabled(transfer.working)
                        Label(transfer.selectedKeyName,
                              systemImage: transfer.keyReady ? "checkmark.shield" : "lock")
                            .font(.callout.monospaced())
                        Text("Copy the same private key file to /SUITE/NET/LAN.CFG on the Cardputer SD. Never share the key.")
                            .font(.caption).foregroundStyle(.secondary)
                    }
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .padding(5)
                }

                Picker("Direction", selection: $transfer.direction) {
                    ForEach(LanFileTransferModel.Direction.allCases) { direction in
                        Text(direction.rawValue).tag(direction)
                    }
                }
                .pickerStyle(.segmented)
                .disabled(transfer.working)

                GroupBox(transfer.direction == .send ? "Send a file" : "Receive a file") {
                    VStack(alignment: .leading, spacing: 12) {
                        if transfer.direction == .send {
                            Text("On Cardputer: open LANFILES, C to connect, then L to receive.")
                                .font(.callout)
                            HStack {
                                Text("Cardputer private IPv4").frame(width: 170, alignment: .leading)
                                TextField("192.168.1.50", text: $transfer.cardputerIP)
                                    .textFieldStyle(.roundedBorder)
                                    .disabled(transfer.working)
                            }
                            HStack {
                                Button("Choose file…") { transfer.chooseFile() }
                                    .disabled(transfer.working)
                                Text(transfer.selectedFileName)
                                    .lineLimit(1)
                                    .truncationMode(.middle)
                            }
                            Text("1–262,144 bytes. DOS-safe filename. Existing Cardputer files will not be overwritten.")
                                .font(.caption).foregroundStyle(.secondary)
                        } else {
                            Text("On Mac: start the receiver here. On Cardputer: C connect, select OUTBOX file, T send.")
                                .font(.callout)
                            HStack {
                                Text("This Mac's private IPv4").frame(width: 170, alignment: .leading)
                                TextField("192.168.1.22", text: $transfer.macIP)
                                    .textFieldStyle(.roundedBorder)
                                    .disabled(transfer.working)
                            }
                            HStack {
                                Button("Choose inbox…") { transfer.chooseInbox() }
                                    .disabled(transfer.working)
                                Text(transfer.inboxDisplay)
                                    .lineLimit(1)
                                    .truncationMode(.middle)
                            }
                            Text("One receive window lasts at most 35 seconds. Files are verified before publication.")
                                .font(.caption).foregroundStyle(.secondary)
                        }
                    }
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .padding(5)
                }

                HStack(spacing: 14) {
                    Button(transfer.direction == .send ? "Send to Cardputer" : "Receive one file") {
                        transfer.start()
                    }
                    .buttonStyle(.borderedProminent)
                    .disabled(!transfer.canRun)
                    Button("Cancel") { transfer.cancel() }
                        .disabled(!transfer.working)
                    Spacer()
                }
                if transfer.working {
                    ProgressView(value: transfer.progress)
                        .progressViewStyle(.linear)
                        .accessibilityLabel("Transfer progress")
                    Text("\(Int(transfer.progress * 100))%")
                        .monospacedDigit()
                        .font(.caption)
                }

                GroupBox("Transfer status") {
                    Label(transfer.activity,
                          systemImage: transfer.lastSuccess ? "checkmark.circle" :
                              transfer.working ? "arrow.clockwise" : "info.circle")
                        .frame(maxWidth: .infinity, alignment: .leading)
                        .textSelection(.enabled)
                        .padding(5)
                }

                Label("Plaintext TCP on a trusted private LAN only. HMAC authenticates the peer but does not encrypt file data. Never use for passwords or sensitive documents.",
                      systemImage: "exclamationmark.shield")
                    .font(.caption)
                    .foregroundStyle(.orange)
                    .fixedSize(horizontal: false, vertical: true)
                Text("A failed or cancelled receive may leave UPLOAD.PART for owner inspection. No files are automatically erased.")
                    .font(.caption).foregroundStyle(.secondary)
                if !transfer.hasPython {
                    Text("Python 3 is required for the bundled P24 transfer engine. Install Python 3, then reopen this app.")
                        .font(.caption).foregroundStyle(.red)
                }
            }
            .frame(maxWidth: 760, alignment: .leading)
            .padding(18)
        }
    }
}
