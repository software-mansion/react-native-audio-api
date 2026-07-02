#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(git rev-parse --show-toplevel)"
SUBMODULE_PATH="packages/react-native-audio-api/wpt_tests/wpt-src"

cd "${REPO_ROOT}"

# Keep submodule config aligned with .gitmodules.
git submodule sync -- "${SUBMODULE_PATH}"

# Ensure submodule is present and checked out to the commit pinned by superproject.
git submodule update --init --checkout --depth 1 --filter=blob:none "${SUBMODULE_PATH}"

# Reset any local drift inside submodule (edited/untracked files, stale sparse state).
git -C "${SUBMODULE_PATH}" reset --hard
git -C "${SUBMODULE_PATH}" clean -fd

# Sparse checkout: keep only webaudio tests locally.
git -C "${SUBMODULE_PATH}" sparse-checkout init --no-cone
git -C "${SUBMODULE_PATH}" sparse-checkout set webaudio
git -C "${SUBMODULE_PATH}" sparse-checkout reapply
