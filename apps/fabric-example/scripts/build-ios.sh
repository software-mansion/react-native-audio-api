#!/usr/bin/env bash

print_help() {
  cat <<'EOF'
Usage: bash scripts/build-ios.sh [--configuration debug|release] [--device UDID | --simulator] [--pod-install]

Builds FabricExample.app into Xcode's DerivedData directory. Does not install or launch it; run-ios.sh does that.
Builds for the first iOS device on USB, otherwise for the simulator.
Runs pod install first when the pods look out of date.

Options:
  --configuration CONFIG  debug (default) or release. Release compiles the native logs out.
  --device UDID           Build for a physical device. Signs with the development team set on
                          the FabricExample target.
  --simulator             Build for the simulator, even when a device is connected.
  --pod-install           Run pod install even when the pods look up to date.
  --help, -h              Show this help.

Examples:
  yarn workspace fabric-example build:ios
  yarn workspace fabric-example build:ios --configuration release --simulator
EOF
}

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/shared/common.sh"
source "$SCRIPT_DIR/shared/ios.sh"

configuration="debug"
force_pod_install=false

while [[ $# -gt 0 ]]; do
  case "$1" in
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
    --pod-install)
      force_pod_install=true
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
ensure_pods "$force_pod_install"
resolve_ios_target

arguments=(
  build
  -workspace "$WORKSPACE"
  -scheme FabricExample
  -configuration "$(capitalized_configuration "$configuration")"
  -quiet
)
if [[ "$target_kind" == "device" ]]; then
  # Automatic signing from the command line only fetches a missing provisioning profile when asked.
  arguments+=(-destination "id=$target_id" -allowProvisioningUpdates)
  echo "Building FabricExample ($configuration) for device $target_id" >&2
elif [[ -n "$target_id" ]]; then
  # Passing id=<udid> matches Xcode GUI and `react-native run-ios` (ONLY_ACTIVE_ARCH=YES),
  # whereas generic/platform=iOS Simulator triggers a full rebuild across architectures.
  arguments+=(-destination "id=$target_id")
  echo "Building FabricExample ($configuration) for simulator $target_id" >&2
else
  arguments+=(-destination 'generic/platform=iOS Simulator')
  echo "Building FabricExample ($configuration) for the simulator" >&2
fi

xcodebuild "${arguments[@]}"
echo "Built $(ios_app_path "$configuration")" >&2
