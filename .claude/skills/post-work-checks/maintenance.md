# Maintenance — post-work-checks

> Used by `/pre-push-update` only — not loaded when the `post-work-checks` skill is active.

Review this skill when `pre-push-update` reports changes in:

| Path                                                                                 | What to check                                                                                  |
| ------------------------------------------------------------------------------------ | ---------------------------------------------------------------------------------------------- |
| Root `package.json` scripts                                                          | New or renamed `check` / `test` / format commands                                              |
| `packages/react-native-audio-api/package.json` scripts                               | Package-level command changes (including per-language lint/format)                             |
| `lefthook.yml`                                                                       | Pre-commit / commit-msg hook changes                                                           |
| `scripts/changes.sh`, `scripts/run-packages.sh`, `scripts/base-ref.sh`               | Shared paths, native/WPT path lists, the base revision, `--staged` behaviour, the CI-job hints |
| `lefthook.yml`                                                                       | Pre-commit steps and their order                                                               |
| `packages/react-native-audio-api/scripts/check-enum-sync.sh`                         | Enum sync check details (Kotlin + TypeScript mirrors)                                          |
| `.github/workflows/ci.yml`, `check-package.yml`, `native-build.yml`, `wpt-tests.yml` | What CI covers vs local validation tiers                                                       |
