#!/bin/bash

print_help() {
  cat <<'EOF'
Usage: test-ios.sh [xcodebuild args...]

Runs the XCTests (FabricExampleTests) on an iPhone simulator: the first iPhone
on the newest installed iOS runtime, unless IOS_DESTINATION is set (an
xcodebuild -destination value). Needs `pod install` in ios/.

  pnpm --filter fabric-example test:ios
  IOS_DESTINATION='platform=iOS Simulator,OS=26.5,name=iPhone 17 Pro' pnpm --filter fabric-example test:ios
  pnpm --filter fabric-example test:ios -only-testing:FabricExampleTests/AudioEngineTests
EOF
}

set -euo pipefail

if [[ "${1:-}" == "--help" || "${1:-}" == "-h" ]]; then
  print_help
  exit 0
fi

IOS_DIR="$(cd "$(dirname "$0")/../ios" && pwd)"

destination="${IOS_DESTINATION:-}"
if [[ -z "$destination" ]]; then
  # A name alone resolves to OS=latest, which fails when that runtime has no such device.
  udid="$(
    xcrun simctl list devices available -j | jq -r '
      .devices | to_entries
      | map(select(.key | test("SimRuntime\\.iOS-")))
      | map({version: (.key | capture("iOS-(?<v>[0-9-]+)").v | split("-") | map(tonumber)), devices: .value})
      | sort_by(.version) | reverse
      | map(.devices[] | select(.name | startswith("iPhone"))) | first | .udid // empty'
  )"
  if [[ -z "$udid" ]]; then
    echo "error: no available iPhone simulator; install one in Xcode or set IOS_DESTINATION" >&2
    exit 1
  fi
  destination="id=${udid}"
fi

cd "$IOS_DIR"
exec xcodebuild test \
  -workspace FabricExample.xcworkspace \
  -scheme FabricExampleTests \
  -destination "$destination" \
  "$@"
