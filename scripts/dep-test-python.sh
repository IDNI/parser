#!/bin/bash
# Make the Python 3.12 venv of the parser tests from tests/requirements.txt.
#
#   ./dev dep-test-python
#
# The venv lands in <TAU_SHARED_PREFIX>/parser-test-python/<hash>, where the
# hash is the sha256 of tests/requirements.txt, so a changed pin makes a new
# venv and every worktree with the same pins shares one. uv makes the venv.
# The last line of the output is "test python: <interpreter>".
set -u

DEV_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${DEV_ROOT}/scripts/devrc"

requirements="${DEV_ROOT}/tests/requirements.txt"
modules="import pexpect, pyte, jsonschema, referencing"

case "$(uname -s)" in
	MINGW*|MSYS*|CYGWIN*) bin_dir=Scripts; python_name=python.exe ;;
	*) bin_dir=bin; python_name=python ;;
esac

prefix="$(dep_shared_prefix)" || exit 1
hash="$(dep_sha256 "$requirements")" || exit 1
hash="${hash:0:16}"
venv="${prefix}/parser-test-python/${hash}"
python="${venv}/${bin_dir}/${python_name}"

if "$python" -c "$modules" > /dev/null 2>&1; then
	echo "test python: ${python}"
	exit 0
fi

if ! command -v uv > /dev/null 2>&1; then
	echo "dep-test-python: uv is not on PATH; install uv to make the test venv" >&2
	exit 2
fi

# A partial venv never takes the final name, so a failed install is retried.
# --relocatable keeps the venv valid after the move to its final name.
staging="${venv}.staging-$$"
rm -rf "$staging"
mkdir -p "$(dirname "$venv")" || exit 1
echo "dep-test-python: making ${venv} from ${requirements}" >&2
if ! uv venv --quiet --relocatable --python 3.12 "$staging" \
		|| ! uv pip install --quiet --python "${staging}/${bin_dir}/${python_name}" \
			-r "$requirements"; then
	rm -rf "$staging"
	echo "dep-test-python: cannot make the test venv" >&2
	exit 1
fi
# A configure that ran at the same time can have placed the venv first.
if "$python" -c "$modules" > /dev/null 2>&1; then
	rm -rf "$staging"
else
	rm -rf "$venv"
	mv "$staging" "$venv" || { rm -rf "$staging"; exit 1; }
fi
if ! "$python" -c "$modules"; then
	echo "dep-test-python: ${python} cannot import the test modules" >&2
	exit 1
fi
echo "test python: ${python}"
