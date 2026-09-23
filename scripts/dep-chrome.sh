#!/bin/env bash

# Exit immediately on a failed command: a failed download or a failed install.
set -euo pipefail

# This script lives in the parser's own scripts/, so devrc is its sibling.
DEV_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${DEV_ROOT}/scripts/devrc"

dep_entry "$@"

# Chrome for Testing build the wasm browser test harness is pinned to; bump
# together with a manual check that the suite still passes in it.
CHROME_TAG="$(dep_var CHROME_TAG 150.0.7871.24)"
# @puppeteer/browsers itself, pinned so this script's own behaviour doesn't
# drift with whatever "latest" resolves to on the day it happens to run.
BROWSERS_CLI_TAG="$(dep_var BROWSERS_CLI_TAG 3.2.0)"

CHROME_PREFIX="$(dep_shared_prefix)/chrome"
CHROME_BIN="${CHROME_PREFIX}/chrome/linux-${CHROME_TAG}/chrome-linux64/chrome"

echo "CHROME_BIN: ${CHROME_BIN}"

dep_done_if_exists "$CHROME_BIN" "chrome installing"

mkdir -p "${CHROME_PREFIX}"

echo "Installing Chrome for Testing ${CHROME_TAG} into ${CHROME_PREFIX}"
npx --yes "@puppeteer/browsers@${BROWSERS_CLI_TAG}" \
	install "chrome@${CHROME_TAG}" --path "${CHROME_PREFIX}"
