import AppKit
import ApplicationServices
import CoreGraphics
import Foundation
import Network
import Security
import SwiftUI
import RemoteCore

struct Macro: Codable, Identifiable, Sendable {
    var id: Int
    var label: String
    var action: String
    var value: String
}

@MainActor
final class RemoteModel: ObservableObject {
    @Published var enabled = false
    @Published var status = "Remote control OFF"
    @Published var lastAction = "None"
    @Published var showPairingSecret = false
    @Published var pairingHost = ""
    @Published var pairingExportStatus = "Choose this Mac’s private LAN IPv4; export an owner-private REMOTE.CFG."
    @Published var macros: [Macro]
    let secret: [UInt8]
    private let server: RemoteServer
    private var lastActionAt = Date.distantPast

    init() {
        secret = KeychainSecret.loadOrCreate()
        if let data = UserDefaults.standard.data(forKey: "macros"),
           let decoded = try? JSONDecoder().decode([Macro].self, from: data),
           decoded.count == 12 {
            macros = decoded
        } else {
            macros = (1...12).map { Macro(id: $0,
                label: "Macro \($0)", action: "Text", value: "") }
        }
        server = RemoteServer()
        server.onEvent = { [weak self] event in
            Task { @MainActor [weak self] in self?.handle(event) }
        }
        server.onStatus = { [weak self] status in
            Task { @MainActor [weak self] in self?.status = status }
        }
    }

    func saveMacros() {
        if let data = try? JSONEncoder().encode(macros) {
            UserDefaults.standard.set(data, forKey: "macros")
        }
    }

    func toggle() {
        if enabled {
            server.stop()
            enabled = false
            status = "Remote control OFF"
        } else {
            guard hasAccessibility else {
                status = "Grant Accessibility permission before enabling remote input"
                requestAccessibility()
                return
            }
            // Enabling requires an explicit click in the Mac UI.
            // Mac listens on port 38947; each session has a fresh challenge
            // and every command requires a 256-bit shared-key HMAC.
            server.start(secret: secret)
            enabled = true
        }
    }

    func requestAccessibility() {
        let opt = ["AXTrustedCheckOptionPrompt": true] as CFDictionary
        _ = AXIsProcessTrustedWithOptions(opt)
    }
    var hasAccessibility: Bool { AXIsProcessTrusted() }
    var pairCode: String { RemoteProtocol.hex(secret) }
    var canExportPairing: Bool { Pairing26.validPrivateIPv4(pairingHost) }

    func exportRemoteConfig() {
        guard canExportPairing else {
            pairingExportStatus = "Enter a valid private Mac IPv4 before exporting."
            return
        }
        let panel = NSOpenPanel()
        panel.title = "Export Cardputer pairing configuration"
        panel.message = "Choose an existing folder, ideally /SUITE/MAC on your private Cardputer SD. Never export to a public/shared folder."
        panel.canChooseFiles = false
        panel.canChooseDirectories = true
        panel.canCreateDirectories = true
        panel.allowsMultipleSelection = false
        guard panel.runModal() == .OK, let folder = panel.url else { return }
        do {
            let output = try Pairing26.saveNew(in: folder,
                                               address: pairingHost, secret: secret)
            pairingExportStatus = "Created REMOTE.CFG in \(output.deletingLastPathComponent().path). Existing files are never overwritten."
        } catch {
            pairingExportStatus = "Export refused: \(error.localizedDescription)"
        }
    }

