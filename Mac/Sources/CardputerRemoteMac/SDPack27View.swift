import AppKit
import UniformTypeIdentifiers
import SwiftUI

struct SDPack27View: View {
    @StateObject private var installer = SDPack27Model()

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 16) {
                HStack {
                    Label("Install SD App Library", systemImage: "sdcard")
                        .font(.title2.bold())
                    Spacer()
                    Text(installer.done ? "COPIED" :
                         installer.busy ? "WORKING" : "NO CHANGES")
                        .foregroundStyle(installer.done ? .green : .secondary)
                }
                Text("Prepare the original stock Cardputer K132 microSD library without flashing firmware.")
                    .foregroundStyle(.secondary)

                GroupBox("1 · Select the complete app archive") {
                    HStack {
                        Button("Choose GitHub suite ZIP…") { installer.chooseArchive() }
                            .disabled(installer.busy)
                        Text(installer.selectedArchive)
                            .lineLimit(1)
                            .truncationMode(.middle)
                        Spacer()
                    }.padding(6)
                }
                GroupBox("2 · Select the microSD card root") {
                    VStack(alignment: .leading, spacing: 8) {
                        HStack {
                            Button("Choose mounted SD volume…") { installer.chooseCardRoot() }
                                .disabled(installer.busy)
                            Text(installer.selectedCard)
                                .lineLimit(1)
                                .truncationMode(.middle)
                        }
                        Text("P28 checks the actual mounted external FAT32 volume. Ordinary Mac folders, internal disks and unsupported filesystems are refused before copying.")
                            .font(.caption).foregroundStyle(.secondary)
                    }.frame(maxWidth: .infinity, alignment: .leading).padding(6)
                }
                GroupBox("3 · Verify the archive and preview changes") {
                    VStack(alignment: .leading, spacing: 10) {
                        Button("Inspect archive and SD card (read-only)") {
                            installer.inspect()
                        }
                        .disabled(!installer.canInspect)
                        HStack(spacing: 22) {
                            Label("\(installer.newFiles) new", systemImage: "plus.square")
                            Label("\(installer.identicalFiles) already identical", systemImage: "checkmark.circle")
                        }.font(.callout)
                        Text("The preview checks all 30 image/manifest pairs, their SHA-256 hashes, and the Mac’s removable FAT32 volume metadata.")
                            .font(.caption).foregroundStyle(.secondary)
                    }.frame(maxWidth: .infinity, alignment: .leading).padding(6)
                }
                GroupBox("4 · Read-only audit and recovery") {
                    VStack(alignment: .leading, spacing: 10) {
                        Button("Audit all 60 files already on SD") { installer.audit() }
                            .disabled(!installer.canInspect)
                        Text(installer.auditReport)
                            .font(.caption)
                            .textSelection(.enabled)
                        Text("Missing or changed files are reported for owner review; this never deletes or repairs anything.")
                            .font(.caption).foregroundStyle(.secondary)
                    }
                    .frame(maxWidth: .infinity, alignment: .leading).padding(6)
                }
                GroupBox("5 · Explicitly copy new apps") {
                    VStack(alignment: .leading, spacing: 11) {
                        Toggle("I selected the root of my own FAT32 Cardputer microSD, and understand that only new files will be created.",
                               isOn: $installer.confirmed)
                            .disabled(!installer.planValid || installer.busy)
                        HStack {
                            Button("Copy verified apps to SD") { installer.copy() }
                                .buttonStyle(.borderedProminent)
                                .disabled(!installer.canCopy)
                            Button("Stop copy") { installer.cancel() }
                                .disabled(!installer.busy)
                        }
                        Text("The card mount is rechecked while copying. Every image is followed by SHA-256 readback and a full 60-file audit. If interrupted, manually inspect partial files.")
                            .font(.caption).foregroundStyle(.secondary)
                    }.frame(maxWidth: .infinity, alignment: .leading).padding(6)
                }
                GroupBox("Current status") {
                    Label(installer.status, systemImage: installer.done ?
                          "checkmark.circle" : "info.circle")
                        .frame(maxWidth: .infinity, alignment: .leading)
                        .textSelection(.enabled).padding(6)
                }

                Label("This installs SD files only; it never flashes the factory launcher or changes ESP32 partitions. The matching factory firmware must be installed separately over USB.",
                      systemImage: "exclamationmark.triangle")
                    .font(.caption)
                    .foregroundStyle(.orange)
                    .fixedSize(horizontal: false, vertical: true)

                Text("Manifest SHA-256 detects accidental corruption, not authenticity. Use only a trusted GitHub build; third-party or modified archives are not signed.")
                    .font(.caption).foregroundStyle(.secondary)
            }
            .frame(maxWidth: 760, alignment: .leading)
            .padding(18)
        }
    }
}
