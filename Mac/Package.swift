// swift-tools-version: 6.0
import PackageDescription
let package = Package(
    name: "CardputerRemoteMac",
    platforms: [.macOS(.v14)],
    products: [.executable(name: "CardputerRemoteMac", targets: ["CardputerRemoteMac"])],
    targets: [
        .target(name: "RemoteCore", path: "Sources/RemoteCore"),
        .executableTarget(name: "CardputerRemoteMac",
            dependencies: ["RemoteCore"], path: "Sources/CardputerRemoteMac"),
        .testTarget(name: "RemoteCoreTests",
            dependencies: ["RemoteCore"], path: "Tests/RemoteCoreTests")
    ]
)
