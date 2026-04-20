# Maintenance — writing-skills

> Used by `/pre-push-update` only — not loaded when the `writing-skills` skill is active.

Review this skill when `pre-push-update` reports changes in:

| Path | What to check |
|---|---|
| `.claude/skills/*/SKILL.md` | New patterns observed — update best-practices or anti-patterns sections |
| `.claude/skills/**/*.md` | Supporting file or maintenance.md conventions changed — update the relevant section |
| `.claude/commands/pre-push-update.md` | Maintenance contract changed — update the `maintenance.md` rules |
| `CLAUDE.md` | Skill table updated — update the checklist step that references `CLAUDE.md` |
