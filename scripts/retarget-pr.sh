#!/usr/bin/env bash
# retarget-pr.sh
#
# Moves a pull request to another base branch together with its commits.
# Changing the base on GitHub only changes what the PR is compared against, so
# a fix branched from `main` shows all of main's history once its base is a
# release branch. This script replays only the PR's own commits onto the new
# base, pushes them to the PR's branch and switches the base.
#
# Usage:
#   scripts/retarget-pr.sh <pr-number> <target-branch> [--from <branch>] [--dry-run]
#
#   --from <branch>  Branch the PR's commits were started from. Defaults to the
#                    PR's current base, or `main` when that already is the target.
#   --dry-run        Show the commits and check that they apply, change nothing.
#
# Example:
#   scripts/retarget-pr.sh 1449 release/1.0
#
# Works for fork PRs when "Allow edits from maintainers" is enabled; it pushes
# with your own git credentials. Runs in a temporary worktree, so your checkout
# and uncommitted changes are never touched. Requires git and gh.

set -euo pipefail

usage() {
  sed -n '10,18p' "$0" | sed 's/^# \{0,1\}//'
  exit 1
}

fail() {
  echo "error: $*" >&2
  exit 1
}

PR_NUMBER=""
TARGET_BRANCH=""
SOURCE_BRANCH=""
DRY_RUN=false

while [[ $# -gt 0 ]]; do
  case "$1" in
    --from)
      SOURCE_BRANCH="${2:-}"
      [[ -n "$SOURCE_BRANCH" ]] || usage
      shift 2
      ;;
    --dry-run)
      DRY_RUN=true
      shift
      ;;
    -h | --help) usage ;;
    *)
      if [[ -z "$PR_NUMBER" ]]; then
        PR_NUMBER="$1"
      elif [[ -z "$TARGET_BRANCH" ]]; then
        TARGET_BRANCH="$1"
      else
        usage
      fi
      shift
      ;;
  esac
done

[[ "$PR_NUMBER" =~ ^[0-9]+$ && -n "$TARGET_BRANCH" ]] || usage
command -v gh >/dev/null || fail "gh is not installed"

REMOTE="${RETARGET_REMOTE:-origin}"
REPO_ROOT="$(git rev-parse --show-toplevel)"
cd "$REPO_ROOT"

IFS=$'\t' read -r PR_STATE CURRENT_BASE HEAD_BRANCH HEAD_OWNER HEAD_REPO IS_FORK MAINTAINER_CAN_MODIFY < <(
  gh pr view "$PR_NUMBER" \
    --json state,baseRefName,headRefName,headRepositoryOwner,headRepository,isCrossRepository,maintainerCanModify \
    --jq '[.state, .baseRefName, .headRefName, .headRepositoryOwner.login, .headRepository.name, .isCrossRepository, .maintainerCanModify] | @tsv'
)
[[ -n "${PR_STATE:-}" ]] || fail "could not read PR #$PR_NUMBER"
[[ "$PR_STATE" == "OPEN" ]] || fail "PR #$PR_NUMBER is $PR_STATE, not OPEN"

if [[ -z "$SOURCE_BRANCH" ]]; then
  if [[ "$CURRENT_BASE" == "$TARGET_BRANCH" ]]; then
    SOURCE_BRANCH="main"
  else
    SOURCE_BRANCH="$CURRENT_BASE"
  fi
fi
[[ "$SOURCE_BRANCH" != "$TARGET_BRANCH" ]] ||
  fail "the commits already start from $TARGET_BRANCH; pass --from <branch> if they don't"

if [[ "$IS_FORK" == "true" && "$MAINTAINER_CAN_MODIFY" != "true" ]]; then
  fail "PR #$PR_NUMBER comes from a fork without \"Allow edits from maintainers\"; ask the author to rebase onto $TARGET_BRANCH"
fi

echo "Fetching $SOURCE_BRANCH, $TARGET_BRANCH and PR #${PR_NUMBER}…"
git fetch --quiet "$REMOTE" "$SOURCE_BRANCH" "$TARGET_BRANCH"
PR_REF="refs/retarget/pr-$PR_NUMBER"
git fetch --quiet --force "$REMOTE" "pull/$PR_NUMBER/head:$PR_REF"

WORKTREE=""
cleanup() {
  if [[ -n "$WORKTREE" ]]; then
    git worktree remove --force "$WORKTREE" 2>/dev/null || true
  fi
  git update-ref -d "$PR_REF" 2>/dev/null || true
}
trap cleanup EXIT

PR_HEAD="$(git rev-parse "$PR_REF")"
SOURCE_TIP="$(git rev-parse "$REMOTE/$SOURCE_BRANCH")"
TARGET_TIP="$(git rev-parse "$REMOTE/$TARGET_BRANCH")"
FORK_POINT="$(git merge-base "$PR_HEAD" "$SOURCE_TIP")"

OWN_COMMITS=()
while IFS= read -r commit; do
  OWN_COMMITS+=("$commit")
done < <(git rev-list --reverse "$FORK_POINT..$PR_HEAD")

