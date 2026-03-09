# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Repository Overview

`react-native-audio-api` is a high-performance Web Audio API-compatible audio engine for React Native, maintained by Software Mansion. It provides audio playback, recording, DSP processing, and real-time analysis across iOS, Android, and Web.

## Architecture

### Monorepo Structure
```
packages/react-native-audio-api/   # Main library
apps/common-app/                   # Example RN app
apps/fabric-example/               # New Architecture example app
packages/audiodocs/                # Documentation
packages/custom-node-generator/    # Code generation tooling
```

### Layers (from JS to hardware)

1. **TypeScript API** (`src/`) — node implementations (`src/core/`), browser passthrough (`src/web-core/`), platform system APIs (`src/system/`), TurboModule specs (`src/specs/`), hooks, events, utils
2. **C++ Engine** (`common/cpp/audioapi/`) — node engine (`core/`), SIMD DSP (`dsp/`), JSI HostObjects, audio events, prebuilt external libraries (`external/`)
3. **Android Native** (`android/`) — CMake + Gradle, Kotlin module, C++ JNI glue (`src/main/cpp/`), Oboe 1.9.3 audio I/O
4. **iOS Native** (`ios/audioapi/ios/`) — Objective-C++ (`.mm`), CocoaPods (`RNAudioAPI.podspec`), CoreAudio I/O

### Key Architectural Patterns
- **JSI**: Audio nodes are exposed as C++ JSI HostObjects — no bridge serialization
- **Audio Thread Safety**: Real-time audio processing happens on a dedicated audio thread; JS-side calls must not block it
- **Dual Platform**: TypeScript code has separate paths for React Native (native engine) and Web (delegates to browser Web Audio API)
- **New Architecture Ready**: Supports both old Bridge and new TurboModules/Fabric
- **Optional FFmpeg**: Audio decoding via FFmpeg can be conditionally compiled out
- **Audio Worklets**: JavaScript runs on the audio thread via React Native Worklets

### Native Module Entry Points
- iOS: `ios/audioapi/ios/AudioAPIModule.mm`
- Android: `android/src/main/java/com/swmansion/audioapi/AudioAPIModule.kt`
- TurboModule spec: `src/specs/NativeAudioAPIModule.ts`

## Web Resources

The following URLs can be fetched without approval and are useful during development:

| URL | When to use |
|---|---|
| https://webaudio.github.io/web-audio-api/ | Web Audio API W3C spec — parameter defaults, processing semantics, error conditions |
| https://developer.mozilla.org/en-US/docs/Web/API/Web_Audio_API | MDN — friendlier spec reference, examples |
| https://en.cppreference.com/ | C++ standard library reference — containers, algorithms, atomics, memory model |
| https://reactnative.dev/docs/the-new-architecture/pure-cxx-modules | RN New Architecture — TurboModules, JSI, fbjni patterns |

Fetch these proactively when implementing a new node (to check spec compliance), when using an unfamiliar C++ stdlib class, or when working on the TurboModule / JSI installation layer.

## Autonomous Parallelization

Claude MUST decide independently whether to parallelize work and run subagents — do not wait for the user to ask. Default to parallel execution whenever tasks are independent.

**Web research**: When a task requires fetching or summarizing content from multiple URLs, or searching for information across multiple topics, launch parallel `general-purpose` or `Explore` subagents — one per source/topic. Each agent fetches, filters, and returns only the relevant excerpt. Never fetch URLs sequentially when they are independent.

**Codebase exploration**: When investigating a bug or implementing a feature that spans multiple layers (TypeScript + HostObject + C++ node + iOS + Android), launch parallel `Explore` agents to read each layer simultaneously rather than reading files one by one.

**Skill updates**: When `pre-push-update` identifies multiple skill files to review, use parallel background agents to update them simultaneously.

The user expects Claude to make these parallelization decisions without being prompted. Spawning a subagent costs less than waiting for sequential work.

## Golden Reference Implementations

When implementing anything new, mirror structure and style from these proven files. Ask the agent to explain any intentional deviation.

| Task | Reference file(s) |
|---|---|
| New C++ effect/analysis node | `common/cpp/audioapi/core/effects/GainNode.h` + `.cpp` |
| New JSI HostObject | `common/cpp/audioapi/HostObjects/effects/GainNodeHostObject.h` + `.cpp` |
| New scheduled source node | `common/cpp/audioapi/core/sources/OscillatorNode.h` + `.cpp` |
| New TypeScript API class | `packages/react-native-audio-api/src/core/GainNode.ts` |

---

## Skills

Detailed skill files live in `.claude/skills/`. Each skill lives in its own directory as `<name>/SKILL.md` and is auto-loaded by Claude Code based on YAML frontmatter trigger phrases. Consult the relevant skill before starting work in that area.

| Skill directory | Domain |
|---|---|
| `host-objects/` | C++ JSI HostObject layer — creating and maintaining HostObjects |
| `audio-nodes/` | C++ audio node engine — implementing and connecting audio nodes |
| `turbo-modules/` | TurboModule/JSI wiring — spec → native → HostObject installation |
| `web-audio-api/` | Web Audio API spec conformance and browser passthrough layer |
| `build-compilation-dependencies/` | CMake, Gradle, podspec, prebuilt libraries |
| `thread-safety-itc/` | Audio thread safety, lock-free patterns, event system |
| `post-work-checks/` | Ordered checklist to run after every change |
| `flow/` | End-to-end feature implementation flow (tests + docs required) |
| `utilities/` | Shared DSP and C++/TS utility helpers |
| `writing-skills/` | How to write, structure, and maintain skill files |

See `.claude/README.md` for a full description of the Claude Code setup and the `/pre-push-update` command.

## Self-Modification Instructions

After completing any task, Claude MUST review whether any of the following apply and make the appropriate edits if so:

1. **New pattern discovered**: If a fix or investigation revealed a non-obvious pattern, invariant, or pitfall that is not yet documented in the relevant skill file, add a concise note to that skill file.

2. **Inconsistency found**: If a path, folder, or file mentioned in this CLAUDE.md or any skill file does not actually exist, correct the reference. If an important path exists in the repo but is not mentioned anywhere, add it to the relevant skill file or to this file.

3. **Outdated information**: If something described here contradicts reality (wrong command, moved file, renamed module), update it immediately — do not leave a known-wrong description in place.

4. **Skill gaps**: If work required knowledge that is not covered by any skill file, note it in the closest matching skill file or propose a new one.

5. **Skill quality issues**: When reading or modifying a skill file and noticing any of the following, fix them immediately — do not leave known-bad skill files in place:
   - Missing `Trigger phrases: "..."` label in the `description` (skill will not auto-load without it)
   - Cross-references using `.md` suffix (e.g. `audio-nodes.md`) instead of bare skill name (e.g. `audio-nodes`)
   - Stale cross-references to renamed or moved files
   - A skill body that exceeds 500 lines without moving verbose content to a supporting file

The goal is that these files stay accurate and grow more useful over time through incremental updates from real work, not just manual maintenance.
