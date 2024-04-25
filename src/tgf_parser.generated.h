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
	"", "eof", "alnum", "alpha", "space", "printable", "__", "_", "digits", "chars", 
	"charvar", "start", "__E_start_0", "statement", "__E_start_1", "directive", "production", "start_statement", "__E___2", "__E____3", 
	"comment", "__E_comment_4", "__E_comment_5", "__E_comment_6", "sep", "__E_sep_7", "sep_required", "terminal_char", "__E_terminal_char_8", "unescaped_c", 
	"escaped_c", "__E_unescaped_c_9", "__E_escaped_c_10", "terminal_string", "terminal_string_char", "__E_terminal_string_11", "unescaped_s", "escaped_s", "__E_unescaped_s_12", "__E_escaped_s_13", 
	"terminal", "sym", "__E_sym_14", "__E_sym_15", "__E_sym_16", "alternation", "conjunction", "__E_alternation_17", "__E_alternation_18", "concatenation", 
	"__E_conjunction_19", "__E_conjunction_20", "factor", "__E_concatenation_21", "__E_concatenation_22", "shorthand_rule", "optional", "repeat", "none_or_repeat", "neg", 
	"term", "group", "optional_group", "repeat_group", "directive_body", "start_dir", "inline_dir", "trim_children_dir", "trim_terminals", "trim_dir", 
	"use_dir", "boolean_dir", "nodisambig_dir", "__E_trim_dir_23", "__E_trim_dir_24", "trim_terminals_dir", "__E_trim_children_dir_25", "__E_trim_children_dir_26", "inline_arg", "__E_inline_dir_27", 
	"__E_inline_dir_28", "tree_path", "char_classes_sym", "__E_tree_path_29", "__E_tree_path_30", "__E_char_classes_sym_31", "use_from", "use_param", "__E_use_dir_32", "__E_use_dir_33", 
	"__E_use_dir_34", "__E_use_dir_35", "char_class_name", "__E_boolean_dir_36", "boolean_action", "boolean_dir_name", "autodisambig_sym", "enable_sym", "disable_sym", "__E_autodisambig_sym_37", 
	"disambig_sym", "__E_disambig_sym_38", "__E___E_disambig_sym_38_39", "__E___E___E_disambig_sym_38_39_40", "nodisambig_sym", "__E_nodisambig_dir_41", "__E_nodisambig_dir_42", "__E_nodisambig_sym_43", "__E___E_nodisambig_sym_43_44", "__E___E_nodisambig_sym_43_45", 
	"__E_nodisambig_sym_46", "__N_0", "__N_1", 
};

static inline ::idni::nonterminals<char_type, terminal_type> nts{symbol_names};

static inline std::vector<terminal_type> terminals{
	'\0', '\t', '\r', '\n', '#', '-', '_', '\'', '\\', 
	'/', 'b', 'f', 'n', 'r', 't', '"', '=', '>', '.', 
	'|', '&', ':', '~', '?', '+', '*', '(', ')', '[', 
	']', '{', '}', '@', 's', 'a', ',', 'i', 'm', 'l', 
	'e', 'c', 'h', 'd', 'u', 'o', 'p', 'k', 'g', 'w', 
	'x', 
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
			6, 7
		},
		.trim_terminals = false,
		.to_inline = {
			{ 8 },
			{ 9 },
			{ 10 }
		},
		.inline_char_classes = true
	}
};
static inline ::idni::parser<char_type, terminal_type>::options parser_options{
};

static inline ::idni::prods<char_type, terminal_type> start_symbol{ nts(11) };

