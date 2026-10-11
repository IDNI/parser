// This file is generated from a file src/format/json/json.tgf by
//       https://github.com/IDNI/parser/src/tgf
//
#ifndef __JSON_PARSER_H__
#define __JSON_PARSER_H__

#include "parser.h"
#include "recoders.h"

namespace json_parser_data {

using char_type     = char;
using terminal_type = char32_t;

inline static constexpr size_t nt_bits = 6;
inline const std::vector<std::string> symbol_names{
	"", "digit", "any", "ascii", "xdigit", "cntrl", "start", "_", "value", "__", 
	"__E____0", "__E___1", "true_sym", "false_sym", "null_sym", "number", "q_str", "arr", "object", "__E_number_2", 
	"integral", "fractional", "__E_number_3", "exponent", "__E_number_4", "zero", "nonzerodigit", "nzintegral", "ascii_digit", "__E_nzintegral_5", 
	"__E_fractional_6", "__E_exponent_7", "__E_exponent_8", "__E_exponent_9", "str", "__E_q_str_10", "unescaped", "escaped", "__E_q_str_11", "__E_unescaped_12", 
	"esc", "__E_escaped_13", "uni_cp", "ascii_xdigit", "__E_arr_14", "values", "__E_values_15", "__E_values_16", "__E_object_17", "object_pair", 
	"__E___E_object_17_18", "__E___E_object_17_19", "__N_0", "__N_1", "__N_2", 
};

inline ::idni::nonterminals<char_type, terminal_type> nts{symbol_names};

inline std::vector<terminal_type> terminals{
	U'\0', U' ', U'\t', U'\r', U'\n', U't', U'r', U'u', U'e', 
	U'f', U'a', U'l', U's', U'n', U'-', U'0', U'.', U'E', U'+', 
	U'"', U'\\', U'', U'/', U'b', U'[', U']', U',', U'{', U'}', 
	U':', 
};

inline ::idni::char_class_fns<terminal_type> char_classes =
	::idni::predefined_char_classes<char_type, terminal_type>({
		"digit",
		"any",
		"ascii",
		"xdigit",
		"cntrl",
	}, nts);

inline struct ::idni::grammar<char_type, terminal_type>::options
	grammar_options
{
	.transform_negation = false,
	.auto_disambiguate = true,
	.shaping = {
		.to_trim = {
			7, 9
		},
		.to_trim_children = {
			12, 13, 14
		},
		.trim_terminals = false,
		.to_inline = {
			{ 16, 34 },
			{ 20 },
			{ 21 },
			{ 25 },
			{ 26 },
			{ 27 },
			{ 28 },
			{ 36 },
			{ 40 },
			{ 42 },
			{ 43 }
		},
		.inline_char_classes = true
	}
};

inline auto make_parser_options() {
	auto o = ::idni::default_parser_options<char_type, terminal_type>();
	o.codec.decode = idni::utf8_to_u32_conv;
	o.codec.encode = idni::u32_to_utf8_conv;
	return o;
}

inline ::idni::prods<char_type, terminal_type> start_symbol{ nts(6) };

#ifdef TAU_PARSER_BUILD_HEADER_ONLY
inline idni::prods<char_type, terminal_type>& productions() {
	static bool loaded = false;
	static idni::prods<char_type, terminal_type>
		p, nul(idni::lit<char_type, terminal_type>{});
	if (loaded) return p;
	#define  T(x) (idni::prods<char_type, terminal_type>{ terminals[x] })
	#define NT(x) (idni::prods<char_type, terminal_type>{ nts(x) })
//G0:   start(6)             => _(7) value(8) _(7).
	p(NT(6), (NT(7)+NT(8)+NT(7)));
//G1:   __E____0(10)         => ' '.
	p(NT(10), (T(1)));
//G2:   __E____0(10)         => '\t'.
	p(NT(10), (T(2)));
//G3:   __E____0(10)         => '\r'.
	p(NT(10), (T(3)));
//G4:   __E____0(10)         => '\n'.
	p(NT(10), (T(4)));
//G5:   __(9)                => __E____0(10) _(7).
	p(NT(9), (NT(10)+NT(7)));
//G6:   __E___1(11)          => __(9).
	p(NT(11), (NT(9)));
//G7:   __E___1(11)          => null.
	p(NT(11), (nul));
//G8:   _(7)                 => __E___1(11).
	p(NT(7), (NT(11)));
//G9:   value(8)             => true_sym(12).
	p(NT(8), (NT(12)));
//G10:  value(8)             => false_sym(13).
	p(NT(8), (NT(13)));
//G11:  value(8)             => null_sym(14).
	p(NT(8), (NT(14)));
//G12:  value(8)             => number(15).
	p(NT(8), (NT(15)));
//G13:  value(8)             => q_str(16).
	p(NT(8), (NT(16)));
//G14:  value(8)             => arr(17).
	p(NT(8), (NT(17)));
//G15:  value(8)             => object(18).
	p(NT(8), (NT(18)));
//G16:  true_sym(12)         => 't' 'r' 'u' 'e'.
	p(NT(12), (T(5)+T(6)+T(7)+T(8)));
//G17:  false_sym(13)        => 'f' 'a' 'l' 's' 'e'.
	p(NT(13), (T(9)+T(10)+T(11)+T(12)+T(8)));
//G18:  null_sym(14)         => 'n' 'u' 'l' 'l'.
	p(NT(14), (T(13)+T(7)+T(11)+T(11)));
//G19:  __E_number_2(19)     => '-'.
	p(NT(19), (T(14)));
//G20:  __E_number_2(19)     => null.
	p(NT(19), (nul));
//G21:  __E_number_3(22)     => fractional(21).
	p(NT(22), (NT(21)));
//G22:  __E_number_3(22)     => null.
	p(NT(22), (nul));
//G23:  __E_number_4(24)     => exponent(23).
	p(NT(24), (NT(23)));
//G24:  __E_number_4(24)     => null.
	p(NT(24), (nul));
//G25:  number(15)           => __E_number_2(19) integral(20) __E_number_3(22) __E_number_4(24).
	p(NT(15), (NT(19)+NT(20)+NT(22)+NT(24)));
//G26:  zero(25)             => '0'.
	p(NT(25), (T(15)));
//G27:  __N_0(52)            => zero(25).
	p(NT(52), (NT(25)));
//G28:  nonzerodigit(26)     => digit(1) & ascii(3) & ~( __N_0(52) ).	 # conjunctive
	p(NT(26), (NT(1)) & (NT(3)) & ~(NT(52)));
//G29:  __E_nzintegral_5(29) => null.
	p(NT(29), (nul));
//G30:  __E_nzintegral_5(29) => __E_nzintegral_5(29) ascii_digit(28).
	p(NT(29), (NT(29)+NT(28)));
//G31:  nzintegral(27)       => nonzerodigit(26) __E_nzintegral_5(29).
	p(NT(27), (NT(26)+NT(29)));
//G32:  integral(20)         => '0'.
	p(NT(20), (T(15)));
//G33:  integral(20)         => nzintegral(27).
	p(NT(20), (NT(27)));
//G34:  __E_fractional_6(30) => ascii_digit(28).
	p(NT(30), (NT(28)));
//G35:  __E_fractional_6(30) => __E_fractional_6(30) ascii_digit(28).
	p(NT(30), (NT(30)+NT(28)));
//G36:  fractional(21)       => '.' __E_fractional_6(30).
	p(NT(21), (T(16)+NT(30)));
//G37:  __E_exponent_7(31)   => 'e'.
	p(NT(31), (T(8)));
//G38:  __E_exponent_7(31)   => 'E'.
	p(NT(31), (T(17)));
//G39:  __E_exponent_8(32)   => '+'.
	p(NT(32), (T(18)));
//G40:  __E_exponent_8(32)   => '-'.
	p(NT(32), (T(14)));
//G41:  __E_exponent_8(32)   => null.
	p(NT(32), (nul));
//G42:  __E_exponent_9(33)   => ascii_digit(28).
	p(NT(33), (NT(28)));
//G43:  __E_exponent_9(33)   => __E_exponent_9(33) ascii_digit(28).
	p(NT(33), (NT(33)+NT(28)));
//G44:  exponent(23)         => __E_exponent_7(31) __E_exponent_8(32) __E_exponent_9(33).
	p(NT(23), (NT(31)+NT(32)+NT(33)));
//G45:  ascii_digit(28)      => digit(1) & ascii(3).	 # conjunctive
	p(NT(28), (NT(1)) & (NT(3)));
//G46:  __E_q_str_10(35)     => unescaped(36).
	p(NT(35), (NT(36)));
//G47:  __E_q_str_10(35)     => escaped(37).
	p(NT(35), (NT(37)));
//G48:  __E_q_str_11(38)     => null.
	p(NT(38), (nul));
//G49:  __E_q_str_11(38)     => __E_q_str_11(38) __E_q_str_10(35).
	p(NT(38), (NT(38)+NT(35)));
//G50:  str(34)              => __E_q_str_11(38).
	p(NT(34), (NT(38)));
//G51:  q_str(16)            => '"' str(34) '"'.
	p(NT(16), (T(19)+NT(34)+T(19)));
//G52:  __E_unescaped_12(39) => '"'.
	p(NT(39), (T(19)));
//G53:  __E_unescaped_12(39) => '\\'.
	p(NT(39), (T(20)));
//G54:  __N_1(53)            => '\x7f'.
	p(NT(53), (T(21)));
//G55:  __E_unescaped_12(39) => ~( __N_1(53) ) & ascii(3) & cntrl(5).	 # conjunctive
	p(NT(39), ~(NT(53)) & (NT(3)) & (NT(5)));
//G56:  __N_2(54)            => __E_unescaped_12(39).
	p(NT(54), (NT(39)));
//G57:  unescaped(36)        => any(2) & ~( __N_2(54) ).	 # conjunctive
	p(NT(36), (NT(2)) & ~(NT(54)));
//G58:  esc(40)              => '\\'.
	p(NT(40), (T(20)));
//G59:  __E_escaped_13(41)   => '"'.
	p(NT(41), (T(19)));
//G60:  __E_escaped_13(41)   => '\\'.
	p(NT(41), (T(20)));
//G61:  __E_escaped_13(41)   => '/'.
	p(NT(41), (T(22)));
//G62:  __E_escaped_13(41)   => 'b'.
	p(NT(41), (T(23)));
//G63:  __E_escaped_13(41)   => 'f'.
	p(NT(41), (T(9)));
//G64:  __E_escaped_13(41)   => 'n'.
	p(NT(41), (T(13)));
//G65:  __E_escaped_13(41)   => 'r'.
	p(NT(41), (T(6)));
//G66:  __E_escaped_13(41)   => 't'.
	p(NT(41), (T(5)));
//G67:  __E_escaped_13(41)   => uni_cp(42).
	p(NT(41), (NT(42)));
//G68:  escaped(37)          => esc(40) __E_escaped_13(41).
	p(NT(37), (NT(40)+NT(41)));
//G69:  uni_cp(42)           => 'u' ascii_xdigit(43) ascii_xdigit(43) ascii_xdigit(43) ascii_xdigit(43).
	p(NT(42), (T(7)+NT(43)+NT(43)+NT(43)+NT(43)));
//G70:  ascii_xdigit(43)     => ascii(3) & xdigit(4).	 # conjunctive
	p(NT(43), (NT(3)) & (NT(4)));
//G71:  __E_arr_14(44)       => values(45) _(7).
	p(NT(44), (NT(45)+NT(7)));
//G72:  __E_arr_14(44)       => null.
	p(NT(44), (nul));
//G73:  arr(17)              => '[' _(7) __E_arr_14(44) ']'.
	p(NT(17), (T(24)+NT(7)+NT(44)+T(25)));
//G74:  __E_values_15(46)    => _(7) ',' _(7) value(8).
	p(NT(46), (NT(7)+T(26)+NT(7)+NT(8)));
//G75:  __E_values_16(47)    => null.
	p(NT(47), (nul));
//G76:  __E_values_16(47)    => __E_values_16(47) __E_values_15(46).
	p(NT(47), (NT(47)+NT(46)));
//G77:  values(45)           => value(8) __E_values_16(47).
	p(NT(45), (NT(8)+NT(47)));
//G78:  __E___E_object_17_18(50) => _(7) ',' _(7) object_pair(49).
	p(NT(50), (NT(7)+T(26)+NT(7)+NT(49)));
//G79:  __E___E_object_17_19(51) => null.
	p(NT(51), (nul));
//G80:  __E___E_object_17_19(51) => __E___E_object_17_19(51) __E___E_object_17_18(50).
	p(NT(51), (NT(51)+NT(50)));
//G81:  __E_object_17(48)    => object_pair(49) __E___E_object_17_19(51) _(7).
	p(NT(48), (NT(49)+NT(51)+NT(7)));
//G82:  __E_object_17(48)    => null.
	p(NT(48), (nul));
//G83:  object(18)           => '{' _(7) __E_object_17(48) '}'.
	p(NT(18), (T(27)+NT(7)+NT(48)+T(28)));
//G84:  object_pair(49)      => q_str(16) _(7) ':' _(7) value(8).
	p(NT(49), (NT(16)+NT(7)+T(29)+NT(7)+NT(8)));
	#undef T
	#undef NT
	return loaded = true, p;
}
#else
idni::prods<char_type, terminal_type>& productions();
#endif

inline ::idni::grammar<char_type, terminal_type> grammar(
	nts, productions(), start_symbol, char_classes, grammar_options);

} // namespace json_parser_data

