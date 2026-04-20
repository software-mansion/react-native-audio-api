# Maintenance — utilities

> Used by `/pre-push-update` only — not loaded when the `utilities` skill is active.

Review this skill when `pre-push-update` reports changes in:

| Path | What to check |
|---|---|
| `common/cpp/audioapi/utils/*.h` | New utility added — add brief usage note to `SKILL.md` |
| `common/cpp/audioapi/utils/SpscChannel.hpp` | `api.md` — API, OverflowStrategy/WaitStrategy values, ResponseStatus enum |
| `common/cpp/audioapi/utils/CrossThreadEventScheduler.hpp` | `api.md` — template parameter meaning, `scheduleEvent` return value |
| `common/cpp/audioapi/utils/AlignedAllocator.hpp` | `api.md` — default alignment, constructor signature |
| `common/cpp/audioapi/utils/MoveOnlyFunction.hpp` | `api.md` — signature, throw behaviour |
| `common/cpp/audioapi/utils/Result.hpp` | `api.md` — Ok/Err constructors, method signatures |
| `common/cpp/audioapi/utils/RingBiDirectionalBuffer.hpp` | `api.md` — push/pop/peek method names, capacity rules |
| `common/cpp/audioapi/utils/TaskOffloader.hpp` | `api.md` — constructor, `getSender`, destructor behaviour |
| `common/cpp/audioapi/utils/Benchmark.hpp` | `api.md` — function names, return type |
| `common/cpp/audioapi/core/utils/AudioDestructor.hpp` | `api.md` — `tryAddForDeconstruction` signature, capacity |
| `common/cpp/audioapi/core/utils/ParamChangeEvent.hpp` | `api.md` — constructor args, getters/setters |
| `common/cpp/audioapi/dsp/AudioUtils.hpp` | `api.md` — new DSP helpers added or signatures changed |
| `common/cpp/audioapi/core/utils/Constants.h` | Constants section in `SKILL.md` |
| `src/utils/**` | TypeScript utils section in `SKILL.md` |
