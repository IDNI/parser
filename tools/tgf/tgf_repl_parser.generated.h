// This file is generated from a file tools/tgf/tgf_repl.tgf by
//       https://github.com/IDNI/parser/tools/tgf
//
#ifndef __TGF_REPL_PARSER_H__
#define __TGF_REPL_PARSER_H__

#include "parser.h"

namespace tgf_repl_parser_data {

using char_type     = char;
using terminal_type = char;

static inline std::vector<std::string> symbol_names{
	"", "eof", "alnum", "alpha", "space", "printable", "parse_input_char", "_", "__", "grammar_sym", 
	"internal_grammar_sym", "unreachable_sym", "start_sym", "parse_sym", "parse_file_sym", "load_sym", "reload_sym", "help_sym", "quit_sym", "version_sym", 
	"clear_sym", "get_sym", "set_sym", "add_sym", "del_sym", "toggle_sym", "enable_sym", "disable_sym", "true_value", "false_value", 
	"basic_sym", "detailed_sym", "root_cause_sym", "error_verbosity_opt", "status_opt", "colors_opt", "print_ambiguity_opt", "print_graphs_opt", "print_rules_opt", "print_facts_opt", 
	"print_terminals_opt", "measure_parsing_opt", "measure_each_pos_opt", "measure_forest_opt", "measure_preprocess_opt", "debug_opt", "auto_disambiguate_opt", "trim_terminals_opt", "inline_cc_opt", "nodisambig_list_opt", 
	"trim_opt", "trim_children_opt", "inline_opt", "start", "__E_start_0", "statement", "__E___E_start_0_1", "__E___E_start_0_2", "__E___3", "__E____4", 
	"comment", "__E_comment_5", "__E_comment_6", "__E_comment_7", "quoted_string", "quoted_string_char", "__E_quoted_string_8", "unescaped_s", "escaped_s", "__E_unescaped_s_9", 
	"__E_unescaped_s_10", "__E_escaped_s_11", "filename", "symbol", "__E_symbol_12", "__E_symbol_13", "__E_symbol_14", "parse_input", "parse_input_char_seq", "__E_parse_input_char_seq_15", 
	"grammar_cmd", "internal_grammar_cmd", "unreachable_cmd", "reload_cmd", "load_cmd", "start_cmd", "help", "version", "quit", "clear", 
	"get", "set", "toggle", "enable", "disable", "add", "del", "parse_file_cmd", "parse_cmd", "__E_parse_cmd_16", 
	"__E_parse_file_cmd_17", "__E_grammar_cmd_18", "__E_internal_grammar_cmd_19", "__E_internal_grammar_cmd_20", "__E_start_cmd_21", "__E_start_cmd_22", "__E_unreachable_cmd_23", "__E_unreachable_cmd_24", "__E_reload_cmd_25", "__E_load_cmd_26", 
	"__E_help_27", "__E_help_28", "cmd_symbol", "__E_version_29", "__E_quit_30", "__E_clear_31", "__E_get_32", "option", "__E_add_33", "list_option", 
	"symbol_list", "treepaths_option", "treepath_list", "__E_del_34", "__E_del_35", "__E_set_36", "bool_option", "__E___E_set_36_37", "bool_value", "__E___E_set_36_38", 
	"__E___E_set_36_39", "enum_ev_option", "__E___E_set_36_40", "error_verbosity", "__E_toggle_41", "__E_enable_42", "__E_disable_43", "__E_enum_ev_option_44", "__E_bool_option_45", "__E_bool_option_46", 
	"__E_bool_option_47", "__E_bool_option_48", "__E_bool_option_49", "__E_bool_option_50", "__E_bool_option_51", "__E_bool_option_52", "__E_bool_option_53", "__E_bool_option_54", "__E_bool_option_55", "__E_bool_option_56", 
	"__E_bool_option_57", "__E_bool_option_58", "__E_bool_option_59", "__E_list_option_60", "__E_list_option_61", "__E_treepaths_option_62", "__E_bool_value_63", "__E_bool_value_64", "__E_symbol_list_65", "__E_symbol_list_66", 
	"treepath", "__E_treepath_list_67", "__E_treepath_list_68", "__E_treepath_69", "__E_treepath_70", "__E_error_verbosity_71", "__E_error_verbosity_72", "__E_error_verbosity_73", "__N_0", 
};

static inline ::idni::nonterminals<char_type, terminal_type> nts{symbol_names};

static inline std::vector<terminal_type> terminals{
	'\0', '.', '\t', '\r', '\n', '#', '"', '\\', '/', 
	'b', 'f', 'n', 'r', 't', '_', 'p', 'a', 's', 'e', 
	' ', 'i', 'l', 'g', 'm', '-', 'u', 'c', 'h', 'o', 
	'd', 'v', 'q', 'x', '=', 'y', '1', '0', ',', '>', 
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
			7, 8
		},
		.to_trim_children = {
			9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
			19, 20, 21, 22, 23, 24, 25, 26, 27, 28,
			29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
			39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
			49, 50, 51, 52
		},
		.trim_terminals = false,
		.to_inline = {
			{ 6 }
		},
		.inline_char_classes = true
	}
};
static inline ::idni::parser<char_type, terminal_type>::options parser_options{
};

static inline ::idni::prods<char_type, terminal_type> start_symbol{ nts(53) };

