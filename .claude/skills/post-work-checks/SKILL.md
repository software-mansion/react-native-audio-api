---
name: post-work-checks
description: >
  Ordered quality gate checklist to run after every code change in react-native-audio-api.
  Covers formatting, linting, type checking, C++ tests, JS tests, and enum sync validation.
  Documents what lefthook pre-commit hooks run automatically vs what must be run manually.
  Use at the end of any implementation task before opening a PR.
  Trigger phrases: "post-work", "before PR", "before commit", "check quality", "run linter",
  "run tests", "format code", "lefthook", "pre-commit", "yarn test", "yarn lint".
---

# Skill: Post-Work Checks

Run these checks after any code change and before opening a PR.

---

## Quick Reference

```bash
yarn format                  # auto-fix all formatting
yarn lint                    # lint all workspaces
yarn typecheck               # TypeScript type checking
yarn test                    # C++ Google Tests
yarn check-audio-enum-sync   # only if AudioEvent enum touched
```

---

## What lefthook Runs Automatically

`lefthook` runs pre-commit hooks on every `git commit` — these run automatically:

| Hook | Command |
|---|---|
| Format | `yarn format` (JS via Prettier, C++ via clang-format, Kotlin via ktlint) |
| Lint | `yarn lint` (JS/TS + C++ + Kotlin) |
| Type check | `yarn typecheck` |
| Commit message | `commitlint` |

**If a hook fails, the commit is aborted.** Fix the issue and re-commit — do NOT use `--no-verify`.

---

## What Must Be Run Manually

These are NOT run by lefthook:

### C++ tests

```bash
yarn test   # from monorepo root
```

Runs `packages/react-native-audio-api/common/cpp/test/RunTests.sh` via CMake + Google Test.

**When**: after any change to `common/cpp/audioapi/core/`, `dsp/`, or `utils/` C++ files.

### AudioEvent enum sync check

```bash
yarn check-audio-enum-sync
```

**When**: only when you modify the `AudioEvent` enum or any file that maps event names across C++/Kotlin/TypeScript.

---

## Per-Language Commands

When you need to target a specific language:

```bash
# TypeScript/JavaScript
yarn lint:js           # ESLint
yarn format:js         # Prettier

# Shared C++
yarn format:common     # clang-format for common/cpp/
yarn lint:cpp          # cpplint

# Android
yarn format:android:cpp     # clang-format for android/src/main/cpp/
yarn format:android:kotlin  # KtLint
yarn lint:kotlin             # Kotlin linter

# iOS
yarn format:ios         # clang-format for ios/
yarn lint:ios           # iOS Objective-C++ format checks
```

---

## Recommended Order

Later steps may surface issues caused by earlier ones — run in this order:

1. `yarn format` — fix formatting first (removes noise from lint)
2. `yarn lint` — catch remaining code issues
3. `yarn typecheck` — catch TypeScript errors
4. `yarn test` — catch C++ regressions (when C++ changed)
5. `yarn check-audio-enum-sync` — when `AudioEvent` enum was touched

---

*Maintenance: see [maintenance.md](maintenance.md).*
