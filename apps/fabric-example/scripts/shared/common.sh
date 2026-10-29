# Sourced by the FabricExample build and run scripts. Platform-independent helpers.

APP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
REPO_ROOT="$(cd "$APP_DIR/../.." && pwd)"
readonly APP_DIR REPO_ROOT
readonly LOG_LEVELS=(error warning debug trace)
readonly METRO_STATUS_URL="http://localhost:8081/status"

fail() {
  echo "$*" >&2
  exit 1
}

require_log_level() {
  local level="$1"
  if [[ " ${LOG_LEVELS[*]} " != *" $level "* ]]; then
    fail "Invalid level: '$level'. Expected one of: ${LOG_LEVELS[*]}"
  fi
}

require_configuration() {
  if [[ "$1" != "debug" && "$1" != "release" ]]; then
    fail "Invalid configuration: '$1'. Expected debug or release."
  fi
}

# Prints Debug or Release, the spelling of Xcode configurations and Gradle tasks.
# Takes a value require_configuration accepted: `fail` cannot stop the script from inside $(...).
capitalized_configuration() {
  if [[ "$1" == "release" ]]; then
    echo "Release"
  else
    echo "Debug"
  fi
}

# A debug build loads its JavaScript from Metro at launch. A release build embeds it.
require_metro_for() {
  local configuration="$1"
  if [[ "$configuration" == "debug" ]] \
    && [[ "$(curl --silent --max-time 2 "$METRO_STATUS_URL" || true)" != "packager-status:running" ]]; then
    fail "Metro is not running on port 8081. Start it with 'yarn start'."
  fi
}

# Set by the target options. target_kind is device, simulator or emulator, and stays empty when
# no option was given and the resolution found nothing. target_id is the UDID or adb serial.
target_kind=""
target_id=""

select_target() {
  if [[ -n "$target_kind" ]]; then
    fail "Pass only one target option."
  fi
  target_kind="$1"
  target_id="${2:-}"
}

# Prints the options that select the resolved target again, for forwarding it and for messages.
# A device id never contains spaces: it is a UDID or an adb serial.
target_options() {
  case "$target_kind" in
    device) echo "--device $target_id" ;;
    simulator|emulator) echo "--$target_kind" ;;
  esac
}
