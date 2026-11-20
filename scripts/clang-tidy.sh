#!/bin/bash

print_help() {
  cat <<'EOF'
Usage: clang-tidy.sh [--warnings] [file...]

Runs clang-tidy on the package's C++, Objective-C++ and Objective-C, from the
package directory (`pnpm lint:cpp`). With files, checks only those in a compile
database.

Compile databases, in order; each file is linted from the first that has it:
  the clangd CMake project (`pnpm setup:clangd`): common/cpp, and iOS when the
    example app's Pods are installed
  the package's Gradle database (android/.cxx): its Android code
  the example app's Gradle database: code only the app compiles for Android, such
    as worklets' NativeAudioWorkletsModule.cpp (needs folly)
The Gradle databases exist after an Android build of the example app. Files under
common/cpp, ios and android/src/main/cpp that no database has are skipped, and the
run names them.

Skipped: vendored code, C files, ios/spm.
Config: the root .clang-tidy, plus common/cpp/test/.clang-tidy and ios/.clang-tidy.

Errors fail the run. Warnings are counted per check; --warnings lists them.

On CI the clangd database is regenerated each run.

Environment:
  CLANG_TIDY  clang-tidy binary (default: PATH, then Homebrew's llvm)
EOF
}

set -euo pipefail

if [[ "${1:-}" == "--help" || "${1:-}" == "-h" ]]; then
  print_help
  exit 0
fi

LIST_WARNINGS=false
if [[ "${1:-}" == "--warnings" ]]; then
  LIST_WARNINGS=true
  shift
fi

PACKAGE_DIR="$(pwd)"
CLANGD_DIR="${PACKAGE_DIR}/common/cpp/clangd"
DB="${CLANGD_DIR}/build/compile_commands.json"

find_clang_tidy() {
  if [[ -n "${CLANG_TIDY:-}" ]]; then
    echo "$CLANG_TIDY"
  elif command -v clang-tidy >/dev/null; then
    command -v clang-tidy
  elif command -v brew >/dev/null && [[ -x "$(brew --prefix llvm 2>/dev/null)/bin/clang-tidy" ]]; then
    echo "$(brew --prefix llvm)/bin/clang-tidy"
  else
    echo "error: clang-tidy not found (install LLVM, e.g. \`brew install llvm\`, or set CLANG_TIDY)" >&2
    return 1
  fi
}

if [[ ! -d "$CLANGD_DIR" ]]; then
  echo "error: ${CLANGD_DIR} not found; run this from a package with a clangd CMake project" >&2
  exit 1
fi

if [[ "${CI:-}" == "true" ]]; then
  bash "${CLANGD_DIR}/generate-and-copy.sh" >&2
elif [[ ! -f "$DB" ]]; then
  echo "error: ${DB} not found; run \`pnpm setup:clangd\` once" >&2
  exit 1
fi

# Gradle writes one database per variant and ABI; the newest arm64 one is current.
newest_gradle_db() {
  local newest="" candidate
  for candidate in "$1"/.cxx/*/*/arm64-v8a/compile_commands.json; do
    if [[ -f "$candidate" && (-z "$newest" || "$candidate" -nt "$newest") ]]; then
      newest="$candidate"
    fi
  done
  echo "$newest"
}
# The package's own native build, and the example app, which also compiles code
# the package has no Android build for (worklets' C++).
PACKAGE_GRADLE_DB="$(newest_gradle_db "${PACKAGE_DIR}/android")"
APP_GRADLE_DB="$(newest_gradle_db "$(git rev-parse --show-toplevel)/apps/fabric-example/android/app")"

CLANG_TIDY_BIN="$(find_clang_tidy)"

# Prints "<database dir><TAB><file>" per lintable file, each from the first database
# that compiles it, then names on stderr the tracked sources no database covers.
lintable_files() {
  python3 - "$PACKAGE_DIR" "$DB" "$PACKAGE_GRADLE_DB" "$APP_GRADLE_DB" <<'PY'
import json, os, subprocess, sys

package_dir = os.path.realpath(sys.argv[1])
databases = [db for db in sys.argv[2:] if db]
excluded = ("/audioapi/libs/", "/audioapi/external/", "/dsp/r8brain/", "/clangd/build/", "/ios/spm/")
extensions = (".cpp", ".h", ".hpp", ".mm", ".m")
linted_dirs = ("common/cpp/", "ios/", "android/src/main/cpp/")

seen = set()
for db_path in databases:
    database_dir = os.path.dirname(db_path)
    for entry in json.load(open(db_path)):
        path = os.path.realpath(os.path.join(entry["directory"], entry["file"]))
        if not path.startswith(package_dir + "/") or any(part in path for part in excluded):
            continue
        if os.path.splitext(path)[1] not in extensions or path in seen:
            continue
        seen.add(path)
        print(f"{database_dir}\t{path}")

top = subprocess.run(["git", "rev-parse", "--show-toplevel"], cwd=package_dir,
                     capture_output=True, text=True, check=True).stdout.strip()
tracked = subprocess.run(
    ["git", "ls-files", "--full-name", "--", "*.cpp", "*.mm", "*.m"],
    cwd=package_dir, capture_output=True, text=True, check=True,
).stdout.split()
hints = {
    "ios/": "iOS files are in the clangd database only with the example app's Pods installed",
    "android/src/main/cpp/": "an Android build of the example app writes their compile commands",
    "common/cpp/": "the clangd project doesn't compile them; an Android build of the example app may",
}
missing = {prefix: [] for prefix in linted_dirs}
for name in tracked:
    path = os.path.realpath(os.path.join(top, name))
    relative = os.path.relpath(path, package_dir)
    prefix = next((p for p in linted_dirs if relative.startswith(p)), None)
    if prefix and path not in seen and not any(part in path for part in excluded):
        missing[prefix].append(relative)
for prefix, files in missing.items():
    if files:
        print(f"not linted, no compile commands ({hints[prefix]}): {', '.join(sorted(files))}",
              file=sys.stderr)
PY
}

databases=()
files=()
add_file() {
  databases+=("$1")
  files+=("$2")
}
# An assignment, not a process substitution, so a failed listing (e.g. a truncated
# database) stops the run instead of linting nothing and passing.
lintable="$(lintable_files)"
if [[ $# -gt 0 ]]; then
  for file in "$@"; do
    path="$(cd "$(dirname "$file")" && pwd)/$(basename "$file")"
    database="$(awk -F '\t' -v path="$path" '$2 == path { print $1; exit }' <<<"$lintable")"
    if [[ -n "$database" ]]; then
      add_file "$database" "$path"
    fi
  done
elif [[ -n "$lintable" ]]; then
  while IFS=$'\t' read -r database path; do
    add_file "$database" "$path"
  done <<<"$lintable"
fi

if [[ ${#files[@]} -eq 0 ]]; then
  exit 0
fi

echo "clang-tidy ($("$CLANG_TIDY_BIN" --version | grep -o 'version [0-9.]*')): ${#files[@]} files" >&2

OUTPUT="$(mktemp)"
trap 'rm -f "$OUTPUT"' EXIT

# One parallel run per database, since `-p` names a single one.
status=0
while IFS= read -r database; do
  for i in "${!files[@]}"; do
    if [[ "${databases[$i]}" == "$database" ]]; then
      printf '%s\0' "${files[$i]}"
    fi
  done |
    xargs -0 -n 4 -P "$(sysctl -n hw.ncpu 2>/dev/null || nproc)" \
      "$CLANG_TIDY_BIN" -p "$database" --quiet >>"$OUTPUT" 2>&1 || status=$?
done < <(printf '%s\n' "${databases[@]}" | sort -u)

python3 - "$OUTPUT" "$LIST_WARNINGS" <<'PY'
import collections, re, sys

output, list_warnings = sys.argv[1], sys.argv[2] == "true"
header = re.compile(r"^(/[^:]+:\d+:\d+): (warning|error): .* \[([^\]]+)\]$")

# A diagnostic is its first line plus the excerpt and notes below it.
blocks, current = [], None
for line in open(output, errors="replace"):
    match = header.match(line.rstrip("\n"))
    if match:
        current = [match.group(1), match.group(2), match.group(3), [line]]
        blocks.append(current)
    elif current is not None and not line.startswith(("Error while processing", "Suppressed", "Use -header-filter", "Found compiler error")):
        current[3].append(line)

seen, errors, warnings = set(), [], []
for location, severity, checks, lines in blocks:
    key = (location, checks)
    if key in seen:
        continue
    seen.add(key)
    (errors if severity == "error" else warnings).append((checks, lines))

if list_warnings:
    for _, lines in warnings:
        sys.stdout.write("".join(lines))
for _, lines in errors:
    sys.stdout.write("".join(lines))

if warnings:
    per_check = collections.Counter(
        check for checks, _ in warnings for check in checks.split(",") if check != "-warnings-as-errors"
    )
    summary = ", ".join(f"{check} {count}" for check, count in per_check.most_common())
    print(f"{len(warnings)} warnings (not failing; --warnings lists them): {summary}", file=sys.stderr)
print(f"{len(errors)} errors", file=sys.stderr)
PY

if [[ $status -ne 0 ]] && ! grep -qE ': error: ' "$OUTPUT"; then
  # Failed without a diagnostic (crash, bad flags).
  cat "$OUTPUT" >&2
fi
exit $status
