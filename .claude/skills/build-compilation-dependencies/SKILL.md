---
name: build-compilation-dependencies
description: >
  Build system overview for react-native-audio-api across all platforms — CMakeLists.txt,
  android/build.gradle, RNAudioAPI.podspec, prebuilt external libraries, and the standalone
  C++ test build. Use this skill when adding a new source file, modifying CMakeLists or podspec,
  debugging compilation errors, integrating a new dependency, or understanding why includes work
  differently in tests vs the app. Trigger phrases: "add source file", "CMakeLists", "podspec",
  "build.gradle", "prebuilt binaries", "FFmpeg disabled", "pod install", "new architecture",
  "compile error", "undefined symbol", "SIMD", "worklets build flag", "C++ tests", "conditional
  compilation", "include path", "gradle build fails", "link error".
---

# Skill: Build, Compilation & Dependencies

For deep CMake/Gradle/podspec analysis see [build-details.md](build-details.md).

---

## Repository Build Overview

```
react-native-audio-api/
├── package.json                        # Yarn 4 workspaces root
├── packages/react-native-audio-api/    # Main library
│   ├── android/
│   │   ├── build.gradle                # Android build config (Gradle)
│   │   ├── CMakeLists.txt              # Android CMake root (delegates to subdirectory)
│   │   └── src/main/cpp/audioapi/
│   │       └── CMakeLists.txt          # Actual Android C++ build target
│   ├── common/cpp/audioapi/            # Shared C++ (used by all platforms)
│   │   └── external/                   # Prebuilt binaries per platform
│   │       ├── android/                # .a static libs (Opus, Ogg, Vorbis, OpenSSL)
│   │       ├── iphoneos/               # iOS device .a libs
│   │       ├── iphonesimulator/        # iOS sim .a libs
│   │       ├── macosx/                 # macOS .a libs
│   │       ├── ffmpeg_ios/             # FFmpeg .xcframeworks (iOS only)
│   │       ├── include/                # Headers for Opus/Ogg/Vorbis/OpenSSL
│   │       └── include_ffmpeg/         # Headers for FFmpeg
│   ├── common/cpp/test/
│   │   ├── CMakeLists.txt              # Standalone test build (no Android/iOS)
│   │   ├── RunTests.sh                 # Test runner script
│   │   └── src/                        # Google Test files
│   ├── RNAudioAPI.podspec              # CocoaPods spec for iOS
│   └── scripts/
│       ├── download-prebuilt-binaries.sh  # Downloads externals from GitHub Releases
│       ├── rnaa_utils.rb               # Ruby helpers for podspec (path resolution, worklets check)
│       └── validate-worklets-version.js
└── apps/
    └── fabric-example/
        └── ios/
            └── Podfile                 # Consumer Podfile (new arch enabled)
```

---

## Prebuilt Binaries

External libraries (Opus, Ogg, Vorbis, OpenSSL, FFmpeg) are **not compiled from source** — they are downloaded as prebuilt `.a` / `.so` / `.xcframework` archives from:

```
https://github.com/software-mansion-labs/rn-audio-libs/releases/download/<TAG>/
```

Current tag: **v3.0.0** (see `scripts/download-prebuilt-binaries.sh`).

The download script is triggered automatically:
- **iOS**: by podspec `prepare_command` during `pod install`
- **Android**: by `downloadPrebuiltBinaries` Gradle task, which runs before `preBuild`

Downloaded artifacts land in:
- `common/cpp/audioapi/external/android/<ABI>/` — `.a` static libs for Android ABIs
- `android/src/main/jniLibs/<ABI>/` — FFmpeg `.so` shared libs for Android (loaded at runtime)
- `common/cpp/audioapi/external/ffmpeg_ios/` — FFmpeg `.xcframework` files for iOS
- `common/cpp/audioapi/external/iphoneos/` / `iphonesimulator/` / `macosx/` — Opus/Ogg/etc `.a`

**These directories are gitignored.** If they're missing, run `pod install` (iOS) or Gradle build (Android) to re-download them. Do not commit them.

---

## Android Build — high-level summary

### Files
- `android/build.gradle` — Gradle library config
- `android/CMakeLists.txt` — Android CMake root (SIMD detection, RN version flags, delegates to subdirectory)
- `android/src/main/cpp/audioapi/CMakeLists.txt` — actual build target (sources, prebuilt libs, include paths)

### Key behaviors
- Feature flags (`newArchEnabled`, `disableAudioapiFFmpeg`) are read from app's `gradle.properties` and forwarded to both CMake and Kotlin `BuildConfig`
- DSP sources always compiled with `-O3` regardless of overall build type
- Sources gathered with `GLOB_RECURSE CONFIGURE_DEPENDS` — CMake re-runs automatically when files are added/removed
- Worklets must be merged before the audio API CMake build starts (explicit Gradle task dependency)
- 16KB page size alignment enabled for Android 15+

