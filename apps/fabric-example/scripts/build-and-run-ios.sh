#!/usr/bin/env bash

print_help() {
  cat <<'EOF'
Usage: bash scripts/build-and-run-ios.sh [--level LEVEL] [--configuration debug|release] [--device UDID | --simulator] [--pod-install] [--rn-cli [-- RN_ARGS...]]

Builds FabricExample with build-ios.sh, then installs and launches it with run-ios.sh.
Uses the first iOS device on USB, otherwise the first booted simulator.
A debug build needs Metro running (yarn start).

Options:
  --level LEVEL           AUDIO_LOG_LEVEL for the app: error (default), warning, debug or trace.
  --configuration CONFIG  debug (default) or release.
  --device UDID           Build for and launch on this physical device.
  --simulator             Use the booted simulator, even when a device is connected.
  --pod-install           Run pod install even when the pods look up to date.
  --rn-cli                Delegate to `react-native run-ios` instead of build-ios.sh + run-ios.sh.
                          Note: `react-native run-ios` cannot pass AUDIO_LOG_LEVEL to a physical device.
  --help, -h              Show this help.

Examples:
  yarn ios --level trace
  yarn ios --configuration release --simulator
  yarn ios --rn-cli --simulator
EOF
}

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/shared/common.sh"
source "$SCRIPT_DIR/shared/ios.sh"

level="error"
level_was_set=false
configuration="debug"
force_pod_install=false
use_rn_cli=false
rn_cli_extra_args=()

while [[ $# -gt 0 ]]; do
  case "$1" in
    --level)
      level="${2:?--level needs a value}"
      level_was_set=true
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
    --pod-install)
      force_pod_install=true
      ;;
    --rn-cli)
      use_rn_cli=true
      ;;
    --)
      shift
      rn_cli_extra_args=("$@")
      break
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

# Checked before the build, which can take minutes, rather than by run-ios.sh after it.
require_log_level "$level"
require_configuration "$configuration"

if [[ "$use_rn_cli" == false && ${#rn_cli_extra_args[@]} -gt 0 ]]; then
  fail "Extra arguments after '--' are only supported with --rn-cli."
fi

resolve_ios_target

if [[ "$use_rn_cli" == true ]]; then
  if [[ "$target_kind" == "device" && "$level_was_set" == true ]]; then
    fail "--level cannot be used with --rn-cli on a physical device: 'react-native run-ios' does not pass environment variables to devicectl. Drop --rn-cli to set AUDIO_LOG_LEVEL on a device."
  fi
  require_metro_for "$configuration"
  ensure_pods "$force_pod_install"

  rn_arguments=(run-ios --mode "$(capitalized_configuration "$configuration")" --no-packager)
  if [[ "$target_kind" == "device" ]]; then
    rn_arguments+=(--device "$target_id" --extra-params "-allowProvisioningUpdates")
  elif [[ -n "$target_id" ]]; then
    rn_arguments+=(--udid "$target_id")
  fi
  if [[ ${#rn_cli_extra_args[@]} -gt 0 ]]; then
    rn_arguments+=("${rn_cli_extra_args[@]}")
  fi

  cd "$APP_DIR"
  SIMCTL_CHILD_AUDIO_LOG_LEVEL="$level" exec yarn react-native "${rn_arguments[@]}"
fi

require_running_ios_target

read -r -a selected_target <<<"$(target_options)"
build_arguments=(--configuration "$configuration" "${selected_target[@]}")
if [[ "$force_pod_install" == true ]]; then
  build_arguments+=(--pod-install)
fi
bash "$SCRIPT_DIR/build-ios.sh" "${build_arguments[@]}"
bash "$SCRIPT_DIR/run-ios.sh" --level "$level" --configuration "$configuration" "${selected_target[@]}"
