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
	"", "digit", "any", "ascii", "xdigit", "cntrl", "start", "_", "value", "wire_line", 
	"__E_wire_line_0", "__", "__E____1", "__E___2", "true_sym", "false_sym", "null_sym", "number", "q_str", "arr", 
	"object", "nonfinite", "__E_number_3", "integral", "fractional", "__E_number_4", "exponent", "__E_number_5", "zero", "nonzerodigit", 
	"nzintegral", "ascii_digit", "__E_nzintegral_6", "__E_fractional_7", "__E_exponent_8", "__E_exponent_9", "__E_exponent_10", "str", "__E_q_str_11", "unescaped", 
	"escaped", "__E_q_str_12", "__E_unescaped_13", "esc", "__E_escaped_14", "uni_cp", "ascii_xdigit", "__E_arr_15", "values", "__E_values_16", 
	"__E_values_17", "__E_object_18", "object_pair", "__E___E_object_18_19", "__E___E_object_18_20", "__N_0", "__N_1", "__N_2", 
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
			7, 11
		},
		.to_trim_children = {
			14, 15, 16
		},
		.trim_terminals = false,
		.to_inline = {
			{ 18, 37 },
			{ 23 },
			{ 28 },
			{ 29 },
			{ 30 },
			{ 31 },
			{ 39 },
			{ 43 },
			{ 45 },
			{ 46 }
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
//G1:   __E_wire_line_0(10)  => _(7) value(8).
	p(NT(10), (NT(7)+NT(8)));
//G2:   __E_wire_line_0(10)  => null.
	p(NT(10), (nul));
//G3:   wire_line(9)         => __E_wire_line_0(10) _(7).
	p(NT(9), (NT(10)+NT(7)));
//G4:   __E____1(12)         => ' '.
	p(NT(12), (T(1)));
//G5:   __E____1(12)         => '\t'.
	p(NT(12), (T(2)));
//G6:   __E____1(12)         => '\r'.
	p(NT(12), (T(3)));
//G7:   __E____1(12)         => '\n'.
	p(NT(12), (T(4)));
//G8:   __(11)               => __E____1(12) _(7).
	p(NT(11), (NT(12)+NT(7)));
//G9:   __E___2(13)          => __(11).
	p(NT(13), (NT(11)));
//G10:  __E___2(13)          => null.
	p(NT(13), (nul));
//G11:  _(7)                 => __E___2(13).
	p(NT(7), (NT(13)));
//G12:  value(8)             => true_sym(14).
	p(NT(8), (NT(14)));
//G13:  value(8)             => false_sym(15).
	p(NT(8), (NT(15)));
//G14:  value(8)             => null_sym(16).
	p(NT(8), (NT(16)));
//G15:  value(8)             => number(17).
	p(NT(8), (NT(17)));
//G16:  value(8)             => q_str(18).
	p(NT(8), (NT(18)));
//G17:  value(8)             => arr(19).
	p(NT(8), (NT(19)));
//G18:  value(8)             => object(20).
	p(NT(8), (NT(20)));
//G19:  value(8)             => nonfinite(21).	 # guarded: nonfinite
	p(NT(8), (NT(21)));
	p.back().guard = "nonfinite";
//G20:  true_sym(14)         => 't' 'r' 'u' 'e'.
	p(NT(14), (T(5)+T(6)+T(7)+T(8)));
//G21:  false_sym(15)        => 'f' 'a' 'l' 's' 'e'.
	p(NT(15), (T(9)+T(10)+T(11)+T(12)+T(8)));
//G22:  null_sym(16)         => 'n' 'u' 'l' 'l'.
	p(NT(16), (T(13)+T(7)+T(11)+T(11)));
//G23:  nonfinite(21)        => 'N' 'a' 'N'.
	p(NT(21), (T(14)+T(10)+T(14)));
//G24:  nonfinite(21)        => 'I' 'n' 'f' 'i' 'n' 'i' 't' 'y'.
	p(NT(21), (T(15)+T(13)+T(9)+T(16)+T(13)+T(16)+T(5)+T(17)));
//G25:  nonfinite(21)        => '-' 'I' 'n' 'f' 'i' 'n' 'i' 't' 'y'.
	p(NT(21), (T(18)+T(15)+T(13)+T(9)+T(16)+T(13)+T(16)+T(5)+T(17)));
//G26:  __E_number_3(22)     => '-'.
	p(NT(22), (T(18)));
//G27:  __E_number_3(22)     => null.
	p(NT(22), (nul));
//G28:  __E_number_4(25)     => fractional(24).
	p(NT(25), (NT(24)));
//G29:  __E_number_4(25)     => null.
	p(NT(25), (nul));
//G30:  __E_number_5(27)     => exponent(26).
	p(NT(27), (NT(26)));
//G31:  __E_number_5(27)     => null.
	p(NT(27), (nul));
//G32:  number(17)           => __E_number_3(22) integral(23) __E_number_4(25) __E_number_5(27).
	p(NT(17), (NT(22)+NT(23)+NT(25)+NT(27)));
//G33:  zero(28)             => '0'.
	p(NT(28), (T(19)));
//G34:  __N_0(55)            => zero(28).
	p(NT(55), (NT(28)));
//G35:  nonzerodigit(29)     => digit(1) & ascii(3) & ~( __N_0(55) ).	 # conjunctive
	p(NT(29), (NT(1)) & (NT(3)) & ~(NT(55)));
//G36:  __E_nzintegral_6(32) => null.
	p(NT(32), (nul));
//G37:  __E_nzintegral_6(32) => __E_nzintegral_6(32) ascii_digit(31).
	p(NT(32), (NT(32)+NT(31)));
//G38:  nzintegral(30)       => nonzerodigit(29) __E_nzintegral_6(32).
	p(NT(30), (NT(29)+NT(32)));
//G39:  integral(23)         => '0'.
	p(NT(23), (T(19)));
//G40:  integral(23)         => nzintegral(30).
	p(NT(23), (NT(30)));
//G41:  __E_fractional_7(33) => ascii_digit(31).
	p(NT(33), (NT(31)));
//G42:  __E_fractional_7(33) => __E_fractional_7(33) ascii_digit(31).
	p(NT(33), (NT(33)+NT(31)));
//G43:  fractional(24)       => '.' __E_fractional_7(33).
	p(NT(24), (T(20)+NT(33)));
//G44:  __E_exponent_8(34)   => 'e'.
	p(NT(34), (T(8)));
//G45:  __E_exponent_8(34)   => 'E'.
	p(NT(34), (T(21)));
//G46:  __E_exponent_9(35)   => '+'.
	p(NT(35), (T(22)));
//G47:  __E_exponent_9(35)   => '-'.
	p(NT(35), (T(18)));
//G48:  __E_exponent_9(35)   => null.
	p(NT(35), (nul));
//G49:  __E_exponent_10(36)  => ascii_digit(31).
	p(NT(36), (NT(31)));
//G50:  __E_exponent_10(36)  => __E_exponent_10(36) ascii_digit(31).
	p(NT(36), (NT(36)+NT(31)));
//G51:  exponent(26)         => __E_exponent_8(34) __E_exponent_9(35) __E_exponent_10(36).
	p(NT(26), (NT(34)+NT(35)+NT(36)));
//G52:  ascii_digit(31)      => digit(1) & ascii(3).	 # conjunctive
	p(NT(31), (NT(1)) & (NT(3)));
//G53:  __E_q_str_11(38)     => unescaped(39).
	p(NT(38), (NT(39)));
//G54:  __E_q_str_11(38)     => escaped(40).
	p(NT(38), (NT(40)));
//G55:  __E_q_str_12(41)     => null.
	p(NT(41), (nul));
//G56:  __E_q_str_12(41)     => __E_q_str_12(41) __E_q_str_11(38).
	p(NT(41), (NT(41)+NT(38)));
//G57:  str(37)              => __E_q_str_12(41).
	p(NT(37), (NT(41)));
//G58:  q_str(18)            => '"' str(37) '"'.
	p(NT(18), (T(23)+NT(37)+T(23)));
//G59:  __E_unescaped_13(42) => '"'.
	p(NT(42), (T(23)));
//G60:  __E_unescaped_13(42) => '\\'.
	p(NT(42), (T(24)));
//G61:  __N_1(56)            => '\x7f'.
	p(NT(56), (T(25)));
//G62:  __E_unescaped_13(42) => ~( __N_1(56) ) & ascii(3) & cntrl(5).	 # conjunctive
	p(NT(42), ~(NT(56)) & (NT(3)) & (NT(5)));
//G63:  __N_2(57)            => __E_unescaped_13(42).
	p(NT(57), (NT(42)));
//G64:  unescaped(39)        => any(2) & ~( __N_2(57) ).	 # conjunctive
	p(NT(39), (NT(2)) & ~(NT(57)));
//G65:  esc(43)              => '\\'.
	p(NT(43), (T(24)));
//G66:  __E_escaped_14(44)   => '"'.
	p(NT(44), (T(23)));
//G67:  __E_escaped_14(44)   => '\\'.
	p(NT(44), (T(24)));
//G68:  __E_escaped_14(44)   => '/'.
	p(NT(44), (T(26)));
//G69:  __E_escaped_14(44)   => 'b'.
	p(NT(44), (T(27)));
//G70:  __E_escaped_14(44)   => 'f'.
	p(NT(44), (T(9)));
//G71:  __E_escaped_14(44)   => 'n'.
	p(NT(44), (T(13)));
//G72:  __E_escaped_14(44)   => 'r'.
	p(NT(44), (T(6)));
//G73:  __E_escaped_14(44)   => 't'.
	p(NT(44), (T(5)));
//G74:  __E_escaped_14(44)   => uni_cp(45).
	p(NT(44), (NT(45)));
//G75:  escaped(40)          => esc(43) __E_escaped_14(44).
	p(NT(40), (NT(43)+NT(44)));
//G76:  uni_cp(45)           => 'u' ascii_xdigit(46) ascii_xdigit(46) ascii_xdigit(46) ascii_xdigit(46).
	p(NT(45), (T(7)+NT(46)+NT(46)+NT(46)+NT(46)));
//G77:  ascii_xdigit(46)     => ascii(3) & xdigit(4).	 # conjunctive
	p(NT(46), (NT(3)) & (NT(4)));
//G78:  __E_arr_15(47)       => values(48) _(7).
	p(NT(47), (NT(48)+NT(7)));
//G79:  __E_arr_15(47)       => null.
	p(NT(47), (nul));
//G80:  arr(19)              => '[' _(7) __E_arr_15(47) ']'.
	p(NT(19), (T(28)+NT(7)+NT(47)+T(29)));
//G81:  __E_values_16(49)    => _(7) ',' _(7) value(8).
	p(NT(49), (NT(7)+T(30)+NT(7)+NT(8)));
//G82:  __E_values_17(50)    => null.
	p(NT(50), (nul));
//G83:  __E_values_17(50)    => __E_values_17(50) __E_values_16(49).
	p(NT(50), (NT(50)+NT(49)));
//G84:  values(48)           => value(8) __E_values_17(50).
	p(NT(48), (NT(8)+NT(50)));
//G85:  __E___E_object_18_19(53) => _(7) ',' _(7) object_pair(52).
	p(NT(53), (NT(7)+T(30)+NT(7)+NT(52)));
//G86:  __E___E_object_18_20(54) => null.
	p(NT(54), (nul));
//G87:  __E___E_object_18_20(54) => __E___E_object_18_20(54) __E___E_object_18_19(53).
	p(NT(54), (NT(54)+NT(53)));
//G88:  __E_object_18(51)    => object_pair(52) __E___E_object_18_20(54) _(7).
	p(NT(51), (NT(52)+NT(54)+NT(7)));
//G89:  __E_object_18(51)    => null.
	p(NT(51), (nul));
//G90:  object(20)           => '{' _(7) __E_object_18(51) '}'.
	p(NT(20), (T(31)+NT(7)+NT(51)+T(32)));
//G91:  object_pair(52)      => q_str(18) _(7) ':' _(7) value(8).
	p(NT(52), (NT(18)+NT(7)+T(33)+NT(7)+NT(8)));
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
		nul, digit, any, ascii, xdigit, cntrl, start, _, value, wire_line, 
		__E_wire_line_0, __, __E____1, __E___2, true_sym, false_sym, null_sym, number, q_str, arr, 
		object, nonfinite, __E_number_3, integral, fractional, __E_number_4, exponent, __E_number_5, zero, nonzerodigit, 
		nzintegral, ascii_digit, __E_nzintegral_6, __E_fractional_7, __E_exponent_8, __E_exponent_9, __E_exponent_10, str, __E_q_str_11, unescaped, 
		escaped, __E_q_str_12, __E_unescaped_13, esc, __E_escaped_14, uni_cp, ascii_xdigit, __E_arr_15, values, __E_values_16, 
		__E_values_17, __E_object_18, object_pair, __E___E_object_18_19, __E___E_object_18_20, __N_0, __N_1, __N_2, 
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
