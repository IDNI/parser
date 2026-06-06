// This file is generated from a file src/tgf.tgf by
//       https://github.com/IDNI/parser/src/tgf
//
#ifndef __TGF_PARSER_H__
#define __TGF_PARSER_H__

#include "parser.h"
#include "recoders.h"

namespace tgf_parser_data {

using char_type     = char;
using terminal_type = char32_t;

inline static constexpr size_t nt_bits = 7;
inline const std::vector<std::string> symbol_names{
	"", "eof", "alnum", "alpha", "space", "printable", "xdigit", "__", "_", "sep", 
	"sym", "cc_name", "escaped_s", "unescaped_s", "escaped_c", "unescaped_c", "terminal_hex", "hex_bytes", "syms", "escape_char", 
	"esc_hex", "esc_u4", "esc_U8", "start", "__E_start_0", "statement", "__E_start_1", "directive", "production", "start_statement", 
	"__E_production_2", "production_guard", "alternation", "conjunction", "__E_alternation_3", "__E_alternation_4", "concatenation", "__E_conjunction_5", "__E_conjunction_6", "factor", 
	"__E_concatenation_7", "__E_concatenation_8", "shorthand_rule", "__E_factor_9", "optional", "__E_factor_10", "term", "repeat", "__E_factor_11", "none_or_repeat", 
	"__E_factor_12", "neg", "__E_factor_13", "group", "__E_term_14", "optional_group", "__E_term_15", "repeat_group", "__E_term_16", "terminal", 
	"terminal_char", "terminal_string", "__E_terminal_hex_17", "__E___E_terminal_hex_17_18", "__E___E_terminal_hex_17_19", "__E_sym_20", "__E_sym_21", "__E_sym_22", "__E_terminal_char_23", "__E_unescaped_c_24", 
	"__E_escaped_c_25", "__E_terminal_string_26", "__E_terminal_string_27", "__E_unescaped_s_28", "__E_escaped_s_29", "__E_esc_hex_30", "__E_esc_hex_31", "directive_body", "start_dir", "__E_directive_body_32", 
	"inline_dir", "__E_directive_body_33", "inline_arg", "__E___E_directive_body_33_34", "__E___E_directive_body_33_35", "trim_children_dir", "__E_directive_body_36", "trim_children_terminals_dir", "__E_directive_body_37", "trim_all_terminals_dir", 
	"__E_directive_body_38", "__E___E_directive_body_38_39", "__E___E___E_directive_body_38_39_40", "trim_dir", "__E_directive_body_41", "use_dir", "__E_directive_body_42", "use_from", "cc_sym", "use_param", 
	"__E___E_directive_body_42_43", "__E___E_directive_body_42_44", "disable_ad_dir", "__E_directive_body_45", "__E___E_directive_body_45_46", "__E___E_directive_body_45_47", "enable_prods_dir", "__E_directive_body_48", "ambiguous_dir", "__E_directive_body_49", 
	"__E_syms_50", "__E_syms_51", "tree_path", "__E_inline_arg_52", "__E___E_inline_arg_52_53", "__E_tree_path_54", "__E_tree_path_55", "__E_use_param_56", "__E_sep_57", "sep_required", 
	"comment", "__E_comment_58", "__E_comment_59", "__E_comment_60", "__N_0", "__N_1", 
};

inline ::idni::nonterminals<char_type, terminal_type> nts{symbol_names};

inline std::vector<terminal_type> terminals{
	U'\0', U'[', U']', U'=', U'>', U'.', U'|', U'&', U':', 
	U'?', U'+', U'*', U'~', U'(', U')', U'{', U'}', U'0', U'x', 
	U'_', U'\'', U'\\', U'"', U'a', U'b', U'f', U'n', U'r', U't', 
	U'v', U'/', U'X', U'u', U'U', U'@', U's', U',', U'i', U'l', 
	U'e', U'm', U'c', U'h', U'd', U'o', U'p', U'g', U'y', U'k', 
	U'w', U'-', U'\t', U'\r', U'\n', U'#', 
};

inline ::idni::char_class_fns<terminal_type> char_classes =
	::idni::predefined_char_classes<char_type, terminal_type>({
		"eof",
		"alnum",
		"alpha",
		"space",
		"printable",
		"xdigit",
	}, nts);

inline struct ::idni::grammar<char_type, terminal_type>::options
	grammar_options
{
	.transform_negation = false,
	.auto_disambiguate = true,
	.shaping = {
		.to_trim = {
			7, 8, 9
		},
		.trim_terminals = true,
		.dont_trim_terminals_of = {
			10, 11, 12, 13, 14, 15, 16, 17
		},
		.to_inline = {
			{ 18 },
			{ 19 },
			{ 20 },
			{ 21 },
			{ 22 }
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

inline ::idni::prods<char_type, terminal_type> start_symbol{ nts(23) };

inline idni::prods<char_type, terminal_type>& productions() {
	static bool loaded = false;
	static idni::prods<char_type, terminal_type>
		p, nul(idni::lit<char_type, terminal_type>{});
	if (loaded) return p;
	#define  T(x) (idni::prods<char_type, terminal_type>{ terminals[x] })
	#define NT(x) (idni::prods<char_type, terminal_type>{ nts(x) })
//G0:   __E_start_0(24)      => _(8) statement(25).
	p(NT(24), (NT(8)+NT(25)));
//G1:   __E_start_1(26)      => null.
	p(NT(26), (nul));
//G2:   __E_start_1(26)      => __E_start_0(24) __E_start_1(26).
	p(NT(26), (NT(24)+NT(26)));
//G3:   start(23)            => __E_start_1(26) _(8).
	p(NT(23), (NT(26)+NT(8)));
//G4:   statement(25)        => directive(27).
	p(NT(25), (NT(27)));
//G5:   statement(25)        => production(28).
	p(NT(25), (NT(28)));
//G6:   start_statement(29)  => _(8) statement(25) _(8).
	p(NT(29), (NT(8)+NT(25)+NT(8)));
//G7:   production_guard(31) => sym(10).
	p(NT(31), (NT(10)));
//G8:   __E_production_2(30) => _(8) '[' _(8) production_guard(31) _(8) ']'.
	p(NT(30), (NT(8)+T(1)+NT(8)+NT(31)+NT(8)+T(2)));
//G9:   __E_production_2(30) => null.
	p(NT(30), (nul));
//G10:  production(28)       => sym(10) __E_production_2(30) _(8) '=' '>' _(8) alternation(32) _(8) '.'.
	p(NT(28), (NT(10)+NT(30)+NT(8)+T(3)+T(4)+NT(8)+NT(32)+NT(8)+T(5)));
//G11:  __E_alternation_3(34) => _(8) '|' _(8) conjunction(33).
	p(NT(34), (NT(8)+T(6)+NT(8)+NT(33)));
//G12:  __E_alternation_4(35) => null.
	p(NT(35), (nul));
//G13:  __E_alternation_4(35) => __E_alternation_3(34) __E_alternation_4(35).
	p(NT(35), (NT(34)+NT(35)));
//G14:  alternation(32)      => conjunction(33) __E_alternation_4(35).
	p(NT(32), (NT(33)+NT(35)));
//G15:  __E_conjunction_5(37) => _(8) '&' _(8) concatenation(36).
	p(NT(37), (NT(8)+T(7)+NT(8)+NT(36)));
//G16:  __E_conjunction_6(38) => null.
	p(NT(38), (nul));
//G17:  __E_conjunction_6(38) => __E_conjunction_5(37) __E_conjunction_6(38).
	p(NT(38), (NT(37)+NT(38)));
//G18:  conjunction(33)      => concatenation(36) __E_conjunction_6(38).
	p(NT(33), (NT(36)+NT(38)));
//G19:  __E_concatenation_7(40) => __(7) factor(39).
	p(NT(40), (NT(7)+NT(39)));
//G20:  __E_concatenation_8(41) => null.
	p(NT(41), (nul));
//G21:  __E_concatenation_8(41) => __E_concatenation_7(40) __E_concatenation_8(41).
	p(NT(41), (NT(40)+NT(41)));
//G22:  concatenation(36)    => factor(39) __E_concatenation_8(41).
	p(NT(36), (NT(39)+NT(41)));
//G23:  __E_factor_9(43)     => factor(39) _(8) ':' sym(10).
	p(NT(43), (NT(39)+NT(8)+T(8)+NT(10)));
//G24:  shorthand_rule(42)   => __E_factor_9(43).
	p(NT(42), (NT(43)));
//G25:  factor(39)           => shorthand_rule(42).
	p(NT(39), (NT(42)));
//G26:  __E_factor_10(45)    => term(46) '?'.
	p(NT(45), (NT(46)+T(9)));
//G27:  optional(44)         => __E_factor_10(45).
	p(NT(44), (NT(45)));
//G28:  factor(39)           => optional(44).
	p(NT(39), (NT(44)));
//G29:  __E_factor_11(48)    => term(46) '+'.
	p(NT(48), (NT(46)+T(10)));
//G30:  repeat(47)           => __E_factor_11(48).
	p(NT(47), (NT(48)));
//G31:  factor(39)           => repeat(47).
	p(NT(39), (NT(47)));
//G32:  __E_factor_12(50)    => term(46) '*'.
	p(NT(50), (NT(46)+T(11)));
//G33:  none_or_repeat(49)   => __E_factor_12(50).
	p(NT(49), (NT(50)));
//G34:  factor(39)           => none_or_repeat(49).
	p(NT(39), (NT(49)));
//G35:  __E_factor_13(52)    => '~' term(46).
	p(NT(52), (T(12)+NT(46)));
//G36:  neg(51)              => __E_factor_13(52).
	p(NT(51), (NT(52)));
//G37:  factor(39)           => neg(51).
	p(NT(39), (NT(51)));
//G38:  factor(39)           => term(46).
	p(NT(39), (NT(46)));
//G39:  __E_term_14(54)      => '(' _(8) alternation(32) _(8) ')'.
	p(NT(54), (T(13)+NT(8)+NT(32)+NT(8)+T(14)));
//G40:  group(53)            => __E_term_14(54).
	p(NT(53), (NT(54)));
//G41:  term(46)             => group(53).
	p(NT(46), (NT(53)));
//G42:  __E_term_15(56)      => '[' _(8) alternation(32) _(8) ']'.
	p(NT(56), (T(1)+NT(8)+NT(32)+NT(8)+T(2)));
//G43:  optional_group(55)   => __E_term_15(56).
	p(NT(55), (NT(56)));
//G44:  term(46)             => optional_group(55).
	p(NT(46), (NT(55)));
//G45:  __E_term_16(58)      => '{' _(8) alternation(32) _(8) '}'.
	p(NT(58), (T(15)+NT(8)+NT(32)+NT(8)+T(16)));
//G46:  repeat_group(57)     => __E_term_16(58).
	p(NT(57), (NT(58)));
//G47:  term(46)             => repeat_group(57).
	p(NT(46), (NT(57)));
//G48:  term(46)             => terminal(59).
	p(NT(46), (NT(59)));
//G49:  term(46)             => sym(10).
	p(NT(46), (NT(10)));
//G50:  terminal(59)         => terminal_char(60).
	p(NT(59), (NT(60)));
//G51:  terminal(59)         => terminal_string(61).
	p(NT(59), (NT(61)));
//G52:  terminal(59)         => terminal_hex(16).
	p(NT(59), (NT(16)));
//G53:  __E_terminal_hex_17(62) => xdigit(6).
	p(NT(62), (NT(6)));
//G54:  __E___E_terminal_hex_17_18(63) => xdigit(6) xdigit(6).
	p(NT(63), (NT(6)+NT(6)));
//G55:  __E___E_terminal_hex_17_19(64) => __E___E_terminal_hex_17_18(63).
	p(NT(64), (NT(63)));
//G56:  __E___E_terminal_hex_17_19(64) => __E___E_terminal_hex_17_18(63) __E___E_terminal_hex_17_19(64).
	p(NT(64), (NT(63)+NT(64)));
//G57:  __E_terminal_hex_17(62) => __E___E_terminal_hex_17_19(64).
	p(NT(62), (NT(64)));
//G58:  hex_bytes(17)        => __E_terminal_hex_17(62).
	p(NT(17), (NT(62)));
//G59:  terminal_hex(16)     => '0' 'x' hex_bytes(17).
	p(NT(16), (T(17)+T(18)+NT(17)));
//G60:  __E_sym_20(65)       => alpha(3).
	p(NT(65), (NT(3)));
//G61:  __E_sym_20(65)       => '_'.
	p(NT(65), (T(19)));
//G62:  __E_sym_21(66)       => alnum(2).
	p(NT(66), (NT(2)));
//G63:  __E_sym_21(66)       => '_'.
	p(NT(66), (T(19)));
//G64:  __E_sym_22(67)       => null.
	p(NT(67), (nul));
//G65:  __E_sym_22(67)       => __E_sym_21(66) __E_sym_22(67).
	p(NT(67), (NT(66)+NT(67)));
//G66:  sym(10)              => __E_sym_20(65) __E_sym_22(67).
	p(NT(10), (NT(65)+NT(67)));
//G67:  __E_terminal_char_23(68) => unescaped_c(15).
	p(NT(68), (NT(15)));
//G68:  __E_terminal_char_23(68) => escaped_c(14).
	p(NT(68), (NT(14)));
//G69:  terminal_char(60)    => '\'' __E_terminal_char_23(68) '\''.
	p(NT(60), (T(20)+NT(68)+T(20)));
//G70:  __E_unescaped_c_24(69) => '\''.
	p(NT(69), (T(20)));
//G71:  __E_unescaped_c_24(69) => '\\'.
	p(NT(69), (T(21)));
//G72:  __N_0(124)           => __E_unescaped_c_24(69).
	p(NT(124), (NT(69)));
//G73:  unescaped_c(15)      => printable(5) & ~( __N_0(124) ).	 # conjunctive
	p(NT(15), (NT(5)) & ~(NT(124)));
//G74:  __E_escaped_c_25(70) => '\''.
	p(NT(70), (T(20)));
//G75:  __E_escaped_c_25(70) => escape_char(19).
	p(NT(70), (NT(19)));
//G76:  escaped_c(14)        => '\\' __E_escaped_c_25(70).
	p(NT(14), (T(21)+NT(70)));
//G77:  __E_terminal_string_26(71) => unescaped_s(13).
	p(NT(71), (NT(13)));
//G78:  __E_terminal_string_26(71) => escaped_s(12).
	p(NT(71), (NT(12)));
//G79:  __E_terminal_string_27(72) => null.
	p(NT(72), (nul));
//G80:  __E_terminal_string_27(72) => __E_terminal_string_26(71) __E_terminal_string_27(72).
	p(NT(72), (NT(71)+NT(72)));
//G81:  terminal_string(61)  => '"' __E_terminal_string_27(72) '"'.
	p(NT(61), (T(22)+NT(72)+T(22)));
//G82:  __E_unescaped_s_28(73) => '"'.
	p(NT(73), (T(22)));
//G83:  __E_unescaped_s_28(73) => '\\'.
	p(NT(73), (T(21)));
//G84:  __N_1(125)           => __E_unescaped_s_28(73).
	p(NT(125), (NT(73)));
//G85:  unescaped_s(13)      => printable(5) & ~( __N_1(125) ).	 # conjunctive
	p(NT(13), (NT(5)) & ~(NT(125)));
//G86:  __E_escaped_s_29(74) => '"'.
	p(NT(74), (T(22)));
//G87:  __E_escaped_s_29(74) => escape_char(19).
	p(NT(74), (NT(19)));
//G88:  escaped_s(12)        => '\\' __E_escaped_s_29(74).
	p(NT(12), (T(21)+NT(74)));
//G89:  escape_char(19)      => 'a'.
	p(NT(19), (T(23)));
//G90:  escape_char(19)      => 'b'.
	p(NT(19), (T(24)));
//G91:  escape_char(19)      => 'f'.
	p(NT(19), (T(25)));
//G92:  escape_char(19)      => 'n'.
	p(NT(19), (T(26)));
//G93:  escape_char(19)      => 'r'.
	p(NT(19), (T(27)));
//G94:  escape_char(19)      => 't'.
	p(NT(19), (T(28)));
//G95:  escape_char(19)      => 'v'.
	p(NT(19), (T(29)));
//G96:  escape_char(19)      => '\\'.
	p(NT(19), (T(21)));
//G97:  escape_char(19)      => '/'.
	p(NT(19), (T(30)));
//G98:  escape_char(19)      => esc_hex(20).
	p(NT(19), (NT(20)));
//G99:  escape_char(19)      => esc_u4(21).
	p(NT(19), (NT(21)));
//G100: escape_char(19)      => esc_U8(22).
	p(NT(19), (NT(22)));
//G101: __E_esc_hex_30(75)   => 'x'.
	p(NT(75), (T(18)));
//G102: __E_esc_hex_30(75)   => 'X'.
	p(NT(75), (T(31)));
//G103: __E_esc_hex_31(76)   => xdigit(6).
	p(NT(76), (NT(6)));
//G104: __E_esc_hex_31(76)   => null.
	p(NT(76), (nul));
//G105: esc_hex(20)          => __E_esc_hex_30(75) xdigit(6) __E_esc_hex_31(76).
	p(NT(20), (NT(75)+NT(6)+NT(76)));
//G106: esc_u4(21)           => 'u' xdigit(6) xdigit(6) xdigit(6) xdigit(6).
	p(NT(21), (T(32)+NT(6)+NT(6)+NT(6)+NT(6)));
//G107: esc_U8(22)           => 'U' xdigit(6) xdigit(6) xdigit(6) xdigit(6) xdigit(6) xdigit(6) xdigit(6) xdigit(6).
	p(NT(22), (T(33)+NT(6)+NT(6)+NT(6)+NT(6)+NT(6)+NT(6)+NT(6)+NT(6)));
//G108: directive(27)        => '@' _(8) directive_body(77) _(8) '.'.
	p(NT(27), (T(34)+NT(8)+NT(77)+NT(8)+T(5)));
//G109: __E_directive_body_32(79) => 's' 't' 'a' 'r' 't' __(7) sym(10).
	p(NT(79), (T(35)+T(28)+T(23)+T(27)+T(28)+NT(7)+NT(10)));
//G110: start_dir(78)        => __E_directive_body_32(79).
	p(NT(78), (NT(79)));
//G111: directive_body(77)   => start_dir(78).
	p(NT(77), (NT(78)));
//G112: __E___E_directive_body_33_34(83) => _(8) ',' _(8) inline_arg(82).
	p(NT(83), (NT(8)+T(36)+NT(8)+NT(82)));
//G113: __E___E_directive_body_33_35(84) => null.
	p(NT(84), (nul));
//G114: __E___E_directive_body_33_35(84) => __E___E_directive_body_33_34(83) __E___E_directive_body_33_35(84).
	p(NT(84), (NT(83)+NT(84)));
//G115: __E_directive_body_33(81) => 'i' 'n' 'l' 'i' 'n' 'e' __(7) inline_arg(82) __E___E_directive_body_33_35(84).
	p(NT(81), (T(37)+T(26)+T(38)+T(37)+T(26)+T(39)+NT(7)+NT(82)+NT(84)));
//G116: inline_dir(80)       => __E_directive_body_33(81).
	p(NT(80), (NT(81)));
//G117: directive_body(77)   => inline_dir(80).
	p(NT(77), (NT(80)));
//G118: __E_directive_body_36(86) => 't' 'r' 'i' 'm' sep(9) 'c' 'h' 'i' 'l' 'd' 'r' 'e' 'n' __(7) syms(18).
	p(NT(86), (T(28)+T(27)+T(37)+T(40)+NT(9)+T(41)+T(42)+T(37)+T(38)+T(43)+T(27)+T(39)+T(26)+NT(7)+NT(18)));
//G119: trim_children_dir(85) => __E_directive_body_36(86).
	p(NT(85), (NT(86)));
//G120: directive_body(77)   => trim_children_dir(85).
	p(NT(77), (NT(85)));
//G121: __E_directive_body_37(88) => 't' 'r' 'i' 'm' sep(9) 'c' 'h' 'i' 'l' 'd' 'r' 'e' 'n' sep(9) 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's' __(7) syms(18).
	p(NT(88), (T(28)+T(27)+T(37)+T(40)+NT(9)+T(41)+T(42)+T(37)+T(38)+T(43)+T(27)+T(39)+T(26)+NT(9)+T(28)+T(39)+T(27)+T(40)+T(37)+T(26)+T(23)+T(38)+T(35)+NT(7)+NT(18)));
//G122: trim_children_terminals_dir(87) => __E_directive_body_37(88).
	p(NT(87), (NT(88)));
//G123: directive_body(77)   => trim_children_terminals_dir(87).
	p(NT(77), (NT(87)));
//G124: __E___E___E_directive_body_38_39_40(92) => sep(9) 'c' 'h' 'i' 'l' 'd' 'r' 'e' 'n' sep(9) 'o' 'f'.
	p(NT(92), (NT(9)+T(41)+T(42)+T(37)+T(38)+T(43)+T(27)+T(39)+T(26)+NT(9)+T(44)+T(25)));
//G125: __E___E___E_directive_body_38_39_40(92) => null.
	p(NT(92), (nul));
//G126: __E___E_directive_body_38_39(91) => sep(9) 'e' 'x' 'c' 'e' 'p' 't' __E___E___E_directive_body_38_39_40(92) __(7) syms(18).
	p(NT(91), (NT(9)+T(39)+T(18)+T(41)+T(39)+T(45)+T(28)+NT(92)+NT(7)+NT(18)));
//G127: __E___E_directive_body_38_39(91) => null.
	p(NT(91), (nul));
//G128: __E_directive_body_38(90) => 't' 'r' 'i' 'm' sep(9) 'a' 'l' 'l' sep(9) 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's' __E___E_directive_body_38_39(91).
	p(NT(90), (T(28)+T(27)+T(37)+T(40)+NT(9)+T(23)+T(38)+T(38)+NT(9)+T(28)+T(39)+T(27)+T(40)+T(37)+T(26)+T(23)+T(38)+T(35)+NT(91)));
//G129: trim_all_terminals_dir(89) => __E_directive_body_38(90).
	p(NT(89), (NT(90)));
//G130: directive_body(77)   => trim_all_terminals_dir(89).
	p(NT(77), (NT(89)));
//G131: __E_directive_body_41(94) => 't' 'r' 'i' 'm' __(7) syms(18).
	p(NT(94), (T(28)+T(27)+T(37)+T(40)+NT(7)+NT(18)));
//G132: trim_dir(93)         => __E_directive_body_41(94).
	p(NT(93), (NT(94)));
//G133: directive_body(77)   => trim_dir(93).
	p(NT(77), (NT(93)));
//G134: use_from(97)         => cc_sym(98).
	p(NT(97), (NT(98)));
//G135: __E___E_directive_body_42_43(100) => _(8) ',' _(8) use_param(99).
	p(NT(100), (NT(8)+T(36)+NT(8)+NT(99)));
//G136: __E___E_directive_body_42_44(101) => null.
	p(NT(101), (nul));
//G137: __E___E_directive_body_42_44(101) => __E___E_directive_body_42_43(100) __E___E_directive_body_42_44(101).
	p(NT(101), (NT(100)+NT(101)));
//G138: __E_directive_body_42(96) => 'u' 's' 'e' __(7) use_from(97) __(7) use_param(99) __E___E_directive_body_42_44(101).
	p(NT(96), (T(32)+T(35)+T(39)+NT(7)+NT(97)+NT(7)+NT(99)+NT(101)));
//G139: use_dir(95)          => __E_directive_body_42(96).
	p(NT(95), (NT(96)));
//G140: directive_body(77)   => use_dir(95).
	p(NT(77), (NT(95)));
//G141: __E___E_directive_body_45_46(104) => 'a' 'u' 't' 'o' sep(9).
	p(NT(104), (T(23)+T(32)+T(28)+T(44)+NT(9)));
//G142: __E___E_directive_body_45_46(104) => null.
	p(NT(104), (nul));
//G143: __E___E_directive_body_45_47(105) => 'u' 'a' 't' 'i' 'o' 'n'.
	p(NT(105), (T(32)+T(23)+T(28)+T(37)+T(44)+T(26)));
//G144: __E___E_directive_body_45_47(105) => null.
	p(NT(105), (nul));
//G145: __E_directive_body_45(103) => 'd' 'i' 's' 'a' 'b' 'l' 'e' __(7) __E___E_directive_body_45_46(104) 'd' 'i' 's' 'a' 'm' 'b' 'i' 'g' __E___E_directive_body_45_47(105).
	p(NT(103), (T(43)+T(37)+T(35)+T(23)+T(24)+T(38)+T(39)+NT(7)+NT(104)+T(43)+T(37)+T(35)+T(23)+T(40)+T(24)+T(37)+T(46)+NT(105)));
//G146: disable_ad_dir(102)  => __E_directive_body_45(103).
	p(NT(102), (NT(103)));
//G147: directive_body(77)   => disable_ad_dir(102).
	p(NT(77), (NT(102)));
//G148: __E_directive_body_48(107) => 'e' 'n' 'a' 'b' 'l' 'e' __(7) 'p' 'r' 'o' 'd' 'u' 'c' 't' 'i' 'o' 'n' 's' __(7) syms(18).
	p(NT(107), (T(39)+T(26)+T(23)+T(24)+T(38)+T(39)+NT(7)+T(45)+T(27)+T(44)+T(43)+T(32)+T(41)+T(28)+T(37)+T(44)+T(26)+T(35)+NT(7)+NT(18)));
//G149: enable_prods_dir(106) => __E_directive_body_48(107).
	p(NT(106), (NT(107)));
//G150: directive_body(77)   => enable_prods_dir(106).
	p(NT(77), (NT(106)));
//G151: __E_directive_body_49(109) => 'a' 'm' 'b' 'i' 'g' 'u' 'o' 'u' 's' __(7) syms(18).
	p(NT(109), (T(23)+T(40)+T(24)+T(37)+T(46)+T(32)+T(44)+T(32)+T(35)+NT(7)+NT(18)));
//G152: ambiguous_dir(108)   => __E_directive_body_49(109).
	p(NT(108), (NT(109)));
//G153: directive_body(77)   => ambiguous_dir(108).
	p(NT(77), (NT(108)));
//G154: __E_syms_50(110)     => _(8) ',' _(8) sym(10).
	p(NT(110), (NT(8)+T(36)+NT(8)+NT(10)));
//G155: __E_syms_51(111)     => null.
	p(NT(111), (nul));
//G156: __E_syms_51(111)     => __E_syms_50(110) __E_syms_51(111).
	p(NT(111), (NT(110)+NT(111)));
//G157: syms(18)             => sym(10) __E_syms_51(111).
	p(NT(18), (NT(10)+NT(111)));
//G158: inline_arg(82)       => tree_path(112).
	p(NT(82), (NT(112)));
//G159: __E___E_inline_arg_52_53(114) => 'e' 's'.
	p(NT(114), (T(39)+T(35)));
//G160: __E___E_inline_arg_52_53(114) => null.
	p(NT(114), (nul));
//G161: __E_inline_arg_52(113) => 'c' 'h' 'a' 'r' sep(9) 'c' 'l' 'a' 's' 's' __E___E_inline_arg_52_53(114).
	p(NT(113), (T(41)+T(42)+T(23)+T(27)+NT(9)+T(41)+T(38)+T(23)+T(35)+T(35)+NT(114)));
//G162: cc_sym(98)           => __E_inline_arg_52(113).
	p(NT(98), (NT(113)));
//G163: inline_arg(82)       => cc_sym(98).
	p(NT(82), (NT(98)));
//G164: __E_tree_path_54(115) => _(8) '>' _(8) sym(10).
	p(NT(115), (NT(8)+T(4)+NT(8)+NT(10)));
//G165: __E_tree_path_55(116) => null.
	p(NT(116), (nul));
//G166: __E_tree_path_55(116) => __E_tree_path_54(115) __E_tree_path_55(116).
	p(NT(116), (NT(115)+NT(116)));
//G167: tree_path(112)       => sym(10) __E_tree_path_55(116).
	p(NT(112), (NT(10)+NT(116)));
//G168: __E_use_param_56(117) => 'e' 'o' 'f'.
	p(NT(117), (T(39)+T(44)+T(25)));
//G169: __E_use_param_56(117) => 'a' 'n' 'y'.
	p(NT(117), (T(23)+T(26)+T(47)));
//G170: __E_use_param_56(117) => 'a' 's' 'c' 'i' 'i'.
	p(NT(117), (T(23)+T(35)+T(41)+T(37)+T(37)));
//G171: __E_use_param_56(117) => 'a' 'l' 'n' 'u' 'm'.
	p(NT(117), (T(23)+T(38)+T(26)+T(32)+T(40)));
//G172: __E_use_param_56(117) => 'a' 'l' 'p' 'h' 'a'.
	p(NT(117), (T(23)+T(38)+T(45)+T(42)+T(23)));
//G173: __E_use_param_56(117) => 'b' 'l' 'a' 'n' 'k'.
	p(NT(117), (T(24)+T(38)+T(23)+T(26)+T(48)));
//G174: __E_use_param_56(117) => 'c' 'n' 't' 'r' 'l'.
	p(NT(117), (T(41)+T(26)+T(28)+T(27)+T(38)));
//G175: __E_use_param_56(117) => 'd' 'i' 'g' 'i' 't'.
	p(NT(117), (T(43)+T(37)+T(46)+T(37)+T(28)));
//G176: __E_use_param_56(117) => 'g' 'r' 'a' 'p' 'h'.
	p(NT(117), (T(46)+T(27)+T(23)+T(45)+T(42)));
//G177: __E_use_param_56(117) => 'l' 'o' 'w' 'e' 'r'.
	p(NT(117), (T(38)+T(44)+T(49)+T(39)+T(27)));
//G178: __E_use_param_56(117) => 'p' 'r' 'i' 'n' 't' 'a' 'b' 'l' 'e'.
	p(NT(117), (T(45)+T(27)+T(37)+T(26)+T(28)+T(23)+T(24)+T(38)+T(39)));
//G179: __E_use_param_56(117) => 'p' 'u' 'n' 'c' 't'.
	p(NT(117), (T(45)+T(32)+T(26)+T(41)+T(28)));
//G180: __E_use_param_56(117) => 's' 'p' 'a' 'c' 'e'.
	p(NT(117), (T(35)+T(45)+T(23)+T(41)+T(39)));
//G181: __E_use_param_56(117) => 'u' 'p' 'p' 'e' 'r'.
	p(NT(117), (T(32)+T(45)+T(45)+T(39)+T(27)));
//G182: __E_use_param_56(117) => 'x' 'd' 'i' 'g' 'i' 't'.
	p(NT(117), (T(18)+T(43)+T(37)+T(46)+T(37)+T(28)));
//G183: cc_name(11)          => __E_use_param_56(117).
	p(NT(11), (NT(117)));
//G184: use_param(99)        => cc_name(11).
	p(NT(99), (NT(11)));
//G185: __E_sep_57(118)      => sep_required(119).
	p(NT(118), (NT(119)));
//G186: __E_sep_57(118)      => null.
	p(NT(118), (nul));
//G187: sep(9)               => __E_sep_57(118).
	p(NT(9), (NT(118)));
//G188: sep_required(119)    => '-'.
	p(NT(119), (T(50)));
//G189: sep_required(119)    => '_'.
	p(NT(119), (T(19)));
//G190: sep_required(119)    => __(7).
	p(NT(119), (NT(7)));
//G191: _(8)                 => __(7).
	p(NT(8), (NT(7)));
//G192: _(8)                 => null.
	p(NT(8), (nul));
//G193: __(7)                => space(4).
	p(NT(7), (NT(4)));
//G194: __(7)                => comment(120).
	p(NT(7), (NT(120)));
//G195: __(7)                => __(7) space(4).
	p(NT(7), (NT(7)+NT(4)));
//G196: __(7)                => __(7) comment(120).
	p(NT(7), (NT(7)+NT(120)));
//G197: __E_comment_58(121)  => printable(5).
	p(NT(121), (NT(5)));
//G198: __E_comment_58(121)  => '\t'.
	p(NT(121), (T(51)));
//G199: __E_comment_59(122)  => null.
	p(NT(122), (nul));
//G200: __E_comment_59(122)  => __E_comment_58(121) __E_comment_59(122).
	p(NT(122), (NT(121)+NT(122)));
//G201: __E_comment_60(123)  => '\r'.
	p(NT(123), (T(52)));
//G202: __E_comment_60(123)  => '\n'.
	p(NT(123), (T(53)));
//G203: __E_comment_60(123)  => eof(1).
	p(NT(123), (NT(1)));
//G204: comment(120)         => '#' __E_comment_59(122) __E_comment_60(123).
	p(NT(120), (T(54)+NT(122)+NT(123)));
	#undef T
	#undef NT
	return loaded = true, p;
}

inline ::idni::grammar<char_type, terminal_type> grammar(
	nts, productions(), start_symbol, char_classes, grammar_options);

} // namespace tgf_parser_data

struct tgf_parser_nonterminals {
	enum nonterminal {
		nul, eof, alnum, alpha, space, printable, xdigit, __, _, sep, 
		sym, cc_name, escaped_s, unescaped_s, escaped_c, unescaped_c, terminal_hex, hex_bytes, syms, escape_char, 
		esc_hex, esc_u4, esc_U8, start, __E_start_0, statement, __E_start_1, directive, production, start_statement, 
		__E_production_2, production_guard, alternation, conjunction, __E_alternation_3, __E_alternation_4, concatenation, __E_conjunction_5, __E_conjunction_6, factor, 
		__E_concatenation_7, __E_concatenation_8, shorthand_rule, __E_factor_9, optional, __E_factor_10, term, repeat, __E_factor_11, none_or_repeat, 
		__E_factor_12, neg, __E_factor_13, group, __E_term_14, optional_group, __E_term_15, repeat_group, __E_term_16, terminal, 
		terminal_char, terminal_string, __E_terminal_hex_17, __E___E_terminal_hex_17_18, __E___E_terminal_hex_17_19, __E_sym_20, __E_sym_21, __E_sym_22, __E_terminal_char_23, __E_unescaped_c_24, 
		__E_escaped_c_25, __E_terminal_string_26, __E_terminal_string_27, __E_unescaped_s_28, __E_escaped_s_29, __E_esc_hex_30, __E_esc_hex_31, directive_body, start_dir, __E_directive_body_32, 
		inline_dir, __E_directive_body_33, inline_arg, __E___E_directive_body_33_34, __E___E_directive_body_33_35, trim_children_dir, __E_directive_body_36, trim_children_terminals_dir, __E_directive_body_37, trim_all_terminals_dir, 
		__E_directive_body_38, __E___E_directive_body_38_39, __E___E___E_directive_body_38_39_40, trim_dir, __E_directive_body_41, use_dir, __E_directive_body_42, use_from, cc_sym, use_param, 
		__E___E_directive_body_42_43, __E___E_directive_body_42_44, disable_ad_dir, __E_directive_body_45, __E___E_directive_body_45_46, __E___E_directive_body_45_47, enable_prods_dir, __E_directive_body_48, ambiguous_dir, __E_directive_body_49, 
		__E_syms_50, __E_syms_51, tree_path, __E_inline_arg_52, __E___E_inline_arg_52_53, __E_tree_path_54, __E_tree_path_55, __E_use_param_56, __E_sep_57, sep_required, 
		comment, __E_comment_58, __E_comment_59, __E_comment_60, __N_0, __N_1, 
	};
};

struct tgf_parser : public idni::parser<char, char32_t>, public tgf_parser_nonterminals {
	static tgf_parser& instance() {
		static tgf_parser inst;
		return inst;
	}
	tgf_parser() : idni::parser<char_type, terminal_type>(
		tgf_parser_data::grammar,
		tgf_parser_data::make_parser_options()) {}
	size_t id(const std::basic_string<char_type>& name) {
		return tgf_parser_data::nts.get(name);
	}
	const std::basic_string<char_type>& name(size_t id) {
		return tgf_parser_data::nts.get(id);
	}
	symbol_type literal(const nonterminal& nt) {
		return symbol_type(nt, &tgf_parser_data::nts);
	}
};

#endif // __TGF_PARSER_H__
