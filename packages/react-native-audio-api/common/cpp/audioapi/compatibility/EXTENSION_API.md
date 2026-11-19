# react-native-audio-api — C++ Extension API

Extension packages integrate with react-native-audio-api in C++ through a **single**
public header:

```cpp
#include <audioapi/compatibility/StableAPI.h>
```

Do **not** include any other `audioapi/...` headers. Types and helpers needed for
extensions are pulled in by `StableAPI.h`. Everything else is internal.

---

## Android (Prefab)

| Item | Value |
|---|---|
| Gradle module | `:react-native-audio-api` |
| Prefab package | `react-native-audio-api` |
| CMake `find_package` | `find_package(react-native-audio-api REQUIRED CONFIG)` |
| Link target | `react-native-audio-api::react-native-audio-api` |
| Shared library | `libreact-native-audio-api.so` |
| **Only public include** | `<audioapi/compatibility/StableAPI.h>` |

### Example `CMakeLists.txt`

```cmake
find_package(react-native-audio-api REQUIRED CONFIG)

target_link_libraries(my-extension
  react-native-audio-api::react-native-audio-api
  ReactAndroid::jsi
  # ...
)
```

Prefab ships `audioapi/**/*.{h,hpp}` headers except `AudioAPIModuleInstaller.h` and
`android/AudioAPIModule.h` (include-only,
same pattern as react-native-worklets). Extension code must still include only `<audioapi/compatibility/StableAPI.h>`;
a type an extension needs belongs in `StableAPI.h`.

---

## iOS (CocoaPods)

| Item | Value |
|---|---|
| Pod | `RNAudioAPI` |
| Podspec dependency | `s.dependency 'RNAudioAPI'` |
| npm peer dependency | `react-native-audio-api` (in extension `package.json`) |
| **Only public include** | `<audioapi/compatibility/StableAPI.h>` |

### Example `*.podspec`

Extension packages ship their own pod (e.g. `RNAudioWorklets`) and **must** depend on
`RNAudioAPI` so CocoaPods links the native library and exposes public headers:

```ruby
Pod::Spec.new do |s|
  s.name         = "MyAudioExtension"
  # ...

  s.source_files = "common/cpp/myextension/**/*.{cpp,h}"
  s.header_dir   = "myextension"
  s.header_mappings_dir = "common/cpp/myextension"

  s.dependency 'RNAudioAPI'
  s.dependency 'React-jsi'

  s.pod_target_xcconfig = {
    "HEADER_SEARCH_PATHS" => [
      '"$(PODS_ROOT)/Headers/Public/RNAudioAPI"',
      # React / Folly paths (see react-native-audio-worklets/RNAudioWorklets.podspec)...
    ].join(' '),
    "CLANG_CXX_LANGUAGE_STANDARD" => "c++20",
  }

  install_modules_dependencies(s)
end
```

Reference: `packages/react-native-audio-worklets/RNAudioWorklets.podspec`.

### `pod_target_xcconfig` (header search paths)

`s.dependency 'RNAudioAPI'` usually adds the RNAudioAPI header search paths
automatically. You may still set this explicitly so `<audioapi/compatibility/StableAPI.h>` is
obvious in the podspec:

```ruby
"HEADER_SEARCH_PATHS" => [
  '"$(PODS_ROOT)/Headers/Public/RNAudioAPI"',
  # React / Folly paths...
].join(' ')
```

---

## API available through `StableAPI.h`

