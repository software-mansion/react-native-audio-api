# Build Details — CMake, Gradle & Podspec Deep Reference

> This file contains the detailed per-platform build system analysis for `CMakeLists.txt`, `android/build.gradle`, `RNAudioAPI.podspec`, and the C++ test build.
>
> For the high-level overview, prebuilt binaries, building apps, conditional flags, and common failures, see [SKILL.md](SKILL.md).

---

## Android: `build.gradle` — detailed analysis

### Feature flags (set by app's `gradle.properties`)

```groovy
def isNewArchitectureEnabled() {
  return rootProject.hasProperty("newArchEnabled") && rootProject.getProperty("newArchEnabled") == "true"
}
def isFFmpegDisabled() {
  return rootProject.hasProperty("disableAudioapiFFmpeg") && rootProject.getProperty("disableAudioapiFFmpeg") == "true"
}
```

### Forwarding flags to CMake

```groovy
"-DRN_AUDIO_API_FFMPEG_DISABLED=${IS_RN_AUDIO_API_FFMPEG_DISABLED}"
"-DRN_AUDIO_API_WORKLETS_ENABLED=${isWorkletsAvailable}"
"-DIS_NEW_ARCHITECTURE_ENABLED=${IS_NEW_ARCHITECTURE_ENABLED}"
```

### Forwarding flags to Kotlin via BuildConfig

```groovy
buildConfigField "boolean", "RN_AUDIO_API_FFMPEG_DISABLED", isFFmpegDisabled().toString()
buildConfigField "boolean", "RN_AUDIO_API_ENABLE_WORKLETS", "${isWorkletsAvailable}"
```

### 16KB page size alignment (Android 15+)

```groovy
packagingOptions {
  jniLibs { useLegacyPackaging = false }
}
```

### Worklets dependency ordering

Worklets must be merged before the audio API's CMake build starts. Gradle task dependency is wired explicitly:

```groovy
tasks.getByName("buildCMakeDebug").dependsOn(rnWorkletsProject.tasks.getByName("mergeDebugNativeLibs"))
```

### Minimum RN version enforcement

Enforced in Gradle task `assertMinimalReactNativeVersionTask` (currently RN 76+).

---

## Android: `android/CMakeLists.txt` (root) — detailed analysis

Thin delegator. Sets up SIMD detection, applies Folly + React Native flags, then:

```cmake
add_subdirectory("${ANDROID_CPP_DIR}/audioapi")
```

### SIMD detection (affects DSP performance)

```cmake
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    set(HAVE_ARM_NEON_INTRINSICS TRUE)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64")
    set(HAVE_X86_SSE2 TRUE)
endif()
```

### RN version-dependent flags (workaround for RN 0.80+ flag changes)

```cmake
if(ReactAndroid_VERSION_MINOR GREATER_EQUAL 80)
  include("${REACT_NATIVE_DIR}/ReactCommon/cmake-utils/react-native-flags.cmake")
  target_compile_reactnative_options(react-native-audio-api PRIVATE)
else()
  string(APPEND CMAKE_CXX_FLAGS " -fexceptions -frtti ...")
endif()
```

---

## Android: `android/src/main/cpp/audioapi/CMakeLists.txt` — detailed analysis

Builds `react-native-audio-api` as a shared library.

### Source gathering

Uses `GLOB_RECURSE CONFIGURE_DEPENDS` — CMake will re-run automatically when files are added/removed:

```cmake
file(GLOB_RECURSE COMMON_CPP_SOURCES CONFIGURE_DEPENDS "${COMMON_CPP_DIR}/audioapi/*.cpp")
```

### FFmpeg conditional exclusion

```cmake
if(RN_AUDIO_API_FFMPEG_DISABLED)
  list(REMOVE_ITEM COMMON_CPP_SOURCES
    "${COMMON_CPP_DIR}/audioapi/libs/ffmpeg/FFmpegDecoding.cpp"
  )
endif()
```

### DSP sources get `-O3` (always, regardless of overall build type)

```cmake
set_source_files_properties(${DSP_CPP_SOURCES} PROPERTIES COMPILE_FLAGS "-O3")
```

### Static prebuilt libs imported via `IMPORTED_LOCATION`

