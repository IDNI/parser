// This file is generated from a file src/tgf.tgf by
//       https://github.com/IDNI/parser/tools/tgf
//
#ifndef __TGF_PARSER_H__
#define __TGF_PARSER_H__

#include "parser.h"

namespace tgf_parser_data {

using char_type     = char;
using terminal_type = char;

inline std::vector<std::string> symbol_names{
	"", "eof", "alnum", "alpha", "space", "printable", "__", "_", "sep", "sym", 
	"cc_name", "escaped_s", "unescaped_s", "escaped_c", "unescaped_c", "syms", "start", "__E_start_0", "statement", "__E_start_1", 
	"directive", "production", "start_statement", "alternation", "conjunction", "__E_alternation_2", "__E_alternation_3", "concatenation", "__E_conjunction_4", "__E_conjunction_5", 
	"factor", "__E_concatenation_6", "__E_concatenation_7", "shorthand_rule", "__E_factor_8", "optional", "__E_factor_9", "term", "repeat", "__E_factor_10", 
	"none_or_repeat", "__E_factor_11", "neg", "__E_factor_12", "group", "__E_term_13", "optional_group", "__E_term_14", "repeat_group", "__E_term_15", 
	"terminal", "terminal_char", "terminal_string", "__E_sym_16", "__E_sym_17", "__E_sym_18", "__E_terminal_char_19", "__E_unescaped_c_20", "__E_escaped_c_21", "__E_terminal_string_22", 
	"terminal_string_char", "__E___E_terminal_string_22_23", "__E_terminal_string_24", "__E_unescaped_s_25", "__E_escaped_s_26", "directive_body", "start_dir", "__E_directive_body_27", "inline_dir", "__E_directive_body_28", 
	"inline_arg", "__E___E_directive_body_28_29", "__E___E_directive_body_28_30", "trim_children_dir", "__E_directive_body_31", "trim_children_terminals_dir", "__E_directive_body_32", "trim_all_terminals_dir", "__E_directive_body_33", "__E___E_directive_body_33_34", 
	"__E___E___E_directive_body_33_34_35", "trim_dir", "__E_directive_body_36", "use_dir", "__E_directive_body_37", "use_from", "cc_sym", "use_param", "__E___E_directive_body_37_38", "__E___E_directive_body_37_39", 
	"disable_ad_dir", "__E_directive_body_40", "__E___E_directive_body_40_41", "__E___E_directive_body_40_42", "ambiguous_dir", "__E_directive_body_43", "__E___E_directive_body_43_44", "__E___E_directive_body_43_45", "__E_syms_46", "__E_syms_47", 
	"tree_path", "__E_inline_arg_48", "__E___E_inline_arg_48_49", "__E_tree_path_50", "__E_tree_path_51", "__E_use_param_52", "__E_sep_53", "sep_required", "__E___54", "__E____55", 
	"comment", "__E_comment_56", "__E_comment_57", "__E_comment_58", "__N_0", "__N_1", 
};

inline ::idni::nonterminals<char_type, terminal_type> nts{symbol_names};

inline std::vector<terminal_type> terminals{
	'\0', '=', '>', '.', '|', '&', ':', '?', '+', 
	'*', '~', '(', ')', '[', ']', '{', '}', '_', '\'', 
	'\\', '/', 'b', 'f', 'n', 'r', 't', '"', '@', 's', 
	'a', ',', 'i', 'l', 'e', 'm', 'c', 'h', 'd', 'o', 
	'x', 'p', 'u', 'g', 'k', 'w', '-', '\t', '\r', '\n', 
	'#', 
};

inline ::idni::char_class_fns<terminal_type> char_classes =
	::idni::predefined_char_classes<char_type, terminal_type>({
		"eof",
		"alnum",
		"alpha",
		"space",
		"printable",
	}, nts);

inline struct ::idni::grammar<char_type, terminal_type>::options
	grammar_options
{
	.transform_negation = false,
	.auto_disambiguate = true,
	.shaping = {
		.to_trim = {
			6, 7, 8
		},
		.trim_terminals = true,
		.dont_trim_terminals_of = {
			9, 10, 11, 12, 13, 14
		},
		.to_inline = {
			{ 15 }
		},
		.inline_char_classes = true
	}
};

inline ::idni::parser<char_type, terminal_type>::options parser_options{
};

inline ::idni::prods<char_type, terminal_type> start_symbol{ nts(16) };

inline idni::prods<char_type, terminal_type>& productions() {
	static bool loaded = false;
	static idni::prods<char_type, terminal_type>
		p, nul(idni::lit<char_type, terminal_type>{});
	if (loaded) return p;
	#define  T(x) (idni::prods<char_type, terminal_type>{ terminals[x] })
	#define NT(x) (idni::prods<char_type, terminal_type>{ nts(x) })
//G0:   __E_start_0(17)      => _(7) statement(18).
	p(NT(17), (NT(7)+NT(18)));
//G1:   __E_start_1(19)      => null.
	p(NT(19), (nul));
//G2:   __E_start_1(19)      => __E_start_0(17) __E_start_1(19).
	p(NT(19), (NT(17)+NT(19)));
//G3:   start(16)            => __E_start_1(19) _(7).
	p(NT(16), (NT(19)+NT(7)));
//G4:   statement(18)        => directive(20).
	p(NT(18), (NT(20)));
//G5:   statement(18)        => production(21).
	p(NT(18), (NT(21)));
//G6:   start_statement(22)  => _(7) statement(18) _(7).
	p(NT(22), (NT(7)+NT(18)+NT(7)));
//G7:   production(21)       => sym(9) _(7) '=' '>' _(7) alternation(23) _(7) '.'.
	p(NT(21), (NT(9)+NT(7)+T(1)+T(2)+NT(7)+NT(23)+NT(7)+T(3)));
//G8:   __E_alternation_2(25) => _(7) '|' _(7) conjunction(24).
	p(NT(25), (NT(7)+T(4)+NT(7)+NT(24)));
//G9:   __E_alternation_3(26) => null.
	p(NT(26), (nul));
//G10:  __E_alternation_3(26) => __E_alternation_2(25) __E_alternation_3(26).
	p(NT(26), (NT(25)+NT(26)));
//G11:  alternation(23)      => conjunction(24) __E_alternation_3(26).
	p(NT(23), (NT(24)+NT(26)));
//G12:  __E_conjunction_4(28) => _(7) '&' _(7) concatenation(27).
	p(NT(28), (NT(7)+T(5)+NT(7)+NT(27)));
//G13:  __E_conjunction_5(29) => null.
	p(NT(29), (nul));
//G14:  __E_conjunction_5(29) => __E_conjunction_4(28) __E_conjunction_5(29).
	p(NT(29), (NT(28)+NT(29)));
//G15:  conjunction(24)      => concatenation(27) __E_conjunction_5(29).
	p(NT(24), (NT(27)+NT(29)));
//G16:  __E_concatenation_6(31) => __(6) factor(30).
	p(NT(31), (NT(6)+NT(30)));
//G17:  __E_concatenation_7(32) => null.
	p(NT(32), (nul));
//G18:  __E_concatenation_7(32) => __E_concatenation_6(31) __E_concatenation_7(32).
	p(NT(32), (NT(31)+NT(32)));
//G19:  concatenation(27)    => factor(30) __E_concatenation_7(32).
	p(NT(27), (NT(30)+NT(32)));
//G20:  __E_factor_8(34)     => factor(30) _(7) ':' sym(9).
	p(NT(34), (NT(30)+NT(7)+T(6)+NT(9)));
//G21:  shorthand_rule(33)   => __E_factor_8(34).
	p(NT(33), (NT(34)));
//G22:  factor(30)           => shorthand_rule(33).
	p(NT(30), (NT(33)));
//G23:  __E_factor_9(36)     => term(37) '?'.
	p(NT(36), (NT(37)+T(7)));
//G24:  optional(35)         => __E_factor_9(36).
	p(NT(35), (NT(36)));
//G25:  factor(30)           => optional(35).
	p(NT(30), (NT(35)));
//G26:  __E_factor_10(39)    => term(37) '+'.
	p(NT(39), (NT(37)+T(8)));
//G27:  repeat(38)           => __E_factor_10(39).
	p(NT(38), (NT(39)));
//G28:  factor(30)           => repeat(38).
	p(NT(30), (NT(38)));
//G29:  __E_factor_11(41)    => term(37) '*'.
	p(NT(41), (NT(37)+T(9)));
//G30:  none_or_repeat(40)   => __E_factor_11(41).
	p(NT(40), (NT(41)));
//G31:  factor(30)           => none_or_repeat(40).
	p(NT(30), (NT(40)));
//G32:  __E_factor_12(43)    => '~' term(37).
	p(NT(43), (T(10)+NT(37)));
//G33:  neg(42)              => __E_factor_12(43).
	p(NT(42), (NT(43)));
//G34:  factor(30)           => neg(42).
	p(NT(30), (NT(42)));
//G35:  factor(30)           => term(37).
	p(NT(30), (NT(37)));
//G36:  __E_term_13(45)      => '(' _(7) alternation(23) _(7) ')'.
	p(NT(45), (T(11)+NT(7)+NT(23)+NT(7)+T(12)));
//G37:  group(44)            => __E_term_13(45).
	p(NT(44), (NT(45)));
//G38:  term(37)             => group(44).
	p(NT(37), (NT(44)));
//G39:  __E_term_14(47)      => '[' _(7) alternation(23) _(7) ']'.
	p(NT(47), (T(13)+NT(7)+NT(23)+NT(7)+T(14)));
//G40:  optional_group(46)   => __E_term_14(47).
	p(NT(46), (NT(47)));
//G41:  term(37)             => optional_group(46).
	p(NT(37), (NT(46)));
//G42:  __E_term_15(49)      => '{' _(7) alternation(23) _(7) '}'.
	p(NT(49), (T(15)+NT(7)+NT(23)+NT(7)+T(16)));
//G43:  repeat_group(48)     => __E_term_15(49).
	p(NT(48), (NT(49)));
//G44:  term(37)             => repeat_group(48).
	p(NT(37), (NT(48)));
//G45:  term(37)             => terminal(50).
	p(NT(37), (NT(50)));
//G46:  term(37)             => sym(9).
	p(NT(37), (NT(9)));
//G47:  terminal(50)         => terminal_char(51).
	p(NT(50), (NT(51)));
//G48:  terminal(50)         => terminal_string(52).
	p(NT(50), (NT(52)));
//G49:  __E_sym_16(53)       => alpha(3).
	p(NT(53), (NT(3)));
//G50:  __E_sym_16(53)       => '_'.
	p(NT(53), (T(17)));
//G51:  __E_sym_17(54)       => alnum(2).
	p(NT(54), (NT(2)));
//G52:  __E_sym_17(54)       => '_'.
	p(NT(54), (T(17)));
//G53:  __E_sym_18(55)       => null.
	p(NT(55), (nul));
//G54:  __E_sym_18(55)       => __E_sym_17(54) __E_sym_18(55).
	p(NT(55), (NT(54)+NT(55)));
//G55:  sym(9)               => __E_sym_16(53) __E_sym_18(55).
	p(NT(9), (NT(53)+NT(55)));
//G56:  __E_terminal_char_19(56) => unescaped_c(14).
	p(NT(56), (NT(14)));
//G57:  __E_terminal_char_19(56) => escaped_c(13).
	p(NT(56), (NT(13)));
//G58:  terminal_char(51)    => '\'' __E_terminal_char_19(56) '\''.
	p(NT(51), (T(18)+NT(56)+T(18)));
//G59:  __E_unescaped_c_20(57) => '\''.
	p(NT(57), (T(18)));
//G60:  __E_unescaped_c_20(57) => '\\'.
	p(NT(57), (T(19)));
//G61:  __N_0(114)           => __E_unescaped_c_20(57).
	p(NT(114), (NT(57)));
//G62:  unescaped_c(14)      => printable(5) & ~( __N_0(114) ).	 # conjunctive
	p(NT(14), (NT(5)) & ~(NT(114)));
//G63:  __E_escaped_c_21(58) => '\''.
	p(NT(58), (T(18)));
//G64:  __E_escaped_c_21(58) => '\\'.
	p(NT(58), (T(19)));
//G65:  __E_escaped_c_21(58) => '/'.
	p(NT(58), (T(20)));
//G66:  __E_escaped_c_21(58) => 'b'.
	p(NT(58), (T(21)));
//G67:  __E_escaped_c_21(58) => 'f'.
	p(NT(58), (T(22)));
//G68:  __E_escaped_c_21(58) => 'n'.
	p(NT(58), (T(23)));
//G69:  __E_escaped_c_21(58) => 'r'.
	p(NT(58), (T(24)));
//G70:  __E_escaped_c_21(58) => 't'.
	p(NT(58), (T(25)));
//G71:  escaped_c(13)        => '\\' __E_escaped_c_21(58).
	p(NT(13), (T(19)+NT(58)));
//G72:  __E___E_terminal_string_22_23(61) => unescaped_s(12).
	p(NT(61), (NT(12)));
//G73:  __E___E_terminal_string_22_23(61) => escaped_s(11).
	p(NT(61), (NT(11)));
//G74:  terminal_string_char(60) => __E___E_terminal_string_22_23(61).
	p(NT(60), (NT(61)));
//G75:  __E_terminal_string_22(59) => terminal_string_char(60).
	p(NT(59), (NT(60)));
//G76:  __E_terminal_string_24(62) => null.
	p(NT(62), (nul));
//G77:  __E_terminal_string_24(62) => __E_terminal_string_22(59) __E_terminal_string_24(62).
	p(NT(62), (NT(59)+NT(62)));
//G78:  terminal_string(52)  => '"' __E_terminal_string_24(62) '"'.
	p(NT(52), (T(26)+NT(62)+T(26)));
//G79:  __E_unescaped_s_25(63) => '"'.
	p(NT(63), (T(26)));
//G80:  __E_unescaped_s_25(63) => '\\'.
	p(NT(63), (T(19)));
//G81:  __N_1(115)           => __E_unescaped_s_25(63).
	p(NT(115), (NT(63)));
//G82:  unescaped_s(12)      => printable(5) & ~( __N_1(115) ).	 # conjunctive
	p(NT(12), (NT(5)) & ~(NT(115)));
//G83:  __E_escaped_s_26(64) => '"'.
	p(NT(64), (T(26)));
//G84:  __E_escaped_s_26(64) => '\\'.
	p(NT(64), (T(19)));
//G85:  __E_escaped_s_26(64) => '/'.
	p(NT(64), (T(20)));
//G86:  __E_escaped_s_26(64) => 'b'.
	p(NT(64), (T(21)));
//G87:  __E_escaped_s_26(64) => 'f'.
	p(NT(64), (T(22)));
//G88:  __E_escaped_s_26(64) => 'n'.
	p(NT(64), (T(23)));
//G89:  __E_escaped_s_26(64) => 'r'.
	p(NT(64), (T(24)));
//G90:  __E_escaped_s_26(64) => 't'.
	p(NT(64), (T(25)));
//G91:  escaped_s(11)        => '\\' __E_escaped_s_26(64).
	p(NT(11), (T(19)+NT(64)));
//G92:  directive(20)        => '@' _(7) directive_body(65) _(7) '.'.
	p(NT(20), (T(27)+NT(7)+NT(65)+NT(7)+T(3)));
//G93:  __E_directive_body_27(67) => 's' 't' 'a' 'r' 't' __(6) sym(9).
	p(NT(67), (T(28)+T(25)+T(29)+T(24)+T(25)+NT(6)+NT(9)));
//G94:  start_dir(66)        => __E_directive_body_27(67).
	p(NT(66), (NT(67)));
//G95:  directive_body(65)   => start_dir(66).
	p(NT(65), (NT(66)));
//G96:  __E___E_directive_body_28_29(71) => _(7) ',' _(7) inline_arg(70).
	p(NT(71), (NT(7)+T(30)+NT(7)+NT(70)));
//G97:  __E___E_directive_body_28_30(72) => null.
	p(NT(72), (nul));
//G98:  __E___E_directive_body_28_30(72) => __E___E_directive_body_28_29(71) __E___E_directive_body_28_30(72).
	p(NT(72), (NT(71)+NT(72)));
//G99:  __E_directive_body_28(69) => 'i' 'n' 'l' 'i' 'n' 'e' __(6) inline_arg(70) __E___E_directive_body_28_30(72).
	p(NT(69), (T(31)+T(23)+T(32)+T(31)+T(23)+T(33)+NT(6)+NT(70)+NT(72)));
//G100: inline_dir(68)       => __E_directive_body_28(69).
	p(NT(68), (NT(69)));
//G101: directive_body(65)   => inline_dir(68).
	p(NT(65), (NT(68)));
//G102: __E_directive_body_31(74) => 't' 'r' 'i' 'm' sep(8) 'c' 'h' 'i' 'l' 'd' 'r' 'e' 'n' __(6) syms(15).
	p(NT(74), (T(25)+T(24)+T(31)+T(34)+NT(8)+T(35)+T(36)+T(31)+T(32)+T(37)+T(24)+T(33)+T(23)+NT(6)+NT(15)));
//G103: trim_children_dir(73) => __E_directive_body_31(74).
	p(NT(73), (NT(74)));
//G104: directive_body(65)   => trim_children_dir(73).
	p(NT(65), (NT(73)));
//G105: __E_directive_body_32(76) => 't' 'r' 'i' 'm' sep(8) 'c' 'h' 'i' 'l' 'd' 'r' 'e' 'n' sep(8) 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's' __(6) syms(15).
	p(NT(76), (T(25)+T(24)+T(31)+T(34)+NT(8)+T(35)+T(36)+T(31)+T(32)+T(37)+T(24)+T(33)+T(23)+NT(8)+T(25)+T(33)+T(24)+T(34)+T(31)+T(23)+T(29)+T(32)+T(28)+NT(6)+NT(15)));
//G106: trim_children_terminals_dir(75) => __E_directive_body_32(76).
	p(NT(75), (NT(76)));
//G107: directive_body(65)   => trim_children_terminals_dir(75).
	p(NT(65), (NT(75)));
//G108: __E___E___E_directive_body_33_34_35(80) => sep(8) 'c' 'h' 'i' 'l' 'd' 'r' 'e' 'n' sep(8) 'o' 'f'.
	p(NT(80), (NT(8)+T(35)+T(36)+T(31)+T(32)+T(37)+T(24)+T(33)+T(23)+NT(8)+T(38)+T(22)));
//G109: __E___E___E_directive_body_33_34_35(80) => null.
	p(NT(80), (nul));
//G110: __E___E_directive_body_33_34(79) => sep(8) 'e' 'x' 'c' 'e' 'p' 't' __E___E___E_directive_body_33_34_35(80) __(6) syms(15).
	p(NT(79), (NT(8)+T(33)+T(39)+T(35)+T(33)+T(40)+T(25)+NT(80)+NT(6)+NT(15)));
//G111: __E___E_directive_body_33_34(79) => null.
	p(NT(79), (nul));
//G112: __E_directive_body_33(78) => 't' 'r' 'i' 'm' sep(8) 'a' 'l' 'l' sep(8) 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's' __E___E_directive_body_33_34(79).
	p(NT(78), (T(25)+T(24)+T(31)+T(34)+NT(8)+T(29)+T(32)+T(32)+NT(8)+T(25)+T(33)+T(24)+T(34)+T(31)+T(23)+T(29)+T(32)+T(28)+NT(79)));
//G113: trim_all_terminals_dir(77) => __E_directive_body_33(78).
	p(NT(77), (NT(78)));
//G114: directive_body(65)   => trim_all_terminals_dir(77).
	p(NT(65), (NT(77)));
//G115: __E_directive_body_36(82) => 't' 'r' 'i' 'm' __(6) syms(15).
	p(NT(82), (T(25)+T(24)+T(31)+T(34)+NT(6)+NT(15)));
//G116: trim_dir(81)         => __E_directive_body_36(82).
	p(NT(81), (NT(82)));
//G117: directive_body(65)   => trim_dir(81).
	p(NT(65), (NT(81)));
//G118: use_from(85)         => cc_sym(86).
	p(NT(85), (NT(86)));
//G119: __E___E_directive_body_37_38(88) => _(7) ',' _(7) use_param(87).
	p(NT(88), (NT(7)+T(30)+NT(7)+NT(87)));
//G120: __E___E_directive_body_37_39(89) => null.
	p(NT(89), (nul));
//G121: __E___E_directive_body_37_39(89) => __E___E_directive_body_37_38(88) __E___E_directive_body_37_39(89).
	p(NT(89), (NT(88)+NT(89)));
//G122: __E_directive_body_37(84) => 'u' 's' 'e' __(6) use_from(85) __(6) use_param(87) __E___E_directive_body_37_39(89).
	p(NT(84), (T(41)+T(28)+T(33)+NT(6)+NT(85)+NT(6)+NT(87)+NT(89)));
//G123: use_dir(83)          => __E_directive_body_37(84).
	p(NT(83), (NT(84)));
//G124: directive_body(65)   => use_dir(83).
	p(NT(65), (NT(83)));
//G125: __E___E_directive_body_40_41(92) => 'a' 'u' 't' 'o' sep(8).
	p(NT(92), (T(29)+T(41)+T(25)+T(38)+NT(8)));
//G126: __E___E_directive_body_40_41(92) => null.
	p(NT(92), (nul));
//G127: __E___E_directive_body_40_42(93) => 'u' 'a' 't' 'i' 'o' 'n'.
	p(NT(93), (T(41)+T(29)+T(25)+T(31)+T(38)+T(23)));
//G128: __E___E_directive_body_40_42(93) => null.
	p(NT(93), (nul));
//G129: __E_directive_body_40(91) => 'd' 'i' 's' 'a' 'b' 'l' 'e' __(6) __E___E_directive_body_40_41(92) 'd' 'i' 's' 'a' 'm' 'b' 'i' 'g' __E___E_directive_body_40_42(93).
	p(NT(91), (T(37)+T(31)+T(28)+T(29)+T(21)+T(32)+T(33)+NT(6)+NT(92)+T(37)+T(31)+T(28)+T(29)+T(34)+T(21)+T(31)+T(42)+NT(93)));
//G130: disable_ad_dir(90)   => __E_directive_body_40(91).
	p(NT(90), (NT(91)));
//G131: directive_body(65)   => disable_ad_dir(90).
	p(NT(65), (NT(90)));
//G132: __E___E_directive_body_43_44(96) => _(7) ',' _(7) sym(9).
	p(NT(96), (NT(7)+T(30)+NT(7)+NT(9)));
//G133: __E___E_directive_body_43_45(97) => null.
	p(NT(97), (nul));
//G134: __E___E_directive_body_43_45(97) => __E___E_directive_body_43_44(96) __E___E_directive_body_43_45(97).
	p(NT(97), (NT(96)+NT(97)));
//G135: __E_directive_body_43(95) => 'a' 'm' 'b' 'i' 'g' 'u' 'o' 'u' 's' _(7) sym(9) __E___E_directive_body_43_45(97).
	p(NT(95), (T(29)+T(34)+T(21)+T(31)+T(42)+T(41)+T(38)+T(41)+T(28)+NT(7)+NT(9)+NT(97)));
//G136: ambiguous_dir(94)    => __E_directive_body_43(95).
	p(NT(94), (NT(95)));
//G137: directive_body(65)   => ambiguous_dir(94).
	p(NT(65), (NT(94)));
//G138: __E_syms_46(98)      => _(7) ',' _(7) sym(9).
	p(NT(98), (NT(7)+T(30)+NT(7)+NT(9)));
//G139: __E_syms_47(99)      => null.
	p(NT(99), (nul));
//G140: __E_syms_47(99)      => __E_syms_46(98) __E_syms_47(99).
	p(NT(99), (NT(98)+NT(99)));
//G141: syms(15)             => sym(9) __E_syms_47(99).
	p(NT(15), (NT(9)+NT(99)));
//G142: inline_arg(70)       => tree_path(100).
	p(NT(70), (NT(100)));
//G143: __E___E_inline_arg_48_49(102) => 'e' 's'.
	p(NT(102), (T(33)+T(28)));
//G144: __E___E_inline_arg_48_49(102) => null.
	p(NT(102), (nul));
//G145: __E_inline_arg_48(101) => 'c' 'h' 'a' 'r' sep(8) 'c' 'l' 'a' 's' 's' __E___E_inline_arg_48_49(102).
	p(NT(101), (T(35)+T(36)+T(29)+T(24)+NT(8)+T(35)+T(32)+T(29)+T(28)+T(28)+NT(102)));
//G146: cc_sym(86)           => __E_inline_arg_48(101).
	p(NT(86), (NT(101)));
//G147: inline_arg(70)       => cc_sym(86).
	p(NT(70), (NT(86)));
//G148: __E_tree_path_50(103) => _(7) '>' _(7) sym(9).
	p(NT(103), (NT(7)+T(2)+NT(7)+NT(9)));
//G149: __E_tree_path_51(104) => null.
	p(NT(104), (nul));
//G150: __E_tree_path_51(104) => __E_tree_path_50(103) __E_tree_path_51(104).
	p(NT(104), (NT(103)+NT(104)));
//G151: tree_path(100)       => sym(9) __E_tree_path_51(104).
	p(NT(100), (NT(9)+NT(104)));
//G152: __E_use_param_52(105) => 'e' 'o' 'f'.
	p(NT(105), (T(33)+T(38)+T(22)));
//G153: __E_use_param_52(105) => 'a' 'l' 'n' 'u' 'm'.
	p(NT(105), (T(29)+T(32)+T(23)+T(41)+T(34)));
//G154: __E_use_param_52(105) => 'a' 'l' 'p' 'h' 'a'.
	p(NT(105), (T(29)+T(32)+T(40)+T(36)+T(29)));
//G155: __E_use_param_52(105) => 'b' 'l' 'a' 'n' 'k'.
	p(NT(105), (T(21)+T(32)+T(29)+T(23)+T(43)));
//G156: __E_use_param_52(105) => 'c' 'n' 't' 'r' 'l'.
	p(NT(105), (T(35)+T(23)+T(25)+T(24)+T(32)));
//G157: __E_use_param_52(105) => 'd' 'i' 'g' 'i' 't'.
	p(NT(105), (T(37)+T(31)+T(42)+T(31)+T(25)));
//G158: __E_use_param_52(105) => 'g' 'r' 'a' 'p' 'h'.
	p(NT(105), (T(42)+T(24)+T(29)+T(40)+T(36)));
//G159: __E_use_param_52(105) => 'l' 'o' 'w' 'e' 'r'.
	p(NT(105), (T(32)+T(38)+T(44)+T(33)+T(24)));
//G160: __E_use_param_52(105) => 'p' 'r' 'i' 'n' 't' 'a' 'b' 'l' 'e'.
	p(NT(105), (T(40)+T(24)+T(31)+T(23)+T(25)+T(29)+T(21)+T(32)+T(33)));
//G161: __E_use_param_52(105) => 'p' 'u' 'n' 'c' 't'.
	p(NT(105), (T(40)+T(41)+T(23)+T(35)+T(25)));
//G162: __E_use_param_52(105) => 's' 'p' 'a' 'c' 'e'.
	p(NT(105), (T(28)+T(40)+T(29)+T(35)+T(33)));
//G163: __E_use_param_52(105) => 'u' 'p' 'p' 'e' 'r'.
	p(NT(105), (T(41)+T(40)+T(40)+T(33)+T(24)));
//G164: __E_use_param_52(105) => 'x' 'd' 'i' 'g' 'i' 't'.
	p(NT(105), (T(39)+T(37)+T(31)+T(42)+T(31)+T(25)));
//G165: cc_name(10)          => __E_use_param_52(105).
	p(NT(10), (NT(105)));
//G166: use_param(87)        => cc_name(10).
	p(NT(87), (NT(10)));
//G167: __E_sep_53(106)      => sep_required(107).
	p(NT(106), (NT(107)));
//G168: __E_sep_53(106)      => null.
	p(NT(106), (nul));
//G169: sep(8)               => __E_sep_53(106).
	p(NT(8), (NT(106)));
//G170: sep_required(107)    => '-'.
	p(NT(107), (T(45)));
//G171: sep_required(107)    => '_'.
	p(NT(107), (T(17)));
//G172: sep_required(107)    => __(6).
	p(NT(107), (NT(6)));
//G173: __E___54(108)        => __(6).
	p(NT(108), (NT(6)));
//G174: __E___54(108)        => null.
	p(NT(108), (nul));
//G175: _(7)                 => __E___54(108).
	p(NT(7), (NT(108)));
//G176: __E____55(109)       => space(4).
	p(NT(109), (NT(4)));
//G177: __E____55(109)       => comment(110).
	p(NT(109), (NT(110)));
//G178: __(6)                => __E____55(109) _(7).
	p(NT(6), (NT(109)+NT(7)));
//G179: __E_comment_56(111)  => printable(5).
	p(NT(111), (NT(5)));
//G180: __E_comment_56(111)  => '\t'.
	p(NT(111), (T(46)));
//G181: __E_comment_57(112)  => null.
	p(NT(112), (nul));
//G182: __E_comment_57(112)  => __E_comment_56(111) __E_comment_57(112).
	p(NT(112), (NT(111)+NT(112)));
//G183: __E_comment_58(113)  => '\r'.
	p(NT(113), (T(47)));
//G184: __E_comment_58(113)  => '\n'.
	p(NT(113), (T(48)));
//G185: __E_comment_58(113)  => eof(1).
	p(NT(113), (NT(1)));
//G186: comment(110)         => '#' __E_comment_57(112) __E_comment_58(113).
	p(NT(110), (T(49)+NT(112)+NT(113)));
	#undef T
	#undef NT
	return loaded = true, p;
}

inline ::idni::grammar<char_type, terminal_type> grammar(
	nts, productions(), start_symbol, char_classes, grammar_options);

} // namespace tgf_parser_data

