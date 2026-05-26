#!/bin/bash

DEV_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
source "${DEV_ROOT}/scripts/devrc"

PRESET_RUN_BIN="tgf"
preset_entry "$@"
