# Semi-automated E2E tests

Tests for behaviour that needs a person: speaking into the microphone, interrupting the audio
session, judging what was played. [Argent](https://www.npmjs.com/package/@swmansion/argent) drives the
app, the terminal asks the person for the rest. This is the "split a flow into multiple files" approach
from `packages/internaldocs/docs/testing/semi-automated-testing.md`.

## Running

Build and start FabricExample on the device first (`yarn ios`, Metro running), then:

```sh
bash apps/fabric-example/tests/recording-and-playback.sh
```

| Variable | Meaning |
|---|---|
| `AUDIO_LOG_LEVEL` | Verbosity of the library's native logs on a simulator: `error`, `warning`, `debug` or `trace` (default). |
| `E2E_LOG` | Path of the run log. Defaults to `/tmp/e2e-<test>-<time>.log`. |
| `E2E_DEVICE` | udid of the device to drive. Optional when exactly one iOS simulator is booted. |
| `APP_BUNDLE_ID` | Bundle id of the installed FabricExample. Set it for a build signed for a physical iPhone. |

A test exits non-zero at the first failed flow, unanswered prompt or "n" answer, and prints
`PASS: <test>` otherwise.

| Test | Device | The person |
|---|---|---|
| `recording-and-playback.sh` | simulator or iPhone | speaks, then judges the played take |
| `recording-interruption.sh` | physical iPhone | speaks, interrupts the recording, then judges the played take |
| `leftover-playback-playground.sh` | physical iPhone | interrupts a recording with a Safari call, then listens for the previous take during the next recording (Audio I/O Playground) |
| `leftover-playback-recorder-demo.sh` | physical iPhone | the same scenario on the Recorder demo |

The iOS simulator never interrupts an audio session, so `recording-interruption.sh` fails there at
`expect-interruption-logged`.

## The run log

On a simulator the harness records every run with `scripts/record-session-ios.sh`, the same tool used
for manual sessions, and prints `Log: <path>` when the test ends (`/tmp/e2e-<test>-<time>.log`, or
`E2E_LOG`). The file holds the library's native logs (`com.swmansion.audioapi`) and, tagged `[test]`,
everything the test did:

- every flow started and passed, every prompt, the answer, and the final `PASS` or `FAIL`;
- what the person typed: a note before Enter in `ask_to`, and what they observed after answering "n".

The library logs only warnings unless `AUDIO_LOG_LEVEL` is set when the app launches, so on a
simulator `start_app` launches the app through `simctl` with that variable. The recorder also streams
through `simctl`, so a run on a physical iPhone is not recorded.

## Writing a test

A test is a bash script that sources `harness.sh` and alternates between:

- `start_app` restarts FabricExample.
- `run_flow <name>` replays `flows/<name>.yaml`, an Argent e2e fragment. Its `executionPrerequisite`
  names the state the previous step has to leave the app in.
- `ask_to "<instruction>"` waits until the person presses Enter; a note typed first goes to the log.
- `confirm "<question>"` fails the test unless the person answers "y", and logs what they observed.
- `check "<question>"` is a `confirm` that lets the test continue: an "n" fails the test when it ends.
- `journal "<text>"` adds a `[test]` line of your own to the run log.

Record new fragments with Argent's e2e recorder against the Audio I/O Playground screen, whose
controls carry stable `testID`s.
