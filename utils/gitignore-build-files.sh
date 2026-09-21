#!/usr/bin/env bash

set -euo pipefail

# Dynamically locate repository root so executing from ./utils/ updates top-level .gitignore
REPO_ROOT="$(git rev-parse --show-toplevel)"
GITIGNORE="$REPO_ROOT/.gitignore"

touch "$GITIGNORE"

# 'git status --porcelain' guarantees a stable output format across Git versions.
# Example output:
#   > git status --porcelain
#    M utils/gitignore-build-files.sh
#   ?? untracked-file

# Running git with '-C "$REPO_ROOT"' ensures returned paths are relative to repo root.
git -C "$REPO_ROOT" status --porcelain -uall | while IFS= read -r line; do
    # Check if the line begins with '?? ' (untracked)
    if [[ "$line" == \?\?\ * ]]; then
        # Strip leading '?? '
        file="${line#\?\? }"

        # Remove surrounding quotes if Git wrapped paths containing spaces or special characters
        file="${file#\"}"
        file="${file%\"}"

        # Skip empty entries and .gitignore itself
        if [[ -z "$file" || "$file" == ".gitignore" ]]; then
            continue
        fi

        # Append exact relative path if not already in .gitignore
        if ! grep -Fxq "$file" "$GITIGNORE"; then
            echo "$file" >> "$GITIGNORE"
            echo "Added: $file"
        fi
    fi
done