struct json_parser_nonterminals {
	enum nonterminal {
		nul, digit, any, ascii, xdigit, cntrl, start, _, value, __, 
		__E____0, __E___1, true_sym, false_sym, null_sym, number, q_str, arr, object, __E_number_2, 
		integral, fractional, __E_number_3, exponent, __E_number_4, zero, nonzerodigit, nzintegral, ascii_digit, __E_nzintegral_5, 
		__E_fractional_6, __E_exponent_7, __E_exponent_8, __E_exponent_9, str, __E_q_str_10, unescaped, escaped, __E_q_str_11, __E_unescaped_12, 
		esc, __E_escaped_13, uni_cp, ascii_xdigit, __E_arr_14, values, __E_values_15, __E_values_16, __E_object_17, object_pair, 
		__E___E_object_17_18, __E___E_object_17_19, __N_0, __N_1, __N_2, 
	};
};

#ifdef _MSC_VER
// the base parser puts its chart indexes on a cache line, so the padding is wanted
#	pragma warning(push)
#	pragma warning(disable : 4324)
#endif
struct json_parser : public idni::parser<char, char32_t>, public json_parser_nonterminals {
	static json_parser& instance() {
		static json_parser inst;
		return inst;
	}
	json_parser() : idni::parser<char_type, terminal_type>(
		json_parser_data::grammar,
		json_parser_data::make_parser_options()) {}
	size_t id(const std::basic_string<char_type>& name) {
		return json_parser_data::nts.get(name);
	}
	const std::basic_string<char_type>& name(size_t id) {
		return json_parser_data::nts.get(id);
	}
	symbol_type literal(const nonterminal& nt) {
		return symbol_type(nt, &json_parser_data::nts);
	}
};
#ifdef _MSC_VER
#	pragma warning(pop)
#endif

#endif // __JSON_PARSER_H__
