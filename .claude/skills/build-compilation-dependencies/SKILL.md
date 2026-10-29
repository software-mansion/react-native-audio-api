---
name: build-compilation-dependencies
description: >
  Build system overview for react-native-audio-api across all platforms — CMakeLists.txt,
  android/build.gradle, RNAudioAPI.podspec, Package.swift (SwiftPM), prebuilt external libraries, and the standalone
  C++ test build. Use this skill when adding a new source file, modifying CMakeLists or podspec,
  debugging compilation errors, integrating a new dependency, or understanding why includes work
  differently in tests vs the app. Trigger phrases: "add source file", "CMakeLists", "podspec",
  "build.gradle", "prebuilt binaries", "FFmpeg disabled", "pod install", "new architecture",
  "compile error", "undefined symbol", "SIMD", "worklets build flag", "C++ tests", "conditional
  compilation", "include path", "gradle build fails", "link error", "yarn catalog", "catalog:", "add a dependency",
  "bump a dependency", "yarn workspaces", "npmMinimalAgeGate", "shared dependency version",
  "Package.swift", "SwiftPM", "Swift Package Manager", "react-native spm".
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
│   │   ├── decoding/                   # Decoder factory, backends, SeekDecoderDaemon, AudioDecoding
│   │   ├── encoding/                   # AudioEncoder interface, EncoderCapabilities, OS encoder/remux selector headers, AudioFileConcatenator, AdtsHeader (ADTS framing)
│   │   ├── libs/                       # Third-party wrappers (FFmpeg, miniaudio, pffft, …)
│   │   └── external/                   # Prebuilt binaries per platform
│   │       ├── android/                # .a static libs (Opus, Ogg, Vorbis, OpenSSL)
│   │       ├── xcframeworks/           # Every iOS prebuilt as <lib>.xcframework (static Opus/Ogg/Vorbis, dynamic FFmpeg)
│   │       ├── include/                # Headers for Opus/Ogg/Vorbis/OpenSSL
│   │       └── include_ffmpeg/         # FFmpeg headers (downloaded, FFmpeg builds only)
│   ├── common/cpp/test/
│   │   ├── CMakeLists.txt              # Standalone test build (no Android/iOS)
│   │   ├── run-tests.sh / filters.sh    # smoke|extended|full (+ categories, CI path filters)
│   │   └── src/                        # Google Test files
│   ├── RNAudioAPI.podspec              # CocoaPods spec for iOS
│   ├── Package.swift                   # SwiftPM manifest for RN's `react-native spm` mode
│   ├── ios/spm/                        # SwiftPM-only wrapper sources + empty public-header dirs
│   └── scripts/
│       ├── download-prebuilt-binaries.sh  # Downloads externals from GitHub Releases
│       └── rnaa_utils.rb               # Ruby helpers for podspec (path resolution)
└── apps/
    └── fabric-example/
        └── ios/
            └── Podfile                 # Consumer Podfile (new arch enabled)
```

---

## Yarn Workspaces & Dependency Catalog

Dependency versions live in a single catalog in the root `.yarnrc.yml`, referenced from
manifests as `"<dep>": "catalog:"`. Bump once there and every workspace follows.
Needs Yarn >= 4.10 (repo is on 4.18).

**What belongs in the catalog** — anything that must move in lockstep, which is broader
than "used more than once":

- used by two or more workspaces, **or**
- version-locked to something already catalogued even if only one workspace uses it —
  `react-test-renderer` must equal `react`; `@react-native-community/cli*` and
  `@babel/runtime` move with `react-native` / `@babel/core`.

A dependency used in exactly one workspace with no such coupling stays a literal range:
it has no second copy to drift from, and catalogue membership is meant to *signal*
"keep this in lockstep". There is no CI check enforcing this — review is the enforcement,
which is why membership has to carry meaning.

**Workflow when touching dependencies:**

1. Changing the version of a catalogued dep → edit the catalog entry, not the manifest.
2. Adding a dep → check the catalog first. Already there? Use `"catalog:"`. Not there but
   lockstep-coupled or now used twice? Add the entry and switch both call sites over.

Three kinds of dependency must **never** be given `catalog:`:

| Never catalogue | Why |
|---|---|
| `peerDependencies` | Public API surface of the published packages, deliberately loose (`"*"`). Pinning narrows what consumers may install. |
| Runtime `dependencies` of published packages (`semver`, `chalk`, `commander`, `fs-extra`) | The release pipeline packs with `npm pack` (`scripts/create-package.sh`, `npm-custom-node-generator-build.yml`) and npm does not understand `catalog:`. It publishes the literal string, producing an uninstallable package. Only `yarn pack` / `yarn npm publish` perform the swap. |
| `workspace:*` ranges | Not version ranges — nothing to centralise. |

Other constraints:

- **`resolutions` does not accept `catalog:`** — install fails with `typescript@catalog: isn't supported by any available resolver`. Root `package.json` pins `typescript` literally; keep it in sync with the catalog entry. It is *not* redundant with the catalog: it forces `@commitlint/load`'s optional `typescript` peer (`^4.6.4 || ^5.2.2`) onto our version, and catalogs only rewrite workspace manifests, never transitive dependencies.
- **`packages/audiodocs` is a separate Yarn 1 project** with its own lockfile and `node_modules`. It is not in root `workspaces`, so the catalog does not reach it.
- **`yarn pack` is not a drop-in for `npm pack` here.** It drops `development/react/` (a nested `package.json` listed in `files[]`) and strips the executable bit from 29 prebuilt FFmpeg binaries. Do not switch the release pipeline to it without re-verifying the tarball contents.

