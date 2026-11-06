#!/usr/bin/env bash

print_help() {
  cat <<'EOF'
Usage: bash scripts/run-android.sh [--configuration debug|release] [--device SERIAL | --emulator]

Installs and launches the APK built by build-android.sh via `react-native run-android --binary-path`.
Does not build. Launches on the first Android device on USB, otherwise on the first running emulator.
Build first for the same target and configuration. A debug build needs Metro running (yarn start).

Options:
  --configuration CONFIG  debug (default) or release.
  --device SERIAL         Launch on this device (adb serial).
  --emulator              Launch on the running emulator, even when a device is connected.
  --help, -h              Show this help.

Examples:
  yarn workspace fabric-example run:android
  yarn workspace fabric-example run:android --configuration release --emulator
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

apk_path="$(android_apk_path "$configuration")"
if [[ ! -f "$apk_path" ]]; then
  fail "$apk_path not found. Build it with 'yarn workspace fabric-example build:android --configuration $configuration $(target_options)'."
fi

require_metro_for "$configuration"

cd "$APP_DIR"
exec yarn react-native run-android \
  --binary-path "$apk_path" \
  --device "$target_id" \
  --mode "$configuration" \
  --no-packager
