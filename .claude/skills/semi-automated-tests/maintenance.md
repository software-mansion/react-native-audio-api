# Maintenance — semi-automated-tests

> Used by `/pre-push-update` only — not loaded when the `semi-automated-tests` skill is active.

Review this skill when `pre-push-update` reports changes in:

| Path                                                  | What to check                                                                                    |
| ----------------------------------------------------- | ------------------------------------------------------------------------------------------------ |
| `apps/fabric-example/tests/scripts/**`                | `test-harness.sh` helper signatures (`init_test`, `run_flow`, `prompt_*`) and `prompt.mjs` flags |
| `apps/fabric-example/tests/shared/**`                 | Shared Argent fragment names and usage examples                                                  |
| `apps/fabric-example/tests/recording-and-playback/**` | Golden reference test layout and YAML examples                                                   |
| `apps/fabric-example/scripts/run-e2e-test.sh`         | `yarn test:e2e` CLI flags and behavior                                                           |
| `apps/fabric-example/scripts/journal-execution.mjs`   | `--headless`, `--output-dir`, `--run-dir-file`, and log merging behavior                         |
| `apps/fabric-example/scripts/log-stream-ios.sh`       | `--subsystem`, `--module`, `--level` filter options                                              |
| `apps/common-app/src/other/AudioIOPlayground/**`      | `AudioIOPlayground` layout (`ScrollView` structure) and control `testID`s                        |
