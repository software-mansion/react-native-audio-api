#!/usr/bin/env bash

print_help() {
  cat <<'EOF'
Usage: ./scripts/validate.sh [tiers…]

Local-only validation (not run in CI). Combine flags as needed.

Tiers:
  --fast           CI parity: format, lint, typecheck, enum sync, build, C++ smoke + JS
  --cpp            C++ smoke only
  --cpp-extended   C++ extended (all registered categories)
  --cpp-full       C++ full (smoke, then all extended categories)
  --android        Android native build (requires ANDROID_HOME)
  --ios            iOS native build (macOS only)
  --full           --fast + C++ extended (all categories) + --android + --ios
                   (C++ full = smoke from --fast + extended; skips unavailable platforms)

Legacy aliases:
  --graph          Same as C++ extended category graph only
                   (prefer --cpp-extended, or yarn test:cpp:extended -- graph)

Examples:
  yarn validate:fast
  yarn validate:cpp
  yarn validate:cpp-extended
  yarn validate:full

Options:
  --help, -h       Show this help.
EOF
}

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
LIBRARY_DIR="$REPO_ROOT/packages/react-native-audio-api"
SCRIPTS_DIR="$LIBRARY_DIR/scripts"

RUN_FAST=false
RUN_CPP=false
RUN_CPP_EXTENDED=false
RUN_CPP_FULL=false
RUN_GRAPH=false
RUN_ANDROID=false
RUN_IOS=false
RUN_FULL=false

PREBUILD_CORE_DONE=false

log_step() {
  echo ""
  echo "==> $*"
  echo ""
}

is_macos() {
  [[ "$(uname -s)" == "Darwin" ]]
}

has_android_sdk() {
  local sdk_root="${ANDROID_HOME:-${ANDROID_SDK_ROOT:-}}"
  [[ -n "$sdk_root" && -d "$sdk_root" ]]
}

enable_ccache_if_available() {
  if command -v ccache >/dev/null 2>&1; then
    export CC="ccache ${CC:-clang}"
    export CXX="ccache ${CXX:-clang++}"
    log_step "ccache enabled (CC=$CC, CXX=$CXX)"
  fi
}

run_prebuild_core() {
  if [[ "$PREBUILD_CORE_DONE" == true ]]; then
    return 0
  fi

  log_step "Prebuild: yarn install --immutable"
  (cd "$REPO_ROOT" && yarn install --immutable)

  log_step "Prebuild: yarn build"
  (cd "$REPO_ROOT" && yarn build)

  log_step "Prebuild: C++ smoke (shared layer compile check)"
  (cd "$REPO_ROOT" && yarn workspace react-native-audio-api test:cpp)

  PREBUILD_CORE_DONE=true
}

download_prebuilts() {
  local platform="$1"
  log_step "Downloading prebuilt binaries ($platform)"
  (cd "$SCRIPTS_DIR" && bash download-prebuilt-binaries.sh "$platform")
}

run_prebuild_for_platform() {
  local platform="$1"
  run_prebuild_core
  download_prebuilts "$platform"
}

print_android_setup_instructions() {
  cat <<'EOF'
Android SDK not found. Set ANDROID_HOME or ANDROID_SDK_ROOT, then retry.

Example (macOS with Android Studio):
  export ANDROID_HOME="$HOME/Library/Android/sdk"
  export PATH="$ANDROID_HOME/platform-tools:$PATH"

Install Android SDK via Android Studio: Settings → Languages & Frameworks → Android SDK
EOF
}

run_fast() {
  log_step "Tier (--fast): CI parity checks"

  log_step "yarn install --immutable"
  (cd "$REPO_ROOT" && yarn install --immutable)

  log_step "yarn format:check"
  (cd "$REPO_ROOT" && yarn format:check)

  log_step "yarn lint"
  (cd "$REPO_ROOT" && yarn lint)

  log_step "yarn typecheck"
  (cd "$REPO_ROOT" && yarn typecheck)

  log_step "yarn check-audio-enum-sync"
  (cd "$REPO_ROOT" && yarn check-audio-enum-sync)

  log_step "yarn build"
  (cd "$REPO_ROOT" && yarn build)

  log_step "yarn workspace react-native-audio-api test:cpp"
  (cd "$REPO_ROOT" && yarn workspace react-native-audio-api test:cpp)

  log_step "yarn workspace react-native-audio-api test:js"
  (cd "$REPO_ROOT" && yarn workspace react-native-audio-api test:js)

  PREBUILD_CORE_DONE=true
}

