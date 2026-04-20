# Utilities API Reference — Full .hpp Documentation

> This file contains the detailed API documentation for template/inline utilities in `common/cpp/audioapi/utils/`, `common/cpp/audioapi/core/utils/`, and `common/cpp/audioapi/dsp/`.
>
> For a concise overview and when-to-use guidance, see [SKILL.md](SKILL.md).

---

## `SpscChannel.hpp` — lock-free SPSC channel

Bounded single-producer, single-consumer queue built on aligned atomics. **Do not use directly — prefer `CrossThreadEventScheduler` or `TaskOffloader`** unless you need fine-grained control.

```cpp
// Create a channel
auto [sender, receiver] = channels::spsc::channel<MyType>(capacity);
// Optional template params: OverflowStrategy, WaitStrategy

// Sender (producer thread)
sender.try_send(value);   // non-blocking, returns ResponseStatus
sender.send(value);       // blocks until space available

// Receiver (consumer thread)
ResponseStatus s = receiver.try_receive(value);  // non-blocking
T val = receiver.receive();                       // blocks until data available
```

**OverflowStrategy:**
- `WAIT_ON_FULL` (default) — `try_send` returns `CHANNEL_FULL` when full
- `OVERWRITE_ON_FULL` — overwrites oldest element (use with `BUSY_LOOP`)

**WaitStrategy** (affects blocking `send`/`receive`):
- `BUSY_LOOP` (default) — spin loop; lowest latency, highest CPU
- `YIELD` — `std::this_thread::yield()`; lower CPU, higher latency
- `ATOMIC_WAIT` — `std::atomic::wait()`; good for long waits (destructor threads)

**ResponseStatus:** `SUCCESS`, `CHANNEL_FULL`, `CHANNEL_EMPTY`, `SKIP_DUE_TO_OVERWRITE`.

Real capacity rounds up to next power of two.

---

## `CrossThreadEventScheduler.hpp` — JS→audio event queue

High-level wrapper over `SpscChannel` for scheduling lambdas from the JS thread to be executed on the audio thread. This is **the standard way** to send updates from JS to audio.

```cpp
// Declaration (e.g. in AudioNode or AudioParam)
CrossThreadEventScheduler<MyNode> eventScheduler_{/*capacity=*/64};

// JS thread — schedule a change
eventScheduler_.scheduleEvent([newValue](MyNode &node) {
    node.setSomething(newValue);
});
// returns false if queue is full (drop the event or handle it)

// Audio thread — drain all pending events before processNode()
eventScheduler_.processAllEvents(*this);
```

Template parameter `T` is the type passed by reference to each event lambda. For `AudioNode` subclasses, `T = BaseAudioContext` (events are called with the context). For `AudioParam`, `T = AudioParam` itself.

Not copyable. Use `std::shared_ptr` if shared ownership is needed across threads.

---

## `AlignedAllocator.hpp` — aligned STL allocator

STL-compatible allocator that guarantees `N`-byte alignment. Default alignment is 16 bytes (SIMD-friendly).

```cpp
// 16-byte aligned float vector (default)
std::vector<float, AlignedAllocator<float>> buf(1024);

// Custom alignment (e.g. 64 bytes for AVX-512)
std::vector<float, AlignedAllocator<float, 64>> buf(1024);
```

Use when creating buffers that will be processed by SIMD code in `VectorMath`.

---

## `MoveOnlyFunction.hpp` — non-copyable function wrapper

Backport of C++23 `std::move_only_function`. Use instead of `std::function` when the callable captures a move-only type (e.g. a `unique_ptr`).

```cpp
audioapi::move_only_function<void(BaseAudioContext&)> event = [ptr = std::move(uniquePtr)](BaseAudioContext &) {
    ptr->doSomething();
};
// move into the scheduler
eventScheduler_.scheduleEvent(std::move(event));
```

Not copyable. Throws `std::bad_function_call` if invoked when empty.

---

## `Result.hpp` — Rust-style Result<T,E>

Represents either success (`Ok`) or error (`Err`). Used at API boundaries (e.g. `AudioRecorder::start()`).

```cpp
// Creating
Result<std::string, std::string> res = Ok(std::string("path/to/file"));
Result<std::string, std::string> err = Err(std::string("permission denied"));

// Checking and extracting
if (result.is_ok()) {
    std::string path = std::move(result).unwrap();
}
if (result.is_err()) {
    std::string msg = std::move(result).unwrap_err();
}

// With default
std::string path = std::move(result).unwrap_or(std::string("default"));

// Transforming
auto mapped     = std::move(result).map([](std::string s) { return s.size(); });
auto mapped_err = std::move(result).map_err([](std::string e) { return -1; });

// Chaining
auto chained = std::move(result).and_then([](std::string s) -> Result<int, std::string> {
    return Ok(42);
});
```

