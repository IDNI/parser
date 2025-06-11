// This file is generated from a file src/tgf.tgf by
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
	"", "eof", "alnum", "alpha", "space", "printable", "__", "_", "sep", "sym", 
	"cc_name", "escaped_s", "unescaped_s", "escaped_c", "unescaped_c", "syms", "start", "__E_start_0", "statement", "__E_start_1", 
	"directive", "production", "start_statement", "__E_production_2", "production_guard", "alternation", "conjunction", "__E_alternation_3", "__E_alternation_4", "concatenation", 
	"__E_conjunction_5", "__E_conjunction_6", "factor", "__E_concatenation_7", "__E_concatenation_8", "shorthand_rule", "__E_factor_9", "optional", "__E_factor_10", "term", 
	"repeat", "__E_factor_11", "none_or_repeat", "__E_factor_12", "neg", "__E_factor_13", "group", "__E_term_14", "optional_group", "__E_term_15", 
	"repeat_group", "__E_term_16", "terminal", "terminal_char", "terminal_string", "__E_sym_17", "__E_sym_18", "__E_sym_19", "__E_terminal_char_20", "__E_unescaped_c_21", 
	"__E_escaped_c_22", "__E_terminal_string_23", "terminal_string_char", "__E___E_terminal_string_23_24", "__E_terminal_string_25", "__E_unescaped_s_26", "__E_escaped_s_27", "directive_body", "start_dir", "__E_directive_body_28", 
	"inline_dir", "__E_directive_body_29", "inline_arg", "__E___E_directive_body_29_30", "__E___E_directive_body_29_31", "trim_children_dir", "__E_directive_body_32", "trim_children_terminals_dir", "__E_directive_body_33", "trim_all_terminals_dir", 
	"__E_directive_body_34", "__E___E_directive_body_34_35", "__E___E___E_directive_body_34_35_36", "trim_dir", "__E_directive_body_37", "use_dir", "__E_directive_body_38", "use_from", "cc_sym", "use_param", 
	"__E___E_directive_body_38_39", "__E___E_directive_body_38_40", "disable_ad_dir", "__E_directive_body_41", "__E___E_directive_body_41_42", "__E___E_directive_body_41_43", "enable_prods_dir", "__E_directive_body_44", "ambiguous_dir", "__E_directive_body_45", 
	"__E_syms_46", "__E_syms_47", "tree_path", "__E_inline_arg_48", "__E___E_inline_arg_48_49", "__E_tree_path_50", "__E_tree_path_51", "__E_use_param_52", "__E_sep_53", "sep_required", 
	"__E___54", "__E____55", "comment", "__E_comment_56", "__E_comment_57", "__E_comment_58", "__N_0", "__N_1", 
};

inline ::idni::nonterminals<char_type, terminal_type> nts{symbol_names};

