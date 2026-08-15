#!/bin/bash

DEV_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
source "${DEV_ROOT}/scripts/devrc"

dev_reject_build_type debug Debug "$@"
./dev build Debug "$@"