| Type / header (transitive) | Purpose |
|---|---|
| `audioapi::BaseAudioContextHostObject` | Resolve `AudioContext` from a JSI object via `getContext()` |
| `audioapi::AudioNodeHostObject` | Base HostObject for custom nodes exposed to JS |
| `audioapi::BaseAudioContext` | Context handle; `getGraph()`, scheduling, sample rate |
| `audioapi::AudioNode` | Base class for custom `AudioNode` implementations |
| `audioapi::AudioScheduledSourceNodeHostObject` | HostObject base for scheduled source nodes (`start`/`stop`/`onended`) |
| `audioapi::AudioScheduledSourceNode` | Base class for scheduled sources (e.g. worklet generators) |
| `audioapi::AudioConsumer` | Base class for a custom consumer: receives every recorded buffer off the audio thread |
| `audioapi::AudioConsumerHostObject` | HostObject that hands an `AudioConsumer` to JS `AudioRecorder.addConsumer`; subclass it to expose properties |
| `audioapi::StreamFormat` (via `encoding/StreamFormat.h`) | The recorder's input format an `AudioConsumer` is prepared for |
| `audioapi::Result` (via `utils/Result.hpp`) | Success or error message returned by `AudioConsumer` calls |
| `audioapi::typedAudioNode<NodeT>()` (via `HostObjects/TypedAudioNodePtr.hpp`) | Downcast a HostObject's graph node to its concrete `AudioNode` type |
| Node option structs (via `types/NodeOptions.h`) | `AudioScheduledSourceNodeOptions` for scheduled source HostObjects; `AnalyserOptions` defaults such as `kDefaultSmoothingTimeConstant` |
| `audioapi::utils::graph::Graph` | Audio graph owned by the context |
| `RENDER_QUANTUM_SIZE` etc. (via `core/utils/Constants.h`) | Frame-size and engine constants |
| `audioapi::AudioBuffer` | Multi-channel float buffer |
| `audioapi::DSPAudioBuffer` / `audioapi::DSPAudioArray` | SIMD-aligned multi-channel / single-channel buffers for DSP work |
| `audioapi::CircularDSPAudioArray` (via `utils/CircularArray.hpp`) | Ring of the newest frames; `push_back` then `pop_back` the latest window, as `AnalyserNode` does |
| `audioapi::dsp::SpectrumAnalyser` (via `dsp/SpectrumAnalyser.h`) | The FFT magnitude analysis behind `AnalyserNode`, for consumers that report a spectrum |
| `audioapi::AudioArrayBuffer` | Single-channel float buffer; implements `jsi::MutableBuffer` for zero-copy JSI `ArrayBuffer` handoff |
| `DELETE_COPY_AND_MOVE` (via `utils/Macros.h`) | Macro to delete copy and move constructors/assignment for extension-owned classes |

---

## Integration pattern

1. **Separate package** with its own pod / pure C++ TurboModule / CMake target.
2. **Declare dependencies:**
   - **iOS:** `s.dependency 'RNAudioAPI'` in your podspec (see example above).
   - **Android:** `find_package(react-native-audio-api)` + link
     `react-native-audio-api::react-native-audio-api` in CMake.
   - **npm:** `react-native-audio-api` as a `peerDependency` in `package.json`.
3. **`#include <audioapi/compatibility/StableAPI.h>`** — the only audio-api C++ include.
4. **Install JSI bindings** from your own TurboModule (do not modify
   `AudioAPIModuleInstaller`).
5. **Resolve the context** from JS:

   ```cpp
   auto host = contextObject.getHostObject<audioapi::BaseAudioContextHostObject>(runtime);
   auto context = host->getContext();
   auto graph = context->getGraph();
   ```

6. **Build a custom node** by subclassing `audioapi::AudioNode` and wrapping it in a
   HostObject that extends `audioapi::AudioNodeHostObject`, then connect it through the
   graph like built-in nodes.

Reference implementation: `packages/react-native-audio-worklets`.

---

## Custom audio consumer

An extension can receive every buffer an `AudioRecorder` captures, with or without an audio
context:

1. **Subclass `audioapi::AudioConsumer`.** The recorder calls it on a worker thread, never on
   the audio thread and never two calls at once:
   - `prepare(StreamFormat)`: a session starts (or the consumer joins a running one); size buffers
     for `maxFramesPerBuffer` frames in `layout.channelCount` channels.
   - `consume(const AudioBuffer &frames, size_t numFrames)`: the next recorded frames, valid
     only for the call. Keep it short: every consumer shares one worker, so a slow `consume`
     delays the others and makes the recorder drop buffers once its queue is full. Hand slow work
     to a thread of your own. A returned `Err` is reported to JS `onError`.
   - `reprepare(StreamFormat)`: the input format changed mid-session. Closes and prepares again
     by default.
   - `close()`: the session ended or the consumer was removed. It may be prepared again.
   - `invalidate()`: drop every JS-owned reference (functions, runtimes). Called on the JS thread,
     possibly while `consume` runs.
2. **Wrap it in `audioapi::AudioConsumerHostObject`** (or a subclass) and return that object to
   JS. Its destructor calls `invalidate()`.
3. **In TS, subclass `AudioConsumer`** from `react-native-audio-api`, passing the HostObject to
   its constructor, so users can call `recorder.addConsumer(consumer)`. Keep your own typed
   reference to the HostObject for its properties: the base class's `consumer` field is internal
   and left out of the published types.

Reference implementation: `RecorderWorklet` in `packages/react-native-audio-worklets`.

---

## Explicitly not supported

- Any `#include` other than `<audioapi/compatibility/StableAPI.h>`
- `audioapi/AudioAPIModuleInstaller.h` — core global install; not for extensions
- `audioapi/libs/**`, `audioapi/external/**`, and `audioapi/dsp/**` other than `SpectrumAnalyser`
- Concrete built-in node HostObjects — reference only, not a stable base API

---

## Versioning

Declare a `react-native-audio-api` **peer dependency** in `package.json` with a
semver range matching the API you target.
