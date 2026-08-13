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

## Audio → JS: `IAudioEventHandlerRegistry` and `EventCaller`

Send events from the audio thread back to JS (e.g. `ended`, `loopEnded`, `positionChanged`).

**Prefer `EventCaller<AudioEvent::X>`** — a small RAII helper templated on the event type. `dispatch()` requires a payload matching `EventPayloadFor<AudioEvent::X, Payload>` (see `AudioEventPayloadMapping.hpp`).

```cpp
// Node member (composition — one EventCaller per event)
EventCaller<AudioEvent::ENDED> onEndedEvent_{context->getAudioEventHandlerRegistry()};

// JS thread: HostObject setter forwards to a named method
void assignOnEndedCallbackId(uint64_t id) { onEndedEvent_.assignCallbackId(id); }

// Audio thread: fire when playback ends (EmptyPayload only for ENDED)
onEndedEvent_.dispatchEmpty();

// With typed payload — compile error if payload does not match the event
errorEvent_.dispatch(StringPayload{.name = "message", .reason = message});
positionChangedEvent_.dispatch(DoubleValuePayload{.value = position});
```

Throttled events (e.g. `positionChanged`) use `PositionChangedDispatcher`, which **contains** an `EventCaller` plus interval/flush logic — still composition, not node inheritance from `EventCaller`.

**Unregister lifecycle:**
- `EventCaller::~EventCaller()` unregisters if `callbackId != 0` when the node is destroyed
- `assignCallbackId(0)` unregisters when JS sets `onX = null`
- **HostObject destructor must call `assignOnXCallbackId(0)`** for every event wired on that node — the C++ node can outlive the HostObject, and the registry JSI function may be invalid after GC
- Clear callbacks in the **most-derived HostObject first** (each layer clears its own events; base destructors run afterward and clear parent events)
- Do **not** inherit `EventCaller` on multiple node bases (ambiguous API); add member `EventCaller` fields per event instead

Low-level registry API (used internally by `EventCaller`):

```cpp
audioEventHandlerRegistry_->dispatchEvent(AudioEvent::ENDED, callbackId, AudioEventPayload{...});
```

Internally calls `callInvoker_->invokeAsync()` — safe to call from the audio thread via `dispatchEvent`.

---

## Graph Mutations: `AudioGraphManager`

Connect/disconnect operations queue via `AudioGraphManager` (its own internal SPSC channel). The audio thread calls `graphManager_->preProcessGraph()` before each render pass to apply pending changes.

Do not call `AudioGraphManager` directly — go through `AudioNode::connect()` / `disconnect()`.

### Processable state is audio-thread-only

Per-quantum processable state (`ALWAYS_`/`CONDITIONAL_`/`NOT_PROCESSABLE`) is derived exclusively on the audio thread by `AudioGraph::settleProcessableState()` (a reverse-topological pull run inside `Graph::process()`), using only audio-thread-owned data: the topo-sorted node array, `InputPool` input lists, and `link_head` processable-links.

**Pitfall (fixed):** earlier, `HostGraph` AGEvents (`addEdge`/`removeEdge`/`removeAllEdges`) walked `HostGraph::Node::{inputs,outputs,linkedNodes}` on the audio thread to mark processable state incrementally. Those vectors mutate on the JS thread under `nodesMutex_` — a cross-thread race. AGEvents must never read HostGraph adjacency for processable state; they only mirror structural edges/links onto the audio graph. The host-side `linkedNodes` list is now kept solely so links can be scrubbed when a linked node is disposed.

---

## Decision Table

| Scenario | Correct pattern |
|---|---|
| JS sets a property → audio thread reads it | Shadow state in HostObject + `scheduleAudioEvent` |
| Audio thread fires an event → JS callback | `EventCaller::dispatch()` / `dispatchEmpty()` |
| JS connects/disconnects nodes | `AudioNode::connect()` → `AudioGraphManager` |
| Property written by audio thread, JS reads it | `std::atomic<T>` on C++ node; getter reads directly |
| Non-primitive, can be written by audio thread | Triple buffer (see `AnalyserNode` for reference) |
| CPU-heavy work, must not block JS or audio | `TaskOffloader` on a dedicated worker thread |
| Context lifecycle (`resume`/`suspend`/`close`) | `scheduleContextPromise` → `pendingPromisesOffloader_` |

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

**Pitfall — file writer / recorder shutdown:** `TaskOffloader::shutdown()` drains the SPSC queue before joining the worker. Call it (or destroy the offloader) only after `isFileOpen_` is cleared so the audio thread stops enqueueing. Otherwise rotated or closed M4A segments lose seconds of buffered audio. Types with a `.slot` member use `slot == size_t max` as the shutdown sentinel.

---

## Driver synchronization (layered model)

Control-plane synchronization uses two layers — both are non-recursive `std::mutex`, never held on the audio thread.

| Layer | Location | Protects |
|---|---|---|
| Context | `BaseAudioContext::driverMutex_` (`AudioContext` + `OfflineAudioContext`) | `start` / `resume` / `suspend` / `close` (live); `resume` / `suspend` / `startRendering` (offline) — JS thread vs promise-pool |
| Engine | `AudioEngine` mutex (iOS only) | Process-wide `AVAudioEngine` graph: attach/detach, engine start/stop, interruptions, recorder paths |

`AudioContext::initialize()`, `createMediaElementSource()`, and `isDriverRunning()` are JS-thread-only — do not take `driverMutex_`. `getState()` returns the atomic control-thread state only (do not gate on `isDriverRunning()`); do not acquire `driverMutex_` from there.

On Android, `AudioPlayer::onErrorAfterClose` also takes `driverMutex_` because Oboe error callbacks bypass `AudioContext`.