Supply-chain gate: `npmMinimalAgeGate: '1w'` refuses versions published in the last 7 days.
Bypass one command with `yarn add --no-time-gate` / `yarn up --no-time-gate`, or exempt
specific packages permanently via `npmPreapprovedPackages`.

---

## C++ Header File Extensions

In `common/cpp/audioapi/`: `.hpp` = header-only templates; `.h` = non-template (usually with a `.cpp`). Vendored code is excluded.

---

## Prebuilt Binaries

External libraries (Opus, Ogg, Vorbis, OpenSSL, FFmpeg) are **not compiled from source** — they are downloaded as prebuilt `.a` / `.so` / `.xcframework` archives from:

```
https://github.com/software-mansion-labs/rn-audio-libs/releases/download/<TAG>/
```

Current tag: **v3.0.0** (see `scripts/download-prebuilt-binaries.sh`).

The download script is triggered automatically:
**FFmpeg is opt-in (off by default).** Both build options (`enableFFmpeg`, `disableStaticExternalLibs`) come from the `"react-native-audio-api"` section of the **app's package.json** on every path: podspec (`rnaa_utils.rb` `read_audio_api_build_options`, `<Podfile dir>/../package.json`), Gradle (`readAudioApiBuildOptions`, `rootProject.projectDir.parentFile`), `Package.swift` (walk from the autolinking symlink). The 0.13 inputs (Podfile `ENV[...]`, `gradle.properties` keys) are simply not read any more — no guard, by decision: it ships in a major. Non-boolean values are errors. The Expo plugin (`src/plugin/withAudioAPI.ts`) still accepts `enableFFmpeg` / `disableStaticExternalLibs`, but only as a *writer*: a `withDangerousMod` on both platforms (so a single-platform prebuild still runs it; idempotent) merges the given keys into the app package.json section during `expo prebuild`, keeping the file's indentation, before prebuild's `pod install`. Omitted options leave package.json alone; the builds never read the plugin options themselves. The pre-1.0 `DISABLE_AUDIOAPI_FFMPEG` / `disableAudioapiFFmpeg` / `disableFFmpeg` flags are **not** read any more and produce no warning either: a leftover disable flag asks for the default, so there is nothing to tell the user, and the switch is covered by the separate upgrade docs page (planned, not written yet). Migration notes belong on that page only — feature pages such as `prebuilt-libraries-support.mdx` describe the current behaviour without "Upgrading from…" notes. Internally every build file and source keys off the positive `$RN_AUDIO_API_FFMPEG_ENABLED` / `RN_AUDIO_API_FFMPEG_ENABLED`. The define is only emitted (`=1`) in an FFmpeg build; `#if RN_AUDIO_API_FFMPEG_ENABLED` reads an undefined macro as `0`, so the test, clangd and WPT builds do not mention it at all. The download script passes `skipffmpeg` in the default build, so neither the `.so`/xcframeworks nor (once published) the `include_ffmpeg.zip` header archive are fetched.

- **iOS**: by podspec `prepare_command` during `pod install` **and** by a `script_phase` (`[CP-User] Download RNAudioAPI prebuilt binaries`, `:before_headers`, always-out-of-date) on every Xcode build — downloads one `<name>.xcframework.zip` per library (plus `include_ffmpeg.zip` with FFmpeg). `prepare_command` ensures all vendored `.xcframework`s exist when CocoaPods generates `[CP] Copy XCFrameworks`. The build-time phase handles CI setups that cache `Pods/` independently from `node_modules/` — it is a fast no-op when binaries are already present. **It must use `:before_headers`, not `:before_compile`** — CocoaPods inserts its own `[CP] Copy XCFrameworks` phase right after `Headers`; `:before_compile` would land *after* it. The phase's `output_files` declares every vendored xcframework. The whole phase is skipped only when `DISABLE_AUDIOAPI_STATIC_EXTERNAL_LIBS` is set *and* FFmpeg is not enabled.
- **Android**: by `downloadPrebuiltBinaries` Gradle task, which runs before `preBuild` — downloads `android`, `jniLibs`

The script is idempotent — it skips any archive whose expected artifacts already exist (`artifacts_present`), so the per-build invocations are fast no-ops once binaries are present. A failed download is a hard error (`exit 1`): a silent skip used to surface much later as a missing header or `undefined symbol`.

**FFmpeg headers.** `external/include_ffmpeg/` is not tracked: it is gitignored, negated in `package.json` `files`, and fetched as `include_ffmpeg.zip` from the pinned rn-audio-libs release in the same pass as the FFmpeg binaries (`FFMPEG_HEADERS_ARCHIVE`, presence checked through `FFMPEG_HEADER_MARKERS`, skipped with `skipffmpeg`). The archive is produced by `yarn export` (`scripts/pack_outputs.sh`) in `rn-audio-libs`, which zips every `outputs/*` directory except `include`, so a new release of that repo ships the headers automatically — but every tag the download script pins must carry the asset or FFmpeg builds fail at the download step. Nothing else references the directory outside the gated include-path entries (podspec `pod_target_xcconfig`, Android CMake); `common/cpp/clangd/CMakeLists.txt` adds the path unconditionally, so in a checkout that never enabled FFmpeg clangd shows unresolved `libav*` includes only inside `FfmpegDecoder.cpp`. `FfmpegDecoder.h` includes `libav*` directly, but `DecoderFactory.cpp` is the only consumer and gates that include behind `RN_AUDIO_API_FFMPEG_ENABLED`, every disabled build also drops `FfmpegDecoder.cpp`, and the Gradle prefab header copy excludes `audioapi/external/**`.