    private func handle(_ event: RemoteProtocol.Message) {
        guard enabled else { return }
        guard hasAccessibility else {
            status = "Accessibility permission required"
            return
        }
        // Bound unsolicited input rate in case a paired device misbehaves.
        let now = Date()
        if now.timeIntervalSince(lastActionAt) < 0.012 { return }
        lastActionAt = now
        switch event.verb {
        case "MACRO":
            guard let index = Int(event.argument),
                  (1...12).contains(index) else { return }
            let macro = macros[index-1]
            switch macro.action {
            case "Text": postText(macro.value)
            case "Shortcut": postShortcut(macro.value)
            default: return
            }
            lastAction = macro.label
        case "TEXT":
            guard let bytes = RemoteProtocol.bytes(from: event.argument),
                  let string = String(bytes: bytes, encoding: .utf8) else { return }
            postText(string)
            lastAction = "Typed \(string.count) characters"
        case "MOVE":
            let parts = event.argument.split(separator: ",")
            if parts.count == 2, let dx = Double(parts[0]), let dy = Double(parts[1]) {
                moveMouse(dx: dx, dy: dy)
                lastAction = "Pointer moved"
            }
        case "CLICK":
            click(right: event.argument == "R")
            lastAction = event.argument == "R" ? "Right click" : "Left click"
        case "SCROLL":
            if let amount = Int32(event.argument) {
                let mouse = CGEvent(scrollWheelEvent2Source: nil, units: .line,
                                    wheelCount: 1, wheel1: amount, wheel2: 0, wheel3: 0)
                mouse?.post(tap: .cghidEventTap)
                lastAction = "Scroll"
            }
        default: break
        }
    }
    private func postText(_ value: String) {
        // Layout-independent Unicode key text (rather than assuming US key positions).
        guard value.utf16.count <= 512 else { return }
        let units = Array(value.utf16)
        guard !units.isEmpty else { return }
        for down in [true, false] {
            guard let ev = CGEvent(keyboardEventSource: nil,
                                   virtualKey: 0, keyDown: down) else { continue }
            units.withUnsafeBufferPointer { buffer in
                ev.keyboardSetUnicodeString(stringLength: buffer.count,
                                            unicodeString: buffer.baseAddress)
            }
            ev.post(tap: .cghidEventTap)
        }
    }
    private func postShortcut(_ value: String) {
        // Fixed, non-shell shortcut grammar. Example: cmd+shift+4
        let p = value.lowercased().split(separator: "+").map(String.init)
        guard !p.isEmpty, p.count <= 5 else { return }
        var flags = CGEventFlags()
        var key: String?
        for token in p {
            switch token {
            case "cmd", "command": flags.insert(.maskCommand)
            case "ctrl", "control": flags.insert(.maskControl)
            case "alt", "option": flags.insert(.maskAlternate)
            case "shift": flags.insert(.maskShift)
            default:
                guard key == nil else { return }
                key = token
            }
        }
        let keys: [String: CGKeyCode] = [
            "a":0,"s":1,"d":2,"f":3,"h":4,"g":5,"z":6,"x":7,"c":8,
            "v":9,"b":11,"q":12,"w":13,"e":14,"r":15,"y":16,"t":17,
            "1":18,"2":19,"3":20,"4":21,"6":22,"5":23,"=":24,"9":25,
            "7":26,"-":27,"8":28,"0":29,"o":31,"u":32,"i":34,"p":35,
            "l":37,"j":38,"k":40,"n":45,"m":46,
            "return":36,"tab":48,"space":49,"delete":51,"escape":53
        ]
        guard let key, let code = keys[key] else { return }
        for down in [true,false] {
            guard let ev=CGEvent(keyboardEventSource: nil,virtualKey:code,keyDown:down)
            else {continue}
            ev.flags=flags;ev.post(tap:.cghidEventTap)
        }
    }
    private func moveMouse(dx: Double, dy: Double) {
        guard let pos=CGEvent(source: nil)?.location else { return }
        let bounds=CGDisplayBounds(CGMainDisplayID())
        let x=min(max(pos.x+dx,bounds.minX),bounds.maxX-1)
        let y=min(max(pos.y+dy,bounds.minY),bounds.maxY-1)
        let ev=CGEvent(mouseEventSource:nil,mouseType:.mouseMoved,
                       mouseCursorPosition:CGPoint(x:x,y:y),mouseButton:.left)
        ev?.post(tap:.cghidEventTap)
    }
    private func click(right:Bool){
        guard let pos=CGEvent(source:nil)?.location else{return}
        let button:CGMouseButton=right ? .right : .left
        let down:CGEventType=right ? .rightMouseDown : .leftMouseDown
        let up:CGEventType=right ? .rightMouseUp : .leftMouseUp
        CGEvent(mouseEventSource:nil,mouseType:down,mouseCursorPosition:pos,
                mouseButton:button)?.post(tap:.cghidEventTap)
        CGEvent(mouseEventSource:nil,mouseType:up,mouseCursorPosition:pos,
                mouseButton:button)?.post(tap:.cghidEventTap)
    }
}

