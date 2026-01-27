#!/bin/bash

# Script to verify AudioEvent enums are in sync between C++ and Kotlin files
# Exit with error if they differ in values or order

set -e

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PACKAGE_DIR="$(dirname "$SCRIPT_DIR")"

CPP_FILE="$PACKAGE_DIR/common/cpp/audioapi/events/AudioEvent.h"
KOTLIN_FILE="$PACKAGE_DIR/android/src/main/java/com/swmansion/audioapi/system/AudioEvent.kt"

# Check if files exist
if [ ! -f "$CPP_FILE" ]; then
  echo "❌ Error: C++ file not found: $CPP_FILE"
  exit 1
fi

if [ ! -f "$KOTLIN_FILE" ]; then
  echo "❌ Error: Kotlin file not found: $KOTLIN_FILE"
  exit 1
fi

# Extract enum values from C++ file (lines between typedef enum { and } AudioEvent;)
CPP_ENUMS=$(sed -n '/enum class AudioEvent {/,/};/p' "$CPP_FILE" | \
  grep -E '^\s*[A-Z_]+' | \
  sed 's/,//g' | \
  sed 's/^[[:space:]]*//' | \
  sed 's/[[:space:]]*$//')

# Extract enum values from Kotlin file (lines between enum class AudioEvent { and })
KOTLIN_ENUMS=$(sed -n '/enum class AudioEvent {/,/^}/p' "$KOTLIN_FILE" | \
  grep -E '^\s*[A-Z_]+' | \
  sed 's/,//g' | \
  sed 's/^[[:space:]]*//' | \
  sed 's/[[:space:]]*$//')

# Compare the enum values
if [ "$CPP_ENUMS" = "$KOTLIN_ENUMS" ]; then
  echo "✅ AudioEvent enums are in sync!"
  echo ""
  echo "Found $(echo "$CPP_ENUMS" | wc -l | tr -d ' ') enum values in both files."
  exit 0
else
  echo "❌ AudioEvent enums are NOT in sync!"
  echo ""
  echo "C++ enum values ($CPP_FILE):"
  echo "$CPP_ENUMS" | nl
  echo ""
  echo "Kotlin enum values ($KOTLIN_FILE):"
  echo "$KOTLIN_ENUMS" | nl
  echo ""
  echo "Differences:"
  diff <(echo "$CPP_ENUMS") <(echo "$KOTLIN_ENUMS") || true
  exit 1
fi