inline std::vector<terminal_type> terminals{
	'\0', '[', ']', '=', '>', '.', '|', '&', ':', 
	'?', '+', '*', '~', '(', ')', '{', '}', '_', '\'', 
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
//G7:   production_guard(24) => sym(9).
	p(NT(24), (NT(9)));
//G8:   __E_production_2(23) => _(7) '[' _(7) production_guard(24) _(7) ']'.
	p(NT(23), (NT(7)+T(1)+NT(7)+NT(24)+NT(7)+T(2)));
//G9:   __E_production_2(23) => null.
	p(NT(23), (nul));
//G10:  production(21)       => sym(9) __E_production_2(23) _(7) '=' '>' _(7) alternation(25) _(7) '.'.
	p(NT(21), (NT(9)+NT(23)+NT(7)+T(3)+T(4)+NT(7)+NT(25)+NT(7)+T(5)));
//G11:  __E_alternation_3(27) => _(7) '|' _(7) conjunction(26).
	p(NT(27), (NT(7)+T(6)+NT(7)+NT(26)));
//G12:  __E_alternation_4(28) => null.
	p(NT(28), (nul));
//G13:  __E_alternation_4(28) => __E_alternation_3(27) __E_alternation_4(28).
	p(NT(28), (NT(27)+NT(28)));
//G14:  alternation(25)      => conjunction(26) __E_alternation_4(28).
	p(NT(25), (NT(26)+NT(28)));
//G15:  __E_conjunction_5(30) => _(7) '&' _(7) concatenation(29).
	p(NT(30), (NT(7)+T(7)+NT(7)+NT(29)));
//G16:  __E_conjunction_6(31) => null.
	p(NT(31), (nul));
//G17:  __E_conjunction_6(31) => __E_conjunction_5(30) __E_conjunction_6(31).
	p(NT(31), (NT(30)+NT(31)));
//G18:  conjunction(26)      => concatenation(29) __E_conjunction_6(31).
	p(NT(26), (NT(29)+NT(31)));
//G19:  __E_concatenation_7(33) => __(6) factor(32).
	p(NT(33), (NT(6)+NT(32)));
//G20:  __E_concatenation_8(34) => null.
	p(NT(34), (nul));
//G21:  __E_concatenation_8(34) => __E_concatenation_7(33) __E_concatenation_8(34).
	p(NT(34), (NT(33)+NT(34)));
//G22:  concatenation(29)    => factor(32) __E_concatenation_8(34).
	p(NT(29), (NT(32)+NT(34)));
//G23:  __E_factor_9(36)     => factor(32) _(7) ':' sym(9).
	p(NT(36), (NT(32)+NT(7)+T(8)+NT(9)));
//G24:  shorthand_rule(35)   => __E_factor_9(36).
	p(NT(35), (NT(36)));
//G25:  factor(32)           => shorthand_rule(35).
	p(NT(32), (NT(35)));
//G26:  __E_factor_10(38)    => term(39) '?'.
	p(NT(38), (NT(39)+T(9)));
//G27:  optional(37)         => __E_factor_10(38).
	p(NT(37), (NT(38)));
//G28:  factor(32)           => optional(37).
	p(NT(32), (NT(37)));
//G29:  __E_factor_11(41)    => term(39) '+'.
	p(NT(41), (NT(39)+T(10)));
//G30:  repeat(40)           => __E_factor_11(41).
	p(NT(40), (NT(41)));
//G31:  factor(32)           => repeat(40).
	p(NT(32), (NT(40)));
//G32:  __E_factor_12(43)    => term(39) '*'.
	p(NT(43), (NT(39)+T(11)));
//G33:  none_or_repeat(42)   => __E_factor_12(43).
	p(NT(42), (NT(43)));
//G34:  factor(32)           => none_or_repeat(42).
	p(NT(32), (NT(42)));
//G35:  __E_factor_13(45)    => '~' term(39).
	p(NT(45), (T(12)+NT(39)));
//G36:  neg(44)              => __E_factor_13(45).
	p(NT(44), (NT(45)));
//G37:  factor(32)           => neg(44).
	p(NT(32), (NT(44)));
//G38:  factor(32)           => term(39).
	p(NT(32), (NT(39)));
//G39:  __E_term_14(47)      => '(' _(7) alternation(25) _(7) ')'.
	p(NT(47), (T(13)+NT(7)+NT(25)+NT(7)+T(14)));
//G40:  group(46)            => __E_term_14(47).
	p(NT(46), (NT(47)));
//G41:  term(39)             => group(46).
	p(NT(39), (NT(46)));
//G42:  __E_term_15(49)      => '[' _(7) alternation(25) _(7) ']'.
	p(NT(49), (T(1)+NT(7)+NT(25)+NT(7)+T(2)));
//G43:  optional_group(48)   => __E_term_15(49).
	p(NT(48), (NT(49)));
//G44:  term(39)             => optional_group(48).
	p(NT(39), (NT(48)));
//G45:  __E_term_16(51)      => '{' _(7) alternation(25) _(7) '}'.
	p(NT(51), (T(15)+NT(7)+NT(25)+NT(7)+T(16)));
//G46:  repeat_group(50)     => __E_term_16(51).
	p(NT(50), (NT(51)));
//G47:  term(39)             => repeat_group(50).
	p(NT(39), (NT(50)));
//G48:  term(39)             => terminal(52).
	p(NT(39), (NT(52)));
//G49:  term(39)             => sym(9).
	p(NT(39), (NT(9)));
//G50:  terminal(52)         => terminal_char(53).
	p(NT(52), (NT(53)));
//G51:  terminal(52)         => terminal_string(54).
	p(NT(52), (NT(54)));
//G52:  __E_sym_17(55)       => alpha(3).
	p(NT(55), (NT(3)));
//G53:  __E_sym_17(55)       => '_'.
	p(NT(55), (T(17)));
//G54:  __E_sym_18(56)       => alnum(2).
	p(NT(56), (NT(2)));
//G55:  __E_sym_18(56)       => '_'.
	p(NT(56), (T(17)));
//G56:  __E_sym_19(57)       => null.
	p(NT(57), (nul));
//G57:  __E_sym_19(57)       => __E_sym_18(56) __E_sym_19(57).
	p(NT(57), (NT(56)+NT(57)));
//G58:  sym(9)               => __E_sym_17(55) __E_sym_19(57).
	p(NT(9), (NT(55)+NT(57)));
//G59:  __E_terminal_char_20(58) => unescaped_c(14).
	p(NT(58), (NT(14)));
//G60:  __E_terminal_char_20(58) => escaped_c(13).
	p(NT(58), (NT(13)));
//G61:  terminal_char(53)    => '\'' __E_terminal_char_20(58) '\''.
	p(NT(53), (T(18)+NT(58)+T(18)));
//G62:  __E_unescaped_c_21(59) => '\''.
	p(NT(59), (T(18)));
//G63:  __E_unescaped_c_21(59) => '\\'.
	p(NT(59), (T(19)));
//G64:  __N_0(116)           => __E_unescaped_c_21(59).
	p(NT(116), (NT(59)));
//G65:  unescaped_c(14)      => printable(5) & ~( __N_0(116) ).	 # conjunctive
	p(NT(14), (NT(5)) & ~(NT(116)));
//G66:  __E_escaped_c_22(60) => '\''.
	p(NT(60), (T(18)));
//G67:  __E_escaped_c_22(60) => '\\'.
	p(NT(60), (T(19)));
//G68:  __E_escaped_c_22(60) => '/'.
	p(NT(60), (T(20)));
//G69:  __E_escaped_c_22(60) => 'b'.
	p(NT(60), (T(21)));
//G70:  __E_escaped_c_22(60) => 'f'.
	p(NT(60), (T(22)));
//G71:  __E_escaped_c_22(60) => 'n'.
	p(NT(60), (T(23)));
//G72:  __E_escaped_c_22(60) => 'r'.
	p(NT(60), (T(24)));
//G73:  __E_escaped_c_22(60) => 't'.
	p(NT(60), (T(25)));
//G74:  escaped_c(13)        => '\\' __E_escaped_c_22(60).
	p(NT(13), (T(19)+NT(60)));
//G75:  __E___E_terminal_string_23_24(63) => unescaped_s(12).
	p(NT(63), (NT(12)));
//G76:  __E___E_terminal_string_23_24(63) => escaped_s(11).
	p(NT(63), (NT(11)));
//G77:  terminal_string_char(62) => __E___E_terminal_string_23_24(63).
	p(NT(62), (NT(63)));
//G78:  __E_terminal_string_23(61) => terminal_string_char(62).
	p(NT(61), (NT(62)));
//G79:  __E_terminal_string_25(64) => null.
	p(NT(64), (nul));
//G80:  __E_terminal_string_25(64) => __E_terminal_string_23(61) __E_terminal_string_25(64).
	p(NT(64), (NT(61)+NT(64)));
//G81:  terminal_string(54)  => '"' __E_terminal_string_25(64) '"'.
	p(NT(54), (T(26)+NT(64)+T(26)));
//G82:  __E_unescaped_s_26(65) => '"'.
	p(NT(65), (T(26)));
//G83:  __E_unescaped_s_26(65) => '\\'.
	p(NT(65), (T(19)));
//G84:  __N_1(117)           => __E_unescaped_s_26(65).
	p(NT(117), (NT(65)));
//G85:  unescaped_s(12)      => printable(5) & ~( __N_1(117) ).	 # conjunctive
	p(NT(12), (NT(5)) & ~(NT(117)));
//G86:  __E_escaped_s_27(66) => '"'.
	p(NT(66), (T(26)));
//G87:  __E_escaped_s_27(66) => '\\'.
	p(NT(66), (T(19)));
//G88:  __E_escaped_s_27(66) => '/'.
	p(NT(66), (T(20)));
//G89:  __E_escaped_s_27(66) => 'b'.
	p(NT(66), (T(21)));
//G90:  __E_escaped_s_27(66) => 'f'.
	p(NT(66), (T(22)));
//G91:  __E_escaped_s_27(66) => 'n'.
	p(NT(66), (T(23)));
//G92:  __E_escaped_s_27(66) => 'r'.
	p(NT(66), (T(24)));
//G93:  __E_escaped_s_27(66) => 't'.
	p(NT(66), (T(25)));
//G94:  escaped_s(11)        => '\\' __E_escaped_s_27(66).
	p(NT(11), (T(19)+NT(66)));
//G95:  directive(20)        => '@' _(7) directive_body(67) _(7) '.'.
	p(NT(20), (T(27)+NT(7)+NT(67)+NT(7)+T(5)));
//G96:  __E_directive_body_28(69) => 's' 't' 'a' 'r' 't' __(6) sym(9).
	p(NT(69), (T(28)+T(25)+T(29)+T(24)+T(25)+NT(6)+NT(9)));
//G97:  start_dir(68)        => __E_directive_body_28(69).
	p(NT(68), (NT(69)));
//G98:  directive_body(67)   => start_dir(68).
	p(NT(67), (NT(68)));
//G99:  __E___E_directive_body_29_30(73) => _(7) ',' _(7) inline_arg(72).
	p(NT(73), (NT(7)+T(30)+NT(7)+NT(72)));
//G100: __E___E_directive_body_29_31(74) => null.
	p(NT(74), (nul));
//G101: __E___E_directive_body_29_31(74) => __E___E_directive_body_29_30(73) __E___E_directive_body_29_31(74).
	p(NT(74), (NT(73)+NT(74)));
//G102: __E_directive_body_29(71) => 'i' 'n' 'l' 'i' 'n' 'e' __(6) inline_arg(72) __E___E_directive_body_29_31(74).
	p(NT(71), (T(31)+T(23)+T(32)+T(31)+T(23)+T(33)+NT(6)+NT(72)+NT(74)));
//G103: inline_dir(70)       => __E_directive_body_29(71).
	p(NT(70), (NT(71)));
//G104: directive_body(67)   => inline_dir(70).
	p(NT(67), (NT(70)));
//G105: __E_directive_body_32(76) => 't' 'r' 'i' 'm' sep(8) 'c' 'h' 'i' 'l' 'd' 'r' 'e' 'n' __(6) syms(15).
	p(NT(76), (T(25)+T(24)+T(31)+T(34)+NT(8)+T(35)+T(36)+T(31)+T(32)+T(37)+T(24)+T(33)+T(23)+NT(6)+NT(15)));
//G106: trim_children_dir(75) => __E_directive_body_32(76).
	p(NT(75), (NT(76)));
//G107: directive_body(67)   => trim_children_dir(75).
	p(NT(67), (NT(75)));
//G108: __E_directive_body_33(78) => 't' 'r' 'i' 'm' sep(8) 'c' 'h' 'i' 'l' 'd' 'r' 'e' 'n' sep(8) 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's' __(6) syms(15).
	p(NT(78), (T(25)+T(24)+T(31)+T(34)+NT(8)+T(35)+T(36)+T(31)+T(32)+T(37)+T(24)+T(33)+T(23)+NT(8)+T(25)+T(33)+T(24)+T(34)+T(31)+T(23)+T(29)+T(32)+T(28)+NT(6)+NT(15)));
//G109: trim_children_terminals_dir(77) => __E_directive_body_33(78).
	p(NT(77), (NT(78)));
//G110: directive_body(67)   => trim_children_terminals_dir(77).
	p(NT(67), (NT(77)));
//G111: __E___E___E_directive_body_34_35_36(82) => sep(8) 'c' 'h' 'i' 'l' 'd' 'r' 'e' 'n' sep(8) 'o' 'f'.
	p(NT(82), (NT(8)+T(35)+T(36)+T(31)+T(32)+T(37)+T(24)+T(33)+T(23)+NT(8)+T(38)+T(22)));
//G112: __E___E___E_directive_body_34_35_36(82) => null.
	p(NT(82), (nul));
//G113: __E___E_directive_body_34_35(81) => sep(8) 'e' 'x' 'c' 'e' 'p' 't' __E___E___E_directive_body_34_35_36(82) __(6) syms(15).
	p(NT(81), (NT(8)+T(33)+T(39)+T(35)+T(33)+T(40)+T(25)+NT(82)+NT(6)+NT(15)));
//G114: __E___E_directive_body_34_35(81) => null.
	p(NT(81), (nul));
//G115: __E_directive_body_34(80) => 't' 'r' 'i' 'm' sep(8) 'a' 'l' 'l' sep(8) 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's' __E___E_directive_body_34_35(81).
	p(NT(80), (T(25)+T(24)+T(31)+T(34)+NT(8)+T(29)+T(32)+T(32)+NT(8)+T(25)+T(33)+T(24)+T(34)+T(31)+T(23)+T(29)+T(32)+T(28)+NT(81)));
//G116: trim_all_terminals_dir(79) => __E_directive_body_34(80).
	p(NT(79), (NT(80)));
//G117: directive_body(67)   => trim_all_terminals_dir(79).
	p(NT(67), (NT(79)));
//G118: __E_directive_body_37(84) => 't' 'r' 'i' 'm' __(6) syms(15).
	p(NT(84), (T(25)+T(24)+T(31)+T(34)+NT(6)+NT(15)));
//G119: trim_dir(83)         => __E_directive_body_37(84).
	p(NT(83), (NT(84)));
//G120: directive_body(67)   => trim_dir(83).
	p(NT(67), (NT(83)));
//G121: use_from(87)         => cc_sym(88).
	p(NT(87), (NT(88)));
//G122: __E___E_directive_body_38_39(90) => _(7) ',' _(7) use_param(89).
	p(NT(90), (NT(7)+T(30)+NT(7)+NT(89)));
//G123: __E___E_directive_body_38_40(91) => null.
	p(NT(91), (nul));
//G124: __E___E_directive_body_38_40(91) => __E___E_directive_body_38_39(90) __E___E_directive_body_38_40(91).
	p(NT(91), (NT(90)+NT(91)));
//G125: __E_directive_body_38(86) => 'u' 's' 'e' __(6) use_from(87) __(6) use_param(89) __E___E_directive_body_38_40(91).
	p(NT(86), (T(41)+T(28)+T(33)+NT(6)+NT(87)+NT(6)+NT(89)+NT(91)));
//G126: use_dir(85)          => __E_directive_body_38(86).
	p(NT(85), (NT(86)));
//G127: directive_body(67)   => use_dir(85).
	p(NT(67), (NT(85)));
//G128: __E___E_directive_body_41_42(94) => 'a' 'u' 't' 'o' sep(8).
	p(NT(94), (T(29)+T(41)+T(25)+T(38)+NT(8)));
//G129: __E___E_directive_body_41_42(94) => null.
	p(NT(94), (nul));
//G130: __E___E_directive_body_41_43(95) => 'u' 'a' 't' 'i' 'o' 'n'.
	p(NT(95), (T(41)+T(29)+T(25)+T(31)+T(38)+T(23)));
//G131: __E___E_directive_body_41_43(95) => null.
	p(NT(95), (nul));
//G132: __E_directive_body_41(93) => 'd' 'i' 's' 'a' 'b' 'l' 'e' __(6) __E___E_directive_body_41_42(94) 'd' 'i' 's' 'a' 'm' 'b' 'i' 'g' __E___E_directive_body_41_43(95).
	p(NT(93), (T(37)+T(31)+T(28)+T(29)+T(21)+T(32)+T(33)+NT(6)+NT(94)+T(37)+T(31)+T(28)+T(29)+T(34)+T(21)+T(31)+T(42)+NT(95)));
//G133: disable_ad_dir(92)   => __E_directive_body_41(93).
	p(NT(92), (NT(93)));
//G134: directive_body(67)   => disable_ad_dir(92).
	p(NT(67), (NT(92)));
//G135: __E_directive_body_44(97) => 'e' 'n' 'a' 'b' 'l' 'e' __(6) 'p' 'r' 'o' 'd' 'u' 'c' 't' 'i' 'o' 'n' 's' __(6) syms(15).
	p(NT(97), (T(33)+T(23)+T(29)+T(21)+T(32)+T(33)+NT(6)+T(40)+T(24)+T(38)+T(37)+T(41)+T(35)+T(25)+T(31)+T(38)+T(23)+T(28)+NT(6)+NT(15)));
//G136: enable_prods_dir(96) => __E_directive_body_44(97).
	p(NT(96), (NT(97)));
//G137: directive_body(67)   => enable_prods_dir(96).
	p(NT(67), (NT(96)));
//G138: __E_directive_body_45(99) => 'a' 'm' 'b' 'i' 'g' 'u' 'o' 'u' 's' __(6) syms(15).
	p(NT(99), (T(29)+T(34)+T(21)+T(31)+T(42)+T(41)+T(38)+T(41)+T(28)+NT(6)+NT(15)));
//G139: ambiguous_dir(98)    => __E_directive_body_45(99).
	p(NT(98), (NT(99)));
//G140: directive_body(67)   => ambiguous_dir(98).
	p(NT(67), (NT(98)));
//G141: __E_syms_46(100)     => _(7) ',' _(7) sym(9).
	p(NT(100), (NT(7)+T(30)+NT(7)+NT(9)));
//G142: __E_syms_47(101)     => null.
	p(NT(101), (nul));
//G143: __E_syms_47(101)     => __E_syms_46(100) __E_syms_47(101).
	p(NT(101), (NT(100)+NT(101)));
//G144: syms(15)             => sym(9) __E_syms_47(101).
	p(NT(15), (NT(9)+NT(101)));
//G145: inline_arg(72)       => tree_path(102).
	p(NT(72), (NT(102)));
//G146: __E___E_inline_arg_48_49(104) => 'e' 's'.
	p(NT(104), (T(33)+T(28)));
//G147: __E___E_inline_arg_48_49(104) => null.
	p(NT(104), (nul));
//G148: __E_inline_arg_48(103) => 'c' 'h' 'a' 'r' sep(8) 'c' 'l' 'a' 's' 's' __E___E_inline_arg_48_49(104).
	p(NT(103), (T(35)+T(36)+T(29)+T(24)+NT(8)+T(35)+T(32)+T(29)+T(28)+T(28)+NT(104)));
//G149: cc_sym(88)           => __E_inline_arg_48(103).
	p(NT(88), (NT(103)));
//G150: inline_arg(72)       => cc_sym(88).
	p(NT(72), (NT(88)));
//G151: __E_tree_path_50(105) => _(7) '>' _(7) sym(9).
	p(NT(105), (NT(7)+T(4)+NT(7)+NT(9)));
//G152: __E_tree_path_51(106) => null.
	p(NT(106), (nul));
//G153: __E_tree_path_51(106) => __E_tree_path_50(105) __E_tree_path_51(106).
	p(NT(106), (NT(105)+NT(106)));
//G154: tree_path(102)       => sym(9) __E_tree_path_51(106).
	p(NT(102), (NT(9)+NT(106)));
//G155: __E_use_param_52(107) => 'e' 'o' 'f'.
	p(NT(107), (T(33)+T(38)+T(22)));
//G156: __E_use_param_52(107) => 'a' 'l' 'n' 'u' 'm'.
	p(NT(107), (T(29)+T(32)+T(23)+T(41)+T(34)));
//G157: __E_use_param_52(107) => 'a' 'l' 'p' 'h' 'a'.
	p(NT(107), (T(29)+T(32)+T(40)+T(36)+T(29)));
//G158: __E_use_param_52(107) => 'b' 'l' 'a' 'n' 'k'.
	p(NT(107), (T(21)+T(32)+T(29)+T(23)+T(43)));
//G159: __E_use_param_52(107) => 'c' 'n' 't' 'r' 'l'.
	p(NT(107), (T(35)+T(23)+T(25)+T(24)+T(32)));
//G160: __E_use_param_52(107) => 'd' 'i' 'g' 'i' 't'.
	p(NT(107), (T(37)+T(31)+T(42)+T(31)+T(25)));
//G161: __E_use_param_52(107) => 'g' 'r' 'a' 'p' 'h'.
	p(NT(107), (T(42)+T(24)+T(29)+T(40)+T(36)));
//G162: __E_use_param_52(107) => 'l' 'o' 'w' 'e' 'r'.
	p(NT(107), (T(32)+T(38)+T(44)+T(33)+T(24)));
//G163: __E_use_param_52(107) => 'p' 'r' 'i' 'n' 't' 'a' 'b' 'l' 'e'.
	p(NT(107), (T(40)+T(24)+T(31)+T(23)+T(25)+T(29)+T(21)+T(32)+T(33)));
//G164: __E_use_param_52(107) => 'p' 'u' 'n' 'c' 't'.
	p(NT(107), (T(40)+T(41)+T(23)+T(35)+T(25)));
//G165: __E_use_param_52(107) => 's' 'p' 'a' 'c' 'e'.
	p(NT(107), (T(28)+T(40)+T(29)+T(35)+T(33)));
//G166: __E_use_param_52(107) => 'u' 'p' 'p' 'e' 'r'.
	p(NT(107), (T(41)+T(40)+T(40)+T(33)+T(24)));
//G167: __E_use_param_52(107) => 'x' 'd' 'i' 'g' 'i' 't'.
	p(NT(107), (T(39)+T(37)+T(31)+T(42)+T(31)+T(25)));
//G168: cc_name(10)          => __E_use_param_52(107).
	p(NT(10), (NT(107)));
//G169: use_param(89)        => cc_name(10).
	p(NT(89), (NT(10)));
//G170: __E_sep_53(108)      => sep_required(109).
	p(NT(108), (NT(109)));
//G171: __E_sep_53(108)      => null.
	p(NT(108), (nul));
//G172: sep(8)               => __E_sep_53(108).
	p(NT(8), (NT(108)));
//G173: sep_required(109)    => '-'.
	p(NT(109), (T(45)));
//G174: sep_required(109)    => '_'.
	p(NT(109), (T(17)));
//G175: sep_required(109)    => __(6).
	p(NT(109), (NT(6)));
//G176: __E___54(110)        => __(6).
	p(NT(110), (NT(6)));
//G177: __E___54(110)        => null.
	p(NT(110), (nul));
//G178: _(7)                 => __E___54(110).
	p(NT(7), (NT(110)));
//G179: __E____55(111)       => space(4).
	p(NT(111), (NT(4)));
//G180: __E____55(111)       => comment(112).
	p(NT(111), (NT(112)));
//G181: __(6)                => __E____55(111) _(7).
	p(NT(6), (NT(111)+NT(7)));
//G182: __E_comment_56(113)  => printable(5).
	p(NT(113), (NT(5)));
//G183: __E_comment_56(113)  => '\t'.
	p(NT(113), (T(46)));
//G184: __E_comment_57(114)  => null.
	p(NT(114), (nul));
//G185: __E_comment_57(114)  => __E_comment_56(113) __E_comment_57(114).
	p(NT(114), (NT(113)+NT(114)));
//G186: __E_comment_58(115)  => '\r'.
	p(NT(115), (T(47)));
//G187: __E_comment_58(115)  => '\n'.
	p(NT(115), (T(48)));
//G188: __E_comment_58(115)  => eof(1).
	p(NT(115), (NT(1)));
//G189: comment(112)         => '#' __E_comment_57(114) __E_comment_58(115).
	p(NT(112), (T(49)+NT(114)+NT(115)));
	#undef T
	#undef NT
	return loaded = true, p;
}

