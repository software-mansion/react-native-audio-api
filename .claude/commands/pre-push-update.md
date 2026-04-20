Run the knowledge update process: collect all commits since the last update, analyse what changed, and update skill files and CLAUDE.md accordingly.

## Step 1 — Collect changes

Run the collection script:

```bash
bash scripts/collect-knowledge-changes.sh
```

If the output starts with `NO_NEW_COMMITS`, stop here and tell the user there is nothing to update.

## Step 2 — Triage the changed files

Look at the **ALL CHANGED FILES** section first. Use it to decide which changes are worth deep analysis. Classify each changed path:

| Path pattern | Potentially affects |
|---|---|
| `common/cpp/audioapi/core/` | `audio-nodes/SKILL.md` (+ `audio-nodes/gainnode-example.md`) |
| `common/cpp/audioapi/HostObjects/` | `host-objects/SKILL.md` (+ `host-objects/examples.md`) |
| `common/cpp/audioapi/utils/` or `dsp/` | `utilities/SKILL.md` (+ `utilities/api.md`) |
| `common/cpp/audioapi/events/` or `core/utils/Audio*` | `thread-safety-itc/SKILL.md` |
| `android/src/main/` | `native-android/SKILL.md` |
| `ios/` | `native-ios/SKILL.md` |
| `src/specs/` or `AudioAPIModule.*` | `turbo-modules/SKILL.md` |
| `src/` (TypeScript) | `turbo-modules/SKILL.md` or `web-audio-api/SKILL.md` |
| `CMakeLists.txt`, `*.podspec`, `*.gradle` | `build-compilation-dependencies/SKILL.md` (+ `build-compilation-dependencies/build-details.md`) |
| `.claude/` | CLAUDE.md itself |

For each identified skill, check the **MAINTENANCE TABLES** section at the end of the script output — it contains every skill's `maintenance.md` so you can see exactly which sections to review without additional file reads.

**Skip a file entirely if:**
- It is a test file with no new patterns (e.g. adding a test for existing behaviour)
- The change is purely formatting/whitespace
- It is a dependency version bump
- It is CI config, example app, or lock file

## Step 3 — Read the source diff

Read the **SOURCE DIFF** section for the files you decided are interesting. For each interesting change ask:

1. **New API or class?** — Is there a new class, method, or utility that a developer working in this area would want to know about? If yes, add a concise entry to the relevant skill file.

2. **New pattern or invariant?** — Did the change reveal a non-obvious pattern, rule, or constraint (e.g. "this must always be called before X", "this field is audio-thread only")? If yes, document it in the relevant skill file.

3. **Broken reference?** — Does the diff rename, move, or delete something that is currently mentioned in a skill file or CLAUDE.md? If yes, correct the reference.

4. **New utility?** — Was a utility helper added to `utils/` or `dsp/`? If yes, add it to `utilities.md` following the existing format (brief usage note for `.h` files, inline docs for `.hpp`).

5. **Nothing documentable?** — If the change is purely internal implementation with no effect on the documented API, patterns, or invariants — skip it.

## Step 4 — Update skill files

Apply only targeted, minimal additions or corrections. Do **not**:
- Rewrite skill files from scratch
- Add documentation for every changed line
- Document internal implementation details that are only useful when reading that specific file

Read each skill file you intend to modify before editing it.

## Step 5 — Advance the marker

Extract the `HEAD_SHA=<sha>` line from the script output. Write only that SHA (no newline, no extra text) to `.claude/last-knowledge-update`.

```bash
# replace <sha> with the actual SHA from the last line of script output
printf '%s' '<sha>' > .claude/last-knowledge-update
```

To reset the marker to "empty" (so the next run falls back to HEAD~10), use:
```bash
> .claude/last-knowledge-update
```
(`touch` only updates the timestamp — it does **not** clear the file contents.)

## Step 6 — Report

Tell the user:
- Which skill files were updated and why (one line each)
- Which files were skipped and why (brief)
- The new marker SHA
