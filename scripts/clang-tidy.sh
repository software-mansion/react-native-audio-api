#!/bin/bash

print_help() {
  cat <<'EOF'
Usage: clang-tidy.sh [--warnings] [file...]

Runs clang-tidy on the package's common/cpp, from the package directory
(`pnpm lint:cpp`). With files, checks only those in the compile database.

Skipped: vendored code, C files, iOS/Android glue, and
NativeAudioWorkletsModule.cpp (needs folly, which only Pods or Gradle provide).
Config: the root .clang-tidy and common/cpp/test/.clang-tidy.

Errors fail the run. Warnings are counted per check; --warnings lists them.
With CHECK_STAGED=1 (`pnpm check --staged`) and no files, checks the staged ones.

The database comes from `pnpm setup:clangd`. On CI it is regenerated each run.

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

if [[ $# -eq 0 && "${CHECK_STAGED:-}" == "1" ]]; then
  ROOT="$(git rev-parse --show-toplevel)"
  # shellcheck source=base-ref.sh
  source "${ROOT}/scripts/base-ref.sh"
  staged=()
  while IFS= read -r file; do
    if [[ "${ROOT}/${file}" == "${PACKAGE_DIR}/common/cpp/"* && "$file" =~ \.(cpp|h|hpp)$ ]]; then
      staged+=("${ROOT}/${file}")
    fi
  done < <(staged_files)
  if [[ ${#staged[@]} -eq 0 ]]; then
    exit 0
  fi
  set -- "${staged[@]}"
fi

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

CLANG_TIDY_BIN="$(find_clang_tidy)"

# Source files from the database, without vendored code and googletest.
db_files() {
  python3 - "$DB" "${PACKAGE_DIR}/common/cpp" <<'PY'
import json, os, sys

db_path, root = sys.argv[1], os.path.realpath(sys.argv[2])
excluded = ("/audioapi/libs/", "/audioapi/external/", "/dsp/r8brain/", "/clangd/build/")
# Needs folly (see --help).
excluded_files = ("/audioworklets/NativeAudioWorkletsModule.cpp",)
seen = set()
for entry in json.load(open(db_path)):
    path = os.path.realpath(os.path.join(entry["directory"], entry["file"]))
    if not path.startswith(root + "/") or any(part in path for part in excluded):
        continue
    if path.endswith(excluded_files):
        continue
    if os.path.splitext(path)[1] not in (".cpp", ".h", ".hpp"):
        continue
    if path not in seen:
        seen.add(path)
        print(path)
PY
}

files=()
if [[ $# -gt 0 ]]; then
  in_db="$(db_files)"
  for file in "$@"; do
    path="$(cd "$(dirname "$file")" && pwd)/$(basename "$file")"
    if grep -qxF "$path" <<<"$in_db"; then
      files+=("$path")
    fi
  done
else
  while IFS= read -r path; do
    files+=("$path")
  done < <(db_files)
fi

if [[ ${#files[@]} -eq 0 ]]; then
  exit 0
fi

echo "clang-tidy ($("$CLANG_TIDY_BIN" --version | grep -o 'version [0-9.]*')): ${#files[@]} files" >&2

OUTPUT="$(mktemp)"
trap 'rm -f "$OUTPUT"' EXIT

status=0
printf '%s\0' "${files[@]}" |
  xargs -0 -n 4 -P "$(sysctl -n hw.ncpu 2>/dev/null || nproc)" \
    "$CLANG_TIDY_BIN" -p "${CLANGD_DIR}/build" --quiet >"$OUTPUT" 2>&1 || status=$?

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