enum KeychainSecret {
    static let account = "cardputer-remote-p22-v1"
    static let service = "com.crunchradio81.cardputer.remote"
    static func loadOrCreate() -> [UInt8] {
        let query: [String:Any]=[
            kSecClass as String:kSecClassGenericPassword,
            kSecAttrAccount as String:account,
            kSecAttrService as String:service,
            kSecReturnData as String:true,
            kSecMatchLimit as String:kSecMatchLimitOne]
        var existing:CFTypeRef?
        if SecItemCopyMatching(query as CFDictionary,&existing)==errSecSuccess,
           let data=existing as? Data, data.count==32 {return Array(data)}
        var secret=[UInt8](repeating:0,count:32)
        guard SecRandomCopyBytes(kSecRandomDefault,secret.count,&secret)==errSecSuccess
        else {fatalError("Secure random unavailable; pairing refused")}
        var add=query
        add.removeValue(forKey:kSecReturnData as String)
        add.removeValue(forKey:kSecMatchLimit as String)
        add[kSecValueData as String]=Data(secret)
        let outcome=SecItemAdd(add as CFDictionary,nil)
        guard outcome==errSecSuccess else {fatalError("Keychain cannot save remote token")}
        return secret
    }
}

/// Small TCP line-framed service. Network callbacks never issue CGEvents directly.
final class RemoteServer: @unchecked Sendable {
    var onEvent: (@Sendable (RemoteProtocol.Message)->Void)?
    var onStatus: (@Sendable (String)->Void)?
    private let queue=DispatchQueue(label:"CardputerRemoteMac.TCP")
    private var listener:NWListener?
    private var peers:[ObjectIdentifier:NWConnection]=[:]
    private var secret:[UInt8]=[]

    func start(secret key:[UInt8]){
        queue.async { [self] in
            stopInternal()
            secret=key
            guard let port=NWEndpoint.Port(rawValue:RemoteProtocol.port),
                  let server=try? NWListener(using:.tcp,on:port) else {
                onStatus?("Cannot open TCP port 38947")
                return
            }
            listener=server
            server.stateUpdateHandler={ [weak self] state in
                switch state {
                case .ready: self?.onStatus?("Listening on TCP 38947 — opt-in LAN")
                case .failed: self?.onStatus?("Remote listener failed")
                default: break
                }
            }
            server.newConnectionHandler={ [weak self] link in
                self?.accept(link)
            }
            server.start(queue:queue)
        }
    }
    func stop(){queue.async { [self] in stopInternal() }}
    private func stopInternal(){
        for c in peers.values {c.cancel()}
        peers.removeAll()
        listener?.cancel();listener=nil
        secret.removeAll()
    }
    private func accept(_ connection:NWConnection){
        // Fresh, unpredictable per-connection challenge closes cross-session replay.
        var nonce=[UInt8](repeating:0,count:16)
        guard SecRandomCopyBytes(kSecRandomDefault,nonce.count,&nonce)==errSecSuccess
        else {connection.cancel();return}
        let challenge=RemoteProtocol.hex(nonce)
        peers[ObjectIdentifier(connection)]=connection
        connection.stateUpdateHandler={ [weak self] state in
            if case .failed = state {self?.close(connection)}
            if case .cancelled = state {self?.close(connection)}
        }
        connection.start(queue:queue)
        connection.send(content:Data("K132R1 CHALLENGE \(challenge)\n".utf8),
                        completion:.contentProcessed({ [weak self] error in
            if error != nil {self?.close(connection)}
            else {self?.read(connection,challenge:challenge,
                             previous:0,pending:Data())}
        }))
    }
    private func read(_ c:NWConnection,challenge:String,previous:UInt64,pending:Data){
        c.receive(minimumIncompleteLength:1,maximumLength:512) {
            [weak self] bytes,_,complete,error in
            guard let self,error == nil,!complete,let bytes,!bytes.isEmpty
            else {self?.close(c);return}
            var received=pending
            received.append(bytes)
            guard received.count <= 1024 else {self.close(c);return}
            var seq=previous
            while let pos=received.firstIndex(of:10){
                let line=received.prefix(upTo:pos)
                received.removeSubrange(...pos)
                guard let text=String(data:line,encoding:.ascii),
                      let msg=try? RemoteProtocol.decode(
                        line:text,challenge:challenge,
                        secret:self.secret,previousSequence:seq) else {
                    self.onStatus?("Authentication or command rejected")
                    self.close(c)
                    return
                }
                seq=msg.sequence
                self.onEvent?(msg)
            }
            self.read(c,challenge:challenge,previous:seq,pending:received)
        }
    }
    private func close(_ c:NWConnection){
        peers.removeValue(forKey:ObjectIdentifier(c))
        c.cancel()
    }
}

