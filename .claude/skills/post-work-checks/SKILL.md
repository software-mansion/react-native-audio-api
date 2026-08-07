---
name: post-work-checks
description: >
  Ordered quality gate checklist to run after every code change in react-native-audio-api.
  Covers formatting, linting, type checking, C++ tests, JS tests, enum sync validation,
  and tiered local validation (validate.sh) for native builds CI does not run.
  Documents what lefthook pre-commit hooks run automatically vs what must be run manually.
  Use at the end of any implementation task before opening a PR.
  Trigger phrases: "post-work", "before PR", "before commit", "check quality", "run linter",
  "run tests", "format code", "lefthook", "pre-commit", "yarn test", "yarn lint",
  "validate", "validate:fast", "validate:full".
---

# Skill: Post-Work Checks

Run these checks after any code change and before opening a PR.

---

## Quick Reference — Local Validation Tiers

CI always runs the **fast** tier (format, lint, typecheck, enum sync, TS build, C++ + JS tests). Graph tests run in a separate path-filtered workflow (`graph-tests.yml`). CI does **not** build Android Gradle, iOS pods, or example apps. Use `validate.sh` locally to close that gap:

```bash
yarn validate:fast      # CI parity — always run before opening a PR
yarn validate:graph     # graph tests (when graph/audio-thread code changes)
yarn validate:android   # Android native build (requires ANDROID_HOME)
yarn validate:ios       # iOS native build (macOS only)
yarn validate:full      # --fast + --android + --ios (skips unavailable platforms)
```

Equivalent: `./scripts/validate.sh --fast` (etc.)

### iOS unit tests (not covered by any tier)

`validate:ios` only builds the pod. The Objective-C++ XCTest suite in `apps/fabric-example/ios/FabricExampleTests/` (engine, session manager, notification manager, player, recorder) is run by neither CI nor `validate.sh`, so changes under `ios/audioapi/` must be exercised by hand:

```bash
cd apps/fabric-example/ios
xcodebuild test -workspace FabricExample.xcworkspace -scheme FabricExampleTests \
  -destination 'platform=iOS Simulator,name=<an installed simulator>' \
  -only-testing:FabricExampleTests/AudioEngineTests   # omit to run everything
```

Check `xcrun simctl list devices available` first — an unavailable `-destination` makes xcodebuild print the device list and fail in a way that is easy to mistake for a passing run when its output is piped. Run `pod install` if the build reports the sandbox is out of sync; it can also rewrite prebuilt-pod checksums in the tracked `Podfile.lock`, which should not be committed with unrelated work.

Because these tests are never run automatically, they rot. Several test files hand-declare mirror copies of C++ classes (`IOSAudioPlayer` in `AudioPlayerTests.mm`, `IOSAudioRecorder` in `IOSAudioRecorderTests.mm`) to reach protected members; adding a pure virtual to a base class makes those mirrors abstract and breaks compilation of the whole target. Add the matching override to the mirror when changing `CommonPlayer` or `AudioRecorder`.

### Which tier to run

| Changed paths | Minimum validation |
|---|---|
| `src/`, types only | `--fast` |
| `common/cpp/core/`, `dsp/`, `utils/` | `--fast` |
| `common/cpp/HostObjects/` | `--fast` + `--android` + `--ios` |
| `android/src/main/cpp/` or `java/` | `--fast` + `--android` |
| `ios/audioapi/` | `--fast` + `--ios` |
| `CMakeLists.txt`, `build.gradle`, `podspec` | `--full` |

Graph changes under `common/cpp/audioapi/core/utils/graph/` → also run `yarn validate:graph`.

---

## Quick Reference — Individual Commands

```bash
yarn format                  # auto-fix all formatting
yarn lint                    # lint all workspaces
yarn typecheck               # TypeScript type checking
yarn test                    # C++ + JS tests (library workspace)
yarn check-audio-enum-sync   # only if AudioEvent enum touched
```

---

## What lefthook Runs Automatically

Hooks run when lefthook is installed (`lefthook install`).

### Pre-commit (every `git commit`)

| Hook | Command |
|---|---|
| Format check | `yarn format:check` (JS via Prettier, C++ via clang-format, Kotlin via ktlint) |
| Lint | `yarn lint` (JS/TS + C++ + Kotlin) |
| Type check | `yarn typecheck` (only when staged files match `*.{js,ts,jsx,tsx}`) |
| Commit message | `commitlint` |

**If a hook fails, the commit is aborted.** Fix the issue and re-commit — do NOT use `--no-verify`.

There is no pre-push hook — `yarn validate:fast` (and native/graph tiers) are run manually before opening a PR. Native builds (`validate:android`, `validate:ios`, `validate:full`) and graph tests (`validate:graph`) are never run by lefthook.

---

## What Must Be Run Manually

These are not covered by lefthook:

### CI-parity gate (before opening a PR)

```bash
yarn validate:fast
```

### Native compile checks (when platform code changes)

```bash
yarn validate:android   # yarn workspace … build:android (~3–4 min)
yarn validate:ios       # yarn workspace … build:ios (macOS only)
yarn validate:full      # --fast + --android + --ios
```

The Gradle project resolves through the `node_modules/react-native-audio-api` workspace symlink, so local edits in `packages/react-native-audio-api/` are picked up.

### Graph tests (when graph / audio-thread code changes)

```bash
yarn validate:graph
```

### C++ tests only

```bash
yarn workspace react-native-audio-api run test:cpp
```

**When**: after any change to `common/cpp/audioapi/core/`, `dsp/`, or `utils/` C++ files. Prefer this for a fast C++-only loop without running Jest; run `yarn validate:fast` before opening a PR.

### Library unit tests (JS + C++)

```bash
yarn test   # from monorepo root — runs test:js + test:cpp
```

**When**: after any change to C++ files or TypeScript files in `src/`. Prefer this for a quick local test loop covering both TS and C++ logic; run `yarn validate:fast` before opening a PR.

### AudioEvent enum sync check

```bash
yarn check-audio-enum-sync
```

**When**: only when you modify the `AudioEvent` enum or any file that maps event names across C++/Kotlin/TypeScript. Skip this step if you already ran `validate:fast` (it includes enum sync).

---

## Per-Language Commands

These scripts live on the `react-native-audio-api` workspace. From the monorepo root:

```bash
# TypeScript/JavaScript
yarn workspace react-native-audio-api lint:js
yarn workspace react-native-audio-api format:js

# Shared C++
yarn workspace react-native-audio-api format:common
yarn workspace react-native-audio-api lint:cpp

# Android
yarn workspace react-native-audio-api format:android:cpp
yarn workspace react-native-audio-api format:android:kotlin
yarn workspace react-native-audio-api lint:kotlin

# iOS
yarn workspace react-native-audio-api format:ios
yarn workspace react-native-audio-api lint:ios
```

---

## Recommended Order

Later steps may surface issues caused by earlier ones — run in this order:

1. `yarn format` — fix formatting first (removes noise from lint)
2. `yarn lint` — catch remaining code issues
3. `yarn typecheck` — catch TypeScript errors
4. `yarn validate:fast` — full CI-parity gate (or `yarn test` / `test:cpp` for a quick local loop; always run `--fast` before opening a PR)
5. `yarn validate:graph` — when graph / audio-thread code changed
6. `yarn validate:android` / `yarn validate:ios` / `yarn validate:full` — when native code or build files changed (see decision table above)

---

*Maintenance: see [maintenance.md](maintenance.md).*
