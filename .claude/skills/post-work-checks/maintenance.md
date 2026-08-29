# Maintenance — post-work-checks

> Used by `/pre-push-update` only — not loaded when the `post-work-checks` skill is active.

Review this skill when `pre-push-update` reports changes in:

| Path | What to check |
|---|---|
| Root `package.json` scripts | New or renamed lint/format/test/`validate:*` commands |
| `packages/react-native-audio-api/package.json` scripts | Package-level command changes (including per-language lint/format) |
| `lefthook.yml` | Pre-commit / commit-msg hook changes |
| `scripts/validate.sh` | Tier behavior (`--fast` / `--graph` / `--android` / `--ios` / `--full`), skip rules |
| `scripts/check-audio-enum-sync*` or `packages/react-native-audio-api/scripts/check-audio-events-sync.sh` | Enum sync check details |
| `.github/workflows/ci.yml`, `tests.yml`, `cpp-extended-job.yml` | What CI covers vs local validation tiers |
