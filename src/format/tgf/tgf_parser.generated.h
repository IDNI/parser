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
	"", "eof", "alnum", "alpha", "space", "printable", "xdigit", "start", "__E_start_0", "_", 
	"statement", "__E_start_1", "directive", "production", "start_statement", "sym", "__E_production_2", "production_guard", "alternation", "conjunction", 
	"__E_alternation_3", "__E_alternation_4", "concatenation", "__E_conjunction_5", "__E_conjunction_6", "factor", "__E_concatenation_7", "__", "__E_concatenation_8", "shorthand_rule", 
	"__E_factor_9", "optional", "__E_factor_10", "term", "repeat", "__E_factor_11", "none_or_repeat", "__E_factor_12", "neg", "__E_factor_13", 
	"group", "__E_term_14", "optional_group", "__E_term_15", "repeat_group", "__E_term_16", "terminal", "terminal_char", "terminal_string", "terminal_hex", 
	"hex_bytes", "__E_terminal_hex_17", "__E___E_terminal_hex_17_18", "__E___E_terminal_hex_17_19", "__E_sym_20", "__E_sym_21", "__E_sym_22", "__E_terminal_char_23", "unescaped_c", "escaped_c", 
	"__E_unescaped_c_24", "__E_escaped_c_25", "escape_char", "__E_terminal_string_26", "unescaped_s", "escaped_s", "__E_terminal_string_27", "__E_unescaped_s_28", "__E_escaped_s_29", "esc_hex", 
	"esc_u4", "esc_U8", "__E_esc_hex_30", "__E_esc_hex_31", "directive_token", "__E_directive_32", "directive_name", "__E_directive_33", "directive_cmd", "__E___E_directive_33_34", 
	"sep", "__E___E___E_directive_33_34_35", "__E___E___E_directive_33_34_36", "__E_directive_37", "__E_directive_38", "dir_pair", "__E___E_directive_38_39", "__E___E_directive_38_40", "dir_list", "__E_dir_pair_41", 
	"dir_arg", "__E_dir_list_42", "__E_dir_list_43", "dir_alias", "tree_path", "dir_sym", "treemr_pattern", "__E_dir_arg_44", "pat_word", "__E___E_dir_arg_44_45", 
	"pat_space", "__E___E___E_dir_arg_44_45_46", "__E___E_dir_arg_44_47", "__E_tree_path_48", "__E_tree_path_49", "__E_dir_sym_50", "__E_dir_sym_51", "__E_dir_sym_52", "pat_unit", "__E_treemr_pattern_53", 
	"__E___E_treemr_pattern_53_54", "__E_treemr_pattern_55", "pat_char", "__E_pat_char_56", "comment", "__E_comment_57", "__E_comment_58", "__E_comment_59", "__N_0", "__N_1", 
	"__N_2", "__N_3", 
};

inline ::idni::nonterminals<char_type, terminal_type> nts{symbol_names};

inline std::vector<terminal_type> terminals{
	'\0', '[', ']', '=', '>', '.', '|', '&', ':', 
	'?', '+', '*', '~', '(', ')', '{', '}', '0', 'x', 
	'_', '\'', '\\', '"', 'a', 'b', 'f', 'n', 'r', 't', 
	'v', '/', 'X', 'u', 'U', '@', ';', '-', ',', 's', 
	'#', ' ', '\t', '\r', '\n', 
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
			9, 27, 80
		},
		.trim_terminals = true,
		.dont_trim_terminals_of = {
			15, 49, 50, 58, 59, 64, 65, 95
		},
		.to_inline = {
			{ 62 },
			{ 69 },
			{ 70 },
			{ 71 }
		},
		.inline_char_classes = true
	}
};

inline auto make_parser_options() {
	auto o = ::idni::default_parser_options<char_type, terminal_type>();
	return o;
}

inline ::idni::prods<char_type, terminal_type> start_symbol{ nts(7) };

