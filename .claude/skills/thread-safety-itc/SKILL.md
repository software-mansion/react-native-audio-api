---
name: thread-safety-itc
description: >
  Audio thread safety rules, lock-free inter-thread communication patterns, and the audio event
  system in react-native-audio-api. Covers the three-thread model (JS / audio / worker),
  CrossThreadEventScheduler for JS→audio scheduling, IAudioEventHandlerRegistry for audio→JS events,
  AudioGraphManager for graph mutations, shadow state vs atomics decision table, TaskOffloader for
  off-thread work, and SpscChannel low-level API. Use when implementing cross-thread data flow,
  adding audio events, debugging thread-safety crashes or data races, or deciding which ITC
  primitive to use.
  Trigger phrases: "lock-free", "SPSC", "thread safety", "ITC", "cross-thread", "audio thread race",
  "scheduleAudioEvent", "invokeHandlerWithEventBody", "TaskOffloader", "off-thread",
  "SpscChannel", "CrossThreadEventScheduler", "shadow state", "atomic".
---

# Skill: Thread Safety & Inter-Thread Communication

Three threads interact in this codebase. Every line of code that crosses a thread boundary must use the correct primitive or it is a bug.

**When in doubt about which ITC primitive to use → go to the Decision Table below first.**

---

## The Three Threads

| Thread | Alias | Runs |
|---|---|---|
| React Native JS thread | "JS thread" | User code, HostObject methods, `scheduleAudioEvent` calls |
| Audio thread | "audio thread" | `processNode()` — driven by Oboe (Android) / CoreAudio (iOS) |
| Worker threads | "off-thread" | FFmpeg decoding, file I/O, `TaskOffloader` tasks |

**Audio thread is real-time.** It has a hard deadline (~3ms at 44100 Hz, 128 frames). Missing it causes audible glitches.

---

## Audio Thread Contract

`processNode()` **MUST NOT**:
- Allocate or free memory (`new`, `delete`, `malloc`, `free`, any `push_back` that grows)
- Acquire any mutex (`std::mutex`, `std::lock_guard`, `std::unique_lock`)
- Make blocking syscalls (file I/O, socket I/O, `sleep`, `wait`)
- Call into JavaScript (no JSI calls, no `callInvoker_->invokeSync()`)
- Throw exceptions

**Preallocate everything in the constructor (JS thread).** The audio thread only uses what was already allocated.

---

## JS → Audio: `CrossThreadEventScheduler`

The standard way to send property updates from JS to the audio thread.

```cpp
// JS thread (HostObject setter):
auto oscillatorNode = std::static_pointer_cast<OscillatorNode>(node_);
auto event = [oscillatorNode, type](BaseAudioContext &) {
  oscillatorNode->setType(type);   // runs on audio thread
};
oscillatorNode->scheduleAudioEvent(std::move(event));
```

`scheduleAudioEvent()` is defined on `AudioNode`. It enqueues a lambda into the node's `CrossThreadEventScheduler<BaseAudioContext>`. The audio thread drains the queue at the start of each render cycle.

**Never assume immediate consistency** — by the time the audio thread processes the event, several render quanta may have passed.

---

## Audio → JS: `IAudioEventHandlerRegistry`

Send events from the audio thread back to JS (e.g. `ended`, `loopEnded`, `positionChanged`).

```cpp
// Audio thread: fire event with no payload
audioEventHandlerRegistry_->invokeHandlerWithEventBody(AudioEvent::ENDED, {});

// Audio thread: fire event with payload
audioEventHandlerRegistry_->invokeHandlerWithEventBody(
    AudioEvent::POSITION_CHANGED, {{"position", currentPosition}});
```

Internally calls `callInvoker_->invokeAsync()` — safe to call from the audio thread.

### Callback ID pattern

Callbacks are stored as `std::atomic<uint64_t>` on the C++ node. `0` means no listener.

```cpp
// C++ node header
std::atomic<uint64_t> onEndedCallbackId_{0};

// HostObject destructor — MUST clear to prevent firing into a destroyed JS function
~AudioScheduledSourceNodeHostObject() {
  auto node = std::static_pointer_cast<AudioScheduledSourceNode>(node_);
  node->setOnEndedCallbackId(0);
}
```

**Always clear callback IDs in the HostObject destructor.**

---

## Graph Mutations: `AudioGraphManager`

Connect/disconnect operations queue via `AudioGraphManager` (its own internal SPSC channel). The audio thread calls `graphManager_->preProcessGraph()` before each render pass to apply pending changes.

Do not call `AudioGraphManager` directly — go through `AudioNode::connect()` / `disconnect()`.

---

## Decision Table

| Scenario | Correct pattern |
|---|---|
| JS sets a property → audio thread reads it | Shadow state in HostObject + `scheduleAudioEvent` |
| Audio thread fires an event → JS callback | `invokeHandlerWithEventBody()` |
| JS connects/disconnects nodes | `AudioNode::connect()` → `AudioGraphManager` |
| Property written by audio thread, JS reads it | `std::atomic<T>` on C++ node; getter reads directly |
| Non-primitive, can be written by audio thread | Triple buffer (see `AnalyserNode` for reference) |
| CPU-heavy work, must not block JS or audio | `TaskOffloader` on a dedicated worker thread |

---

## Off-Thread Work: `TaskOffloader`

For work that would block both the JS thread and the audio thread (decoding, file writing):

```cpp
TaskOffloader<MyWorkItem> offloader([](MyWorkItem item) {
  // runs on dedicated worker thread — allocs OK, blocking I/O OK
  item.process();
});
offloader.scheduleTask(std::move(workItem));
```

See the `utilities` skill for full API.

---

## Common Mistakes

- **Reading `node_->field_` in a getter** when that field is written by the audio thread → use shadow state or atomics.
- **Calling `node_->method()` directly from a setter** → always schedule via `scheduleAudioEvent`.
- **Not clearing callback IDs in the HostObject destructor** → audio thread fires into destroyed JS function.
- **`std::vector::push_back` in `processNode()`** → may allocate; preallocate in constructor.
- **`std::mutex` anywhere in `processNode()`** → deadlock risk and real-time violation.
- **Copying `shared_ptr` inside `processNode()`** — increments atomic refcount; capture before entering hot path.

---

*Maintenance: see [maintenance.md](maintenance.md).*
