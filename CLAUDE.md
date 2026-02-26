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

## Development Notes

- **Node version**: 18 (see `.nvmrc`)
- **Package manager**: Yarn 4.5.0 (workspaces)
- **Pre-commit hooks**: `lefthook` runs format, lint, typecheck, and commitlint automatically
- **C++ formatting**: `.clang-format` at repo root defines the style; always run `yarn format:common` after editing shared C++
- **Minimum targets**: iOS 14.0+, Android API 21+, React Native 0.76+