Downloaded artifacts land in:
- `common/cpp/audioapi/external/android/<ABI>/` — `.a` static libs for Android ABIs
- `android/src/main/jniLibs/<ABI>/` — FFmpeg `.so` shared libs for Android (loaded at runtime)
- `common/cpp/audioapi/external/xcframeworks/` — every iOS prebuilt as `<lib>.xcframework` (static Opus/Ogg/Vorbis, dynamic FFmpeg) (each release zip unpacks to exactly that directory). Older checkouts may still hold the pre-v4.0.0 `ffmpeg_ios/`, `iphoneos/`, `iphonesimulator/`, `macosx/` — unused and gitignored

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
- `newArchEnabled` is read from the app's `gradle.properties` and defaults to **on**: only an explicit `newArchEnabled=false` builds the legacy path, because RN 0.82+ apps no longer declare the property (the podspec likewise treats only `RCT_NEW_ARCH_ENABLED=0` as legacy); `enableFFmpeg` / `disableStaticExternalLibs` from the app's `package.json` (`readAudioApiBuildOptions`); all are forwarded to both CMake and Kotlin `BuildConfig`
- Source exclusion in `android/src/main/cpp/audioapi/CMakeLists.txt` must use `list(FILTER … EXCLUDE REGEX)` — `COMMON_CPP_SOURCES` holds paths relative to that directory, so a `list(REMOVE_ITEM …)` with an absolute path silently matches nothing (this is how the FFmpeg exclusion was a no-op for a long time)
- Third-party include dirs (`external/include`, `external/include_ffmpeg`) are only added when the corresponding library set is enabled
- DSP sources always compiled with `-O3` regardless of overall build type
- Sources gathered with `GLOB_RECURSE CONFIGURE_DEPENDS` — CMake re-runs automatically when files are added/removed
- 16KB page size alignment enabled for Android 15+
- **Extension API (prefab)**: single public C++ header `<audioapi/compatibility/StableAPI.h>`; prefab publishes transitive headers needed to compile it (`prepareAudioApiHeadersForPrefabs`); `fix-prefab.gradle` ensures the `.so` is in prefab metadata. Contract: `EXTENSION_API.md`
- CMake exposes `COMMON_CPP_DIR` and `ANDROID_CPP_DIR` as **PUBLIC** include dirs so prefab consumers resolve `<audioapi/...>`
- **React Native's transitive Kotlin/Java deps are usable without declaring them.** `ReactAndroid` declares several libraries with `api(...)` rather than `implementation(...)`, so they reach us through the existing `implementation "com.facebook.react:react-native:+"`. Fresco is the notable one (`com.facebook.fresco:fresco` — 3.2.0 on RN 0.76, 3.7.0 on RN 0.87), used by `system/notification/ArtworkLoader.kt`. Check the RN version's `ReactAndroid/build.gradle.kts` before adding a dependency that RN may already expose; adding it explicitly only risks a version conflict.
- **Verify any such API at the RN floor, not just the checked-in version.** `android/build.gradle` asserts RN minor >= 76, so a Kotlin API that exists in `node_modules` today may not exist at 0.76. Fresco's API surface happens to be identical across 3.2.0-3.7.0, but the third `ResizeOptions` constructor parameter was renamed (`maxBitmapSize` -> `maxBitmapDimension`), so positional arguments are required. Sources for a given version are fetchable from `raw.githubusercontent.com/facebook/{react-native,fresco}/v<tag>/...`.