```cmake
foreach(lib IN ITEMS opus opusfile ogg vorbis vorbisenc vorbisfile crypto ssl)
  add_library(${lib} STATIC IMPORTED)
  set_target_properties(${lib} PROPERTIES IMPORTED_LOCATION ${EXTERNAL_DIR}/${ANDROID_ABI}/lib${lib}.a)
endforeach()
```

### FFmpeg shared libs (`.so`) in `jniLibs/` linked as shared IMPORTED

```cmake
add_library(${lib} SHARED IMPORTED)
set_target_properties(${lib} PROPERTIES IMPORTED_LOCATION ${JNI_LIBS_DIR}/${ANDROID_ABI}/lib${lib}.so)
```

### Include paths for Android (key paths)

```
common/cpp                        — for #include <audioapi/...>
android/src/main/cpp              — for Android-specific headers
external/include                  — Opus/Ogg/Vorbis/OpenSSL headers
external/include/opus             — nested Opus headers
external/include_ffmpeg           — FFmpeg headers
ReactCommon                       — JSI headers
ReactAndroid/src/main/jni/...     — TurboModule JNI headers
```

---

## iOS: `RNAudioAPI.podspec` — detailed analysis

### Subspecs split compilation

| Subspec | Sources | Special |
|---|---|---|
| `audioapi` | `common/cpp/audioapi/**/*.{cpp,h,hpp}` | Excludes FFmpeg sources if disabled |
| `audioapi/ios` | `ios/audioapi/**/*.{mm,h}` | ObjC++ platform layer |
| `audioapi/audioapi_dsp` | `common/cpp/audioapi/dsp/**/*.cpp` | Compiled with `-O3` |
| `audioapi/miniaudio_impl` | `utils/MiniaudioImplementation.cpp` | Compiled as `-x objective-c++` (required by miniaudio) |

### `miniaudio_impl` workaround

miniaudio's implementation file must be compiled as Objective-C++ on iOS (it uses Apple APIs). CocoaPods can't set per-file compiler flags the same way CMake can, so it gets its own subspec with `compiler_flags = "-x objective-c++"`.

### Prebuilt binaries linked via `-force_load`

All static libs (Opus, Ogg, Vorbis, OpenSSL) use `-force_load` to prevent dead-stripping of symbols needed at runtime but not referenced at link time:

```ruby
s.xcconfig = {
  'OTHER_LDFLAGS' => %W[
    -force_load #{lib_dir}/libopus.a
    -force_load #{lib_dir}/libogg.a
    ...
  ].join(" ")
}
```

### FFmpeg xcframeworks listed in `s.ios.vendored_frameworks`

CocoaPods handles embedding and signing automatically:

```ruby
s.ios.vendored_frameworks = $RN_AUDIO_API_FFMPEG_DISABLED ? [] : [
  'common/cpp/audioapi/external/ffmpeg_ios/libavcodec.xcframework',
  ...
]
```

### Header search paths split between pod target and consumers

- `pod_target_xcconfig` — `HEADER_SEARCH_PATHS` for compiling the library itself (Boost, Folly, Yoga, external headers)
- `xcconfig` — `HEADER_SEARCH_PATHS` for app targets consuming this pod (ReactCommon, dynamic framework dirs)

### `rnaa_utils.rb` — dynamic path resolution at `pod install` time

Resolves three paths that vary per machine/monorepo layout:

1. `react_native_common_dir` — ReactCommon path relative to Pods root
2. `dynamic_frameworks_audio_api_dir` — this pod's dir relative to Pods root
3. `dynamic_frameworks_worklets_dir` — worklets pod dir (only if enabled)

These vary per machine/monorepo layout, so they're computed at install time rather than hardcoded.

### System frameworks linked

```ruby
s.ios.frameworks = 'Accelerate', 'AVFoundation', 'MediaPlayer'
```

`Accelerate` enables `HAVE_ACCELERATE=1` for vDSP SIMD on iOS.

### Podfile (fabric-example)

Enables New Architecture: `ENV['RCT_NEW_ARCH_ENABLED'] = '1'` at the top. This is the only example app using the new arch — always test new features here.