run_cpp() {
  log_step "C++ smoke (--cpp)"
  run_prebuild_core
  (cd "$REPO_ROOT" && yarn workspace react-native-audio-api test:cpp:smoke)
}

run_cpp_extended() {
  log_step "C++ extended all categories (--cpp-extended)"
  run_prebuild_core
  (cd "$REPO_ROOT" && yarn workspace react-native-audio-api test:cpp:extended)
}

run_cpp_full() {
  log_step "C++ full (--cpp-full)"
  run_prebuild_core
  (cd "$REPO_ROOT" && yarn workspace react-native-audio-api test:cpp:full)
}

# Legacy alias: extended category graph only.
run_graph() {
  log_step "C++ extended category graph (--graph, legacy alias)"
  run_prebuild_core
  (cd "$REPO_ROOT" && yarn workspace react-native-audio-api test:cpp:extended -- graph)
}

run_android() {
  local allow_skip="${1:-false}"

  if ! has_android_sdk; then
    if [[ "$allow_skip" == true ]]; then
      echo ""
      echo "WARNING: Skipping Android validation — Android SDK not configured."
      print_android_setup_instructions
      return 0
    fi

    echo "ERROR: Android SDK not configured." >&2
    print_android_setup_instructions >&2
    exit 1
  fi

  log_step "Tier (--android): Android native build"

  run_prebuild_for_platform android

  log_step "yarn workspace react-native-audio-api build:android"
  (cd "$REPO_ROOT" && yarn workspace react-native-audio-api build:android)
}

run_ios() {
  local allow_skip="${1:-false}"

  if ! is_macos; then
    echo ""
    echo "Skipping iOS validation — macOS with Xcode required (current OS: $(uname -s))."
    return 0
  fi

  if ! command -v xcodebuild >/dev/null 2>&1; then
    if [[ "$allow_skip" == true ]]; then
      echo ""
      echo "WARNING: Skipping iOS validation — xcodebuild not found. Install Xcode."
      return 0
    fi

    echo "ERROR: xcodebuild not found. Install Xcode from the App Store." >&2
    exit 1
  fi

  log_step "Tier (--ios): iOS native build"

  run_prebuild_for_platform ios

  log_step "yarn workspace react-native-audio-api build:ios"
  (cd "$REPO_ROOT" && yarn workspace react-native-audio-api build:ios)
}

run_full() {
  log_step "Full local validation (--full)"
  run_fast
  # --fast already ran C++ smoke; extended completes C++ full without re-running smoke.
  log_step "C++ extended all categories (remainder of C++ full)"
  (cd "$REPO_ROOT" && yarn workspace react-native-audio-api test:cpp:extended)
  run_android true
  run_ios true
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --fast)
      RUN_FAST=true
      ;;
    --cpp)
      RUN_CPP=true
      ;;
    --cpp-extended)
      RUN_CPP_EXTENDED=true
      ;;
    --cpp-full)
      RUN_CPP_FULL=true
      ;;
    --graph)
      # Legacy alias for extended category graph only.
      RUN_GRAPH=true
      ;;
    --android)
      RUN_ANDROID=true
      ;;
    --ios)
      RUN_IOS=true
      ;;
    --full)
      RUN_FULL=true
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

if [[ "$RUN_FAST" == false && "$RUN_CPP" == false && "$RUN_CPP_EXTENDED" == false && "$RUN_CPP_FULL" == false && "$RUN_GRAPH" == false && "$RUN_ANDROID" == false && "$RUN_IOS" == false && "$RUN_FULL" == false ]]; then
  print_help >&2
  exit 1
fi

cd "$REPO_ROOT"

enable_ccache_if_available

if [[ "$RUN_FULL" == true ]]; then
  run_full
fi
if [[ "$RUN_FAST" == true && "$RUN_FULL" == false ]]; then
  run_fast
fi
if [[ "$RUN_CPP" == true ]]; then
  run_cpp
fi
if [[ "$RUN_CPP_EXTENDED" == true ]]; then
  run_cpp_extended
fi
if [[ "$RUN_CPP_FULL" == true ]]; then
  run_cpp_full
fi
if [[ "$RUN_GRAPH" == true ]]; then
  run_graph
fi
if [[ "$RUN_ANDROID" == true && "$RUN_FULL" == false ]]; then
  run_android false
fi
if [[ "$RUN_IOS" == true && "$RUN_FULL" == false ]]; then
  run_ios false
fi

log_step "Validation complete."
