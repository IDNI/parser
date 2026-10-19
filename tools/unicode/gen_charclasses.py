#!/usr/bin/env python3
# To view the license please visit
# https://github.com/IDNI/parser/blob/main/LICENSE.md

"""Generate src/utility/charclasses_unicode.generated.h and
src/utility/unicode_properties.generated.h from the Unicode Character
Database (UCD).

The first header holds a two-stage lookup table that maps each code point to
a bitmask of the 12 <cctype> classes. The second holds a two-stage table of
the digit value properties. Do not edit the headers by hand: change this
script and run it again.
"""

import argparse
import os
import re
import sys
import urllib.request
from pathlib import Path

UNICODE_VERSION = "17.0.0"
UCD_URL = "https://www.unicode.org/Public/{version}/ucd/{name}"
UCD_FILES = ("UnicodeData.txt", "DerivedCoreProperties.txt", "PropList.txt",
	"ReadMe.txt")

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_OUTPUT = ROOT / "src" / "utility" / "charclasses_unicode.generated.h"
DEFAULT_PROPERTIES_OUTPUT = (ROOT / "src" / "utility"
	/ "unicode_properties.generated.h")

MAX_CP = 0x110000
BLOCK_SIZE = 256

UC_ALPHA = 1 << 0
UC_UPPER = 1 << 1
UC_LOWER = 1 << 2
UC_DIGIT = 1 << 3
UC_XDIGIT = 1 << 4
UC_ALNUM = 1 << 5
UC_SPACE = 1 << 6
UC_BLANK = 1 << 7
UC_CNTRL = 1 << 8
UC_PUNCT = 1 << 9
UC_GRAPH = 1 << 10
UC_PRINT = 1 << 11

# ASCII and fullwidth A-F and a-f
XDIGIT_EXTRA = frozenset(
	list(range(0x41, 0x47)) + list(range(0x61, 0x67)) +
	list(range(0xFF21, 0xFF27)) + list(range(0xFF41, 0xFF47)))

NO_GLYPH_CATEGORIES = frozenset(("Cn", "Cs", "Cf", "Zl", "Zp"))


def default_cache_dir():
	base = os.environ.get("XDG_CACHE_HOME") or Path.home() / ".cache"
	return Path(base) / "idni-ucd"


def download_ucd(version, cache_dir):
	target = Path(cache_dir) / version
	target.mkdir(parents=True, exist_ok=True)
	for name in UCD_FILES:
		path = target / name
		if path.is_file():
			continue
		url = UCD_URL.format(version=version, name=name)
		print(f"downloading {url}", file=sys.stderr)
		# A partial download must not look like a cached file.
		tmp = path.with_suffix(".part")
		with urllib.request.urlopen(url) as response, open(tmp, "wb") as out:
			out.write(response.read())
		tmp.replace(path)
	return target


def data_lines(path):
	with open(path, encoding="utf-8") as f:
		for line in f:
			line = line.split("#", 1)[0].strip()
			if line:
				yield line


def parse_unicode_data(path):
	"""Return a dict that maps a code point to its General Category."""
	gc = {}
	first = None
	for line in data_lines(path):
		fields = line.split(";")
		cp, name, cat = int(fields[0], 16), fields[1], fields[2]
		if name.endswith(", First>"):
			first = cp
			continue
		if name.endswith(", Last>"):
			for c in range(first, cp + 1):
				gc[c] = cat
			first = None
			continue
		gc[cp] = cat
	return gc


def parse_unicode_fields(path):
	"""Return a dict that maps a code point to its UnicodeData.txt fields.
	A First/Last range gives each code point of the range its fields."""
	fields_of = {}
	first = None
	for line in data_lines(path):
		fields = line.split(";")
		cp, name = int(fields[0], 16), fields[1]
		if name.endswith(", First>"):
			first = cp
			continue
		if name.endswith(", Last>"):
			for c in range(first, cp + 1):
				fields_of[c] = fields
			first = None
			continue
		fields_of[cp] = fields
	return fields_of


def parse_range_file(path):
	"""Return a dict that maps a property name to a set of code points."""
	props = {}
	for line in data_lines(path):
		fields = [f.strip() for f in line.split(";")]
		# Lines with a value field (for example InCB) are not binary properties.
		if len(fields) != 2:
			continue
		cps, name = fields
		lo, _, hi = cps.partition("..")
		lo = int(lo, 16)
		hi = int(hi, 16) if hi else lo
		props.setdefault(name, set()).update(range(lo, hi + 1))
	return props


