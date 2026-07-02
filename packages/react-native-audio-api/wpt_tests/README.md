# Node Bindings (JSI-only)

This directory contains the Node.js bootstrap for running Web Audio WPT against
`react-native-audio-api`.

## What this provides

- Native Node addon (`wpt_tests/src`) using JSI HostObjects via `node-api-jsi`.
- JSI-backed runtime installation (`jsi_install.cpp`).
- Smoke WPT harness (`wpt_tests/wpt/wpt-harness.mjs`) with allowlist + skip policy.
- Vendored Web Audio API tests under `wpt_tests/webaudio/` (~3 MB, full `webaudio` subtree).

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

The Node harness exports only spec Web Audio API classes via `wpt-api.js` (no recorder, decoder, streamer, or worklet nodes). Failures for unimplemented spec APIs (e.g. `PannerNode`, `ChannelMergerNode`) are expected until those nodes land.
