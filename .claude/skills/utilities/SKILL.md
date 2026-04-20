---
name: utilities
description: >
  Overview of all shared utility helpers, data structures, and DSP primitives available in
  react-native-audio-api. Covers C++ utilities in common/cpp/audioapi/utils/,
  common/cpp/audioapi/core/utils/, common/cpp/audioapi/dsp/, and TypeScript utilities in
  src/utils/. Use this skill before writing new math, buffer management, or cross-thread code —
  check if a utility already exists. Trigger phrases: "add a new utility", "what helpers are
  available", "cross-thread communication", "audio buffer", "DSP math", "circular buffer",
  "lock-free queue", "off-thread", "SIMD buffer", "time conversion", "parameter automation",
  "audio constants", "TypeScript utils".
---

# Skill: Utilities

## `common/cpp/audioapi/utils/` — Core data structures & primitives

### `AudioArray.h` — float audio array (read header for full API)

Single-channel float buffer. The foundational data type. Use for any per-channel audio data.

Key operations: `zero()`, `sum(source, gain)`, `multiply(source)`, `copy(source)`, `copyReverse(...)`, `copyTo(float*)`, `copyWithin(...)`, `scale(float)`, `normalize()`, `getMaxAbsValue()`, `computeConvolution(kernel)`.

Access: `operator[]`, `begin()`/`end()`, `span()`, `subSpan(length, offset)`.

Constructors copy data — `AudioArray` always owns its buffer.

---

### `AudioBuffer.h` — multi-channel container (read header for full API)

Holds N channels of `AudioArrayBuffer`. Handles up/down-mixing automatically on `sum()` and `copy()`.

Key operations: `getChannel(index)`, `getSharedChannel(index)`, `zero()`, `sum(source, interpretation)`, `copy(source)`, `deinterleaveFrom(float*, frames)`, `interleaveTo(float*, frames)`, `normalize()`, `scale(float)`.

Channel layout constants: `ChannelMono=0`, `ChannelLeft=0`, `ChannelRight=1`, `ChannelCenter=2`, `ChannelLFE=3`, `ChannelSurroundLeft=4`, `ChannelSurroundRight=5`.

`getChannel()` returns a non-owning `AudioArray*`. `getSharedChannel()` returns `shared_ptr<AudioArrayBuffer>` (use for JSI transfer).

---

### `AudioArrayBuffer.hpp` — JSI-transferable audio array

`AudioArray` + `jsi::MutableBuffer`. Allows zero-copy transfer of audio data to JS as an `ArrayBuffer`. Use in `getChannelData()` patterns.

```cpp
// Typical usage in HostObject — no copy, JS sees the native memory
auto audioArrayBuffer = audioBuffer_->getSharedChannel(channel);
auto arrayBuffer = jsi::ArrayBuffer(runtime, audioArrayBuffer);
auto float32Array = runtime.global()
    .getPropertyAsFunction(runtime, "Float32Array")
    .callAsConstructor(runtime, arrayBuffer)
    .getObject(runtime);
float32Array.setExternalMemoryPressure(runtime, audioArrayBuffer->size());
```

---

### `CircularAudioArray.h` — circular float buffer (read header)

`AudioArray` subclass acting as a circular queue for streaming audio. **Not thread-safe** — use only from one thread.

Key operations:
- `push_back(AudioArray&, size)` / `push_back(float*, size)` — write frames
- `pop_front(AudioArray&, size)` / `pop_front(float*, size)` — read oldest frames
- `pop_back(AudioArray&, size, offset)` — read newest frames
- `getNumberOfAvailableFrames()` — how many frames ready to read

Used in delay lines and streaming buffers.

---

### `CircularOverflowableAudioArray.h` — overwritable circular buffer

Like `CircularAudioArray` but overwrites oldest data when full instead of rejecting. Use for recording input where you always want the latest data, not the oldest.

---

### `SpscChannel.hpp` — lock-free SPSC channel

Bounded single-producer, single-consumer queue built on aligned atomics. **Do not use directly — prefer `CrossThreadEventScheduler` or `TaskOffloader`** unless you need fine-grained control.

