// swift-tools-version: 6.0
import Foundation
import PackageDescription

let packageDirectory = Context.packageDirectory

func readPackageVersion() -> String {
  let url = URL(fileURLWithPath: packageDirectory).appendingPathComponent("package.json")
  guard let data = try? Data(contentsOf: url),
    let packageJSON = (try? JSONSerialization.jsonObject(with: data)) as? [String: Any],
    let version = packageJSON["version"] as? String
  else {
    fatalError("[RNAudioWorklets] Cannot read the version from \(url.path)")
  }
  return version
}

/// React Native autolinks every library as a sibling under
/// `<app>/ios/build/generated/autolinking/libs/<SwiftName>`, which is also where the `.package`
/// dependencies below point. react-native-audio-api's target exposes no headers (a module over
/// every engine header is not wanted), so its `<audioapi/...>` headers are reached by path, the
/// SwiftPM counterpart of the podspec's `Headers/Public/RNAudioAPI` search path.
let audioAPIHeadersPath = URL(fileURLWithPath: packageDirectory)
  .deletingLastPathComponent()
  .appendingPathComponent("ReactNativeAudioApi/common/cpp")
  .path

let headerSearchPaths = ["common/cpp", "ios"]
let defines: [(name: String, value: String)] = [
  ("RCT_NEW_ARCH_ENABLED", "1"),
  ("AUDIO_WORKLETS_VERSION", readPackageVersion()),
]

// React Native's headers change class layouts on NDEBUG (without it `DebugStringConvertible`
// gains virtual methods) and the prebuilt React framework is built with NDEBUG in Release, so
// DEBUG/NDEBUG follow the configuration exactly, as in RNAudioAPI and React Native's own targets.
let cSettings: [CSetting] =
  headerSearchPaths.map { .headerSearchPath($0) } + defines.map { .define($0.name, to: $0.value) } + [
    .define("DEBUG", to: "1", .when(configuration: .debug)),
    .define("NDEBUG", .when(configuration: .release)),
    .unsafeFlags(["-I", audioAPIHeadersPath]),
  ]
let cxxSettings: [CXXSetting] =
  headerSearchPaths.map { .headerSearchPath($0) } + defines.map { .define($0.name, to: $0.value) } + [
    .define("DEBUG", to: "1", .when(configuration: .debug)),
    .define("NDEBUG", .when(configuration: .release)),
    .unsafeFlags(["-I", audioAPIHeadersPath]),
  ]

let reactHeaders: [Target.Dependency] = [
  .product(name: "ReactHeaders", package: "ReactNative"),
  .product(name: "ReactNativeHeaders", package: "ReactNative"),
  .product(name: "ReactNativeDependenciesHeaders", package: "ReactNative"),
  .product(name: "ReactAppHeaders", package: "React-GeneratedCode"),
]

// The package and product name is what React Native 0.87 derives from the npm name; 0.88 reads it
// from `swiftpmConfig.name` in package.json, which repeats it.
let package = Package(
  name: "ReactNativeAudioWorklets",
  platforms: [.iOS(.v15)],
  products: [
    .library(name: "ReactNativeAudioWorklets", targets: ["RNAudioWorklets"])
  ],
  dependencies: [
    .package(name: "ReactNative", path: "../../../../xcframeworks"),
    .package(name: "React-GeneratedCode", path: "../../../ios"),
    .package(name: "ReactNativeAudioApi", path: "../ReactNativeAudioApi"),
    .package(name: "RNWorklets", path: "../RNWorklets"),
  ],
  targets: [
    .target(
      name: "RNAudioWorklets",
      dependencies: reactHeaders + [
        .product(name: "ReactNativeAudioApi", package: "ReactNativeAudioApi"),
        .product(name: "RNWorklets", package: "RNWorklets"),
      ],
      path: ".",
      sources: ["common/cpp/audioworklets", "ios/audioworklets"],
      publicHeadersPath: "ios/spm/include",
      cSettings: cSettings,
      cxxSettings: cxxSettings
    )
  ],
  cxxLanguageStandard: .cxx20
)
