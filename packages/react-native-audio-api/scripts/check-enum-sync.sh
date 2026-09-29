#!/bin/bash

# Verifies that the enums mirrored between C++ and Kotlin stay in sync.
#
# These enums cross the JNI boundary as plain ints: Kotlin maps the value it receives
# back to its own enum by ordinal, so both declarations have to list the same entries
# in the same order. Neither compiler can notice when they drift apart — a reordered
# or inserted entry silently remaps every value after it. Hence this check.
#
# Entries are compared ignoring case and underscores, because the two languages name
# them differently by convention (C++ `Idle` vs Kotlin `IDLE`).

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PACKAGE_DIR="$(dirname "$SCRIPT_DIR")"

# One entry per mirrored enum: <C++ header>|<C++ enum>|<Kotlin file>|<Kotlin enum>
MIRRORED_ENUMS=(
  "common/cpp/audioapi/events/AudioEvent.h|AudioEvent|android/src/main/java/com/swmansion/audioapi/system/AudioEvent.kt|AudioEvent"
  "common/cpp/audioapi/core/inputs/RecorderState.h|RecorderState|android/src/main/java/com/swmansion/audioapi/system/RecorderState.kt|RecorderState"
)

# Prints one entry name per line, in declaration order. Handles both the multi-line and
# the single-line enum layouts, drops `= value` initializers, and stops at the `;` that
# separates Kotlin's entries from the rest of its enum body.
extract_entries() {
  local file="$1" enum_name="$2"

  awk -v name="$enum_name" '
    { sub(/\/\/.*/, "") }
    !inside && $0 ~ "enum class[ \t]+" name "[ \t]*[:{]" {
      inside = 1
      sub(/.*\{/, "")
    }
    inside {
      if (match($0, /[};]/)) {
        print substr($0, 1, RSTART - 1)
        exit
      }
      print
    }
  ' "$file" |
    tr ',' '\n' |
    sed 's/=.*//; s/[[:space:]]//g' |
    grep -E '^[A-Za-z_][A-Za-z0-9_]*$' || true
}

normalize() {
  tr -d '_' | tr '[:lower:]' '[:upper:]'
}

FAILED=0

for mirrored_enum in "${MIRRORED_ENUMS[@]}"; do
  IFS='|' read -r cpp_path cpp_enum kotlin_path kotlin_enum <<<"$mirrored_enum"

  CPP_FILE="$PACKAGE_DIR/$cpp_path"
  KOTLIN_FILE="$PACKAGE_DIR/$kotlin_path"

  for file in "$CPP_FILE" "$KOTLIN_FILE"; do
    if [ ! -f "$file" ]; then
      echo "❌ $cpp_enum: file not found: $file"
      FAILED=1
      continue 2
    fi
  done

  CPP_ENTRIES=$(extract_entries "$CPP_FILE" "$cpp_enum")
  KOTLIN_ENTRIES=$(extract_entries "$KOTLIN_FILE" "$kotlin_enum")

  if [ -z "$CPP_ENTRIES" ]; then
    echo "❌ $cpp_enum: no entries found in $CPP_FILE — has the declaration changed shape?"
    FAILED=1
    continue
  fi

  if [ -z "$KOTLIN_ENTRIES" ]; then
    echo "❌ $kotlin_enum: no entries found in $KOTLIN_FILE — has the declaration changed shape?"
    FAILED=1
    continue
  fi

  if [ "$(echo "$CPP_ENTRIES" | normalize)" = "$(echo "$KOTLIN_ENTRIES" | normalize)" ]; then
    echo "✅ $cpp_enum is in sync ($(echo "$CPP_ENTRIES" | wc -l | tr -d ' ') entries)."
  else
    FAILED=1
    echo "❌ $cpp_enum is NOT in sync with $kotlin_enum!"
    echo ""
    echo "C++ ($cpp_path):"
    echo "$CPP_ENTRIES" | nl
    echo ""
    echo "Kotlin ($kotlin_path):"
    echo "$KOTLIN_ENTRIES" | nl
    echo ""
    echo "Differences (normalized):"
    diff <(echo "$CPP_ENTRIES" | normalize) <(echo "$KOTLIN_ENTRIES" | normalize) || true
    echo ""
  fi
done

exit $FAILED
