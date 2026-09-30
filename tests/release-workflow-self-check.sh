#!/bin/bash
# Check that a release tag points to the source commit that built its packages.
# This script reads the workflow and does not create a release.
set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKFLOW="${SCRIPT_DIR}/../.github/workflows/release-packages.yml"

fail() { echo "release-workflow self-check: $*" >&2; exit 1; }

[ -f "$WORKFLOW" ] || fail "missing $WORKFLOW"

grep -q 'uses: softprops/action-gh-release@' "$WORKFLOW" \
	|| fail "the publish job no longer uses softprops/action-gh-release"

# The value must be exactly the building commit, not a branch name.
grep -qE 'target_commitish:[[:space:]]*"?\$\{\{ github\.sha \}\}"?[[:space:]]*$' \
	"$WORKFLOW" \
	|| fail "gh-release must set target_commitish to \${{ github.sha }}"

echo "release-workflow self-check passed" >&2
exit 0