**iOS refused restarts (`AudioEngine`):** iOS can reject an engine start that a route or configuration change triggered — typically `'!int'` / `560557684` (`AVAudioSessionErrorCodeCannotInterruptOthers`) while the device is locked or another app holds the session. Two invariants:

- **Never leave `state` at `Running` after a refused start.** `IOSAudioPlayer::isPlaying` and `IOSAudioRecorder` gate on `getState() == Running`, so a state that outlives a dead engine hides the failure from every consumer. `handleRefusedRestart` drops to `Paused`, sets `graphNeedsRebuild` (a refused start can leave the graph without an input node), and marks the restart pending.
- **Restart paths must not call each other.** `startEngine` rebuilds the graph inline rather than delegating to `rebuildAudioEngineAndResumeIfNeeded`, which would call `startEngine` back. That mutual recursion caused #1161/#1167 and was previously only muted by a flag.

Retries are scheduled with `dispatch_after` on the main queue and re-armed by `SystemNotificationManager` on foreground, route change, and interruption end. Because the retry block re-acquires the non-recursive engine mutex, it must be scheduled (never `dispatch_sync`) from under the lock, and each scheduling bumps a generation counter so an already-queued retry recognises itself as stale instead of racing a newer one.

**Live `AudioContext` render quiescence:** `currentRenders_` on `AudioContext` is incremented at the start of each platform I/O callback (`IOSAudioPlayer::deliverOutputBuffers` / `AudioPlayer::onAudioReady`) via a reference passed in `initialize()`, and decremented when the callback returns (RAII scope). `suspend()` and `close()` call `waitForRenderQuiescence()` (under `driverMutex_`) before `processAudioEvents()` / `cleanup()`. Platform drivers share the `CommonPlayer` abstract base (`common/cpp/audioapi/core/CommonPlayer.h`).

**Graph Channel A producer self-drain:** `Graph::setProducerSelfDrain(true)` makes the JS/main producer drain Channel A after each enqueue. Enable only when there is no audio/render consumer (realtime: construction + after `suspend`/`close` quiescence; offline: before `startRendering` and after a scheduled suspend). Before disabling for `start`/`resume`/`renderAudio`, call `processEvents()` once (still as sole consumer) so the bounded channel is empty, then disable, then start the audio/render consumer; re-enable if start/resume fails. After enabling, call `processEvents()` once to flush backlog (avoids `WAIT_ON_FULL` deadlock if the channel was already full).

**Context lifecycle promises:** HostObjects wrap JSI `Promise`s via `ContextPromiseResolver`
(`jsi/ContextPromiseResolver.hpp`); tasks are queued as `ContextPromiseTask`
(`core/types/ContextPromiseTask.h`). Lifecycle
ops (`resume` / `suspend` / `close` / offline start) are **control messages** on
`pendingPromisesOffloader_` via `scheduleContextPromise` from the JS thread
(`PromiseVendor::createPromise`, not the multi-worker `createAsyncPromise` — that would violate
SPSC single-producer). A dedicated `TaskOffloader` worker thread drains the queue under
`driverMutex_`, runs `collectDisposedNodes()` (host-graph ghost cleanup — never on the audio
thread), then executes the lifecycle body and settles the promise on the CallInvoker. The
`TaskOffloader` destructor joins the worker and drains any queued control messages. When the driver
is stopped, `scheduleAudioEvent` still drains already-queued SPSC events then runs
the new event synchronously under `driverMutex_` (FIFO with prior messages). Control-message
bodies must not re-lock `driverMutex_` or `waitForRenderQuiescence()` while `currentRenders_ > 0`
(audio-callback self-deadlock). Apply the visible `state` attribute and settle the promise
together in the `ContextPromise` resolve task (CallInvoker), after driver work — so `.state`
still reads the prior value until settlement (needed when `resume()` then `suspend()` are issued
back-to-back).

---

## Common Mistakes

- **Reading `node_->field_` in a getter** when that field is written by the audio thread → use shadow state or atomics.
- **Calling `node_->method()` directly from a setter** → always schedule via `scheduleAudioEvent`.
- **Not clearing callback IDs in the HostObject destructor** → node keeps firing into a GC'd JSI function; call `assignOnXCallbackId(0)` from each event HostObject layer on teardown
- **`std::vector::push_back` in `processNode()`** → may allocate; preallocate in constructor.
- **`std::mutex` anywhere in `processNode()`** → deadlock risk and real-time violation.
- **Copying `shared_ptr` inside `processNode()`** — increments atomic refcount; capture before entering hot path.
- **Locking `initialize()` or graph factory methods** — `initialize()` runs synchronously during HostObject construction on the JS thread; node factories and `createMediaElementSource()` are synchronous JS calls. Only lifecycle methods that touch the driver or offline render thread need `driverMutex_`.
- **Locking only `AudioContext`** — iOS recorder, session, and interruption paths mutate the shared `AVAudioEngine` outside `AudioContext`; keep the `AudioEngine` mutex on those entry points. Offline render uses the same `driverMutex_` on `BaseAudioContext`.
- **Re-entering `driverMutex_` or the `AudioEngine` mutex on the same thread** — call `tryStartDriver()` directly from `resume()` instead of `start()`; use lock-free `isStreamRunning()` from `isDriverRunning()`. `AudioContext::start()` does not acquire `driverMutex_`; it asserts the lock is already held when the driver is not initialized (via `scheduleAudioEvent` synchronous path). When already initialized, `start()` is a lock-free no-op so `source.start()` on the audio thread does not take the mutex.

---

*Maintenance: see [maintenance.md](maintenance.md).*
