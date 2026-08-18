# Node Bindings (JSI-only)

This directory contains the Node.js bootstrap for running Web Audio WPT against
`react-native-audio-api`.

## What this provides

- Native Node addon (`wpt_tests/src`) using JSI HostObjects via `node-api-jsi`.
- JSI-backed runtime installation (`jsi_install.cpp`).
- Smoke WPT harness (`wpt_tests/wpt/wpt-harness.mjs`) with allowlist + skip policy.
- Vendored Web Audio API tests under `wpt_tests/webaudio/` (~3 MB, full `webaudio` subtree).
- Manual conformance reporting (`wpt-results.mjs`) that produces a [wpt.fyi](https://wpt.fyi/results/webaudio/the-audio-api?label=experimental&label=master&aligned)-style markdown table.

## Prerequisites

- Node.js 22+
- CMake 3.14+
- Monorepo dependencies installed (`yarn install` at repo root)

## Local workflow

From `packages/react-native-audio-api`:

1. Build JS + native addon:
   - `yarn wpt:build`
2. List selected smoke tests:
   - `yarn wpt:list`
3. Run smoke WPT:
   - `yarn wpt`
4. Run filtered subset:
   - `node ./wpt_tests/wpt/wpt-harness.mjs --filter gain`

### Testing the JS layer of a published npm version

Set `RN_AUDIO_API_APP_ROOT` to an app directory to load the `react-native-audio-api` JS wrappers the way a `require()` from that app would resolve them (its `node_modules`, or a hoisted ancestor copy in a monorepo) instead of the workspace `lib/commonjs`:

```bash
RN_AUDIO_API_APP_ROOT=/path/to/app yarn wpt:only
```

Classes missing from an older version are skipped with a warning, so tests using them fail at runtime rather than crashing the harness. The native addon is not affected — it is always built from the workspace C++ sources.

## Maintainer conformance report

Full WPT runs are intentionally manual for docs refresh. CI runs the smoke profile
twice on every PR (base branch vs head), compares per-category pass counts, and
fails only when any category or the overall summary regresses relative to base.

A full smoke run completes in well under 5 minutes and is deterministic: the same
run always produces identical, complete numbers.

1. Build once:
   - `yarn wpt:build`
2. Run the report profile and write artifacts:
   - `yarn wpt:report`
   - writes `wpt_tests/results/latest.json`
   - writes `wpt_tests/results/latest.md`
3. Update the docs summary in one step:
   - `yarn wpt:report:docs`
   - runs the suite twice — once against the workspace build (`main`) and once
     against the stable release installed in `apps/common-app`
     (`yarn wpt:report:stable`, writes `wpt_tests/results/stable.json`)
   - regenerates the WPT summary block in
     `packages/audiodocs/docs/other/web-audio-api-coverage.mdx` as a
     side-by-side latest-release vs `main` table
4. Review and commit the docs change. Raw result files stay local by default.

CI helpers:

- `yarn wpt:ci-report`: build + smoke run with `--allow-failures` (always writes JSON)
- `yarn wpt:compare --baseline <base.json> --candidate <head.json>`: non-regression gate

Useful flags:

- `--profile smoke` (default): `the-audio-api` subtree, aligned with wpt.fyi
- `--profile full`: entire vendored `webaudio/` tree
- `--report-json <path>` / `--write-markdown <path>`: custom output locations
- `--allow-failures`: exit 0 after a completed run even when assertions fail
- `--update-docs`: rewrite the summary block in the audiodocs coverage page
- `yarn wpt:markdown`: regenerate markdown from an existing JSON report

The published conformance summary lives in the docs, not here:
`packages/audiodocs/docs/other/web-audio-api-coverage.mdx`.

## Troubleshooting

- **Native addon fails to load**
  - Rebuild with `yarn node:build`.
- **Device-related instability in CI**
  - Node test backend is sink-less; keep tests within the smoke profile.
- **Runner appears hung**
  - Kill stale processes: `pkill -f wpt-harness.mjs`
  - Some tests are excluded in `wpt/skip-list.json` (crashtests, AudioWorklet, known engine hangs).
- **Subset runs**
  - `yarn wpt --filter gain` or `node ./wpt_tests/wpt/wpt-harness.mjs --filter the-analysernode-interface`

## Scope

The Node harness exports only spec Web Audio API classes via `wpt-api.js` (no recorder, decoder, or worklet nodes). Failures for unimplemented spec APIs (e.g. `PannerNode`, `DynamicsCompressorNode`) are expected until those nodes land.
