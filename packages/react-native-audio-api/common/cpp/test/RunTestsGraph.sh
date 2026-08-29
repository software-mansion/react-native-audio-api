#!/bin/bash

print_help() {
  cat <<'EOF'
Usage: RunTestsGraph.sh [options]

Legacy alias for:
  RunTests.sh extended graph [options]

Prefer: yarn test:cpp:extended -- graph

GRAPH_FILTER (legacy) is mapped to GTEST_FILTER when unset.
EOF
}

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

if [[ $# -eq 1 && ( "$1" == "--help" || "$1" == "-h" ) ]]; then
  print_help
  exit 0
fi

if [[ -n "${GRAPH_FILTER:-}" && -z "${GTEST_FILTER:-}" ]]; then
  export GTEST_FILTER="$GRAPH_FILTER"
fi

exec bash "${SCRIPT_DIR}/RunTests.sh" extended graph "$@"
