#!/usr/bin/env bash

print_help() {
  cat <<'EOF'
Usage: bash scripts/build-android.sh [--configuration debug|release] [--device SERIAL | --emulator] [--all-architectures]

Builds the FabricExample APK.
Builds only the ABI of the first Android device on USB, otherwise of the first running emulator.
Without either, builds every ABI in android/gradle.properties.

Options:
  --configuration CONFIG  debug (default) or release. Both are signed with the debug keystore.
  --device SERIAL         Build for this device (adb serial).
  --emulator              Build for the emulator, even when a device is connected. Without a
                          running emulator, builds the host architecture.
  --all-architectures     Build every ABI in android/gradle.properties, whatever the target.
  --help, -h              Show this help.

Examples:
  yarn workspace fabric-example build:android
  yarn workspace fabric-example build:android --configuration release --emulator
EOF
}

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/shared/common.sh"
source "$SCRIPT_DIR/shared/android.sh"

configuration="debug"
all_architectures=false

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
    --all-architectures)
      all_architectures=true
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

arguments=(":app:assemble$(capitalized_configuration "$configuration")")
if [[ "$all_architectures" == true ]]; then
  echo "Building FabricExample ($configuration) for every ABI" >&2
else
  resolve_android_target
  resolve_target_abi
  if [[ -n "$target_abi" ]]; then
    arguments+=("-PreactNativeArchitectures=$target_abi")
    echo "Building FabricExample ($configuration) for $target_abi, the ABI of the ${target_kind}${target_id:+ $target_id}" >&2
  else
    echo "Building FabricExample ($configuration) for every ABI: no device or emulator found" >&2
  fi
fi

(cd "$ANDROID_DIR" && ./gradlew "${arguments[@]}")
echo "Built $(android_apk_path "$configuration")" >&2