static inline idni::prods<char_type, terminal_type>& productions() {
	static bool loaded = false;
	static idni::prods<char_type, terminal_type>
		p, nul(idni::lit<char_type, terminal_type>{});
	if (loaded) return p;
	#define  T(x) (idni::prods<char_type, terminal_type>{ terminals[x] })
	#define NT(x) (idni::prods<char_type, terminal_type>{ nts(x) })
//G0:   __E_start_0(12)      => _(7) statement(13).
	p(NT(12), (NT(7)+NT(13)));
//G1:   __E_start_1(14)      => null.
	p(NT(14), (nul));
//G2:   __E_start_1(14)      => __E_start_0(12) __E_start_1(14).
	p(NT(14), (NT(12)+NT(14)));
//G3:   start(11)            => __E_start_1(14) _(7).
	p(NT(11), (NT(14)+NT(7)));
//G4:   statement(13)        => directive(15).
	p(NT(13), (NT(15)));
//G5:   statement(13)        => production(16).
	p(NT(13), (NT(16)));
//G6:   start_statement(17)  => _(7) statement(13) _(7).
	p(NT(17), (NT(7)+NT(13)+NT(7)));
//G7:   __E___2(18)          => __(6).
	p(NT(18), (NT(6)));
//G8:   __E___2(18)          => null.
	p(NT(18), (nul));
//G9:   _(7)                 => __E___2(18).
	p(NT(7), (NT(18)));
//G10:  __E____3(19)         => space(4).
	p(NT(19), (NT(4)));
//G11:  __E____3(19)         => comment(20).
	p(NT(19), (NT(20)));
//G12:  __(6)                => __E____3(19) _(7).
	p(NT(6), (NT(19)+NT(7)));
//G13:  __E_comment_4(21)    => printable(5).
	p(NT(21), (NT(5)));
//G14:  __E_comment_4(21)    => '\t'.
	p(NT(21), (T(1)));
//G15:  __E_comment_5(22)    => null.
	p(NT(22), (nul));
//G16:  __E_comment_5(22)    => __E_comment_4(21) __E_comment_5(22).
	p(NT(22), (NT(21)+NT(22)));
//G17:  __E_comment_6(23)    => '\r'.
	p(NT(23), (T(2)));
//G18:  __E_comment_6(23)    => '\n'.
	p(NT(23), (T(3)));
//G19:  __E_comment_6(23)    => eof(1).
	p(NT(23), (NT(1)));
//G20:  comment(20)          => '#' __E_comment_5(22) __E_comment_6(23).
	p(NT(20), (T(4)+NT(22)+NT(23)));
//G21:  __E_sep_7(25)        => sep_required(26).
	p(NT(25), (NT(26)));
//G22:  __E_sep_7(25)        => null.
	p(NT(25), (nul));
//G23:  sep(24)              => __E_sep_7(25).
	p(NT(24), (NT(25)));
//G24:  sep_required(26)     => '-'.
	p(NT(26), (T(5)));
//G25:  sep_required(26)     => '_'.
	p(NT(26), (T(6)));
//G26:  sep_required(26)     => __(6).
	p(NT(26), (NT(6)));
//G27:  __E_terminal_char_8(28) => unescaped_c(29).
	p(NT(28), (NT(29)));
//G28:  __E_terminal_char_8(28) => escaped_c(30).
	p(NT(28), (NT(30)));
//G29:  terminal_char(27)    => '\'' __E_terminal_char_8(28) '\''.
	p(NT(27), (T(7)+NT(28)+T(7)));
//G30:  __E_unescaped_c_9(31) => '\''.
	p(NT(31), (T(7)));
//G31:  __E_unescaped_c_9(31) => '\\'.
	p(NT(31), (T(8)));
//G32:  __N_0(111)           => __E_unescaped_c_9(31).
	p(NT(111), (NT(31)));
//G33:  unescaped_c(29)      => printable(5) & ~( __N_0(111) ).	 # conjunctive
	p(NT(29), (NT(5)) & ~(NT(111)));
//G34:  __E_escaped_c_10(32) => '\''.
	p(NT(32), (T(7)));
//G35:  __E_escaped_c_10(32) => '\\'.
	p(NT(32), (T(8)));
//G36:  __E_escaped_c_10(32) => '/'.
	p(NT(32), (T(9)));
//G37:  __E_escaped_c_10(32) => 'b'.
	p(NT(32), (T(10)));
//G38:  __E_escaped_c_10(32) => 'f'.
	p(NT(32), (T(11)));
//G39:  __E_escaped_c_10(32) => 'n'.
	p(NT(32), (T(12)));
//G40:  __E_escaped_c_10(32) => 'r'.
	p(NT(32), (T(13)));
//G41:  __E_escaped_c_10(32) => 't'.
	p(NT(32), (T(14)));
//G42:  escaped_c(30)        => '\\' __E_escaped_c_10(32).
	p(NT(30), (T(8)+NT(32)));
//G43:  __E_terminal_string_11(35) => null.
	p(NT(35), (nul));
//G44:  __E_terminal_string_11(35) => terminal_string_char(34) __E_terminal_string_11(35).
	p(NT(35), (NT(34)+NT(35)));
//G45:  terminal_string(33)  => '"' __E_terminal_string_11(35) '"'.
	p(NT(33), (T(15)+NT(35)+T(15)));
//G46:  terminal_string_char(34) => unescaped_s(36).
	p(NT(34), (NT(36)));
//G47:  terminal_string_char(34) => escaped_s(37).
	p(NT(34), (NT(37)));
//G48:  __E_unescaped_s_12(38) => '"'.
	p(NT(38), (T(15)));
//G49:  __E_unescaped_s_12(38) => '\\'.
	p(NT(38), (T(8)));
//G50:  __N_1(112)           => __E_unescaped_s_12(38).
	p(NT(112), (NT(38)));
//G51:  unescaped_s(36)      => printable(5) & ~( __N_1(112) ).	 # conjunctive
	p(NT(36), (NT(5)) & ~(NT(112)));
//G52:  __E_escaped_s_13(39) => '"'.
	p(NT(39), (T(15)));
//G53:  __E_escaped_s_13(39) => '\\'.
	p(NT(39), (T(8)));
//G54:  __E_escaped_s_13(39) => '/'.
	p(NT(39), (T(9)));
//G55:  __E_escaped_s_13(39) => 'b'.
	p(NT(39), (T(10)));
//G56:  __E_escaped_s_13(39) => 'f'.
	p(NT(39), (T(11)));
//G57:  __E_escaped_s_13(39) => 'n'.
	p(NT(39), (T(12)));
//G58:  __E_escaped_s_13(39) => 'r'.
	p(NT(39), (T(13)));
//G59:  __E_escaped_s_13(39) => 't'.
	p(NT(39), (T(14)));
//G60:  escaped_s(37)        => '\\' __E_escaped_s_13(39).
	p(NT(37), (T(8)+NT(39)));
//G61:  terminal(40)         => terminal_char(27).
	p(NT(40), (NT(27)));
//G62:  terminal(40)         => terminal_string(33).
	p(NT(40), (NT(33)));
//G63:  __E_sym_14(42)       => alpha(3).
	p(NT(42), (NT(3)));
//G64:  __E_sym_14(42)       => '_'.
	p(NT(42), (T(6)));
//G65:  __E_sym_15(43)       => alnum(2).
	p(NT(43), (NT(2)));
//G66:  __E_sym_15(43)       => '_'.
	p(NT(43), (T(6)));
//G67:  __E_sym_16(44)       => null.
	p(NT(44), (nul));
//G68:  __E_sym_16(44)       => __E_sym_15(43) __E_sym_16(44).
	p(NT(44), (NT(43)+NT(44)));
//G69:  sym(41)              => __E_sym_14(42) __E_sym_16(44).
	p(NT(41), (NT(42)+NT(44)));
//G70:  production(16)       => sym(41) _(7) '=' '>' _(7) alternation(45) _(7) '.'.
	p(NT(16), (NT(41)+NT(7)+T(16)+T(17)+NT(7)+NT(45)+NT(7)+T(18)));
//G71:  __E_alternation_17(47) => _(7) '|' _(7) conjunction(46).
	p(NT(47), (NT(7)+T(19)+NT(7)+NT(46)));
//G72:  __E_alternation_18(48) => null.
	p(NT(48), (nul));
//G73:  __E_alternation_18(48) => __E_alternation_17(47) __E_alternation_18(48).
	p(NT(48), (NT(47)+NT(48)));
//G74:  alternation(45)      => conjunction(46) __E_alternation_18(48).
	p(NT(45), (NT(46)+NT(48)));
//G75:  __E_conjunction_19(50) => _(7) '&' _(7) concatenation(49).
	p(NT(50), (NT(7)+T(20)+NT(7)+NT(49)));
//G76:  __E_conjunction_20(51) => null.
	p(NT(51), (nul));
//G77:  __E_conjunction_20(51) => __E_conjunction_19(50) __E_conjunction_20(51).
	p(NT(51), (NT(50)+NT(51)));
//G78:  conjunction(46)      => concatenation(49) __E_conjunction_20(51).
	p(NT(46), (NT(49)+NT(51)));
//G79:  __E_concatenation_21(53) => __(6) factor(52).
	p(NT(53), (NT(6)+NT(52)));
//G80:  __E_concatenation_22(54) => null.
	p(NT(54), (nul));
//G81:  __E_concatenation_22(54) => __E_concatenation_21(53) __E_concatenation_22(54).
	p(NT(54), (NT(53)+NT(54)));
//G82:  concatenation(49)    => factor(52) __E_concatenation_22(54).
	p(NT(49), (NT(52)+NT(54)));
//G83:  factor(52)           => shorthand_rule(55).
	p(NT(52), (NT(55)));
//G84:  factor(52)           => optional(56).
	p(NT(52), (NT(56)));
//G85:  factor(52)           => repeat(57).
	p(NT(52), (NT(57)));
//G86:  factor(52)           => none_or_repeat(58).
	p(NT(52), (NT(58)));
//G87:  factor(52)           => neg(59).
	p(NT(52), (NT(59)));
//G88:  factor(52)           => term(60).
	p(NT(52), (NT(60)));
//G89:  term(60)             => group(61).
	p(NT(60), (NT(61)));
//G90:  term(60)             => optional_group(62).
	p(NT(60), (NT(62)));
//G91:  term(60)             => repeat_group(63).
	p(NT(60), (NT(63)));
//G92:  term(60)             => terminal(40).
	p(NT(60), (NT(40)));
//G93:  term(60)             => sym(41).
	p(NT(60), (NT(41)));
//G94:  shorthand_rule(55)   => factor(52) _(7) ':' sym(41).
	p(NT(55), (NT(52)+NT(7)+T(21)+NT(41)));
//G95:  neg(59)              => '~' term(60).
	p(NT(59), (T(22)+NT(60)));
//G96:  optional(56)         => term(60) '?'.
	p(NT(56), (NT(60)+T(23)));
//G97:  repeat(57)           => term(60) '+'.
	p(NT(57), (NT(60)+T(24)));
//G98:  none_or_repeat(58)   => term(60) '*'.
	p(NT(58), (NT(60)+T(25)));
//G99:  group(61)            => '(' _(7) alternation(45) _(7) ')'.
	p(NT(61), (T(26)+NT(7)+NT(45)+NT(7)+T(27)));
//G100: optional_group(62)   => '[' _(7) alternation(45) _(7) ']'.
	p(NT(62), (T(28)+NT(7)+NT(45)+NT(7)+T(29)));
//G101: repeat_group(63)     => '{' _(7) alternation(45) _(7) '}'.
	p(NT(63), (T(30)+NT(7)+NT(45)+NT(7)+T(31)));
//G102: directive(15)        => '@' _(7) directive_body(64) _(7) '.'.
	p(NT(15), (T(32)+NT(7)+NT(64)+NT(7)+T(18)));
//G103: directive_body(64)   => start_dir(65).
	p(NT(64), (NT(65)));
//G104: directive_body(64)   => inline_dir(66).
	p(NT(64), (NT(66)));
//G105: directive_body(64)   => trim_children_dir(67).
	p(NT(64), (NT(67)));
//G106: directive_body(64)   => trim_terminals(68).
	p(NT(64), (NT(68)));
//G107: directive_body(64)   => trim_dir(69).
	p(NT(64), (NT(69)));
//G108: directive_body(64)   => use_dir(70).
	p(NT(64), (NT(70)));
//G109: directive_body(64)   => boolean_dir(71).
	p(NT(64), (NT(71)));
//G110: directive_body(64)   => nodisambig_dir(72).
	p(NT(64), (NT(72)));
//G111: start_dir(65)        => 's' 't' 'a' 'r' 't' __(6) sym(41).
	p(NT(65), (T(33)+T(14)+T(34)+T(13)+T(14)+NT(6)+NT(41)));
//G112: __E_trim_dir_23(73)  => _(7) ',' _(7) sym(41).
	p(NT(73), (NT(7)+T(35)+NT(7)+NT(41)));
//G113: __E_trim_dir_24(74)  => null.
	p(NT(74), (nul));
//G114: __E_trim_dir_24(74)  => __E_trim_dir_23(73) __E_trim_dir_24(74).
	p(NT(74), (NT(73)+NT(74)));
//G115: trim_dir(69)         => 't' 'r' 'i' 'm' __(6) sym(41) __E_trim_dir_24(74).
	p(NT(69), (T(14)+T(13)+T(36)+T(37)+NT(6)+NT(41)+NT(74)));
//G116: trim_terminals_dir(75) => 't' 'r' 'i' 'm' sep(24) 'a' 'l' 'l' sep(24) 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
	p(NT(75), (T(14)+T(13)+T(36)+T(37)+NT(24)+T(34)+T(38)+T(38)+NT(24)+T(14)+T(39)+T(13)+T(37)+T(36)+T(12)+T(34)+T(38)+T(33)));
//G117: __E_trim_children_dir_25(76) => _(7) ',' _(7) sym(41).
	p(NT(76), (NT(7)+T(35)+NT(7)+NT(41)));
//G118: __E_trim_children_dir_26(77) => null.
	p(NT(77), (nul));
//G119: __E_trim_children_dir_26(77) => __E_trim_children_dir_25(76) __E_trim_children_dir_26(77).
	p(NT(77), (NT(76)+NT(77)));
//G120: trim_children_dir(67) => 't' 'r' 'i' 'm' sep(24) 'c' 'h' 'i' 'l' 'd' 'r' 'e' 'n' __(6) sym(41) __E_trim_children_dir_26(77).
	p(NT(67), (T(14)+T(13)+T(36)+T(37)+NT(24)+T(40)+T(41)+T(36)+T(38)+T(42)+T(13)+T(39)+T(12)+NT(6)+NT(41)+NT(77)));
//G121: __E_inline_dir_27(79) => _(7) ',' _(7) inline_arg(78).
	p(NT(79), (NT(7)+T(35)+NT(7)+NT(78)));
//G122: __E_inline_dir_28(80) => null.
	p(NT(80), (nul));
//G123: __E_inline_dir_28(80) => __E_inline_dir_27(79) __E_inline_dir_28(80).
	p(NT(80), (NT(79)+NT(80)));
//G124: inline_dir(66)       => 'i' 'n' 'l' 'i' 'n' 'e' __(6) inline_arg(78) __E_inline_dir_28(80).
	p(NT(66), (T(36)+T(12)+T(38)+T(36)+T(12)+T(39)+NT(6)+NT(78)+NT(80)));
//G125: inline_arg(78)       => tree_path(81).
	p(NT(78), (NT(81)));
//G126: inline_arg(78)       => char_classes_sym(82).
	p(NT(78), (NT(82)));
//G127: __E_tree_path_29(83) => _(7) '>' _(7) sym(41).
	p(NT(83), (NT(7)+T(17)+NT(7)+NT(41)));
//G128: __E_tree_path_30(84) => null.
	p(NT(84), (nul));
//G129: __E_tree_path_30(84) => __E_tree_path_29(83) __E_tree_path_30(84).
	p(NT(84), (NT(83)+NT(84)));
//G130: tree_path(81)        => sym(41) __E_tree_path_30(84).
	p(NT(81), (NT(41)+NT(84)));
//G131: __E_char_classes_sym_31(85) => 'e' 's'.
	p(NT(85), (T(39)+T(33)));
//G132: __E_char_classes_sym_31(85) => null.
	p(NT(85), (nul));
//G133: char_classes_sym(82) => 'c' 'h' 'a' 'r' sep(24) 'c' 'l' 'a' 's' 's' __E_char_classes_sym_31(85).
	p(NT(82), (T(40)+T(41)+T(34)+T(13)+NT(24)+T(40)+T(38)+T(34)+T(33)+T(33)+NT(85)));
//G134: __E_use_dir_32(88)   => _(7) ',' _(7) use_param(87).
	p(NT(88), (NT(7)+T(35)+NT(7)+NT(87)));
//G135: __E_use_dir_33(89)   => null.
	p(NT(89), (nul));
//G136: __E_use_dir_33(89)   => __E_use_dir_32(88) __E_use_dir_33(89).
	p(NT(89), (NT(88)+NT(89)));
//G137: use_dir(70)          => 'u' 's' 'e' __(6) use_from(86) __(6) use_param(87) __E_use_dir_33(89).
	p(NT(70), (T(43)+T(33)+T(39)+NT(6)+NT(86)+NT(6)+NT(87)+NT(89)));
//G138: __E_use_dir_34(90)   => _(7) ',' _(7) use_param(87).
	p(NT(90), (NT(7)+T(35)+NT(7)+NT(87)));
//G139: __E_use_dir_35(91)   => null.
	p(NT(91), (nul));
//G140: __E_use_dir_35(91)   => __E_use_dir_34(90) __E_use_dir_35(91).
	p(NT(91), (NT(90)+NT(91)));
//G141: use_dir(70)          => 'u' 's' 'e' '_' 'c' 'h' 'a' 'r' '_' 'c' 'l' 'a' 's' 's' __(6) use_param(87) __E_use_dir_35(91).
	p(NT(70), (T(43)+T(33)+T(39)+T(6)+T(40)+T(41)+T(34)+T(13)+T(6)+T(40)+T(38)+T(34)+T(33)+T(33)+NT(6)+NT(87)+NT(91)));
//G142: use_from(86)         => char_classes_sym(82).
	p(NT(86), (NT(82)));
//G143: use_param(87)        => char_class_name(92).
	p(NT(87), (NT(92)));
//G144: char_class_name(92)  => 'e' 'o' 'f'.
	p(NT(92), (T(39)+T(44)+T(11)));
//G145: char_class_name(92)  => 'a' 'l' 'n' 'u' 'm'.
	p(NT(92), (T(34)+T(38)+T(12)+T(43)+T(37)));
//G146: char_class_name(92)  => 'a' 'l' 'p' 'h' 'a'.
	p(NT(92), (T(34)+T(38)+T(45)+T(41)+T(34)));
//G147: char_class_name(92)  => 'b' 'l' 'a' 'n' 'k'.
	p(NT(92), (T(10)+T(38)+T(34)+T(12)+T(46)));
//G148: char_class_name(92)  => 'c' 'n' 't' 'r' 'l'.
	p(NT(92), (T(40)+T(12)+T(14)+T(13)+T(38)));
//G149: char_class_name(92)  => 'd' 'i' 'g' 'i' 't'.
	p(NT(92), (T(42)+T(36)+T(47)+T(36)+T(14)));
//G150: char_class_name(92)  => 'g' 'r' 'a' 'p' 'h'.
	p(NT(92), (T(47)+T(13)+T(34)+T(45)+T(41)));
//G151: char_class_name(92)  => 'l' 'o' 'w' 'e' 'r'.
	p(NT(92), (T(38)+T(44)+T(48)+T(39)+T(13)));
//G152: char_class_name(92)  => 'p' 'r' 'i' 'n' 't' 'a' 'b' 'l' 'e'.
	p(NT(92), (T(45)+T(13)+T(36)+T(12)+T(14)+T(34)+T(10)+T(38)+T(39)));
//G153: char_class_name(92)  => 'p' 'u' 'n' 'c' 't'.
	p(NT(92), (T(45)+T(43)+T(12)+T(40)+T(14)));
//G154: char_class_name(92)  => 's' 'p' 'a' 'c' 'e'.
	p(NT(92), (T(33)+T(45)+T(34)+T(40)+T(39)));
//G155: char_class_name(92)  => 'u' 'p' 'p' 'e' 'r'.
	p(NT(92), (T(43)+T(45)+T(45)+T(39)+T(13)));
//G156: char_class_name(92)  => 'x' 'd' 'i' 'g' 'i' 't'.
	p(NT(92), (T(49)+T(42)+T(36)+T(47)+T(36)+T(14)));
//G157: __E_boolean_dir_36(93) => boolean_action(94) __(6).
	p(NT(93), (NT(94)+NT(6)));
//G158: __E_boolean_dir_36(93) => null.
	p(NT(93), (nul));
//G159: boolean_dir(71)      => __E_boolean_dir_36(93) boolean_dir_name(95).
	p(NT(71), (NT(93)+NT(95)));
//G160: boolean_dir_name(95) => autodisambig_sym(96).
	p(NT(95), (NT(96)));
//G161: boolean_action(94)   => enable_sym(97).
	p(NT(94), (NT(97)));
//G162: boolean_action(94)   => disable_sym(98).
	p(NT(94), (NT(98)));
//G163: enable_sym(97)       => 'e' 'n' 'a' 'b' 'l' 'e'.
	p(NT(97), (T(39)+T(12)+T(34)+T(10)+T(38)+T(39)));
//G164: disable_sym(98)      => 'd' 'i' 's' 'a' 'b' 'l' 'e'.
	p(NT(98), (T(42)+T(36)+T(33)+T(34)+T(10)+T(38)+T(39)));
//G165: __E_autodisambig_sym_37(99) => 'a' 'u' 't' 'o' sep(24).
	p(NT(99), (T(34)+T(43)+T(14)+T(44)+NT(24)));
//G166: __E_autodisambig_sym_37(99) => null.
	p(NT(99), (nul));
//G167: autodisambig_sym(96) => __E_autodisambig_sym_37(99) disambig_sym(100).
	p(NT(96), (NT(99)+NT(100)));
//G168: __E___E_disambig_sym_38_39(102) => 'e'.
	p(NT(102), (T(39)));
//G169: __E___E___E_disambig_sym_38_39_40(103) => 'o' 'n'.
	p(NT(103), (T(44)+T(12)));
//G170: __E___E___E_disambig_sym_38_39_40(103) => 'n' 'g'.
	p(NT(103), (T(12)+T(47)));
//G171: __E___E_disambig_sym_38_39(102) => 'i' __E___E___E_disambig_sym_38_39_40(103).
	p(NT(102), (T(36)+NT(103)));
//G172: __E_disambig_sym_38(101) => 'u' 'a' 't' __E___E_disambig_sym_38_39(102).
	p(NT(101), (T(43)+T(34)+T(14)+NT(102)));
//G173: __E_disambig_sym_38(101) => null.
	p(NT(101), (nul));
//G174: disambig_sym(100)    => 'd' 'i' 's' 'a' 'm' 'b' 'i' 'g' __E_disambig_sym_38(101).
	p(NT(100), (T(42)+T(36)+T(33)+T(34)+T(37)+T(10)+T(36)+T(47)+NT(101)));
//G175: __E_nodisambig_dir_41(105) => _(7) ',' _(7) sym(41).
	p(NT(105), (NT(7)+T(35)+NT(7)+NT(41)));
//G176: __E_nodisambig_dir_42(106) => null.
	p(NT(106), (nul));
//G177: __E_nodisambig_dir_42(106) => __E_nodisambig_dir_41(105) __E_nodisambig_dir_42(106).
	p(NT(106), (NT(105)+NT(106)));
//G178: nodisambig_dir(72)   => nodisambig_sym(104) _(7) sym(41) __E_nodisambig_dir_42(106).
	p(NT(72), (NT(104)+NT(7)+NT(41)+NT(106)));
//G179: __E___E_nodisambig_sym_43_44(108) => 'n'.
	p(NT(108), (T(12)));
//G180: __E___E_nodisambig_sym_43_44(108) => null.
	p(NT(108), (nul));
//G181: __E_nodisambig_sym_43(107) => 'n' 'o' __E___E_nodisambig_sym_43_44(108).
	p(NT(107), (T(12)+T(44)+NT(108)));
//G182: __E___E_nodisambig_sym_43_45(109) => '\''.
	p(NT(109), (T(7)));
//G183: __E___E_nodisambig_sym_43_45(109) => null.
	p(NT(109), (nul));
//G184: __E_nodisambig_sym_43(107) => 'd' 'o' 'n' __E___E_nodisambig_sym_43_45(109) 't'.
	p(NT(107), (T(42)+T(44)+T(12)+NT(109)+T(14)));
//G185: __E_nodisambig_sym_46(110) => sep(24) 'l' 'i' 's' 't'.
	p(NT(110), (NT(24)+T(38)+T(36)+T(33)+T(14)));
//G186: __E_nodisambig_sym_46(110) => null.
	p(NT(110), (nul));
//G187: nodisambig_sym(104)  => __E_nodisambig_sym_43(107) sep(24) disambig_sym(100) __E_nodisambig_sym_46(110).
	p(NT(104), (NT(107)+NT(24)+NT(100)+NT(110)));
	#undef T
	#undef NT
	return loaded = true, p;
}

