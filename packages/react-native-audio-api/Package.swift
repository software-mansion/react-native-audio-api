// swift-tools-version: 6.0

import Foundation
import PackageDescription

let packageDirectory = Context.packageDirectory
let npmPackageName = "react-native-audio-api"

// Prebuilt libraries, one zipped xcframework per library.
let prebuiltBinariesTag = "v4.0.0"
let prebuiltBinariesURL =
  "https://github.com/software-mansion-labs/rn-audio-libs/releases/download/\(prebuiltBinariesTag)"
let staticExternalLibChecksums: KeyValuePairs = [
  "libopusfile": "c6ca1cd533cf91e2f8ff6e8636a6f9a5282a2548cc4a27d19b03c843b2ba8d09",
  "libopus": "5a25f6633c2b95f9e5faef96b17b5f23ffeabe66883608c1d72ca091ff7f54cf",
  "libogg": "b1bd6d27cac2ea188f2d34339d52afe5e71715f03c73bb6db277b5cd02828460",
  "libvorbis": "8923c7292b71993e0960c39aa29dc90bfd5b8cdf24b070ca1e33eb782cd52508",
  "libvorbisenc": "185f50deed4642e5f5b919e4654d281b61e310e0c7228c87cd6c8e13dc35e3b5",
  "libvorbisfile": "48594352908306f3c828aa06b53d774b123b745f63e4a7c3d75d23e4fcf00aca",
]
let ffmpegChecksums: KeyValuePairs = [
  "libavcodec": "48421a2a2ba940d8099d48788709860294e8b4dd581219454af80ee63632e4aa",
  "libavformat": "b5bd67c71301967d2b6a2cf86efd5bd0cd18ab7f97feeec07cd32241792bdf92",
  "libavutil": "9b21830f970d03dc1082ba29890c197a1970b9ce7532d3dc170db7956497a4d5",
  "libswresample": "0f89341a856d167ac89ce04f0d9ecec421b458b551a6beccb835c658a281c082",
  "FFmpegHeaders": "ffb54a6d111569c37f98f0ff314e9c1546e2327ab6444830ab9929d8b41b07ff",
]

func readJSONObject(at url: URL) -> [String: Any]? {
  guard let data = try? Data(contentsOf: url) else { return nil }
  return (try? JSONSerialization.jsonObject(with: data)) as? [String: Any]
}

/// The build options: the `"react-native-audio-api"` section of the nearest app package.json above
/// this package, e.g. `"react-native-audio-api": { "enableFFmpeg": true, "disableStaticExternalLibs": false }`.
/// Bare apps write the section by hand; in Expo apps `expo prebuild` writes it from the plugin options.
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
