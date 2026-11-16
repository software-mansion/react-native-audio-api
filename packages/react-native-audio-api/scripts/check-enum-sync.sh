#!/bin/bash

# Verifies that the enums mirrored from C++ into Kotlin and TypeScript stay in sync.
#
# These enums cross the JNI and JSI boundaries as plain ints: Kotlin maps the value it
# receives back to its own enum by ordinal, and C++ static_casts the number it gets from
# TypeScript. So each mirror has to list the same entries in the same order with the same
# values. No compiler notices when they drift apart: a reordered or inserted entry
# silently remaps every value after it. Hence this check.
#
# Entry names are compared ignoring case and underscores, because the languages name
# them differently by convention (C++ `OPUS_OGG` vs TypeScript `OpusOgg`, C++ `Idle` vs
# Kotlin `IDLE`).

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PACKAGE_DIR="$(dirname "$SCRIPT_DIR")"

AUDIO_FILE_PROPERTIES="common/cpp/audioapi/utils/AudioFileProperties.h"
TYPES="src/types.ts"

# One entry per mirrored enum: <C++ header>|<C++ enum>|<mirror file>|<mirror enum>
MIRRORED_ENUMS=(
  "common/cpp/audioapi/events/AudioEvent.h|AudioEvent|android/src/main/java/com/swmansion/audioapi/system/AudioEvent.kt|AudioEvent"
  "common/cpp/audioapi/core/inputs/RecorderState.h|RecorderState|android/src/main/java/com/swmansion/audioapi/system/RecorderState.kt|RecorderState"
  "$AUDIO_FILE_PROPERTIES|FileFormat|$TYPES|FileFormat"
  "$AUDIO_FILE_PROPERTIES|FileDirectory|$TYPES|FileDirectory"
  "$AUDIO_FILE_PROPERTIES|BitDepth|$TYPES|BitDepth"
  "$AUDIO_FILE_PROPERTIES|IOSAudioQuality|$TYPES|IOSAudioQuality"
)

# Prints "Name=Value" per entry, in declaration order. Handles C++ `enum class`,
# TypeScript `export enum` and Kotlin `enum class`, in multi-line and single-line
# layouts, and stops at the `;` that separates Kotlin's entries from the rest of its
# enum body. An entry without an initializer takes the previous value + 1, which is
# also how a Kotlin ordinal counts.
extract_entries() {
  local file="$1" enum_name="$2"

  awk -v name="$enum_name" '
    { sub(/\/\/.*/, "") }
    !inside && $0 ~ "enum([ \t]+class)?[ \t]+" name "[ \t]*[:{]" {
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
    sed 's/[[:space:]]//g' |
    awk -F '=' -v value=-1 '
      $1 !~ /^[A-Za-z_][A-Za-z0-9_]*$/ { next }
      { value = (NF > 1) ? $2 + 0 : value + 1; print $1 "=" value }
    '
}

normalize() {
  tr -d '_' | tr '[:lower:]' '[:upper:]'
}

FAILED=0

for mirrored_enum in "${MIRRORED_ENUMS[@]}"; do
  IFS='|' read -r cpp_path cpp_enum mirror_path mirror_enum <<<"$mirrored_enum"

  CPP_FILE="$PACKAGE_DIR/$cpp_path"
  MIRROR_FILE="$PACKAGE_DIR/$mirror_path"

  for file in "$CPP_FILE" "$MIRROR_FILE"; do
    if [ ! -f "$file" ]; then
      echo "❌ $cpp_enum: file not found: $file"
      FAILED=1
      continue 2
    fi
  done

  CPP_ENTRIES=$(extract_entries "$CPP_FILE" "$cpp_enum")
  MIRROR_ENTRIES=$(extract_entries "$MIRROR_FILE" "$mirror_enum")

  if [ -z "$CPP_ENTRIES" ]; then
    echo "❌ $cpp_enum: found no entries in $cpp_path. Did the declaration change shape?"
    FAILED=1
    continue
  fi

  if [ -z "$MIRROR_ENTRIES" ]; then
    echo "❌ $mirror_enum: found no entries in $mirror_path. Did the declaration change shape?"
    FAILED=1
    continue
  fi

  if [ "$(echo "$CPP_ENTRIES" | normalize)" = "$(echo "$MIRROR_ENTRIES" | normalize)" ]; then
    echo "✅ $cpp_enum is in sync with $mirror_path ($(echo "$CPP_ENTRIES" | wc -l | tr -d ' ') entries)."
  else
    FAILED=1
    echo "❌ $cpp_enum differs from $mirror_enum in $mirror_path."
    echo ""
    echo "C++ ($cpp_path):"
    echo "$CPP_ENTRIES" | nl
    echo ""
    echo "Mirror ($mirror_path):"
    echo "$MIRROR_ENTRIES" | nl
    echo ""
    echo "Differences (case and underscores ignored):"
    diff <(echo "$CPP_ENTRIES" | normalize) <(echo "$MIRROR_ENTRIES" | normalize) || true
    echo ""
  fi
done

exit $FAILED
