# Sourced after common.sh. Android target selection and build products.

ANDROID_DIR="$APP_DIR/android"
readonly ANDROID_DIR
readonly APK_DIR="$ANDROID_DIR/app/build/outputs/apk"

has_adb() {
  command -v adb >/dev/null 2>&1
}

require_adb() {
  if ! has_adb; then
    fail "adb not found. Add \$ANDROID_HOME/platform-tools to PATH."
  fi
}

# `adb devices -l` marks a USB device with a usb: field. A Wi-Fi device has none, and an emulator's
# serial starts with emulator-. Only the "device" state is usable; "unauthorized" and "offline" are not.
first_usb_device_serial() {
  adb devices -l | awk '$2 == "device" && / usb:/ { print $1; exit }'
}

first_running_emulator_serial() {
  adb devices -l | awk '$2 == "device" && $1 ~ /^emulator-/ { print $1; exit }'
}

# Fills target_kind and target_id: the device the options name, otherwise the first USB device,
# otherwise the first running emulator. Both stay empty when there is neither, or no adb.
resolve_android_target() {
  if ! has_adb; then
    if [[ "$target_kind" == "device" ]]; then
      require_adb
    fi
    echo "adb not found, so no Android device or emulator was looked for." >&2
    return 0
  fi
  case "$target_kind" in
    device) ;;
    emulator) target_id="$(first_running_emulator_serial)" ;;
    "")
      target_id="$(first_usb_device_serial)"
      if [[ -n "$target_id" ]]; then
        target_kind="device"
      else
        target_id="$(first_running_emulator_serial)"
        if [[ -n "$target_id" ]]; then
          target_kind="emulator"
        fi
      fi
      ;;
  esac
}

require_running_android_target() {
  require_adb
  if [[ -n "$target_id" ]]; then
    return 0
  fi
  if [[ "$target_kind" == "emulator" ]]; then
    fail "No running Android emulator. Start one in Android Studio's Device Manager, or with 'emulator -avd <name>'."
  fi
  fail "No Android device on USB and no running emulator. Connect a device or start an emulator."
}

# Sets target_abi to the ABI the resolved target runs, or to nothing when there is no target.
# An emulator that is not running yet will run the host architecture.
resolve_target_abi() {
  target_abi=""
  if [[ -n "$target_id" ]]; then
    target_abi="$(adb -s "$target_id" shell getprop ro.product.cpu.abi | tr -d '\r')"
  elif [[ "$target_kind" == "emulator" ]]; then
    if [[ "$(uname -m)" == "arm64" ]]; then
      target_abi="arm64-v8a"
    else
      target_abi="x86_64"
    fi
  fi
}

android_apk_path() {
  local configuration="$1"
  echo "$APK_DIR/$configuration/app-$configuration.apk"
}
