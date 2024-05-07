// This file is generated from a file src/tgf.tgf by
//       https://github.com/IDNI/parser/tools/tgf
//
#ifndef __TGF_PARSER_H__
#define __TGF_PARSER_H__

#include "parser.h"

namespace tgf_parser_data {

using char_type     = char;
using terminal_type = char;

static inline std::vector<std::string> symbol_names{
	"", "eof", "alnum", "alpha", "space", "printable", "__", "_", "sep", "cc_sym", 
	"production", "directive", "tree_path", "shorthand_rule", "optional", "repeat", "none_or_repeat", "neg", "group", "optional_group", 
	"repeat_group", "start_dir", "inline_dir", "trim_children_dir", "trim_children_terminals_dir", "trim_terminals_dir", "trim_dir", "use_dir", "disable_ad_dir", "ambiguous_dir", 
	"start", "__E_start_0", "statement", "__E_start_1", "start_statement", "sym", "alternation", "conjunction", "__E_alternation_2", "__E_alternation_3", 
	"concatenation", "__E_conjunction_4", "__E_conjunction_5", "factor", "__E_concatenation_6", "__E_concatenation_7", "__E_factor_8", "__E_factor_9", "term", "__E_factor_10", 
	"__E_factor_11", "__E_factor_12", "__E_term_13", "__E_term_14", "__E_term_15", "terminal", "terminal_char", "terminal_string", "__E_sym_16", "__E_sym_17", 
	"__E_sym_18", "__E_terminal_char_19", "unescaped_c", "escaped_c", "__E_unescaped_c_20", "__E_escaped_c_21", "__E_terminal_string_22", "terminal_string_char", "__E___E_terminal_string_22_23", "unescaped_s", 
	"escaped_s", "__E_terminal_string_24", "__E_unescaped_s_25", "__E_escaped_s_26", "directive_body", "__E_directive_body_27", "__E_directive_body_28", "inline_arg", "__E___E_directive_body_28_29", "__E___E_directive_body_28_30", 
	"__E_directive_body_31", "__E___E_directive_body_31_32", "__E___E_directive_body_31_33", "__E_directive_body_34", "__E___E_directive_body_34_35", "__E___E_directive_body_34_36", "__E_directive_body_37", "__E_directive_body_38", "__E___E_directive_body_38_39", "__E___E_directive_body_38_40", 
	"__E_directive_body_41", "use_from", "use_param", "__E___E_directive_body_41_42", "__E___E_directive_body_41_43", "__E_directive_body_44", "__E___E_directive_body_44_45", "__E___E_directive_body_44_46", "__E_directive_body_47", "__E___E_directive_body_47_48", 
	"__E___E_directive_body_47_49", "__E_inline_arg_50", "__E___E_inline_arg_50_51", "__E_tree_path_52", "__E_tree_path_53", "cc_name", "__E_use_param_54", "__E_sep_55", "sep_required", "__E___56", 
	"__E____57", "comment", "__E_comment_58", "__E_comment_59", "__E_comment_60", "__N_0", "__N_1", 
};

static inline ::idni::nonterminals<char_type, terminal_type> nts{symbol_names};

static inline std::vector<terminal_type> terminals{
	'\0', '=', '>', '.', '|', '&', ':', '?', '+', 
	'*', '~', '(', ')', '[', ']', '{', '}', '_', '\'', 
	'\\', '/', 'b', 'f', 'n', 'r', 't', '"', '@', 's', 
	'a', ',', 'i', 'l', 'e', 'm', 'c', 'h', 'd', 'u', 
	'o', 'g', 'p', 'k', 'w', 'x', '-', '\t', '\r', '\n', 
	'#', 
};

static inline ::idni::char_class_fns<terminal_type> char_classes =
	::idni::predefined_char_classes<char_type, terminal_type>({
		"eof",
		"alnum",
		"alpha",
		"space",
		"printable",
	}, nts);

static inline struct ::idni::grammar<char_type, terminal_type>::options
	grammar_options
{
	.transform_negation = false,
	.auto_disambiguate = true,
	.shaping = {
		.to_trim = {
			6, 7, 8
		},
		.to_trim_children_terminals = {
			9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
			19, 20, 21, 22, 23, 24, 25, 26, 27, 28,
			29
		},
		.trim_terminals = false,
		.inline_char_classes = true
	}
};
static inline ::idni::parser<char_type, terminal_type>::options parser_options{
};

static inline ::idni::prods<char_type, terminal_type> start_symbol{ nts(30) };

static inline idni::prods<char_type, terminal_type>& productions() {
	static bool loaded = false;
	static idni::prods<char_type, terminal_type>
		p, nul(idni::lit<char_type, terminal_type>{});
	if (loaded) return p;
	#define  T(x) (idni::prods<char_type, terminal_type>{ terminals[x] })
	#define NT(x) (idni::prods<char_type, terminal_type>{ nts(x) })
//G0:   __E_start_0(31)      => _(7) statement(32).
	p(NT(31), (NT(7)+NT(32)));
//G1:   __E_start_1(33)      => null.
	p(NT(33), (nul));
//G2:   __E_start_1(33)      => __E_start_0(31) __E_start_1(33).
	p(NT(33), (NT(31)+NT(33)));
//G3:   start(30)            => __E_start_1(33) _(7).
	p(NT(30), (NT(33)+NT(7)));
//G4:   statement(32)        => directive(11).
	p(NT(32), (NT(11)));
//G5:   statement(32)        => production(10).
	p(NT(32), (NT(10)));
//G6:   start_statement(34)  => _(7) statement(32) _(7).
	p(NT(34), (NT(7)+NT(32)+NT(7)));
//G7:   production(10)       => sym(35) _(7) '=' '>' _(7) alternation(36) _(7) '.'.
	p(NT(10), (NT(35)+NT(7)+T(1)+T(2)+NT(7)+NT(36)+NT(7)+T(3)));
//G8:   __E_alternation_2(38) => _(7) '|' _(7) conjunction(37).
	p(NT(38), (NT(7)+T(4)+NT(7)+NT(37)));
//G9:   __E_alternation_3(39) => null.
	p(NT(39), (nul));
//G10:  __E_alternation_3(39) => __E_alternation_2(38) __E_alternation_3(39).
	p(NT(39), (NT(38)+NT(39)));
//G11:  alternation(36)      => conjunction(37) __E_alternation_3(39).
	p(NT(36), (NT(37)+NT(39)));
//G12:  __E_conjunction_4(41) => _(7) '&' _(7) concatenation(40).
	p(NT(41), (NT(7)+T(5)+NT(7)+NT(40)));
//G13:  __E_conjunction_5(42) => null.
	p(NT(42), (nul));
//G14:  __E_conjunction_5(42) => __E_conjunction_4(41) __E_conjunction_5(42).
	p(NT(42), (NT(41)+NT(42)));
//G15:  conjunction(37)      => concatenation(40) __E_conjunction_5(42).
	p(NT(37), (NT(40)+NT(42)));
//G16:  __E_concatenation_6(44) => __(6) factor(43).
	p(NT(44), (NT(6)+NT(43)));
//G17:  __E_concatenation_7(45) => null.
	p(NT(45), (nul));
//G18:  __E_concatenation_7(45) => __E_concatenation_6(44) __E_concatenation_7(45).
	p(NT(45), (NT(44)+NT(45)));
//G19:  concatenation(40)    => factor(43) __E_concatenation_7(45).
	p(NT(40), (NT(43)+NT(45)));
//G20:  __E_factor_8(46)     => factor(43) _(7) ':' sym(35).
	p(NT(46), (NT(43)+NT(7)+T(6)+NT(35)));
//G21:  shorthand_rule(13)   => __E_factor_8(46).
	p(NT(13), (NT(46)));
//G22:  factor(43)           => shorthand_rule(13).
	p(NT(43), (NT(13)));
//G23:  __E_factor_9(47)     => term(48) '?'.
	p(NT(47), (NT(48)+T(7)));
//G24:  optional(14)         => __E_factor_9(47).
	p(NT(14), (NT(47)));
//G25:  factor(43)           => optional(14).
	p(NT(43), (NT(14)));
//G26:  __E_factor_10(49)    => term(48) '+'.
	p(NT(49), (NT(48)+T(8)));
//G27:  repeat(15)           => __E_factor_10(49).
	p(NT(15), (NT(49)));
//G28:  factor(43)           => repeat(15).
	p(NT(43), (NT(15)));
//G29:  __E_factor_11(50)    => term(48) '*'.
	p(NT(50), (NT(48)+T(9)));
//G30:  none_or_repeat(16)   => __E_factor_11(50).
	p(NT(16), (NT(50)));
//G31:  factor(43)           => none_or_repeat(16).
	p(NT(43), (NT(16)));
//G32:  __E_factor_12(51)    => '~' term(48).
	p(NT(51), (T(10)+NT(48)));
//G33:  neg(17)              => __E_factor_12(51).
	p(NT(17), (NT(51)));
//G34:  factor(43)           => neg(17).
	p(NT(43), (NT(17)));
//G35:  factor(43)           => term(48).
	p(NT(43), (NT(48)));
//G36:  __E_term_13(52)      => '(' _(7) alternation(36) _(7) ')'.
	p(NT(52), (T(11)+NT(7)+NT(36)+NT(7)+T(12)));
//G37:  group(18)            => __E_term_13(52).
	p(NT(18), (NT(52)));
//G38:  term(48)             => group(18).
	p(NT(48), (NT(18)));
//G39:  __E_term_14(53)      => '[' _(7) alternation(36) _(7) ']'.
	p(NT(53), (T(13)+NT(7)+NT(36)+NT(7)+T(14)));
//G40:  optional_group(19)   => __E_term_14(53).
	p(NT(19), (NT(53)));
//G41:  term(48)             => optional_group(19).
	p(NT(48), (NT(19)));
//G42:  __E_term_15(54)      => '{' _(7) alternation(36) _(7) '}'.
	p(NT(54), (T(15)+NT(7)+NT(36)+NT(7)+T(16)));
//G43:  repeat_group(20)     => __E_term_15(54).
	p(NT(20), (NT(54)));
//G44:  term(48)             => repeat_group(20).
	p(NT(48), (NT(20)));
//G45:  term(48)             => terminal(55).
	p(NT(48), (NT(55)));
//G46:  term(48)             => sym(35).
	p(NT(48), (NT(35)));
//G47:  terminal(55)         => terminal_char(56).
	p(NT(55), (NT(56)));
//G48:  terminal(55)         => terminal_string(57).
	p(NT(55), (NT(57)));
//G49:  __E_sym_16(58)       => alpha(3).
	p(NT(58), (NT(3)));
//G50:  __E_sym_16(58)       => '_'.
	p(NT(58), (T(17)));
//G51:  __E_sym_17(59)       => alnum(2).
	p(NT(59), (NT(2)));
//G52:  __E_sym_17(59)       => '_'.
	p(NT(59), (T(17)));
//G53:  __E_sym_18(60)       => null.
	p(NT(60), (nul));
//G54:  __E_sym_18(60)       => __E_sym_17(59) __E_sym_18(60).
	p(NT(60), (NT(59)+NT(60)));
//G55:  sym(35)              => __E_sym_16(58) __E_sym_18(60).
	p(NT(35), (NT(58)+NT(60)));
//G56:  __E_terminal_char_19(61) => unescaped_c(62).
	p(NT(61), (NT(62)));
//G57:  __E_terminal_char_19(61) => escaped_c(63).
	p(NT(61), (NT(63)));
//G58:  terminal_char(56)    => '\'' __E_terminal_char_19(61) '\''.
	p(NT(56), (T(18)+NT(61)+T(18)));
//G59:  __E_unescaped_c_20(64) => '\''.
	p(NT(64), (T(18)));
//G60:  __E_unescaped_c_20(64) => '\\'.
	p(NT(64), (T(19)));
//G61:  __N_0(115)           => __E_unescaped_c_20(64).
	p(NT(115), (NT(64)));
//G62:  unescaped_c(62)      => printable(5) & ~( __N_0(115) ).	 # conjunctive
	p(NT(62), (NT(5)) & ~(NT(115)));
//G63:  __E_escaped_c_21(65) => '\''.
	p(NT(65), (T(18)));
//G64:  __E_escaped_c_21(65) => '\\'.
	p(NT(65), (T(19)));
//G65:  __E_escaped_c_21(65) => '/'.
	p(NT(65), (T(20)));
//G66:  __E_escaped_c_21(65) => 'b'.
	p(NT(65), (T(21)));
//G67:  __E_escaped_c_21(65) => 'f'.
	p(NT(65), (T(22)));
//G68:  __E_escaped_c_21(65) => 'n'.
	p(NT(65), (T(23)));
//G69:  __E_escaped_c_21(65) => 'r'.
	p(NT(65), (T(24)));
//G70:  __E_escaped_c_21(65) => 't'.
	p(NT(65), (T(25)));
//G71:  escaped_c(63)        => '\\' __E_escaped_c_21(65).
	p(NT(63), (T(19)+NT(65)));
//G72:  __E___E_terminal_string_22_23(68) => unescaped_s(69).
	p(NT(68), (NT(69)));
//G73:  __E___E_terminal_string_22_23(68) => escaped_s(70).
	p(NT(68), (NT(70)));
//G74:  terminal_string_char(67) => __E___E_terminal_string_22_23(68).
	p(NT(67), (NT(68)));
//G75:  __E_terminal_string_22(66) => terminal_string_char(67).
	p(NT(66), (NT(67)));
//G76:  __E_terminal_string_24(71) => null.
	p(NT(71), (nul));
//G77:  __E_terminal_string_24(71) => __E_terminal_string_22(66) __E_terminal_string_24(71).
	p(NT(71), (NT(66)+NT(71)));
//G78:  terminal_string(57)  => '"' __E_terminal_string_24(71) '"'.
	p(NT(57), (T(26)+NT(71)+T(26)));
//G79:  __E_unescaped_s_25(72) => '"'.
	p(NT(72), (T(26)));
//G80:  __E_unescaped_s_25(72) => '\\'.
	p(NT(72), (T(19)));
//G81:  __N_1(116)           => __E_unescaped_s_25(72).
	p(NT(116), (NT(72)));
//G82:  unescaped_s(69)      => printable(5) & ~( __N_1(116) ).	 # conjunctive
	p(NT(69), (NT(5)) & ~(NT(116)));
//G83:  __E_escaped_s_26(73) => '"'.
	p(NT(73), (T(26)));
//G84:  __E_escaped_s_26(73) => '\\'.
	p(NT(73), (T(19)));
//G85:  __E_escaped_s_26(73) => '/'.
	p(NT(73), (T(20)));
//G86:  __E_escaped_s_26(73) => 'b'.
	p(NT(73), (T(21)));
//G87:  __E_escaped_s_26(73) => 'f'.
	p(NT(73), (T(22)));
//G88:  __E_escaped_s_26(73) => 'n'.
	p(NT(73), (T(23)));
//G89:  __E_escaped_s_26(73) => 'r'.
	p(NT(73), (T(24)));
//G90:  __E_escaped_s_26(73) => 't'.
	p(NT(73), (T(25)));
//G91:  escaped_s(70)        => '\\' __E_escaped_s_26(73).
	p(NT(70), (T(19)+NT(73)));
//G92:  directive(11)        => '@' _(7) directive_body(74) _(7) '.'.
	p(NT(11), (T(27)+NT(7)+NT(74)+NT(7)+T(3)));
//G93:  __E_directive_body_27(75) => 's' 't' 'a' 'r' 't' __(6) sym(35).
	p(NT(75), (T(28)+T(25)+T(29)+T(24)+T(25)+NT(6)+NT(35)));
//G94:  start_dir(21)        => __E_directive_body_27(75).
	p(NT(21), (NT(75)));
//G95:  directive_body(74)   => start_dir(21).
	p(NT(74), (NT(21)));
//G96:  __E___E_directive_body_28_29(78) => _(7) ',' _(7) inline_arg(77).
	p(NT(78), (NT(7)+T(30)+NT(7)+NT(77)));
//G97:  __E___E_directive_body_28_30(79) => null.
	p(NT(79), (nul));
//G98:  __E___E_directive_body_28_30(79) => __E___E_directive_body_28_29(78) __E___E_directive_body_28_30(79).
	p(NT(79), (NT(78)+NT(79)));
//G99:  __E_directive_body_28(76) => 'i' 'n' 'l' 'i' 'n' 'e' __(6) inline_arg(77) __E___E_directive_body_28_30(79).
	p(NT(76), (T(31)+T(23)+T(32)+T(31)+T(23)+T(33)+NT(6)+NT(77)+NT(79)));
//G100: inline_dir(22)       => __E_directive_body_28(76).
	p(NT(22), (NT(76)));
//G101: directive_body(74)   => inline_dir(22).
	p(NT(74), (NT(22)));
//G102: __E___E_directive_body_31_32(81) => _(7) ',' _(7) sym(35).
	p(NT(81), (NT(7)+T(30)+NT(7)+NT(35)));
//G103: __E___E_directive_body_31_33(82) => null.
	p(NT(82), (nul));
//G104: __E___E_directive_body_31_33(82) => __E___E_directive_body_31_32(81) __E___E_directive_body_31_33(82).
	p(NT(82), (NT(81)+NT(82)));
//G105: __E_directive_body_31(80) => 't' 'r' 'i' 'm' sep(8) 'c' 'h' 'i' 'l' 'd' 'r' 'e' 'n' __(6) sym(35) __E___E_directive_body_31_33(82).
	p(NT(80), (T(25)+T(24)+T(31)+T(34)+NT(8)+T(35)+T(36)+T(31)+T(32)+T(37)+T(24)+T(33)+T(23)+NT(6)+NT(35)+NT(82)));
//G106: trim_children_dir(23) => __E_directive_body_31(80).
	p(NT(23), (NT(80)));
//G107: directive_body(74)   => trim_children_dir(23).
	p(NT(74), (NT(23)));
//G108: __E___E_directive_body_34_35(84) => _(7) ',' _(7) sym(35).
	p(NT(84), (NT(7)+T(30)+NT(7)+NT(35)));
//G109: __E___E_directive_body_34_36(85) => null.
	p(NT(85), (nul));
//G110: __E___E_directive_body_34_36(85) => __E___E_directive_body_34_35(84) __E___E_directive_body_34_36(85).
	p(NT(85), (NT(84)+NT(85)));
//G111: __E_directive_body_34(83) => 't' 'r' 'i' 'm' sep(8) 'c' 'h' 'i' 'l' 'd' 'r' 'e' 'n' sep(8) 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's' __(6) sym(35) __E___E_directive_body_34_36(85).
	p(NT(83), (T(25)+T(24)+T(31)+T(34)+NT(8)+T(35)+T(36)+T(31)+T(32)+T(37)+T(24)+T(33)+T(23)+NT(8)+T(25)+T(33)+T(24)+T(34)+T(31)+T(23)+T(29)+T(32)+T(28)+NT(6)+NT(35)+NT(85)));
//G112: trim_children_terminals_dir(24) => __E_directive_body_34(83).
	p(NT(24), (NT(83)));
//G113: directive_body(74)   => trim_children_terminals_dir(24).
	p(NT(74), (NT(24)));
//G114: __E_directive_body_37(86) => 't' 'r' 'i' 'm' sep(8) 'a' 'l' 'l' sep(8) 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
	p(NT(86), (T(25)+T(24)+T(31)+T(34)+NT(8)+T(29)+T(32)+T(32)+NT(8)+T(25)+T(33)+T(24)+T(34)+T(31)+T(23)+T(29)+T(32)+T(28)));
//G115: trim_terminals_dir(25) => __E_directive_body_37(86).
	p(NT(25), (NT(86)));
//G116: directive_body(74)   => trim_terminals_dir(25).
	p(NT(74), (NT(25)));
//G117: __E___E_directive_body_38_39(88) => _(7) ',' _(7) sym(35).
	p(NT(88), (NT(7)+T(30)+NT(7)+NT(35)));
//G118: __E___E_directive_body_38_40(89) => null.
	p(NT(89), (nul));
//G119: __E___E_directive_body_38_40(89) => __E___E_directive_body_38_39(88) __E___E_directive_body_38_40(89).
	p(NT(89), (NT(88)+NT(89)));
//G120: __E_directive_body_38(87) => 't' 'r' 'i' 'm' __(6) sym(35) __E___E_directive_body_38_40(89).
	p(NT(87), (T(25)+T(24)+T(31)+T(34)+NT(6)+NT(35)+NT(89)));
//G121: trim_dir(26)         => __E_directive_body_38(87).
	p(NT(26), (NT(87)));
//G122: directive_body(74)   => trim_dir(26).
	p(NT(74), (NT(26)));
//G123: use_from(91)         => cc_sym(9).
	p(NT(91), (NT(9)));
//G124: __E___E_directive_body_41_42(93) => _(7) ',' _(7) use_param(92).
	p(NT(93), (NT(7)+T(30)+NT(7)+NT(92)));
//G125: __E___E_directive_body_41_43(94) => null.
	p(NT(94), (nul));
//G126: __E___E_directive_body_41_43(94) => __E___E_directive_body_41_42(93) __E___E_directive_body_41_43(94).
	p(NT(94), (NT(93)+NT(94)));
//G127: __E_directive_body_41(90) => 'u' 's' 'e' __(6) use_from(91) __(6) use_param(92) __E___E_directive_body_41_43(94).
	p(NT(90), (T(38)+T(28)+T(33)+NT(6)+NT(91)+NT(6)+NT(92)+NT(94)));
//G128: use_dir(27)          => __E_directive_body_41(90).
	p(NT(27), (NT(90)));
//G129: directive_body(74)   => use_dir(27).
	p(NT(74), (NT(27)));
//G130: __E___E_directive_body_44_45(96) => 'a' 'u' 't' 'o' sep(8).
	p(NT(96), (T(29)+T(38)+T(25)+T(39)+NT(8)));
//G131: __E___E_directive_body_44_45(96) => null.
	p(NT(96), (nul));
//G132: __E___E_directive_body_44_46(97) => 'u' 'a' 't' 'i' 'o' 'n'.
	p(NT(97), (T(38)+T(29)+T(25)+T(31)+T(39)+T(23)));
//G133: __E___E_directive_body_44_46(97) => null.
	p(NT(97), (nul));
//G134: __E_directive_body_44(95) => 'd' 'i' 's' 'a' 'b' 'l' 'e' __(6) __E___E_directive_body_44_45(96) 'd' 'i' 's' 'a' 'm' 'b' 'i' 'g' __E___E_directive_body_44_46(97).
	p(NT(95), (T(37)+T(31)+T(28)+T(29)+T(21)+T(32)+T(33)+NT(6)+NT(96)+T(37)+T(31)+T(28)+T(29)+T(34)+T(21)+T(31)+T(40)+NT(97)));
//G135: disable_ad_dir(28)   => __E_directive_body_44(95).
	p(NT(28), (NT(95)));
//G136: directive_body(74)   => disable_ad_dir(28).
	p(NT(74), (NT(28)));
//G137: __E___E_directive_body_47_48(99) => _(7) ',' _(7) sym(35).
	p(NT(99), (NT(7)+T(30)+NT(7)+NT(35)));
//G138: __E___E_directive_body_47_49(100) => null.
	p(NT(100), (nul));
//G139: __E___E_directive_body_47_49(100) => __E___E_directive_body_47_48(99) __E___E_directive_body_47_49(100).
	p(NT(100), (NT(99)+NT(100)));
//G140: __E_directive_body_47(98) => 'a' 'm' 'b' 'i' 'g' 'u' 'o' 'u' 's' _(7) sym(35) __E___E_directive_body_47_49(100).
	p(NT(98), (T(29)+T(34)+T(21)+T(31)+T(40)+T(38)+T(39)+T(38)+T(28)+NT(7)+NT(35)+NT(100)));
//G141: ambiguous_dir(29)    => __E_directive_body_47(98).
	p(NT(29), (NT(98)));
//G142: directive_body(74)   => ambiguous_dir(29).
	p(NT(74), (NT(29)));
//G143: inline_arg(77)       => tree_path(12).
	p(NT(77), (NT(12)));
//G144: __E___E_inline_arg_50_51(102) => 'e' 's'.
	p(NT(102), (T(33)+T(28)));
//G145: __E___E_inline_arg_50_51(102) => null.
	p(NT(102), (nul));
//G146: __E_inline_arg_50(101) => 'c' 'h' 'a' 'r' sep(8) 'c' 'l' 'a' 's' 's' __E___E_inline_arg_50_51(102).
	p(NT(101), (T(35)+T(36)+T(29)+T(24)+NT(8)+T(35)+T(32)+T(29)+T(28)+T(28)+NT(102)));
//G147: cc_sym(9)            => __E_inline_arg_50(101).
	p(NT(9), (NT(101)));
//G148: inline_arg(77)       => cc_sym(9).
	p(NT(77), (NT(9)));
//G149: __E_tree_path_52(103) => _(7) '>' _(7) sym(35).
	p(NT(103), (NT(7)+T(2)+NT(7)+NT(35)));
//G150: __E_tree_path_53(104) => null.
	p(NT(104), (nul));
//G151: __E_tree_path_53(104) => __E_tree_path_52(103) __E_tree_path_53(104).
	p(NT(104), (NT(103)+NT(104)));
//G152: tree_path(12)        => sym(35) __E_tree_path_53(104).
	p(NT(12), (NT(35)+NT(104)));
//G153: __E_use_param_54(106) => 'e' 'o' 'f'.
	p(NT(106), (T(33)+T(39)+T(22)));
//G154: __E_use_param_54(106) => 'a' 'l' 'n' 'u' 'm'.
	p(NT(106), (T(29)+T(32)+T(23)+T(38)+T(34)));
//G155: __E_use_param_54(106) => 'a' 'l' 'p' 'h' 'a'.
	p(NT(106), (T(29)+T(32)+T(41)+T(36)+T(29)));
//G156: __E_use_param_54(106) => 'b' 'l' 'a' 'n' 'k'.
	p(NT(106), (T(21)+T(32)+T(29)+T(23)+T(42)));
//G157: __E_use_param_54(106) => 'c' 'n' 't' 'r' 'l'.
	p(NT(106), (T(35)+T(23)+T(25)+T(24)+T(32)));
//G158: __E_use_param_54(106) => 'd' 'i' 'g' 'i' 't'.
	p(NT(106), (T(37)+T(31)+T(40)+T(31)+T(25)));
//G159: __E_use_param_54(106) => 'g' 'r' 'a' 'p' 'h'.
	p(NT(106), (T(40)+T(24)+T(29)+T(41)+T(36)));
//G160: __E_use_param_54(106) => 'l' 'o' 'w' 'e' 'r'.
	p(NT(106), (T(32)+T(39)+T(43)+T(33)+T(24)));
//G161: __E_use_param_54(106) => 'p' 'r' 'i' 'n' 't' 'a' 'b' 'l' 'e'.
	p(NT(106), (T(41)+T(24)+T(31)+T(23)+T(25)+T(29)+T(21)+T(32)+T(33)));
//G162: __E_use_param_54(106) => 'p' 'u' 'n' 'c' 't'.
	p(NT(106), (T(41)+T(38)+T(23)+T(35)+T(25)));
//G163: __E_use_param_54(106) => 's' 'p' 'a' 'c' 'e'.
	p(NT(106), (T(28)+T(41)+T(29)+T(35)+T(33)));
//G164: __E_use_param_54(106) => 'u' 'p' 'p' 'e' 'r'.
	p(NT(106), (T(38)+T(41)+T(41)+T(33)+T(24)));
//G165: __E_use_param_54(106) => 'x' 'd' 'i' 'g' 'i' 't'.
	p(NT(106), (T(44)+T(37)+T(31)+T(40)+T(31)+T(25)));
//G166: cc_name(105)         => __E_use_param_54(106).
	p(NT(105), (NT(106)));
//G167: use_param(92)        => cc_name(105).
	p(NT(92), (NT(105)));
//G168: __E_sep_55(107)      => sep_required(108).
	p(NT(107), (NT(108)));
//G169: __E_sep_55(107)      => null.
	p(NT(107), (nul));
//G170: sep(8)               => __E_sep_55(107).
	p(NT(8), (NT(107)));
//G171: sep_required(108)    => '-'.
	p(NT(108), (T(45)));
//G172: sep_required(108)    => '_'.
	p(NT(108), (T(17)));
//G173: sep_required(108)    => __(6).
	p(NT(108), (NT(6)));
//G174: __E___56(109)        => __(6).
	p(NT(109), (NT(6)));
//G175: __E___56(109)        => null.
	p(NT(109), (nul));
//G176: _(7)                 => __E___56(109).
	p(NT(7), (NT(109)));
//G177: __E____57(110)       => space(4).
	p(NT(110), (NT(4)));
//G178: __E____57(110)       => comment(111).
	p(NT(110), (NT(111)));
//G179: __(6)                => __E____57(110) _(7).
	p(NT(6), (NT(110)+NT(7)));
//G180: __E_comment_58(112)  => printable(5).
	p(NT(112), (NT(5)));
//G181: __E_comment_58(112)  => '\t'.
	p(NT(112), (T(46)));
//G182: __E_comment_59(113)  => null.
	p(NT(113), (nul));
//G183: __E_comment_59(113)  => __E_comment_58(112) __E_comment_59(113).
	p(NT(113), (NT(112)+NT(113)));
//G184: __E_comment_60(114)  => '\r'.
	p(NT(114), (T(47)));
//G185: __E_comment_60(114)  => '\n'.
	p(NT(114), (T(48)));
//G186: __E_comment_60(114)  => eof(1).
	p(NT(114), (NT(1)));
//G187: comment(111)         => '#' __E_comment_59(113) __E_comment_60(114).
	p(NT(111), (T(49)+NT(113)+NT(114)));
	#undef T
	#undef NT
	return loaded = true, p;
}

static inline ::idni::grammar<char_type, terminal_type> grammar(
	nts, productions(), start_symbol, char_classes, grammar_options);

} // namespace tgf_parser_data

