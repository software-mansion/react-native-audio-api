# Maintenance — host-objects

> Used by `/pre-push-update` only — not loaded when the `host-objects` skill is active.

Review this skill when `pre-push-update` reports changes in:

| Path | What to check |
|---|---|
| `common/cpp/audioapi/HostObjects/**` | New HostObjects, macro usage changes, shadow state patterns |
| `common/cpp/audioapi/jsi/JsiHostObject.*` | Macro signatures, new export macros — update both `SKILL.md` and `examples.md` |
| `common/cpp/audioapi/HostObjects/BaseAudioContextHostObject.*` | Factory wiring section (3-step checklist) |
| Any new `*HostObject.h` | Add to directory structure, document any new patterns it introduces |
| `common/cpp/audioapi/HostObjects/effects/GainNodeHostObject.*` | `examples.md` — macro usage, AudioParam wrapping pattern |
| `common/cpp/audioapi/HostObjects/sources/OscillatorNodeHostObject.*` | `examples.md` — shadow state pattern, enum parsing |

Update the **three-layer architecture diagram** in `SKILL.md` only if the communication mechanism between layers changes.
