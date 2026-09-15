# Maintenance — flow

> Used by `/pre-push-update` only — not loaded when the `flow` skill is active.

Review this skill when `pre-push-update` reports changes in:

| Path | What to check |
|---|---|
| `common/cpp/test/CMakeLists.txt` | Test build section — new exclusions, new defines |
| `packages/react-native-audio-api/tests/` | JS test section — new test patterns |
| `packages/audiodocs/` | Docs section — new public documentation conventions |
| `packages/internaldocs/` | Docs section — internal docs authoring (page tags, topic folders) |
| `package.json` scripts | Post-work checks step — new or renamed commands |

This skill describes **process** — update it when the team's workflow changes, not for every new node added.
