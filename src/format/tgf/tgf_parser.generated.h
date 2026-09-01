// This file is generated from a file src/format/tgf/tgf.tgf by
//       https://github.com/IDNI/parser/src/tgf
//
#ifndef __TGF_PARSER_H__
#define __TGF_PARSER_H__

#include "parser.h"
#include "recoders.h"

namespace tgf_parser_data {

using char_type     = char;
using terminal_type = char32_t;

inline static constexpr size_t nt_bits = 8;
inline const std::vector<std::string> symbol_names{
	"", "eof", "alnum", "alpha", "space", "printable", "xdigit", "__", "_", "sep", 
	"sym", "cc_name", "escaped_s", "unescaped_s", "escaped_c", "unescaped_c", "terminal_hex", "hex_bytes", "syms", "dynamic_decls", 
	"dynamic_values", "escape_char", "esc_hex", "esc_u4", "esc_U8", "start", "__E_start_0", "statement", "__E_start_1", "directive", 
	"production", "start_statement", "__E_production_2", "production_guard", "alternation", "conjunction", "__E_alternation_3", "__E_alternation_4", "concatenation", "__E_conjunction_5", 
	"__E_conjunction_6", "factor", "__E_concatenation_7", "__E_concatenation_8", "shorthand_rule", "__E_factor_9", "optional", "__E_factor_10", "term", "repeat", 
	"__E_factor_11", "none_or_repeat", "__E_factor_12", "neg", "__E_factor_13", "group", "__E_term_14", "optional_group", "__E_term_15", "repeat_group", 
	"__E_term_16", "terminal", "terminal_char", "terminal_string", "__E_terminal_hex_17", "__E___E_terminal_hex_17_18", "__E___E_terminal_hex_17_19", "__E_sym_20", "__E_sym_21", "__E_sym_22", 
	"__E_terminal_char_23", "__E_unescaped_c_24", "__E_escaped_c_25", "__E_terminal_string_26", "__E_terminal_string_27", "__E_unescaped_s_28", "__E_escaped_s_29", "__E_esc_hex_30", "__E_esc_hex_31", "directive_body", 
	"start_dir", "__E_directive_body_32", "inline_dir", "__E_directive_body_33", "inline_arg", "__E___E_directive_body_33_34", "__E___E_directive_body_33_35", "trim_children_dir", "__E_directive_body_36", "trim_children_terminals_dir", 
	"__E_directive_body_37", "trim_all_terminals_dir", "__E_directive_body_38", "__E___E_directive_body_38_39", "__E___E___E_directive_body_38_39_40", "trim_dir", "__E_directive_body_41", "use_dir", "__E_directive_body_42", "use_from", 
	"cc_sym", "use_param", "__E___E_directive_body_42_43", "__E___E_directive_body_42_44", "disable_ad_dir", "__E_directive_body_45", "__E___E_directive_body_45_46", "__E___E_directive_body_45_47", "enable_prods_dir", "__E_directive_body_48", 
	"ambiguous_dir", "__E_directive_body_49", "dynamic_dir", "__E_directive_body_50", "__E_syms_51", "__E_syms_52", "dynamic_decl", "__E_dynamic_decls_53", "__E_dynamic_decls_54", "dynamic_name", 
	"__E_dynamic_decl_55", "dynamic_value", "__E_dynamic_values_56", "__E_dynamic_values_57", "tree_path", "__E_inline_arg_58", "__E___E_inline_arg_58_59", "__E_tree_path_60", "__E_tree_path_61", "__E_use_param_62", 
	"__E_sep_63", "sep_required", "comment", "__E_comment_64", "__E_comment_65", "__E_comment_66", "__N_0", "__N_1", 
};

inline ::idni::nonterminals<char_type, terminal_type> nts{symbol_names};

inline std::vector<terminal_type> terminals{
	U'\0', U'[', U']', U'=', U'>', U'.', U'|', U'&', U':', 
	U'?', U'+', U'*', U'~', U'(', U')', U'{', U'}', U'0', U'x', 
	U'_', U'\'', U'\\', U'"', U'a', U'b', U'f', U'n', U'r', U't', 
	U'v', U'/', U'X', U'u', U'U', U'@', U's', U',', U'i', U'l', 
	U'e', U'm', U'c', U'h', U'd', U'o', U'p', U'g', U'y', U';', 
	U'k', U'w', U'-', U'\t', U'\r', U'\n', U'#', 
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
			{ 22 },
			{ 23 },
			{ 24 }
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

inline ::idni::prods<char_type, terminal_type> start_symbol{ nts(25) };

#ifdef TAU_PARSER_BUILD_HEADER_ONLY
inline idni::prods<char_type, terminal_type>& productions() {
	static bool loaded = false;
	static idni::prods<char_type, terminal_type>
		p, nul(idni::lit<char_type, terminal_type>{});
	if (loaded) return p;
	#define  T(x) (idni::prods<char_type, terminal_type>{ terminals[x] })
	#define NT(x) (idni::prods<char_type, terminal_type>{ nts(x) })
//G0:   __E_start_0(26)      => _(8) statement(27).
	p(NT(26), (NT(8)+NT(27)));
//G1:   __E_start_1(28)      => null.
	p(NT(28), (nul));
//G2:   __E_start_1(28)      => __E_start_1(28) __E_start_0(26).
	p(NT(28), (NT(28)+NT(26)));
//G3:   start(25)            => __E_start_1(28) _(8).
	p(NT(25), (NT(28)+NT(8)));
//G4:   statement(27)        => directive(29).
	p(NT(27), (NT(29)));
//G5:   statement(27)        => production(30).
	p(NT(27), (NT(30)));
//G6:   start_statement(31)  => _(8) statement(27) _(8).
	p(NT(31), (NT(8)+NT(27)+NT(8)));
//G7:   production_guard(33) => sym(10).
	p(NT(33), (NT(10)));
//G8:   __E_production_2(32) => _(8) '[' _(8) production_guard(33) _(8) ']'.
	p(NT(32), (NT(8)+T(1)+NT(8)+NT(33)+NT(8)+T(2)));
//G9:   __E_production_2(32) => null.
	p(NT(32), (nul));
//G10:  production(30)       => sym(10) __E_production_2(32) _(8) '=' '>' _(8) alternation(34) _(8) '.'.
	p(NT(30), (NT(10)+NT(32)+NT(8)+T(3)+T(4)+NT(8)+NT(34)+NT(8)+T(5)));
//G11:  __E_alternation_3(36) => _(8) '|' _(8) conjunction(35).
	p(NT(36), (NT(8)+T(6)+NT(8)+NT(35)));
//G12:  __E_alternation_4(37) => null.
	p(NT(37), (nul));
//G13:  __E_alternation_4(37) => __E_alternation_4(37) __E_alternation_3(36).
	p(NT(37), (NT(37)+NT(36)));
//G14:  alternation(34)      => conjunction(35) __E_alternation_4(37).
	p(NT(34), (NT(35)+NT(37)));
//G15:  __E_conjunction_5(39) => _(8) '&' _(8) concatenation(38).
	p(NT(39), (NT(8)+T(7)+NT(8)+NT(38)));
//G16:  __E_conjunction_6(40) => null.
	p(NT(40), (nul));
//G17:  __E_conjunction_6(40) => __E_conjunction_6(40) __E_conjunction_5(39).
	p(NT(40), (NT(40)+NT(39)));
//G18:  conjunction(35)      => concatenation(38) __E_conjunction_6(40).
	p(NT(35), (NT(38)+NT(40)));
//G19:  __E_concatenation_7(42) => __(7) factor(41).
	p(NT(42), (NT(7)+NT(41)));
//G20:  __E_concatenation_8(43) => null.
	p(NT(43), (nul));
//G21:  __E_concatenation_8(43) => __E_concatenation_8(43) __E_concatenation_7(42).
	p(NT(43), (NT(43)+NT(42)));
//G22:  concatenation(38)    => factor(41) __E_concatenation_8(43).
	p(NT(38), (NT(41)+NT(43)));
//G23:  __E_factor_9(45)     => factor(41) _(8) ':' sym(10).
	p(NT(45), (NT(41)+NT(8)+T(8)+NT(10)));
//G24:  shorthand_rule(44)   => __E_factor_9(45).
	p(NT(44), (NT(45)));
//G25:  factor(41)           => shorthand_rule(44).
	p(NT(41), (NT(44)));
//G26:  __E_factor_10(47)    => term(48) '?'.
	p(NT(47), (NT(48)+T(9)));
//G27:  optional(46)         => __E_factor_10(47).
	p(NT(46), (NT(47)));
//G28:  factor(41)           => optional(46).
	p(NT(41), (NT(46)));
//G29:  __E_factor_11(50)    => term(48) '+'.
	p(NT(50), (NT(48)+T(10)));
//G30:  repeat(49)           => __E_factor_11(50).
	p(NT(49), (NT(50)));
//G31:  factor(41)           => repeat(49).
	p(NT(41), (NT(49)));
//G32:  __E_factor_12(52)    => term(48) '*'.
	p(NT(52), (NT(48)+T(11)));
//G33:  none_or_repeat(51)   => __E_factor_12(52).
	p(NT(51), (NT(52)));
//G34:  factor(41)           => none_or_repeat(51).
	p(NT(41), (NT(51)));
//G35:  __E_factor_13(54)    => '~' term(48).
	p(NT(54), (T(12)+NT(48)));
//G36:  neg(53)              => __E_factor_13(54).
	p(NT(53), (NT(54)));
//G37:  factor(41)           => neg(53).
	p(NT(41), (NT(53)));
//G38:  factor(41)           => term(48).
	p(NT(41), (NT(48)));
//G39:  __E_term_14(56)      => '(' _(8) alternation(34) _(8) ')'.
	p(NT(56), (T(13)+NT(8)+NT(34)+NT(8)+T(14)));
//G40:  group(55)            => __E_term_14(56).
	p(NT(55), (NT(56)));
//G41:  term(48)             => group(55).
	p(NT(48), (NT(55)));
//G42:  __E_term_15(58)      => '[' _(8) alternation(34) _(8) ']'.
	p(NT(58), (T(1)+NT(8)+NT(34)+NT(8)+T(2)));
//G43:  optional_group(57)   => __E_term_15(58).
	p(NT(57), (NT(58)));
//G44:  term(48)             => optional_group(57).
	p(NT(48), (NT(57)));
//G45:  __E_term_16(60)      => '{' _(8) alternation(34) _(8) '}'.
	p(NT(60), (T(15)+NT(8)+NT(34)+NT(8)+T(16)));
//G46:  repeat_group(59)     => __E_term_16(60).
	p(NT(59), (NT(60)));
//G47:  term(48)             => repeat_group(59).
	p(NT(48), (NT(59)));
//G48:  term(48)             => terminal(61).
	p(NT(48), (NT(61)));
//G49:  term(48)             => sym(10).
	p(NT(48), (NT(10)));
//G50:  terminal(61)         => terminal_char(62).
	p(NT(61), (NT(62)));
//G51:  terminal(61)         => terminal_string(63).
	p(NT(61), (NT(63)));
//G52:  terminal(61)         => terminal_hex(16).
	p(NT(61), (NT(16)));
//G53:  __E_terminal_hex_17(64) => xdigit(6).
	p(NT(64), (NT(6)));
//G54:  __E___E_terminal_hex_17_18(65) => xdigit(6) xdigit(6).
	p(NT(65), (NT(6)+NT(6)));
//G55:  __E___E_terminal_hex_17_19(66) => __E___E_terminal_hex_17_18(65).
	p(NT(66), (NT(65)));
//G56:  __E___E_terminal_hex_17_19(66) => __E___E_terminal_hex_17_19(66) __E___E_terminal_hex_17_18(65).
	p(NT(66), (NT(66)+NT(65)));
//G57:  __E_terminal_hex_17(64) => __E___E_terminal_hex_17_19(66).
	p(NT(64), (NT(66)));
//G58:  hex_bytes(17)        => __E_terminal_hex_17(64).
	p(NT(17), (NT(64)));
//G59:  terminal_hex(16)     => '0' 'x' hex_bytes(17).
	p(NT(16), (T(17)+T(18)+NT(17)));
//G60:  __E_sym_20(67)       => alpha(3).
	p(NT(67), (NT(3)));
//G61:  __E_sym_20(67)       => '_'.
	p(NT(67), (T(19)));
//G62:  __E_sym_21(68)       => alnum(2).
	p(NT(68), (NT(2)));
//G63:  __E_sym_21(68)       => '_'.
	p(NT(68), (T(19)));
//G64:  __E_sym_22(69)       => null.
	p(NT(69), (nul));
//G65:  __E_sym_22(69)       => __E_sym_22(69) __E_sym_21(68).
	p(NT(69), (NT(69)+NT(68)));
//G66:  sym(10)              => __E_sym_20(67) __E_sym_22(69).
	p(NT(10), (NT(67)+NT(69)));
//G67:  __E_terminal_char_23(70) => unescaped_c(15).
	p(NT(70), (NT(15)));
//G68:  __E_terminal_char_23(70) => escaped_c(14).
	p(NT(70), (NT(14)));
//G69:  terminal_char(62)    => '\'' __E_terminal_char_23(70) '\''.
	p(NT(62), (T(20)+NT(70)+T(20)));
//G70:  __E_unescaped_c_24(71) => '\''.
	p(NT(71), (T(20)));
//G71:  __E_unescaped_c_24(71) => '\\'.
	p(NT(71), (T(21)));
//G72:  __N_0(136)           => __E_unescaped_c_24(71).
	p(NT(136), (NT(71)));
//G73:  unescaped_c(15)      => printable(5) & ~( __N_0(136) ).	 # conjunctive
	p(NT(15), (NT(5)) & ~(NT(136)));
//G74:  __E_escaped_c_25(72) => '\''.
	p(NT(72), (T(20)));
//G75:  __E_escaped_c_25(72) => escape_char(21).
	p(NT(72), (NT(21)));
//G76:  escaped_c(14)        => '\\' __E_escaped_c_25(72).
	p(NT(14), (T(21)+NT(72)));
//G77:  __E_terminal_string_26(73) => unescaped_s(13).
	p(NT(73), (NT(13)));
//G78:  __E_terminal_string_26(73) => escaped_s(12).
	p(NT(73), (NT(12)));
//G79:  __E_terminal_string_27(74) => null.
	p(NT(74), (nul));
//G80:  __E_terminal_string_27(74) => __E_terminal_string_27(74) __E_terminal_string_26(73).
	p(NT(74), (NT(74)+NT(73)));
//G81:  terminal_string(63)  => '"' __E_terminal_string_27(74) '"'.
	p(NT(63), (T(22)+NT(74)+T(22)));
//G82:  __E_unescaped_s_28(75) => '"'.
	p(NT(75), (T(22)));
//G83:  __E_unescaped_s_28(75) => '\\'.
	p(NT(75), (T(21)));
//G84:  __N_1(137)           => __E_unescaped_s_28(75).
	p(NT(137), (NT(75)));
//G85:  unescaped_s(13)      => printable(5) & ~( __N_1(137) ).	 # conjunctive
	p(NT(13), (NT(5)) & ~(NT(137)));
//G86:  __E_escaped_s_29(76) => '"'.
	p(NT(76), (T(22)));
//G87:  __E_escaped_s_29(76) => escape_char(21).
	p(NT(76), (NT(21)));
//G88:  escaped_s(12)        => '\\' __E_escaped_s_29(76).
	p(NT(12), (T(21)+NT(76)));
//G89:  escape_char(21)      => 'a'.
	p(NT(21), (T(23)));
//G90:  escape_char(21)      => 'b'.
	p(NT(21), (T(24)));
//G91:  escape_char(21)      => 'f'.
	p(NT(21), (T(25)));
//G92:  escape_char(21)      => 'n'.
	p(NT(21), (T(26)));
//G93:  escape_char(21)      => 'r'.
	p(NT(21), (T(27)));
//G94:  escape_char(21)      => 't'.
	p(NT(21), (T(28)));
//G95:  escape_char(21)      => 'v'.
	p(NT(21), (T(29)));
//G96:  escape_char(21)      => '\\'.
	p(NT(21), (T(21)));
//G97:  escape_char(21)      => '/'.
	p(NT(21), (T(30)));
//G98:  escape_char(21)      => esc_hex(22).
	p(NT(21), (NT(22)));
//G99:  escape_char(21)      => esc_u4(23).
	p(NT(21), (NT(23)));
//G100: escape_char(21)      => esc_U8(24).
	p(NT(21), (NT(24)));
//G101: __E_esc_hex_30(77)   => 'x'.
	p(NT(77), (T(18)));
//G102: __E_esc_hex_30(77)   => 'X'.
	p(NT(77), (T(31)));
//G103: __E_esc_hex_31(78)   => xdigit(6).
	p(NT(78), (NT(6)));
//G104: __E_esc_hex_31(78)   => null.
	p(NT(78), (nul));
//G105: esc_hex(22)          => __E_esc_hex_30(77) xdigit(6) __E_esc_hex_31(78).
	p(NT(22), (NT(77)+NT(6)+NT(78)));
//G106: esc_u4(23)           => 'u' xdigit(6) xdigit(6) xdigit(6) xdigit(6).
	p(NT(23), (T(32)+NT(6)+NT(6)+NT(6)+NT(6)));
//G107: esc_U8(24)           => 'U' xdigit(6) xdigit(6) xdigit(6) xdigit(6) xdigit(6) xdigit(6) xdigit(6) xdigit(6).
	p(NT(24), (T(33)+NT(6)+NT(6)+NT(6)+NT(6)+NT(6)+NT(6)+NT(6)+NT(6)));
//G108: directive(29)        => '@' _(8) directive_body(79) _(8) '.'.
	p(NT(29), (T(34)+NT(8)+NT(79)+NT(8)+T(5)));
//G109: __E_directive_body_32(81) => 's' 't' 'a' 'r' 't' __(7) sym(10).
	p(NT(81), (T(35)+T(28)+T(23)+T(27)+T(28)+NT(7)+NT(10)));
//G110: start_dir(80)        => __E_directive_body_32(81).
	p(NT(80), (NT(81)));
//G111: directive_body(79)   => start_dir(80).
	p(NT(79), (NT(80)));
//G112: __E___E_directive_body_33_34(85) => _(8) ',' _(8) inline_arg(84).
	p(NT(85), (NT(8)+T(36)+NT(8)+NT(84)));
//G113: __E___E_directive_body_33_35(86) => null.
	p(NT(86), (nul));
//G114: __E___E_directive_body_33_35(86) => __E___E_directive_body_33_35(86) __E___E_directive_body_33_34(85).
	p(NT(86), (NT(86)+NT(85)));
//G115: __E_directive_body_33(83) => 'i' 'n' 'l' 'i' 'n' 'e' __(7) inline_arg(84) __E___E_directive_body_33_35(86).
	p(NT(83), (T(37)+T(26)+T(38)+T(37)+T(26)+T(39)+NT(7)+NT(84)+NT(86)));
//G116: inline_dir(82)       => __E_directive_body_33(83).
	p(NT(82), (NT(83)));
//G117: directive_body(79)   => inline_dir(82).
	p(NT(79), (NT(82)));
//G118: __E_directive_body_36(88) => 't' 'r' 'i' 'm' sep(9) 'c' 'h' 'i' 'l' 'd' 'r' 'e' 'n' __(7) syms(18).
	p(NT(88), (T(28)+T(27)+T(37)+T(40)+NT(9)+T(41)+T(42)+T(37)+T(38)+T(43)+T(27)+T(39)+T(26)+NT(7)+NT(18)));
//G119: trim_children_dir(87) => __E_directive_body_36(88).
	p(NT(87), (NT(88)));
//G120: directive_body(79)   => trim_children_dir(87).
	p(NT(79), (NT(87)));
//G121: __E_directive_body_37(90) => 't' 'r' 'i' 'm' sep(9) 'c' 'h' 'i' 'l' 'd' 'r' 'e' 'n' sep(9) 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's' __(7) syms(18).
	p(NT(90), (T(28)+T(27)+T(37)+T(40)+NT(9)+T(41)+T(42)+T(37)+T(38)+T(43)+T(27)+T(39)+T(26)+NT(9)+T(28)+T(39)+T(27)+T(40)+T(37)+T(26)+T(23)+T(38)+T(35)+NT(7)+NT(18)));
//G122: trim_children_terminals_dir(89) => __E_directive_body_37(90).
	p(NT(89), (NT(90)));
//G123: directive_body(79)   => trim_children_terminals_dir(89).
	p(NT(79), (NT(89)));
//G124: __E___E___E_directive_body_38_39_40(94) => sep(9) 'c' 'h' 'i' 'l' 'd' 'r' 'e' 'n' sep(9) 'o' 'f'.
	p(NT(94), (NT(9)+T(41)+T(42)+T(37)+T(38)+T(43)+T(27)+T(39)+T(26)+NT(9)+T(44)+T(25)));
//G125: __E___E___E_directive_body_38_39_40(94) => null.
	p(NT(94), (nul));
//G126: __E___E_directive_body_38_39(93) => sep(9) 'e' 'x' 'c' 'e' 'p' 't' __E___E___E_directive_body_38_39_40(94) __(7) syms(18).
	p(NT(93), (NT(9)+T(39)+T(18)+T(41)+T(39)+T(45)+T(28)+NT(94)+NT(7)+NT(18)));
//G127: __E___E_directive_body_38_39(93) => null.
	p(NT(93), (nul));
//G128: __E_directive_body_38(92) => 't' 'r' 'i' 'm' sep(9) 'a' 'l' 'l' sep(9) 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's' __E___E_directive_body_38_39(93).
	p(NT(92), (T(28)+T(27)+T(37)+T(40)+NT(9)+T(23)+T(38)+T(38)+NT(9)+T(28)+T(39)+T(27)+T(40)+T(37)+T(26)+T(23)+T(38)+T(35)+NT(93)));
//G129: trim_all_terminals_dir(91) => __E_directive_body_38(92).
	p(NT(91), (NT(92)));
//G130: directive_body(79)   => trim_all_terminals_dir(91).
	p(NT(79), (NT(91)));
//G131: __E_directive_body_41(96) => 't' 'r' 'i' 'm' __(7) syms(18).
	p(NT(96), (T(28)+T(27)+T(37)+T(40)+NT(7)+NT(18)));
//G132: trim_dir(95)         => __E_directive_body_41(96).
	p(NT(95), (NT(96)));
//G133: directive_body(79)   => trim_dir(95).
	p(NT(79), (NT(95)));
//G134: use_from(99)         => cc_sym(100).
	p(NT(99), (NT(100)));
//G135: __E___E_directive_body_42_43(102) => _(8) ',' _(8) use_param(101).
	p(NT(102), (NT(8)+T(36)+NT(8)+NT(101)));
//G136: __E___E_directive_body_42_44(103) => null.
	p(NT(103), (nul));
//G137: __E___E_directive_body_42_44(103) => __E___E_directive_body_42_44(103) __E___E_directive_body_42_43(102).
	p(NT(103), (NT(103)+NT(102)));
//G138: __E_directive_body_42(98) => 'u' 's' 'e' __(7) use_from(99) __(7) use_param(101) __E___E_directive_body_42_44(103).
	p(NT(98), (T(32)+T(35)+T(39)+NT(7)+NT(99)+NT(7)+NT(101)+NT(103)));
//G139: use_dir(97)          => __E_directive_body_42(98).
	p(NT(97), (NT(98)));
//G140: directive_body(79)   => use_dir(97).
	p(NT(79), (NT(97)));
//G141: __E___E_directive_body_45_46(106) => 'a' 'u' 't' 'o' sep(9).
	p(NT(106), (T(23)+T(32)+T(28)+T(44)+NT(9)));
//G142: __E___E_directive_body_45_46(106) => null.
	p(NT(106), (nul));
//G143: __E___E_directive_body_45_47(107) => 'u' 'a' 't' 'i' 'o' 'n'.
	p(NT(107), (T(32)+T(23)+T(28)+T(37)+T(44)+T(26)));
//G144: __E___E_directive_body_45_47(107) => null.
	p(NT(107), (nul));
//G145: __E_directive_body_45(105) => 'd' 'i' 's' 'a' 'b' 'l' 'e' __(7) __E___E_directive_body_45_46(106) 'd' 'i' 's' 'a' 'm' 'b' 'i' 'g' __E___E_directive_body_45_47(107).
	p(NT(105), (T(43)+T(37)+T(35)+T(23)+T(24)+T(38)+T(39)+NT(7)+NT(106)+T(43)+T(37)+T(35)+T(23)+T(40)+T(24)+T(37)+T(46)+NT(107)));
//G146: disable_ad_dir(104)  => __E_directive_body_45(105).
	p(NT(104), (NT(105)));
//G147: directive_body(79)   => disable_ad_dir(104).
	p(NT(79), (NT(104)));
//G148: __E_directive_body_48(109) => 'e' 'n' 'a' 'b' 'l' 'e' __(7) 'p' 'r' 'o' 'd' 'u' 'c' 't' 'i' 'o' 'n' 's' __(7) syms(18).
	p(NT(109), (T(39)+T(26)+T(23)+T(24)+T(38)+T(39)+NT(7)+T(45)+T(27)+T(44)+T(43)+T(32)+T(41)+T(28)+T(37)+T(44)+T(26)+T(35)+NT(7)+NT(18)));
//G149: enable_prods_dir(108) => __E_directive_body_48(109).
	p(NT(108), (NT(109)));
//G150: directive_body(79)   => enable_prods_dir(108).
	p(NT(79), (NT(108)));
//G151: __E_directive_body_49(111) => 'a' 'm' 'b' 'i' 'g' 'u' 'o' 'u' 's' __(7) syms(18).
	p(NT(111), (T(23)+T(40)+T(24)+T(37)+T(46)+T(32)+T(44)+T(32)+T(35)+NT(7)+NT(18)));
//G152: ambiguous_dir(110)   => __E_directive_body_49(111).
	p(NT(110), (NT(111)));
//G153: directive_body(79)   => ambiguous_dir(110).
	p(NT(79), (NT(110)));
//G154: __E_directive_body_50(113) => 'd' 'y' 'n' 'a' 'm' 'i' 'c' __(7) dynamic_decls(19).
	p(NT(113), (T(43)+T(47)+T(26)+T(23)+T(40)+T(37)+T(41)+NT(7)+NT(19)));
//G155: dynamic_dir(112)     => __E_directive_body_50(113).
	p(NT(112), (NT(113)));
//G156: directive_body(79)   => dynamic_dir(112).
	p(NT(79), (NT(112)));
//G157: __E_syms_51(114)     => _(8) ',' _(8) sym(10).
	p(NT(114), (NT(8)+T(36)+NT(8)+NT(10)));
//G158: __E_syms_52(115)     => null.
	p(NT(115), (nul));
//G159: __E_syms_52(115)     => __E_syms_52(115) __E_syms_51(114).
	p(NT(115), (NT(115)+NT(114)));
//G160: syms(18)             => sym(10) __E_syms_52(115).
	p(NT(18), (NT(10)+NT(115)));
//G161: __E_dynamic_decls_53(117) => _(8) ';' _(8) dynamic_decl(116).
	p(NT(117), (NT(8)+T(48)+NT(8)+NT(116)));
//G162: __E_dynamic_decls_54(118) => null.
	p(NT(118), (nul));
//G163: __E_dynamic_decls_54(118) => __E_dynamic_decls_54(118) __E_dynamic_decls_53(117).
	p(NT(118), (NT(118)+NT(117)));
//G164: dynamic_decls(19)    => dynamic_decl(116) __E_dynamic_decls_54(118).
	p(NT(19), (NT(116)+NT(118)));
//G165: dynamic_name(119)    => sym(10).
	p(NT(119), (NT(10)));
//G166: __E_dynamic_decl_55(120) => __(7) 'd' 'e' 'f' 'a' 'u' 'l' 't' 's' __(7) dynamic_values(20).
	p(NT(120), (NT(7)+T(43)+T(39)+T(25)+T(23)+T(32)+T(38)+T(28)+T(35)+NT(7)+NT(20)));
//G167: __E_dynamic_decl_55(120) => null.
	p(NT(120), (nul));
//G168: dynamic_decl(116)    => dynamic_name(119) __E_dynamic_decl_55(120).
	p(NT(116), (NT(119)+NT(120)));
//G169: __E_dynamic_values_56(122) => _(8) ',' _(8) dynamic_value(121).
	p(NT(122), (NT(8)+T(36)+NT(8)+NT(121)));
//G170: __E_dynamic_values_57(123) => null.
	p(NT(123), (nul));
//G171: __E_dynamic_values_57(123) => __E_dynamic_values_57(123) __E_dynamic_values_56(122).
	p(NT(123), (NT(123)+NT(122)));
//G172: dynamic_values(20)   => dynamic_value(121) __E_dynamic_values_57(123).
	p(NT(20), (NT(121)+NT(123)));
//G173: dynamic_value(121)   => sym(10).
	p(NT(121), (NT(10)));
//G174: dynamic_value(121)   => terminal_string(63).
	p(NT(121), (NT(63)));
//G175: inline_arg(84)       => tree_path(124).
	p(NT(84), (NT(124)));
//G176: __E___E_inline_arg_58_59(126) => 'e' 's'.
	p(NT(126), (T(39)+T(35)));
//G177: __E___E_inline_arg_58_59(126) => null.
	p(NT(126), (nul));
//G178: __E_inline_arg_58(125) => 'c' 'h' 'a' 'r' sep(9) 'c' 'l' 'a' 's' 's' __E___E_inline_arg_58_59(126).
	p(NT(125), (T(41)+T(42)+T(23)+T(27)+NT(9)+T(41)+T(38)+T(23)+T(35)+T(35)+NT(126)));
//G179: cc_sym(100)          => __E_inline_arg_58(125).
	p(NT(100), (NT(125)));
//G180: inline_arg(84)       => cc_sym(100).
	p(NT(84), (NT(100)));
//G181: __E_tree_path_60(127) => _(8) '>' _(8) sym(10).
	p(NT(127), (NT(8)+T(4)+NT(8)+NT(10)));
//G182: __E_tree_path_61(128) => null.
	p(NT(128), (nul));
//G183: __E_tree_path_61(128) => __E_tree_path_61(128) __E_tree_path_60(127).
	p(NT(128), (NT(128)+NT(127)));
//G184: tree_path(124)       => sym(10) __E_tree_path_61(128).
	p(NT(124), (NT(10)+NT(128)));
//G185: __E_use_param_62(129) => 'e' 'o' 'f'.
	p(NT(129), (T(39)+T(44)+T(25)));
//G186: __E_use_param_62(129) => 'a' 'n' 'y'.
	p(NT(129), (T(23)+T(26)+T(47)));
//G187: __E_use_param_62(129) => 'a' 's' 'c' 'i' 'i'.
	p(NT(129), (T(23)+T(35)+T(41)+T(37)+T(37)));
//G188: __E_use_param_62(129) => 'a' 'l' 'n' 'u' 'm'.
	p(NT(129), (T(23)+T(38)+T(26)+T(32)+T(40)));
//G189: __E_use_param_62(129) => 'a' 'l' 'p' 'h' 'a'.
	p(NT(129), (T(23)+T(38)+T(45)+T(42)+T(23)));
//G190: __E_use_param_62(129) => 'b' 'l' 'a' 'n' 'k'.
	p(NT(129), (T(24)+T(38)+T(23)+T(26)+T(49)));
//G191: __E_use_param_62(129) => 'c' 'n' 't' 'r' 'l'.
	p(NT(129), (T(41)+T(26)+T(28)+T(27)+T(38)));
//G192: __E_use_param_62(129) => 'd' 'i' 'g' 'i' 't'.
	p(NT(129), (T(43)+T(37)+T(46)+T(37)+T(28)));
//G193: __E_use_param_62(129) => 'g' 'r' 'a' 'p' 'h'.
	p(NT(129), (T(46)+T(27)+T(23)+T(45)+T(42)));
//G194: __E_use_param_62(129) => 'l' 'o' 'w' 'e' 'r'.
	p(NT(129), (T(38)+T(44)+T(50)+T(39)+T(27)));
//G195: __E_use_param_62(129) => 'p' 'r' 'i' 'n' 't' 'a' 'b' 'l' 'e'.
	p(NT(129), (T(45)+T(27)+T(37)+T(26)+T(28)+T(23)+T(24)+T(38)+T(39)));
//G196: __E_use_param_62(129) => 'p' 'u' 'n' 'c' 't'.
	p(NT(129), (T(45)+T(32)+T(26)+T(41)+T(28)));
//G197: __E_use_param_62(129) => 's' 'p' 'a' 'c' 'e'.
	p(NT(129), (T(35)+T(45)+T(23)+T(41)+T(39)));
//G198: __E_use_param_62(129) => 'u' 'p' 'p' 'e' 'r'.
	p(NT(129), (T(32)+T(45)+T(45)+T(39)+T(27)));
//G199: __E_use_param_62(129) => 'x' 'd' 'i' 'g' 'i' 't'.
	p(NT(129), (T(18)+T(43)+T(37)+T(46)+T(37)+T(28)));
//G200: cc_name(11)          => __E_use_param_62(129).
	p(NT(11), (NT(129)));
//G201: use_param(101)       => cc_name(11).
	p(NT(101), (NT(11)));
//G202: __E_sep_63(130)      => sep_required(131).
	p(NT(130), (NT(131)));
//G203: __E_sep_63(130)      => null.
	p(NT(130), (nul));
//G204: sep(9)               => __E_sep_63(130).
	p(NT(9), (NT(130)));
//G205: sep_required(131)    => '-'.
	p(NT(131), (T(51)));
//G206: sep_required(131)    => '_'.
	p(NT(131), (T(19)));
//G207: sep_required(131)    => __(7).
	p(NT(131), (NT(7)));
//G208: _(8)                 => __(7).
	p(NT(8), (NT(7)));
//G209: _(8)                 => null.
	p(NT(8), (nul));
//G210: __(7)                => space(4).
	p(NT(7), (NT(4)));
//G211: __(7)                => comment(132).
	p(NT(7), (NT(132)));
//G212: __(7)                => __(7) space(4).
	p(NT(7), (NT(7)+NT(4)));
//G213: __(7)                => __(7) comment(132).
	p(NT(7), (NT(7)+NT(132)));
//G214: __E_comment_64(133)  => printable(5).
	p(NT(133), (NT(5)));
//G215: __E_comment_64(133)  => '\t'.
	p(NT(133), (T(52)));
//G216: __E_comment_65(134)  => null.
	p(NT(134), (nul));
//G217: __E_comment_65(134)  => __E_comment_65(134) __E_comment_64(133).
	p(NT(134), (NT(134)+NT(133)));
//G218: __E_comment_66(135)  => '\r'.
	p(NT(135), (T(53)));
//G219: __E_comment_66(135)  => '\n'.
	p(NT(135), (T(54)));
//G220: __E_comment_66(135)  => eof(1).
	p(NT(135), (NT(1)));
//G221: comment(132)         => '#' __E_comment_65(134) __E_comment_66(135).
	p(NT(132), (T(55)+NT(134)+NT(135)));
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
		sym, cc_name, escaped_s, unescaped_s, escaped_c, unescaped_c, terminal_hex, hex_bytes, syms, dynamic_decls, 
		dynamic_values, escape_char, esc_hex, esc_u4, esc_U8, start, __E_start_0, statement, __E_start_1, directive, 
		production, start_statement, __E_production_2, production_guard, alternation, conjunction, __E_alternation_3, __E_alternation_4, concatenation, __E_conjunction_5, 
		__E_conjunction_6, factor, __E_concatenation_7, __E_concatenation_8, shorthand_rule, __E_factor_9, optional, __E_factor_10, term, repeat, 
		__E_factor_11, none_or_repeat, __E_factor_12, neg, __E_factor_13, group, __E_term_14, optional_group, __E_term_15, repeat_group, 
		__E_term_16, terminal, terminal_char, terminal_string, __E_terminal_hex_17, __E___E_terminal_hex_17_18, __E___E_terminal_hex_17_19, __E_sym_20, __E_sym_21, __E_sym_22, 
		__E_terminal_char_23, __E_unescaped_c_24, __E_escaped_c_25, __E_terminal_string_26, __E_terminal_string_27, __E_unescaped_s_28, __E_escaped_s_29, __E_esc_hex_30, __E_esc_hex_31, directive_body, 
		start_dir, __E_directive_body_32, inline_dir, __E_directive_body_33, inline_arg, __E___E_directive_body_33_34, __E___E_directive_body_33_35, trim_children_dir, __E_directive_body_36, trim_children_terminals_dir, 
		__E_directive_body_37, trim_all_terminals_dir, __E_directive_body_38, __E___E_directive_body_38_39, __E___E___E_directive_body_38_39_40, trim_dir, __E_directive_body_41, use_dir, __E_directive_body_42, use_from, 
		cc_sym, use_param, __E___E_directive_body_42_43, __E___E_directive_body_42_44, disable_ad_dir, __E_directive_body_45, __E___E_directive_body_45_46, __E___E_directive_body_45_47, enable_prods_dir, __E_directive_body_48, 
		ambiguous_dir, __E_directive_body_49, dynamic_dir, __E_directive_body_50, __E_syms_51, __E_syms_52, dynamic_decl, __E_dynamic_decls_53, __E_dynamic_decls_54, dynamic_name, 
		__E_dynamic_decl_55, dynamic_value, __E_dynamic_values_56, __E_dynamic_values_57, tree_path, __E_inline_arg_58, __E___E_inline_arg_58_59, __E_tree_path_60, __E_tree_path_61, __E_use_param_62, 
		__E_sep_63, sep_required, comment, __E_comment_64, __E_comment_65, __E_comment_66, __N_0, __N_1, 
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