struct tgf_parser : public idni::parser<char, char> {
	enum nonterminal {
		nul, eof, alnum, alpha, space, printable, __, _, sep, cc_sym, 
		production, directive, tree_path, shorthand_rule, optional, repeat, none_or_repeat, neg, group, optional_group, 
		repeat_group, start_dir, inline_dir, trim_children_dir, trim_children_terminals_dir, trim_terminals_dir, trim_dir, use_dir, disable_ad_dir, ambiguous_dir, 
		start, __E_start_0, statement, __E_start_1, start_statement, sym, alternation, conjunction, __E_alternation_2, __E_alternation_3, 
		concatenation, __E_conjunction_4, __E_conjunction_5, factor, __E_concatenation_6, __E_concatenation_7, __E_factor_8, __E_factor_9, term, __E_factor_10, 
		__E_factor_11, __E_factor_12, __E_term_13, __E_term_14, __E_term_15, terminal, terminal_char, terminal_string, __E_sym_16, __E_sym_17, 
		__E_sym_18, __E_terminal_char_19, unescaped_c, escaped_c, __E_unescaped_c_20, __E_escaped_c_21, __E_terminal_string_22, terminal_string_char, __E___E_terminal_string_22_23, unescaped_s, 
		escaped_s, __E_terminal_string_24, __E_unescaped_s_25, __E_escaped_s_26, directive_body, __E_directive_body_27, __E_directive_body_28, inline_arg, __E___E_directive_body_28_29, __E___E_directive_body_28_30, 
		__E_directive_body_31, __E___E_directive_body_31_32, __E___E_directive_body_31_33, __E_directive_body_34, __E___E_directive_body_34_35, __E___E_directive_body_34_36, __E_directive_body_37, __E_directive_body_38, __E___E_directive_body_38_39, __E___E_directive_body_38_40, 
		__E_directive_body_41, use_from, use_param, __E___E_directive_body_41_42, __E___E_directive_body_41_43, __E_directive_body_44, __E___E_directive_body_44_45, __E___E_directive_body_44_46, __E_directive_body_47, __E___E_directive_body_47_48, 
		__E___E_directive_body_47_49, __E_inline_arg_50, __E___E_inline_arg_50_51, __E_tree_path_52, __E_tree_path_53, cc_name, __E_use_param_54, __E_sep_55, sep_required, __E___56, 
		__E____57, comment, __E_comment_58, __E_comment_59, __E_comment_60, __N_0, __N_1, 
	};
	static tgf_parser& instance() {
		static tgf_parser inst;
		return inst;
	}
	tgf_parser() : idni::parser<char_type, terminal_type>(
		tgf_parser_data::grammar,
		tgf_parser_data::parser_options) {}
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
