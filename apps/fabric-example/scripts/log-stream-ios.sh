#!/usr/bin/env bash

print_help() {
  cat <<'EOF'
Usage: bash scripts/log-stream-ios.sh [--device UDID] [--module LIST] [--level LEVEL]

Streams the react-native-audio-api native log (subsystem com.swmansion.audioapi).
With no --device it uses the first iPhone or iPad connected over USB, and the
booted simulator when none is.

Options:
  --device UDID    Stream from this physical device. Requires pymobiledevice3
                   (pipx install pymobiledevice3), which is also what finds the
                   first USB device when this is omitted. Without it, the
                   booted simulator is used. List UDIDs with
                   'pymobiledevice3 usbmux list --usb'.
  --module LIST    Keep only these modules (log categories), separated by '|'.
                   Example: 'AudioEngine|AudioSessionManager'. Default: every module.
  --level LEVEL    Keep LEVEL and every more severe level, in the order
                   error, warning, debug, trace. Same meaning as AUDIO_LOG_LEVEL,
                   which decides what the app prints in the first place.
                   Default: every level.
  --help, -h       Show this help.

Examples:
  yarn workspace fabric-example log:stream:ios
  yarn workspace fabric-example log:stream:ios --module 'AudioEngine|AudioSessionManager' --level warning
  yarn workspace fabric-example log:stream:ios --device <udid> --level debug
EOF
}

set -euo pipefail

readonly SUBSYSTEM="com.swmansion.audioapi"
readonly LEVELS_BY_SEVERITY=(error warning debug trace)
readonly MODULE_NAME_PATTERN='^[A-Za-z_][A-Za-z0-9_]*$'

device_udid=""
modules=""
level=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --device)
      device_udid="${2:?--device needs a UDID}"
      shift
      ;;
    --module)
      modules="${2:?--module needs a value}"
      shift
      ;;
    --level)
      level="${2:?--level needs a value}"
      shift
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

selected_modules=()
if [[ -n "$modules" ]]; then
  IFS='|' read -r -a selected_modules <<<"$modules"
  for module in "${selected_modules[@]}"; do
    if [[ ! "$module" =~ $MODULE_NAME_PATTERN ]]; then
      echo "Invalid module name: '$module'" >&2
      exit 1
    fi
  done
fi

selected_levels=()
if [[ -n "$level" ]]; then
  for candidate in "${LEVELS_BY_SEVERITY[@]}"; do
    selected_levels+=("$candidate")
    [[ "$candidate" == "$level" ]] && break
  done
  if [[ "${selected_levels[${#selected_levels[@]} - 1]}" != "$level" ]]; then
    echo "Invalid level: '$level'. Expected one of: ${LEVELS_BY_SEVERITY[*]}" >&2
    exit 1
  fi
fi

# The unified log has no warning or trace type, so the level is matched on the tag that Log.h
# puts at the start of every message. Log.h writes debug lines as os_log_info and trace lines as
# os_log_debug, so both backends also skip those log types when the level excludes them.

stream_from_simulator() {
  if ! xcrun simctl list devices booted | grep -q "(Booted)"; then
    echo "No booted iOS simulator. Boot one, for example with 'yarn workspace fabric-example ios'." >&2
    exit 1
  fi

  local predicate="subsystem == \"$SUBSYSTEM\""
  if [[ ${#selected_modules[@]} -gt 0 ]]; then
    local quoted_modules
    quoted_modules="$(printf ', "%s"' "${selected_modules[@]}")"
    predicate+=" AND category IN {${quoted_modules#, }}"
  fi
  if [[ ${#selected_levels[@]} -gt 0 ]]; then
    local tag_matches
    tag_matches="$(printf ' OR eventMessage BEGINSWITH "[%s]"' "${selected_levels[@]}")"
    predicate+=" AND (${tag_matches# OR })"
  fi

  local stream_level
  case "$level" in
    error|warning) stream_level="default" ;;
    debug) stream_level="info" ;;
    *) stream_level="debug" ;;
  esac

  exec xcrun simctl spawn booted log stream \
    --style compact \
    --level "$stream_level" \
    --predicate "$predicate"
}

# Apple's tools cannot stream a device log: `log stream` has no device option, and
# `log collect --device` takes a snapshot without info and debug lines. pymobiledevice3 reads the
# same live os_trace relay Console.app uses.
stream_from_device() {
  if ! command -v pymobiledevice3 >/dev/null 2>&1; then
    echo "pymobiledevice3 not found. Install it with 'pipx install pymobiledevice3'." >&2
    exit 1
  fi

  local arguments=(syslog live --udid "$device_udid" --subsystem "$SUBSYSTEM" --label)
  local module
  for module in "${selected_modules[@]+"${selected_modules[@]}"}"; do
    arguments+=(--category "$module")
  done
  case "$level" in
    error|warning) arguments+=(--no-info --no-debug) ;;
    debug) arguments+=(--no-debug) ;;
  esac
  if [[ ${#selected_levels[@]} -gt 0 ]]; then
    # pymobiledevice3 renders "... <LEVEL>: message", so ">: " anchors the tag to the message start.
    local level_alternatives
    level_alternatives="$(IFS='|'; echo "${selected_levels[*]}")"
    arguments+=(--regex ">: \\[($level_alternatives)\\] ")
  fi

  exec pymobiledevice3 "${arguments[@]}"
}

# pymobiledevice3 prints a JSON list of UDIDs. The first one is the device usbmux reports first.
first_usb_udid() {
  pymobiledevice3 usbmux list --usb --simple \
    | sed -n 's/^[[:space:]]*"\([^"]*\)".*/\1/p' \
    | head -1
}

if [[ -z "$device_udid" ]]; then
  if ! command -v pymobiledevice3 >/dev/null 2>&1; then
    echo "pymobiledevice3 is not installed, so no USB device is checked. Streaming from the booted simulator." >&2
  else
    device_udid="$(first_usb_udid)"
    if [[ -z "$device_udid" ]]; then
      echo "No USB device found. Streaming from the booted simulator." >&2
    fi
  fi
fi

if [[ -n "$device_udid" ]]; then
  echo "Streaming from USB device $device_udid" >&2
  stream_from_device
else
  stream_from_simulator
fi