static inline idni::prods<char_type, terminal_type>& productions() {
	static bool loaded = false;
	static idni::prods<char_type, terminal_type>
		p, nul(idni::lit<char_type, terminal_type>{});
	if (loaded) return p;
	#define  T(x) (idni::prods<char_type, terminal_type>{ terminals[x] })
	#define NT(x) (idni::prods<char_type, terminal_type>{ nts(x) })
//G0:   __E___E_start_0_1(56) => _(7) '.' _(7) statement(55).
	p(NT(56), (NT(7)+T(1)+NT(7)+NT(55)));
//G1:   __E___E_start_0_2(57) => null.
	p(NT(57), (nul));
//G2:   __E___E_start_0_2(57) => __E___E_start_0_1(56) __E___E_start_0_2(57).
	p(NT(57), (NT(56)+NT(57)));
//G3:   __E_start_0(54)      => statement(55) __E___E_start_0_2(57) _(7).
	p(NT(54), (NT(55)+NT(57)+NT(7)));
//G4:   __E_start_0(54)      => null.
	p(NT(54), (nul));
//G5:   start(53)            => _(7) __E_start_0(54).
	p(NT(53), (NT(7)+NT(54)));
//G6:   __E___3(58)          => __(8).
	p(NT(58), (NT(8)));
//G7:   __E___3(58)          => null.
	p(NT(58), (nul));
//G8:   _(7)                 => __E___3(58).
	p(NT(7), (NT(58)));
//G9:   __E____4(59)         => space(4).
	p(NT(59), (NT(4)));
//G10:  __E____4(59)         => comment(60).
	p(NT(59), (NT(60)));
//G11:  __(8)                => __E____4(59) _(7).
	p(NT(8), (NT(59)+NT(7)));
//G12:  __E_comment_5(61)    => printable(5).
	p(NT(61), (NT(5)));
//G13:  __E_comment_5(61)    => '\t'.
	p(NT(61), (T(2)));
//G14:  __E_comment_6(62)    => null.
	p(NT(62), (nul));
//G15:  __E_comment_6(62)    => __E_comment_5(61) __E_comment_6(62).
	p(NT(62), (NT(61)+NT(62)));
//G16:  __E_comment_7(63)    => '\r'.
	p(NT(63), (T(3)));
//G17:  __E_comment_7(63)    => '\n'.
	p(NT(63), (T(4)));
//G18:  __E_comment_7(63)    => eof(1).
	p(NT(63), (NT(1)));
//G19:  comment(60)          => '#' __E_comment_6(62) __E_comment_7(63).
	p(NT(60), (T(5)+NT(62)+NT(63)));
//G20:  __E_quoted_string_8(66) => null.
	p(NT(66), (nul));
//G21:  __E_quoted_string_8(66) => quoted_string_char(65) __E_quoted_string_8(66).
	p(NT(66), (NT(65)+NT(66)));
//G22:  quoted_string(64)    => '"' __E_quoted_string_8(66) '"'.
	p(NT(64), (T(6)+NT(66)+T(6)));
//G23:  quoted_string_char(65) => unescaped_s(67).
	p(NT(65), (NT(67)));
//G24:  quoted_string_char(65) => escaped_s(68).
	p(NT(65), (NT(68)));
//G25:  __E_unescaped_s_9(69) => space(4).
	p(NT(69), (NT(4)));
//G26:  __E_unescaped_s_9(69) => printable(5).
	p(NT(69), (NT(5)));
//G27:  __E_unescaped_s_10(70) => '"'.
	p(NT(70), (T(6)));
//G28:  __E_unescaped_s_10(70) => '\\'.
	p(NT(70), (T(7)));
//G29:  __N_0(168)           => __E_unescaped_s_10(70).
	p(NT(168), (NT(70)));
//G30:  unescaped_s(67)      => __E_unescaped_s_9(69) & ~( __N_0(168) ).	 # conjunctive
	p(NT(67), (NT(69)) & ~(NT(168)));
//G31:  __E_escaped_s_11(71) => '"'.
	p(NT(71), (T(6)));
//G32:  __E_escaped_s_11(71) => '\\'.
	p(NT(71), (T(7)));
//G33:  __E_escaped_s_11(71) => '/'.
	p(NT(71), (T(8)));
//G34:  __E_escaped_s_11(71) => 'b'.
	p(NT(71), (T(9)));
//G35:  __E_escaped_s_11(71) => 'f'.
	p(NT(71), (T(10)));
//G36:  __E_escaped_s_11(71) => 'n'.
	p(NT(71), (T(11)));
//G37:  __E_escaped_s_11(71) => 'r'.
	p(NT(71), (T(12)));
//G38:  __E_escaped_s_11(71) => 't'.
	p(NT(71), (T(13)));
//G39:  escaped_s(68)        => '\\' __E_escaped_s_11(71).
	p(NT(68), (T(7)+NT(71)));
//G40:  filename(72)         => quoted_string(64).
	p(NT(72), (NT(64)));
//G41:  __E_symbol_12(74)    => alpha(3).
	p(NT(74), (NT(3)));
//G42:  __E_symbol_12(74)    => '_'.
	p(NT(74), (T(14)));
//G43:  __E_symbol_13(75)    => alnum(2).
	p(NT(75), (NT(2)));
//G44:  __E_symbol_13(75)    => '_'.
	p(NT(75), (T(14)));
//G45:  __E_symbol_14(76)    => null.
	p(NT(76), (nul));
//G46:  __E_symbol_14(76)    => __E_symbol_13(75) __E_symbol_14(76).
	p(NT(76), (NT(75)+NT(76)));
//G47:  symbol(73)           => __E_symbol_12(74) __E_symbol_14(76).
	p(NT(73), (NT(74)+NT(76)));
//G48:  parse_input(77)      => quoted_string(64).
	p(NT(77), (NT(64)));
//G49:  parse_input(77)      => parse_input_char_seq(78).
	p(NT(77), (NT(78)));
//G50:  __E_parse_input_char_seq_15(79) => parse_input_char(6).
	p(NT(79), (NT(6)));
//G51:  __E_parse_input_char_seq_15(79) => parse_input_char(6) __E_parse_input_char_seq_15(79).
	p(NT(79), (NT(6)+NT(79)));
//G52:  parse_input_char_seq(78) => __E_parse_input_char_seq_15(79).
	p(NT(78), (NT(79)));
//G53:  parse_input_char(6)  => printable(5).
	p(NT(6), (NT(5)));
//G54:  parse_input_char(6)  => '\t'.
	p(NT(6), (T(2)));
//G55:  statement(55)        => grammar_cmd(80).
	p(NT(55), (NT(80)));
//G56:  statement(55)        => internal_grammar_cmd(81).
	p(NT(55), (NT(81)));
//G57:  statement(55)        => unreachable_cmd(82).
	p(NT(55), (NT(82)));
//G58:  statement(55)        => reload_cmd(83).
	p(NT(55), (NT(83)));
//G59:  statement(55)        => load_cmd(84).
	p(NT(55), (NT(84)));
//G60:  statement(55)        => start_cmd(85).
	p(NT(55), (NT(85)));
//G61:  statement(55)        => help(86).
	p(NT(55), (NT(86)));
//G62:  statement(55)        => version(87).
	p(NT(55), (NT(87)));
//G63:  statement(55)        => quit(88).
	p(NT(55), (NT(88)));
//G64:  statement(55)        => clear(89).
	p(NT(55), (NT(89)));
//G65:  statement(55)        => get(90).
	p(NT(55), (NT(90)));
//G66:  statement(55)        => set(91).
	p(NT(55), (NT(91)));
//G67:  statement(55)        => toggle(92).
	p(NT(55), (NT(92)));
//G68:  statement(55)        => enable(93).
	p(NT(55), (NT(93)));
//G69:  statement(55)        => disable(94).
	p(NT(55), (NT(94)));
//G70:  statement(55)        => add(95).
	p(NT(55), (NT(95)));
//G71:  statement(55)        => del(96).
	p(NT(55), (NT(96)));
//G72:  statement(55)        => parse_file_cmd(97).
	p(NT(55), (NT(97)));
//G73:  statement(55)        => parse_cmd(98).
	p(NT(55), (NT(98)));
//G74:  __E_parse_cmd_16(99) => 'p'.
	p(NT(99), (T(15)));
//G75:  __E_parse_cmd_16(99) => 'p' 'a' 'r' 's' 'e'.
	p(NT(99), (T(15)+T(16)+T(12)+T(17)+T(18)));
//G76:  parse_sym(13)        => __E_parse_cmd_16(99).
	p(NT(13), (NT(99)));
//G77:  parse_cmd(98)        => parse_sym(13) __(8) parse_input(77).
	p(NT(98), (NT(13)+NT(8)+NT(77)));
//G78:  __E_parse_file_cmd_17(100) => 'f'.
	p(NT(100), (T(10)));
//G79:  __E_parse_file_cmd_17(100) => 'p' 'f'.
	p(NT(100), (T(15)+T(10)));
//G80:  __E_parse_file_cmd_17(100) => 'p' 'a' 'r' 's' 'e' ' ' 'f' 'i' 'l' 'e'.
	p(NT(100), (T(15)+T(16)+T(12)+T(17)+T(18)+T(19)+T(10)+T(20)+T(21)+T(18)));
//G81:  parse_file_sym(14)   => __E_parse_file_cmd_17(100).
	p(NT(14), (NT(100)));
//G82:  parse_file_cmd(97)   => parse_file_sym(14) __(8) filename(72).
	p(NT(97), (NT(14)+NT(8)+NT(72)));
//G83:  __E_grammar_cmd_18(101) => 'g'.
	p(NT(101), (T(22)));
//G84:  __E_grammar_cmd_18(101) => 'g' 'r' 'a' 'm' 'm' 'a' 'r'.
	p(NT(101), (T(22)+T(12)+T(16)+T(23)+T(23)+T(16)+T(12)));
//G85:  grammar_sym(9)       => __E_grammar_cmd_18(101).
	p(NT(9), (NT(101)));
//G86:  grammar_cmd(80)      => grammar_sym(9).
	p(NT(80), (NT(9)));
//G87:  __E_internal_grammar_cmd_19(102) => 'i'.
	p(NT(102), (T(20)));
//G88:  __E_internal_grammar_cmd_19(102) => 'i' 'g'.
	p(NT(102), (T(20)+T(22)));
//G89:  __E_internal_grammar_cmd_19(102) => 'i' 'n' 't' 'e' 'r' 'n' 'a' 'l' '-' 'g' 'r' 'a' 'm' 'm' 'a' 'r'.
	p(NT(102), (T(20)+T(11)+T(13)+T(18)+T(12)+T(11)+T(16)+T(21)+T(24)+T(22)+T(12)+T(16)+T(23)+T(23)+T(16)+T(12)));
//G90:  internal_grammar_sym(10) => __E_internal_grammar_cmd_19(102).
	p(NT(10), (NT(102)));
//G91:  __E_internal_grammar_cmd_20(103) => __(8) symbol(73).
	p(NT(103), (NT(8)+NT(73)));
//G92:  __E_internal_grammar_cmd_20(103) => null.
	p(NT(103), (nul));
//G93:  internal_grammar_cmd(81) => internal_grammar_sym(10) __E_internal_grammar_cmd_20(103).
	p(NT(81), (NT(10)+NT(103)));
//G94:  __E_start_cmd_21(104) => 's'.
	p(NT(104), (T(17)));
//G95:  __E_start_cmd_21(104) => 's' 't' 'a' 'r' 't'.
	p(NT(104), (T(17)+T(13)+T(16)+T(12)+T(13)));
//G96:  start_sym(12)        => __E_start_cmd_21(104).
	p(NT(12), (NT(104)));
//G97:  __E_start_cmd_22(105) => __(8) symbol(73).
	p(NT(105), (NT(8)+NT(73)));
//G98:  __E_start_cmd_22(105) => null.
	p(NT(105), (nul));
//G99:  start_cmd(85)        => start_sym(12) __E_start_cmd_22(105).
	p(NT(85), (NT(12)+NT(105)));
//G100: __E_unreachable_cmd_23(106) => 'u'.
	p(NT(106), (T(25)));
//G101: __E_unreachable_cmd_23(106) => 'u' 'n' 'r' 'e' 'a' 'c' 'h' 'a' 'b' 'l' 'e'.
	p(NT(106), (T(25)+T(11)+T(12)+T(18)+T(16)+T(26)+T(27)+T(16)+T(9)+T(21)+T(18)));
//G102: unreachable_sym(11)  => __E_unreachable_cmd_23(106).
	p(NT(11), (NT(106)));
//G103: __E_unreachable_cmd_24(107) => __(8) symbol(73).
	p(NT(107), (NT(8)+NT(73)));
//G104: __E_unreachable_cmd_24(107) => null.
	p(NT(107), (nul));
//G105: unreachable_cmd(82)  => unreachable_sym(11) __E_unreachable_cmd_24(107).
	p(NT(82), (NT(11)+NT(107)));
//G106: __E_reload_cmd_25(108) => 'r'.
	p(NT(108), (T(12)));
//G107: __E_reload_cmd_25(108) => 'r' 'e' 'l' 'o' 'a' 'd'.
	p(NT(108), (T(12)+T(18)+T(21)+T(28)+T(16)+T(29)));
//G108: reload_sym(16)       => __E_reload_cmd_25(108).
	p(NT(16), (NT(108)));
//G109: reload_cmd(83)       => reload_sym(16).
	p(NT(83), (NT(16)));
//G110: __E_load_cmd_26(109) => 'l'.
	p(NT(109), (T(21)));
//G111: __E_load_cmd_26(109) => 'l' 'o' 'a' 'd'.
	p(NT(109), (T(21)+T(28)+T(16)+T(29)));
//G112: load_sym(15)         => __E_load_cmd_26(109).
	p(NT(15), (NT(109)));
//G113: load_cmd(84)         => load_sym(15) __(8) filename(72).
	p(NT(84), (NT(15)+NT(8)+NT(72)));
//G114: __E_help_27(110)     => 'h'.
	p(NT(110), (T(27)));
//G115: __E_help_27(110)     => 'h' 'e' 'l' 'p'.
	p(NT(110), (T(27)+T(18)+T(21)+T(15)));
//G116: help_sym(17)         => __E_help_27(110).
	p(NT(17), (NT(110)));
//G117: __E_help_28(111)     => __(8) cmd_symbol(112).
	p(NT(111), (NT(8)+NT(112)));
//G118: __E_help_28(111)     => null.
	p(NT(111), (nul));
//G119: help(86)             => help_sym(17) __E_help_28(111).
	p(NT(86), (NT(17)+NT(111)));
//G120: __E_version_29(113)  => 'v'.
	p(NT(113), (T(30)));
//G121: __E_version_29(113)  => 'v' 'e' 'r' 's' 'i' 'o' 'n'.
	p(NT(113), (T(30)+T(18)+T(12)+T(17)+T(20)+T(28)+T(11)));
//G122: version_sym(19)      => __E_version_29(113).
	p(NT(19), (NT(113)));
//G123: version(87)          => version_sym(19).
	p(NT(87), (NT(19)));
//G124: __E_quit_30(114)     => 'q'.
	p(NT(114), (T(31)));
//G125: __E_quit_30(114)     => 'q' 'u' 'i' 't'.
	p(NT(114), (T(31)+T(25)+T(20)+T(13)));
//G126: __E_quit_30(114)     => 'e'.
	p(NT(114), (T(18)));
//G127: __E_quit_30(114)     => 'e' 'x' 'i' 't'.
	p(NT(114), (T(18)+T(32)+T(20)+T(13)));
//G128: quit_sym(18)         => __E_quit_30(114).
	p(NT(18), (NT(114)));
//G129: quit(88)             => quit_sym(18).
	p(NT(88), (NT(18)));
//G130: __E_clear_31(115)    => 'c' 'l' 's'.
	p(NT(115), (T(26)+T(21)+T(17)));
//G131: __E_clear_31(115)    => 'c' 'l' 'e' 'a' 'r'.
	p(NT(115), (T(26)+T(21)+T(18)+T(16)+T(12)));
//G132: clear_sym(20)        => __E_clear_31(115).
	p(NT(20), (NT(115)));
//G133: clear(89)            => clear_sym(20).
	p(NT(89), (NT(20)));
//G134: get_sym(21)          => 'g' 'e' 't'.
	p(NT(21), (T(22)+T(18)+T(13)));
//G135: __E_get_32(116)      => __(8) option(117).
	p(NT(116), (NT(8)+NT(117)));
//G136: __E_get_32(116)      => null.
	p(NT(116), (nul));
//G137: get(90)              => get_sym(21) __E_get_32(116).
	p(NT(90), (NT(21)+NT(116)));
//G138: add_sym(23)          => 'a' 'd' 'd'.
	p(NT(23), (T(16)+T(29)+T(29)));
//G139: __E_add_33(118)      => list_option(119) __(8) symbol_list(120).
	p(NT(118), (NT(119)+NT(8)+NT(120)));
//G140: __E_add_33(118)      => treepaths_option(121) __(8) treepath_list(122).
	p(NT(118), (NT(121)+NT(8)+NT(122)));
//G141: add(95)              => add_sym(23) __(8) __E_add_33(118).
	p(NT(95), (NT(23)+NT(8)+NT(118)));
//G142: __E_del_34(123)      => 'd' 'e' 'l'.
	p(NT(123), (T(29)+T(18)+T(21)));
//G143: __E_del_34(123)      => 'd' 'e' 'l' 'e' 't' 'e'.
	p(NT(123), (T(29)+T(18)+T(21)+T(18)+T(13)+T(18)));
//G144: __E_del_34(123)      => 'r' 'm'.
	p(NT(123), (T(12)+T(23)));
//G145: __E_del_34(123)      => 'r' 'e' 'm'.
	p(NT(123), (T(12)+T(18)+T(23)));
//G146: __E_del_34(123)      => 'r' 'e' 'm' 'o' 'v' 'e'.
	p(NT(123), (T(12)+T(18)+T(23)+T(28)+T(30)+T(18)));
//G147: del_sym(24)          => __E_del_34(123).
	p(NT(24), (NT(123)));
//G148: __E_del_35(124)      => list_option(119) __(8) symbol_list(120).
	p(NT(124), (NT(119)+NT(8)+NT(120)));
//G149: __E_del_35(124)      => treepaths_option(121) __(8) treepath_list(122).
	p(NT(124), (NT(121)+NT(8)+NT(122)));
//G150: del(96)              => del_sym(24) __(8) __E_del_35(124).
	p(NT(96), (NT(24)+NT(8)+NT(124)));
//G151: set_sym(22)          => 's' 'e' 't'.
	p(NT(22), (T(17)+T(18)+T(13)));
//G152: __E___E_set_36_37(127) => __(8).
	p(NT(127), (NT(8)));
//G153: __E___E_set_36_37(127) => _(7) '=' _(7).
	p(NT(127), (NT(7)+T(33)+NT(7)));
//G154: __E_set_36(125)      => bool_option(126) __E___E_set_36_37(127) bool_value(128).
	p(NT(125), (NT(126)+NT(127)+NT(128)));
//G155: __E___E_set_36_38(129) => __(8).
	p(NT(129), (NT(8)));
//G156: __E___E_set_36_38(129) => _(7) '=' _(7).
	p(NT(129), (NT(7)+T(33)+NT(7)));
//G157: __E_set_36(125)      => list_option(119) __E___E_set_36_38(129) symbol_list(120).
	p(NT(125), (NT(119)+NT(129)+NT(120)));
//G158: __E___E_set_36_39(130) => __(8).
	p(NT(130), (NT(8)));
//G159: __E___E_set_36_39(130) => _(7) '=' _(7).
	p(NT(130), (NT(7)+T(33)+NT(7)));
//G160: __E_set_36(125)      => treepaths_option(121) __E___E_set_36_39(130) treepath_list(122).
	p(NT(125), (NT(121)+NT(130)+NT(122)));
//G161: __E___E_set_36_40(132) => __(8).
	p(NT(132), (NT(8)));
//G162: __E___E_set_36_40(132) => _(7) '=' _(7).
	p(NT(132), (NT(7)+T(33)+NT(7)));
//G163: __E_set_36(125)      => enum_ev_option(131) __E___E_set_36_40(132) error_verbosity(133).
	p(NT(125), (NT(131)+NT(132)+NT(133)));
//G164: set(91)              => set_sym(22) __(8) __E_set_36(125).
	p(NT(91), (NT(22)+NT(8)+NT(125)));
//G165: __E_toggle_41(134)   => 't' 'o' 'g'.
	p(NT(134), (T(13)+T(28)+T(22)));
//G166: __E_toggle_41(134)   => 't' 'o' 'g' 'g' 'l' 'e'.
	p(NT(134), (T(13)+T(28)+T(22)+T(22)+T(21)+T(18)));
//G167: toggle_sym(25)       => __E_toggle_41(134).
	p(NT(25), (NT(134)));
//G168: toggle(92)           => toggle_sym(25) __(8) bool_option(126).
	p(NT(92), (NT(25)+NT(8)+NT(126)));
//G169: __E_enable_42(135)   => 'e' 'n' __(8).
	p(NT(135), (T(18)+T(11)+NT(8)));
//G170: __E_enable_42(135)   => 'e' 'n' 'a' 'b' 'l' 'e' __(8).
	p(NT(135), (T(18)+T(11)+T(16)+T(9)+T(21)+T(18)+NT(8)));
//G171: enable_sym(26)       => __E_enable_42(135).
	p(NT(26), (NT(135)));
//G172: enable(93)           => enable_sym(26) bool_option(126).
	p(NT(93), (NT(26)+NT(126)));
//G173: __E_disable_43(136)  => 'd' 'i' 's' __(8).
	p(NT(136), (T(29)+T(20)+T(17)+NT(8)));
//G174: __E_disable_43(136)  => 'd' 'i' 's' 'a' 'b' 'l' 'e' __(8).
	p(NT(136), (T(29)+T(20)+T(17)+T(16)+T(9)+T(21)+T(18)+NT(8)));
//G175: disable_sym(27)      => __E_disable_43(136).
	p(NT(27), (NT(136)));
//G176: disable(94)          => disable_sym(27) bool_option(126).
	p(NT(94), (NT(27)+NT(126)));
//G177: cmd_symbol(112)      => grammar_sym(9).
	p(NT(112), (NT(9)));
//G178: cmd_symbol(112)      => internal_grammar_sym(10).
	p(NT(112), (NT(10)));
//G179: cmd_symbol(112)      => unreachable_sym(11).
	p(NT(112), (NT(11)));
//G180: cmd_symbol(112)      => start_sym(12).
	p(NT(112), (NT(12)));
//G181: cmd_symbol(112)      => parse_sym(13).
	p(NT(112), (NT(13)));
//G182: cmd_symbol(112)      => parse_file_sym(14).
	p(NT(112), (NT(14)));
//G183: cmd_symbol(112)      => load_sym(15).
	p(NT(112), (NT(15)));
//G184: cmd_symbol(112)      => reload_sym(16).
	p(NT(112), (NT(16)));
//G185: cmd_symbol(112)      => help_sym(17).
	p(NT(112), (NT(17)));
//G186: cmd_symbol(112)      => quit_sym(18).
	p(NT(112), (NT(18)));
//G187: cmd_symbol(112)      => version_sym(19).
	p(NT(112), (NT(19)));
//G188: cmd_symbol(112)      => clear_sym(20).
	p(NT(112), (NT(20)));
//G189: cmd_symbol(112)      => get_sym(21).
	p(NT(112), (NT(21)));
//G190: cmd_symbol(112)      => set_sym(22).
	p(NT(112), (NT(22)));
//G191: cmd_symbol(112)      => add_sym(23).
	p(NT(112), (NT(23)));
//G192: cmd_symbol(112)      => del_sym(24).
	p(NT(112), (NT(24)));
//G193: cmd_symbol(112)      => toggle_sym(25).
	p(NT(112), (NT(25)));
//G194: cmd_symbol(112)      => enable_sym(26).
	p(NT(112), (NT(26)));
//G195: cmd_symbol(112)      => disable_sym(27).
	p(NT(112), (NT(27)));
//G196: option(117)          => bool_option(126).
	p(NT(117), (NT(126)));
//G197: option(117)          => enum_ev_option(131).
	p(NT(117), (NT(131)));
//G198: option(117)          => list_option(119).
	p(NT(117), (NT(119)));
//G199: option(117)          => treepaths_option(121).
	p(NT(117), (NT(121)));
//G200: __E_enum_ev_option_44(137) => 'e'.
	p(NT(137), (T(18)));
//G201: __E_enum_ev_option_44(137) => 'e' 'r' 'r' 'o' 'r' '-' 'v' 'e' 'r' 'b' 'o' 's' 'i' 't' 'y'.
	p(NT(137), (T(18)+T(12)+T(12)+T(28)+T(12)+T(24)+T(30)+T(18)+T(12)+T(9)+T(28)+T(17)+T(20)+T(13)+T(34)));
//G202: error_verbosity_opt(33) => __E_enum_ev_option_44(137).
	p(NT(33), (NT(137)));
//G203: enum_ev_option(131)  => error_verbosity_opt(33).
	p(NT(131), (NT(33)));
//G204: __E_bool_option_45(138) => 's'.
	p(NT(138), (T(17)));
//G205: __E_bool_option_45(138) => 's' 't' 'a' 't' 'u' 's'.
	p(NT(138), (T(17)+T(13)+T(16)+T(13)+T(25)+T(17)));
//G206: status_opt(34)       => __E_bool_option_45(138).
	p(NT(34), (NT(138)));
//G207: bool_option(126)     => status_opt(34).
	p(NT(126), (NT(34)));
//G208: __E_bool_option_46(139) => 'c'.
	p(NT(139), (T(26)));
//G209: __E_bool_option_46(139) => 'c' 'o' 'l' 'o' 'r'.
	p(NT(139), (T(26)+T(28)+T(21)+T(28)+T(12)));
//G210: __E_bool_option_46(139) => 'c' 'o' 'l' 'o' 'r' 's'.
	p(NT(139), (T(26)+T(28)+T(21)+T(28)+T(12)+T(17)));
//G211: colors_opt(35)       => __E_bool_option_46(139).
	p(NT(35), (NT(139)));
//G212: bool_option(126)     => colors_opt(35).
	p(NT(126), (NT(35)));
//G213: __E_bool_option_47(140) => 'a'.
	p(NT(140), (T(16)));
//G214: __E_bool_option_47(140) => 'a' 'm' 'b' 'i' 'g' 'u' 'i' 't' 'y'.
	p(NT(140), (T(16)+T(23)+T(9)+T(20)+T(22)+T(25)+T(20)+T(13)+T(34)));
//G215: __E_bool_option_47(140) => 'p' 'r' 'i' 'n' 't' '-' 'a' 'm' 'b' 'i' 'g' 'u' 'i' 't' 'y'.
	p(NT(140), (T(15)+T(12)+T(20)+T(11)+T(13)+T(24)+T(16)+T(23)+T(9)+T(20)+T(22)+T(25)+T(20)+T(13)+T(34)));
//G216: print_ambiguity_opt(36) => __E_bool_option_47(140).
	p(NT(36), (NT(140)));
//G217: bool_option(126)     => print_ambiguity_opt(36).
	p(NT(126), (NT(36)));
//G218: __E_bool_option_48(141) => 'g'.
	p(NT(141), (T(22)));
//G219: __E_bool_option_48(141) => 'g' 'r' 'a' 'p' 'h' 's'.
	p(NT(141), (T(22)+T(12)+T(16)+T(15)+T(27)+T(17)));
//G220: __E_bool_option_48(141) => 'p' 'r' 'i' 'n' 't' '-' 'g' 'r' 'a' 'p' 'h' 's'.
	p(NT(141), (T(15)+T(12)+T(20)+T(11)+T(13)+T(24)+T(22)+T(12)+T(16)+T(15)+T(27)+T(17)));
//G221: print_graphs_opt(37) => __E_bool_option_48(141).
	p(NT(37), (NT(141)));
//G222: bool_option(126)     => print_graphs_opt(37).
	p(NT(126), (NT(37)));
//G223: __E_bool_option_49(142) => 'r'.
	p(NT(142), (T(12)));
//G224: __E_bool_option_49(142) => 'r' 'u' 'l' 'e' 's'.
	p(NT(142), (T(12)+T(25)+T(21)+T(18)+T(17)));
//G225: __E_bool_option_49(142) => 'p' 'r' 'i' 'n' 't' '-' 'r' 'u' 'l' 'e' 's'.
	p(NT(142), (T(15)+T(12)+T(20)+T(11)+T(13)+T(24)+T(12)+T(25)+T(21)+T(18)+T(17)));
//G226: print_rules_opt(38)  => __E_bool_option_49(142).
	p(NT(38), (NT(142)));
//G227: bool_option(126)     => print_rules_opt(38).
	p(NT(126), (NT(38)));
//G228: __E_bool_option_50(143) => 'f'.
	p(NT(143), (T(10)));
//G229: __E_bool_option_50(143) => 'f' 'a' 'c' 't' 's'.
	p(NT(143), (T(10)+T(16)+T(26)+T(13)+T(17)));
//G230: __E_bool_option_50(143) => 'p' 'r' 'i' 'n' 't' '-' 'f' 'a' 'c' 't' 's'.
	p(NT(143), (T(15)+T(12)+T(20)+T(11)+T(13)+T(24)+T(10)+T(16)+T(26)+T(13)+T(17)));
//G231: print_facts_opt(39)  => __E_bool_option_50(143).
	p(NT(39), (NT(143)));
//G232: bool_option(126)     => print_facts_opt(39).
	p(NT(126), (NT(39)));
//G233: __E_bool_option_51(144) => 't'.
	p(NT(144), (T(13)));
//G234: __E_bool_option_51(144) => 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
	p(NT(144), (T(13)+T(18)+T(12)+T(23)+T(20)+T(11)+T(16)+T(21)+T(17)));
//G235: __E_bool_option_51(144) => 'p' 'r' 'i' 'n' 't' '-' 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
	p(NT(144), (T(15)+T(12)+T(20)+T(11)+T(13)+T(24)+T(13)+T(18)+T(12)+T(23)+T(20)+T(11)+T(16)+T(21)+T(17)));
//G236: print_terminals_opt(40) => __E_bool_option_51(144).
	p(NT(40), (NT(144)));
//G237: bool_option(126)     => print_terminals_opt(40).
	p(NT(126), (NT(40)));
//G238: __E_bool_option_52(145) => 'm'.
	p(NT(145), (T(23)));
//G239: __E_bool_option_52(145) => 'm' 'e' 'a' 's' 'u' 'r' 'e'.
	p(NT(145), (T(23)+T(18)+T(16)+T(17)+T(25)+T(12)+T(18)));
//G240: __E_bool_option_52(145) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'p' 'a' 'r' 's' 'i' 'n' 'g'.
	p(NT(145), (T(23)+T(18)+T(16)+T(17)+T(25)+T(12)+T(18)+T(24)+T(15)+T(16)+T(12)+T(17)+T(20)+T(11)+T(22)));
//G241: measure_parsing_opt(41) => __E_bool_option_52(145).
	p(NT(41), (NT(145)));
//G242: bool_option(126)     => measure_parsing_opt(41).
	p(NT(126), (NT(41)));
//G243: __E_bool_option_53(146) => 'm' 'e'.
	p(NT(146), (T(23)+T(18)));
//G244: __E_bool_option_53(146) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'e' 'a' 'c' 'h'.
	p(NT(146), (T(23)+T(18)+T(16)+T(17)+T(25)+T(12)+T(18)+T(24)+T(18)+T(16)+T(26)+T(27)));
//G245: __E_bool_option_53(146) => 'm' 'e' 'p'.
	p(NT(146), (T(23)+T(18)+T(15)));
//G246: __E_bool_option_53(146) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'e' 'a' 'c' 'h' '-' 'p' 'o' 's'.
	p(NT(146), (T(23)+T(18)+T(16)+T(17)+T(25)+T(12)+T(18)+T(24)+T(18)+T(16)+T(26)+T(27)+T(24)+T(15)+T(28)+T(17)));
//G247: measure_each_pos_opt(42) => __E_bool_option_53(146).
	p(NT(42), (NT(146)));
//G248: bool_option(126)     => measure_each_pos_opt(42).
	p(NT(126), (NT(42)));
//G249: __E_bool_option_54(147) => 'm' 'f'.
	p(NT(147), (T(23)+T(10)));
//G250: __E_bool_option_54(147) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'f' 'o' 'r' 'e' 's' 't'.
	p(NT(147), (T(23)+T(18)+T(16)+T(17)+T(25)+T(12)+T(18)+T(24)+T(10)+T(28)+T(12)+T(18)+T(17)+T(13)));
//G251: measure_forest_opt(43) => __E_bool_option_54(147).
	p(NT(43), (NT(147)));
//G252: bool_option(126)     => measure_forest_opt(43).
	p(NT(126), (NT(43)));
//G253: __E_bool_option_55(148) => 'm' 'p'.
	p(NT(148), (T(23)+T(15)));
//G254: __E_bool_option_55(148) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'p' 'r' 'e' 'p' 'r' 'o' 'c' 'e' 's' 's'.
	p(NT(148), (T(23)+T(18)+T(16)+T(17)+T(25)+T(12)+T(18)+T(24)+T(15)+T(12)+T(18)+T(15)+T(12)+T(28)+T(26)+T(18)+T(17)+T(17)));
//G255: measure_preprocess_opt(44) => __E_bool_option_55(148).
	p(NT(44), (NT(148)));
//G256: bool_option(126)     => measure_preprocess_opt(44).
	p(NT(126), (NT(44)));
//G257: __E_bool_option_56(149) => 'd'.
	p(NT(149), (T(29)));
//G258: __E_bool_option_56(149) => 'd' 'e' 'b' 'u' 'g'.
	p(NT(149), (T(29)+T(18)+T(9)+T(25)+T(22)));
//G259: debug_opt(45)        => __E_bool_option_56(149).
	p(NT(45), (NT(149)));
//G260: bool_option(126)     => debug_opt(45).
	p(NT(126), (NT(45)));
//G261: __E_bool_option_57(150) => 'a' 'd'.
	p(NT(150), (T(16)+T(29)));
//G262: __E_bool_option_57(150) => 'a' 'u' 't' 'o' '-' 'd' 'i' 's' 'a' 'm' 'b' 'i' 'g' 'u' 'a' 't' 'e'.
	p(NT(150), (T(16)+T(25)+T(13)+T(28)+T(24)+T(29)+T(20)+T(17)+T(16)+T(23)+T(9)+T(20)+T(22)+T(25)+T(16)+T(13)+T(18)));
//G263: auto_disambiguate_opt(46) => __E_bool_option_57(150).
	p(NT(46), (NT(150)));
//G264: bool_option(126)     => auto_disambiguate_opt(46).
	p(NT(126), (NT(46)));
//G265: __E_bool_option_58(151) => 't' 't'.
	p(NT(151), (T(13)+T(13)));
//G266: __E_bool_option_58(151) => 't' 'r' 'i' 'm' '-' 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
	p(NT(151), (T(13)+T(12)+T(20)+T(23)+T(24)+T(13)+T(18)+T(12)+T(23)+T(20)+T(11)+T(16)+T(21)+T(17)));
//G267: trim_terminals_opt(47) => __E_bool_option_58(151).
	p(NT(47), (NT(151)));
//G268: bool_option(126)     => trim_terminals_opt(47).
	p(NT(126), (NT(47)));
//G269: __E_bool_option_59(152) => 'i' 'c' 'c'.
	p(NT(152), (T(20)+T(26)+T(26)));
//G270: __E_bool_option_59(152) => 'i' 'n' 'l' 'i' 'n' 'e' '-' 'c' 'c'.
	p(NT(152), (T(20)+T(11)+T(21)+T(20)+T(11)+T(18)+T(24)+T(26)+T(26)));
//G271: __E_bool_option_59(152) => 'i' 'n' 'l' 'i' 'n' 'e' '-' 'c' 'h' 'a' 'r' '-' 'c' 'l' 'a' 's' 's' 'e' 's'.
	p(NT(152), (T(20)+T(11)+T(21)+T(20)+T(11)+T(18)+T(24)+T(26)+T(27)+T(16)+T(12)+T(24)+T(26)+T(21)+T(16)+T(17)+T(17)+T(18)+T(17)));
//G272: inline_cc_opt(48)    => __E_bool_option_59(152).
	p(NT(48), (NT(152)));
//G273: bool_option(126)     => inline_cc_opt(48).
	p(NT(126), (NT(48)));
//G274: __E_list_option_60(153) => 'n' 'd'.
	p(NT(153), (T(11)+T(29)));
//G275: __E_list_option_60(153) => 'n' 'd' 'l'.
	p(NT(153), (T(11)+T(29)+T(21)));
//G276: __E_list_option_60(153) => 'n' 'o' 'd' 'i' 's' 'a' 'm' 'b' 'i' 'g' '-' 'l' 'i' 's' 't'.
	p(NT(153), (T(11)+T(28)+T(29)+T(20)+T(17)+T(16)+T(23)+T(9)+T(20)+T(22)+T(24)+T(21)+T(20)+T(17)+T(13)));
//G277: nodisambig_list_opt(49) => __E_list_option_60(153).
	p(NT(49), (NT(153)));
//G278: list_option(119)     => nodisambig_list_opt(49).
	p(NT(119), (NT(49)));
//G279: trim_opt(50)         => 't' 'r' 'i' 'm'.
	p(NT(50), (T(13)+T(12)+T(20)+T(23)));
//G280: list_option(119)     => trim_opt(50).
	p(NT(119), (NT(50)));
//G281: __E_list_option_61(154) => 't' 'c'.
	p(NT(154), (T(13)+T(26)));
//G282: __E_list_option_61(154) => 't' 'r' 'i' 'm' '-' 'c' 'h' 'i' 'l' 'd' 'r' 'e' 'n'.
	p(NT(154), (T(13)+T(12)+T(20)+T(23)+T(24)+T(26)+T(27)+T(20)+T(21)+T(29)+T(12)+T(18)+T(11)));
//G283: trim_children_opt(51) => __E_list_option_61(154).
	p(NT(51), (NT(154)));
//G284: list_option(119)     => trim_children_opt(51).
	p(NT(119), (NT(51)));
//G285: __E_treepaths_option_62(155) => 'i'.
	p(NT(155), (T(20)));
//G286: __E_treepaths_option_62(155) => 'i' 'n' 'l' 'i' 'n' 'e'.
	p(NT(155), (T(20)+T(11)+T(21)+T(20)+T(11)+T(18)));
//G287: inline_opt(52)       => __E_treepaths_option_62(155).
	p(NT(52), (NT(155)));
//G288: treepaths_option(121) => inline_opt(52).
	p(NT(121), (NT(52)));
//G289: __E_bool_value_63(156) => 't'.
	p(NT(156), (T(13)));
//G290: __E_bool_value_63(156) => 't' 'r' 'u' 'e'.
	p(NT(156), (T(13)+T(12)+T(25)+T(18)));
//G291: __E_bool_value_63(156) => 'o' 'n'.
	p(NT(156), (T(28)+T(11)));
//G292: __E_bool_value_63(156) => '1'.
	p(NT(156), (T(35)));
//G293: __E_bool_value_63(156) => 'y'.
	p(NT(156), (T(34)));
//G294: __E_bool_value_63(156) => 'y' 'e' 's'.
	p(NT(156), (T(34)+T(18)+T(17)));
//G295: true_value(28)       => __E_bool_value_63(156).
	p(NT(28), (NT(156)));
//G296: bool_value(128)      => true_value(28).
	p(NT(128), (NT(28)));
//G297: __E_bool_value_64(157) => 'f'.
	p(NT(157), (T(10)));
//G298: __E_bool_value_64(157) => 'f' 'a' 'l' 's' 'e'.
	p(NT(157), (T(10)+T(16)+T(21)+T(17)+T(18)));
//G299: __E_bool_value_64(157) => 'o' 'f' 'f'.
	p(NT(157), (T(28)+T(10)+T(10)));
//G300: __E_bool_value_64(157) => '0'.
	p(NT(157), (T(36)));
//G301: __E_bool_value_64(157) => 'n'.
	p(NT(157), (T(11)));
//G302: __E_bool_value_64(157) => 'n' 'o'.
	p(NT(157), (T(11)+T(28)));
//G303: false_value(29)      => __E_bool_value_64(157).
	p(NT(29), (NT(157)));
//G304: bool_value(128)      => false_value(29).
	p(NT(128), (NT(29)));
//G305: __E_symbol_list_65(158) => _(7) ',' _(7) symbol(73).
	p(NT(158), (NT(7)+T(37)+NT(7)+NT(73)));
//G306: __E_symbol_list_66(159) => null.
	p(NT(159), (nul));
//G307: __E_symbol_list_66(159) => __E_symbol_list_65(158) __E_symbol_list_66(159).
	p(NT(159), (NT(158)+NT(159)));
//G308: symbol_list(120)     => symbol(73) __E_symbol_list_66(159).
	p(NT(120), (NT(73)+NT(159)));
//G309: __E_treepath_list_67(161) => _(7) ',' _(7) treepath(160).
	p(NT(161), (NT(7)+T(37)+NT(7)+NT(160)));
//G310: __E_treepath_list_68(162) => null.
	p(NT(162), (nul));
//G311: __E_treepath_list_68(162) => __E_treepath_list_67(161) __E_treepath_list_68(162).
	p(NT(162), (NT(161)+NT(162)));
//G312: treepath_list(122)   => treepath(160) __E_treepath_list_68(162).
	p(NT(122), (NT(160)+NT(162)));
//G313: __E_treepath_69(163) => _(7) '>' _(7) symbol(73).
	p(NT(163), (NT(7)+T(38)+NT(7)+NT(73)));
//G314: __E_treepath_70(164) => null.
	p(NT(164), (nul));
//G315: __E_treepath_70(164) => __E_treepath_69(163) __E_treepath_70(164).
	p(NT(164), (NT(163)+NT(164)));
//G316: treepath(160)        => symbol(73) __E_treepath_70(164).
	p(NT(160), (NT(73)+NT(164)));
//G317: __E_error_verbosity_71(165) => 'b'.
	p(NT(165), (T(9)));
//G318: __E_error_verbosity_71(165) => 'b' 'a' 's' 'i' 'c'.
	p(NT(165), (T(9)+T(16)+T(17)+T(20)+T(26)));
//G319: basic_sym(30)        => __E_error_verbosity_71(165).
	p(NT(30), (NT(165)));
//G320: error_verbosity(133) => basic_sym(30).
	p(NT(133), (NT(30)));
//G321: __E_error_verbosity_72(166) => 'd'.
	p(NT(166), (T(29)));
//G322: __E_error_verbosity_72(166) => 'd' 'e' 't' 'a' 'i' 'l' 'e' 'd'.
	p(NT(166), (T(29)+T(18)+T(13)+T(16)+T(20)+T(21)+T(18)+T(29)));
//G323: detailed_sym(31)     => __E_error_verbosity_72(166).
	p(NT(31), (NT(166)));
//G324: error_verbosity(133) => detailed_sym(31).
	p(NT(133), (NT(31)));
//G325: __E_error_verbosity_73(167) => 'r'.
	p(NT(167), (T(12)));
//G326: __E_error_verbosity_73(167) => 'r' 'c'.
	p(NT(167), (T(12)+T(26)));
//G327: __E_error_verbosity_73(167) => 'r' 'o' 'o' 't' '-' 'c' 'a' 'u' 's' 'e'.
	p(NT(167), (T(12)+T(28)+T(28)+T(13)+T(24)+T(26)+T(16)+T(25)+T(17)+T(18)));
//G328: root_cause_sym(32)   => __E_error_verbosity_73(167).
	p(NT(32), (NT(167)));
//G329: error_verbosity(133) => root_cause_sym(32).
	p(NT(133), (NT(32)));
	#undef T
	#undef NT
	return loaded = true, p;
}

