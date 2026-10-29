#!/usr/bin/env bash

print_help() {
  cat <<'EOF'
Usage: bash scripts/build-and-run-android.sh [--configuration debug|release] [--device SERIAL | --emulator]

Builds, installs, and launches FabricExample via `react-native run-android --active-arch-only`.
Uses the first Android device on USB, otherwise the first running emulator.
A debug build needs Metro running (yarn start).

Options:
  --configuration CONFIG  debug (default) or release.
  --device SERIAL         Build for and launch on this device (adb serial).
  --emulator              Use the running emulator, even when a device is connected.
  --help, -h              Show this help.

Examples:
  yarn android
  yarn android --configuration release --emulator
EOF
}

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/shared/common.sh"
source "$SCRIPT_DIR/shared/android.sh"

configuration="debug"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --configuration)
      configuration="${2:?--configuration needs debug or release}"
      shift
      ;;
    --device)
      select_target device "${2:?--device needs a serial}"
      shift
      ;;
    --emulator)
      select_target emulator
      ;;
    --help|-h)
      print_help
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      print_help >&2
      exit 1
      ;;
  esac
  shift
done

require_configuration "$configuration"
resolve_android_target
require_running_android_target
require_metro_for "$configuration"

cd "$APP_DIR"
exec yarn react-native run-android \
  --device "$target_id" \
  --active-arch-only \
  --mode "$configuration" \
  --no-packager
