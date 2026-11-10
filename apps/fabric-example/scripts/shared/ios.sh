# Sourced after common.sh. iOS target selection.

# Prints the UDID of the first iPhone or iPad connected over USB, or nothing.
# The list also holds simulators and paired devices that are not connected.
first_usb_device_udid() {
  local devices_json
  devices_json="$(mktemp)"
  if ! xcrun devicectl list devices --quiet --json-output "$devices_json" >/dev/null 2>&1; then
    rm -f "$devices_json"
    echo "devicectl could not list devices, so no USB device was looked for." >&2
    return 0
  fi
  python3 - "$devices_json" <<'PY'
import json
import sys

with open(sys.argv[1]) as devices_file:
    devices = json.load(devices_file)["result"]["devices"]
for device in devices:
    hardware = device["properties"]["hardware"]
    connection = device["properties"]["connection"]
    if (hardware.get("platform") == "iOS"
            and hardware.get("reality") == "physical"
            and connection.get("transportType") == "wired"
            and connection.get("state") == "connected"):
        print(hardware["udid"])
        break
PY
  rm -f "$devices_json"
}

# Prints the UDID of the first booted simulator, or nothing.
first_booted_simulator_udid() {
  xcrun simctl list devices booted --json | python3 -c '
import json
import sys

for runtime_devices in json.load(sys.stdin)["devices"].values():
    for device in runtime_devices:
        print(device["udid"])
        sys.exit(0)
'
}

# Fills target_kind and target_id: the device the options name, otherwise the first USB device,
# otherwise the first booted simulator. Both stay empty when there is neither.
resolve_ios_target() {
  case "$target_kind" in
    device) ;;
    simulator) target_id="$(first_booted_simulator_udid)" ;;
    "")
      target_id="$(first_usb_device_udid)"
      if [[ -n "$target_id" ]]; then
        target_kind="device"
      else
        target_id="$(first_booted_simulator_udid)"
        if [[ -n "$target_id" ]]; then
          target_kind="simulator"
        fi
      fi
      ;;
  esac
}

require_running_ios_target() {
  if [[ -n "$target_id" ]]; then
    return 0
  fi
  if [[ "$target_kind" == "simulator" ]]; then
    fail "No booted iOS simulator. Boot one with 'open -a Simulator'."
  fi
  fail "No iOS device on USB and no booted simulator. Connect a device or boot a simulator with 'open -a Simulator'."
}