inline ::idni::grammar<char_type, terminal_type> grammar(
	nts, productions(), start_symbol, char_classes, grammar_options);

} // namespace tgf_parser_data

struct tgf_parser_nonterminals {
	enum nonterminal {
		nul, eof, alnum, alpha, space, printable, __, _, sep, sym, 
		cc_name, escaped_s, unescaped_s, escaped_c, unescaped_c, syms, start, __E_start_0, statement, __E_start_1, 
		directive, production, start_statement, __E_production_2, production_guard, alternation, conjunction, __E_alternation_3, __E_alternation_4, concatenation, 
		__E_conjunction_5, __E_conjunction_6, factor, __E_concatenation_7, __E_concatenation_8, shorthand_rule, __E_factor_9, optional, __E_factor_10, term, 
		repeat, __E_factor_11, none_or_repeat, __E_factor_12, neg, __E_factor_13, group, __E_term_14, optional_group, __E_term_15, 
		repeat_group, __E_term_16, terminal, terminal_char, terminal_string, __E_sym_17, __E_sym_18, __E_sym_19, __E_terminal_char_20, __E_unescaped_c_21, 
		__E_escaped_c_22, __E_terminal_string_23, terminal_string_char, __E___E_terminal_string_23_24, __E_terminal_string_25, __E_unescaped_s_26, __E_escaped_s_27, directive_body, start_dir, __E_directive_body_28, 
		inline_dir, __E_directive_body_29, inline_arg, __E___E_directive_body_29_30, __E___E_directive_body_29_31, trim_children_dir, __E_directive_body_32, trim_children_terminals_dir, __E_directive_body_33, trim_all_terminals_dir, 
		__E_directive_body_34, __E___E_directive_body_34_35, __E___E___E_directive_body_34_35_36, trim_dir, __E_directive_body_37, use_dir, __E_directive_body_38, use_from, cc_sym, use_param, 
		__E___E_directive_body_38_39, __E___E_directive_body_38_40, disable_ad_dir, __E_directive_body_41, __E___E_directive_body_41_42, __E___E_directive_body_41_43, enable_prods_dir, __E_directive_body_44, ambiguous_dir, __E_directive_body_45, 
		__E_syms_46, __E_syms_47, tree_path, __E_inline_arg_48, __E___E_inline_arg_48_49, __E_tree_path_50, __E_tree_path_51, __E_use_param_52, __E_sep_53, sep_required, 
		__E___54, __E____55, comment, __E_comment_56, __E_comment_57, __E_comment_58, __N_0, __N_1, 
	};
};

struct tgf_parser : public idni::parser<char, char>, public tgf_parser_nonterminals {
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