For full API see [api.md](api.md#spscchannelhpp--lock-free-spsc-channel).

---

### `CrossThreadEventScheduler.hpp` — JS→audio event queue

High-level wrapper over `SpscChannel` for scheduling lambdas from the JS thread to be executed on the audio thread. This is **the standard way** to send updates from JS to audio.

For full API see [api.md](api.md#crossthreadeventschedulerhpp--jsaudio-event-queue).

---

### `AlignedAllocator.hpp` — aligned STL allocator

STL-compatible allocator that guarantees `N`-byte alignment (default 16 bytes, SIMD-friendly). Use when creating buffers that will be processed by SIMD code in `VectorMath`.

For full API see [api.md](api.md#alignedallocatorhpp--aligned-stl-allocator).

---

### `MoveOnlyFunction.hpp` — non-copyable function wrapper

Backport of C++23 `std::move_only_function`. Use instead of `std::function` when the callable captures a move-only type (e.g. a `unique_ptr`).

For full API see [api.md](api.md#moveonlyfunctionhpp--non-copyable-function-wrapper).

---

### `Result.hpp` — Rust-style Result<T,E>

Represents either success (`Ok`) or error (`Err`). Use at API boundaries (e.g. `AudioRecorder::start()`). Use `NoneType` / `None` for void variants.

For full API see [api.md](api.md#resulthpp--rust-style-resultte).

---

### `RingBiDirectionalBuffer.hpp` — compile-time capacity ring deque

Non-thread-safe bounded ring buffer with push/pop from both ends. Capacity is a **compile-time** power-of-two template parameter. Used for `AudioParamEventQueue`.

For full API see [api.md](api.md#ringbidirectionalbufferhpp--compile-time-capacity-ring-deque).

---

### `TaskOffloader.hpp` — worker thread with SPSC input

Spawns a dedicated worker thread that processes items from a SPSC channel. Use when you need to offload a recurring task (e.g. file writing, decoding) to a dedicated thread.

For full API see [api.md](api.md#taskoffloaderhpp--worker-thread-with-spsc-input).

---

### `Benchmark.hpp` — timing utilities (dev/debug only)

Use `getExecutionTime()` for one-shot nanosecond timing. **Do not leave `logAvgExecutionTime` in production code.**

For full API see [api.md](api.md#benchmarkhpp--timing-utilities-devdebug-only).

---

### `UnitConversion.h` — byte unit constants

```cpp
audioapi::KB_IN_BYTES   // 1024.0
audioapi::MB_IN_BYTES   // 1024 * 1024.0
audioapi::GB_IN_BYTES   // 1024^3.0
```

---

## `common/cpp/audioapi/core/utils/` — Node and context utilities

### `Constants.h` — global audio constants

```cpp
RENDER_QUANTUM_SIZE   = 128      // frames per render block — never hardcode 128
MAX_FFT_SIZE          = 32768
MAX_CHANNEL_COUNT     = 32
OCTAVE_RANGE          = 1200     // cents per octave
PI                    = std::numbers::pi_v<float>
MOST_POSITIVE_SINGLE_FLOAT / MOST_NEGATIVE_SINGLE_FLOAT
PROMISE_VENDOR_THREAD_POOL_WORKER_COUNT = 4
```

---

### `AudioDestructor.hpp` — off-thread destruction

Offloads `shared_ptr` destruction to a dedicated worker thread. Use for any object whose destructor may block or deallocate large buffers — both are forbidden on the audio thread.

For full API see [api.md](api.md#audiodestructorhpp--off-thread-destruction).

---

### `ParamChangeEvent.hpp` — AudioParam automation event

Represents a single Web Audio API automation command (`setValueAtTime`, `linearRampToValueAtTime`, etc.). Move-only. Used exclusively within `AudioParamEventQueue` — do not construct outside of `AudioParam` scheduling methods.

For full API see [api.md](api.md#paramchangeeventhpp--audioparam-automation-event).

---

### `AudioParamEventQueue.h` — sorted automation event queue

Stores and processes `ParamChangeEvent` objects in time order on the audio thread. Read the header for full API.

---

### `AudioGraphManager.h` — thread-safe graph mutation queue

Queues connect/disconnect operations from the JS thread for application before each render pass. Do not call its methods directly — go through `AudioNode::connect()`/`disconnect()`. Read the header for implementation details.

---

### `Locker.h` — nullable mutex RAII wrapper

RAII mutex wrapper that can hold `nullptr` (no-op). Supports `Locker::tryLock(mutex)` for non-blocking acquisition. **Do not use on the audio thread.** Locks are forbidden in `processNode()`.

---

### Other `core/utils/` classes

- **`AudioDecoder.h`** — decodes audio files to `AudioBuffer` (FFmpeg, conditional). Read the header.
- **`AudioFileWriter.h`** — writes PCM to audio files. Read the header.
- **`AudioStretcher.h`** — pitch/time stretching (signalsmith-stretch). Read the header.
- **`AudioRecorderCallback.h`** — callback adapter for the platform recorder. Internal.
- **`worklets/WorkletsRunner.h`** — manages JS worklet execution on the audio thread. Internal.

---

## `common/cpp/audioapi/dsp/` — DSP helpers

### `AudioUtils.hpp` — inline DSP math

Provides `timeToSampleFrame()`, `sampleFrameToTime()`, `linearInterpolate()`, `linearToDecibels()`, `decibelsToLinear()`.

For full API see [api.md](api.md#audioutilshpp--inline-dsp-math).

---

### `VectorMath.h` — SIMD-optimized vector math

SIMD-accelerated array operations (ARM NEON / x86 SSE2). Use for per-channel hot-path processing. Read the header for available functions before writing manual loops.

---

### `FFT.h` / `Convolver.h` / `Resampler.h` / `WaveShaper.h` / `Windows.hpp`

Higher-level DSP blocks. Read each header before use.

---

## `src/utils/` — TypeScript utilities

### `paths.ts`

```ts
import { isRemoteSource, isBase64Source, isDataBlobString } from './utils/paths';

isRemoteSource(url)      // true if starts with http:// or https://
isBase64Source(data)     // true if 'data:audio/...;base64,...'
isDataBlobString(data)   // true if starts with 'blob:'
```

Use before passing a URL/path to decoder or streaming APIs to determine the source type.

---

### `filePresets.ts`

```ts
import FilePreset from './utils/filePresets';

FilePreset.Low        // 22050 Hz, 48kbps,  16-bit
FilePreset.Medium     // 44100 Hz, 128kbps, 16-bit
FilePreset.High       // 48000 Hz, 192kbps, 24-bit
FilePreset.Lossless   // 48000 Hz, 320kbps, 24-bit, FLAC L8
```

Use when configuring `AudioRecorder` file output instead of building `FilePresetType` objects manually.

---

*Maintenance: see [maintenance.md](maintenance.md).*
