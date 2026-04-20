---
name: writing-skills
description: >
  How to write, structure, and maintain Claude skill files. Covers the three-level progressive
  disclosure model, all YAML frontmatter fields (context:fork, allowed-tools,
  disable-model-invocation, user-invocable, hooks, argument-hint), invocation control patterns,
  string substitutions ($ARGUMENTS, ${CLAUDE_SKILL_DIR}), shell preprocessing with backtick
  syntax, and the Maintenance section contract. Use when creating a new skill file, rewriting an
  existing one, or reviewing a skill for quality.
  Trigger phrases: "add a skill", "write a skill", "create a skill file", "update skill",
  "skill quality", "skill review", "context fork", "allowed-tools", "user-invocable".
user-invocable: false
---

# Skill: Writing Skills

A skill file is a scoped knowledge document that Claude loads on demand. Good skills are concise, trigger reliably, and stay accurate over time.

---

## Three-Level Model

```
Level 1 — YAML frontmatter       always loaded, ~100 tokens
Level 2 — skill body (SKILL.md)  loaded on trigger, < 500 lines / ~5 k tokens
Level 3 — supporting .md files   loaded explicitly when more detail is needed
```

Claude sees the `description` field in every conversation. It reads the skill body only when the topic is relevant. It reads a supporting file only when the skill body links to it and more detail is needed.

**Design each level independently.** The frontmatter alone must be enough to trigger the skill. The skill body must be useful without any supporting file. Supporting files are supplementary.

---

## File Structure

Each skill lives in its own directory:

```
.claude/skills/<skill-name>/
├── SKILL.md              # Required — main skill body with YAML frontmatter
└── <supporting-file>.md  # Optional — verbose reference material linked from SKILL.md
```

Supporting files (e.g. `gainnode-example.md`, `api.md`, `build-details.md`) live alongside `SKILL.md` in the same directory. They are plain `.md` files (not `SKILL.md`). Alternatively group them under an optional `references/` subdirectory when you have several related documents, or `scripts/` for executable code Claude can run.

Link from the parent skill:

```markdown
See [gainnode-example.md](gainnode-example.md) for a complete header + .cpp.
```

---

## YAML Frontmatter

Required: `name` and `description`. Everything else is optional.

```yaml
---
name: kebab-case-name           # matches the directory name; no claude/anthropic prefix
description: >
  What the skill covers. When to use it.
  Trigger phrases: "phrase one", "phrase two".

# — optional fields —
context: fork                   # run as isolated subagent (see below)
agent: Explore                  # subagent type: Explore / Plan / general-purpose / custom
allowed-tools: "Read Grep Glob" # tools available without per-use approval
model: claude-sonnet-4-6        # model override for this skill
disable-model-invocation: true  # user can /invoke; Claude won't auto-load
user-invocable: false           # Claude auto-loads; hidden from the /menu
argument-hint: "[node-name]"    # shown in slash-command autocomplete
hooks:                          # hooks scoped to this skill's lifecycle
metadata:
  author: team
  version: 1.0.0
---
```

### Rules

- `name`: kebab-case, matches directory name. Forbidden prefixes: `claude`, `anthropic`.
- `description`: max 1024 characters. Include **what**, **when**, and **trigger phrases** (quoted, comma-separated at the end).
- Trigger phrases must match how developers actually speak: `"add a node"`, `"processNode"`, `"shadow state"` — not `"audio processing implementation"`.
- No XML angle brackets (`<` or `>`) anywhere in frontmatter — security restriction.

### Anti-patterns

```yaml
# BAD — vague, no trigger phrases
description: This skill covers audio node implementation details.

# BAD — too long (gets cut at 1024 chars)
description: >
  This skill covers every aspect of ... [500 words] ...

# BAD — missing the literal "Trigger phrases:" label (skill will never auto-load)
description: >
  How to create a C++ audio node. Covers processNode() contract, AudioParam a-rate/k-rate,
  cross-thread scheduling. Use when implementing a new node or debugging audio rendering.
  "add a node", "processNode", "audio thread", "AudioParam automation".

# GOOD
description: >
  How to create a C++ audio node. Covers processNode() contract, AudioParam a-rate/k-rate,
  cross-thread scheduling. Use when implementing a new node or debugging audio rendering.
  Trigger phrases: "add a node", "processNode", "audio thread", "AudioParam automation".
```

**The `Trigger phrases:` label is mandatory** — without the exact label, Claude Code does not recognise the list as trigger phrases and the skill will not auto-load.

---

## Invocation Control

By default a skill is both user-invokable (`/skill-name`) and auto-loaded by Claude. Override with:

| Frontmatter | User `/invoke` | Claude auto-load | Use when |
|---|---|---|---|
| (default) | Yes | Yes | General knowledge skills |
| `disable-model-invocation: true` | Yes | No | Task-only / deployment skills |
| `user-invocable: false` | No | Yes | Internal / meta knowledge |

---

## context: fork

Runs the skill in an isolated subagent. The skill body becomes the task prompt — the subagent has no access to conversation history.

```yaml
---
name: deep-research
description: Research a topic thoroughly
context: fork
agent: Explore   # Explore / Plan / general-purpose / custom .claude/agents/ name
---

Research $ARGUMENTS thoroughly:
1. Find relevant files using Glob and Grep
2. Read and analyze the code
3. Summarize findings with specific file references
```