@main
struct CardputerRemoteMacApp: App {
    @StateObject private var model = RemoteModel()
    var body: some Scene {
        WindowGroup("Cardputer Remote Mac") {
            TabView {
                RemoteContentView(model: model)
                    .tabItem {
                        Label("Macros & Mouse", systemImage: "keyboard")
                    }
                LanFileTransferView()
                    .tabItem {
                        Label("LAN File Transfer", systemImage: "arrow.left.arrow.right.square")
                    }
                SDPack27View()
                    .tabItem {
                        Label("Install SD Apps", systemImage: "sdcard")
                    }
            }
            .frame(minWidth: 690, minHeight: 660)
        }
        MenuBarExtra("Cardputer Remote",systemImage:"keyboard") {
            Text(model.enabled ? "LAN remote ON" : "Remote OFF")
            Button(model.enabled ? "Disable immediately" : "Enable remote") {
                model.toggle()
            }
            Button("Quit") {NSApp.terminate(nil)}
        }
    }
}

struct RemoteContentView:View {
    @ObservedObject var model:RemoteModel
    var body:some View {
        VStack(alignment:.leading,spacing:14){
            HStack {
                Text("Cardputer Remote").font(.largeTitle.bold())
                Spacer()
                Text(model.enabled ? "ENABLED" : "DISABLED")
                    .foregroundStyle(model.enabled ? .green : .secondary)
            }
            Text("Native macOS companion • original K132 • keyboard macros + pointer")
                .foregroundStyle(.secondary)
            HStack {
                Button(model.enabled ? "Stop accepting commands" : "Enable remote LAN") {
                    model.toggle()
                }.buttonStyle(.borderedProminent)
                Button("Request Accessibility permission") {
                    model.requestAccessibility()
                }
            }
            Text("Status: \(model.status)")
            Text("Last action: \(model.lastAction)").font(.footnote)
            Divider()
            Text("Pairing").font(.headline)
            Text("On your private Cardputer SD, open /SUITE/MAC (create it if needed). Enter this Mac's private LAN IPv4 below and export REMOTE.CFG to that folder.")
            HStack {
                TextField("This Mac's IPv4 (e.g. 192.168.1.22)",
                          text: $model.pairingHost)
                    .textFieldStyle(.roundedBorder)
                Button("Export REMOTE.CFG…") { model.exportRemoteConfig() }
                    .disabled(!model.canExportPairing)
            }
            Text(model.pairingExportStatus)
                .font(.caption)
                .foregroundStyle(.secondary)
                .textSelection(.enabled)
            Text("The exported file contains the secret. Keep the microSD private, and configure the existing /SUITE/NET/NET.CFG separately for Wi-Fi. Never upload pairing files to GitHub.")
                .font(.caption).foregroundStyle(.orange)
            Toggle("Reveal pairing secret",isOn:$model.showPairingSecret)
            if model.showPairingSecret {
                Text(model.pairCode).font(.system(.caption,design:.monospaced))
                    .textSelection(.enabled)
                Button("Copy pairing secret") {
                    NSPasteboard.general.clearContents()
                    NSPasteboard.general.setString(model.pairCode,forType:.string)
                }
            }
            Text("TCP port 38947. Commands are HMAC-authenticated but not encrypted. Use a trusted LAN only. Stop remote access when finished.")
                .font(.caption).foregroundStyle(.orange)
            Divider()
            Text("Programmable macro keys").font(.headline)
            Text("Text enters Unicode in the current app. Shortcut uses named Mac keys (e.g. cmd+shift+4). No shell commands.")
                .font(.caption)
            ScrollView {
                VStack(spacing:7){
                    ForEach(model.macros.indices,id:\.self){i in
                        HStack {
                            Text(String(format:"%02d",i+1))
                                .font(.system(.callout,design:.monospaced))
                                .frame(width:25)
                            TextField("Label",text:Binding(
                                get:{model.macros[i].label},
                                set:{model.macros[i].label=$0;model.saveMacros()}))
                                .frame(width:110)
                            Picker("Action",selection:Binding(
                                get:{model.macros[i].action},
                                set:{model.macros[i].action=$0;model.saveMacros()})) {
                                Text("Text").tag("Text")
                                Text("Shortcut").tag("Shortcut")
                            }.frame(width:135)
                            TextField("Text or shortcut",text:Binding(
                                get:{model.macros[i].value},
                                set:{model.macros[i].value=String($0.prefix(512));model.saveMacros()}))
                        }
                    }
                }
            }.frame(maxHeight:310)
            Text("The Mac must remain unlocked and Accessibility permission must be granted to this app. Keyboard layout-sensitive shortcuts may differ; Unicode text is preferred for multilingual typing.")
                .font(.caption).foregroundStyle(.secondary)
            Spacer(minLength:0)
        }.padding(18)
    }
}
