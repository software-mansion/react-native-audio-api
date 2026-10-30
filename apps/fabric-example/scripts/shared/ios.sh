# Sourced after common.sh. iOS target selection, pod installation, and build products.

IOS_DIR="$APP_DIR/ios"
readonly IOS_DIR
readonly WORKSPACE="$IOS_DIR/FabricExample.xcworkspace"
readonly PODS_MANIFEST="$IOS_DIR/Pods/Manifest.lock"
readonly XCODE_DERIVED_DATA_ROOT="$HOME/Library/Developer/Xcode/DerivedData"

# A full check would evaluate the Podfile and every podspec, which is what pod install does. The
# first two conditions are the ones the build's own manifest check fails on. The modification
# times also catch changed inputs; after a checkout they can trigger an install that was not needed.
pods_look_outdated() {
  if [[ ! -f "$PODS_MANIFEST" ]] || ! cmp -s "$PODS_MANIFEST" "$IOS_DIR/Podfile.lock"; then
    return 0
  fi
  local inputs=(
    "$IOS_DIR/Podfile"
    "$APP_DIR/package.json"
    "$REPO_ROOT/yarn.lock"
    "$REPO_ROOT"/packages/*/*.podspec
  )
  [[ -n "$(find "${inputs[@]}" -newer "$PODS_MANIFEST")" ]]
}

# The Gemfile pins CocoaPods, but only when its gems are installed (bundle install).
install_pods() {
  echo "Installing pods" >&2
  if (cd "$APP_DIR" && bundle check >/dev/null 2>&1); then
    (cd "$IOS_DIR" && bundle exec pod install)
  else
    echo "The Gemfile's gems are not installed, so the pod on PATH is used." >&2
    (cd "$IOS_DIR" && pod install)
  fi
  # pod install leaves Manifest.lock untouched when nothing changed, so the newer inputs would
  # trigger it again on every build.
  touch "$PODS_MANIFEST"
}

ensure_pods() {
  local force="${1:-false}"
  if [[ "$force" == true ]] || pods_look_outdated; then
    install_pods
  fi
}

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

# Without a resolved target a build is for the simulator.
ios_sdk() {
  if [[ "$target_kind" == "device" ]]; then
    echo "iphoneos"
  else
    echo "iphonesimulator"
  fi
}

# Locates Xcode's DerivedData/FabricExample-<hash> folder for this checkout's workspace.
# Each folder has an info.plist with WorkspacePath; reading it avoids a 3s xcodebuild call.
ios_derived_data_dir() {
  local candidate workspace_path resolved_candidate_workspace
  for candidate in "$XCODE_DERIVED_DATA_ROOT"/FabricExample-*; do
    [[ -f "$candidate/info.plist" ]] || continue
    workspace_path="$(/usr/libexec/PlistBuddy -c 'Print :WorkspacePath' "$candidate/info.plist" 2>/dev/null || true)"
    if [[ "$workspace_path" == "$WORKSPACE" ]]; then
      echo "$candidate"
      return 0
    fi
  done

  for candidate in "$XCODE_DERIVED_DATA_ROOT"/FabricExample-*; do
    [[ -f "$candidate/info.plist" ]] || continue
    workspace_path="$(/usr/libexec/PlistBuddy -c 'Print :WorkspacePath' "$candidate/info.plist" 2>/dev/null || true)"
    [[ -n "$workspace_path" && -d "$(dirname "$workspace_path")" ]] || continue
    resolved_candidate_workspace="$(cd -P "$(dirname "$workspace_path")" && pwd -P)/$(basename "$workspace_path")"
    if [[ "$resolved_candidate_workspace" == "$WORKSPACE" ]]; then
      echo "$candidate"
      return 0
    fi
  done

  # Fallback when Xcode has not written info.plist yet (or DerivedData was relocated in Xcode settings).
  xcodebuild -workspace "$WORKSPACE" -scheme FabricExample -sdk "$(ios_sdk)" -showBuildSettings -json 2>/dev/null \
    | python3 -c '
import json, pathlib, sys
for entry in json.load(sys.stdin):
    if entry.get("target") == "FabricExample":
        # TARGET_BUILD_DIR is <DerivedData>/Build/Products/<Config>-<sdk>
        print(pathlib.Path(entry["buildSettings"]["TARGET_BUILD_DIR"]).parents[2])
        break
'
}

ios_app_path() {
  local configuration="$1"
  local xcode_configuration derived_data_dir
  xcode_configuration="$(capitalized_configuration "$configuration")"
  derived_data_dir="$(ios_derived_data_dir)"
  if [[ -z "$derived_data_dir" ]]; then
    echo "$XCODE_DERIVED_DATA_ROOT/FabricExample/Build/Products/$xcode_configuration-$(ios_sdk)/FabricExample.app"
    return 0
  fi
  echo "$derived_data_dir/Build/Products/$xcode_configuration-$(ios_sdk)/FabricExample.app"
}

ios_bundle_id() {
  local configuration="${1:-debug}"
  local project_bundle_id app_path
  project_bundle_id="$(awk -F' = ' '/PRODUCT_BUNDLE_IDENTIFIER/ { gsub(/;|"/, "", $2); print $2; exit }' \
    "$IOS_DIR/FabricExample.xcodeproj/project.pbxproj")"
  if [[ -n "$project_bundle_id" && "$project_bundle_id" != *'$('* ]]; then
    echo "$project_bundle_id"
    return 0
  fi
  app_path="$(ios_app_path "$configuration")"
  if [[ -f "$app_path/Info.plist" ]]; then
    /usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$app_path/Info.plist"
  fi
}
