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

inline const std::vector<std::string>& symbol_names() {
	static const std::vector<std::string> v{
	"", "digit", "any", "ascii", "xdigit", "cntrl", "start", "_", "value", "__", 
	"__E____0", "__E___1", "true_sym", "false_sym", "null_sym", "number", "q_str", "arr", "object", "nonfinite", 
	"__E_number_2", "integral", "fractional", "__E_number_3", "exponent", "__E_number_4", "zero", "nonzerodigit", "nzintegral", "ascii_digit", 
	"__E_nzintegral_5", "__E_fractional_6", "__E_exponent_7", "__E_exponent_8", "__E_exponent_9", "str", "__E_q_str_10", "unescaped", "escaped", "__E_q_str_11", 
	"__E_unescaped_12", "esc", "__E_escaped_13", "uni_cp", "ascii_xdigit", "__E_arr_14", "values", "__E_values_15", "__E_values_16", "__E_object_17", 
	"object_pair", "__E___E_object_17_18", "__E___E_object_17_19", "__N_0", "__N_1", "__N_2", 
};
	return v;
}

inline ::idni::nonterminals<char_type, terminal_type>& nts() {
	static ::idni::nonterminals<char_type, terminal_type> v{
		symbol_names() };
	return v;
}

inline std::vector<terminal_type>& terminals() {
	static std::vector<terminal_type> v{
	U'\0', U' ', U'\t', U'\r', U'\n', U't', U'r', U'u', U'e', 
	U'f', U'a', U'l', U's', U'n', U'N', U'I', U'i', U'y', U'-', 
	U'0', U'.', U'E', U'+', U'"', U'\\', U'', U'/', U'b', U'[', 
	U']', U',', U'{', U'}', U':', 
};
	return v;
}

inline ::idni::char_class_fns<terminal_type>& char_classes() {
	static ::idni::char_class_fns<terminal_type> v =
	::idni::predefined_char_classes<char_type, terminal_type>({
		"digit",
		"any",
		"ascii",
		"xdigit",
		"cntrl",
	}, nts());
	return v;
}

inline ::idni::grammar<char_type, terminal_type>::options&
	grammar_options()
{
	static struct ::idni::grammar<char_type, terminal_type>::options v
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
			{ 16, 35 },
			{ 21 },
			{ 26 },
			{ 27 },
			{ 28 },
			{ 29 },
			{ 37 },
			{ 41 },
			{ 43 },
			{ 44 }
		},
		.inline_char_classes = true
	},
	.enabled_guards = {
		"standard"
	},
	.variants = {
		{ "nonfinite", { "nonfinite" } },
		{ "standard", { "standard" } }
	},
	.variant = "standard"
};
	return v;
}

inline auto make_parser_options() {
	auto o = ::idni::default_parser_options<char_type, terminal_type>();
	o.codec.decode = idni::utf8_to_u32_conv;
	o.codec.encode = idni::u32_to_utf8_conv;
	return o;
}

inline ::idni::prods<char_type, terminal_type>& start_symbol() {
	static ::idni::prods<char_type, terminal_type> v{ nts()(6) };
	return v;
}