struct tgf_parser : public idni::parser<char, char> {
	enum nonterminal {
		nul, eof, alnum, alpha, space, printable, __, _, sep, sym, 
		cc_name, escaped_s, unescaped_s, escaped_c, unescaped_c, syms, start, __E_start_0, statement, __E_start_1, 
		directive, production, start_statement, alternation, conjunction, __E_alternation_2, __E_alternation_3, concatenation, __E_conjunction_4, __E_conjunction_5, 
		factor, __E_concatenation_6, __E_concatenation_7, shorthand_rule, __E_factor_8, optional, __E_factor_9, term, repeat, __E_factor_10, 
		none_or_repeat, __E_factor_11, neg, __E_factor_12, group, __E_term_13, optional_group, __E_term_14, repeat_group, __E_term_15, 
		terminal, terminal_char, terminal_string, __E_sym_16, __E_sym_17, __E_sym_18, __E_terminal_char_19, __E_unescaped_c_20, __E_escaped_c_21, __E_terminal_string_22, 
		terminal_string_char, __E___E_terminal_string_22_23, __E_terminal_string_24, __E_unescaped_s_25, __E_escaped_s_26, directive_body, start_dir, __E_directive_body_27, inline_dir, __E_directive_body_28, 
		inline_arg, __E___E_directive_body_28_29, __E___E_directive_body_28_30, trim_children_dir, __E_directive_body_31, trim_children_terminals_dir, __E_directive_body_32, trim_all_terminals_dir, __E_directive_body_33, __E___E_directive_body_33_34, 
		__E___E___E_directive_body_33_34_35, trim_dir, __E_directive_body_36, use_dir, __E_directive_body_37, use_from, cc_sym, use_param, __E___E_directive_body_37_38, __E___E_directive_body_37_39, 
		disable_ad_dir, __E_directive_body_40, __E___E_directive_body_40_41, __E___E_directive_body_40_42, ambiguous_dir, __E_directive_body_43, __E___E_directive_body_43_44, __E___E_directive_body_43_45, __E_syms_46, __E_syms_47, 
		tree_path, __E_inline_arg_48, __E___E_inline_arg_48_49, __E_tree_path_50, __E_tree_path_51, __E_use_param_52, __E_sep_53, sep_required, __E___54, __E____55, 
		comment, __E_comment_56, __E_comment_57, __E_comment_58, __N_0, __N_1, 
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
