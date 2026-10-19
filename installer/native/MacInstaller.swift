import SwiftUI
import AppKit
import UniformTypeIdentifiers

final class InstallerModel: ObservableObject {
    @Published var game: URL?
    @Published var audio: URL?
    @Published var card: URL?
    @Published var progress = 0.0
    @Published var status = "Choose both original PC discs and the Miyoo SD card."
    @Published var busy = false
    @Published var failed = false
    private var process: Process?
    private var buffer = Data()
    private var cancelURL: URL?
    var ready: Bool { game != nil && audio != nil && card != nil && !busy }
    func choose(_ kind: Int) {
        let p = NSOpenPanel()
        p.canChooseDirectories = kind == 2; p.canChooseFiles = kind != 2
        p.allowsMultipleSelection = false
        p.title = kind == 2 ? "Choose the root of the Miyoo SD card" : "Choose an original GTA III disc image"
        if kind != 2 { p.allowedContentTypes = [UTType(filenameExtension: "iso") ?? .data] }
        if p.runModal() == .OK, let u = p.url { assign(u, kind) }
    }
    func assign(_ u: URL, _ kind: Int) {
        guard !busy else { return }
        if kind == 2 { card = u }
        else if u.pathExtension.lowercased() == "iso" { if kind == 0 { game = u } else { audio = u } }
    }
    func acceptDrop(_ providers: [NSItemProvider], kind: Int) -> Bool {
        guard !busy else { return false }
        for (i,p) in providers.enumerated() {
            p.loadItem(forTypeIdentifier: UTType.fileURL.identifier, options: nil) { item,_ in
                var url: URL?
                if let d = item as? Data { url = URL(dataRepresentation: d, relativeTo: nil) }
                else if let u = item as? URL { url = u }
                if let u = url { DispatchQueue.main.async { self.assign(u, kind == 0 && i > 0 ? 1 : kind) } }
            }
        }
        return true
    }
    private func consume(_ data: Data) {
        buffer.append(data)
        while let end = buffer.firstIndex(of: 10) {
            let line = String(decoding: buffer[..<end], as: UTF8.self)
            buffer.removeSubrange(...end)
            let fields = line.components(separatedBy: "\t")
            if fields.count >= 3 && fields[0] == "PROGRESS" {
                progress = Double(fields[1]) ?? progress; status = fields[2]
            } else if fields.count >= 2 && ["ERROR","CANCELLED"].contains(fields[0]) {
                status = fields.dropFirst().joined(separator: " "); failed = fields[0] == "ERROR"
            }
        }
    }
    func start() {
        guard ready, let g = game, let a = audio, let c = card, let resource = Bundle.main.resourceURL else { return }
        let backend = resource.appendingPathComponent("backend")
        let cancel = FileManager.default.temporaryDirectory.appendingPathComponent("gta3-cancel-" + UUID().uuidString)
        let log = FileManager.default.homeDirectoryForCurrentUser.appendingPathComponent("Library/Logs/GTA3 Installer/installation.log")
        cancelURL = cancel; buffer.removeAll(); progress = 0; failed = false; busy = true; status = "Checking the selected files..."
        let p = Process(); let pipe = Pipe()
        p.executableURL = backend.appendingPathComponent("runtime/bin/python3.13")
        p.arguments = ["-u", backend.appendingPathComponent("backend.py").path, "--game", g.path, "--audio", a.path, "--card", c.path, "--cancel-file", cancel.path, "--log", log.path]
        var env = ProcessInfo.processInfo.environment
        env.removeValue(forKey: "PYTHONHOME"); env.removeValue(forKey: "PYTHONPATH"); env["PYTHONNOUSERSITE"] = "1"; env["PYTHONDONTWRITEBYTECODE"] = "1"
        p.environment = env; p.standardOutput = pipe; p.standardError = pipe
        pipe.fileHandleForReading.readabilityHandler = { h in
            let data = h.availableData
            if !data.isEmpty { DispatchQueue.main.async { self.consume(data) } }
        }
        p.terminationHandler = { task in
            DispatchQueue.main.async {
                pipe.fileHandleForReading.readabilityHandler = nil
                self.busy = false; self.process = nil
                if task.terminationStatus == 0 { self.progress = 100; self.status = "Installed. Safely eject the card, then launch GTA III (re3) from Ports." }
                if task.terminationStatus == 3 { self.status = "Installation cancelled. Your existing installation was kept." }
                if task.terminationStatus != 0 && !self.failed && !self.status.hasPrefix("Installation cancelled") {
                    self.failed = true; self.status = "Installation stopped. See Library/Logs/GTA3 Installer/installation.log."
                }
                try? FileManager.default.removeItem(at: cancel)
            }
        }
        process = p
        do { try p.run() }
        catch { busy = false; process = nil; failed = true; status = "Could not start the installer: " + error.localizedDescription }
    }
    func cancel() {
        guard busy, let u = cancelURL else { return }
        try? Data().write(to: u); status = "Cancelling safely. Please keep the card connected..."
    }
}
final class AppDelegate: NSObject, NSApplicationDelegate {
    weak var model: InstallerModel?
    func applicationShouldTerminate(_ sender: NSApplication) -> NSApplication.TerminateReply {
        if model?.busy == true {
            let alert = NSAlert(); alert.messageText = "Installation is still running"
            alert.informativeText = "Cancel the installation and wait for it to stop before closing."
            alert.addButton(withTitle: "Keep open"); alert.addButton(withTitle: "Cancel installation")
            if alert.runModal() == .alertSecondButtonReturn { model?.cancel() }
            return .terminateCancel
        }
        return .terminateNow
    }
    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool { model?.busy != true }
}
struct InstallerView: View {
    @ObservedObject var model: InstallerModel
    private var portVersion: String { Bundle.main.object(forInfoDictionaryKey: "CFBundleShortVersionString") as? String ?? "unknown" }
    func field(_ title: String, _ subtitle: String, _ value: URL?, _ kind: Int) -> some View {
        VStack(alignment: .leading, spacing: 5) {
            Text(title).font(.headline)
            HStack {
                Text(value?.path ?? subtitle).lineLimit(2).truncationMode(.middle).foregroundColor(value == nil ? .secondary : .primary).frame(maxWidth: .infinity, alignment: .leading)
                    .padding(11).background(Color.primary.opacity(0.045)).cornerRadius(7)
                    .onDrop(of: [UTType.fileURL.identifier], isTargeted: nil) { model.acceptDrop($0, kind: kind) }
                Button("Choose…") { model.choose(kind) }.disabled(model.busy)
            }
        }
    }
    var body: some View {
        VStack(alignment: .leading, spacing: 19) {
            HStack(alignment: .top) {
                Image(systemName: "externaldrive.badge.plus").font(.system(size: 32)).foregroundColor(.accentColor)
                VStack(alignment: .leading, spacing: 4) {
                    Text("GTA III Installer").font(.title).bold()
                    Text("Miyoo Mini · OnionOS · Port \(portVersion)").foregroundColor(.secondary)
                }
            }
            Text("Prepare your own classic PC game and install it on the Miyoo SD card. Nothing is downloaded.").font(.subheadline)
            field("Game disc", "Drop GTAIII.iso here", model.game, 0)
            field("Audio disc", "Drop GTA3_audio.iso here", model.audio, 1)
            field("Miyoo SD card", "Choose the card itself, not a folder inside it", model.card, 2)
            VStack(alignment: .leading, spacing: 8) {
                ProgressView(value: model.progress, total: 100)
                Text(model.status).font(.callout).foregroundColor(model.failed ? .red : .secondary).fixedSize(horizontal: false, vertical: true).frame(minHeight: 35, alignment: .topLeading)
            }
            HStack {
                Text("Existing saves and settings are kept.").font(.caption).foregroundColor(.secondary)
                Spacer()
                if model.busy { Button("Cancel") { model.cancel() } }
                Button("Prepare and install") { model.start() }.keyboardShortcut(.defaultAction).disabled(!model.ready)
            }
        }.padding(28).frame(width: 620)
    }
}
@main struct GTAInstallerApp: App {
    @NSApplicationDelegateAdaptor(AppDelegate.self) var delegate
    @StateObject private var model = InstallerModel()
    var body: some Scene {
        WindowGroup { InstallerView(model: model).onAppear { delegate.model = model; NSApp.activate(ignoringOtherApps: true) } }
            .windowStyle(.hiddenTitleBar).windowResizability(.contentSize)
    }
}
