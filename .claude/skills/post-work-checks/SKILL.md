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

CI always runs the **fast** tier (format, lint, typecheck, enum sync, TS build, C++ smoke + JS tests). Extended C++ categories (e.g. graph) run from `tests.yml` when matching paths change, or via `workflow_dispatch` booleans. CI does **not** build Android Gradle, iOS pods, or example apps. Use `validate.sh` locally to close that gap:

```bash
yarn validate:fast          # CI parity — always run before opening a PR
yarn validate:cpp           # C++ smoke
yarn validate:cpp-extended  # C++ extended (all categories)
yarn validate:graph         # legacy alias: extended category graph only
yarn validate:android       # Android native build (requires ANDROID_HOME)
yarn validate:ios           # iOS native build (macOS only)
yarn validate:full          # --fast + C++ extended + --android + --ios
```

Equivalent: `./scripts/validate.sh --fast` (etc.)

### Which tier to run

| Changed paths | Minimum validation |
|---|---|
| `src/`, types only | `--fast` |
| `common/cpp/core/`, `dsp/`, `utils/` | `--fast` |
| `common/cpp/HostObjects/` | `--fast` + `--android` + `--ios` |
| `android/src/main/cpp/` or `java/` | `--fast` + `--android` |
| `ios/audioapi/` | `--fast` + `--ios` |
| `CMakeLists.txt`, `build.gradle`, `podspec` | `--full` |

Graph changes under `common/cpp/audioapi/core/utils/graph/` → also run `yarn validate:graph` or `yarn validate:cpp-extended` (extended category `graph`).

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

There is no pre-push hook — `yarn validate:fast` (and native/C++ extended tiers) are run manually before opening a PR. Native builds (`validate:android`, `validate:ios`, `validate:full`) and extended C++ (`validate:cpp-extended` / legacy `validate:graph`) are never run by lefthook.

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
yarn validate:full      # --fast + C++ extended + --android + --ios
```

The Gradle project resolves through the `node_modules/react-native-audio-api` workspace symlink, so local edits in `packages/react-native-audio-api/` are picked up.

### Extended graph (when graph / audio-thread code changes)

```bash
yarn validate:graph              # legacy alias: extended category graph
yarn validate:cpp-extended       # all extended categories
# or: yarn workspace react-native-audio-api test:cpp:extended -- graph
```

### C++ tests only

```bash
yarn validate:cpp                                            # smoke via validate.sh
yarn workspace react-native-audio-api run test:cpp           # smoke
yarn workspace react-native-audio-api run test:cpp:full      # smoke + all extended
```

**When**: after any change to `common/cpp/audioapi/core/`, `dsp/`, or `utils/` C++ files. Prefer smoke for a fast C++-only loop; see `common/cpp/test/TESTING.md`. Run `yarn validate:fast` before opening a PR.

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
5. `yarn validate:cpp-extended` / `yarn validate:graph` — when graph / audio-thread code changed
6. `yarn validate:android` / `yarn validate:ios` / `yarn validate:full` — when native code or build files changed (see decision table above)

---

*Maintenance: see [maintenance.md](maintenance.md).*