For full per-line analysis see [build-details.md](build-details.md#android-androidcmakeliststxt-root--detailed-analysis).

---

## iOS Build (CocoaPods) — high-level summary

### Files
- `RNAudioAPI.podspec` — library spec with subspecs
- `scripts/rnaa_utils.rb` — Ruby helpers called by podspec
- `apps/fabric-example/ios/Podfile` — consumer

### Key behaviors
- Four subspecs split compilation: `audioapi` (core C++), `audioapi/ios` (ObjC++), `audioapi/audioapi_dsp` (DSP with `-O3`), `audioapi/miniaudio_impl` (compiled as `-x objective-c++`)
- Every prebuilt (static Opus/Ogg/Vorbis, dynamic FFmpeg) is an `.xcframework` in `s.ios.vendored_frameworks` (named after the library: CocoaPods links a dynamic xcframework as `-framework <xcframework name>`) — the same release assets Package.swift downloads; CocoaPods picks the device/simulator/Catalyst slice and embeds the dynamic ones. The pre-v4.0.0 per-SDK `.a` dirs (`iphoneos/`, `iphonesimulator/`, `macosx/`) and their `-force_load …/$(PLATFORM_NAME)` flags are gone
- **CocoaPods never links a pod's own vendored dynamic xcframeworks into the pod's own target** (only into targets that depend on it). Under `use_frameworks! :linkage => :dynamic` `RNAudioAPI` must link FFmpeg itself, so the podspec adds `ffmpeg_ldflags` to `pod_target_xcconfig` `OTHER_LDFLAGS` (FFmpeg builds only). Build the xcframework list and the flags from the same `ffmpeg_libs`; see [build-details.md](build-details.md#ffmpeg-link-flags-on-the-pod-target)
- `Accelerate` framework linked, enabling `HAVE_ACCELERATE=1` for vDSP SIMD on iOS
- Header search paths split: `pod_target_xcconfig` (library compilation) vs `xcconfig` (app consumers)
- `rnaa_utils.rb` resolves dynamic paths at `pod install` time (not hardcoded)
- JSI internals use distinct names (`HostObject`, `RuntimeInstanceCache`, `RuntimeObserver`) instead of generic names like `JsiHostObject` / `RuntimeAwareCache` / `RuntimeLifecycleMonitor` — `react-native-skia` publishes headers with those generic names and CocoaPods can resolve the wrong file when both libraries are in the same app

For full per-line analysis see [build-details.md](build-details.md#ios-rnaudioapipodspec--detailed-analysis).

**Include our own headers by full path** (`<audioapi/ios/system/AudioEngine.h>`), never bare
(`<AudioEngine.h>`): CocoaPods' header map resolves bare names, SwiftPM does not.

---

## iOS Build (SwiftPM) — `Package.swift`

React Native's experimental `npx react-native spm` mode (0.87+) symlinks each library to
`<app>/ios/build/generated/autolinking/libs/<SwiftName>/` and resolves our hand-written
`Package.swift` from there — hence the fixed relative paths `../../../../xcframeworks` (`ReactNative`
headers-only package) and `../../../ios` (`React-GeneratedCode`, which serves `<rnaudioapi/rnaudioapi.h>`).
It only builds inside such an app.

**The package and its library product must both be named `ReactNativeAudioApi`.** Autolinking emits
`.product(name: X, package: X)` for a library's Swift name X. 0.88+ takes X from `swiftpmConfig.name` in
package.json, then the deprecated `spm.name` in `react-native.config.js`. 0.87 reads only `spm.name` and
otherwise derives X from the npm name (`react-native-audio-api` → `ReactNativeAudioApi`). Setting
`swiftpmConfig.name` to that derived name makes both versions agree without a `react-native.config.js`
(0.88 would warn that `spm` is deprecated). A different name breaks 0.87 with "product 'ReactNativeAudioApi'
… not found". The internal targets keep the `RNAudioAPI` / `RNAudioAPIDSP` names. Verified 2026-10-19 on
0.87.1 and 0.88.0.

Keep it in sync with the podspec; how each podspec feature maps:

| Podspec | SwiftPM |
|---|---|
| `miniaudio_impl` subspec (`-x objective-c++`) | `ios/spm/sources/MiniaudioImplementation.mm` includes the `.cpp`; the `.cpp` is excluded |
| `EXCLUDED_SOURCE_FILE_NAMES[config=Release]` `AudioThreadGuard.cpp` | `ios/spm/sources/AudioThreadGuardDebug.cpp` wraps it in `#ifdef DEBUG` |
| `audioapi_dsp` subspec (`-O3`) | separate `RNAudioAPIDSP` target with `.unsafeFlags(["-O3"])` |
| `vendored_frameworks`, `include_ffmpeg` | remote `.binaryTarget(url:checksum:)` per library — the same zips (a binary target is named after its xcframework, so the targets are plain `libopus`, `libavcodec`, …); FFmpeg headers ship as the header-only `FFmpegHeaders.xcframework` (stub lib + `Headers/`) because the FFmpeg frameworks carry none. SwiftPM rejects `$(…)` macros in `unsafeFlags`, so per-SDK `.a` linking is impossible |
| `prepare_command` download | none — SwiftPM downloads and checksum-verifies the zips while resolving. Assets come from rn-audio-libs `scripts/pack_spm_xcframeworks.sh`; bump `prebuiltBinariesTag` and every checksum together |
| package.json build options (`rnaa_utils.rb`) | the same `"react-native-audio-api"` section, found by walking up from the autolinking symlink; `booleanOption()` rejects non-booleans. No env vars: Xcode re-resolves packages without the shell env |

- Every non-source file under `common/cpp/audioapi` (`.md` docs so far) must be listed in `mainTargetExcludes`. Not fatal, tested 2026-10-19: without the excludes the build succeeds but Xcode shows "no rule to process file … of type 'file'" once per file per architecture in every SwiftPM app. SwiftPM has no globs, so add each new doc file by path.
- `DEBUG=1` / `NDEBUG` per configuration are mandatory: the prebuilt React framework's C++ ABI depends on `NDEBUG`.
- `publicHeadersPath` points at the empty `ios/spm/include` / `ios/spm/dsp-include` (a `.gitkeep` holding a note keeps each in the npm tarball, which drops empty directories; same trick as Reanimated's root `include/.gitkeep`). `package.json` `files` must re-list both `.gitkeep` paths after its `!**/.*` rule, or npm drops them and SwiftPM resolution fails — check with `npm pack --dry-run --ignore-scripts | grep ios/spm`. Both are required, tested 2026-10-19: with the default `include` missing (targets use `path: "."`), resolution fails with "public headers (\"include\") directory path for 'RNAudioAPIDSP' is invalid"; one shared directory resolves but compiling fails with "umbrella for module 'RNAudioAPIDSP' already covers this directory", because each target gets its own umbrella module map. Pointing it at `common/cpp` would build an umbrella module over every C++ header; revisit when react-native-audio-worklets needs our headers under SwiftPM.
- `package.json` `files` negates every download directory (`external/xcframeworks`, `external/android`, `external/include_ffmpeg`), so publishing from a machine with binaries downloaded still ships only the tracked `external/include` headers. Add a negation for any new download directory; check with `npm pack --dry-run --ignore-scripts`.
- **`Context.packageDirectory` is the autolinking symlink** (`<app>/ios/build/generated/autolinking/libs/ReactNativeAudioApi`, probed with a `fatalError` during `xcodebuild -resolvePackageDependencies` on 2026-10-19), not the resolved `node_modules` path. `readAppConfiguration()` therefore walks `libs → … → ios → <app>` and finds the app even when the package is hoisted in a monorepo; it never passes the library's own `package.json`. This relies on RN's symlink layout — re-probe if a future RN version references libraries by real path.
- **SwiftPM's manifest cache ignores files the manifest reads.** `~/Library/Caches/org.swift.swiftpm/manifests/manifest.db` is keyed by `Package.swift` contents + path + tools version + env + SwiftPM version (SwiftPM `ManifestLoader.CacheKey`), so a changed `package.json` section is invisible until `rm -rf ~/Library/Caches/org.swift.swiftpm/manifests` (Xcode ▸ File ▸ Packages ▸ Reset Package Caches) — verified 2026-10-19: an identical manifest kept FFmpeg on after the option flipped off; appending a comment to the manifest made it re-evaluate. When testing option changes, bust the cache or change the manifest text; a fresh DerivedData is not enough.
- **Never replace a published asset in place.** SwiftPM remembers each binary target's checksum per URL and refuses a changed artifact ("has changed checksum; this is a potential security risk"), so anyone who already resolved the tag breaks. Repacking always changes checksums (zip timestamps) — publish under a new tag instead.
- To test unpublished assets, rewrite a packed copy's binary targets to `binaryTarget(path: "<dir>/X.xcframework.zip")` (local zips work) and install it with `npm install --ignore-scripts` (a directory dependency otherwise runs `prepare`).
- **react-native-audio-worklets** has its own `Package.swift` (package/product `ReactNativeAudioWorklets`, the name 0.87 derives; `swiftpmConfig` repeats it and lists `react-native-audio-api` / `react-native-worklets` as `dependencies`). It depends on the sibling autolinked packages `../ReactNativeAudioApi` and `../RNWorklets` (worklets names itself `RNWorklets` via the `spm` block in its `react-native.config.js`; Package.swift since worklets 0.13.0). Our RNAudioAPI target exposes no headers, so the worklets target reaches `<audioapi/...>` with `.unsafeFlags(["-I", "<packageDirectory>/../ReactNativeAudioApi/common/cpp"])` — the SwiftPM counterpart of the podspec's `Headers/Public/RNAudioAPI` path (`headerSearchPath` cannot leave the package). It must define `DEBUG`/`NDEBUG` per configuration like RNAudioAPI, for the same reason: React Native's headers change class layouts on `NDEBUG`. (Our own headers only use `NDEBUG` inside inline function bodies, e.g. `BaseAudioContext::assertDriverMutexHeld`, which changes behaviour, not layout.) Verified 2026-10-19: Debug + Release on 0.88.0, Release on 0.87.1, and at runtime on the iOS 27 simulator (`NativeAudioWorkletsModule` resolves, `install()` returns true, AudioContext running).
- Verify in throwaway apps on the oldest (0.87.x) and newest supported RN: `@react-native-community/cli init … --version <exact x.y.z>` (a bare `0.87` finds no template), install an `npm pack --ignore-scripts` tarball under a unique file name, `cd ios && npx react-native spm --deintegrate --yes` (both versions pick `add`/`update` themselves), then `xcodebuild -project <App>.xcodeproj`.

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
- After changing `rnaa_utils.rb`
- When prebuilt binaries need to be re-downloaded (podspec `prepare_command` runs on `pod install`)

**Enable FFmpeg** (off by default; `apps/fabric-example/package.json` enables it for every build path):
```json
"react-native-audio-api": { "enableFFmpeg": true }
```

### Android (fabric-example)

```bash
yarn workspace fabric-example android
# or open in Android Studio:
open apps/fabric-example/android
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
yarn validate:cpp       # C++ smoke
yarn validate:cpp-extended  # C++ extended (all categories)
yarn validate:android   # yarn workspace … build:android
yarn validate:ios       # yarn workspace … build:ios (macOS only)
yarn validate:full      # --fast + C++ extended + --android + --ios
```

Script: [`scripts/validate.sh`](../../../scripts/validate.sh) at monorepo root.

### What CI covers vs what validation tiers add

| Layer | CI (`ci.yml` + `tests.yml`) | Local tiers |
|---|---|---|
| TS build (`bob build`) | Yes | `--fast` |
| C++ smoke (`run-tests.sh`) | Yes | `--fast` |
| C++ coverage (`run-coverage.sh`, smoke, Clang) | Yes (`cpp-coverage` artifact) | `yarn test:cpp:coverage` |
| Jest | Yes | `--fast` |
| Extended C++ by category (e.g. graph) | Path change or manual dispatch in `tests.yml` | `--cpp-extended` |
| HostObjects (26 JSI `.cpp` files) | **No** | `--android` + `--ios` |
| Android JNI C++ + Kotlin | **No** | `--android` |
| iOS ObjC++ | **No** | `--ios` |
| Prebuilt libs (Opus, FFmpeg, etc.) | **No** | `--android` / `--ios` (via `download-prebuilt-binaries.sh`) |
| TurboModule codegen, worklets linking | **No** | `--android` / `--ios` |

The C++ test build excludes HostObjects, `AudioContext.cpp`, `FfmpegDecoder.cpp`, and worklet nodes — see [C++ Tests](#c-tests-standalone-build) below. Passing `yarn test` alone does not prove native layers compile.

### Shared prebuild phase (before native tiers)

Before `--android` or `--ios`, the script runs once:

1. `yarn install --immutable` + `yarn build`
2. [`download-prebuilt-binaries.sh`](../../../packages/react-native-audio-api/scripts/download-prebuilt-binaries.sh) `{android|ios}` (must run from `packages/react-native-audio-api/scripts/`)
3. `yarn workspace react-native-audio-api test:cpp`

Android (NDK) and iOS (Clang) cannot share object files — reuse is at the prebuild/download level. If `ccache` is installed, `validate.sh` wraps host `CC`/`CXX`; only the host C++ tests (`test:cpp`) honor that — Gradle/NDK and `xcodebuild` use their own toolchains and are unaffected.

### Platform skip behavior

- `--ios` on Linux → skip with message (exit 0)
- `--android` without `ANDROID_HOME` → fail on explicit `--android`; skip with warning inside `--full`
- `--full` includes C++ extended (all categories) after `--fast`’s smoke, so local full covers C++ full + native builds

### Which tier to run

See the decision table in [post-work-checks](../post-work-checks/SKILL.md).

---

## C++ Tests (standalone build)

### Location
`packages/react-native-audio-api/common/cpp/test/`

### How to run
```bash
yarn test   # Jest + C++ smoke
yarn workspace react-native-audio-api test:cpp:smoke|extended|full
yarn workspace react-native-audio-api test:cpp:extended -- graph
```

`run-tests.sh [smoke|extended|full] [category…] [--ubasan|--tsan|--no-ubasan]` uses filters from `filters.sh`. Docs: `common/cpp/test/TESTING.md`. Shell scripts in this repo use kebab-case plus `.sh` (`run-tests.sh`, not `RunTests.sh`).

> **Sanitizer binaries compile every engine translation unit with that sanitizer.** `add_sanitized_tests(<name> <compile-flags> <link-flags>)` builds `tests_<name>` plus `rnaudioapi_<name>` and `rnaudioapi_libs_<name>`. `tsan` uses `-fsanitize=thread`; `asan` uses `-fsanitize=address,undefined`. The plain `rnaudioapi` pair is only for `tests` and the coverage build. TSan misses release/acquire pairs in uninstrumented code, so an uninstrumented engine turns every SPSC hand-off into a false race (`<null>` frames, "As if synchronized via sleep"). ASan and UBSan simply never check a translation unit they did not compile. `AudioThreadGuard.cpp` is compiled into the executable, not the static lib, so its `__SANITIZE_*` macros match that binary. GoogleTest stays the shared uninstrumented target: one gtest cannot be compiled with both sanitizers and with neither.

### Coverage (Clang / llvm-cov)

```bash
yarn workspace react-native-audio-api test:cpp:coverage
# open packages/react-native-audio-api/common/cpp/test/coverage-html/index.html
```

`run-coverage.sh` configures a separate `build-coverage/` tree with `-DENABLE_COVERAGE=ON` (Clang-only LLVM source-based coverage: `-fprofile-instr-generate -fcoverage-mapping`), defaults `CC`/`CXX` to `clang`/`clang++` when unset, runs the **smoke** filter from `filters.sh`, then prints `llvm-cov report` and writes HTML via `llvm-cov show -format=html`. When `GITHUB_STEP_SUMMARY` is set, the report is also appended there. Sanitizer targets are skipped when coverage is enabled. Requires Apple Clang / `xcrun llvm-profdata` and `xcrun llvm-cov` on macOS (or the same tools on PATH for Linux).

CI runs a parallel `cpp-coverage` job via `.github/workflows/cpp-coverage-job.yml` (called from `tests.yml` on pull requests; Clang + LLVM apt packages, separate from the GCC `cpp-smoke-tests` job). It uploads the HTML tree as the `cpp-coverage-html` artifact (14-day retention); download the zip from the Actions run and open `index.html`. Manual `workflow_dispatch` on `tests.yml` accepts booleans `run_cpp_smoke_tests` / `run_cpp_coverage` / `run_js_tests` (default true); non-draft PRs always run all three, including when a draft is marked ready for review (`ready_for_review` is listed explicitly because it is not a default `pull_request` type). Extended categories share `cpp-job.yml` and pass `categories`; path-filter YAML is generated by `filters.sh path-filters`.

> **Generated build trees must be named `build*`.** The C++ linters walk the filesystem with `find` and never consult git, so a `.gitignore` entry does not keep generated sources out of them. Exclusion happens by directory name in two places that must stay in sync: `**/build*/**` in `.clang-format-ignore` (used by `format:check:common`) and `-type d -name 'build*' -prune` in `scripts/cpplint.sh`. A CMake binary directory outside that prefix makes the pre-commit hook fail on generated files such as `CMakeFiles/*/CompilerIdCXX/CMakeCXXCompilerId.cpp`. CI never hits this because it checks out a clean tree.

### Key design decisions
- Completely standalone — no Gradle, no Xcode, no prebuilt Android libraries needed
- Sources resolved from `node_modules` (symlinked to `packages/` in yarn workspaces)
- HostObjects (except the `list(APPEND RNAUDIOAPI_SRC …)` exceptions), AudioContext, and FfmpegDecoder are excluded from the test build. There is no worklets-node filter and no `RN_AUDIO_API_ENABLE_WORKLETS` define; engine sources no longer read that flag
- The static lib compiles `jsi/jsi.cpp`, so real JSI types (`jsi::HostObject`, `jsi::MutableBuffer`) work in tests. When core code a test links constructs a HostObject (e.g. `AudioRecorderCallback` → `AudioBufferHostObject`), add that HostObject's `.cpp` to the `list(APPEND RNAUDIOAPI_SRC …)` exceptions in `test/CMakeLists.txt` (as for `JsEnumParser.cpp` and `AudioBufferHostObject.cpp`) instead of writing a stub translation unit or an `#if RN_AUDIO_API_TEST` test double
- Compile definitions: `RN_AUDIO_API_TEST=1`, `RN_AUDIO_API_STATIC_EXTERNAL_LIBS_DISABLED=1`, `MA_NO_LIBOPUS=1`, `MA_NO_LIBVORBIS=1` (FFmpeg is off because its define is absent). `add_compile_definitions` is directory-scoped, so every target in this file receives them
- Google Test auto-fetched via `FetchContent` if not installed locally
- New test files in `test/src/**/*.cpp` are picked up automatically by glob — no CMakeLists edit needed
- `jsi.cpp` is compiled into the static lib so library members that reference JSI symbols (e.g. `AudioFileProperties::CreateFromJSIValue`) link when a test first pulls them in; a static-lib member costs nothing unless demanded. If a new test triggers `Undefined symbols: facebook::jsi::...`, the referenced runtime source is missing from the lib — add it there rather than stubbing the symbol

For `MockAudioEventHandlerRegistry`, `TestableXxx` pattern, and full CMakeLists analysis see [build-details.md](build-details.md#c-test-build--commoncpptestcmakeliststxt--detailed-analysis).

---

## WPT Node bindings build (`wpt_tests/CMakeLists.txt`)

Builds the C++ engine as a Node.js addon (`.node`, via cmake-js + microsoft/node-api-jsi) so Web Platform Tests can run against it: `yarn workspace react-native-audio-api node:build` (part of `yarn wpt`). The engine is always compiled from the workspace `common/cpp` sources.

The JS wrapper layer (`lib/commonjs` classes and error types) is resolved by `wpt_tests/package-root.js`. By default it uses the workspace build output of `yarn build`; set `RN_AUDIO_API_APP_ROOT` to an app directory to load the `react-native-audio-api` JS installed for that app instead (e.g. a published npm release):

```bash
RN_AUDIO_API_APP_ROOT=/path/to/app yarn wpt:only
```

Classes missing from an older package version are skipped with a warning, so their tests fail at runtime instead of crashing the harness at load. Note the native addon is NOT swapped by this env var — only the JS layer.

Resolution pitfalls learned the hard way (both handled inside `package-root.js`):

- Yarn (node-modules linker) may **hoist** the app's pinned npm copy to the monorepo root `node_modules` rather than `<app>/node_modules`, so the package must be resolved with Node resolution from the app directory, not a hard-coded `<app>/node_modules/<pkg>` path. If `yarn install` claims success but the copy is missing everywhere, delete `.yarn/install-state.gz` and reinstall.
- `require.resolve('react-native-audio-api/...', { paths: [appRoot] })` from a file *inside* the workspace package still resolves to the workspace itself (Node package **self-reference** beats the `paths` option). Use `createRequire(path.join(appRoot, 'anything.js'))` to anchor resolution in the app.

`yarn wpt:report:docs` runs the suite twice (workspace `main`, then the stable release pinned in `apps/common-app` via `wpt:report:stable`) and regenerates the side-by-side table in `packages/audiodocs/docs/other/web-audio-api-coverage.mdx`.

---

## Conditional Compilation Flags Summary

| Flag | Android (CMake) | iOS (podspec) | Tests |
|---|---|---|---|
| `RN_AUDIO_API_FFMPEG_ENABLED` | `-DRN_AUDIO_API_FFMPEG_ENABLED=1` (only with `enableFFmpeg: true` in the app's package.json) | same | | Not set (off) |
| `RN_AUDIO_API_ENABLE_WORKLETS` | Not set (engine no longer reads it) | Not set | Not set. `wpt_tests/CMakeLists.txt` still passes `=0` |
| `RCT_NEW_ARCH_ENABLED` | `-DRCT_NEW_ARCH_ENABLED` | `-DRCT_NEW_ARCH_ENABLED` | Not set |
| `HAVE_ARM_NEON_INTRINSICS` | Set by CMake SIMD detection | Set by Xcode/Clang for arm64 | Set by CMake SIMD detection |
| `HAVE_X86_SSE2` | Set by CMake SIMD detection | Not used | Set by CMake SIMD detection |
| `HAVE_ACCELERATE` | Not set | `GCC_PREPROCESSOR_DEFINITIONS` | Not set |
| `RN_AUDIO_API_TEST` | Not set | Not set | Always set to 1 |

**OS-API selector headers** (`decoding/OSDecoding.h`, `encoding/OSEncoding.h`, `encoding/OSRemux.h`, `encoding/OSFilePath.h`): common code reaches platform implementations through `#if defined(__ANDROID__)` / `#elif defined(__APPLE__) && !defined(RN_AUDIO_API_TEST) && !defined(RN_AUDIO_API_NODE)` dispatch. The Apple branch must exclude **both** desktop defines: the gtest build (`RN_AUDIO_API_TEST`) and the WPT node addon (`RN_AUDIO_API_NODE`) run on macOS (where `__APPLE__` is defined) but do not compile or link the `ios/` ObjC++ sources. The node build cannot borrow `RN_AUDIO_API_TEST` instead — that flag also switches on gtest-only code (`gtest_prod.h` includes, test `ArrayBuffer` shims). When adding a new OS-selector header, copy the full three-clause guard; an incremental `wpt_tests/build` dir can mask a missing clause for a long time, so verify with a clean `yarn node:build`. Platform glue selected this way lives in `android/src/main/cpp/audioapi/android/` (e.g. `AndroidDecoding`, `AndroidEncoder`, `AndroidRemux`) and `ios/audioapi/ios/core/utils/` (e.g. `IOSDecoding`, `IOSEncoder`, `IOSRemux`) — both picked up automatically by the CMake glob / podspec glob, no build-file edits needed.

---

## Common Build Failure Patterns

| Symptom | Likely cause | Fix |
|---|---|---|
| `file not found: libopus.a` | Prebuilt binaries not downloaded | Run `pod install` (iOS) or Gradle build (triggers download task) |
| `No such module 'RNAudioAPI'` | Pod not installed | `cd apps/fabric-example/ios && pod install` |
| `undefined symbol: av_*` | FFmpeg .so not in jniLibs | Build triggers download; verify `"enableFFmpeg": true` is in the app's package.json (FFmpeg is off by default) |
| `'libavcodec/avcodec.h' file not found` | FFmpeg include dir is gated on the enable flag; something compiled `FfmpegDecoder.cpp` in a disabled build | Check the source filter (`list(FILTER … FfmpegDecoder\.cpp$)` / podspec `exclude_files`) picked the file up |
| CMake error on clean build | Stale `.cxx` cache | `rm -rf packages/react-native-audio-api/android/.cxx` |
| Test build: `Cannot open include file: audioapi/...` | Node modules not linked | `yarn install` from root, then re-run tests |
| New `.cpp` not compiled in tests | Glob picks it up automatically — may need cmake reconfigure | Delete `test/build/` and re-run |
| iOS compile error `unknown type 'id'` | C++ file included ObjC-only header | Compile that file as ObjC++ (separate subspec with `-x objective-c++`) |
| `RCT_NEW_ARCH_ENABLED` undefined on Android | App sets `newArchEnabled=false` | Remove it; a missing property already means the New Architecture |
| iOS: `'to_chars' is unavailable: introduced in iOS 16.3` from `formatter_floating_point.h`, instantiated by `std::format<...>` | `std::format` in code compiled for iOS. libc++ availability-gates the whole `<format>` library to iOS 16.3; the podspec minimum is `ios_min_version = '14.0'`. The desktop C++ test build and Android NDK have no such gate, so `yarn test:cpp` passes and only the iOS build fails. | Use `std::string` concatenation / `std::to_string` in `common/cpp` and `ios/`. Zero-pad by hand (`insert(0, n, '0')`). Android-only files (`android/src/main/cpp`) may keep `std::format`. |
| iOS test target: `Undefined symbol: audioapi::<something>` for a function that exists in `common/cpp` and compiles in the pod | The XCTest bundle links against the host app, and the host app only pulls the static-lib members its own code references. A common symbol nobody in `ios/` or the HostObjects uses never reaches the host. | Compute it inline in the test, or make the library reference it; do not add a fake use in production code. |
| iOS: new `common/cpp` `.cpp` not compiled at all (`Undefined symbol` with no `CompileC` line for it) | CocoaPods expands `source_files` globs at `pod install`, not at build time | `cd apps/fabric-example/ios && pod install` after adding a source file |
| `check-audio-file-properties-enum-sync.sh` passes while `FileFormat` is out of sync | Its extractor only reads `Name = <number>` lines; `FileFormat` has no explicit values on either side, so it compares two empty lists ("in sync (0 entries)") | Keep the C++ and TS `FileFormat` lists in the same order by hand; `EncoderCapabilities.h`'s `static_assert` only checks the C++ count |
| Android/iOS: after editing a C++/TS mirrored enum (`FileFormat`, `BitDepth`, …) the app silently uses the wrong value (e.g. records FLAC when JS asked for ADTS) | Metro hot-reloaded the TS enum with the new numbering, but the installed binary still carries the old C++ numbering; enum values cross JSI as plain numbers | Reinstall the app (`installDebug` / `run-ios`) after any enum reorder; a JS reload is never enough |
| Android incremental build only: `Unresolved reference` to a top-level Kotlin function, or `Cannot access '…': it is internal in file`, while `--rerun-tasks` passes | Incremental Kotlin rounds treat the module's own earlier classes as a foreign module (library is built through the `node_modules/react-native-audio-api` symlink) | Use only public classes/objects across files in `android/src/main/java`; no top-level functions, no `internal` |
| clangd only: `'React/RCTBridgeModule.h' file not found` in `.mm` files | `compile_commands.json` has no framework search path | See *clangd compile database* below — regenerate with `yarn setup:clangd` |

## clangd compile database

`packages/react-native-audio-api/common/cpp/clangd/` generates the repo-root
`compile_commands.json` that clangd reads (`yarn setup:clangd`, or
`yarn setup:clangd:clean` after a native dependency bump). It is editor tooling
only — it never participates in a real build. See its `SETUP.md`.

The iOS half reuses whatever `pod install` resolved for `apps/fabric-example`,
reading **both** `HEADER_SEARCH_PATHS` and `FRAMEWORK_SEARCH_PATHS` out of
`Pods/Target Support Files/Pods-FabricExample/Pods-FabricExample.debug.xcconfig`.

Framework search paths matter because React-Core ships prebuilt as
`React.xcframework` since RN 0.87: `#import <React/RCTBridgeModule.h>` resolves
via `-F`, not `-I`, so an `-I`-only database fails every `.mm` that imports
React. Xcode's own `-F` points at `PODS_XCFRAMEWORKS_BUILD_DIR`, which only
exists after Xcode has unpacked the slices — the CMakeLists globs the
`ios-*simulator*` slices inside the `.xcframework` bundles instead, so a bare
`pod install` suffices.

To check the database rather than guessing at clangd, replay an entry directly:

```bash
python3 -c "
import json,shlex,subprocess
e=[x for x in json.load(open('compile_commands.json')) if x['file'].endswith('ios/AudioAPIModule.mm')][0]
toks=[t for t in shlex.split(e['command']) if t!='-c']
subprocess.run(toks[:toks.index('-o')]+toks[toks.index('-o')+2:]+['-fsyntax-only'], cwd=e['directory'])
"
```

If that reproduces the error, the database is wrong — not the editor's LSP setup.

---

*Maintenance: see [maintenance.md](maintenance.md).*
