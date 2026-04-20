# Claude Code Setup

This directory contains project-specific configuration for [Claude Code](https://claude.ai/code).

## Structure

```
.claude/
├── settings.json              # Tool permissions and MCP server configuration
├── last-knowledge-update      # Tracks last SHA processed by /pre-push-update
├── commands/
│   └── pre-push-update.md     # /pre-push-update slash command
├── skills/
│   ├── audio-nodes/
│   │   ├── SKILL.md           # C++ audio node engine
│   │   ├── gainnode-example.md
│   │   └── maintenance.md     # For /pre-push-update only
│   ├── host-objects/
│   │   ├── SKILL.md           # JSI HostObject layer
│   │   ├── examples.md
│   │   └── maintenance.md
│   ├── build-compilation-dependencies/
│   │   ├── SKILL.md           # CMake, Gradle, podspec, prebuilt libs
│   │   ├── build-details.md
│   │   └── maintenance.md
│   ├── utilities/
│   │   ├── SKILL.md           # Shared C++ and TS utilities
│   │   ├── api.md
│   │   └── maintenance.md
│   ├── native-ios/
│   │   ├── SKILL.md           # iOS native layer
│   │   └── maintenance.md
│   ├── native-android/
│   │   ├── SKILL.md           # Android native layer
│   │   └── maintenance.md
│   ├── turbo-modules/
│   │   ├── SKILL.md           # TurboModule/JSI wiring
│   │   └── maintenance.md
│   ├── web-audio-api/
│   │   ├── SKILL.md           # Web Audio API spec conformance
│   │   └── maintenance.md
│   ├── thread-safety-itc/
│   │   ├── SKILL.md           # Audio thread safety & ITC
│   │   └── maintenance.md
│   ├── post-work-checks/
│   │   ├── SKILL.md           # Checklist after every change
│   │   └── maintenance.md
│   ├── flow/
│   │   ├── SKILL.md           # End-to-end feature flow
│   │   └── maintenance.md
│   └── writing-skills/
│       ├── SKILL.md           # How to write and maintain skill files (meta)
│       └── maintenance.md
└── README.md                  # This file
```

## Skills

Skill files are a reference library for Claude. Each skill lives in its own directory (`.claude/skills/<name>/SKILL.md`) and is **auto-loaded** by Claude Code based on the YAML frontmatter `name` and `description` fields. The description contains trigger phrases — when the conversation context matches, the skill is surfaced automatically.

Skills use a **three-level progressive disclosure model**:
1. **Frontmatter** — always loaded; name + description with trigger phrases
2. **`SKILL.md` body** — loaded when the skill is triggered; concise patterns and APIs
3. **Supporting `.md` files** (e.g. `gainnode-example.md`, `build-details.md`) — linked from `SKILL.md`; loaded explicitly when deep reference is needed

Skills are intentionally kept concise (under 500 lines). They answer "what exists and how do I use it", not "how is it implemented". Verbose material (full code examples, deep build analysis, complete API docs for large `.hpp` files) lives alongside `SKILL.md` in the same skill directory.

**To update a skill manually**: edit the relevant `SKILL.md` file directly.

**To keep skills in sync with code automatically**: use `/pre-push-update` (see below).

## Maintenance Files

Every skill directory has a `maintenance.md` file. It maps source file path patterns to what needs checking in that skill when those paths change. This file is **not loaded during normal skill usage** — only `/pre-push-update` reads it.

**Purpose**: when `/pre-push-update` runs, it reads each relevant skill's `maintenance.md` to decide exactly which sections to review. Without this table, Claude has to guess — with it, the mapping is explicit and reliable.

**Format** (same in every `maintenance.md`):

```markdown
# Maintenance — skill-name

> Used by /pre-push-update only — not loaded during skill usage.

| Path | What to check |
|---|---|
| `path/to/file.*` | What in this skill to review or update |
```

Each `SKILL.md` ends with a single footer line: `*Maintenance: see [maintenance.md](maintenance.md).*`

Supporting files (e.g. `gainnode-example.md`) do **not** have their own maintenance sections — their rows are merged into the skill's `maintenance.md`.

**Rule**: if you add a new pattern, invariant, or code example to a skill, also add or update the relevant row in `maintenance.md` so future runs of `/pre-push-update` know to revisit it.

## `/pre-push-update` command

A slash command that reviews all commits since its last run and updates skill files to reflect what changed.

### How it works

1. `scripts/collect-knowledge-changes.sh` reads `.claude/last-knowledge-update` for the last-processed git SHA. On first run (empty file), it falls back to `HEAD~10`.
2. The script outputs:
   - All commits in the range
   - **All changed files** (full stat, unfiltered) — Claude uses this to triage what is interesting
   - **Source diff** filtered to `*.h / *.hpp / *.cpp / *.mm / *.kt / *.ts / *.tsx` inside the tracked source directories
   - **Maintenance tables** — all `maintenance.md` files concatenated, so Claude knows exactly which sections to review without extra file reads
3. Claude reads the output, classifies each changed path against the skill map, and makes targeted additions or corrections.
4. Claude advances `.claude/last-knowledge-update` to the new HEAD SHA.

### When to run it

Run it before pushing a branch, after merging a PR, or whenever you feel the skill files may have drifted from the code. It is safe to run at any time — it only reads git history and writes to `.claude/`.

```bash
# Inside a Claude Code session:
/pre-push-update
```

### What it updates

| Change type | Action |
|---|---|
| New audio node class | Add entry to `audio-nodes/SKILL.md` |
| New HostObject pattern | Add entry to `host-objects/SKILL.md` |
| New utility helper | Add entry to `utilities/SKILL.md` |
| Renamed/moved class referenced in a skill | Correct the reference |
| New thread-safety invariant | Add to `thread-safety-itc/SKILL.md` |
| Pure formatting / test-only / CI changes | Skipped |

### What it does NOT do

- Rewrite skills from scratch
- Document internal implementation details
- Process binary files, lock files, or generated code
- Touch anything outside `.claude/`

### Marker file

`.claude/last-knowledge-update` contains the SHA of the last commit that was successfully processed. If it is empty or the SHA is not found in history, the script falls back to `HEAD~10`. You can reset it manually by writing any valid commit SHA.

```bash
# Reset to a specific commit (process everything since that point next run)
git rev-parse <commit-ish> > .claude/last-knowledge-update

# Reset to process the last 20 commits next run
git rev-parse HEAD~20 > .claude/last-knowledge-update
```

### Diff limits

The script caps the source diff at **4000 lines**. If a batch of commits exceeds this, the diff is truncated with a warning. In that case run `/pre-push-update` more frequently, or review large refactors manually.

## `settings.json`

Defines tool permissions for Claude Code:

- **Always allow**: `yarn build/lint/format/test`, read-only git commands, reading all source files, writing/editing `.claude/**` and `**/CLAUDE.md`, common inspection commands (`ls`, `which`, etc.)
- **Ask before**: destructive git operations (`commit`, `push`, `reset`, `checkout`, etc.), `yarn clean`
- **Always deny**: force push, `rm -rf`, `sudo`, reading build artifacts and binary files

Also configures MCP servers:
- `filesystem` — `@modelcontextprotocol/server-filesystem` pointed at the repo root
- `lsp` — `mcp-language-server` using `typescript-language-server` for TS/JS code intelligence