VERSIONED_FILES = ("PropList.txt", "DerivedCoreProperties.txt")
# UnicodeData.txt has no version line, so ReadMe.txt of the same folder
# gives its version.
README_VERSION = re.compile(r"for Version (\S+) of the Unicode Standard")


def check_ucd_version(ucd_dir, version):
	"""Return an error message when a file is missing or its version
	differs, else None."""
	for name in UCD_FILES:
		path = Path(ucd_dir) / name
		if not path.is_file():
			return f"{path}: the file is missing"
	readme = Path(ucd_dir) / "ReadMe.txt"
	match = README_VERSION.search(readme.read_text(encoding="utf-8"))
	if not match:
		return f"{readme}: no line 'for Version <version> of the Unicode Standard'"
	if match.group(1) != version:
		return (f"{readme}: the version is '{match.group(1)}', "
			f"expected '{version}' for --unicode-version {version}")
	for name in VERSIONED_FILES:
		path = Path(ucd_dir) / name
		with open(path, encoding="utf-8") as f:
			first = f.readline().strip()
		stem = name[:-len(".txt")]
		expected = f"# {stem}-{version}.txt"
		if first != expected:
			return (f"{path}: the first line is '{first}', "
				f"expected '{expected}' for --unicode-version {version}")
	return None


def read_ucd(ucd_dir):
	ucd_dir = Path(ucd_dir)
	gc = parse_unicode_data(ucd_dir / "UnicodeData.txt")
	derived = parse_range_file(ucd_dir / "DerivedCoreProperties.txt")
	proplist = parse_range_file(ucd_dir / "PropList.txt")
	return gc, derived, proplist


# The bits of a word of the properties table. The low four bits hold the
# digit value.
UP_DIGIT_VALUE = 0xF
UP_DIGIT = 1 << 4
UP_DECIMAL = 1 << 5


def compute_unicode_properties(fields_of):
	"""Return the property word of each code point."""
	words = [0] * MAX_CP
	for cp in range(MAX_CP):
		fields = fields_of.get(cp)
		decimal = fields[6] if fields else ""
		digit = fields[7] if fields else ""
		word = 0
		# Numeric_Type=Decimal has field 6, Numeric_Type=Digit field 7 only.
		if digit:
			word |= UP_DIGIT | int(digit)
		if decimal:
			word |= UP_DECIMAL
		words[cp] = word
	return words


def is_punct(cp, cat):
	# <cctype> ispunct also takes the ASCII symbols $+<=>^`|~ (category S*).
	return cat[0] == "P" or (cp < 0x80 and cat[0] == "S")


def compute_props(gc, derived, proplist):
	alpha = derived["Alphabetic"]
	upper = derived["Uppercase"]
	lower = derived["Lowercase"]
	space = proplist["White_Space"]
	props = [0] * MAX_CP
	for cp in range(MAX_CP):
		cat = gc.get(cp, "Cn")
		mask = 0
		if cp in alpha:
			mask |= UC_ALPHA
		if cp in upper:
			mask |= UC_UPPER
		if cp in lower:
			mask |= UC_LOWER
		if cat == "Nd":
			mask |= UC_DIGIT
		if mask & UC_DIGIT or cp in XDIGIT_EXTRA:
			mask |= UC_XDIGIT
		if mask & (UC_ALPHA | UC_DIGIT):
			mask |= UC_ALNUM
		if cp in space:
			mask |= UC_SPACE
		if cp == 0x0009 or cat == "Zs":
			mask |= UC_BLANK
		if cat == "Cc":
			mask |= UC_CNTRL
		if is_punct(cp, cat):
			mask |= UC_PUNCT
		if (not mask & (UC_CNTRL | UC_SPACE) and cp != 0
				and cat not in NO_GLYPH_CATEGORIES):
			mask |= UC_GRAPH
		# Zs and not isblank, so TAB stays unprintable as in <cctype>.
		if mask & UC_GRAPH or cat == "Zs":
			mask |= UC_PRINT
		props[cp] = mask
	return props


