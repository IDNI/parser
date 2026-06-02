// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__ESCAPES_TMPL_H__
#define __IDNI__PARSER__UTILITY__ESCAPES_TMPL_H__

#include <cstdio>
#include "escapes.h"
#include "characters.h"
#include "charclasses.h"
#include "../parser_strings.h"

namespace idni::escapes {

inline unsigned hex_val(char c) {
	if (c >= '0' && c <= '9') return (unsigned)(c - '0');
	if (c >= 'a' && c <= 'f') return 10u + (unsigned)(c - 'a');
	if (c >= 'A' && c <= 'F') return 10u + (unsigned)(c - 'A');
	return 0;
}

static constexpr struct { char letter; char32_t cp; } control_escapes[] = {
	{ 'a', 0x07 }, { 'b', 0x08 }, { 'e', 0x1B }, { 'f', 0x0C },
	{ 'n', 0x0A }, { 'r', 0x0D }, { 't', 0x09 }, { 'v', 0x0B },
};

static constexpr char32_t to_cp(char c) { return (char32_t)(unsigned char)c; }

inline char32_t control_escape_codepoint(char c) {
	for (auto& e : control_escapes) if (e.letter == c) return e.cp;
	return (char32_t)-1;
}

inline char control_escape_char(char32_t cp) {
	for (auto& e : control_escapes) if (e.cp == cp) return e.letter;
	return 0;
}

inline void append_cp(std::string& out, char32_t cp) {
	if (cp < 0x80) { out += (char)cp; return; }
	utf8char buf[4];
	size_t n = idni::emit_codepoint(cp, buf);
	out.append(reinterpret_cast<const char*>(buf), n);
}

inline bool emit_escaped_value(std::string& out, unsigned v, const profile& p) {
	char tmp[16];
	const char32_t esc32 = to_cp(p.prefix);
	auto emit_octal = [&]() -> bool {
		append_cp(out, esc32);
		std::snprintf(tmp, sizeof tmp, "%03o", v);
		out += tmp;
		return true;
	};
	if (p.hex_x == hex_x_mode::greedy && p.octal && v <= 0xFF)
		return emit_octal();
	if (p.hex_x != hex_x_mode::off && v <= 0xFF) {
		append_cp(out, esc32);
		std::snprintf(tmp, sizeof tmp, "x%02x", v);
		out += tmp;
		return true;
	}
	if (p.hex_u4 != hex_u4_mode::off && v <= 0xFFFF) {
		append_cp(out, esc32);
		std::snprintf(tmp, sizeof tmp, "u%04x", v);
		out += tmp;
		return true;
	}
	if (p.hex_U8) {
		append_cp(out, esc32);
		std::snprintf(tmp, sizeof tmp, "U%08X", v);
		out += tmp;
		return true;
	}
	if (p.octal) return emit_octal();
	return false;
}

// Codepoint-domain output sink: collects char32_t scalars.
struct u32sink {
	std::u32string out;
	void unicode(char32_t cp) { out.push_back(cp); }
	void hexbyte(unsigned b)  { out.push_back((char32_t)b); }
	std::string_view octal(unsigned v) {
		if (!idni::is_valid_codepoint((char32_t)v))
			return parser_strings::messages::escape_octal_scalar;
		out.push_back((char32_t)v);
		return {};
	}
	size_t literal(const utf8char* s, size_t n) {
		char32_t cp = 0;
		size_t k = idni::peek_codepoint(s, n, cp);
		if (k == (size_t)-1 || k == 0) return 0;
		out.push_back(cp);
		return k;
	}
};

// Byte-domain output sink: collects raw bytes (UTF-8 for codepoint escapes).
struct utf8sink {
	std::string out;
	void unicode(char32_t cp) {
		if (cp < 0x80) { out += (char)cp; return; }
		utf8char buf[4];
		size_t n = idni::emit_codepoint(cp, buf);
		out.append(reinterpret_cast<const char*>(buf), n);
	}
	void hexbyte(unsigned b) { out.push_back((char)(unsigned char)b); }
	std::string_view octal(unsigned v) {
		if (v > 0xFF)
			return parser_strings::messages::escape_octal_overflow;
		out.push_back((char)(unsigned char)v);
		return {};
	}
	size_t literal(const utf8char* s, size_t /*n*/) {
		out.push_back((char)s[0]);
		return 1;
	}
};

// Shared escape recognizer. Parses `body` per profile `p`, emitting each
// decoded value through `sink`.
template <class Sink>
inline bool decode_core(std::string_view body, const profile& p, Sink& sink,
			diagnostics::report& rep) {
	using ps = parser_strings::messages;
	using label = parser_strings::label;
	using code = diagnostics::code;
	auto isxdigit = [](char c) { return charclasses::isxdigit<char>(c); };
	const auto* src = reinterpret_cast<const utf8char*>(body.data());
	size_t len = body.size(), i = 0;

	auto fail = [&](size_t pos, std::string_view ps) -> bool {
		rep.error(code::parse_error, ps, pos,
			  { { label::offset, static_cast<int_t>(pos) } });
		return false;
	};

	// Reads n hex digits from src[i..], accumulates into val, advances i.
	// Returns false without touching i if fewer than n hex digits are present.
	auto read_nhex = [&](char32_t& val, size_t n) -> bool {
		if (i + n > len) return false;
		for (size_t k = 0; k < n; k++)
			if (!isxdigit((char)src[i + k]))
				return false;
		val = 0;
		for (size_t k = 0; k < n; k++)
			val = (val << 4) | hex_val((char)src[i + k]);
		i += n;
		return true;
	};

	auto parse_hex_x = [&](size_t es) -> bool {
		if (i >= len || !isxdigit((char)src[i]))
			return fail(es, ps::escape_x_need);
		unsigned b = hex_val((char)src[i++]);
		if (p.hex_x == hex_x_mode::fixed) {
			if (i >= len || !isxdigit((char)src[i]))
				return fail(es, ps::escape_x_need);
			b = (b << 4) | hex_val((char)src[i++]);
		} else if (p.hex_x == hex_x_mode::lax) {
			if (i < len && isxdigit((char)src[i]))
				b = (b << 4) | hex_val((char)src[i++]);
		} else { // greedy
			while (i < len && isxdigit((char)src[i])) {
				b = (b << 4) | hex_val((char)src[i++]);
				if (b > 0xFF) return fail(es, ps::escape_x_overflow);
			}
		}
		sink.hexbyte(b);
		return true;
	};

	auto parse_hex_u4 = [&](size_t es) -> bool {
		char32_t cp = 0;
		if (!read_nhex(cp, 4)) return fail(es, ps::escape_u4_need_4);
		if (p.hex_u4 == hex_u4_mode::utf16 && cp >= 0xD800 && cp <= 0xDBFF) {
			if (i + 6 > len || src[i] != (unsigned char)p.prefix || src[i + 1] != 'u')
				return fail(es, ps::escape_u4_high_no_low);
			i += 2;
			char32_t lo = 0;
			if (!read_nhex(lo, 4)) return fail(es, ps::escape_u4_lo_need_4);
			if (lo < 0xDC00 || lo > 0xDFFF) return fail(es, ps::escape_u4_bad_low);
			sink.unicode(0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00));
			return true;
		}
		if (p.hex_u4 == hex_u4_mode::utf16 && cp >= 0xDC00 && cp <= 0xDFFF)
			return fail(es, ps::escape_u4_lone_low);
		if (!idni::is_valid_codepoint(cp)) return fail(es, ps::escape_u4_invalid);
		sink.unicode(cp);
		return true;
	};

