#!/bin/bash

# Verify AudioFileProperties enums stay in sync with TypeScript counterparts.
# Numeric values cross the JSI boundary via static_cast, so order + assigned
# integers must match even when naming style differs (WAV vs Wav).

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PACKAGE_DIR="$(dirname "$SCRIPT_DIR")"

CPP_FILE="$PACKAGE_DIR/common/cpp/audioapi/utils/AudioFileProperties.h"
TS_FILE="$PACKAGE_DIR/src/types.ts"

FAILED=0

if [ ! -f "$CPP_FILE" ]; then
  echo "❌ Error: C++ file not found: $CPP_FILE"
  exit 1
fi

if [ ! -f "$TS_FILE" ]; then
  echo "❌ Error: TypeScript file not found: $TS_FILE"
  exit 1
fi

# Extract "Name=Value" lines from a C++ nested enum class.
extract_cpp_enum() {
  local enum_name="$1"
  sed -n "/enum class ${enum_name}[[:space:]]*[:{]/,/};/p" "$CPP_FILE" |
    grep -E '^\s*[A-Za-z_][A-Za-z0-9_]*[[:space:]]*=' |
    sed -E 's/^[[:space:]]*([A-Za-z_][A-Za-z0-9_]*)[[:space:]]*=[[:space:]]*([0-9]+).*/\1=\2/' |
    sed 's/[[:space:]]*$//'
}

# Extract "Name=Value" lines from a TypeScript export enum.
extract_ts_enum() {
  local enum_name="$1"
  sed -n "/export enum ${enum_name} {/,/^}/p" "$TS_FILE" |
    grep -E '^\s*[A-Za-z_][A-Za-z0-9_]*[[:space:]]*=' |
    sed -E 's/^[[:space:]]*([A-Za-z_][A-Za-z0-9_]*)[[:space:]]*=[[:space:]]*([0-9]+).*/\1=\2/' |
    sed 's/[[:space:]]*$//'
}

# Compare numeric values in declaration order (names may differ in style).
compare_enum_values() {
  local label="$1"
  local cpp_pairs="$2"
  local ts_pairs="$3"

  local cpp_values ts_values
  cpp_values=$(echo "$cpp_pairs" | sed 's/.*=//')
  ts_values=$(echo "$ts_pairs" | sed 's/.*=//')

  if [ "$cpp_values" = "$ts_values" ]; then
    local count
    count=$(echo "$cpp_values" | grep -c . || true)
    echo "✅ ${label} values are in sync (${count} entries)."
  else
    echo "❌ ${label} values are NOT in sync!"
    echo ""
    echo "C++ ($CPP_FILE):"
    echo "$cpp_pairs" | nl
    echo ""
    echo "TypeScript ($TS_FILE):"
    echo "$ts_pairs" | nl
    echo ""
    echo "Value differences:"
    diff <(echo "$cpp_values") <(echo "$ts_values") || true
    echo ""
    FAILED=1
  fi
}

# Compare Name=Value pairs when both sides use the same identifiers.
compare_enum_pairs() {
  local label="$1"
  local cpp_pairs="$2"
  local ts_pairs="$3"

  if [ "$cpp_pairs" = "$ts_pairs" ]; then
    local count
    count=$(echo "$cpp_pairs" | grep -c . || true)
    echo "✅ ${label} enums are in sync (${count} entries)."
  else
    echo "❌ ${label} enums are NOT in sync!"
    echo ""
    echo "C++ ($CPP_FILE):"
    echo "$cpp_pairs" | nl
    echo ""
    echo "TypeScript ($TS_FILE):"
    echo "$ts_pairs" | nl
    echo ""
    echo "Differences:"
    diff <(echo "$cpp_pairs") <(echo "$ts_pairs") || true
    echo ""
    FAILED=1
  fi
}

compare_enum_values \
  "FileFormat / AudioFileProperties::Format" \
  "$(extract_cpp_enum Format)" \
  "$(extract_ts_enum FileFormat)"

compare_enum_pairs \
  "FileDirectory" \
  "$(extract_cpp_enum FileDirectory)" \
  "$(extract_ts_enum FileDirectory)"

compare_enum_pairs \
  "BitDepth" \
  "$(extract_cpp_enum BitDepth)" \
  "$(extract_ts_enum BitDepth)"

compare_enum_pairs \
  "IOSAudioQuality" \
  "$(extract_cpp_enum IOSAudioQuality)" \
  "$(extract_ts_enum IOSAudioQuality)"

if [ "$FAILED" -ne 0 ]; then
  exit 1
fi

exit 0