def build_two_stage(props):
	"""Deduplicate blocks in the order of their first use."""
	blocks = {}
	stage1 = []
	stage2 = []
	for start in range(0, len(props), BLOCK_SIZE):
		block = tuple(props[start:start + BLOCK_SIZE])
		if block not in blocks:
			blocks[block] = len(blocks)
			stage2.extend(block)
		stage1.append(blocks[block])
	return stage1, stage2, len(blocks)


def emit_array(out, values):
	rows = []
	for i in range(0, len(values), 8):
		rows.append("    " + ", ".join(
			f"0x{v:04X}" for v in values[i:i + 8]))
	out.append(",\n".join(rows))


HEADER_TOP = """\
// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// ============================================================================
// Unicode character class lookup tables for char32_t.
// Unicode version: {version}
// ============================================================================

#ifndef __IDNI__PARSER__UTILITY__CHARCLASSES_UNICODE_GENERATED_H__
#define __IDNI__PARSER__UTILITY__CHARCLASSES_UNICODE_GENERATED_H__

#include <cstdint>

namespace idni::charclasses {{

// Property bitmask bits — one bit per <cctype> class
enum unicode_prop : uint16_t {{
    UC_ALPHA  = 1 << 0,   // isalpha  (DerivedCoreProperties: Alphabetic)
    UC_UPPER  = 1 << 1,   // isupper  (DerivedCoreProperties: Uppercase)
    UC_LOWER  = 1 << 2,   // islower  (DerivedCoreProperties: Lowercase)
    UC_DIGIT  = 1 << 3,   // isdigit  (General_Category = Nd)
    UC_XDIGIT = 1 << 4,   // isxdigit (Nd | hex letters A-F/a-f, plus fullwidth)
    UC_ALNUM  = 1 << 5,   // isalnum  (UC_ALPHA | UC_DIGIT)
    UC_SPACE  = 1 << 6,   // isspace  (PropList: White_Space)
    UC_BLANK  = 1 << 7,   // isblank  (U+0009 | General_Category = Zs)
    UC_CNTRL  = 1 << 8,   // iscntrl  (General_Category = Cc)
    UC_PUNCT  = 1 << 9,   // ispunct  (General_Category P*, plus ASCII S* as in <cctype>)
    UC_GRAPH  = 1 << 10,  // isgraph  (visible glyph, not control/space/unassigned)
    UC_PRINT  = 1 << 11,  // isprint  (isgraph | General_Category = Zs)
}};

// Stage 1: maps high 13 bits (cp >> 8) to a block index.
// {stage1_len} entries, {blocks} unique blocks.
inline constexpr uint16_t stage1[{stage1_len}] = {{
"""


def render_header(version, stage1, stage2, blocks):
	out = [HEADER_TOP.format(version=version, stage1_len=len(stage1),
		blocks=blocks)]
	emit_array(out, stage1)
	out.append(f"\n}};\n\ninline constexpr uint16_t stage2[{len(stage2)}] = {{\n")
	emit_array(out, stage2)
	out.append("\n};\n\n} // namespace idni::charclasses\n\n"
		"#endif //__IDNI__PARSER__UTILITY__CHARCLASSES_UNICODE_GENERATED_H__\n")
	return "".join(out)


PROPERTIES_TOP = """\
// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// ============================================================================
// Unicode property lookup tables for char32_t.
// Unicode version: {version}
// ============================================================================

#ifndef __IDNI__PARSER__UTILITY__UNICODE_PROPERTIES_GENERATED_H__
#define __IDNI__PARSER__UTILITY__UNICODE_PROPERTIES_GENERATED_H__

#include <cstdint>

namespace idni::charclasses::unicode::tables {{

// The bits of a word of props_stage2. The low four bits hold the digit value.
enum property_bit : uint16_t {{
    UP_DIGIT_VALUE = 0xF,     // the value of a UP_DIGIT code point
    UP_DIGIT       = 1 << 4,  // Numeric_Type = Decimal or Digit
    UP_DECIMAL     = 1 << 5,  // Numeric_Type = Decimal
}};

// Stage 1: maps the high bits (cp >> 8) to a block index.
// {stage1_len} entries, {blocks} unique blocks.
inline constexpr uint16_t props_stage1[{stage1_len}] = {{
"""