Use `NoneType` / `None` for void variants: `Result<NoneType, std::string>`.

---

## `RingBiDirectionalBuffer.hpp` — compile-time capacity ring deque

Non-thread-safe bounded ring buffer with push/pop from both ends. Capacity is a **compile-time** power-of-two template parameter.

```cpp
RingBiDirectionalBuffer<MyEvent, 128> queue;  // capacity must be power of 2

queue.pushBack(event);   // add to back, returns false if full
queue.pushFront(event);  // add to front, returns false if full

MyEvent e;
queue.popFront(e);       // remove from front into e, returns false if empty
queue.popBack(e);        // remove from back into e, returns false if empty
queue.popFront();        // discard front element
queue.popBack();         // discard back element

const MyEvent &front = queue.peekFront();   // const peek, no removal
MyEvent &back = queue.peekBackMut();        // mutable peek

queue.isEmpty(); queue.isFull();
queue.size(); queue.getCapacity();          // real capacity = getCapacity() - 1
```

Used for `AudioParamEventQueue` (parameter automation event storage on the audio thread).

---

## `TaskOffloader.hpp` — worker thread with SPSC input

Spawns a dedicated worker thread that processes items from a SPSC channel. Use when you need to offload a recurring task (e.g. file writing, decoding) to a dedicated thread.

```cpp
// T must be default_initializable
TaskOffloader<AudioData,
    OverflowStrategy::WAIT_ON_FULL,
    WaitStrategy::ATOMIC_WAIT> offloader{
    /*capacity=*/64,
    [](AudioData data) {    // runs on worker thread
        writeToFile(data);
    }
};

// Push work from any thread
auto *sender = offloader.getSender();
sender->send(audioData);
```

Not copyable or movable. Destructor sends a dummy value to unblock the worker and joins the thread.

---

## `Benchmark.hpp` — timing utilities (dev/debug only)

```cpp
#include <audioapi/utils/Benchmark.hpp>

// One-shot timing — returns nanoseconds
double ns = audioapi::benchmarks::getExecutionTime([]{ doSomething(); });

// Running average with logging (NOT thread-safe, not for production)
audioapi::benchmarks::logAvgExecutionTime("render quantum", []{ renderAudio(); });
```

**Do not leave `logAvgExecutionTime` in production code.**

---

## `AudioDestructor.hpp` — off-thread destruction

Offloads `shared_ptr` destruction to a dedicated worker thread. Use for any object whose destructor may block or deallocate large buffers — both are forbidden on the audio thread.

```cpp
// Typically owned by AudioContext or BaseAudioContext
AudioDestructor<AudioNode> destructor;

// Audio thread — hand off a node for destruction without blocking
bool ok = destructor.tryAddForDeconstruction(std::move(nodePtr));
// if false (queue capacity=1024 is full), the shared_ptr is NOT moved
```

The worker thread blocks on receive and drops the `shared_ptr`, triggering the destructor off the audio thread.

---

## `ParamChangeEvent.hpp` — AudioParam automation event

Represents a single Web Audio API automation command (`setValueAtTime`, `linearRampToValueAtTime`, etc.). Stores timing, start/end values, and a `calculateValue_` lambda. Move-only.

```cpp
ParamChangeEvent event(
    startTime, endTime, startValue, endValue,
    [](double start, double end, float startVal, float endVal, double currentTime) -> float {
        return computedValue;  // interpolation logic
    },
    ParamChangeEventType::LINEAR_RAMP
);

event.getStartTime(); event.getEndTime();
event.getStartValue(); event.getEndValue();
event.getCalculateValue();   // the interpolation lambda
event.getType();

// Mutators used by AudioParamEventQueue when adjusting event boundaries:
event.setEndTime(t); event.setStartValue(v); event.setEndValue(v);
```

Used exclusively within `AudioParamEventQueue`. Do not construct outside of `AudioParam` scheduling methods.

---

## `AudioUtils.hpp` — inline DSP math

```cpp
#include <audioapi/dsp/AudioUtils.hpp>
using namespace audioapi::dsp;

size_t frame = timeToSampleFrame(time, sampleRate);   // double → size_t
double t     = sampleFrameToTime(frame, sampleRate);  // int → double

// Linear interpolation with edge-case handling (extrapolates at end of array)
float v = linearInterpolate(span, firstIndex, secondIndex, factor);

float db     = linearToDecibels(linearValue);    // 20 * log10(v)
float linear = decibelsToLinear(dbValue);        // 10^(db/20)
```

