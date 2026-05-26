#!/bin/bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${ROOT}/build/release}"
TGF_BIN="${BUILD_DIR}/tgf"
CASES="${ROOT}/tests/fixtures/tau_parse_cases.txt"

resolve_tgf() {
	if [[ -n "${TAU_TGF:-}" && -f "${TAU_TGF}" ]]; then
		echo "${TAU_TGF}"
		return
	fi
	local candidate
	for candidate in \
		"${ROOT}/tests/fixtures/tau.tgf" \
		"${ROOT}/../tau-lang/parser/tau.tgf"; do
		if [[ -f "${candidate}" ]]; then
			echo "${candidate}"
			return
		fi
	done
	echo "specialize-test: tau.tgf not found" >&2
	echo "  set TAU_TGF or add tests/fixtures/tau.tgf" >&2
	exit 1
}

TGF="$(resolve_tgf)"
BASENAME="$(basename "${TGF}" .tgf)"
SPEC_BIN="${BUILD_DIR}/${BASENAME}_grammar"

"${ROOT}/dev" specialize "${TGF}"

test -x "${SPEC_BIN}"
test -x "${TGF_BIN}"
test -f "${CASES}"

# Strip ANSI, timing scopes, measure counters, and terminal literals
# (char vs char32_t display may differ between runtime and generated parsers).
normalize() {
	sed -r 's/\x1b\[[0-9;]*[a-zA-Z]//g' \
		| grep -v -E '(^parse:|µs| ms$|Mb|rss |inner loop|complete calls|predict |scan cc|position count|preprocess|build bintree|reconstruct forest|shaped tree|trim|inline |max per pos|size peak|input length|scan calls|conjunction attempts|fromS writes)' \
		| sed -E "s/'[^']*'/'T'/" \
		| sed '/^$/d'
}

run_spec_parse() {
	"${SPEC_BIN}" parse -c false -e "$1" 2>/dev/null | normalize
}

run_tgf_parse() {
	"${TGF_BIN}" "${TGF}" parse -c false -e "$1" 2>/dev/null | normalize
}

compare_case() {
	local input="$1"
	local out_spec out_tgf

	out_spec="$(run_spec_parse "${input}")"
	out_tgf="$(run_tgf_parse "${input}")"

	if [[ "${out_spec}" != "${out_tgf}" ]]; then
		echo "specialize-test: parse output mismatch"
		echo "  grammar: ${TGF}"
		echo "  input:   ${input}"
		echo "--- ${BASENAME}_grammar ---"
		echo "${out_spec}"
		echo "--- tgf ${BASENAME}.tgf ---"
		echo "${out_tgf}"
		diff -u <(echo "${out_tgf}") <(echo "${out_spec}") || true
		exit 1
	fi
}

cases_run=0
while IFS= read -r line || [[ -n "${line}" ]]; do
	line="${line%%#*}"
	line="$(echo "${line}" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')"
	[[ -z "${line}" ]] && continue
	compare_case "${line}"
	cases_run=$((cases_run + 1))
done < "${CASES}"

if [[ ${cases_run} -eq 0 ]]; then
	echo "specialize-test: no parse cases in ${CASES}" >&2
	exit 1
fi

echo "specialize-test: OK (${cases_run} cases, ${BASENAME}_grammar vs tgf ${BASENAME}.tgf)"