def render_properties_header(version, stage1, stage2, blocks):
	out = [PROPERTIES_TOP.format(version=version, stage1_len=len(stage1),
		blocks=blocks)]
	emit_array(out, stage1)
	out.append(f"\n}};\n\ninline constexpr uint16_t props_stage2[{len(stage2)}] = {{\n")
	emit_array(out, stage2)
	out.append("\n};\n\n} // namespace idni::charclasses::unicode::tables\n\n"
		"#endif //__IDNI__PARSER__UTILITY__UNICODE_PROPERTIES_GENERATED_H__\n")
	return "".join(out)


# (code point, the digit value or None, the bits that must be set, the bits
# that must be clear)
PROPERTY_CHECKS = (
	(0x0031, 1, UP_DIGIT | UP_DECIMAL, 0),
	(0xFF11, 1, UP_DIGIT | UP_DECIMAL, 0),
	(0x0661, 1, UP_DIGIT | UP_DECIMAL, 0),
	(0x00B2, 2, UP_DIGIT, UP_DECIMAL),
	(0xD800, None, 0, 0xFF),
	(0x10FFFF, None, 0, 0xFF),
)


# (code point, bits that must be set, bits that must be clear)
SPOT_CHECKS = (
	(0x00C0, UC_ALPHA | UC_UPPER, UC_LOWER),
	(0x0660, UC_DIGIT | UC_XDIGIT | UC_ALNUM, UC_ALPHA),
	(0x3000, UC_SPACE | UC_BLANK | UC_PRINT, UC_GRAPH),
	(0x4E00, UC_ALPHA | UC_GRAPH | UC_PRINT, UC_DIGIT),
	(0x1F600, UC_GRAPH | UC_PRINT, UC_ALPHA),
	(0x200B, 0, UC_SPACE | UC_GRAPH | UC_PRINT),
	(0x00A0, UC_SPACE | UC_BLANK, UC_ALPHA | UC_GRAPH),
	(0x00AD, 0, UC_ALPHA | UC_GRAPH),
	(0x00A9, UC_GRAPH | UC_PRINT, UC_ALPHA | UC_PUNCT),
	(0xFF21, UC_ALPHA | UC_XDIGIT, UC_DIGIT),
	(0xD800, 0, 0xFFFF),
	(0x10FFFF, 0, 0xFFFF),
)

# Python's str methods differ from <cctype> on ASCII in a few places, so
# the C locale classes are written out here.
ASCII_CLASSES = (
	(UC_ALPHA, lambda c: chr(c).isascii() and chr(c).isalpha()),
	(UC_UPPER, lambda c: 0x41 <= c <= 0x5A),
	(UC_LOWER, lambda c: 0x61 <= c <= 0x7A),
	(UC_DIGIT, lambda c: 0x30 <= c <= 0x39),
	(UC_XDIGIT, lambda c: chr(c) in "0123456789abcdefABCDEF"),
	(UC_ALNUM, lambda c: chr(c).isascii() and chr(c).isalnum()),
	(UC_SPACE, lambda c: chr(c) in " \t\n\v\f\r"),
	(UC_BLANK, lambda c: chr(c) in " \t"),
	(UC_CNTRL, lambda c: c < 0x20 or c == 0x7F),
	(UC_PUNCT, lambda c: 0x21 <= c <= 0x7E and not chr(c).isalnum()),
	(UC_GRAPH, lambda c: 0x21 <= c <= 0x7E),
	(UC_PRINT, lambda c: 0x20 <= c <= 0x7E),
)


def selftest(props, stage1, stage2, gc, derived, proplist):
	errors = []

	def lookup(cp):
		return stage2[stage1[cp >> 8] * BLOCK_SIZE + (cp & (BLOCK_SIZE - 1))]

	for cp in range(MAX_CP):
		if lookup(cp) != props[cp]:
			errors.append(f"U+{cp:04X}: table 0x{lookup(cp):04X}, "
				f"computed 0x{props[cp]:04X}")
	for cp in range(0x80):
		for bit, expected in ASCII_CLASSES:
			if bool(props[cp] & bit) != expected(cp):
				errors.append(f"U+{cp:04X}: bit 0x{bit:04X} differs from <cctype>")
	for cp, must_set, must_clear in SPOT_CHECKS:
		mask = props[cp]
		if mask & must_set != must_set or mask & must_clear:
			errors.append(f"U+{cp:04X}: 0x{mask:04X} fails the spot check")
	# Each bit against its own data source.
	sources = (
		(UC_ALPHA, derived["Alphabetic"]),
		(UC_UPPER, derived["Uppercase"]),
		(UC_LOWER, derived["Lowercase"]),
		(UC_SPACE, proplist["White_Space"]),
		(UC_DIGIT, {c for c, v in gc.items() if v == "Nd"}),
		(UC_CNTRL, {c for c, v in gc.items() if v == "Cc"}),
		(UC_PUNCT, {c for c, v in gc.items() if is_punct(c, v)}),
	)
	for bit, cps in sources:
		table = {cp for cp in range(MAX_CP) if props[cp] & bit}
		if table != cps:
			errors.append(f"bit 0x{bit:04X}: {len(table ^ cps)} code points "
				"differ from the data files")
	report_errors(errors)
	return not errors


