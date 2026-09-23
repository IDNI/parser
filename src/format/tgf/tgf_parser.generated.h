// This file is generated from a file src/format/tgf/tgf.tgf by
//       https://github.com/IDNI/parser/src/tgf
//
#ifndef __TGF_PARSER_H__
#define __TGF_PARSER_H__

#include "parser.h"

namespace tgf_parser_data {

using char_type     = char;
using terminal_type = char;

inline static constexpr size_t nt_bits = 7;
inline const std::vector<std::string> symbol_names{
	"", "eof", "alnum", "alpha", "space", "printable", "xdigit", "__", "_", "sep", 
	"sym", "dir_sym", "cc_name", "escaped_s", "unescaped_s", "escaped_c", "unescaped_c", "terminal_hex", "hex_bytes", "escape_char", 
	"esc_hex", "esc_u4", "esc_U8", "start", "__E_start_0", "statement", "__E_start_1", "directive", "production", "start_statement", 
	"__E_production_2", "production_guard", "alternation", "conjunction", "__E_alternation_3", "__E_alternation_4", "concatenation", "__E_conjunction_5", "__E_conjunction_6", "factor", 
	"__E_concatenation_7", "__E_concatenation_8", "shorthand_rule", "__E_factor_9", "optional", "__E_factor_10", "term", "repeat", "__E_factor_11", "none_or_repeat", 
	"__E_factor_12", "neg", "__E_factor_13", "group", "__E_term_14", "optional_group", "__E_term_15", "repeat_group", "__E_term_16", "terminal", 
	"terminal_char", "terminal_string", "__E_terminal_hex_17", "__E___E_terminal_hex_17_18", "__E___E_terminal_hex_17_19", "__E_sym_20", "__E_sym_21", "__E_sym_22", "__E_terminal_char_23", "__E_unescaped_c_24", 
	"__E_escaped_c_25", "__E_terminal_string_26", "__E_terminal_string_27", "__E_unescaped_s_28", "__E_escaped_s_29", "__E_esc_hex_30", "__E_esc_hex_31", "directive_token", "__E_directive_32", "directive_name", 
	"__E_directive_33", "directive_cmd", "__E___E_directive_33_34", "__E___E___E_directive_33_34_35", "__E___E___E_directive_33_34_36", "__E_directive_37", "__E_directive_38", "dir_pair", "__E___E_directive_38_39", "__E___E_directive_38_40", 
	"dir_list", "__E_dir_pair_41", "dir_arg", "__E_dir_list_42", "__E_dir_list_43", "tree_path", "treemr_pattern", "__E_dir_arg_44", "pat_word", "__E___E_dir_arg_44_45", 
	"pat_space", "__E___E___E_dir_arg_44_45_46", "__E___E_dir_arg_44_47", "__E_tree_path_48", "__E_tree_path_49", "__E_dir_sym_50", "__E_dir_sym_51", "__E_dir_sym_52", "pat_unit", "__E_treemr_pattern_53", 
	"__E___E_treemr_pattern_53_54", "__E_treemr_pattern_55", "pat_char", "__E_pat_char_56", "comment", "__E_comment_57", "__E_comment_58", "__E_comment_59", "__N_0", "__N_1", 
	"__N_2", "__N_3", 
};

inline ::idni::nonterminals<char_type, terminal_type> nts{symbol_names};

inline std::vector<terminal_type> terminals{
	'\0', '[', ']', '=', '>', '.', '|', '&', ':', 
	'?', '+', '*', '~', '(', ')', '{', '}', '0', 'x', 
	'_', '\'', '\\', '"', 'a', 'b', 'f', 'n', 'r', 't', 
	'v', '/', 'X', 'u', 'U', '@', ';', '-', ',', '#', 
	' ', '\t', '\r', '\n', 
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
			10, 11, 12, 13, 14, 15, 16, 17, 18
		},
		.to_inline = {
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
	return o;
}

inline ::idni::prods<char_type, terminal_type> start_symbol{ nts(23) };

#ifdef TAU_PARSER_BUILD_HEADER_ONLY
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
//G2:   __E_start_1(26)      => __E_start_1(26) __E_start_0(24).
	p(NT(26), (NT(26)+NT(24)));
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
//G13:  __E_alternation_4(35) => __E_alternation_4(35) __E_alternation_3(34).
	p(NT(35), (NT(35)+NT(34)));
//G14:  alternation(32)      => conjunction(33) __E_alternation_4(35).
	p(NT(32), (NT(33)+NT(35)));
//G15:  __E_conjunction_5(37) => _(8) '&' _(8) concatenation(36).
	p(NT(37), (NT(8)+T(7)+NT(8)+NT(36)));
//G16:  __E_conjunction_6(38) => null.
	p(NT(38), (nul));
//G17:  __E_conjunction_6(38) => __E_conjunction_6(38) __E_conjunction_5(37).
	p(NT(38), (NT(38)+NT(37)));
//G18:  conjunction(33)      => concatenation(36) __E_conjunction_6(38).
	p(NT(33), (NT(36)+NT(38)));
//G19:  __E_concatenation_7(40) => __(7) factor(39).
	p(NT(40), (NT(7)+NT(39)));
//G20:  __E_concatenation_8(41) => null.
	p(NT(41), (nul));
//G21:  __E_concatenation_8(41) => __E_concatenation_8(41) __E_concatenation_7(40).
	p(NT(41), (NT(41)+NT(40)));
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
//G52:  terminal(59)         => terminal_hex(17).
	p(NT(59), (NT(17)));
//G53:  __E_terminal_hex_17(62) => xdigit(6).
	p(NT(62), (NT(6)));
//G54:  __E___E_terminal_hex_17_18(63) => xdigit(6) xdigit(6).
	p(NT(63), (NT(6)+NT(6)));
//G55:  __E___E_terminal_hex_17_19(64) => __E___E_terminal_hex_17_18(63).
	p(NT(64), (NT(63)));
//G56:  __E___E_terminal_hex_17_19(64) => __E___E_terminal_hex_17_19(64) __E___E_terminal_hex_17_18(63).
	p(NT(64), (NT(64)+NT(63)));
//G57:  __E_terminal_hex_17(62) => __E___E_terminal_hex_17_19(64).
	p(NT(62), (NT(64)));
//G58:  hex_bytes(18)        => __E_terminal_hex_17(62).
	p(NT(18), (NT(62)));
//G59:  terminal_hex(17)     => '0' 'x' hex_bytes(18).
	p(NT(17), (T(17)+T(18)+NT(18)));
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
//G65:  __E_sym_22(67)       => __E_sym_22(67) __E_sym_21(66).
	p(NT(67), (NT(67)+NT(66)));
//G66:  sym(10)              => __E_sym_20(65) __E_sym_22(67).
	p(NT(10), (NT(65)+NT(67)));
//G67:  __E_terminal_char_23(68) => unescaped_c(16).
	p(NT(68), (NT(16)));
//G68:  __E_terminal_char_23(68) => escaped_c(15).
	p(NT(68), (NT(15)));
//G69:  terminal_char(60)    => '\'' __E_terminal_char_23(68) '\''.
	p(NT(60), (T(20)+NT(68)+T(20)));
//G70:  __E_unescaped_c_24(69) => '\''.
	p(NT(69), (T(20)));
//G71:  __E_unescaped_c_24(69) => '\\'.
	p(NT(69), (T(21)));
//G72:  __N_0(118)           => __E_unescaped_c_24(69).
	p(NT(118), (NT(69)));
//G73:  unescaped_c(16)      => printable(5) & ~( __N_0(118) ).	 # conjunctive
	p(NT(16), (NT(5)) & ~(NT(118)));
//G74:  __E_escaped_c_25(70) => '\''.
	p(NT(70), (T(20)));
//G75:  __E_escaped_c_25(70) => escape_char(19).
	p(NT(70), (NT(19)));
//G76:  escaped_c(15)        => '\\' __E_escaped_c_25(70).
	p(NT(15), (T(21)+NT(70)));
//G77:  __E_terminal_string_26(71) => unescaped_s(14).
	p(NT(71), (NT(14)));
//G78:  __E_terminal_string_26(71) => escaped_s(13).
	p(NT(71), (NT(13)));
//G79:  __E_terminal_string_27(72) => null.
	p(NT(72), (nul));
//G80:  __E_terminal_string_27(72) => __E_terminal_string_27(72) __E_terminal_string_26(71).
	p(NT(72), (NT(72)+NT(71)));
//G81:  terminal_string(61)  => '"' __E_terminal_string_27(72) '"'.
	p(NT(61), (T(22)+NT(72)+T(22)));
//G82:  __E_unescaped_s_28(73) => '"'.
	p(NT(73), (T(22)));
//G83:  __E_unescaped_s_28(73) => '\\'.
	p(NT(73), (T(21)));
//G84:  __N_1(119)           => __E_unescaped_s_28(73).
	p(NT(119), (NT(73)));
//G85:  unescaped_s(14)      => printable(5) & ~( __N_1(119) ).	 # conjunctive
	p(NT(14), (NT(5)) & ~(NT(119)));
//G86:  __E_escaped_s_29(74) => '"'.
	p(NT(74), (T(22)));
//G87:  __E_escaped_s_29(74) => escape_char(19).
	p(NT(74), (NT(19)));
//G88:  escaped_s(13)        => '\\' __E_escaped_s_29(74).
	p(NT(13), (T(21)+NT(74)));
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
//G108: directive_name(79)   => sym(10).
	p(NT(79), (NT(10)));
//G109: __E_directive_32(78) => '@' _(8) directive_name(79) _(8).
	p(NT(78), (T(34)+NT(8)+NT(79)+NT(8)));
//G110: directive_token(77)  => __E_directive_32(78).
	p(NT(77), (NT(78)));
//G111: __E___E___E_directive_33_34_35(83) => sep(9) sym(10).
	p(NT(83), (NT(9)+NT(10)));
//G112: __E___E___E_directive_33_34_36(84) => null.
	p(NT(84), (nul));
//G113: __E___E___E_directive_33_34_36(84) => __E___E___E_directive_33_34_36(84) __E___E___E_directive_33_34_35(83).
	p(NT(84), (NT(84)+NT(83)));
//G114: __E___E_directive_33_34(82) => sep(9) sym(10) __E___E___E_directive_33_34_36(84).
	p(NT(82), (NT(9)+NT(10)+NT(84)));
//G115: directive_cmd(81)    => __E___E_directive_33_34(82).
	p(NT(81), (NT(82)));
//G116: __E_directive_33(80) => directive_cmd(81).
	p(NT(80), (NT(81)));
//G117: __E_directive_37(85) => null.
	p(NT(85), (nul));
//G118: __E_directive_37(85) => __E_directive_37(85) __E_directive_33(80).
	p(NT(85), (NT(85)+NT(80)));
//G119: __E___E_directive_38_39(88) => _(8) ';' _(8) dir_pair(87).
	p(NT(88), (NT(8)+T(35)+NT(8)+NT(87)));
//G120: __E___E_directive_38_40(89) => null.
	p(NT(89), (nul));
//G121: __E___E_directive_38_40(89) => __E___E_directive_38_40(89) __E___E_directive_38_39(88).
	p(NT(89), (NT(89)+NT(88)));
//G122: __E_directive_38(86) => __(7) dir_pair(87) __E___E_directive_38_40(89).
	p(NT(86), (NT(7)+NT(87)+NT(89)));
//G123: __E_directive_38(86) => null.
	p(NT(86), (nul));
//G124: directive(27)        => directive_token(77) __E_directive_37(85) __E_directive_38(86) _(8) '.'.
	p(NT(27), (NT(77)+NT(85)+NT(86)+NT(8)+T(5)));
//G125: sep(9)               => '-'.
	p(NT(9), (T(36)));
//G126: sep(9)               => '_'.
	p(NT(9), (T(19)));
//G127: sep(9)               => __(7).
	p(NT(9), (NT(7)));
//G128: __E_dir_pair_41(91)  => _(8) ':' _(8) dir_list(90).
	p(NT(91), (NT(8)+T(8)+NT(8)+NT(90)));
//G129: __E_dir_pair_41(91)  => null.
	p(NT(91), (nul));
//G130: dir_pair(87)         => dir_list(90) __E_dir_pair_41(91).
	p(NT(87), (NT(90)+NT(91)));
//G131: __E_dir_list_42(93)  => _(8) ',' _(8) dir_arg(92).
	p(NT(93), (NT(8)+T(37)+NT(8)+NT(92)));
//G132: __E_dir_list_43(94)  => null.
	p(NT(94), (nul));
//G133: __E_dir_list_43(94)  => __E_dir_list_43(94) __E_dir_list_42(93).
	p(NT(94), (NT(94)+NT(93)));
//G134: dir_list(90)         => dir_arg(92) __E_dir_list_43(94).
	p(NT(90), (NT(92)+NT(94)));
//G135: dir_arg(92)          => tree_path(95).
	p(NT(92), (NT(95)));
//G136: dir_arg(92)          => dir_sym(11).
	p(NT(92), (NT(11)));
//G137: dir_arg(92)          => terminal_string(61).
	p(NT(92), (NT(61)));
//G138: __E___E___E_dir_arg_44_45_46(101) => pat_space(100).
	p(NT(101), (NT(100)));
//G139: __E___E___E_dir_arg_44_45_46(101) => __E___E___E_dir_arg_44_45_46(101) pat_space(100).
	p(NT(101), (NT(101)+NT(100)));
//G140: __E___E_dir_arg_44_45(99) => __E___E___E_dir_arg_44_45_46(101) pat_word(98).
	p(NT(99), (NT(101)+NT(98)));
//G141: __E___E_dir_arg_44_47(102) => __E___E_dir_arg_44_45(99).
	p(NT(102), (NT(99)));
//G142: __E___E_dir_arg_44_47(102) => __E___E_dir_arg_44_47(102) __E___E_dir_arg_44_45(99).
	p(NT(102), (NT(102)+NT(99)));
//G143: __E_dir_arg_44(97)   => pat_word(98) __E___E_dir_arg_44_47(102).
	p(NT(97), (NT(98)+NT(102)));
//G144: __N_2(120)           => __E_dir_arg_44(97).
	p(NT(120), (NT(97)));
//G145: dir_arg(92)          => treemr_pattern(96) & ~( __N_2(120) ).	 # conjunctive
	p(NT(92), (NT(96)) & ~(NT(120)));
//G146: pat_word(98)         => dir_sym(11).
	p(NT(98), (NT(11)));
//G147: pat_word(98)         => terminal_string(61).
	p(NT(98), (NT(61)));
//G148: __E_tree_path_48(103) => _(8) '>' _(8) dir_sym(11).
	p(NT(103), (NT(8)+T(4)+NT(8)+NT(11)));
//G149: __E_tree_path_49(104) => null.
	p(NT(104), (nul));
//G150: __E_tree_path_49(104) => __E_tree_path_49(104) __E_tree_path_48(103).
	p(NT(104), (NT(104)+NT(103)));
//G151: tree_path(95)        => dir_sym(11) _(8) '>' _(8) dir_sym(11) __E_tree_path_49(104).
	p(NT(95), (NT(11)+NT(8)+T(4)+NT(8)+NT(11)+NT(104)));
//G152: __E_dir_sym_50(105)  => alpha(3).
	p(NT(105), (NT(3)));
//G153: __E_dir_sym_50(105)  => '_'.
	p(NT(105), (T(19)));
//G154: __E_dir_sym_50(105)  => '*'.
	p(NT(105), (T(11)));
//G155: __E_dir_sym_51(106)  => alnum(2).
	p(NT(106), (NT(2)));
//G156: __E_dir_sym_51(106)  => '_'.
	p(NT(106), (T(19)));
//G157: __E_dir_sym_51(106)  => '*'.
	p(NT(106), (T(11)));
//G158: __E_dir_sym_52(107)  => null.
	p(NT(107), (nul));
//G159: __E_dir_sym_52(107)  => __E_dir_sym_52(107) __E_dir_sym_51(106).
	p(NT(107), (NT(107)+NT(106)));
//G160: dir_sym(11)          => __E_dir_sym_50(105) __E_dir_sym_52(107).
	p(NT(11), (NT(105)+NT(107)));
//G161: __E___E_treemr_pattern_53_54(110) => null.
	p(NT(110), (nul));
//G162: __E___E_treemr_pattern_53_54(110) => __E___E_treemr_pattern_53_54(110) pat_space(100).
	p(NT(110), (NT(110)+NT(100)));
//G163: __E_treemr_pattern_53(109) => __E___E_treemr_pattern_53_54(110) pat_unit(108).
	p(NT(109), (NT(110)+NT(108)));
//G164: __E_treemr_pattern_55(111) => null.
	p(NT(111), (nul));
//G165: __E_treemr_pattern_55(111) => __E_treemr_pattern_55(111) __E_treemr_pattern_53(109).
	p(NT(111), (NT(111)+NT(109)));
//G166: treemr_pattern(96)   => pat_unit(108) __E_treemr_pattern_55(111).
	p(NT(96), (NT(108)+NT(111)));
//G167: pat_unit(108)        => terminal_string(61).
	p(NT(108), (NT(61)));
//G168: pat_unit(108)        => terminal_char(60).
	p(NT(108), (NT(60)));
//G169: pat_unit(108)        => pat_char(112).
	p(NT(108), (NT(112)));
//G170: __E_pat_char_56(113) => space(4).
	p(NT(113), (NT(4)));
//G171: __E_pat_char_56(113) => ','.
	p(NT(113), (T(37)));
//G172: __E_pat_char_56(113) => '.'.
	p(NT(113), (T(5)));
//G173: __E_pat_char_56(113) => ':'.
	p(NT(113), (T(8)));
//G174: __E_pat_char_56(113) => ';'.
	p(NT(113), (T(35)));
//G175: __E_pat_char_56(113) => '#'.
	p(NT(113), (T(38)));
//G176: __E_pat_char_56(113) => '"'.
	p(NT(113), (T(22)));
//G177: __E_pat_char_56(113) => '\''.
	p(NT(113), (T(20)));
//G178: __N_3(121)           => __E_pat_char_56(113).
	p(NT(121), (NT(113)));
//G179: pat_char(112)        => printable(5) & ~( __N_3(121) ).	 # conjunctive
	p(NT(112), (NT(5)) & ~(NT(121)));
//G180: pat_space(100)       => ' '.
	p(NT(100), (T(39)));
//G181: pat_space(100)       => '\t'.
	p(NT(100), (T(40)));
//G182: _(8)                 => __(7).
	p(NT(8), (NT(7)));
//G183: _(8)                 => null.
	p(NT(8), (nul));
//G184: __(7)                => space(4).
	p(NT(7), (NT(4)));
//G185: __(7)                => comment(114).
	p(NT(7), (NT(114)));
//G186: __(7)                => __(7) space(4).
	p(NT(7), (NT(7)+NT(4)));
//G187: __(7)                => __(7) comment(114).
	p(NT(7), (NT(7)+NT(114)));
//G188: __E_comment_57(115)  => printable(5).
	p(NT(115), (NT(5)));
//G189: __E_comment_57(115)  => '\t'.
	p(NT(115), (T(40)));
//G190: __E_comment_58(116)  => null.
	p(NT(116), (nul));
//G191: __E_comment_58(116)  => __E_comment_58(116) __E_comment_57(115).
	p(NT(116), (NT(116)+NT(115)));
//G192: __E_comment_59(117)  => '\r'.
	p(NT(117), (T(41)));
//G193: __E_comment_59(117)  => '\n'.
	p(NT(117), (T(42)));
//G194: __E_comment_59(117)  => eof(1).
	p(NT(117), (NT(1)));
//G195: comment(114)         => '#' __E_comment_58(116) __E_comment_59(117).
	p(NT(114), (T(38)+NT(116)+NT(117)));
	#undef T
	#undef NT
	return loaded = true, p;
}
#else
idni::prods<char_type, terminal_type>& productions();
#endif

