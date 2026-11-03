---
name: semi-automated-tests
description: >
  Create, edit, and run semi-automated E2E tests in apps/fabric-example/tests/ that combine
  Argent YAML fragment workflows, interactive human prompts, and chronological native/JS log
  and journal collection. Use when writing or running an E2E test that requires human interaction
  (interruptions, Siri, phone calls, headphones plug/unplug, locking screen, speaking into mic,
  or verifying audio output) or correlated device log capture.
  Trigger phrases: "semi-automated test", "fabric-example/tests", "write an e2e test with logs",
  "human prompt test", "test:e2e", "interruption test", "argent test with prompt",
  "journal-execution".
---

# Skill: Semi-Automated Tests

Author and run semi-automated E2E tests in `apps/fabric-example/tests/` that interleave automated Argent UI fragments with interactive human prompts and capture a single chronologically merged native + JS + journal `.log` file.

---

## Critical Rules

1. **Never build the app inside test scripts.** Tests attach to the already-running `FabricExample` instance.
2. **Always author Argent `.yaml` files as fragments (`executionPrerequisite: ...`, NO `launch:` step).**
   - A `launch:` step calls `restart-app`, which terminates the app, wipes out the `AUDIO_LOG_LEVEL` environment variable read once at startup by `Log.h`, and destroys mid-test UI state between sub-workflows.
   - `launch:` also hardcodes a bundle ID, which differs across developers' physical iPhone signing setups. `test-harness.sh` resolves `CFBundleIdentifier` dynamically via `ios_bundle_id` and calls `npx argent run launch-app` once at test start to register and foreground the running app without killing it.
3. **Split workflows at every human interaction boundary.**
   - Argent YAML workflows cannot prompt a human with arguments or read terminal `stdin`.
   - Split every test into numbered Argent fragment files (`01-before.yaml`, `02-after.yaml`, ...) orchestrated by `run.sh` with `prompt_action`, `prompt_choice`, or `prompt_text` calls in between.
4. **Use `id: log-clear` for `AudioIOPlayground` screen identity.**
   - `AudioIOPlayground` opens in **Detailed mode**. Tests switch to Compact mode with `tap: { id: playground-compact }` (`id: playground-detailed` switches back). `../shared/open-audio-io-playground.yaml` does that after opening the screen.
   - In Compact mode, all buttons, statuses, and inline `<testID>-option-<value>` chips fit inside the viewport above `LogView` (`id: log-clear`, `id: log`) with no scrolling and no modal dropdowns.
5. **Tap `<testID>-option-<value>` chips directly in Compact mode.**
   - Presets, categories, modes, options, playback sources, and device inputs render as inline chips in Compact mode (`id: 'playback-source-option-Last take'`, `id: 'system-preset-option-Record demo'`, etc.), so a single `tap:` selects the value without opening a modal or scrolling.
   - If a test ever switches to Detailed mode (`tap: { id: playground-detailed }`), scope any `scroll-to` to the upper `ScrollView` with `within: { role: ScrollView, next: { text: Audio I/O Playground } }`.
6. **Follow `argent-create-e2e` when recording new UI steps.**
   - Record steps live against the running app, use strict `testID` selectors (`{ id: ... }`), never commit device UDIDs in `.yaml` files, and verify each fragment with `e2e-execute` or `yarn test:e2e <test-name>`.

---

## Directory Structure & Golden Reference

Golden reference test: `apps/fabric-example/tests/recording-and-playback/`.

```
apps/fabric-example/
├── scripts/
│   ├── run-e2e-test.sh                # Entry point for `yarn test:e2e`
│   ├── journal-execution.mjs          # Log + journal streamer and chronological merger
│   └── log-stream-ios.sh              # iOS log streamer (simctl / pymobiledevice3)
└── tests/
    ├── scripts/
    │   ├── test-harness.sh            # Sourced by each test's run.sh
    │   └── prompt.mjs                 # Human prompt CLI (TTY + macOS osascript GUI fallback)
    ├── shared/
    │   └── open-audio-io-playground.yaml  # Opens Other -> Audio I/O Playground if not open
    └── <test-name>/
        ├── 01-<phase-1>.yaml          # Argent fragment 1
        ├── 02-<phase-2>.yaml          # Argent fragment 2
        ├── run.sh                     # Executable test orchestration script (chmod +x)
        └── logs/                      # Gitignored; stores <commit>_<test-name>_<timestamp>.log
```

---

## Writing a Test

### 1. Create `apps/fabric-example/tests/<test-name>/run.sh`

Make `run.sh` executable (`chmod +x run.sh`) and source `../scripts/test-harness.sh`:

```bash
#!/usr/bin/env bash
set -euo pipefail

TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$TEST_DIR/../scripts/test-harness.sh"

init_test \
  --subsystem "audioapi|js" \
  --module "AudioSessionManager|AudioEngine|IOSRecorder|SystemNotificationManager" \
  --level trace \
  --grep "interruption|routeChange|Record|playground" \
  "$@"

run_flow "01-start-recording.yaml"

prompt_action "Activate Siri on the device, speak for 3 seconds, then dismiss Siri."

run_flow "02-stop-and-play-take.yaml"

prompt_choice "What did you hear during playback of the recorded take?" \
  --pass "1" \
  "Speech before and after Siri was recorded and clearly audible" \
  "Only speech before Siri was audible (silent after Siri)" \
  "Playback produced no sound at all"
```

### 2. `test-harness.sh` API

