// swift-tools-version: 6.0

import Foundation
import PackageDescription

let packageDirectory = Context.packageDirectory
let npmPackageName = "react-native-audio-api"

// Prebuilt libraries, one zipped xcframework per library.
let prebuiltBinariesTag = "v5.0.0"
let prebuiltBinariesURL =
  "https://github.com/software-mansion-labs/rn-audio-libs/releases/download/\(prebuiltBinariesTag)"
let staticExternalLibChecksums: KeyValuePairs = [
  "libopusfile": "f3a421f51c9cb317981cf75e60595280eb59033283ddaa1dc64da515f0a49d3d",
  "libopus": "791b19106e8e43c46e35fd29bbf3a23a5bdaa717f2ba53bd40889a0b68d57a9c",
  "libogg": "3e0113d1d7d31103479b317f58f14369ea560cf5dd78de7bf0a4a3b7439b90f7",
  "libvorbis": "8f6a77bc683d8a491a79c2dc2b91c3ac23d655e06d0b5693a399364df741ccbf",
  "libvorbisenc": "d4477f7ca8fd546f5eaa23329b6f3425871798caaf34e4e59f29a844f89854d2",
  "libvorbisfile": "04b62f2a52158f9725bd570b1521d265ef9777d3bc8525e8a25b7ca683cb6e18",
]
let ffmpegChecksums: KeyValuePairs = [
  "libavcodec": "2a9a4cfa36173729d35f87fbc3ca289bd8273e307e4b0b2a9b5df87812bf13fd",
  "libavformat": "a4d3c83752be1c5e9b81173d1ad4422b3366b37e21200c79c68df932fb0069bd",
  "libavutil": "2bef1fd43b934dd873efeca1f48183cd5e7158a63269e8280a4435c31ded033e",
  "libswresample": "1c79fed39eae639f0c76c18e922ddc6980ca53dc5816bc305624645a0e7a6bd6",
  "FFmpegHeaders": "d37a50aae9b94239fb295e274a04d1e4e4dfb2f56680316250a403c3eb0365f8",
]

func readJSONObject(at url: URL) -> [String: Any]? {
  guard let data = try? Data(contentsOf: url) else { return nil }
  return (try? JSONSerialization.jsonObject(with: data)) as? [String: Any]
}

/// The build options: the `"react-native-audio-api"` section of the nearest app package.json above
/// this package, e.g. `"react-native-audio-api": { "enableFFmpeg": true, "disableStaticExternalLibs": false, "disableAudioRecorder": false }`.
/// Bare apps write the section by hand; in Expo apps `expo prebuild` writes the two older options from
/// the (deprecated) plugin options.
/// React Native evaluates this manifest through its autolinking symlink
/// (`<app>/ios/build/generated/autolinking/libs/ReactNativeAudioApi`), so walking up reaches the app through
/// its `ios/` directory wherever the package manager placed this package.
func readAppConfiguration() -> [String: Any] {
  var directory = URL(fileURLWithPath: packageDirectory).deletingLastPathComponent()
  while directory.path != "/" {
    let packageJSON = readJSONObject(at: directory.appendingPathComponent("package.json"))
    if let packageJSON, packageJSON["name"] as? String != npmPackageName,
      let configuration = packageJSON[npmPackageName] as? [String: Any]
    {
      return configuration
    }
    directory.deleteLastPathComponent()
  }
  return [:]
}

func prebuiltBinaryTargets(_ checksums: KeyValuePairs<String, String>) -> [Target] {
  checksums.map { name, checksum in
    .binaryTarget(
      name: name, url: "\(prebuiltBinariesURL)/\(name).xcframework.zip", checksum: checksum)
  }
}

/// `false` when the key is absent. Any value other than a JSON boolean stops resolution: an
/// option that is silently ignored would build the wrong configuration.
func booleanOption(_ key: String, in configuration: [String: Any]) -> Bool {
  guard let value = configuration[key] else { return false }
  // JSONSerialization bridges JSON booleans and numbers alike to NSNumber.
  if let number = value as? NSNumber, CFGetTypeID(number) == CFBooleanGetTypeID() {
    return number.boolValue
  }
  let shownValue = value is String ? "the string \"\(value)\"" : "\(value)"
  fatalError("[RNAudioAPI] \"\(key)\" must be true or false, got \(shownValue)")
}

let appConfiguration = readAppConfiguration()
let isFFmpegEnabled = booleanOption("enableFFmpeg", in: appConfiguration)
let areStaticExternalLibsDisabled = booleanOption("disableStaticExternalLibs", in: appConfiguration)
let isAudioRecorderDisabled = booleanOption("disableAudioRecorder", in: appConfiguration)

