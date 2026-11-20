#!/bin/bash

print_help() {
  cat <<'EOF'
Usage: clang-tidy.sh [--warnings] [file...]

Runs clang-tidy on the package's C++, Objective-C++ and Objective-C, from the
package directory (`pnpm lint:cpp`). With files, checks only those in a compile
database.

Compile databases:
  common/cpp and iOS  the clangd CMake project (`pnpm setup:clangd`); iOS files are
                      in it only when the example app's Pods are installed
  Android             Gradle's android/.cxx database, written by an Android build
A platform whose files have no compile commands is skipped, and the run says so.

Skipped: vendored code, C files, ios/spm, and NativeAudioWorkletsModule.cpp
(needs folly, which only Pods or Gradle provide).
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
ANDROID_DB=""
for candidate in "${PACKAGE_DIR}"/android/.cxx/*/*/arm64-v8a/compile_commands.json; do
  if [[ -f "$candidate" && (-z "$ANDROID_DB" || "$candidate" -nt "$ANDROID_DB") ]]; then
    ANDROID_DB="$candidate"
  fi
done

CLANG_TIDY_BIN="$(find_clang_tidy)"

# Prints "<database dir><TAB><file>" per lintable file, then notes on stderr the tracked
# platform files that no database covers.
lintable_files() {
  python3 - "$PACKAGE_DIR" "$DB" "$ANDROID_DB" <<'PY'
import json, os, subprocess, sys

package_dir, clangd_db, android_db = sys.argv[1], sys.argv[2], sys.argv[3]
package_dir = os.path.realpath(package_dir)
excluded = ("/audioapi/libs/", "/audioapi/external/", "/dsp/r8brain/", "/clangd/build/", "/ios/spm/")
# Needs folly (see --help).
excluded_files = ("/audioworklets/NativeAudioWorkletsModule.cpp",)
extensions = (".cpp", ".h", ".hpp", ".mm", ".m")
platform_dirs = {"iOS": "ios/", "Android": "android/src/main/cpp/"}


def entries(db_path):
    if not db_path:
        return []
    database_dir = os.path.dirname(db_path)
    files = []
    for entry in json.load(open(db_path)):
        path = os.path.realpath(os.path.join(entry["directory"], entry["file"]))
        files.append((database_dir, path))
    return files


seen = set()
for database_dir, path in entries(clangd_db) + entries(android_db):
    if not path.startswith(package_dir + "/") or any(part in path for part in excluded):
        continue
    if path.endswith(excluded_files) or os.path.splitext(path)[1] not in extensions:
        continue
    if path not in seen:
        seen.add(path)
        print(f"{database_dir}\t{path}")

tracked = subprocess.run(
    ["git", "ls-files", "--full-name", "--", "*.cpp", "*.mm", "*.m"],
    cwd=package_dir, capture_output=True, text=True, check=True,
).stdout.split()
top = subprocess.run(["git", "rev-parse", "--show-toplevel"], cwd=package_dir,
                     capture_output=True, text=True, check=True).stdout.strip()
for platform, prefix in platform_dirs.items():
    missing = []
    for name in tracked:
        path = os.path.realpath(os.path.join(top, name))
        relative = os.path.relpath(path, package_dir)
        if relative.startswith(prefix) and not any(part in path for part in excluded) and path not in seen:
            missing.append(relative)
    if missing:
        source = ("the clangd database, which has iOS files only with the example app's Pods installed"
                  if platform == "iOS" else "Gradle's database, written by an Android build of the example app")
        print(f"{platform}: not linted, no compile commands in {source}: {', '.join(sorted(missing))}",
              file=sys.stderr)
PY
}

databases=()
files=()
add_file() {
  databases+=("$1")
  files+=("$2")
}
if [[ $# -gt 0 ]]; then
  lintable="$(lintable_files 2>/dev/null)"
  for file in "$@"; do
    path="$(cd "$(dirname "$file")" && pwd)/$(basename "$file")"
    database="$(awk -F '\t' -v path="$path" '$2 == path { print $1; exit }' <<<"$lintable")"
    if [[ -n "$database" ]]; then
      add_file "$database" "$path"
    fi
  done
else
  while IFS=$'\t' read -r database path; do
    add_file "$database" "$path"
  done < <(lintable_files)
fi

if [[ ${#files[@]} -eq 0 ]]; then
  exit 0
fi

echo "clang-tidy ($("$CLANG_TIDY_BIN" --version | grep -o 'version [0-9.]*')): ${#files[@]} files" >&2

OUTPUT="$(mktemp)"
trap 'rm -f "$OUTPUT"' EXIT

# One parallel run per database, since `-p` names a single one.
status=0
for database in $(printf '%s\n' "${databases[@]}" | sort -u); do
  for i in "${!files[@]}"; do
    if [[ "${databases[$i]}" == "$database" ]]; then
      printf '%s\0' "${files[$i]}"
    fi
  done |
    xargs -0 -n 4 -P "$(sysctl -n hw.ncpu 2>/dev/null || nproc)" \
      "$CLANG_TIDY_BIN" -p "$database" --quiet >>"$OUTPUT" 2>&1 || status=$?
done

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
