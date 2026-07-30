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
- **iOS**: by podspec `prepare_command` during `pod install` **and** by a `script_phase` (`[CP-User] Download RNAudioAPI prebuilt binaries`, `:before_headers`, always-out-of-date) on every Xcode build — downloads `ffmpeg_ios`, `iphoneos`, `iphonesimulator`, `macosx`. `prepare_command` ensures all vendored `.xcframework`s exist when CocoaPods generates `[CP] Copy XCFrameworks` (required for correct FFmpeg linking). The build-time phase handles CI setups that cache `Pods/` independently from `node_modules/` — it is a fast no-op when binaries are already present. **It must use `:before_headers`, not `:before_compile`** — CocoaPods inserts its own `[CP] Copy XCFrameworks` phase right after `Headers`; `:before_compile` would land *after* it. The phase's `output_files` declares both the `-force_load` static libs and the four FFmpeg xcframeworks. The whole phase is skipped only when *both* `DISABLE_AUDIOAPI_STATIC_EXTERNAL_LIBS` and `DISABLE_AUDIOAPI_FFMPEG` are set.
- **Android**: by `downloadPrebuiltBinaries` Gradle task, which runs before `preBuild` — downloads `android`, `jniLibs`

The script is idempotent — it skips any archive whose destination directory already exists, so the per-build invocations are fast no-ops once binaries are present.

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
- `android/fix-prefab.gradle` — prefab publication workaround (ported from react-native-worklets)
- `android/CMakeLists.txt` — Android CMake root (SIMD detection, RN version flags, delegates to subdirectory)
- `android/src/main/cpp/audioapi/CMakeLists.txt` — actual build target (sources, prebuilt libs, include paths)
- `common/cpp/audioapi/EXTENSION_API.md` — stable C++ extension contract for dependent native modules
- `common/cpp/audioapi/compatibility/StableAPI.h` — single public C++ compatibility header for extensions

### Key behaviors
- Feature flags (`newArchEnabled`, `disableAudioapiFFmpeg`) are read from app's `gradle.properties` and forwarded to both CMake and Kotlin `BuildConfig`
- DSP sources always compiled with `-O3` regardless of overall build type
- Sources gathered with `GLOB_RECURSE CONFIGURE_DEPENDS` — CMake re-runs automatically when files are added/removed
- 16KB page size alignment enabled for Android 15+
- **Extension API (prefab)**: single public C++ header `<audioapi/compatibility/StableAPI.h>`; prefab publishes transitive headers needed to compile it (`prepareAudioApiHeadersForPrefabs`); `fix-prefab.gradle` ensures the `.so` is in prefab metadata. Contract: `EXTENSION_API.md`
- CMake exposes `COMMON_CPP_DIR` and `ANDROID_CPP_DIR` as **PUBLIC** include dirs so prefab consumers resolve `<audioapi/...>`

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
- JSI internals use distinct names (`HostObject`, `RuntimeInstanceCache`, `RuntimeObserver`) instead of generic names like `JsiHostObject` / `RuntimeAwareCache` / `RuntimeLifecycleMonitor` — `react-native-skia` publishes headers with those generic names and CocoaPods can resolve the wrong file when both libraries are in the same app

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

## Local Full Validation

CI intentionally skips native Android/iOS builds (expensive). Use the tiered local validation script for pre-PR checks:

```bash
yarn validate:fast      # CI parity (format, lint, typecheck, enum sync, build, C++ + JS tests)
yarn validate:graph     # graph tests + ASan (optional; graph path changes)
yarn validate:android   # yarn workspace … build:android
yarn validate:ios       # yarn workspace … build:ios (macOS only)
yarn validate:full      # --fast + --android + --ios
```

Script: [`scripts/validate.sh`](../../../scripts/validate.sh) at monorepo root.

### What CI covers vs what validation tiers add