#ifdef TAU_PARSER_BUILD_HEADER_ONLY
inline idni::prods<char_type, terminal_type>& productions() {
	static idni::prods<char_type, terminal_type> p = [] {
	idni::prods<char_type, terminal_type>
		p, nul(idni::lit<char_type, terminal_type>{});
	const auto& ts = terminals();
	auto& ns = nts();
	#define  T(x) (idni::prods<char_type, terminal_type>{ ts[x] })
	#define NT(x) (idni::prods<char_type, terminal_type>{ ns(x) })
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
//G16:  value(8)             => nonfinite(19).	 # guarded: nonfinite
	p(NT(8), (NT(19)));
	p.back().guard = "nonfinite";
//G17:  true_sym(12)         => 't' 'r' 'u' 'e'.
	p(NT(12), (T(5)+T(6)+T(7)+T(8)));
//G18:  false_sym(13)        => 'f' 'a' 'l' 's' 'e'.
	p(NT(13), (T(9)+T(10)+T(11)+T(12)+T(8)));
//G19:  null_sym(14)         => 'n' 'u' 'l' 'l'.
	p(NT(14), (T(13)+T(7)+T(11)+T(11)));
//G20:  nonfinite(19)        => 'N' 'a' 'N'.
	p(NT(19), (T(14)+T(10)+T(14)));
//G21:  nonfinite(19)        => 'I' 'n' 'f' 'i' 'n' 'i' 't' 'y'.
	p(NT(19), (T(15)+T(13)+T(9)+T(16)+T(13)+T(16)+T(5)+T(17)));
//G22:  nonfinite(19)        => '-' 'I' 'n' 'f' 'i' 'n' 'i' 't' 'y'.
	p(NT(19), (T(18)+T(15)+T(13)+T(9)+T(16)+T(13)+T(16)+T(5)+T(17)));
//G23:  __E_number_2(20)     => '-'.
	p(NT(20), (T(18)));
//G24:  __E_number_2(20)     => null.
	p(NT(20), (nul));
//G25:  __E_number_3(23)     => fractional(22).
	p(NT(23), (NT(22)));
//G26:  __E_number_3(23)     => null.
	p(NT(23), (nul));
//G27:  __E_number_4(25)     => exponent(24).
	p(NT(25), (NT(24)));
//G28:  __E_number_4(25)     => null.
	p(NT(25), (nul));
//G29:  number(15)           => __E_number_2(20) integral(21) __E_number_3(23) __E_number_4(25).
	p(NT(15), (NT(20)+NT(21)+NT(23)+NT(25)));
//G30:  zero(26)             => '0'.
	p(NT(26), (T(19)));
//G31:  __N_0(53)            => zero(26).
	p(NT(53), (NT(26)));
//G32:  nonzerodigit(27)     => digit(1) & ascii(3) & ~( __N_0(53) ).	 # conjunctive
	p(NT(27), (NT(1)) & (NT(3)) & ~(NT(53)));
//G33:  __E_nzintegral_5(30) => null.
	p(NT(30), (nul));
//G34:  __E_nzintegral_5(30) => __E_nzintegral_5(30) ascii_digit(29).
	p(NT(30), (NT(30)+NT(29)));
//G35:  nzintegral(28)       => nonzerodigit(27) __E_nzintegral_5(30).
	p(NT(28), (NT(27)+NT(30)));
//G36:  integral(21)         => '0'.
	p(NT(21), (T(19)));
//G37:  integral(21)         => nzintegral(28).
	p(NT(21), (NT(28)));
//G38:  __E_fractional_6(31) => ascii_digit(29).
	p(NT(31), (NT(29)));
//G39:  __E_fractional_6(31) => __E_fractional_6(31) ascii_digit(29).
	p(NT(31), (NT(31)+NT(29)));
//G40:  fractional(22)       => '.' __E_fractional_6(31).
	p(NT(22), (T(20)+NT(31)));
//G41:  __E_exponent_7(32)   => 'e'.
	p(NT(32), (T(8)));
//G42:  __E_exponent_7(32)   => 'E'.
	p(NT(32), (T(21)));
//G43:  __E_exponent_8(33)   => '+'.
	p(NT(33), (T(22)));
//G44:  __E_exponent_8(33)   => '-'.
	p(NT(33), (T(18)));
//G45:  __E_exponent_8(33)   => null.
	p(NT(33), (nul));
//G46:  __E_exponent_9(34)   => ascii_digit(29).
	p(NT(34), (NT(29)));
//G47:  __E_exponent_9(34)   => __E_exponent_9(34) ascii_digit(29).
	p(NT(34), (NT(34)+NT(29)));
//G48:  exponent(24)         => __E_exponent_7(32) __E_exponent_8(33) __E_exponent_9(34).
	p(NT(24), (NT(32)+NT(33)+NT(34)));
//G49:  ascii_digit(29)      => digit(1) & ascii(3).	 # conjunctive
	p(NT(29), (NT(1)) & (NT(3)));
//G50:  __E_q_str_10(36)     => unescaped(37).
	p(NT(36), (NT(37)));
//G51:  __E_q_str_10(36)     => escaped(38).
	p(NT(36), (NT(38)));
//G52:  __E_q_str_11(39)     => null.
	p(NT(39), (nul));
//G53:  __E_q_str_11(39)     => __E_q_str_11(39) __E_q_str_10(36).
	p(NT(39), (NT(39)+NT(36)));
//G54:  str(35)              => __E_q_str_11(39).
	p(NT(35), (NT(39)));
//G55:  q_str(16)            => '"' str(35) '"'.
	p(NT(16), (T(23)+NT(35)+T(23)));
//G56:  __E_unescaped_12(40) => '"'.
	p(NT(40), (T(23)));
//G57:  __E_unescaped_12(40) => '\\'.
	p(NT(40), (T(24)));
//G58:  __N_1(54)            => '\x7f'.
	p(NT(54), (T(25)));
//G59:  __E_unescaped_12(40) => ~( __N_1(54) ) & ascii(3) & cntrl(5).	 # conjunctive
	p(NT(40), ~(NT(54)) & (NT(3)) & (NT(5)));
//G60:  __N_2(55)            => __E_unescaped_12(40).
	p(NT(55), (NT(40)));
//G61:  unescaped(37)        => any(2) & ~( __N_2(55) ).	 # conjunctive
	p(NT(37), (NT(2)) & ~(NT(55)));
//G62:  esc(41)              => '\\'.
	p(NT(41), (T(24)));
//G63:  __E_escaped_13(42)   => '"'.
	p(NT(42), (T(23)));
//G64:  __E_escaped_13(42)   => '\\'.
	p(NT(42), (T(24)));
//G65:  __E_escaped_13(42)   => '/'.
	p(NT(42), (T(26)));
//G66:  __E_escaped_13(42)   => 'b'.
	p(NT(42), (T(27)));
//G67:  __E_escaped_13(42)   => 'f'.
	p(NT(42), (T(9)));
//G68:  __E_escaped_13(42)   => 'n'.
	p(NT(42), (T(13)));
//G69:  __E_escaped_13(42)   => 'r'.
	p(NT(42), (T(6)));
//G70:  __E_escaped_13(42)   => 't'.
	p(NT(42), (T(5)));
//G71:  __E_escaped_13(42)   => uni_cp(43).
	p(NT(42), (NT(43)));
//G72:  escaped(38)          => esc(41) __E_escaped_13(42).
	p(NT(38), (NT(41)+NT(42)));
//G73:  uni_cp(43)           => 'u' ascii_xdigit(44) ascii_xdigit(44) ascii_xdigit(44) ascii_xdigit(44).
	p(NT(43), (T(7)+NT(44)+NT(44)+NT(44)+NT(44)));
//G74:  ascii_xdigit(44)     => ascii(3) & xdigit(4).	 # conjunctive
	p(NT(44), (NT(3)) & (NT(4)));
//G75:  __E_arr_14(45)       => values(46) _(7).
	p(NT(45), (NT(46)+NT(7)));
//G76:  __E_arr_14(45)       => null.
	p(NT(45), (nul));
//G77:  arr(17)              => '[' _(7) __E_arr_14(45) ']'.
	p(NT(17), (T(28)+NT(7)+NT(45)+T(29)));
//G78:  __E_values_15(47)    => _(7) ',' _(7) value(8).
	p(NT(47), (NT(7)+T(30)+NT(7)+NT(8)));
//G79:  __E_values_16(48)    => null.
	p(NT(48), (nul));
//G80:  __E_values_16(48)    => __E_values_16(48) __E_values_15(47).
	p(NT(48), (NT(48)+NT(47)));
//G81:  values(46)           => value(8) __E_values_16(48).
	p(NT(46), (NT(8)+NT(48)));
//G82:  __E___E_object_17_18(51) => _(7) ',' _(7) object_pair(50).
	p(NT(51), (NT(7)+T(30)+NT(7)+NT(50)));
//G83:  __E___E_object_17_19(52) => null.
	p(NT(52), (nul));
//G84:  __E___E_object_17_19(52) => __E___E_object_17_19(52) __E___E_object_17_18(51).
	p(NT(52), (NT(52)+NT(51)));
//G85:  __E_object_17(49)    => object_pair(50) __E___E_object_17_19(52) _(7).
	p(NT(49), (NT(50)+NT(52)+NT(7)));
//G86:  __E_object_17(49)    => null.
	p(NT(49), (nul));
//G87:  object(18)           => '{' _(7) __E_object_17(49) '}'.
	p(NT(18), (T(31)+NT(7)+NT(49)+T(32)));
//G88:  object_pair(50)      => q_str(16) _(7) ':' _(7) value(8).
	p(NT(50), (NT(16)+NT(7)+T(33)+NT(7)+NT(8)));
	#undef T
	#undef NT
	return p;
	}();
	return p;
}
#else
idni::prods<char_type, terminal_type>& productions();
#endif

