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

Prefab ships `audioapi/**/*.{h,hpp}` headers (include-only, same pattern as
react-native-worklets). Extension code must still include only `<audioapi/compatibility/StableAPI.h>`.

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
| `audioapi::AudioScheduledSourceNodeHostObject` | HostObject base for scheduled source nodes (`start`/`stop`/`onEnded`) |
| `audioapi::AudioScheduledSourceNode` | Base class for scheduled sources (e.g. worklet generators) |
| `AudioScheduledSourceNodeOptions` (via `types/NodeOptions.h`) | Options for scheduled source HostObjects |
| `audioapi::utils::graph::Graph` | Audio graph owned by the context |
| `RENDER_QUANTUM_SIZE` etc. (via `core/utils/Constants.h`) | Frame-size and engine constants |
| `audioapi::AudioBuffer` | Multi-channel float buffer |
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

## Explicitly not supported

- Any `#include` other than `<audioapi/compatibility/StableAPI.h>`
- `audioapi/AudioAPIModuleInstaller.h` — core global install; not for extensions
- `audioapi/libs/**`, `audioapi/dsp/**`, `audioapi/external/**`
- Concrete built-in node HostObjects — reference only, not a stable base API

---

## Versioning

Declare a `react-native-audio-api` **peer dependency** in `package.json` with a
semver range matching the API you target.