The podfile resolves the minimum iOS version from RN's `min_ios_version_supported` helper — do not hardcode `14.0` in the Podfile itself.

---

## C++ Test Build — `common/cpp/test/CMakeLists.txt` — detailed analysis

### Design: completely standalone

No Gradle, no Xcode, no prebuilt Android libraries needed.

### Sources resolved from `node_modules` (not from monorepo source directly)

```cmake
set(ROOT ${CMAKE_SOURCE_DIR}/../../../../..)
set(REACT_NATIVE_AUDIO_API_DIR "${ROOT}/node_modules/react-native-audio-api")

file(GLOB_RECURSE RNAUDIOAPI_SRC CONFIGURE_DEPENDS
  "${REACT_NATIVE_AUDIO_API_DIR}/common/cpp/audioapi/*.cpp"
)
```

This means **tests build against the installed/published version of the library** in `node_modules`, not the local `packages/` source directly. In a monorepo with yarn workspaces, `node_modules/react-native-audio-api` symlinks to `packages/react-native-audio-api`, so this works fine locally.

### Excluded from tests (not relevant for unit testing)

```cmake
list(FILTER RNAUDIOAPI_SRC EXCLUDE REGEX ".*/audioapi/HostObjects/.*\\.cpp$")  # no JSI in tests
list(FILTER RNAUDIOAPI_SRC EXCLUDE REGEX ".*/Worklet.*Node\\.cpp$")            # worklets not compiled
list(REMOVE_ITEM RNAUDIOAPI_SRC ... "AudioContext.cpp")                         # needs real audio I/O
list(REMOVE_ITEM RNAUDIOAPI_SRC ... "FFmpegDecoding.cpp")                       # FFmpeg disabled in tests
```

### Compile definitions always set in tests

```cmake
add_compile_definitions(RN_AUDIO_API_ENABLE_WORKLETS=0)
add_compile_definitions(RN_AUDIO_API_TEST=1)
add_compile_definitions(RN_AUDIO_API_FFMPEG_DISABLED=1)
```

Use `RN_AUDIO_API_TEST` in source code to conditionally compile test-only hooks.

### Google Test auto-fetched if not installed

```cmake
find_package(GTest QUIET)
if(NOT GTest_FOUND)
  include(FetchContent)
  FetchContent_Declare(googletest URL https://github.com/google/googletest/archive/<sha>.zip)
  FetchContent_MakeAvailable(googletest)
endif()
```

### JSI headers included for compilation

JSI headers are included from `node_modules/react-native/ReactCommon/jsi` so core audio classes that depend on JSI types can compile even without a running JS engine.

### `MockAudioEventHandlerRegistry` — test fixture boilerplate

`test/src/MockAudioEventHandlerRegistry.h` provides a Google Mock implementation of `IAudioEventHandlerRegistry`. Pass it to `OfflineAudioContext` in every test fixture:

```cpp
class MyNodeTest : public ::testing::Test {
 protected:
  std::shared_ptr<MockAudioEventHandlerRegistry> eventRegistry;
  std::shared_ptr<OfflineAudioContext> context;

  void SetUp() override {
    eventRegistry = std::make_shared<MockAudioEventHandlerRegistry>();
    context = std::make_shared<OfflineAudioContext>(
        2, 5 * sampleRate, sampleRate, eventRegistry, RuntimeRegistry{});
    context->initialize();
  }
};
```

### `TestableXxx` pattern — white-box unit testing

Expose `processNode()` and internal state for white-box unit testing (since `processNode` is `protected`):

```cpp
class TestableGainNode : public GainNode {
 public:
  explicit TestableGainNode(std::shared_ptr<BaseAudioContext> ctx)
      : GainNode(ctx, GainOptions()) {}

  std::shared_ptr<AudioBuffer> processNode(
      const std::shared_ptr<AudioBuffer> &buf, int frames) override {
    return GainNode::processNode(buf, frames);  // call through to real implementation
  }
};
```

### Adding a new test file

1. Create `test/src/core/<category>/MyNodeTest.cpp`
2. `GLOB_RECURSE test_src "src/*.cpp"` picks it up automatically — no CMakeLists edit needed.
3. Verify `context->initialize()` is called in `SetUp()` if your node needs the context to be running.

