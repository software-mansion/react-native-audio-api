# AudioRecorder (not an AudioNode)

> Supporting file for the `audio-nodes` skill.

`core/inputs/AudioRecorder` is a standalone `final` class, not part of the `AudioNode` hierarchy — it
feeds the graph through a `RecorderAdapterNode` instead of being processed by it.

It owns the session state machine: an explicit `Phase` (`Idle`, `Starting`, `Recording`, `Paused`,
`Rerouting`; the table of what holds in each is in `AudioRecorder.h`) that only `enterPhase()` changes; it arms the
gate exactly for `Recording`, and `repointOutputsAt()`, which disarms the gate while the outputs
follow a new format, re-arms it only when the phase is `Recording`. `start`/`stop`/`pause`/`resume`/`rerouteInput` are a `switch`
over the phase, serialized by `controlMutex_`; add a phase rather than a flag when the session gains
a state. `start()` claims the session in the registry and enters `Starting` before it acquires a route, so a start that cannot claim never touches routing (on Android that can bring the shared Bluetooth link up and down); both waits for a route (`Starting`, `Rerouting`) record what to become in `phaseAfterOpening` and, once the route is in, re-check what may have changed while the control mutex was released. `start()` gives up unless the phase is still `Starting` and `session_.id` is still the session it claimed (a stop, or a stop and a new start, came in between). `reopenInput()` gives up unless the phase is still `Rerouting` and `inputGeneration` is still the `closedGeneration` it bumped to (a stop, a new session or a later reroute moved it); it needs no session id, because ending a session also bumps the generation. On entry `reopenInput()` also ignores an event whose `openedGeneration` is not the current one, so a late notification of a closed opening reroutes nothing. A reroute during `Starting` is not dropped: the route the start waits for may predate the device change (Android's `AudioInputRouter` decides the device at acquire time), so `reopenInput()` bumps `inputGeneration` and `start()`, finding the generation moved once its route is in, releases that route and acquires again, opening only on a route that no reroute overtook (the same takeover rule `reopenInput()` applies to an earlier reroute). Prefer that to a flag replayed after the start: the input never opens on the old device. A reroute that cannot reopen the input ends the session with `stop(failedGeneration)`, the generation `reopenInput()` reported for its failed opening: `stop()` with a generation refuses once `inputGeneration` has moved, so a stale failure can never end the newer opening a later reroute or a new session made in the meantime. The session
registry keeps what it recorded, as for every stop. Session state lives in one `SessionState` (phase, input
generation, stream format; guarded by `controlMutex_`); put new session state there. Everything that
happens to recorded frames lives in `RecorderConsumers` (`core/inputs/RecorderConsumers.h`, the
recorder's `consumers_`): a list of `AudioConsumer`s (`core/AudioConsumer.h`:
`prepare` / `consume(const AudioBuffer &, numFrames) -> Result` / `reprepare` / `close`) and this
session's `AudioFileWriter`, itself an `AudioConsumer` but kept apart and typed in `file_` (a new
writer per session, closed by `finalize` through `closeFile()`, which returns its files). All of
them are fed by one effects worker (an `AudioBufferOffloader` sized per format; `worker_.stop()`
is the drain); the writer queues into its own `AudioBufferOffloader`, so encoding and rotation
never stall the other consumers. `prepare` at start, `follow` on a format change (drain, then
`reprepare` each prepared consumer: by default `close` + `prepare`; the writer overrides it to keep
its file), `detach` at the end (drain, close every listed consumer, move the writer out unclosed),
`finalize` closes the file with no mutex held. Disabling the file mid-session drops the buffers
still queued for the effects worker. `push()` also drops a buffer, before any consumer sees it, when
no pool slot is free (the worker fell `POOL_SIZE` buffers behind).
A new consumer is an `AudioConsumer` subclass, never a new slot: `AudioRecorderCallback` (JS
`onAudioReady`, created by the installer's `createAudioReadyConsumer` and added by JS `onAudioReady`) and the adapter's `RecorderAdapterConsumer`
(added by JS `AudioRecorder.connect` through `addConsumer`, from the node HostObject's `consumer`
getter) are listed with `AudioRecorder::addConsumer` / `removeConsumer`; the recorder knows no
concrete consumer but the file. The adapter is split in two: `RecorderAdapterConsumer` owns the
ring buffers and the resampler (`prepare` sizes them, `consume` writes, `close` waits for a render
quantum in flight and frees them, `render` reads on the audio thread), and `RecorderAdapterNode` is
a plain `AudioNode` whose `processNode` calls `consumer_->render()`. They share only the consumer,
so the recorder never extends a graph node's lifetime; once the node is gone, its consumer's frames
reach no one. `RecorderConsumers` never calls a consumer concurrently nor on the audio thread, so a
consumer needs no locks of its own for those calls: the worker's `deliver()` holds `consumersMutex_`
while it feeds the list, the recorder's control calls are serialized by `controlMutex_`, and
`remove()` unlists the consumer under `consumersMutex_` before it calls `close()` without it, so the
worker can no longer reach it. A consumer that another thread also reads synchronizes that side itself:
`RecorderAdapterConsumer::render()` runs on the graph's audio thread, against rings that lock
themselves and an `isInitialized_` / `currentProcesses_` guard on their lifetime (see
`thread-safety-itc`). Errors go through the recorder's one `EventCaller<RECORDER_ERROR>`, handed to `RecorderConsumers`
and from there to the file writer; `setOnError` is a single `assignCallbackId`. A listed consumer
never holds the event: a failed `consume()` returns `Err` and `RecorderConsumers::deliver` reports it
(the consumer stays prepared for the next frames). Only the file writer reports on its own, because its
encode and rotation errors happen on its own worker after its `consume()` returned. `RecorderConsumers`
does not synchronize its control calls: `AudioRecorder` makes them only under `controlMutex_` (the
audio thread's `write()` from `onInputFrames()` holds no mutex), and disarms the gate before `prepare()`, `follow()` and `detach()`, the only calls that start or stop
the worker (the audio thread reads `worker_` without a lock). When anything fails after
`start()` opened the input it ends the session like `stop()`: detach under the control mutex,
finalize without it, so no file stays open. Listed consumers stay listed across sessions (JS
`connect()` creates its adapter once). Test consumers through `RecorderConsumers`'s public
API (`RecorderConsumersTest.cpp`), not through a friend of the recorder.

The platform's part is only the capture stream behind `IRecorderInput` (`AndroidRecorderInput`,
`IOSRecorderInput`; created by `createPlatformRecorderInput()`, defined next to each). The input
reports its `StreamFormat` through `getCurrentFormat()`, which the recorder reads only after
`start()` (starting can reconfigure the input: the iOS engine rebuilds first when the hardware
changed). On iOS, attaching the input node needs the engine stopped and started again right
after, so `IOSRecorderInput::open()` stops, attaches and starts, rolling all of it back when it
fails, and `start()` only makes sure the engine runs (`startIfNecessary:`). Keep it that way when
`AudioEngine` takes over the stop-around-attach rule: `open()` then only attaches, and `start()`
is what starts an engine playback was not already running. The input hands frames to the
`RecorderInputGate` it was given (the recorder arms it only while recording and disarms it before
changing anything the audio thread reads), and reports what it did not cause through
`RecorderInputEvents` (`formatChanged` for an iOS route change, `lost` for an Android disconnect,
`failed`); events carry the generation of the opening they come from and are dropped once that
opening has ended. Add shared recorder behavior to `AudioRecorder`, not to one input. State a
consumer already owns stays with the consumer: the adapter's ring layout
(`RecorderAdapterConsumer::prepare`) and the session's file paths (returned by
`AudioFileWriter::closeFile()`) are not mirrored in the recorder.