static inline ::idni::grammar<char_type, terminal_type> grammar(
	nts, productions(), start_symbol, char_classes, grammar_options);

} // namespace tgf_parser_data

struct tgf_parser : public idni::parser<char, char> {
	enum nonterminal {
		nul, eof, alnum, alpha, space, printable, __, _, digits, chars, 
		charvar, start, __E_start_0, statement, __E_start_1, directive, production, start_statement, __E___2, __E____3, 
		comment, __E_comment_4, __E_comment_5, __E_comment_6, sep, __E_sep_7, sep_required, terminal_char, __E_terminal_char_8, unescaped_c, 
		escaped_c, __E_unescaped_c_9, __E_escaped_c_10, terminal_string, terminal_string_char, __E_terminal_string_11, unescaped_s, escaped_s, __E_unescaped_s_12, __E_escaped_s_13, 
		terminal, sym, __E_sym_14, __E_sym_15, __E_sym_16, alternation, conjunction, __E_alternation_17, __E_alternation_18, concatenation, 
		__E_conjunction_19, __E_conjunction_20, factor, __E_concatenation_21, __E_concatenation_22, shorthand_rule, optional, repeat, none_or_repeat, neg, 
		term, group, optional_group, repeat_group, directive_body, start_dir, inline_dir, trim_children_dir, trim_terminals, trim_dir, 
		use_dir, boolean_dir, nodisambig_dir, __E_trim_dir_23, __E_trim_dir_24, trim_terminals_dir, __E_trim_children_dir_25, __E_trim_children_dir_26, inline_arg, __E_inline_dir_27, 
		__E_inline_dir_28, tree_path, char_classes_sym, __E_tree_path_29, __E_tree_path_30, __E_char_classes_sym_31, use_from, use_param, __E_use_dir_32, __E_use_dir_33, 
		__E_use_dir_34, __E_use_dir_35, char_class_name, __E_boolean_dir_36, boolean_action, boolean_dir_name, autodisambig_sym, enable_sym, disable_sym, __E_autodisambig_sym_37, 
		disambig_sym, __E_disambig_sym_38, __E___E_disambig_sym_38_39, __E___E___E_disambig_sym_38_39_40, nodisambig_sym, __E_nodisambig_dir_41, __E_nodisambig_dir_42, __E_nodisambig_sym_43, __E___E_nodisambig_sym_43_44, __E___E_nodisambig_sym_43_45, 
		__E_nodisambig_sym_46, __N_0, __N_1, 
	};
	static tgf_parser& instance() {
		static tgf_parser inst;
		return inst;
	}
	tgf_parser() : idni::parser<char_type, terminal_type>(
		tgf_parser_data::grammar,
		tgf_parser_data::parser_options) {}
};

#endif // __TGF_PARSER_H__
