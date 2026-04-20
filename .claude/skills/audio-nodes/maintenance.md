# Maintenance — audio-nodes

> Used by `/pre-push-update` only — not loaded when the `audio-nodes` skill is active.

Review this skill when `pre-push-update` reports changes in:

| Path | What to check |
|---|---|
| `common/cpp/audioapi/core/AudioNode.*` | Base class contract, `processNode()` signature, `processAudio()` graph traversal |
| `common/cpp/audioapi/core/AudioParam.*` | A-rate / k-rate section, automation method list |
| `common/cpp/audioapi/core/sources/AudioScheduledSourceNode.*` | Playback state machine, `updatePlaybackInfo` contract |
| `common/cpp/audioapi/core/<new file>` | Add new node to class hierarchy diagram and directory tree |
| `common/cpp/audioapi/core/effects/GainNode.h` | `gainnode-example.md` — constructor signature, AudioParam declarations, thread annotations |
| `common/cpp/audioapi/core/effects/GainNode.cpp` | `gainnode-example.md` — `processNode()` body, AudioParam initialization |

Update the **class hierarchy diagram** in `SKILL.md` when a node is added, removed, or changes its base class.