[[ ${#OWN_COMMITS[@]} -gt 0 ]] || fail "PR #$PR_NUMBER has no commits of its own on top of $SOURCE_BRANCH"
[[ -z "$(git rev-list --merges "$FORK_POINT..$PR_HEAD")" ]] ||
  fail "PR #$PR_NUMBER contains merge commits; rebase it by hand"

echo
echo "PR #$PR_NUMBER ($HEAD_OWNER:$HEAD_BRANCH): ${#OWN_COMMITS[@]} commit(s) started from $SOURCE_BRANCH"
git log --oneline --no-decorate "$FORK_POINT..$PR_HEAD" | sed 's/^/  /'

# Replays the PR's own commits onto `$1` in memory. On success prints the
# replayed tip; on a conflict prints the conflicting files to stderr.
replay_onto() {
  local tip="$1" commit merge_output
  for commit in "${OWN_COMMITS[@]}"; do
    if ! merge_output="$(git merge-tree --write-tree --name-only --merge-base="$commit^" "$tip" "$commit")"; then
      echo "    $(git log -1 --format='%h %s' "$commit") conflicts in:" >&2
      echo "$merge_output" | sed -n '2,/^$/p' | sed '/^$/d; s/^/      /' >&2
      return 1
    fi
    tip="$(git commit-tree "$(echo "$merge_output" | head -1)" -p "$tip" -m replay)"
  done
  echo "$tip"
}

# Files that still conflict once commits replayed onto an older point of the
# target are compared with its tip, which is what GitHub will report.
conflicts_with_target() {
  git merge-tree --write-tree --name-only "$1" "$TARGET_TIP" | sed -n '2,/^$/p' | sed '/^$/d' || true
}

# Landing points, newest first. Each is in the target's history, so GitHub
# compares the PR from there and shows only its own commits. The tip gives a
# PR ready to merge; an older point leaves conflicts to resolve in the PR.
LANDING_POINTS=(
  "$TARGET_TIP"
  "$(git merge-base "$SOURCE_TIP" "$TARGET_TIP")"
  "$(git merge-base "$FORK_POINT" "$TARGET_TIP")"
)

echo
LANDING_POINT=""
REMAINING_CONFLICTS=""
TRIED_POINTS=" "
for candidate in "${LANDING_POINTS[@]}"; do
  [[ "$TRIED_POINTS" == *" $candidate "* ]] && continue
  TRIED_POINTS+="$candidate "
  echo "Replaying onto $(git log -1 --format='%h %s' "$candidate")…"
  if replayed_tip="$(replay_onto "$candidate")"; then
    LANDING_POINT="$candidate"
    REMAINING_CONFLICTS="$(conflicts_with_target "$replayed_tip")"
    break
  fi
done

if [[ -z "$LANDING_POINT" ]]; then
  fail "the commits don't apply onto $TARGET_BRANCH or any point it shares with $SOURCE_BRANCH, so the fix needs code that only exists on $SOURCE_BRANCH; keep it there"
fi

if [[ -z "$REMAINING_CONFLICTS" ]]; then
  echo "  applies cleanly: the PR will be ready to merge"
else
  echo "  applies, but GitHub will report conflicts with $TARGET_BRANCH in:"
  echo "$REMAINING_CONFLICTS" | sed 's/^/    /'
  echo "  resolve them in the PR (\"Resolve conflicts\") or locally before merging"
fi

if $DRY_RUN; then
  echo
  echo "Dry run: nothing pushed, base left at $CURRENT_BASE."
  exit 0
fi

WORKTREE="$(mktemp -d "${TMPDIR:-/tmp}/retarget-pr-$PR_NUMBER.XXXXXX")"
git worktree add --quiet --detach "$WORKTREE" "$PR_HEAD"
git -C "$WORKTREE" rebase --quiet --onto "$LANDING_POINT" "$FORK_POINT"
REBASED_HEAD="$(git -C "$WORKTREE" rev-parse HEAD)"

if [[ "$IS_FORK" == "true" ]]; then
  PUSH_TARGET="https://github.com/$HEAD_OWNER/$HEAD_REPO.git"
else
  PUSH_TARGET="$REMOTE"
fi

echo
echo "Pushing $(git rev-parse --short "$REBASED_HEAD") to ${HEAD_OWNER}:${HEAD_BRANCH}…"
git push --quiet --force-with-lease="refs/heads/$HEAD_BRANCH:$PR_HEAD" \
  "$PUSH_TARGET" "$REBASED_HEAD:refs/heads/$HEAD_BRANCH"

if [[ "$CURRENT_BASE" != "$TARGET_BRANCH" ]]; then
  gh pr edit "$PR_NUMBER" --base "$TARGET_BRANCH" >/dev/null
fi

CONFLICT_NOTE=""
if [[ -n "$REMAINING_CONFLICTS" ]]; then
  CONFLICT_NOTE="$(printf '\nIt still conflicts with fixes already on `%s` in:\n\n%s\n\nThose need resolving here before merging.\n' \
    "$TARGET_BRANCH" "$(echo "$REMAINING_CONFLICTS" | sed 's/^/- `/; s/$/`/')")"
fi

gh pr comment "$PR_NUMBER" --body "$(cat <<EOF
Moved this PR onto \`$TARGET_BRANCH\` so it ships in the next patch release: its commits were replayed onto \`$TARGET_BRANCH\` and the branch was force-pushed.
$CONFLICT_NOTE
If you push to this branch again, reset your local copy first:

\`\`\`sh
git fetch <remote-of-your-fork>
git reset --hard <remote-of-your-fork>/$HEAD_BRANCH
\`\`\`
EOF
)" >/dev/null

if [[ -n "$REMAINING_CONFLICTS" ]]; then
  echo "Done: PR #$PR_NUMBER now targets $TARGET_BRANCH with ${#OWN_COMMITS[@]} commit(s); resolve its conflicts before merging."
else
  echo "Done: PR #$PR_NUMBER now targets $TARGET_BRANCH with ${#OWN_COMMITS[@]} commit(s)."
fi
