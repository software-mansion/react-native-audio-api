# Sourced by the FabricExample log and test scripts. Platform-independent helpers.

readonly LOG_LEVELS=(error warning debug trace)

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

# Set by the target options. target_kind is device or simulator, and stays empty when no option
# was given and the resolution found nothing. target_id is the UDID.
target_kind=""
target_id=""

select_target() {
  if [[ -n "$target_kind" ]]; then
    fail "Pass only one target option."
  fi
  target_kind="$1"
  target_id="${2:-}"
}