#ifdef TAU_PARSER_BUILD_HEADER_ONLY
inline idni::prods<char_type, terminal_type>& productions() {
	static bool loaded = false;
	static idni::prods<char_type, terminal_type>
		p, nul(idni::lit<char_type, terminal_type>{});
	if (loaded) return p;
	#define  T(x) (idni::prods<char_type, terminal_type>{ terminals[x] })
	#define NT(x) (idni::prods<char_type, terminal_type>{ nts(x) })
//G0:   __E_start_0(8)       => _(9) statement(10).
	p(NT(8), (NT(9)+NT(10)));
//G1:   __E_start_1(11)      => null.
	p(NT(11), (nul));
//G2:   __E_start_1(11)      => __E_start_1(11) __E_start_0(8).
	p(NT(11), (NT(11)+NT(8)));
//G3:   start(7)             => __E_start_1(11) _(9).
	p(NT(7), (NT(11)+NT(9)));
//G4:   statement(10)        => directive(12).
	p(NT(10), (NT(12)));
//G5:   statement(10)        => production(13).
	p(NT(10), (NT(13)));
//G6:   start_statement(14)  => _(9) statement(10) _(9).
	p(NT(14), (NT(9)+NT(10)+NT(9)));
//G7:   production_guard(17) => sym(15).
	p(NT(17), (NT(15)));
//G8:   __E_production_2(16) => _(9) '[' _(9) production_guard(17) _(9) ']'.
	p(NT(16), (NT(9)+T(1)+NT(9)+NT(17)+NT(9)+T(2)));
//G9:   __E_production_2(16) => null.
	p(NT(16), (nul));
//G10:  production(13)       => sym(15) __E_production_2(16) _(9) '=' '>' _(9) alternation(18) _(9) '.'.
	p(NT(13), (NT(15)+NT(16)+NT(9)+T(3)+T(4)+NT(9)+NT(18)+NT(9)+T(5)));
//G11:  __E_alternation_3(20) => _(9) '|' _(9) conjunction(19).
	p(NT(20), (NT(9)+T(6)+NT(9)+NT(19)));
//G12:  __E_alternation_4(21) => null.
	p(NT(21), (nul));
//G13:  __E_alternation_4(21) => __E_alternation_4(21) __E_alternation_3(20).
	p(NT(21), (NT(21)+NT(20)));
//G14:  alternation(18)      => conjunction(19) __E_alternation_4(21).
	p(NT(18), (NT(19)+NT(21)));
//G15:  __E_conjunction_5(23) => _(9) '&' _(9) concatenation(22).
	p(NT(23), (NT(9)+T(7)+NT(9)+NT(22)));
//G16:  __E_conjunction_6(24) => null.
	p(NT(24), (nul));
//G17:  __E_conjunction_6(24) => __E_conjunction_6(24) __E_conjunction_5(23).
	p(NT(24), (NT(24)+NT(23)));
//G18:  conjunction(19)      => concatenation(22) __E_conjunction_6(24).
	p(NT(19), (NT(22)+NT(24)));
//G19:  __E_concatenation_7(26) => __(27) factor(25).
	p(NT(26), (NT(27)+NT(25)));
//G20:  __E_concatenation_8(28) => null.
	p(NT(28), (nul));
//G21:  __E_concatenation_8(28) => __E_concatenation_8(28) __E_concatenation_7(26).
	p(NT(28), (NT(28)+NT(26)));
//G22:  concatenation(22)    => factor(25) __E_concatenation_8(28).
	p(NT(22), (NT(25)+NT(28)));
//G23:  __E_factor_9(30)     => factor(25) _(9) ':' sym(15).
	p(NT(30), (NT(25)+NT(9)+T(8)+NT(15)));
//G24:  shorthand_rule(29)   => __E_factor_9(30).
	p(NT(29), (NT(30)));
//G25:  factor(25)           => shorthand_rule(29).
	p(NT(25), (NT(29)));
//G26:  __E_factor_10(32)    => term(33) '?'.
	p(NT(32), (NT(33)+T(9)));
//G27:  optional(31)         => __E_factor_10(32).
	p(NT(31), (NT(32)));
//G28:  factor(25)           => optional(31).
	p(NT(25), (NT(31)));
//G29:  __E_factor_11(35)    => term(33) '+'.
	p(NT(35), (NT(33)+T(10)));
//G30:  repeat(34)           => __E_factor_11(35).
	p(NT(34), (NT(35)));
//G31:  factor(25)           => repeat(34).
	p(NT(25), (NT(34)));
//G32:  __E_factor_12(37)    => term(33) '*'.
	p(NT(37), (NT(33)+T(11)));
//G33:  none_or_repeat(36)   => __E_factor_12(37).
	p(NT(36), (NT(37)));
//G34:  factor(25)           => none_or_repeat(36).
	p(NT(25), (NT(36)));
//G35:  __E_factor_13(39)    => '~' term(33).
	p(NT(39), (T(12)+NT(33)));
//G36:  neg(38)              => __E_factor_13(39).
	p(NT(38), (NT(39)));
//G37:  factor(25)           => neg(38).
	p(NT(25), (NT(38)));
//G38:  factor(25)           => term(33).
	p(NT(25), (NT(33)));
//G39:  __E_term_14(41)      => '(' _(9) alternation(18) _(9) ')'.
	p(NT(41), (T(13)+NT(9)+NT(18)+NT(9)+T(14)));
//G40:  group(40)            => __E_term_14(41).
	p(NT(40), (NT(41)));
//G41:  term(33)             => group(40).
	p(NT(33), (NT(40)));
//G42:  __E_term_15(43)      => '[' _(9) alternation(18) _(9) ']'.
	p(NT(43), (T(1)+NT(9)+NT(18)+NT(9)+T(2)));
//G43:  optional_group(42)   => __E_term_15(43).
	p(NT(42), (NT(43)));
//G44:  term(33)             => optional_group(42).
	p(NT(33), (NT(42)));
//G45:  __E_term_16(45)      => '{' _(9) alternation(18) _(9) '}'.
	p(NT(45), (T(15)+NT(9)+NT(18)+NT(9)+T(16)));
//G46:  repeat_group(44)     => __E_term_16(45).
	p(NT(44), (NT(45)));
//G47:  term(33)             => repeat_group(44).
	p(NT(33), (NT(44)));
//G48:  term(33)             => terminal(46).
	p(NT(33), (NT(46)));
//G49:  term(33)             => sym(15).
	p(NT(33), (NT(15)));
//G50:  terminal(46)         => terminal_char(47).
	p(NT(46), (NT(47)));
//G51:  terminal(46)         => terminal_string(48).
	p(NT(46), (NT(48)));
//G52:  terminal(46)         => terminal_hex(49).
	p(NT(46), (NT(49)));
//G53:  __E_terminal_hex_17(51) => xdigit(6).
	p(NT(51), (NT(6)));
//G54:  __E___E_terminal_hex_17_18(52) => xdigit(6) xdigit(6).
	p(NT(52), (NT(6)+NT(6)));
//G55:  __E___E_terminal_hex_17_19(53) => __E___E_terminal_hex_17_18(52).
	p(NT(53), (NT(52)));
//G56:  __E___E_terminal_hex_17_19(53) => __E___E_terminal_hex_17_19(53) __E___E_terminal_hex_17_18(52).
	p(NT(53), (NT(53)+NT(52)));
//G57:  __E_terminal_hex_17(51) => __E___E_terminal_hex_17_19(53).
	p(NT(51), (NT(53)));
//G58:  hex_bytes(50)        => __E_terminal_hex_17(51).
	p(NT(50), (NT(51)));
//G59:  terminal_hex(49)     => '0' 'x' hex_bytes(50).
	p(NT(49), (T(17)+T(18)+NT(50)));
//G60:  __E_sym_20(54)       => alpha(3).
	p(NT(54), (NT(3)));
//G61:  __E_sym_20(54)       => '_'.
	p(NT(54), (T(19)));
//G62:  __E_sym_21(55)       => alnum(2).
	p(NT(55), (NT(2)));
//G63:  __E_sym_21(55)       => '_'.
	p(NT(55), (T(19)));
//G64:  __E_sym_22(56)       => null.
	p(NT(56), (nul));
//G65:  __E_sym_22(56)       => __E_sym_22(56) __E_sym_21(55).
	p(NT(56), (NT(56)+NT(55)));
//G66:  sym(15)              => __E_sym_20(54) __E_sym_22(56).
	p(NT(15), (NT(54)+NT(56)));
//G67:  __E_terminal_char_23(57) => unescaped_c(58).
	p(NT(57), (NT(58)));
//G68:  __E_terminal_char_23(57) => escaped_c(59).
	p(NT(57), (NT(59)));
//G69:  terminal_char(47)    => '\'' __E_terminal_char_23(57) '\''.
	p(NT(47), (T(20)+NT(57)+T(20)));
//G70:  __E_unescaped_c_24(60) => '\''.
	p(NT(60), (T(20)));
//G71:  __E_unescaped_c_24(60) => '\\'.
	p(NT(60), (T(21)));
//G72:  __N_0(118)           => __E_unescaped_c_24(60).
	p(NT(118), (NT(60)));
//G73:  unescaped_c(58)      => printable(5) & ~( __N_0(118) ).	 # conjunctive
	p(NT(58), (NT(5)) & ~(NT(118)));
//G74:  __E_escaped_c_25(61) => '\''.
	p(NT(61), (T(20)));
//G75:  __E_escaped_c_25(61) => escape_char(62).
	p(NT(61), (NT(62)));
//G76:  escaped_c(59)        => '\\' __E_escaped_c_25(61).
	p(NT(59), (T(21)+NT(61)));
//G77:  __E_terminal_string_26(63) => unescaped_s(64).
	p(NT(63), (NT(64)));
//G78:  __E_terminal_string_26(63) => escaped_s(65).
	p(NT(63), (NT(65)));
//G79:  __E_terminal_string_27(66) => null.
	p(NT(66), (nul));
//G80:  __E_terminal_string_27(66) => __E_terminal_string_27(66) __E_terminal_string_26(63).
	p(NT(66), (NT(66)+NT(63)));
//G81:  terminal_string(48)  => '"' __E_terminal_string_27(66) '"'.
	p(NT(48), (T(22)+NT(66)+T(22)));
//G82:  __E_unescaped_s_28(67) => '"'.
	p(NT(67), (T(22)));
//G83:  __E_unescaped_s_28(67) => '\\'.
	p(NT(67), (T(21)));
//G84:  __N_1(119)           => __E_unescaped_s_28(67).
	p(NT(119), (NT(67)));
//G85:  unescaped_s(64)      => printable(5) & ~( __N_1(119) ).	 # conjunctive
	p(NT(64), (NT(5)) & ~(NT(119)));
//G86:  __E_escaped_s_29(68) => '"'.
	p(NT(68), (T(22)));
//G87:  __E_escaped_s_29(68) => escape_char(62).
	p(NT(68), (NT(62)));
//G88:  escaped_s(65)        => '\\' __E_escaped_s_29(68).
	p(NT(65), (T(21)+NT(68)));
//G89:  escape_char(62)      => 'a'.
	p(NT(62), (T(23)));
//G90:  escape_char(62)      => 'b'.
	p(NT(62), (T(24)));
//G91:  escape_char(62)      => 'f'.
	p(NT(62), (T(25)));
//G92:  escape_char(62)      => 'n'.
	p(NT(62), (T(26)));
//G93:  escape_char(62)      => 'r'.
	p(NT(62), (T(27)));
//G94:  escape_char(62)      => 't'.
	p(NT(62), (T(28)));
//G95:  escape_char(62)      => 'v'.
	p(NT(62), (T(29)));
//G96:  escape_char(62)      => '\\'.
	p(NT(62), (T(21)));
//G97:  escape_char(62)      => '/'.
	p(NT(62), (T(30)));
//G98:  escape_char(62)      => esc_hex(69).
	p(NT(62), (NT(69)));
//G99:  escape_char(62)      => esc_u4(70).
	p(NT(62), (NT(70)));
//G100: escape_char(62)      => esc_U8(71).
	p(NT(62), (NT(71)));
//G101: __E_esc_hex_30(72)   => 'x'.
	p(NT(72), (T(18)));
//G102: __E_esc_hex_30(72)   => 'X'.
	p(NT(72), (T(31)));
//G103: __E_esc_hex_31(73)   => xdigit(6).
	p(NT(73), (NT(6)));
//G104: __E_esc_hex_31(73)   => null.
	p(NT(73), (nul));
//G105: esc_hex(69)          => __E_esc_hex_30(72) xdigit(6) __E_esc_hex_31(73).
	p(NT(69), (NT(72)+NT(6)+NT(73)));
//G106: esc_u4(70)           => 'u' xdigit(6) xdigit(6) xdigit(6) xdigit(6).
	p(NT(70), (T(32)+NT(6)+NT(6)+NT(6)+NT(6)));
//G107: esc_U8(71)           => 'U' xdigit(6) xdigit(6) xdigit(6) xdigit(6) xdigit(6) xdigit(6) xdigit(6) xdigit(6).
	p(NT(71), (T(33)+NT(6)+NT(6)+NT(6)+NT(6)+NT(6)+NT(6)+NT(6)+NT(6)));
//G108: directive_name(76)   => sym(15).
	p(NT(76), (NT(15)));
//G109: __E_directive_32(75) => '@' _(9) directive_name(76) _(9).
	p(NT(75), (T(34)+NT(9)+NT(76)+NT(9)));
//G110: directive_token(74)  => __E_directive_32(75).
	p(NT(74), (NT(75)));
//G111: __E___E___E_directive_33_34_35(81) => sep(80) sym(15).
	p(NT(81), (NT(80)+NT(15)));
//G112: __E___E___E_directive_33_34_36(82) => null.
	p(NT(82), (nul));
//G113: __E___E___E_directive_33_34_36(82) => __E___E___E_directive_33_34_36(82) __E___E___E_directive_33_34_35(81).
	p(NT(82), (NT(82)+NT(81)));
//G114: __E___E_directive_33_34(79) => sep(80) sym(15) __E___E___E_directive_33_34_36(82).
	p(NT(79), (NT(80)+NT(15)+NT(82)));
//G115: directive_cmd(78)    => __E___E_directive_33_34(79).
	p(NT(78), (NT(79)));
//G116: __E_directive_33(77) => directive_cmd(78).
	p(NT(77), (NT(78)));
//G117: __E_directive_37(83) => null.
	p(NT(83), (nul));
//G118: __E_directive_37(83) => __E_directive_37(83) __E_directive_33(77).
	p(NT(83), (NT(83)+NT(77)));
//G119: __E___E_directive_38_39(86) => _(9) ';' _(9) dir_pair(85).
	p(NT(86), (NT(9)+T(35)+NT(9)+NT(85)));
//G120: __E___E_directive_38_40(87) => null.
	p(NT(87), (nul));
//G121: __E___E_directive_38_40(87) => __E___E_directive_38_40(87) __E___E_directive_38_39(86).
	p(NT(87), (NT(87)+NT(86)));
//G122: __E_directive_38(84) => __(27) dir_pair(85) __E___E_directive_38_40(87).
	p(NT(84), (NT(27)+NT(85)+NT(87)));
//G123: __E_directive_38(84) => null.
	p(NT(84), (nul));
//G124: directive(12)        => directive_token(74) __E_directive_37(83) __E_directive_38(84) _(9) '.'.
	p(NT(12), (NT(74)+NT(83)+NT(84)+NT(9)+T(5)));
//G125: sep(80)              => '-'.
	p(NT(80), (T(36)));
//G126: sep(80)              => '_'.
	p(NT(80), (T(19)));
//G127: sep(80)              => __(27).
	p(NT(80), (NT(27)));
//G128: __E_dir_pair_41(89)  => _(9) ':' _(9) dir_list(88).
	p(NT(89), (NT(9)+T(8)+NT(9)+NT(88)));
//G129: __E_dir_pair_41(89)  => null.
	p(NT(89), (nul));
//G130: dir_pair(85)         => dir_list(88) __E_dir_pair_41(89).
	p(NT(85), (NT(88)+NT(89)));
//G131: __E_dir_list_42(91)  => _(9) ',' _(9) dir_arg(90).
	p(NT(91), (NT(9)+T(37)+NT(9)+NT(90)));
//G132: __E_dir_list_43(92)  => null.
	p(NT(92), (nul));
//G133: __E_dir_list_43(92)  => __E_dir_list_43(92) __E_dir_list_42(91).
	p(NT(92), (NT(92)+NT(91)));
//G134: dir_list(88)         => dir_arg(90) __E_dir_list_43(92).
	p(NT(88), (NT(90)+NT(92)));
//G135: dir_arg(90)          => dir_alias(93).
	p(NT(90), (NT(93)));
//G136: dir_arg(90)          => tree_path(94).
	p(NT(90), (NT(94)));
//G137: dir_arg(90)          => dir_sym(95).
	p(NT(90), (NT(95)));
//G138: dir_arg(90)          => terminal_string(48).
	p(NT(90), (NT(48)));
//G139: __E___E___E_dir_arg_44_45_46(101) => pat_space(100).
	p(NT(101), (NT(100)));
//G140: __E___E___E_dir_arg_44_45_46(101) => __E___E___E_dir_arg_44_45_46(101) pat_space(100).
	p(NT(101), (NT(101)+NT(100)));
//G141: __E___E_dir_arg_44_45(99) => __E___E___E_dir_arg_44_45_46(101) pat_word(98).
	p(NT(99), (NT(101)+NT(98)));
//G142: __E___E_dir_arg_44_47(102) => __E___E_dir_arg_44_45(99).
	p(NT(102), (NT(99)));
//G143: __E___E_dir_arg_44_47(102) => __E___E_dir_arg_44_47(102) __E___E_dir_arg_44_45(99).
	p(NT(102), (NT(102)+NT(99)));
//G144: __E_dir_arg_44(97)   => pat_word(98) __E___E_dir_arg_44_47(102).
	p(NT(97), (NT(98)+NT(102)));
//G145: __N_2(120)           => __E_dir_arg_44(97).
	p(NT(120), (NT(97)));
//G146: dir_arg(90)          => treemr_pattern(96) & ~( __N_2(120) ).	 # conjunctive
	p(NT(90), (NT(96)) & ~(NT(120)));
//G147: dir_alias(93)        => dir_sym(95) __(27) 'a' 's' __(27) dir_sym(95).
	p(NT(93), (NT(95)+NT(27)+T(23)+T(38)+NT(27)+NT(95)));
//G148: pat_word(98)         => dir_sym(95).
	p(NT(98), (NT(95)));
//G149: pat_word(98)         => terminal_string(48).
	p(NT(98), (NT(48)));
//G150: __E_tree_path_48(103) => _(9) '>' _(9) dir_sym(95).
	p(NT(103), (NT(9)+T(4)+NT(9)+NT(95)));
//G151: __E_tree_path_49(104) => null.
	p(NT(104), (nul));
//G152: __E_tree_path_49(104) => __E_tree_path_49(104) __E_tree_path_48(103).
	p(NT(104), (NT(104)+NT(103)));
//G153: tree_path(94)        => dir_sym(95) _(9) '>' _(9) dir_sym(95) __E_tree_path_49(104).
	p(NT(94), (NT(95)+NT(9)+T(4)+NT(9)+NT(95)+NT(104)));
//G154: __E_dir_sym_50(105)  => alpha(3).
	p(NT(105), (NT(3)));
//G155: __E_dir_sym_50(105)  => '_'.
	p(NT(105), (T(19)));
//G156: __E_dir_sym_50(105)  => '*'.
	p(NT(105), (T(11)));
//G157: __E_dir_sym_51(106)  => alnum(2).
	p(NT(106), (NT(2)));
//G158: __E_dir_sym_51(106)  => '_'.
	p(NT(106), (T(19)));
//G159: __E_dir_sym_51(106)  => '*'.
	p(NT(106), (T(11)));
//G160: __E_dir_sym_52(107)  => null.
	p(NT(107), (nul));
//G161: __E_dir_sym_52(107)  => __E_dir_sym_52(107) __E_dir_sym_51(106).
	p(NT(107), (NT(107)+NT(106)));
//G162: dir_sym(95)          => __E_dir_sym_50(105) __E_dir_sym_52(107).
	p(NT(95), (NT(105)+NT(107)));
//G163: __E___E_treemr_pattern_53_54(110) => null.
	p(NT(110), (nul));
//G164: __E___E_treemr_pattern_53_54(110) => __E___E_treemr_pattern_53_54(110) pat_space(100).
	p(NT(110), (NT(110)+NT(100)));
//G165: __E_treemr_pattern_53(109) => __E___E_treemr_pattern_53_54(110) pat_unit(108).
	p(NT(109), (NT(110)+NT(108)));
//G166: __E_treemr_pattern_55(111) => null.
	p(NT(111), (nul));
//G167: __E_treemr_pattern_55(111) => __E_treemr_pattern_55(111) __E_treemr_pattern_53(109).
	p(NT(111), (NT(111)+NT(109)));
//G168: treemr_pattern(96)   => pat_unit(108) __E_treemr_pattern_55(111).
	p(NT(96), (NT(108)+NT(111)));
//G169: pat_unit(108)        => terminal_string(48).
	p(NT(108), (NT(48)));
//G170: pat_unit(108)        => terminal_char(47).
	p(NT(108), (NT(47)));
//G171: pat_unit(108)        => pat_char(112).
	p(NT(108), (NT(112)));
//G172: __E_pat_char_56(113) => space(4).
	p(NT(113), (NT(4)));
//G173: __E_pat_char_56(113) => ','.
	p(NT(113), (T(37)));
//G174: __E_pat_char_56(113) => '.'.
	p(NT(113), (T(5)));
//G175: __E_pat_char_56(113) => ':'.
	p(NT(113), (T(8)));
//G176: __E_pat_char_56(113) => ';'.
	p(NT(113), (T(35)));
//G177: __E_pat_char_56(113) => '#'.
	p(NT(113), (T(39)));
//G178: __E_pat_char_56(113) => '"'.
	p(NT(113), (T(22)));
//G179: __E_pat_char_56(113) => '\''.
	p(NT(113), (T(20)));
//G180: __N_3(121)           => __E_pat_char_56(113).
	p(NT(121), (NT(113)));
//G181: pat_char(112)        => printable(5) & ~( __N_3(121) ).	 # conjunctive
	p(NT(112), (NT(5)) & ~(NT(121)));
//G182: pat_space(100)       => ' '.
	p(NT(100), (T(40)));
//G183: pat_space(100)       => '\t'.
	p(NT(100), (T(41)));
//G184: _(9)                 => __(27).
	p(NT(9), (NT(27)));
//G185: _(9)                 => null.
	p(NT(9), (nul));
//G186: __(27)               => space(4).
	p(NT(27), (NT(4)));
//G187: __(27)               => comment(114).
	p(NT(27), (NT(114)));
//G188: __(27)               => __(27) space(4).
	p(NT(27), (NT(27)+NT(4)));
//G189: __(27)               => __(27) comment(114).
	p(NT(27), (NT(27)+NT(114)));
//G190: __E_comment_57(115)  => printable(5).
	p(NT(115), (NT(5)));
//G191: __E_comment_57(115)  => '\t'.
	p(NT(115), (T(41)));
//G192: __E_comment_58(116)  => null.
	p(NT(116), (nul));
//G193: __E_comment_58(116)  => __E_comment_58(116) __E_comment_57(115).
	p(NT(116), (NT(116)+NT(115)));
//G194: __E_comment_59(117)  => '\r'.
	p(NT(117), (T(42)));
//G195: __E_comment_59(117)  => '\n'.
	p(NT(117), (T(43)));
//G196: __E_comment_59(117)  => eof(1).
	p(NT(117), (NT(1)));
//G197: comment(114)         => '#' __E_comment_58(116) __E_comment_59(117).
	p(NT(114), (T(39)+NT(116)+NT(117)));
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
		nul, eof, alnum, alpha, space, printable, xdigit, start, __E_start_0, _, 
		statement, __E_start_1, directive, production, start_statement, sym, __E_production_2, production_guard, alternation, conjunction, 
		__E_alternation_3, __E_alternation_4, concatenation, __E_conjunction_5, __E_conjunction_6, factor, __E_concatenation_7, __, __E_concatenation_8, shorthand_rule, 
		__E_factor_9, optional, __E_factor_10, term, repeat, __E_factor_11, none_or_repeat, __E_factor_12, neg, __E_factor_13, 
		group, __E_term_14, optional_group, __E_term_15, repeat_group, __E_term_16, terminal, terminal_char, terminal_string, terminal_hex, 
		hex_bytes, __E_terminal_hex_17, __E___E_terminal_hex_17_18, __E___E_terminal_hex_17_19, __E_sym_20, __E_sym_21, __E_sym_22, __E_terminal_char_23, unescaped_c, escaped_c, 
		__E_unescaped_c_24, __E_escaped_c_25, escape_char, __E_terminal_string_26, unescaped_s, escaped_s, __E_terminal_string_27, __E_unescaped_s_28, __E_escaped_s_29, esc_hex, 
		esc_u4, esc_U8, __E_esc_hex_30, __E_esc_hex_31, directive_token, __E_directive_32, directive_name, __E_directive_33, directive_cmd, __E___E_directive_33_34, 
		sep, __E___E___E_directive_33_34_35, __E___E___E_directive_33_34_36, __E_directive_37, __E_directive_38, dir_pair, __E___E_directive_38_39, __E___E_directive_38_40, dir_list, __E_dir_pair_41, 
		dir_arg, __E_dir_list_42, __E_dir_list_43, dir_alias, tree_path, dir_sym, treemr_pattern, __E_dir_arg_44, pat_word, __E___E_dir_arg_44_45, 
		pat_space, __E___E___E_dir_arg_44_45_46, __E___E_dir_arg_44_47, __E_tree_path_48, __E_tree_path_49, __E_dir_sym_50, __E_dir_sym_51, __E_dir_sym_52, pat_unit, __E_treemr_pattern_53, 
		__E___E_treemr_pattern_53_54, __E_treemr_pattern_55, pat_char, __E_pat_char_56, comment, __E_comment_57, __E_comment_58, __E_comment_59, __N_0, __N_1, 
		__N_2, __N_3, 
	};
};

#ifdef _MSC_VER
// the base parser puts its chart indexes on a cache line, so the padding is wanted
#	pragma warning(push)
#	pragma warning(disable : 4324)
#endif
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
#ifdef _MSC_VER
#	pragma warning(pop)
#endif

#endif // __TGF_PARSER_H__
