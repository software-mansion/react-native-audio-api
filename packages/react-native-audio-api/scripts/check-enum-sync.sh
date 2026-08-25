#!/bin/bash

# Runs all cross-language enum sync checks required for this package.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

bash "$SCRIPT_DIR/check-audio-events-sync.sh"
bash "$SCRIPT_DIR/check-audio-file-properties-enum-sync.sh"
