# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Repository Overview

`react-native-audio-api` is a high-performance Web Audio API-compatible audio engine for React Native, maintained by Software Mansion. It provides audio playback, recording, DSP processing, and real-time analysis across iOS, Android, and Web.

## Commands

### Root (Monorepo)
```bash
yarn build          # Build all workspaces
yarn lint           # Lint all workspaces
yarn format         # Format all code
yarn typecheck      # TypeScript type checking
yarn test           # Run C++ tests via RunTests.sh
yarn clean          # Remove build artifacts and node_modules
yarn check-audio-enum-sync  # Validate AudioEvent enum synchronization
```

### Main Package (`packages/react-native-audio-api`)
```bash
yarn lint:js        # ESLint for TypeScript/JavaScript
yarn lint:cpp       # C++ linting (cpplint)
yarn lint:ios       # iOS Objective-C++ format checks
yarn lint:kotlin    # Kotlin linting (Android)

yarn format:js      # Prettier
yarn format:common  # clang-format for shared C++
yarn format:android:cpp     # clang-format for Android C++
yarn format:android:kotlin  # KtLint
yarn format:ios             # clang-format for iOS
```

### Tests
- **C++ tests**: `yarn test` from root runs `packages/react-native-audio-api/common/cpp/test/RunTests.sh` using CMake + Google Test
- **JS tests**: Jest with preset `react-native`, test files in `packages/react-native-audio-api/tests/` matching `**/*.test.ts`

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

1. **TypeScript API** (`packages/react-native-audio-api/src/`)
   - `src/core/` — Web Audio API node implementations (GainNode, BiquadFilterNode, etc.)
   - `src/web-core/` / `src/web-system/` — Browser Web Audio API passthrough
   - `src/system/` — Platform-specific session/permission/device management
   - `src/specs/` — TurboModule specs (native method signatures)
   - `src/hooks/`, `src/events/`, `src/utils/`, `src/mock/`

2. **C++ Engine** (`packages/react-native-audio-api/common/cpp/audioapi/`)
   - `core/` — Audio node engine (sources, destinations, effects, analysis, inputs)
   - `dsp/` — DSP algorithms with SIMD optimization (ARM NEON, x86 SSE2)
   - `HostObjects/` — JSI bridge objects (C++ ↔ JavaScript)
   - `jsi/` — JavaScript Interface bindings
   - `events/` — Audio thread event system
   - `libs/` — Third-party library wrappers
   - `external/` — Prebuilt binaries: FFmpeg, Opus, Ogg, Vorbis, OpenSSL

3. **Android Native** (`android/`)
   - CMake + Gradle build
   - Kotlin modules in `src/main/java/com/swmansion/audioapi/`
   - C++ glue in `src/main/cpp/audioapi/`
   - Uses Oboe 1.9.3 for high-performance audio I/O

4. **iOS Native** (`ios/audioapi/ios/`)
   - Objective-C++ (`.mm` files)
   - CocoaPods via `RNAudioAPI.podspec`
   - Prebuilt FFmpeg frameworks downloaded at pod install

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

## Development Notes

- **Node version**: 18 (see `.nvmrc`)
- **Package manager**: Yarn 4.5.0 (workspaces)
- **Pre-commit hooks**: `lefthook` runs format, lint, typecheck, and commitlint automatically
- **C++ formatting**: `.clang-format` at repo root defines the style; always run `yarn format:common` after editing shared C++
- **Minimum targets**: iOS 14.0+, Android API 21+, React Native 0.76+

## Skills

Detailed skill files live in `.claude/skills/`. Each skill lives in its own directory as `<name>/SKILL.md` and is auto-loaded by Claude Code based on YAML frontmatter trigger phrases. Consult the relevant skill before starting work in that area.

| Skill directory | Domain |
|---|---|
| `host-objects/` | C++ JSI HostObject layer — creating and maintaining HostObjects |
| `audio-nodes/` | C++ audio node engine — implementing and connecting audio nodes |
| `native-ios/` | iOS native layer — Objective-C++, CocoaPods, AVFoundation |
| `native-android/` | Android native layer — Kotlin, CMake, Oboe, JNI |
| `turbo-modules/` | TurboModule/JSI wiring — spec → native → HostObject installation |
| `web-audio-api/` | Web Audio API spec conformance and browser passthrough layer |
| `build-compilation-dependencies/` | CMake, Gradle, podspec, prebuilt libraries |
| `thread-safety-itc/` | Audio thread safety, lock-free patterns, event system |
| `post-work-checks/` | Ordered checklist to run after every change |
| `flow/` | End-to-end feature implementation flow (tests + docs required) |
| `utilities/` | Shared DSP and C++/TS utility helpers |
| `writing-skills/` | How to write, structure, and maintain skill files |

Additional context CLAUDE.md files exist in subdirectories:
- `apps/CLAUDE.md` — working with example apps
- `packages/audiodocs/CLAUDE.md` — working with the documentation site
- `packages/react-native-audio-api/tests/CLAUDE.md` — JS/TS test suite

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