inline ::idni::grammar<char_type, terminal_type>& grammar() {
	static ::idni::grammar<char_type, terminal_type> v(nts(),
		productions(), start_symbol(), char_classes(),
		grammar_options());
	return v;
}

} // namespace json_parser_data

struct json_parser_nonterminals {
	enum nonterminal {
		nul, digit, any, ascii, xdigit, cntrl, start, _, value, __, 
		__E____0, __E___1, true_sym, false_sym, null_sym, number, q_str, arr, object, nonfinite, 
		__E_number_2, integral, fractional, __E_number_3, exponent, __E_number_4, zero, nonzerodigit, nzintegral, ascii_digit, 
		__E_nzintegral_5, __E_fractional_6, __E_exponent_7, __E_exponent_8, __E_exponent_9, str, __E_q_str_10, unescaped, escaped, __E_q_str_11, 
		__E_unescaped_12, esc, __E_escaped_13, uni_cp, ascii_xdigit, __E_arr_14, values, __E_values_15, __E_values_16, __E_object_17, 
		object_pair, __E___E_object_17_18, __E___E_object_17_19, __N_0, __N_1, __N_2, 
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
		json_parser_data::grammar(),
		json_parser_data::make_parser_options()) {}
	size_t id(const std::basic_string<char_type>& name) {
		return json_parser_data::nts().get(name);
	}
	const std::basic_string<char_type>& name(size_t id) {
		return json_parser_data::nts().get(id);
	}
	symbol_type literal(const nonterminal& nt) {
		return symbol_type(nt, &json_parser_data::nts());
	}
};
#ifdef _MSC_VER
#	pragma warning(pop)
#endif

#endif // __JSON_PARSER_H__
