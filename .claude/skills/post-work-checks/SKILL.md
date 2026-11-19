---
name: post-work-checks
description: >
  Ordered quality gate checklist to run after every code change in react-native-audio-api.
  Covers formatting, linting, type checking, C++ tests, JS tests, enum sync validation,
  the change-aware `pnpm check` / `pnpm test` (the same package set CI checks), native
  builds and XCTests CI only runs when native code changes. Documents what the lefthook
  pre-commit hook runs automatically vs what must be run manually.
  Use at the end of any implementation task before opening a PR.
  Trigger phrases: "post-work", "before PR", "before commit", "check quality", "run linter",
  "run tests", "format code", "lefthook", "pre-commit", "pnpm test", "pnpm check",
  "check --staged", "run-packages", "changes.sh", "which packages", "XCTests", "test:ios".
---

# Skill: Post-Work Checks

Run these checks after any code change and before opening a PR.

---

## Quick Reference

CI checks the packages a pull request affects: the ones its files are in plus their workspace
dependents (`scripts/changes.sh`; a change to a shared path such as the lockfile, a root config
or a CI script affects every package). For each it runs the package's `check` and `test`. The
library's `test` runs Jest and only the C++ suites the changes touch (`run-tests.sh affected`);
outside pull requests it runs all of them. Native builds and WPT run when their paths change.

`pnpm check` and `pnpm test` run the same set locally (`scripts/run-packages.sh`), against the
same base (`scripts/base-ref.sh`: `BASE_REF`, else the merge-base with `origin/main`), and then
list the other CI jobs your changes start, with the local command for each:

```bash
pnpm check                 # every affected package's `check` (format check, lint, typecheck…)
pnpm test                  # every affected package's `test`
pnpm check --all           # every package
pnpm check --staged        # staged changes only (what the pre-commit hook runs)
bash scripts/changes.sh    # just print the affected set (JSON)
pnpm format                # auto-fix formatting everywhere
```

### Native and slow checks (CI runs them only when the paths change)

| Changed paths                                       | Local command                                                                 |
| --------------------------------------------------- | ----------------------------------------------------------------------------- |
| `common/cpp/HostObjects/`, `android/`, Gradle/CMake | `pnpm android` (or `pnpm --filter react-native-audio-api build:android`)      |
| `common/cpp/HostObjects/`, `ios/audioapi/`, podspec | `pnpm ios` (or `build:ios`); XCTests: `pnpm --filter fabric-example test:ios` |
| `common/cpp/`, `src/`, `wpt_tests/`                 | WPT: `pnpm --filter react-native-audio-api wpt`                               |
| `common/cpp/audioapi/core/utils/graph/`             | `pnpm --filter react-native-audio-api test:cpp:extended -- graph`             |

`test:ios` picks the newest installed iPhone simulator; set `IOS_DESTINATION` to override, and
pass extra `xcodebuild` arguments (e.g. `-only-testing:FabricExampleTests/AudioEngineTests`).

---

## What lefthook Runs Automatically

Hooks run once lefthook is installed (`pnpm install` does it; or `pnpm exec lefthook install`).

### Pre-commit (every `git commit`)

1. **Formatters on the staged files**, fixes re-staged: oxfmt (JS/TS/JSON/Markdown/MDX),
   clang-format (C/C++/Objective-C), `ktlint -F` (Kotlin).
2. **`pnpm check --staged`**: the `check` of every package the staged files affect. clang-tidy
   then lints only the staged C++ files; it needs `pnpm setup:clangd` to have run once.

### Commit-msg

`commitlint` (conventional commits).

**If a hook fails, the commit is aborted.** Fix the issue and re-commit — do NOT use `--no-verify`.
Tests are not in the hook and there is no pre-push hook: run `pnpm test` before pushing.

---

## What Must Be Run Manually

```bash
pnpm check && pnpm test    # before opening a PR: what CI will run for this branch
```

Native builds, XCTests and WPT: see the table above. Autolinking registers workspace packages
under their real `packages/` path (`apps/common-app/scripts/dependencies.js` follows the
`node_modules` symlink), so Gradle and CocoaPods build local edits in
`packages/react-native-audio-api/` directly.

### C++ tests only

```bash
pnpm --filter react-native-audio-api test:cpp           # smoke
pnpm --filter react-native-audio-api test:cpp:full      # smoke + all extended
```

**When**: after any change to `common/cpp/audioapi/core/`, `dsp/`, or `utils/` C++ files; see
`common/cpp/test/TESTING.md`.

### Native enum sync check

```bash
pnpm --filter react-native-audio-api check:enums
```

Runs `packages/react-native-audio-api/scripts/check-enum-sync.sh`, which compares every
enum mirrored from C++: into Kotlin (`AudioEvent`, `RecorderState`, crossing JNI) and into
TypeScript (`FileFormat`, `FileDirectory`, `BitDepth`, `IOSAudioQuality` from
`AudioFileProperties.h`, crossing JSI). Both cross as plain ints, so entry _order_ and
values are the contract: an entry without an initializer counts as previous + 1, which is
also how a Kotlin ordinal counts. Names are compared ignoring case and underscores, since
the languages name entries differently by convention. An enum the script finds no entries
for fails the check. Add a new pair to the `MIRRORED_ENUMS` table at the top of the script.

**When**: only when you modify one of those enums or any file that maps event names across
C++/Kotlin/TypeScript. `pnpm check` includes it whenever the library is affected.

---

## Per-Language Commands

These scripts live on the `react-native-audio-api` workspace. From the monorepo root:

```bash
# TypeScript/JavaScript
pnpm --filter react-native-audio-api lint:js
pnpm --filter react-native-audio-api format:js

# Shared C++
pnpm --filter react-native-audio-api format:common
pnpm --filter react-native-audio-api lint:cpp   # clang-tidy; needs `pnpm setup:clangd` once

# Android
pnpm --filter react-native-audio-api format:android:cpp
pnpm --filter react-native-audio-api format:android:kotlin
pnpm --filter react-native-audio-api lint:kotlin

# iOS
pnpm --filter react-native-audio-api format:ios
pnpm --filter react-native-audio-api format:check:ios
```

---

## Recommended Order

1. `pnpm format` — fix formatting first (removes noise from lint)
2. `pnpm check` — format check, lint and typecheck of the affected packages
3. `pnpm test` — tests of the affected packages
4. Native builds / XCTests / WPT when `pnpm check` lists them

---

_Maintenance: see [maintenance.md](maintenance.md)._