static inline ::idni::grammar<char_type, terminal_type> grammar(
	nts, productions(), start_symbol, char_classes, grammar_options);

} // namespace tgf_repl_parser_data

struct tgf_repl_parser : public idni::parser<char, char> {
	enum nonterminal {
		nul, eof, alnum, alpha, space, printable, parse_input_char, _, __, grammar_sym, 
		internal_grammar_sym, unreachable_sym, start_sym, parse_sym, parse_file_sym, load_sym, reload_sym, help_sym, quit_sym, version_sym, 
		clear_sym, get_sym, set_sym, add_sym, del_sym, toggle_sym, enable_sym, disable_sym, true_value, false_value, 
		basic_sym, detailed_sym, root_cause_sym, error_verbosity_opt, status_opt, colors_opt, print_ambiguity_opt, print_graphs_opt, print_rules_opt, print_facts_opt, 
		print_terminals_opt, measure_parsing_opt, measure_each_pos_opt, measure_forest_opt, measure_preprocess_opt, debug_opt, auto_disambiguate_opt, trim_terminals_opt, inline_cc_opt, nodisambig_list_opt, 
		trim_opt, trim_children_opt, inline_opt, start, __E_start_0, statement, __E___E_start_0_1, __E___E_start_0_2, __E___3, __E____4, 
		comment, __E_comment_5, __E_comment_6, __E_comment_7, quoted_string, quoted_string_char, __E_quoted_string_8, unescaped_s, escaped_s, __E_unescaped_s_9, 
		__E_unescaped_s_10, __E_escaped_s_11, filename, symbol, __E_symbol_12, __E_symbol_13, __E_symbol_14, parse_input, parse_input_char_seq, __E_parse_input_char_seq_15, 
		grammar_cmd, internal_grammar_cmd, unreachable_cmd, reload_cmd, load_cmd, start_cmd, help, version, quit, clear, 
		get, set, toggle, enable, disable, add, del, parse_file_cmd, parse_cmd, __E_parse_cmd_16, 
		__E_parse_file_cmd_17, __E_grammar_cmd_18, __E_internal_grammar_cmd_19, __E_internal_grammar_cmd_20, __E_start_cmd_21, __E_start_cmd_22, __E_unreachable_cmd_23, __E_unreachable_cmd_24, __E_reload_cmd_25, __E_load_cmd_26, 
		__E_help_27, __E_help_28, cmd_symbol, __E_version_29, __E_quit_30, __E_clear_31, __E_get_32, option, __E_add_33, list_option, 
		symbol_list, treepaths_option, treepath_list, __E_del_34, __E_del_35, __E_set_36, bool_option, __E___E_set_36_37, bool_value, __E___E_set_36_38, 
		__E___E_set_36_39, enum_ev_option, __E___E_set_36_40, error_verbosity, __E_toggle_41, __E_enable_42, __E_disable_43, __E_enum_ev_option_44, __E_bool_option_45, __E_bool_option_46, 
		__E_bool_option_47, __E_bool_option_48, __E_bool_option_49, __E_bool_option_50, __E_bool_option_51, __E_bool_option_52, __E_bool_option_53, __E_bool_option_54, __E_bool_option_55, __E_bool_option_56, 
		__E_bool_option_57, __E_bool_option_58, __E_bool_option_59, __E_list_option_60, __E_list_option_61, __E_treepaths_option_62, __E_bool_value_63, __E_bool_value_64, __E_symbol_list_65, __E_symbol_list_66, 
		treepath, __E_treepath_list_67, __E_treepath_list_68, __E_treepath_69, __E_treepath_70, __E_error_verbosity_71, __E_error_verbosity_72, __E_error_verbosity_73, __N_0, 
	};
	static tgf_repl_parser& instance() {
		static tgf_repl_parser inst;
		return inst;
	}
	tgf_repl_parser() : idni::parser<char_type, terminal_type>(
		tgf_repl_parser_data::grammar,
		tgf_repl_parser_data::parser_options) {}
};

#endif // __TGF_REPL_PARSER_H__
