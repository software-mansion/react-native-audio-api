#!/bin/bash

print_help() {
  cat <<'EOF'
Usage: run-tests-docker.sh [run-tests.sh args…]

Thin Docker wrapper around run-tests.sh (Linux leak/ASan parity from macOS).
Forwards all arguments into the container. If none are given, runs:
  extended

Examples:
  run-tests-docker.sh
  run-tests-docker.sh extended --tsan
  run-tests-docker.sh extended graph --tsan
  run-tests-docker.sh --help   # this help (container not started)
  GTEST_FILTER='GraphTest.*' run-tests-docker.sh extended graph

See run-tests.sh --help and TESTING.md.
EOF
}

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../../../../.." && pwd)"

IMAGE_NAME=cpp-tests
CONTAINER_NAME=cpp-tests-container

if [[ $# -eq 1 && ( "$1" == "--help" || "$1" == "-h" ) ]]; then
  print_help
  exit 0
fi

if [[ $# -eq 0 ]]; then
  set -- extended
fi

docker build -t "$IMAGE_NAME" "${SCRIPT_DIR}"

docker run --rm -it \
  --name "$CONTAINER_NAME" \
  -v "$REPO_ROOT:/workspace" \
  -w /workspace/packages/react-native-audio-api/common/cpp/test \
  -e ASAN_OPTIONS=detect_leaks=1:verbosity=2 \
  ${GTEST_FILTER:+-e GTEST_FILTER="$GTEST_FILTER"} \
  "$IMAGE_NAME" \
  bash run-tests.sh "$@"
