#!/usr/bin/env bash
# find_level_zero_tag_git.sh
# Find the most recent level-zero tag whose history contains a commit
# referencing a given L0 Spec version (e.g. v1.15).
#
# Usage:
#   ./find_level_zero_tag_git.sh <spec_version> [repo_url]
#
# Examples:
#   ./find_level_zero_tag_git.sh 1.15
#   ./find_level_zero_tag_git.sh 1.15 https://github.com/myorg/level-zero

set -euo pipefail

SPEC_VERSION="${1:-}"
REPO_URL="${2:-https://github.com/oneapi-src/level-zero.git}"

if [[ -z "$SPEC_VERSION" ]]; then
  echo "Usage: $0 <spec_version> [repo_url]" >&2
  echo "  e.g. $0 1.15" >&2
  exit 1
fi

# Escape dots and add the string to search
ESCAPED=$(printf '%s' "$SPEC_VERSION" | sed 's/\./\\./g')
GREP_PATTERN="L0 Spec v${ESCAPED}"

WORK_DIR=$(mktemp -d)
trap 'rm -rf "$WORK_DIR"' EXIT

echo "Cloning $REPO_URL" >&2
# --bare + --filter=blob:none → fetch commits & tags but skip file blobs
git clone --bare --filter=blob:none --quiet "$REPO_URL" "$WORK_DIR/repo.git"
cd "$WORK_DIR/repo.git"

echo "Searching commit history for: ${GREP_PATTERN=}" >&2

# ── Step 1: find all commits whose message matches the spec version ───────────
# In case of pre-release, or not yet tagget we keep multiple of them
MATCHING_COMMITS=$(git log --all --oneline --grep="$GREP_PATTERN" --format="%H %s")

if [[ -z "$MATCHING_COMMITS" ]]; then
  echo "✘  No commit found referencing spec version: ${SPEC_VERSION}" >&2
  exit 1
fi
echo "${MATCHING_COMMITS}" >&2

# ── Step 3: find the most recent v* tag containing any of those commits ──────
CONTAINS_ARGS=$(awk '{print "--contains " $1}' <<<"$MATCHING_COMMITS")
MOST_RECENT=$(git tag -l --sort=-version:refname $CONTAINS_ARGS -- 'v*' | head -1)

if [[ -z "$MOST_RECENT" ]]; then
  echo "Matching commit(s) found but no v* tags contain them yet." >&2
  exit 1
fi

echo "Latest maching tag containing any of those commit" >&2
echo "$MOST_RECENT"
