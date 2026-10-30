---
sidebar_position: 2
sidebar_custom_props:
  tags:
    - wip
---

# Semi-Automated End-to-End Testing, Debugging and Evidence Collection System (S.A.T.D.E.C.S.)

## Rationale

Some audio features cannot be tested by fully automated E2E tests alone because they require physical or environmental interaction (speaking into a microphone, plugging or unplugging headphones, answering a phone call, locking the screen, or judging whether playback sounds clean). When these scenarios are tested purely by hand, the steps drift between runs and evidence is scattered across Xcode/Logcat output and ad-hoc notes.

This system standardizes semi-automated testing in `apps/fabric-example`:

- **Automated device steps** are driven by [Argent](https://docs.swmansion.com/argent/) YAML fragment workflows.
- **Human interactions and observations** are requested between workflows via interactive CLI prompts.
- **Native, JavaScript, and human journal logs** are streamed in parallel and merged chronologically into a single timestamped `.log` file per run.

---

## Practical Guide

### Quick Start & Running Tests

Tests run against an already running `FabricExample` app instance.

:::note Physical iOS Device Setup
When running on a physical iPhone, make sure **Developer Mode** is enabled (**Settings → Privacy & Security → Developer Mode**) and **Enable UI Automation** is turned **ON** under **Settings → Developer → Enable UI Automation**. On the first Argent run, trust the developer certificate under **Settings → General → VPN & Device Management**.
:::

```bash
# 1. Start Metro (if using a debug build)
yarn start

# 2. Build and launch FabricExample with native trace logs (iOS)
yarn ios --level trace             # device-first: USB iPhone or booted simulator
# or: yarn ios --simulator --level trace

# 3. Run all semi-automated E2E tests sequentially
yarn test:e2e

# 4. Or list and run a specific test
yarn test:e2e --list
yarn test:e2e recording-and-playback
yarn test:e2e recording-and-playback --simulator --level debug
```

All flags passed to `yarn test:e2e` (`--device <udid>`, `--simulator`, `--level <level>`, `--gui`, `--grep <regex>`) are forwarded directly to each test's `run.sh` script.

When a test finishes, it prints the location of the final merged evidence file:
`apps/fabric-example/tests/<test-name>/logs/<commit>_<test-name>_<timestamp>.log`.

---

### Writing a New Test

The simplest way to create a new test is to describe the scenario to an AI agent (what automated UI steps should run, when the human should interact, and what outcome should be verified) and let the agent generate the `run.sh` script and record the Argent `.yaml` fragments.

Each test lives in its own directory under `apps/fabric-example/tests/<test-name>/`:

```
apps/fabric-example/tests/<test-name>/
├── 01-start-recording.yaml    # Argent fragment before human interaction
├── 02-stop-and-verify.yaml    # Argent fragment after human interaction
└── run.sh                     # Orchestration script (executable)
```

#### 1. Test Script (`run.sh`)

Create `apps/fabric-example/tests/<test-name>/run.sh` and make it executable (`chmod +x run.sh`):

#### 2. Argent Fragment Workflows (`.yaml`)

Every workflow `.yaml` in `apps/fabric-example/tests/` must be a **fragment** denoted with `executionPrerequisite: ...`. It shouldn't have a `launch:`.

#### 3. Available Prompt Types

The following functions are provided after sourcing `test-harness.sh`:

- `prompt_action "<instruction>"`: Instructs the human to perform an action. Appends `[prompt:action:start]` and `[prompt:action:done]` (with any optional human note) to `journal.log`.
- `prompt_choice "<question>" [--pass "1,2"] "<option 1>" "<option 2>" ...`: Presents numbered choices plus an automatic `Other (provide custom answer)` option. If `--pass` is specified and the user selects a non-passing option, logs `[test:failed]` and exits non-zero.
- `prompt_text "<question>"`: Prompts for free-form prose notes.
- `journal_note "<text>"`: Appends an arbitrary timestamped `[journal] <text>` line to `journal.log`.

Add `--gui` to `run.sh` or pass it via CLI to render prompts as native macOS dialogs (`osascript`) when running in environments without an interactive TTY.

Choice prompt example:

```text
── Human Verification ────────────────────────────────────────────────────
What did you observe when Siri was activated?
─────────────────────────────────────────────────────────────────────────
  1) Recording indicator stayed orange and resumed smoothly
  2) Recording stopped immediately and did not resume
  3) App crashed or audio became distorted
  4) Other (provide custom answer)

Select [1-4]: 1
Optional note (Enter to skip): audio resumed within ~0.5s after dismissing Siri
```

What gets recorded in the `.log` file:

```text
2026-10-29 19:20:21.115 [journal] [prompt:question] What did you observe when Siri was activated?
2026-10-29 19:20:29.440 [journal] [prompt:answer] What did you observe when Siri was activated? -> [1] Recording indicator stayed orange and resumed smoothly (note: audio resumed within ~0.5s after dismissing Siri)
```

If the tester selects option `2`, `3`, or `4`, the test harness logs `[test:failed]` and exits with code `1`, failing the run while preserving all captured native and JS logs up to that point.

---

### Interactive Manual Journaling

For exploratory manual debugging without an Argent script:

```bash
yarn journal-execution --name siri-interruption --subsystem 'audioapi|js' --level trace
```

- Streams filtered logs live to the terminal.
- Press **any key** to enter interactive journaling mode: terminal output freezes while background logs continue buffering, and an alternate screen prompt opens (`Enter` to save, `Esc` to discard).
- Press **`Ctrl+D`** to exit and save the chronologically merged log file in `apps/fabric-example/logs/<commit>[_<name>]_<timestamp>.log`.

---

### Commands Summary

| Command                                        | Script                             | Purpose                                                |
| ---------------------------------------------- | ---------------------------------- | ------------------------------------------------------ |
| `yarn test:e2e`                                | `scripts/run-e2e-test.sh`          | Runs all semi-automated tests sequentially.            |
| `yarn test:e2e --list`                         | `scripts/run-e2e-test.sh`          | Lists runnable tests in `apps/fabric-example/tests/`.  |
| `yarn test:e2e <name> [flags]`                 | `scripts/run-e2e-test.sh`          | Runs a specific test with optional target/level flags. |
| `yarn journal-execution [flags]`               | `scripts/journal-execution.mjs`    | Standalone interactive log streaming and journaling.   |
| `yarn workspace fabric-example log:stream:ios` | `scripts/log-stream-ios.sh`        | Streams filtered iOS logs from device or simulator.    |
| `yarn ios [--level trace]`                     | `scripts/build-and-run-ios.sh`     | Builds and launches iOS app with log level.            |
| `yarn android`                                 | `scripts/build-and-run-android.sh` | Builds and launches Android app.                       |

---

## Details

### Log Filtering Layers

Logs are filtered at three places depending on what is needed:

1. **Compile-time:** In Release builds, `AUDIO_LOG_*` macros expand to no-ops, so no log calls or format strings exist in the binary.
2. **Runtime:** In Debug builds, `Log.h` checks the `AUDIO_LOG_LEVEL` environment variable (`error` by default) once at startup and skips log calls above that verbosity level. It is especially useful with `trace` level logs, which tend to be located in significant number of places.
3. **Computer:** `log-stream-ios.sh` filters the OS log stream by `--subsystem`, `--module`, and `--level`, and `journal-execution.mjs` optionally filters log lines with `--grep` before saving to `logs.log` (human `[journal]` entries are never filtered out).

### Native iOS Logging (`Log.h`)

Header: `packages/react-native-audio-api/ios/audioapi/ios/utils/Log.h`.

Each Objective-C/Objective-C++ file defines `LOG_MODULE` before importing `<audioapi/ios/utils/Log.h>`:

```objc
#define LOG_MODULE AudioSessionManager
#import <audioapi/ios/utils/Log.h>
```

- **Subsystem & Category:** Writes to Apple's unified log under `com.swmansion.audioapi` with the category set to `LOG_MODULE`.
- **Levels:** `error`, `warning`, `debug`, and `trace`. Messages are prefixed with `[<level>]` (`AUDIO_LOG_ERROR`, `AUDIO_LOG_WARNING`, `AUDIO_LOG_DEBUG`, `AUDIO_LOG_TRACE`, and `AUDIO_LOG_TRACE_FUNCTION`).

### Limitations

Argent does not natively support interactive human prompts inside `.yaml` workflows (even its `script:` directive cannot pass arguments or read from the caller's terminal `stdin`). Because of this, each test must be split into multiple `.yaml` fragment files orchestrated by an external `run.sh` script. Native support in Argent for a `prompt` directive (or passing arguments to interactive scripts) would allow expressing an entire semi-automated test in a single `.yaml` file.

---

## ToDo
- Android
    - [ ] Log streaming
    - [ ] Log journaling
    - [ ] Tests support
- Logging
    - [ ] C++
    - [ ] Kotlin
