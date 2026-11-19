#!/bin/bash

print_help() {
  cat <<'EOF'
Usage: check-docs-fences.sh [path...]

Fails when a tracked .md / .mdx file under the given paths (default: the
current directory) has a blank line missing where oxfmt needs one:

  - a code fence opening right after a JSX/HTML tag line (<summary>…</summary>,
    <TabItem …>, <div>, …): oxfmt reads the fence as part of the HTML block and
    flattens the code into prose, or breaks the MDX;
  - a `:::` admonition closer right after a list item: oxfmt indents it into
    the list.

Add a blank line before the fence or the `:::`.
EOF
}

set -euo pipefail

if [[ "${1:-}" == "--help" || "${1:-}" == "-h" ]]; then
  print_help
  exit 0
fi

paths=("$@")
if [[ ${#paths[@]} -eq 0 ]]; then
  paths=(.)
fi

files=()
while IFS= read -r -d '' file; do
  files+=("$file")
done < <(git ls-files -z -- "${paths[@]/%//*.md}" "${paths[@]/%//*.mdx}")

if [[ ${#files[@]} -eq 0 ]]; then
  exit 0
fi

awk '
  FNR == 1 { prev = ""; in_fence = 0 }
  {
    is_fence = ($0 ~ /^[ \t]*(```|~~~)/)
    if (is_fence && !in_fence && prev ~ /^[ \t]*<\/?[A-Za-z][^>]*>[ \t]*$|<\/summary>[ \t]*$/) {
      print FILENAME ":" FNR ": add a blank line before this code fence (it follows a JSX/HTML tag line)"
      found = 1
    }
    if (!in_fence && prev ~ /^[ \t]*([-*+]|[0-9]+\.)[ \t]/ && $0 ~ /^:::[ \t]*$/) {
      print FILENAME ":" FNR ": add a blank line before this ::: (it follows a list item)"
      found = 1
    }
    if (is_fence) {
      in_fence = !in_fence
    }
    prev = $0
  }
  END { exit found }
' "${files[@]}"