	auto parse_hex_U8 = [&](size_t es) -> bool {
		char32_t cp = 0;
		if (!read_nhex(cp, 8)) return fail(es, ps::escape_U8_need_8);
		if (!idni::is_valid_codepoint(cp)) return fail(es, ps::escape_U8_invalid);
		sink.unicode(cp);
		return true;
	};

	auto parse_octal = [&](size_t es, char first) -> bool {
		unsigned v = (unsigned)(first - '0');
		int nd = 1;
		while (nd < 3 && i < len && src[i] >= '0' && src[i] <= '7') {
			v = v * 8 + (unsigned)(src[i++] - '0');
			nd++;
		}
		if (auto e = sink.octal(v); !e.empty()) return fail(es, e);
		return true;
	};

	while (i < len) {
		unsigned char ch = src[i];

		// doubled-delimiter mode (e.g. CSV "")
		if (p.prefix != '\0' && p.prefix == p.delim && ch == (unsigned char)p.delim) {
			if (i + 1 < len && src[i + 1] == (unsigned char)p.delim) {
				sink.unicode(to_cp(p.delim));
				i += 2;
				continue;
			}
			return fail(i, ps::escape_lone_delimiter);
		}

		// prefix escape
		if (p.prefix != '\0' && p.prefix != p.delim && ch == (unsigned char)p.prefix) {
			size_t es = i++;
			if (i >= len) return fail(es, ps::escape_trailing);
			char c = (char)src[i++];
			char32_t cc = to_cp(c);

			if (c == p.prefix || c == p.delim)                           { sink.unicode(cc); continue; }
			if (p.escape_chars.find(c) != std::string_view::npos) { char32_t m = control_escape_codepoint(c);
										sink.unicode(m == (char32_t)-1 ? cc : m);
										continue; }
			if (p.hex_x  != hex_x_mode::off   && (c == 'x' || c == 'X')) { if (!parse_hex_x(es))    return false; continue; }
			if (p.hex_u4 != hex_u4_mode::off  &&  c == 'u')              { if (!parse_hex_u4(es))   return false; continue; }
			if (p.hex_U8                      &&  c == 'U')              { if (!parse_hex_U8(es))   return false; continue; }
			if (p.octal                       &&  c >= '0' && c <= '7')  { if (!parse_octal(es, c)) return false; continue; }
			if (p.lenient == lenient_mode::drop)                         { sink.unicode(cc); continue; }
			if (p.lenient == lenient_mode::keep)                         { sink.unicode(to_cp(p.prefix)); sink.unicode(cc); continue; }
			return fail(es, std::string("Unknown escape: '") + p.prefix + c + "'");
		}

		// normal text
		size_t n = sink.literal(src + i, len - i);
		if (n == 0) return fail(i, ps::escape_invalid_utf8);
		i += n;
	}
	return true;
}

