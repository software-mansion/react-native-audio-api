#!/usr/bin/env bash

print_help() {
  cat <<'EOF'
Usage: bash scripts/run-ios.sh [--level LEVEL] [--configuration debug|release] [--device UDID | --simulator]

Installs and launches the FabricExample.app built by build-ios.sh. Does not build.
Launches on the first iOS device on USB, otherwise on the first booted simulator.
Build first for the same target and configuration. A debug build needs Metro running (yarn start).

Options:
  --level LEVEL           AUDIO_LOG_LEVEL for the app: error (default), warning, debug or trace.
                          Release builds have no native logs, so it changes nothing there.
  --configuration CONFIG  debug (default) or release.
  --device UDID           Launch on this physical device.
  --simulator             Launch on the booted simulator, even when a device is connected.
  --help, -h              Show this help.

Examples:
  yarn workspace fabric-example run:ios --level trace
  yarn workspace fabric-example run:ios --level debug --simulator
EOF
}

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/shared/common.sh"
source "$SCRIPT_DIR/shared/ios.sh"

level="error"
configuration="debug"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --level)
      level="${2:?--level needs a value}"
      shift
      ;;
    --configuration)
      configuration="${2:?--configuration needs debug or release}"
      shift
      ;;
    --device)
      select_target device "${2:?--device needs a UDID}"
      shift
      ;;
    --simulator)
      select_target simulator
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

require_log_level "$level"
require_configuration "$configuration"
resolve_ios_target
require_running_ios_target

app_path="$(ios_app_path "$configuration")"
if [[ ! -d "$app_path" ]]; then
  fail "$app_path not found. Build it with 'yarn workspace fabric-example build:ios --configuration $configuration $(target_options)'."
fi

# The installed app keeps the bundle id it was built with, even if the target changed since.
bundle_id="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$app_path/Info.plist")"

require_metro_for "$configuration"

launch_on_simulator() {
  echo "Installing $bundle_id on simulator $target_id" >&2
  xcrun simctl install "$target_id" "$app_path"
  echo "Launching $bundle_id with AUDIO_LOG_LEVEL=$level" >&2
  # simctl has no environment option. It passes SIMCTL_CHILD_<NAME> to the app as <NAME>.
  SIMCTL_CHILD_AUDIO_LOG_LEVEL="$level" xcrun simctl launch --terminate-running-process "$target_id" "$bundle_id"
}

launch_on_device() {
  echo "Installing $bundle_id on device $target_id" >&2
  xcrun devicectl device install app --device "$target_id" "$app_path"
  echo "Launching $bundle_id with AUDIO_LOG_LEVEL=$level" >&2
  xcrun devicectl device process launch --device "$target_id" \
    --terminate-existing \
    --environment-variables "{\"AUDIO_LOG_LEVEL\":\"$level\"}" \
    "$bundle_id"
}

if [[ "$target_kind" == "device" ]]; then
  launch_on_device
else
  launch_on_simulator
fi
