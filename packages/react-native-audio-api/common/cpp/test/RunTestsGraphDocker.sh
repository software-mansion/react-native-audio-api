#!/bin/bash

print_help() {
  cat <<'EOF'
Usage: RunTestsGraphDocker.sh [RunTests.sh args…]

Thin Docker wrapper around RunTests.sh (Linux leak/ASan parity from macOS).
Forwards all arguments into the container. If none are given, runs:
  extended graph

Examples:
  RunTestsGraphDocker.sh
  RunTestsGraphDocker.sh extended graph --tsan
  RunTestsGraphDocker.sh --help   # this help (container not started)
  GTEST_FILTER='GraphTest.*' RunTestsGraphDocker.sh extended graph

See RunTests.sh --help and TESTING.md.
EOF
}

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../../../../.." && pwd)"

IMAGE_NAME=asan-graph-test
CONTAINER_NAME=asan-graph-test-container

if [[ $# -eq 1 && ( "$1" == "--help" || "$1" == "-h" ) ]]; then
  print_help
  exit 0
fi

if [[ $# -eq 0 ]]; then
  set -- extended graph
fi

docker build -t "$IMAGE_NAME" "${SCRIPT_DIR}"

docker run --rm -it \
  --name "$CONTAINER_NAME" \
  -v "$REPO_ROOT:/workspace" \
  -w /workspace/packages/react-native-audio-api/common/cpp/test \
  -e ASAN_OPTIONS=detect_leaks=1:verbosity=2 \
  ${GTEST_FILTER:+-e GTEST_FILTER="$GTEST_FILTER"} \
  ${GRAPH_FILTER:+-e GRAPH_FILTER="$GRAPH_FILTER"} \
  "$IMAGE_NAME" \
  bash RunTests.sh "$@"