inline ::idni::grammar<char_type, terminal_type> grammar(
	nts, productions(), start_symbol, char_classes, grammar_options);

} // namespace tgf_parser_data

struct tgf_parser_nonterminals {
	enum nonterminal {
		nul, eof, alnum, alpha, space, printable, xdigit, __, _, sep, 
		sym, dir_sym, cc_name, escaped_s, unescaped_s, escaped_c, unescaped_c, terminal_hex, hex_bytes, escape_char, 
		esc_hex, esc_u4, esc_U8, start, __E_start_0, statement, __E_start_1, directive, production, start_statement, 
		__E_production_2, production_guard, alternation, conjunction, __E_alternation_3, __E_alternation_4, concatenation, __E_conjunction_5, __E_conjunction_6, factor, 
		__E_concatenation_7, __E_concatenation_8, shorthand_rule, __E_factor_9, optional, __E_factor_10, term, repeat, __E_factor_11, none_or_repeat, 
		__E_factor_12, neg, __E_factor_13, group, __E_term_14, optional_group, __E_term_15, repeat_group, __E_term_16, terminal, 
		terminal_char, terminal_string, __E_terminal_hex_17, __E___E_terminal_hex_17_18, __E___E_terminal_hex_17_19, __E_sym_20, __E_sym_21, __E_sym_22, __E_terminal_char_23, __E_unescaped_c_24, 
		__E_escaped_c_25, __E_terminal_string_26, __E_terminal_string_27, __E_unescaped_s_28, __E_escaped_s_29, __E_esc_hex_30, __E_esc_hex_31, directive_token, __E_directive_32, directive_name, 
		__E_directive_33, directive_cmd, __E___E_directive_33_34, __E___E___E_directive_33_34_35, __E___E___E_directive_33_34_36, __E_directive_37, __E_directive_38, dir_pair, __E___E_directive_38_39, __E___E_directive_38_40, 
		dir_list, __E_dir_pair_41, dir_arg, __E_dir_list_42, __E_dir_list_43, tree_path, treemr_pattern, __E_dir_arg_44, pat_word, __E___E_dir_arg_44_45, 
		pat_space, __E___E___E_dir_arg_44_45_46, __E___E_dir_arg_44_47, __E_tree_path_48, __E_tree_path_49, __E_dir_sym_50, __E_dir_sym_51, __E_dir_sym_52, pat_unit, __E_treemr_pattern_53, 
		__E___E_treemr_pattern_53_54, __E_treemr_pattern_55, pat_char, __E_pat_char_56, comment, __E_comment_57, __E_comment_58, __E_comment_59, __N_0, __N_1, 
		__N_2, __N_3, 
	};
};

struct tgf_parser : public idni::parser<char, char>, public tgf_parser_nonterminals {
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
