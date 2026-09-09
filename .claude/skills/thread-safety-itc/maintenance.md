# Maintenance — thread-safety-itc

> Used by `/pre-push-update` only — not loaded when the `thread-safety-itc` skill is active.

Review this skill when `pre-push-update` reports changes in:

| Path | What to check |
|---|---|
| `common/cpp/audioapi/events/**` | New event types, handler patterns, `AudioEvent` enum |
| `common/cpp/audioapi/utils/SpscChannel.hpp` | Lock-free queue API changes |
| `common/cpp/audioapi/utils/CrossThreadEventScheduler.hpp` | Scheduler API changes — update decision table |
| `common/cpp/audioapi/core/AudioNode.*` | Audio thread contract changes |
| `common/cpp/audioapi/core/utils/AudioGraphManager.*` | Graph mutation queue changes |
| `common/cpp/audioapi/core/utils/EncodedAudioFileWriter.*`, `RotatingFileWriter.*` | The "reacting to audio without blocking on it" pattern: `switchToFile`/`setOnBufferEncodedCallback` contract, the listener-outside-the-lock rule, the never-join-while-holding rule, the `final`/`virtual` test seam |
| Any new cross-thread primitive in `utils/` | Document in the decision table |