**Only useful for task-oriented skills** with concrete step-by-step instructions. Do NOT set on reference/knowledge skills — the subagent receives guidelines but has no actionable task.

---

## String Substitutions

Available in skill body content:

```
$ARGUMENTS             all arguments passed when user invokes /skill-name arg
$ARGUMENTS[0]          first argument by index (0-based); $0 is shorthand
${CLAUDE_SESSION_ID}   current session ID
${CLAUDE_SKILL_DIR}    absolute path to the skill directory — use to reference bundled scripts
```

## Shell Preprocessing

`` !`command` `` runs a shell command **before** the skill content is sent to Claude. Output replaces the placeholder at load time — Claude sees only the result, not the syntax.

```markdown
## Context
Branch: !`git branch --show-current`
Last commits: !`git log --oneline -5`
```

Useful for skills that need live repository state injected automatically.

---

## Skill Body

### Structure

```markdown
---
name: my-skill
description: > ...
---

# Skill: My Skill

One-sentence summary of what this covers (not a repeat of frontmatter — add context).

---

## Most Important Section First

Critical invariants, hard constraints, common mistakes go at the top.
Readers may stop reading early — put the highest-value content first.

## Secondary Sections

...

---

## Maintenance

Review this skill when `pre-push-update` reports changes in:

| Path | What to check |
|---|---|
| `path/to/file.*` | What to review in this skill |
```

### Rules

- **Under 500 lines** — non-negotiable. If the file exceeds 500 lines, move verbose content to a supporting file in the same directory.
- **Imperative form**: "Use `AudioParam`", "Declare in protected:", "Call `scheduleAudioEvent`". Not: "You should use", "It is recommended to call".
- **Code over prose**: a 5-line snippet teaches faster than two paragraphs. Prefer concrete examples.
- **Critical first**: MUST NOT lists, common pitfalls, and hard constraints go in the FIRST section, not buried at the bottom. Readers stop reading early.
- **No scope blockquotes**: do not add `> **Scope**: ...` / `> **What this skill covers**: ...` / `> **When Claude should consult this skill**: ...` — this duplicates the frontmatter and wastes lines.
- **Escape hatches**: add "If unsure → [do X]" guidance wherever a wrong choice causes hard-to-debug bugs (e.g. "If unsure which ITC primitive → check the decision table in `thread-safety-itc`").
- **Golden references**: link to one or two existing files that exemplify the patterns in the skill. These let Claude anchor new code to proven implementations.

### What belongs in the skill body vs supporting files

| Belongs in skill body | Move to supporting file |
|---|---|
| API overview (1-line per method) | Complete header + .cpp of a class |
| Key usage patterns (5–15 line snippets) | Full working example (50+ lines) |
| Decision tables, checklists | Exhaustive per-flag build analysis |
| Common pitfalls (concise) | Line-by-line CMakeLists commentary |
| Links to spec or docs | Full `.hpp` API with every overload |

---

## The Maintenance File

Each skill directory has a `maintenance.md` file mapping source paths to what needs checking. It is **not loaded during normal skill usage** — only `/pre-push-update` reads it.

```markdown
# Maintenance — skill-name

> Used by /pre-push-update only — not loaded during skill usage.

| Path | What to check |
|---|---|
| `path/to/file.*` | Specific section or element to review |
```

`SKILL.md` ends with a single footer line linking to it:

```markdown
*Maintenance: see [maintenance.md](maintenance.md).*
```

Supporting files do **not** have their own maintenance sections — their rows go into the same `maintenance.md`.

### Why separate

`## Maintenance` embedded in `SKILL.md` wastes tokens on every skill load. A dedicated `maintenance.md` is only read by `/pre-push-update`.

### Rules

- Use glob-style patterns (`*`, `**`) for directories.
- The "What to check" column must name the specific section or element to review — not just "update if needed".
- Add a row whenever you add a new documented pattern to the skill.
- Supporting file paths belong in the skill's `maintenance.md` — not in the supporting file itself.

---

## Supporting Files

For content that is too large for the skill body but still useful to load on demand.

### When to create a supporting file

- A complete class header + implementation (50+ lines of code)
- Full API documentation for a large `.hpp` template file
- Deep line-by-line analysis of a build file
- A complete worked example spanning multiple files

### Location

Place supporting files in the same directory as `SKILL.md`:

```
.claude/skills/audio-nodes/
├── SKILL.md
└── gainnode-example.md    # supporting file
```

Link from the parent skill:

```markdown
See [gainnode-example.md](gainnode-example.md) for a complete header + .cpp.
```

Supporting files do **not** need YAML frontmatter or a `name` field — they are plain markdown.

---

## Checklist: New Skill File

1. Create directory `.claude/skills/<skill-name>/`
2. Create `SKILL.md` with YAML frontmatter (`name`, `description` with what + when + trigger phrases, ≤1024 chars)
3. `# Skill: Name` heading — no scope blockquotes below it
4. Most important content first
5. Under 500 lines — move verbose content to a supporting `.md` file in the same directory
6. Imperative form, code snippets preferred over prose
7. Create `maintenance.md` with path → what-to-check table; add `*Maintenance: see [maintenance.md](maintenance.md).*` at the bottom of `SKILL.md`
8. If supporting files created: their maintenance rows go into `maintenance.md` (not in the supporting files)
9. Add the skill directory to the table in `CLAUDE.md` and the tree in `.claude/README.md`

---

*Maintenance: see [maintenance.md](maintenance.md).*