def properties_selftest(words, stage1, stage2, fields_of):
	errors = []
	for cp in range(MAX_CP):
		got = stage2[stage1[cp >> 8] * BLOCK_SIZE + (cp & (BLOCK_SIZE - 1))]
		if got != words[cp]:
			errors.append(f"U+{cp:04X}: table 0x{got:04X}, "
				f"computed 0x{words[cp]:04X}")
	for cp, value, must_set, must_clear in PROPERTY_CHECKS:
		word = words[cp]
		if word & must_set != must_set or word & must_clear or (value
				is not None and word & UP_DIGIT_VALUE != value):
			errors.append(f"U+{cp:04X}: 0x{word:04X} fails the spot check")
	for cp, f in fields_of.items():
		if bool(words[cp] & UP_DECIMAL) != bool(f[6]) or (f[6]
				and f[6] != f[7]):
			errors.append(f"U+{cp:04X}: the decimal value differs")
	report_errors(errors)
	return not errors


def report_errors(errors):
	for msg in errors[:50]:
		print(f"selftest: {msg}", file=sys.stderr)
	print(f"selftest: {'FAILED, ' + str(len(errors)) + ' errors' if errors else 'passed'}",
		file=sys.stderr)


def main():
	parser = argparse.ArgumentParser(description=__doc__,
		formatter_class=argparse.RawDescriptionHelpFormatter)
	parser.add_argument("--unicode-version", default=UNICODE_VERSION,
		help=f"Unicode version to download (default {UNICODE_VERSION})")
	parser.add_argument("--ucd-dir",
		help="read the UCD files from this folder, do not download")
	parser.add_argument("--cache-dir", default=default_cache_dir(),
		help="folder for downloaded UCD files (default %(default)s)")
	parser.add_argument("--output", default=DEFAULT_OUTPUT,
		help="header to write (default %(default)s)")
	parser.add_argument("--properties-output",
		default=DEFAULT_PROPERTIES_OUTPUT,
		help="header of the properties to write (default %(default)s)")
	parser.add_argument("--selftest", action="store_true",
		help="check the tables against the UCD files, do not write the header")
	args = parser.parse_args()

	ucd_dir = args.ucd_dir or download_ucd(args.unicode_version, args.cache_dir)
	error = check_ucd_version(ucd_dir, args.unicode_version)
	if error:
		print(f"error: {error}", file=sys.stderr)
		return 1
	gc, derived, proplist = read_ucd(ucd_dir)
	props = compute_props(gc, derived, proplist)
	stage1, stage2, blocks = build_two_stage(props)
	fields_of = parse_unicode_fields(Path(ucd_dir) / "UnicodeData.txt")
	words = compute_unicode_properties(fields_of)
	p_stage1, p_stage2, p_blocks = build_two_stage(words)

	if args.selftest:
		ok = selftest(props, stage1, stage2, gc, derived, proplist)
		ok = properties_selftest(words, p_stage1, p_stage2, fields_of) and ok
		return 0 if ok else 1

	text = render_header(args.unicode_version, stage1, stage2, blocks)
	with open(args.output, "w", encoding="utf-8", newline="\n") as f:
		f.write(text)
	print(f"wrote {args.output}: {blocks} unique blocks", file=sys.stderr)
	text = render_properties_header(args.unicode_version, p_stage1, p_stage2,
		p_blocks)
	with open(args.properties_output, "w", encoding="utf-8",
			newline="\n") as f:
		f.write(text)
	print(f"wrote {args.properties_output}: {p_blocks} unique blocks",
		file=sys.stderr)
	return 0


if __name__ == "__main__":
	sys.exit(main())
