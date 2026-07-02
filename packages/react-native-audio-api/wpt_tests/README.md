# Node Bindings (JSI-only)

This directory contains the Node.js bootstrap for running Web Audio WPT against
`react-native-audio-api`.

## What this phase provides

- Native Node addon scaffold (`wpt_tests/src`) using JSI HostObjects via `node-api-jsi`.
- JSI-backed runtime installation (`jsi_install.cpp`) wired through `AudioAPIModuleInstaller` infrastructure.
- Smoke WPT harness (`wpt_tests/wpt/wpt-harness.mjs`) with allowlist + skip policy.
- WPT source checkout as git submodule (`wpt_tests/wpt-src`).

## Prerequisites

- Node.js 22+
- CMake 3.14+
- `react-native` dependencies installed in the monorepo (`yarn install`)
- Initialized sparse WPT source:
  - `yarn wpt:init`

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
  - Rebuild with `yarn workspace react-native-audio-api node:build`.
- **No tests found**
  - Confirm `wpt_tests/wpt-src/webaudio` exists under this package directory.
- **Device-related instability in CI**
  - Node test backend is sink-less; keep tests within the smoke profile.