| Function        | Usage                                                                              | Behavior                                                                                                                                                                                                                                                                                              |
| --------------- | ---------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `init_test`     | `init_test [log-defaults...] "$@"`                                                 | Resolves the target device once (`resolve_ios_target`), starts `journal-execution.mjs --headless` writing to `$TEST_DIR/logs/`, registers the app via `launch-app`, and sets an `EXIT` trap to merge `logs.log` + `journal.log` on exit. Always pass `"$@"` last so CLI flags override test defaults. |
| `run_flow`      | `run_flow "01-step.yaml"`                                                          | Appends `[flow:start]` to `journal.log`, runs `npx argent e2e run` on the resolved `--device <udid>`, auto-recovers if the physical device's `ArgentRunner` session went stale (`Code=41`), and appends `[flow:pass]` or `[flow:fail]`.                                                               |
| `prompt_action` | `prompt_action "<instruction>"`                                                    | Pauses for the human to perform an external action. Logs `[prompt:action:start]` and `[prompt:action:done]` (with optional tester note).                                                                                                                                                              |
| `prompt_choice` | `prompt_choice "<question>" [--pass "1,2"] [--continue "3"] "<opt1>" "<opt2>" ...` | Displays numbered options plus an automatic `Other (provide custom answer)` option. If `--pass` is set and the human picks another option, logs `[test:failed]` and exits `1`. Options listed in `--continue` log the same failure but let the test proceed; the run still exits `1` at the end.      |
| `prompt_text`   | `prompt_text "<question>"`                                                         | Prompts for free-form prose observation and logs `[prompt:question]` + `[prompt:answer]`.                                                                                                                                                                                                             |
| `journal_note`  | `journal_note "<text>"`                                                            | Appends a timestamped `[journal] <text>` line directly to `journal.log`.                                                                                                                                                                                                                              |

### 3. Log Collection Options for `init_test`

- `--subsystem LIST`: `|`-separated list (`audioapi` for `com.swmansion.audioapi`, `js` for `com.facebook.react.log`, `all`, or full subsystem name). Default: `audioapi`.
- `--module LIST`: `|`-separated iOS log categories (`LOG_MODULE` in `Log.h`), e.g. `'AudioEngine|AudioSessionManager'`. Default: all.
- `--level LEVEL`: `error`, `warning`, `debug`, or `trace` (keeps that level and all more severe levels).
- `--grep PATTERN`: Regular expression matched against device log lines only (never filters out `[journal]` entries).

### 4. Authoring Argent Fragments for `AudioIOPlayground`

Reuse `../shared/open-audio-io-playground.yaml` in the first fragment of a test:

```yaml
executionPrerequisite: FabricExample is running in the foreground on the target device
steps:
  - run: ../shared/open-audio-io-playground.yaml
  - echo: Apply and activate the default Record demo session preset
  - tap: { id: system-apply }
  - tap: { id: system-activate }
  - echo: Start recording in Audio I/O Playground
  - tap: { id: recorder-start }
  - await: { text: { in: { id: recorder-status }, matches: '^recording ' } }
```

Key `AudioIOPlayground` `testID`s (defined in `apps/common-app/src/other/AudioIOPlayground/`):

- **Mode toggle:** `playground-detailed` (shown while in Compact mode), `playground-compact` (shown while in Detailed mode)
- **System:** `system-preset`, `system-category`, `system-mode`, `system-options`, `system-options-done`, `system-voice-processing`, `system-apply`, `system-activate`, `system-deactivate`, `system-recreate-recorder`, `system-recreate-context`
- **Recorder:** `recorder-status` (`idle` / `recording N.N s` / `paused N.N s`), `recorder-level`, `recorder-start`, `recorder-pause`, `recorder-resume`, `recorder-stop`, `recorder-options`
- **Playback:** `playback-status` (`context <state>, <sourceState>`), `playback-last-take`, `playback-source`, `playback-loop`, `playback-play`, `playback-pause`, `playback-resume`, `playback-stop`, `playback-disconnect`, `playback-options`
- **Devices & Log:** `device-current-inputs`, `device-current-outputs`, `device-input`, `device-refresh`, `log`, `log-clear`
- **Option chips / modal options:** `<testID>-option-<value>`, e.g. `id: 'playback-source-option-Last take'`, `id: 'system-preset-option-Record demo'`. In Compact mode these are always visible inline.

---

## Running Tests & Inspecting Evidence

```bash
# Run all tests sequentially
yarn test:e2e

# List available tests
yarn test:e2e --list

# Run a specific test (use --gui when invoking from an agent shell without a TTY)
yarn test:e2e recording-and-playback --gui
yarn test:e2e recording-and-playback --simulator --level debug
```

- **When an agent runs a test via the `Shell` tool:** Pass `--gui` (or rely on `prompt.mjs`'s automatic fallback when `/dev/tty` is unavailable) so human prompts appear as native macOS dialogs (`osascript`).
- **Evidence location:** On exit, `journal-execution.mjs` merges `<run-dir>/logs.log` and `<run-dir>/journal.log` by their `YYYY-MM-DD HH:MM:SS.mmm` timestamps into:
  `apps/fabric-example/tests/<test-name>/logs/<commit-hash>_<test-name>_<timestamp>.log`.
- Read that `.log` file after a run to correlate `[flow:*]`, `[prompt:*]`, `[playground]` JS logs, and `[com.swmansion.audioapi]` native logs on one timeline.

---

_Maintenance: see [maintenance.md](maintenance.md)._
