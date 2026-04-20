# Maintenance — post-work-checks

> Used by `/pre-push-update` only — not loaded when the `post-work-checks` skill is active.

Review this skill when `pre-push-update` reports changes in:

| Path | What to check |
|---|---|
| Root `package.json` scripts | New or renamed lint/format/test commands |
| `packages/react-native-audio-api/package.json` scripts | Package-level command changes |
| `.lefthook.yml` or lefthook config | Pre-commit hook changes |
| `scripts/check-audio-enum-sync*` | Enum sync check details |
