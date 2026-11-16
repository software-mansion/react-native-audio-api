# Maintenance — post-work-checks

> Used by `/pre-push-update` only — not loaded when the `post-work-checks` skill is active.

Review this skill when `pre-push-update` reports changes in:

| Path | What to check |
|---|---|
| Root `package.json` scripts | New or renamed lint/format/test/`validate:*` commands |
| `packages/react-native-audio-api/package.json` scripts | Package-level command changes (including per-language lint/format) |
| `lefthook.yml` | Pre-commit / commit-msg hook changes |
| `scripts/validate.sh` | Tier behavior (`--fast` / `--cpp-extended` / `--android` / `--ios` / `--full`), skip rules |
| `packages/react-native-audio-api/scripts/check-enum-sync.sh` | Enum sync check details (Kotlin + TypeScript mirrors) |
| `.github/workflows/ci.yml`, `tests.yml`, `cpp-job.yml` | What CI covers vs local validation tiers |
