---
name: post-work-checks
description: >
  Ordered quality gate checklist to run after every code change in react-native-audio-api.
  Covers formatting, linting, type checking, C++ tests, JS tests, enum sync validation,
  and tiered local validation (validate.sh) for native builds CI does not run.
  Documents what lefthook pre-commit hooks run automatically vs what must be run manually.
  Use at the end of any implementation task before opening a PR.
  Trigger phrases: "post-work", "before PR", "before commit", "check quality", "run linter",
  "run tests", "format code", "lefthook", "pre-commit", "pnpm test", "pnpm lint",
  "validate", "validate:fast", "validate:full".
---

# Skill: Post-Work Checks

Run these checks after any code change and before opening a PR.

---

## Quick Reference — Local Validation Tiers

CI always runs the **fast** tier (format, lint, typecheck, enum sync, TS build, C++ smoke + JS tests). Extended C++ categories (e.g. graph) run from `tests.yml` when matching paths change, or via `workflow_dispatch` booleans. CI does **not** build Android Gradle, iOS pods, or example apps. Use `validate.sh` locally to close that gap:

```bash
pnpm validate:fast          # CI parity — always run before opening a PR
pnpm validate:cpp           # C++ smoke
pnpm validate:cpp-extended  # C++ extended (all categories)
pnpm validate:android       # Android native build (requires ANDROID_HOME)
pnpm validate:ios           # iOS native build (macOS only)
pnpm validate:full          # --fast + C++ extended + --android + --ios
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

Graph changes under `common/cpp/audioapi/core/utils/graph/` → also run `pnpm validate:cpp-extended` or `pnpm --filter react-native-audio-api test:cpp:extended -- graph`.

---

## Quick Reference — Individual Commands

```bash
pnpm format                  # auto-fix all formatting
pnpm lint                    # lint all workspaces
pnpm typecheck               # TypeScript type checking
pnpm test                    # C++ + JS tests (library workspace)
pnpm check-audio-enum-sync   # only if a C++/Kotlin mirrored enum touched
```

---

## What lefthook Runs Automatically

Hooks run when lefthook is installed (`lefthook install`).

### Pre-commit (every `git commit`)

| Hook | Command |
|---|---|
| Format check | `pnpm format:check` (JS via Prettier, C++ via clang-format, Kotlin via ktlint) |
| Lint | `pnpm lint` (JS/TS + C++ + Kotlin) |
| Type check | `pnpm typecheck` (only when staged files match `*.{js,ts,jsx,tsx}`) |
| Commit message | `commitlint` |

**If a hook fails, the commit is aborted.** Fix the issue and re-commit — do NOT use `--no-verify`.

There is no pre-push hook — `pnpm validate:fast` (and native/C++ extended tiers) are run manually before opening a PR. Native builds (`validate:android`, `validate:ios`, `validate:full`) and extended C++ (`validate:cpp-extended`) are never run by lefthook.

---

## What Must Be Run Manually

These are not covered by lefthook:

### CI-parity gate (before opening a PR)

```bash
pnpm validate:fast
```

### Native compile checks (when platform code changes)

```bash
pnpm validate:android   # pnpm --filter … build:android (~3–4 min)
pnpm validate:ios       # pnpm --filter … build:ios (macOS only)
pnpm validate:full      # --fast + C++ extended + --android + --ios
```

Autolinking registers workspace packages under their real `packages/` path (`apps/common-app/scripts/dependencies.js` follows the `node_modules` symlink), so Gradle and CocoaPods build local edits in `packages/react-native-audio-api/` directly.

### Extended graph (when graph / audio-thread code changes)

```bash
pnpm validate:cpp-extended       # all extended categories
# or: pnpm --filter react-native-audio-api test:cpp:extended -- graph
```

### C++ tests only

```bash
pnpm validate:cpp                                            # smoke via validate.sh
pnpm --filter react-native-audio-api test:cpp           # smoke
pnpm --filter react-native-audio-api test:cpp:full      # smoke + all extended
```

**When**: after any change to `common/cpp/audioapi/core/`, `dsp/`, or `utils/` C++ files. Prefer smoke for a fast C++-only loop; see `common/cpp/test/TESTING.md`. Run `pnpm validate:fast` before opening a PR.

### Library unit tests (JS + C++)

```bash
pnpm test   # from monorepo root — runs test:js + test:cpp
```

**When**: after any change to C++ files or TypeScript files in `src/`. Prefer this for a quick local test loop covering both TS and C++ logic; run `pnpm validate:fast` before opening a PR.

### Native enum sync check

```bash
pnpm check-audio-enum-sync
```

Runs `packages/react-native-audio-api/scripts/check-enum-sync.sh`, which compares every
enum mirrored between C++ and Kotlin — currently `AudioEvent` and `RecorderState`. These
cross JNI as plain ints and Kotlin maps them back by ordinal, so entry *order* is part of
the contract; the check compares ignoring case and underscores, since the two languages
name entries differently by convention. Add a new pair to the `MIRRORED_ENUMS` table at
the top of the script. It then runs `check-audio-file-properties-enum-sync.sh`, which does
the same for the `AudioFileProperties` enums that cross JSI into TypeScript (`FileFormat`,
`FileDirectory`, `BitDepth`, `IOSAudioQuality`).

**When**: only when you modify one of those enums or any file that maps event names across
C++/Kotlin/TypeScript. Skip this step if you already ran `validate:fast` (it includes enum sync).

---

## Per-Language Commands

These scripts live on the `react-native-audio-api` workspace. From the monorepo root:

```bash
# TypeScript/JavaScript
pnpm --filter react-native-audio-api lint:js
pnpm --filter react-native-audio-api format:js

# Shared C++
pnpm --filter react-native-audio-api format:common
pnpm --filter react-native-audio-api lint:cpp

# Android
pnpm --filter react-native-audio-api format:android:cpp
pnpm --filter react-native-audio-api format:android:kotlin
pnpm --filter react-native-audio-api lint:kotlin

# iOS
pnpm --filter react-native-audio-api format:ios
pnpm --filter react-native-audio-api lint:ios
```

---

## Recommended Order

Later steps may surface issues caused by earlier ones — run in this order:

1. `pnpm format` — fix formatting first (removes noise from lint)
2. `pnpm lint` — catch remaining code issues
3. `pnpm typecheck` — catch TypeScript errors
4. `pnpm validate:fast` — full CI-parity gate (or `pnpm test` / `test:cpp` for a quick local loop; always run `--fast` before opening a PR)
5. `pnpm validate:cpp-extended` — when graph / audio-thread code changed
6. `pnpm validate:android` / `pnpm validate:ios` / `pnpm validate:full` — when native code or build files changed (see decision table above)

---

*Maintenance: see [maintenance.md](maintenance.md).*
