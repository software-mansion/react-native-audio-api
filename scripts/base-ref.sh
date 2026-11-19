# Sourced, not run. The revision changes are measured against, and the files they touch.

# BASE_REF when set; HEAD^1 on a GitHub pull request (the checkout is a merge
# commit with the base branch first; needs fetch-depth 2); otherwise the
# merge-base with origin/main.
base_ref() {
  if [[ -n "${BASE_REF:-}" ]]; then
    printf '%s' "$BASE_REF"
  elif [[ "${GITHUB_EVENT_NAME:-}" == "pull_request" ]]; then
    printf 'HEAD^1'
  else
    git merge-base origin/main HEAD
  fi
}

# Files changed since base_ref, one per line, relative to the repository root.
# Uncommitted and untracked files count too.
changed_files() {
  local base root
  base="$(base_ref)" || return 1
  root="$(git rev-parse --show-toplevel)" || return 1
  (
    cd "$root" &&
      git diff --name-only "$base" -- &&
      git ls-files --others --exclude-standard
  ) | sort -u
}

# Staged files, one per line, relative to the repository root (deletions excluded).
staged_files() {
  git -C "$(git rev-parse --show-toplevel)" diff --cached --name-only --diff-filter=d
}