inline u32decoded u32decode(std::string_view body, const profile& p) {
	u32sink sink;
	u32decoded r("escapes::u32decode");
	if (decode_core(body, p, sink, r.report())) r.emplace(std::move(sink.out));
	return r;
}

inline decoded decode(std::string_view body, const profile& p) {
	utf8sink     sink;
	decoded r("escapes::decode");
	if (decode_core(body, p, sink, r.report())) r.emplace(std::move(sink.out));
	return r;
}

template <class C>
inline std::string encode_(const std::basic_string<C>& units,
	const profile& p) {
	std::string out;
	out.reserve(units.size() * 2);
	const bool     has_prefix = p.prefix != '\0' && p.prefix != p.delim;
	const bool     doubling   = p.prefix != '\0' && p.prefix == p.delim;
	const char32_t esc32      = to_cp(p.prefix);
	const char32_t delim32    = to_cp(p.delim);
	constexpr bool is_byte    = sizeof(C) == 1;

	auto append_unit = [&](char32_t v) {
		if (is_byte) out += (char)(unsigned char)v;
		else         append_cp(out, v);
	};

	for (C u : units) {
		char32_t cp = (char32_t)(std::make_unsigned_t<C>)u;
		if (cp == delim32) {
			if (doubling)        { append_unit(cp); append_unit(cp); }
			else if (has_prefix) { append_unit(esc32); append_unit(cp); }
			else                   append_unit(cp);
			continue;
		}
		if (has_prefix && cp == esc32) {
			append_unit(esc32); append_unit(esc32); continue;
		}
		if (has_prefix) {
			char letter = control_escape_char(cp);
			if (letter != 0 &&
				p.escape_chars.find(letter) != std::string_view::npos) {
				append_unit(esc32);
				out += letter;
				continue;
			}
		}
		if (cp < 0x20 || cp == 0x7F) {
			if (!has_prefix || !emit_escaped_value(out,
				(unsigned)cp, p))
				append_unit(cp);
			continue;
		}
		append_unit(cp);
	}
	return out;
}

inline std::string encode(std::u32string_view cps, const profile& p) {
	return encode_<char32_t>(std::u32string(cps), p);
}

inline decoded utf8encode(std::string_view utf8, const profile& p) {
	using ps = parser_strings::messages;
	using label = parser_strings::label;
	const auto* src = reinterpret_cast<const idni::utf8char*>(utf8.data());
	size_t len = utf8.size();
	decoded r("escapes::utf8encode");
	auto& rep = r.report();
	std::string out;
	out.reserve(len * 2);
	size_t i = 0;
	while (i < len) {
		char32_t cp = 0;
		size_t n = idni::peek_codepoint(src + i, len - i, cp);
		if (n == (size_t)-1 || n == 0) {
			rep.warning(ps::escape_invalid_utf8, i,
				{ { label::offset, static_cast<int_t>(i) } });
			out += (char)src[i];
			i++;
		} else {
			out += encode(std::u32string_view(&cp, 1), p);
			i += n;
		}
	}
	r.emplace(std::move(out));
	return r;
}

inline std::string encode(std::string_view bytes, const profile& p) {
	return encode_<char>(std::string(bytes), p);
}

} // namespace idni::escapes

#endif // __IDNI__PARSER__UTILITY__ESCAPES_TMPL_H__
