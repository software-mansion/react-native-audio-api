#!/bin/bash

print_help() {
  cat <<'EOF'
Usage: changes.sh [--base <ref> | --staged | --all | --files <file>...]

Prints, as JSON, what a set of changes needs checked:

  {
    "all": false,           every package, because a shared path changed (or --all)
    "packages": [           packages with a `check` script: the ones the files are
      {"name": "...", "directory": "..."}   in, plus their workspace dependents
    ],
    "android": false,       the example app's native builds
    "ios": false,
    "wpt": false            the Web Platform Tests
  }

  --base <ref>  files changed since <ref>, uncommitted and untracked included
                (default: scripts/base-ref.sh, i.e. BASE_REF, HEAD^1 on a pull
                request, or the merge-base with origin/main)
  --staged      staged files
  --all         everything
  --files       the given paths, relative to the repository root
EOF
}

set -euo pipefail

ROOT="$(git rev-parse --show-toplevel)"
# shellcheck source=base-ref.sh
source "${ROOT}/scripts/base-ref.sh"

mode=base
case "${1:-}" in
  --help | -h)
    print_help
    exit 0
    ;;
  --base)
    BASE_REF="${2:?--base needs a ref}"
    ;;
  --staged) mode=staged ;;
  --all) mode=all ;;
  --files)
    mode=files
    shift
    given_files=("$@")
    ;;
  '') ;;
  *)
    echo "error: unexpected argument '$1'" >&2
    print_help >&2
    exit 1
    ;;
esac

# A change here can affect every package. Patterns are bash globs; `*` also
# matches `/`.
SHARED=(
  pnpm-lock.yaml pnpm-workspace.yaml .nvmrc '.github/actions/setup-env/*'
  package.json tsconfig.json
  .oxlintrc.json .oxfmtrc.json .clang-format .clang-tidy
  scripts/run-steps.sh scripts/changes.sh scripts/base-ref.sh scripts/clang-tidy.sh
  .github/workflows/ci.yml .github/workflows/check-package.yml
)
TOOLCHAIN=(pnpm-lock.yaml pnpm-workspace.yaml .nvmrc '.github/actions/setup-env/*')
NATIVE=(
  "${TOOLCHAIN[@]}"
  'packages/react-native-audio-api/common/*'
  'packages/react-native-audio-api/src/specs/*'
  'packages/react-native-audio-api/scripts/*'
  packages/react-native-audio-api/package.json
  'packages/react-native-audio-worklets/common/*'
  'packages/react-native-audio-worklets/src/specs/*'
  packages/react-native-audio-worklets/package.json
  apps/common-app/package.json apps/fabric-example/package.json
  .github/workflows/native-build.yml
)
ANDROID=(
  'packages/react-native-audio-api/android/*'
  'packages/react-native-audio-worklets/android/*'
  packages/react-native-audio-worklets/CMakeLists.txt
  'apps/fabric-example/android/*'
)
IOS=(
  'packages/react-native-audio-api/ios/*'
  packages/react-native-audio-api/RNAudioAPI.podspec
  'packages/react-native-audio-worklets/ios/*'
  packages/react-native-audio-worklets/RNAudioWorklets.podspec
  'apps/fabric-example/ios/*'
  apps/fabric-example/Gemfile apps/fabric-example/Gemfile.lock
)
WPT=(
  "${TOOLCHAIN[@]}"
  package.json
  'packages/react-native-audio-api/common/*'
  'packages/react-native-audio-api/src/*'
  'packages/react-native-audio-api/wpt_tests/*'
  packages/react-native-audio-api/scripts/build.sh
  packages/react-native-audio-api/package.json
  .github/workflows/wpt-tests.yml
)

files=()
case "$mode" in
  base) while IFS= read -r file; do files+=("$file"); done < <(changed_files) ;;
  staged) while IFS= read -r file; do files+=("$file"); done < <(staged_files) ;;
  files) files=("${given_files[@]+"${given_files[@]}"}") ;;
esac

# True when a changed file matches one of the patterns given.
any_matches() {
  local file pattern
  for file in "${files[@]+"${files[@]}"}"; do
    for pattern in "$@"; do
      # shellcheck disable=SC2053
      if [[ "$file" == $pattern ]]; then
        return 0
      fi
    done
  done
  return 1
}

workspace="$(cd "$ROOT" && pnpm ls -r --depth -1 --json)"

all=false
android=false
ios=false
wpt=false
if [[ "$mode" == all ]]; then
  all=true android=true ios=true wpt=true
else
  any_matches "${SHARED[@]}" && all=true
  any_matches "${NATIVE[@]}" "${ANDROID[@]}" && android=true
  any_matches "${NATIVE[@]}" "${IOS[@]}" && ios=true
  any_matches "${WPT[@]}" && wpt=true
fi

if [[ "$all" == true ]]; then
  selected="$workspace"
else
  # The workspace package each file belongs to (the deepest directory holding it).
  owners=()
  while IFS= read -r name; do
    owners+=("$name")
  done < <(
    printf '%s\n' "${files[@]+"${files[@]}"}" |
      jq -rR --argjson ws "$workspace" --arg root "$ROOT" '
        . as $file
        | [$ws[] | select(.path != $root) | {name, dir: (.path | ltrimstr($root + "/"))}
            | select(.dir as $dir | $file | startswith($dir + "/"))]
        | max_by(.dir | length) | .name // empty' |
      sort -u
  )
  if [[ ${#owners[@]} -eq 0 ]]; then
    selected='[]'
  else
    filters=()
    for name in "${owners[@]}"; do
      filters+=(--filter "...${name}")
    done
    selected="$(cd "$ROOT" && pnpm ls -r --depth -1 --json "${filters[@]}")"
  fi
fi

# Only packages with a `check` script run as CI jobs.
packages="$(
  jq -c --arg root "$ROOT" '
    [.[] | select(.path != $root)
      | {name, directory: (.path | ltrimstr($root + "/"))}]' <<<"$selected"
)"
with_check='[]'
while IFS= read -r package; do
  directory="$(jq -r '.directory' <<<"$package")"
  if jq -e '.scripts.check' "${ROOT}/${directory}/package.json" >/dev/null; then
    with_check="$(jq -c --argjson p "$package" '. + [$p]' <<<"$with_check")"
  fi
done < <(jq -c '.[]' <<<"$packages")

jq -n \
  --argjson all "$all" --argjson packages "$with_check" \
  --argjson android "$android" --argjson ios "$ios" --argjson wpt "$wpt" \
  '{all: $all, packages: ($packages | sort_by(.directory)), android: $android, ios: $ios, wpt: $wpt}'
