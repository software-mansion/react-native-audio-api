#!/usr/bin/env bash

print_help() {
  cat <<'EOF'
Usage: bash scripts/log-stream-ios.sh [--device UDID] [--subsystem LIST] [--module LIST] [--level LEVEL]

Streams iOS logs from the first USB device, or from the booted simulator if none is connected.
Devices need pymobiledevice3 (pipx install pymobiledevice3).

Options:
  --device UDID       Stream from this device.
  --subsystem LIST    Subsystems, separated by '|'. A full name is used as-is.
                      Shortcuts:
                        audioapi  com.swmansion.audioapi (default)
                        js        com.facebook.react.log
                        all       every subsystem

  --module LIST       Log categories, e.g. 'AudioEngine|AudioSessionManager'. Default: all.
  --level LEVEL       error, warning, debug or trace. Keeps that level and the more severe ones.
                      Default: all.
  --help, -h          Show this help.

Examples:
  yarn workspace fabric-example log:stream:ios --level warning
  yarn workspace fabric-example log:stream:ios --subsystem 'audioapi|js' --level trace
  yarn workspace fabric-example log:stream:ios --subsystem all
EOF
}

set -euo pipefail

readonly OWN_SUBSYSTEM="com.swmansion.audioapi"
readonly LEVELS_BY_SEVERITY=(error warning debug trace)
readonly MODULE_NAME_PATTERN='^[A-Za-z_][A-Za-z0-9_]*$'
readonly SUBSYSTEM_NAME_PATTERN='^[A-Za-z0-9][A-Za-z0-9_-]*(\.[A-Za-z0-9_-]+)+$'

device_udid=""
subsystems="audioapi"
modules=""
level=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --device)
      device_udid="${2:?--device needs a UDID}"
      shift
      ;;
    --subsystem)
      subsystems="${2:?--subsystem needs a value}"
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

selected_subsystems=()
if [[ "$subsystems" != "all" ]]; then
  IFS='|' read -r -a subsystem_list <<<"$subsystems"
  for subsystem in "${subsystem_list[@]}"; do
    case "$subsystem" in
      audioapi) selected_subsystems+=("$OWN_SUBSYSTEM") ;;
      js) selected_subsystems+=("com.facebook.react.log") ;;
      all)
        echo "'all' cannot be combined with other subsystems." >&2
        exit 1
        ;;
      *)
        if [[ ! "$subsystem" =~ $SUBSYSTEM_NAME_PATTERN ]]; then
          echo "Unknown subsystem: '$subsystem'. Use audioapi, js, all, or a full name such as com.apple.network." >&2
          exit 1
        fi
        selected_subsystems+=("$subsystem")
        ;;
    esac
  done
fi

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

# The unified log has no warning or trace type, so for OWN_SUBSYSTEM the level is matched on the
# tag Log.h puts at the start of every message. Other subsystems carry no such tag, so their level
# comes from the log type, using the same mapping Log.h uses: debug is info and trace is debug.
# Dropping those two types is done by the stream itself, for every subsystem. On top of that,
# `error` keeps only the error and fault types of other subsystems.

stream_from_simulator() {
  if ! xcrun simctl list devices booted | grep -q "(Booted)"; then
    echo "No booted iOS simulator. Boot one, for example with 'yarn workspace fabric-example ios'." >&2
    exit 1
  fi

  local clauses=()
  if [[ ${#selected_subsystems[@]} -gt 0 ]]; then
    local quoted_subsystems
    quoted_subsystems="$(printf ', "%s"' "${selected_subsystems[@]}")"
    clauses+=("subsystem IN {${quoted_subsystems#, }}")
  fi
  if [[ ${#selected_modules[@]} -gt 0 ]]; then
    local quoted_modules
    quoted_modules="$(printf ', "%s"' "${selected_modules[@]}")"
    clauses+=("category IN {${quoted_modules#, }}")
  fi
  if [[ ${#selected_levels[@]} -gt 0 ]]; then
    local tag_matches
    tag_matches="$(printf ' OR eventMessage BEGINSWITH "[%s]"' "${selected_levels[@]}")"
    local other_subsystems="subsystem != \"$OWN_SUBSYSTEM\""
    if [[ "$level" == "error" ]]; then
      other_subsystems+=' AND (messageType == "error" OR messageType == "fault")'
    fi
    clauses+=("((subsystem == \"$OWN_SUBSYSTEM\" AND (${tag_matches# OR })) OR ($other_subsystems))")
  fi

  local stream_level
  case "$level" in
    error|warning) stream_level="default" ;;
    debug) stream_level="info" ;;
    *) stream_level="debug" ;;
  esac

  local arguments=(log stream --style compact --level "$stream_level")
  if [[ ${#clauses[@]} -gt 0 ]]; then
    local predicate
    predicate="$(printf ' AND %s' "${clauses[@]}")"
    arguments+=(--predicate "${predicate# AND }")
  fi
  exec xcrun simctl spawn booted "${arguments[@]}"
}

# Apple's tools cannot stream a device log: `log stream` has no device option, and
# `log collect --device` takes a snapshot without info and debug lines. pymobiledevice3 reads the
# same live os_trace relay Console.app uses.
stream_from_device() {
  if ! command -v pymobiledevice3 >/dev/null 2>&1; then
    echo "pymobiledevice3 not found. Install it with 'pipx install pymobiledevice3'." >&2
    exit 1
  fi

  local arguments=(syslog live --udid "$device_udid" --label)
  local subsystem
  for subsystem in "${selected_subsystems[@]+"${selected_subsystems[@]}"}"; do
    arguments+=(--subsystem "$subsystem")
  done
  local module
  for module in "${selected_modules[@]+"${selected_modules[@]}"}"; do
    arguments+=(--category "$module")
  done
  case "$level" in
    error|warning) arguments+=(--no-info --no-debug) ;;
    debug) arguments+=(--no-debug) ;;
  esac
  if [[ ${#selected_levels[@]} -gt 0 ]]; then
    # A line is "<time> <process>{<image>}[<pid>] <TYPE>: <message> [<subsystem>][<category>]",
    # and a line is kept when any --regex matches it.
    local own_label="\\[${OWN_SUBSYSTEM//./\\.}\\]\\[[^]]*\\]\$"
    local level_alternatives
    level_alternatives="$(IFS='|'; echo "${selected_levels[*]}")"
    local other_subsystem_line="^(?!.* $own_label)"
    if [[ "$level" == "error" ]]; then
      other_subsystem_line+=".*?\\] <(ERROR|FAULT)>: "
    fi
    arguments+=(--regex ">: \\[($level_alternatives)\\] .* $own_label")
    arguments+=(--regex "$other_subsystem_line")
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