For full per-line analysis see [build-details.md](build-details.md#android-androidcmakeliststxt-root--detailed-analysis).

---

## iOS Build (CocoaPods) — high-level summary

### Files
- `RNAudioAPI.podspec` — library spec with subspecs
- `scripts/rnaa_utils.rb` — Ruby helpers called by podspec
- `apps/fabric-example/ios/Podfile` — consumer

### Key behaviors
- Four subspecs split compilation: `audioapi` (core C++), `audioapi/ios` (ObjC++), `audioapi/audioapi_dsp` (DSP with `-O3`), `audioapi/miniaudio_impl` (compiled as `-x objective-c++`)
- Static prebuilt libs linked with `-force_load` to prevent dead-stripping
- FFmpeg xcframeworks listed in `s.ios.vendored_frameworks` — CocoaPods handles embedding and signing
- `Accelerate` framework linked, enabling `HAVE_ACCELERATE=1` for vDSP SIMD on iOS
- Header search paths split: `pod_target_xcconfig` (library compilation) vs `xcconfig` (app consumers)
- `rnaa_utils.rb` resolves dynamic paths at `pod install` time (not hardcoded)

For full per-line analysis see [build-details.md](build-details.md#ios-rnaudioapipodspec--detailed-analysis).

---

## Building the Apps

### iOS (fabric-example)

```bash
# From the monorepo root first:
yarn install

# Then install pods — must be done from the ios/ directory:
cd apps/fabric-example/ios
pod install

# Run the app (from repo root):
yarn workspace fabric-example ios
# or open Xcode:
open apps/fabric-example/ios/FabricExample.xcworkspace
```

**When to re-run `pod install`**:
- After `yarn install` (any dependency change)
- After changing `RNAudioAPI.podspec`
- After adding/removing iOS source files that need to be picked up
- After changing `rnaa_utils.rb` or `scripts/validate-worklets-version.js`
- When prebuilt binaries need to be re-downloaded (podspec `prepare_command` runs on `pod install`)

**Disable FFmpeg on iOS**:
```bash
DISABLE_AUDIOAPI_FFMPEG=1 pod install
```

### Android (fabric-example)

```bash
yarn workspace fabric-example android
# or open in Android Studio:
open apps/fabric-example/android
```

**Disable FFmpeg on Android**: set in `android/gradle.properties`:
```
disableAudioapiFFmpeg=true
```

**Clean CMake cache** (fixes most mysterious native build failures):
```bash
yarn workspace react-native-audio-api clean  # or manually:
rm -rf packages/react-native-audio-api/android/.cxx
```

---

## C++ Tests (standalone build)

### Location
`packages/react-native-audio-api/common/cpp/test/`

### How to run
```bash
yarn test   # from monorepo root — runs RunTests.sh
```

`RunTests.sh` does:
```bash
cd packages/react-native-audio-api/common/cpp/test
cmake -S . -B build -Wno-dev
cd build && make -j10
./tests --gtest_print_time=1
```

The `build/` directory is deleted after each run.

### Key design decisions
- Completely standalone — no Gradle, no Xcode, no prebuilt Android libraries needed
- Sources resolved from `node_modules` (symlinked to `packages/` in yarn workspaces)
- HostObjects, worklets nodes, AudioContext, and FFmpegDecoding are excluded from the test build
- Compile definitions: `RN_AUDIO_API_ENABLE_WORKLETS=0`, `RN_AUDIO_API_TEST=1`, `RN_AUDIO_API_FFMPEG_DISABLED=1`
- Google Test auto-fetched via `FetchContent` if not installed locally
- New test files in `test/src/**/*.cpp` are picked up automatically by glob — no CMakeLists edit needed

For `MockAudioEventHandlerRegistry`, `TestableXxx` pattern, and full CMakeLists analysis see [build-details.md](build-details.md#c-test-build--commoncpptestcmakeliststxt--detailed-analysis).

---

## Conditional Compilation Flags Summary

| Flag | Android (CMake) | iOS (podspec) | Tests |
|---|---|---|---|
| `RN_AUDIO_API_FFMPEG_DISABLED` | `-DRN_AUDIO_API_FFMPEG_DISABLED` | `-DRN_AUDIO_API_FFMPEG_DISABLED=1` | Always set to 1 |
| `RN_AUDIO_API_ENABLE_WORKLETS` | `-DRN_AUDIO_API_ENABLE_WORKLETS=1/0` | `-DRN_AUDIO_API_ENABLE_WORKLETS=1` | Always set to 0 |
| `RCT_NEW_ARCH_ENABLED` | `-DRCT_NEW_ARCH_ENABLED` | `-DRCT_NEW_ARCH_ENABLED` | Not set |
| `HAVE_ARM_NEON_INTRINSICS` | Set by CMake SIMD detection | Set by Xcode/Clang for arm64 | Set by CMake SIMD detection |
| `HAVE_X86_SSE2` | Set by CMake SIMD detection | Not used | Set by CMake SIMD detection |
| `HAVE_ACCELERATE` | Not set | `GCC_PREPROCESSOR_DEFINITIONS` | Not set |
| `RN_AUDIO_API_TEST` | Not set | Not set | Always set to 1 |

---

## Common Build Failure Patterns

| Symptom | Likely cause | Fix |
|---|---|---|
| `file not found: libopus.a` | Prebuilt binaries not downloaded | Run `pod install` (iOS) or Gradle build (triggers download task) |
| `No such module 'RNAudioAPI'` | Pod not installed | `cd apps/fabric-example/ios && pod install` |
| `undefined symbol: av_*` | FFmpeg .so not in jniLibs | Build triggers download; verify `disableAudioapiFFmpeg` not set unexpectedly |
| CMake error on clean build | Stale `.cxx` cache | `rm -rf packages/react-native-audio-api/android/.cxx` |
| Test build: `Cannot open include file: audioapi/...` | Node modules not linked | `yarn install` from root, then re-run tests |
| New `.cpp` not compiled in tests | Glob picks it up automatically — may need cmake reconfigure | Delete `test/build/` and re-run |
| iOS compile error `unknown type 'id'` | C++ file included ObjC-only header | Compile that file as ObjC++ (separate subspec with `-x objective-c++`) |
| `RCT_NEW_ARCH_ENABLED` undefined on Android | Old RN gradle plugin | Ensure `newArchEnabled=true` in app's `gradle.properties` |

---

*Maintenance: see [maintenance.md](maintenance.md).*