var headerSearchPaths = ["common/cpp", "ios", "common/cpp/audioapi/external/include"]
var defines: [(name: String, value: String)] = [
  ("HAVE_ACCELERATE", "1"),
  ("RCT_NEW_ARCH_ENABLED", "1"),
]
var binaryTargets: [Target] = []
var mainTargetExcludes = [
  "common/cpp/audioapi/dsp",
  "common/cpp/audioapi/external",
  // including them prints warning in the xcode so we exclude them from the main target
  "common/cpp/audioapi/compatibility/EXTENSION_API.md",
  "common/cpp/audioapi/core/utils/graph/README.md",
  "common/cpp/audioapi/libs/ffmpeg/relinking.md",
  // Compiled through the wrappers in ios/spm.
  "common/cpp/audioapi/utils/MiniaudioImplementation.cpp",
  "common/cpp/audioapi/utils/AudioThreadGuard.cpp",
]

if areStaticExternalLibsDisabled {
  defines += [
    ("RN_AUDIO_API_STATIC_EXTERNAL_LIBS_DISABLED", "1"),
    ("MA_NO_LIBOPUS", "1"),
    ("MA_NO_LIBVORBIS", "1"),
  ]
} else {
  headerSearchPaths += [
    "common/cpp/audioapi/external/include/opus",
    "common/cpp/audioapi/external/include/vorbis",
  ]
  binaryTargets += prebuiltBinaryTargets(staticExternalLibChecksums)
}

if isAudioRecorderDisabled {
  defines.append(("RN_AUDIO_API_AUDIO_RECORDER_DISABLED", "1"))
  mainTargetExcludes += [
    "common/cpp/audioapi/HostObjects/inputs/AudioRecorderHostObject.cpp",
    "ios/audioapi/ios/core/IOSRecorderInput.mm",
  ]
}

if isFFmpegEnabled {
  defines.append(("RN_AUDIO_API_FFMPEG_ENABLED", "1"))
  binaryTargets += prebuiltBinaryTargets(ffmpegChecksums)
} else {
  mainTargetExcludes.append("common/cpp/audioapi/decoding/backends/FfmpegDecoder.cpp")
}

// React Native's headers change class layouts on NDEBUG (without it `DebugStringConvertible`
// gains virtual methods) and the prebuilt React framework is built with NDEBUG in Release, so
// DEBUG/NDEBUG follow the configuration exactly, as in React Native's own generated targets.
let cSettings: [CSetting] =
  headerSearchPaths.map { .headerSearchPath($0) } + defines.map { .define($0.name, to: $0.value) } + [
    .define("DEBUG", to: "1", .when(configuration: .debug)),
    .define("NDEBUG", .when(configuration: .release)),
  ]
let cxxSettings: [CXXSetting] =
  headerSearchPaths.map { .headerSearchPath($0) } + defines.map { .define($0.name, to: $0.value) } + [
    .define("DEBUG", to: "1", .when(configuration: .debug)),
    .define("NDEBUG", .when(configuration: .release)),
  ]

let reactHeaders: [Target.Dependency] = [
  .product(name: "ReactHeaders", package: "ReactNative"),
  .product(name: "ReactNativeHeaders", package: "ReactNative"),
  .product(name: "ReactNativeDependenciesHeaders", package: "ReactNative"),
  .product(name: "ReactAppHeaders", package: "React-GeneratedCode"),
]

let package = Package(
  name: "ReactNativeAudioApi",
  platforms: [.iOS(.v15)],
  products: [
    .library(name: "ReactNativeAudioApi", targets: ["RNAudioAPI"])
  ],
  dependencies: [
    .package(name: "ReactNative", path: "../../../../xcframeworks"),
    .package(name: "React-GeneratedCode", path: "../../../ios"),
  ],
  targets: [
    .target(
      name: "RNAudioAPI",
      dependencies: reactHeaders + ["RNAudioAPIDSP"] + binaryTargets.map { .target(name: $0.name) },
      path: ".",
      exclude: mainTargetExcludes,
      sources: ["common/cpp/audioapi", "ios/audioapi", "ios/spm/sources"],
      publicHeadersPath: "ios/spm/include",
      cSettings: cSettings,
      cxxSettings: cxxSettings,
      linkerSettings: [
        .linkedFramework("Accelerate"),
        .linkedFramework("AVFoundation"),
        .linkedFramework("AudioToolbox"),
        .linkedFramework("CoreGraphics"),
        .linkedFramework("Foundation"),
        .linkedFramework("ImageIO"),
        .linkedFramework("MediaPlayer"),
        .linkedFramework("UIKit"),
      ]
    ),
    // The podspec compiles the DSP sources with -O3 in every configuration; SwiftPM has no
    // per-file flags, so they get their own target.
    .target(
      name: "RNAudioAPIDSP",
      dependencies: reactHeaders,
      path: ".",
      exclude: [
        "common/cpp/audioapi/dsp/r8brain/CDSPHBDownsampler.inc",
        "common/cpp/audioapi/dsp/r8brain/CDSPHBUpsampler.inc",
      ],
      sources: ["common/cpp/audioapi/dsp"],
      publicHeadersPath: "ios/spm/dsp-include",
      cSettings: cSettings + [.unsafeFlags(["-O3"])],
      cxxSettings: cxxSettings + [.unsafeFlags(["-O3"])],
      linkerSettings: [.linkedFramework("Accelerate")]
    ),
  ] + binaryTargets,
  cxxLanguageStandard: .cxx20
)