| Layer | CI (`ci.yml` + `tests.yml`) | Local tiers |
|---|---|---|
| TS build (`bob build`) | Yes | `--fast` |
| C++ test subset (`RunTests.sh`) | Yes | `--fast` |
| C++ coverage (`RunCoverage.sh`, Clang) | Yes (`cpp-coverage` artifact) | `yarn test:cpp:coverage` |
| Jest | Yes | `--fast` |
| Graph tests | No, path-filtered in `graph-tests.yml` | `--graph` |
| HostObjects (26 JSI `.cpp` files) | **No** | `--android` + `--ios` |
| Android JNI C++ + Kotlin | **No** | `--android` |
| iOS ObjC++ | **No** | `--ios` |
| Prebuilt libs (Opus, FFmpeg, etc.) | **No** | `--android` / `--ios` (via `download-prebuilt-binaries.sh`) |
| TurboModule codegen, worklets linking | **No** | `--android` / `--ios` |

The C++ test build excludes HostObjects, `AudioContext.cpp`, `FFmpegDecoding.cpp`, and worklet nodes — see [C++ Tests](#c-tests-standalone-build) below. Passing `yarn test` alone does not prove native layers compile.

### Shared prebuild phase (before native tiers)

Before `--android` or `--ios`, the script runs once:

1. `yarn install --immutable` + `yarn build`
2. [`download-prebuilt-binaries.sh`](../../../packages/react-native-audio-api/scripts/download-prebuilt-binaries.sh) `{android|ios}` (must run from `packages/react-native-audio-api/scripts/`)
3. `yarn workspace react-native-audio-api test:cpp`

Android (NDK) and iOS (Clang) cannot share object files — reuse is at the prebuild/download level. If `ccache` is installed, `validate.sh` wraps host `CC`/`CXX`; only the host C++ tests (`test:cpp`) honor that — Gradle/NDK and `xcodebuild` use their own toolchains and are unaffected.

### Platform skip behavior

- `--ios` on Linux → skip with message (exit 0)
- `--android` without `ANDROID_HOME` → fail on explicit `--android`; skip with warning inside `--full`
- Graph tests are separate from `--full` (slow; CI path-filters them)

### Which tier to run

See the decision table in [post-work-checks](../post-work-checks/SKILL.md).

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

### Coverage (Clang / llvm-cov)

```bash
yarn workspace react-native-audio-api test:cpp:coverage
# open packages/react-native-audio-api/common/cpp/test/coverage-html/index.html
```

`RunCoverage.sh` configures a separate `build-coverage/` tree with `-DENABLE_COVERAGE=ON` (Clang-only LLVM source-based coverage: `-fprofile-instr-generate -fcoverage-mapping`), defaults `CC`/`CXX` to `clang`/`clang++` when unset, runs the same gtest filter as `RunTests.sh`, then prints `llvm-cov report` and writes HTML via `llvm-cov show -format=html`. When `GITHUB_STEP_SUMMARY` is set, the report is also appended there. Sanitizer targets are skipped when coverage is enabled. Requires Apple Clang / `xcrun llvm-profdata` and `xcrun llvm-cov` on macOS (or the same tools on PATH for Linux).

CI runs a parallel `cpp-coverage` job via `.github/workflows/cpp-coverage-job.yml` (called from `tests.yml` on pull requests; Clang + LLVM apt packages, separate from the GCC `cpp-tests` job). It uploads the HTML tree as the `cpp-coverage-html` artifact (14-day retention); download the zip from the Actions run and open `index.html`.

> **Generated build trees must be named `build*`.** The C++ linters walk the filesystem with `find` and never consult git, so a `.gitignore` entry does not keep generated sources out of them. Exclusion happens by directory name in two places that must stay in sync: `**/build*/**` in `.clang-format-ignore` (used by `format:check:common`) and `-type d -name 'build*' -prune` in `scripts/cpplint.sh`. A CMake binary directory outside that prefix makes the pre-commit hook fail on generated files such as `CMakeFiles/*/CompilerIdCXX/CMakeCXXCompilerId.cpp`. CI never hits this because it checks out a clean tree.

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
