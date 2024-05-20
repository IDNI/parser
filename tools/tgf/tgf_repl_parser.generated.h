// This file is generated from a file tools/tgf/tgf_repl.tgf by
//       https://github.com/IDNI/parser/tools/tgf
//
#ifndef __TGF_REPL_PARSER_H__
#define __TGF_REPL_PARSER_H__

#include "parser.h"

namespace tgf_repl_parser_data {

using char_type     = char;
using terminal_type = char;

inline std::vector<std::string> symbol_names{
	"", "eof", "alnum", "alpha", "space", "printable", "_", "__", "grammar_sym", "internal_grammar_sym", 
	"unreachable_sym", "start_sym", "parse_sym", "parse_file_sym", "load_sym", "reload_sym", "help_sym", "quit_sym", "version_sym", "clear_sym", 
	"get_sym", "set_sym", "add_sym", "del_sym", "toggle_sym", "enable_sym", "disable_sym", "true_value", "false_value", "basic_sym", 
	"detailed_sym", "root_cause_sym", "error_verbosity_opt", "status_opt", "colors_opt", "print_ambiguity_opt", "print_graphs_opt", "print_rules_opt", "print_facts_opt", "print_terminals_opt", 
	"measure_parsing_opt", "measure_each_pos_opt", "measure_forest_opt", "measure_preprocess_opt", "debug_opt", "auto_disambiguate_opt", "trim_terminals_opt", "inline_cc_opt", "nodisambig_list_opt", "trim_opt", 
	"trim_children_opt", "inline_opt", "start", "__E_start_0", "statement", "__E___E_start_0_1", "__E___E_start_0_2", "grammar_cmd", "igrammar_cmd", "unreachable_cmd", 
	"reload_cmd", "load_cmd", "start_cmd", "help", "version", "quit", "clear", "get", "set", "toggle", 
	"enable", "disable", "add", "del", "parse_file_cmd", "parse_cmd", "__E_parse_cmd_3", "parse_input", "__E_parse_file_cmd_4", "filename", 
	"__E_grammar_cmd_5", "igrammar_sym", "__E_igrammar_cmd_6", "__E_igrammar_cmd_7", "symbol", "__E_start_cmd_8", "__E_start_cmd_9", "__E_unreachable_cmd_10", "__E_unreachable_cmd_11", "__E_reload_cmd_12", 
	"__E_load_cmd_13", "__E_help_14", "__E_help_15", "help_arg", "__E_version_16", "__E_quit_17", "__E_clear_18", "__E_get_19", "option", "__E_add_20", 
	"list_option", "symbol_list", "treepaths_option", "treepath_list", "__E_del_21", "__E_del_22", "__E_toggle_23", "bool_option", "__E_enable_24", "__E_disable_25", 
	"__E_set_26", "__E___E_set_26_27", "bool_value", "__E___E_set_26_28", "__E___E_set_26_29", "enum_ev_option", "__E___E_set_26_30", "error_verbosity", "quoted_string", "parse_input_char_seq", 
	"__E_parse_input_31", "__E_parse_input_32", "__E_enum_ev_option_33", "__E_bool_option_34", "__E_bool_option_35", "__E_bool_option_36", "__E_bool_option_37", "__E_bool_option_38", "__E_bool_option_39", "__E_bool_option_40", 
	"__E_bool_option_41", "__E_bool_option_42", "__E_bool_option_43", "__E_bool_option_44", "__E_bool_option_45", "__E_bool_option_46", "__E_bool_option_47", "__E_bool_option_48", "__E_list_option_49", "__E_list_option_50", 
	"trim_children_terminals_opt", "__E_list_option_51", "__E_treepaths_option_52", "__E_bool_value_53", "__E_bool_value_54", "__E_error_verbosity_55", "__E_error_verbosity_56", "__E_error_verbosity_57", "__E_symbol_58", "__E_symbol_59", 
	"__E_symbol_60", "__E_symbol_list_61", "__E_symbol_list_62", "treepath", "__E_treepath_63", "__E_treepath_64", "__E_treepath_list_65", "__E_treepath_list_66", "quoted_string_char", "__E_quoted_string_67", 
	"unescaped_s", "escaped_s", "__E_unescaped_s_68", "__E_unescaped_s_69", "__E_escaped_s_70", "__E___71", "__E____72", "comment", "__E_comment_73", "__E_comment_74", 
	"__E_comment_75", "__N_0", 
};

inline ::idni::nonterminals<char_type, terminal_type> nts{symbol_names};

inline std::vector<terminal_type> terminals{
	'\0', '.', 'p', 'a', 'r', 's', 'e', 'f', ' ', 
	'i', 'l', 'g', 'm', 'n', 't', '-', 'u', 'c', 'h', 
	'b', 'o', 'd', 'v', 'q', 'x', '=', '\t', 'y', '1', 
	'0', '_', ',', '>', '"', '\\', '/', '\r', '\n', '#', 
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
			6, 7
		},
		.to_trim_children = {
			8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
			18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
			28, 29, 30, 31, 32, 33, 34, 35, 36, 37,
			38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
			48, 49, 50, 51
		},
		.trim_terminals = false,
		.inline_char_classes = true
	}
};

inline ::idni::parser<char_type, terminal_type>::options parser_options{
};

inline ::idni::prods<char_type, terminal_type> start_symbol{ nts(52) };

inline idni::prods<char_type, terminal_type>& productions() {
	static bool loaded = false;
	static idni::prods<char_type, terminal_type>
		p, nul(idni::lit<char_type, terminal_type>{});
	if (loaded) return p;
	#define  T(x) (idni::prods<char_type, terminal_type>{ terminals[x] })
	#define NT(x) (idni::prods<char_type, terminal_type>{ nts(x) })
//G0:   __E___E_start_0_1(55) => _(6) '.' _(6) statement(54).
	p(NT(55), (NT(6)+T(1)+NT(6)+NT(54)));
//G1:   __E___E_start_0_2(56) => null.
	p(NT(56), (nul));
//G2:   __E___E_start_0_2(56) => __E___E_start_0_1(55) __E___E_start_0_2(56).
	p(NT(56), (NT(55)+NT(56)));
//G3:   __E_start_0(53)      => statement(54) __E___E_start_0_2(56) _(6).
	p(NT(53), (NT(54)+NT(56)+NT(6)));
//G4:   __E_start_0(53)      => null.
	p(NT(53), (nul));
//G5:   start(52)            => _(6) __E_start_0(53).
	p(NT(52), (NT(6)+NT(53)));
//G6:   statement(54)        => grammar_cmd(57).
	p(NT(54), (NT(57)));
//G7:   statement(54)        => igrammar_cmd(58).
	p(NT(54), (NT(58)));
//G8:   statement(54)        => unreachable_cmd(59).
	p(NT(54), (NT(59)));
//G9:   statement(54)        => reload_cmd(60).
	p(NT(54), (NT(60)));
//G10:  statement(54)        => load_cmd(61).
	p(NT(54), (NT(61)));
//G11:  statement(54)        => start_cmd(62).
	p(NT(54), (NT(62)));
//G12:  statement(54)        => help(63).
	p(NT(54), (NT(63)));
//G13:  statement(54)        => version(64).
	p(NT(54), (NT(64)));
//G14:  statement(54)        => quit(65).
	p(NT(54), (NT(65)));
//G15:  statement(54)        => clear(66).
	p(NT(54), (NT(66)));
//G16:  statement(54)        => get(67).
	p(NT(54), (NT(67)));
//G17:  statement(54)        => set(68).
	p(NT(54), (NT(68)));
//G18:  statement(54)        => toggle(69).
	p(NT(54), (NT(69)));
//G19:  statement(54)        => enable(70).
	p(NT(54), (NT(70)));
//G20:  statement(54)        => disable(71).
	p(NT(54), (NT(71)));
//G21:  statement(54)        => add(72).
	p(NT(54), (NT(72)));
//G22:  statement(54)        => del(73).
	p(NT(54), (NT(73)));
//G23:  statement(54)        => parse_file_cmd(74).
	p(NT(54), (NT(74)));
//G24:  statement(54)        => parse_cmd(75).
	p(NT(54), (NT(75)));
//G25:  __E_parse_cmd_3(76)  => 'p'.
	p(NT(76), (T(2)));
//G26:  __E_parse_cmd_3(76)  => 'p' 'a' 'r' 's' 'e'.
	p(NT(76), (T(2)+T(3)+T(4)+T(5)+T(6)));
//G27:  parse_sym(12)        => __E_parse_cmd_3(76).
	p(NT(12), (NT(76)));
//G28:  parse_cmd(75)        => parse_sym(12) __(7) parse_input(77).
	p(NT(75), (NT(12)+NT(7)+NT(77)));
//G29:  __E_parse_file_cmd_4(78) => 'f'.
	p(NT(78), (T(7)));
//G30:  __E_parse_file_cmd_4(78) => 'p' 'f'.
	p(NT(78), (T(2)+T(7)));
//G31:  __E_parse_file_cmd_4(78) => 'p' 'a' 'r' 's' 'e' ' ' 'f' 'i' 'l' 'e'.
	p(NT(78), (T(2)+T(3)+T(4)+T(5)+T(6)+T(8)+T(7)+T(9)+T(10)+T(6)));
//G32:  parse_file_sym(13)   => __E_parse_file_cmd_4(78).
	p(NT(13), (NT(78)));
//G33:  parse_file_cmd(74)   => parse_file_sym(13) __(7) filename(79).
	p(NT(74), (NT(13)+NT(7)+NT(79)));
//G34:  __E_grammar_cmd_5(80) => 'g'.
	p(NT(80), (T(11)));
//G35:  __E_grammar_cmd_5(80) => 'g' 'r' 'a' 'm' 'm' 'a' 'r'.
	p(NT(80), (T(11)+T(4)+T(3)+T(12)+T(12)+T(3)+T(4)));
//G36:  grammar_sym(8)       => __E_grammar_cmd_5(80).
	p(NT(8), (NT(80)));
//G37:  grammar_cmd(57)      => grammar_sym(8).
	p(NT(57), (NT(8)));
//G38:  __E_igrammar_cmd_6(82) => 'i'.
	p(NT(82), (T(9)));
//G39:  __E_igrammar_cmd_6(82) => 'i' 'g'.
	p(NT(82), (T(9)+T(11)));
//G40:  __E_igrammar_cmd_6(82) => 'i' 'n' 't' 'e' 'r' 'n' 'a' 'l' '-' 'g' 'r' 'a' 'm' 'm' 'a' 'r'.
	p(NT(82), (T(9)+T(13)+T(14)+T(6)+T(4)+T(13)+T(3)+T(10)+T(15)+T(11)+T(4)+T(3)+T(12)+T(12)+T(3)+T(4)));
//G41:  igrammar_sym(81)     => __E_igrammar_cmd_6(82).
	p(NT(81), (NT(82)));
//G42:  __E_igrammar_cmd_7(83) => __(7) symbol(84).
	p(NT(83), (NT(7)+NT(84)));
//G43:  __E_igrammar_cmd_7(83) => null.
	p(NT(83), (nul));
//G44:  igrammar_cmd(58)     => igrammar_sym(81) __E_igrammar_cmd_7(83).
	p(NT(58), (NT(81)+NT(83)));
//G45:  __E_start_cmd_8(85)  => 's'.
	p(NT(85), (T(5)));
//G46:  __E_start_cmd_8(85)  => 's' 't' 'a' 'r' 't'.
	p(NT(85), (T(5)+T(14)+T(3)+T(4)+T(14)));
//G47:  start_sym(11)        => __E_start_cmd_8(85).
	p(NT(11), (NT(85)));
//G48:  __E_start_cmd_9(86)  => __(7) symbol(84).
	p(NT(86), (NT(7)+NT(84)));
//G49:  __E_start_cmd_9(86)  => null.
	p(NT(86), (nul));
//G50:  start_cmd(62)        => start_sym(11) __E_start_cmd_9(86).
	p(NT(62), (NT(11)+NT(86)));
//G51:  __E_unreachable_cmd_10(87) => 'u'.
	p(NT(87), (T(16)));
//G52:  __E_unreachable_cmd_10(87) => 'u' 'n' 'r' 'e' 'a' 'c' 'h' 'a' 'b' 'l' 'e'.
	p(NT(87), (T(16)+T(13)+T(4)+T(6)+T(3)+T(17)+T(18)+T(3)+T(19)+T(10)+T(6)));
//G53:  unreachable_sym(10)  => __E_unreachable_cmd_10(87).
	p(NT(10), (NT(87)));
//G54:  __E_unreachable_cmd_11(88) => __(7) symbol(84).
	p(NT(88), (NT(7)+NT(84)));
//G55:  __E_unreachable_cmd_11(88) => null.
	p(NT(88), (nul));
//G56:  unreachable_cmd(59)  => unreachable_sym(10) __E_unreachable_cmd_11(88).
	p(NT(59), (NT(10)+NT(88)));
//G57:  __E_reload_cmd_12(89) => 'r'.
	p(NT(89), (T(4)));
//G58:  __E_reload_cmd_12(89) => 'r' 'e' 'l' 'o' 'a' 'd'.
	p(NT(89), (T(4)+T(6)+T(10)+T(20)+T(3)+T(21)));
//G59:  reload_sym(15)       => __E_reload_cmd_12(89).
	p(NT(15), (NT(89)));
//G60:  reload_cmd(60)       => reload_sym(15).
	p(NT(60), (NT(15)));
//G61:  __E_load_cmd_13(90)  => 'l'.
	p(NT(90), (T(10)));
//G62:  __E_load_cmd_13(90)  => 'l' 'o' 'a' 'd'.
	p(NT(90), (T(10)+T(20)+T(3)+T(21)));
//G63:  load_sym(14)         => __E_load_cmd_13(90).
	p(NT(14), (NT(90)));
//G64:  load_cmd(61)         => load_sym(14) __(7) filename(79).
	p(NT(61), (NT(14)+NT(7)+NT(79)));
//G65:  __E_help_14(91)      => 'h'.
	p(NT(91), (T(18)));
//G66:  __E_help_14(91)      => 'h' 'e' 'l' 'p'.
	p(NT(91), (T(18)+T(6)+T(10)+T(2)));
//G67:  help_sym(16)         => __E_help_14(91).
	p(NT(16), (NT(91)));
//G68:  __E_help_15(92)      => __(7) help_arg(93).
	p(NT(92), (NT(7)+NT(93)));
//G69:  __E_help_15(92)      => null.
	p(NT(92), (nul));
//G70:  help(63)             => help_sym(16) __E_help_15(92).
	p(NT(63), (NT(16)+NT(92)));
//G71:  __E_version_16(94)   => 'v'.
	p(NT(94), (T(22)));
//G72:  __E_version_16(94)   => 'v' 'e' 'r' 's' 'i' 'o' 'n'.
	p(NT(94), (T(22)+T(6)+T(4)+T(5)+T(9)+T(20)+T(13)));
//G73:  version_sym(18)      => __E_version_16(94).
	p(NT(18), (NT(94)));
//G74:  version(64)          => version_sym(18).
	p(NT(64), (NT(18)));
//G75:  __E_quit_17(95)      => 'q'.
	p(NT(95), (T(23)));
//G76:  __E_quit_17(95)      => 'q' 'u' 'i' 't'.
	p(NT(95), (T(23)+T(16)+T(9)+T(14)));
//G77:  __E_quit_17(95)      => 'e'.
	p(NT(95), (T(6)));
//G78:  __E_quit_17(95)      => 'e' 'x' 'i' 't'.
	p(NT(95), (T(6)+T(24)+T(9)+T(14)));
//G79:  quit_sym(17)         => __E_quit_17(95).
	p(NT(17), (NT(95)));
//G80:  quit(65)             => quit_sym(17).
	p(NT(65), (NT(17)));
//G81:  __E_clear_18(96)     => 'c' 'l' 's'.
	p(NT(96), (T(17)+T(10)+T(5)));
//G82:  __E_clear_18(96)     => 'c' 'l' 'e' 'a' 'r'.
	p(NT(96), (T(17)+T(10)+T(6)+T(3)+T(4)));
//G83:  clear_sym(19)        => __E_clear_18(96).
	p(NT(19), (NT(96)));
//G84:  clear(66)            => clear_sym(19).
	p(NT(66), (NT(19)));
//G85:  get_sym(20)          => 'g' 'e' 't'.
	p(NT(20), (T(11)+T(6)+T(14)));
//G86:  __E_get_19(97)       => __(7) option(98).
	p(NT(97), (NT(7)+NT(98)));
//G87:  __E_get_19(97)       => null.
	p(NT(97), (nul));
//G88:  get(67)              => get_sym(20) __E_get_19(97).
	p(NT(67), (NT(20)+NT(97)));
//G89:  add_sym(22)          => 'a' 'd' 'd'.
	p(NT(22), (T(3)+T(21)+T(21)));
//G90:  __E_add_20(99)       => list_option(100) __(7) symbol_list(101).
	p(NT(99), (NT(100)+NT(7)+NT(101)));
//G91:  __E_add_20(99)       => treepaths_option(102) __(7) treepath_list(103).
	p(NT(99), (NT(102)+NT(7)+NT(103)));
//G92:  add(72)              => add_sym(22) __(7) __E_add_20(99).
	p(NT(72), (NT(22)+NT(7)+NT(99)));
//G93:  __E_del_21(104)      => 'd' 'e' 'l'.
	p(NT(104), (T(21)+T(6)+T(10)));
//G94:  __E_del_21(104)      => 'd' 'e' 'l' 'e' 't' 'e'.
	p(NT(104), (T(21)+T(6)+T(10)+T(6)+T(14)+T(6)));
//G95:  __E_del_21(104)      => 'r' 'm'.
	p(NT(104), (T(4)+T(12)));
//G96:  __E_del_21(104)      => 'r' 'e' 'm'.
	p(NT(104), (T(4)+T(6)+T(12)));
//G97:  __E_del_21(104)      => 'r' 'e' 'm' 'o' 'v' 'e'.
	p(NT(104), (T(4)+T(6)+T(12)+T(20)+T(22)+T(6)));
//G98:  del_sym(23)          => __E_del_21(104).
	p(NT(23), (NT(104)));
//G99:  __E_del_22(105)      => list_option(100) __(7) symbol_list(101).
	p(NT(105), (NT(100)+NT(7)+NT(101)));
//G100: __E_del_22(105)      => treepaths_option(102) __(7) treepath_list(103).
	p(NT(105), (NT(102)+NT(7)+NT(103)));
//G101: del(73)              => del_sym(23) __(7) __E_del_22(105).
	p(NT(73), (NT(23)+NT(7)+NT(105)));
//G102: __E_toggle_23(106)   => 't' 'o' 'g'.
	p(NT(106), (T(14)+T(20)+T(11)));
//G103: __E_toggle_23(106)   => 't' 'o' 'g' 'g' 'l' 'e'.
	p(NT(106), (T(14)+T(20)+T(11)+T(11)+T(10)+T(6)));
//G104: toggle_sym(24)       => __E_toggle_23(106).
	p(NT(24), (NT(106)));
//G105: toggle(69)           => toggle_sym(24) __(7) bool_option(107).
	p(NT(69), (NT(24)+NT(7)+NT(107)));
//G106: __E_enable_24(108)   => 'e' 'n' __(7).
	p(NT(108), (T(6)+T(13)+NT(7)));
//G107: __E_enable_24(108)   => 'e' 'n' 'a' 'b' 'l' 'e' __(7).
	p(NT(108), (T(6)+T(13)+T(3)+T(19)+T(10)+T(6)+NT(7)));
//G108: enable_sym(25)       => __E_enable_24(108).
	p(NT(25), (NT(108)));
//G109: enable(70)           => enable_sym(25) bool_option(107).
	p(NT(70), (NT(25)+NT(107)));
//G110: __E_disable_25(109)  => 'd' 'i' 's' __(7).
	p(NT(109), (T(21)+T(9)+T(5)+NT(7)));
//G111: __E_disable_25(109)  => 'd' 'i' 's' 'a' 'b' 'l' 'e' __(7).
	p(NT(109), (T(21)+T(9)+T(5)+T(3)+T(19)+T(10)+T(6)+NT(7)));
//G112: disable_sym(26)      => __E_disable_25(109).
	p(NT(26), (NT(109)));
//G113: disable(71)          => disable_sym(26) bool_option(107).
	p(NT(71), (NT(26)+NT(107)));
//G114: set_sym(21)          => 's' 'e' 't'.
	p(NT(21), (T(5)+T(6)+T(14)));
//G115: __E___E_set_26_27(111) => __(7).
	p(NT(111), (NT(7)));
//G116: __E___E_set_26_27(111) => _(6) '=' _(6).
	p(NT(111), (NT(6)+T(25)+NT(6)));
//G117: __E_set_26(110)      => bool_option(107) __E___E_set_26_27(111) bool_value(112).
	p(NT(110), (NT(107)+NT(111)+NT(112)));
//G118: __E___E_set_26_28(113) => __(7).
	p(NT(113), (NT(7)));
//G119: __E___E_set_26_28(113) => _(6) '=' _(6).
	p(NT(113), (NT(6)+T(25)+NT(6)));
//G120: __E_set_26(110)      => list_option(100) __E___E_set_26_28(113) symbol_list(101).
	p(NT(110), (NT(100)+NT(113)+NT(101)));
//G121: __E___E_set_26_29(114) => __(7).
	p(NT(114), (NT(7)));
//G122: __E___E_set_26_29(114) => _(6) '=' _(6).
	p(NT(114), (NT(6)+T(25)+NT(6)));
//G123: __E_set_26(110)      => treepaths_option(102) __E___E_set_26_29(114) treepath_list(103).
	p(NT(110), (NT(102)+NT(114)+NT(103)));
//G124: __E___E_set_26_30(116) => __(7).
	p(NT(116), (NT(7)));
//G125: __E___E_set_26_30(116) => _(6) '=' _(6).
	p(NT(116), (NT(6)+T(25)+NT(6)));
//G126: __E_set_26(110)      => enum_ev_option(115) __E___E_set_26_30(116) error_verbosity(117).
	p(NT(110), (NT(115)+NT(116)+NT(117)));
//G127: set(68)              => set_sym(21) __(7) __E_set_26(110).
	p(NT(68), (NT(21)+NT(7)+NT(110)));
//G128: parse_input(77)      => quoted_string(118).
	p(NT(77), (NT(118)));
//G129: __E_parse_input_31(120) => printable(5).
	p(NT(120), (NT(5)));
//G130: __E_parse_input_31(120) => '\t'.
	p(NT(120), (T(26)));
//G131: __E_parse_input_32(121) => __E_parse_input_31(120).
	p(NT(121), (NT(120)));
//G132: __E_parse_input_32(121) => __E_parse_input_31(120) __E_parse_input_32(121).
	p(NT(121), (NT(120)+NT(121)));
//G133: parse_input_char_seq(119) => __E_parse_input_32(121).
	p(NT(119), (NT(121)));
//G134: parse_input(77)      => parse_input_char_seq(119).
	p(NT(77), (NT(119)));
//G135: help_arg(93)         => grammar_sym(8).
	p(NT(93), (NT(8)));
//G136: help_arg(93)         => igrammar_sym(81).
	p(NT(93), (NT(81)));
//G137: help_arg(93)         => unreachable_sym(10).
	p(NT(93), (NT(10)));
//G138: help_arg(93)         => start_sym(11).
	p(NT(93), (NT(11)));
//G139: help_arg(93)         => parse_sym(12).
	p(NT(93), (NT(12)));
//G140: help_arg(93)         => parse_file_sym(13).
	p(NT(93), (NT(13)));
//G141: help_arg(93)         => load_sym(14).
	p(NT(93), (NT(14)));
//G142: help_arg(93)         => reload_sym(15).
	p(NT(93), (NT(15)));
//G143: help_arg(93)         => help_sym(16).
	p(NT(93), (NT(16)));
//G144: help_arg(93)         => quit_sym(17).
	p(NT(93), (NT(17)));
//G145: help_arg(93)         => version_sym(18).
	p(NT(93), (NT(18)));
//G146: help_arg(93)         => clear_sym(19).
	p(NT(93), (NT(19)));
//G147: help_arg(93)         => get_sym(20).
	p(NT(93), (NT(20)));
//G148: help_arg(93)         => set_sym(21).
	p(NT(93), (NT(21)));
//G149: help_arg(93)         => add_sym(22).
	p(NT(93), (NT(22)));
//G150: help_arg(93)         => del_sym(23).
	p(NT(93), (NT(23)));
//G151: help_arg(93)         => toggle_sym(24).
	p(NT(93), (NT(24)));
//G152: help_arg(93)         => enable_sym(25).
	p(NT(93), (NT(25)));
//G153: help_arg(93)         => disable_sym(26).
	p(NT(93), (NT(26)));
//G154: option(98)           => bool_option(107).
	p(NT(98), (NT(107)));
//G155: option(98)           => enum_ev_option(115).
	p(NT(98), (NT(115)));
//G156: option(98)           => list_option(100).
	p(NT(98), (NT(100)));
//G157: option(98)           => treepaths_option(102).
	p(NT(98), (NT(102)));
//G158: __E_enum_ev_option_33(122) => 'e'.
	p(NT(122), (T(6)));
//G159: __E_enum_ev_option_33(122) => 'e' 'r' 'r' 'o' 'r' '-' 'v' 'e' 'r' 'b' 'o' 's' 'i' 't' 'y'.
	p(NT(122), (T(6)+T(4)+T(4)+T(20)+T(4)+T(15)+T(22)+T(6)+T(4)+T(19)+T(20)+T(5)+T(9)+T(14)+T(27)));
//G160: error_verbosity_opt(32) => __E_enum_ev_option_33(122).
	p(NT(32), (NT(122)));
//G161: enum_ev_option(115)  => error_verbosity_opt(32).
	p(NT(115), (NT(32)));
//G162: __E_bool_option_34(123) => 's'.
	p(NT(123), (T(5)));
//G163: __E_bool_option_34(123) => 's' 't' 'a' 't' 'u' 's'.
	p(NT(123), (T(5)+T(14)+T(3)+T(14)+T(16)+T(5)));
//G164: status_opt(33)       => __E_bool_option_34(123).
	p(NT(33), (NT(123)));
//G165: bool_option(107)     => status_opt(33).
	p(NT(107), (NT(33)));
//G166: __E_bool_option_35(124) => 'c'.
	p(NT(124), (T(17)));
//G167: __E_bool_option_35(124) => 'c' 'o' 'l' 'o' 'r'.
	p(NT(124), (T(17)+T(20)+T(10)+T(20)+T(4)));
//G168: __E_bool_option_35(124) => 'c' 'o' 'l' 'o' 'r' 's'.
	p(NT(124), (T(17)+T(20)+T(10)+T(20)+T(4)+T(5)));
//G169: colors_opt(34)       => __E_bool_option_35(124).
	p(NT(34), (NT(124)));
//G170: bool_option(107)     => colors_opt(34).
	p(NT(107), (NT(34)));
//G171: __E_bool_option_36(125) => 'a'.
	p(NT(125), (T(3)));
//G172: __E_bool_option_36(125) => 'a' 'm' 'b' 'i' 'g' 'u' 'i' 't' 'y'.
	p(NT(125), (T(3)+T(12)+T(19)+T(9)+T(11)+T(16)+T(9)+T(14)+T(27)));
//G173: __E_bool_option_36(125) => 'p' 'r' 'i' 'n' 't' '-' 'a' 'm' 'b' 'i' 'g' 'u' 'i' 't' 'y'.
	p(NT(125), (T(2)+T(4)+T(9)+T(13)+T(14)+T(15)+T(3)+T(12)+T(19)+T(9)+T(11)+T(16)+T(9)+T(14)+T(27)));
//G174: print_ambiguity_opt(35) => __E_bool_option_36(125).
	p(NT(35), (NT(125)));
//G175: bool_option(107)     => print_ambiguity_opt(35).
	p(NT(107), (NT(35)));
//G176: __E_bool_option_37(126) => 'g'.
	p(NT(126), (T(11)));
//G177: __E_bool_option_37(126) => 'g' 'r' 'a' 'p' 'h' 's'.
	p(NT(126), (T(11)+T(4)+T(3)+T(2)+T(18)+T(5)));
//G178: __E_bool_option_37(126) => 'p' 'r' 'i' 'n' 't' '-' 'g' 'r' 'a' 'p' 'h' 's'.
	p(NT(126), (T(2)+T(4)+T(9)+T(13)+T(14)+T(15)+T(11)+T(4)+T(3)+T(2)+T(18)+T(5)));
//G179: print_graphs_opt(36) => __E_bool_option_37(126).
	p(NT(36), (NT(126)));
//G180: bool_option(107)     => print_graphs_opt(36).
	p(NT(107), (NT(36)));
//G181: __E_bool_option_38(127) => 'r'.
	p(NT(127), (T(4)));
//G182: __E_bool_option_38(127) => 'r' 'u' 'l' 'e' 's'.
	p(NT(127), (T(4)+T(16)+T(10)+T(6)+T(5)));
//G183: __E_bool_option_38(127) => 'p' 'r' 'i' 'n' 't' '-' 'r' 'u' 'l' 'e' 's'.
	p(NT(127), (T(2)+T(4)+T(9)+T(13)+T(14)+T(15)+T(4)+T(16)+T(10)+T(6)+T(5)));
//G184: print_rules_opt(37)  => __E_bool_option_38(127).
	p(NT(37), (NT(127)));
//G185: bool_option(107)     => print_rules_opt(37).
	p(NT(107), (NT(37)));
//G186: __E_bool_option_39(128) => 'f'.
	p(NT(128), (T(7)));
//G187: __E_bool_option_39(128) => 'f' 'a' 'c' 't' 's'.
	p(NT(128), (T(7)+T(3)+T(17)+T(14)+T(5)));
//G188: __E_bool_option_39(128) => 'p' 'r' 'i' 'n' 't' '-' 'f' 'a' 'c' 't' 's'.
	p(NT(128), (T(2)+T(4)+T(9)+T(13)+T(14)+T(15)+T(7)+T(3)+T(17)+T(14)+T(5)));
//G189: print_facts_opt(38)  => __E_bool_option_39(128).
	p(NT(38), (NT(128)));
//G190: bool_option(107)     => print_facts_opt(38).
	p(NT(107), (NT(38)));
//G191: __E_bool_option_40(129) => 't'.
	p(NT(129), (T(14)));
//G192: __E_bool_option_40(129) => 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
	p(NT(129), (T(14)+T(6)+T(4)+T(12)+T(9)+T(13)+T(3)+T(10)+T(5)));
//G193: __E_bool_option_40(129) => 'p' 'r' 'i' 'n' 't' '-' 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
	p(NT(129), (T(2)+T(4)+T(9)+T(13)+T(14)+T(15)+T(14)+T(6)+T(4)+T(12)+T(9)+T(13)+T(3)+T(10)+T(5)));
//G194: print_terminals_opt(39) => __E_bool_option_40(129).
	p(NT(39), (NT(129)));
//G195: bool_option(107)     => print_terminals_opt(39).
	p(NT(107), (NT(39)));
//G196: __E_bool_option_41(130) => 'm'.
	p(NT(130), (T(12)));
//G197: __E_bool_option_41(130) => 'm' 'e' 'a' 's' 'u' 'r' 'e'.
	p(NT(130), (T(12)+T(6)+T(3)+T(5)+T(16)+T(4)+T(6)));
//G198: __E_bool_option_41(130) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'p' 'a' 'r' 's' 'i' 'n' 'g'.
	p(NT(130), (T(12)+T(6)+T(3)+T(5)+T(16)+T(4)+T(6)+T(15)+T(2)+T(3)+T(4)+T(5)+T(9)+T(13)+T(11)));
//G199: measure_parsing_opt(40) => __E_bool_option_41(130).
	p(NT(40), (NT(130)));
//G200: bool_option(107)     => measure_parsing_opt(40).
	p(NT(107), (NT(40)));
//G201: __E_bool_option_42(131) => 'm' 'e'.
	p(NT(131), (T(12)+T(6)));
//G202: __E_bool_option_42(131) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'e' 'a' 'c' 'h'.
	p(NT(131), (T(12)+T(6)+T(3)+T(5)+T(16)+T(4)+T(6)+T(15)+T(6)+T(3)+T(17)+T(18)));
//G203: __E_bool_option_42(131) => 'm' 'e' 'p'.
	p(NT(131), (T(12)+T(6)+T(2)));
//G204: __E_bool_option_42(131) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'e' 'a' 'c' 'h' '-' 'p' 'o' 's'.
	p(NT(131), (T(12)+T(6)+T(3)+T(5)+T(16)+T(4)+T(6)+T(15)+T(6)+T(3)+T(17)+T(18)+T(15)+T(2)+T(20)+T(5)));
//G205: measure_each_pos_opt(41) => __E_bool_option_42(131).
	p(NT(41), (NT(131)));
//G206: bool_option(107)     => measure_each_pos_opt(41).
	p(NT(107), (NT(41)));
//G207: __E_bool_option_43(132) => 'm' 'f'.
	p(NT(132), (T(12)+T(7)));
//G208: __E_bool_option_43(132) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'f' 'o' 'r' 'e' 's' 't'.
	p(NT(132), (T(12)+T(6)+T(3)+T(5)+T(16)+T(4)+T(6)+T(15)+T(7)+T(20)+T(4)+T(6)+T(5)+T(14)));
//G209: measure_forest_opt(42) => __E_bool_option_43(132).
	p(NT(42), (NT(132)));
//G210: bool_option(107)     => measure_forest_opt(42).
	p(NT(107), (NT(42)));
//G211: __E_bool_option_44(133) => 'm' 'p'.
	p(NT(133), (T(12)+T(2)));
//G212: __E_bool_option_44(133) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'p' 'r' 'e' 'p' 'r' 'o' 'c' 'e' 's' 's'.
	p(NT(133), (T(12)+T(6)+T(3)+T(5)+T(16)+T(4)+T(6)+T(15)+T(2)+T(4)+T(6)+T(2)+T(4)+T(20)+T(17)+T(6)+T(5)+T(5)));
//G213: measure_preprocess_opt(43) => __E_bool_option_44(133).
	p(NT(43), (NT(133)));
//G214: bool_option(107)     => measure_preprocess_opt(43).
	p(NT(107), (NT(43)));
//G215: __E_bool_option_45(134) => 'd'.
	p(NT(134), (T(21)));
//G216: __E_bool_option_45(134) => 'd' 'e' 'b' 'u' 'g'.
	p(NT(134), (T(21)+T(6)+T(19)+T(16)+T(11)));
//G217: debug_opt(44)        => __E_bool_option_45(134).
	p(NT(44), (NT(134)));
//G218: bool_option(107)     => debug_opt(44).
	p(NT(107), (NT(44)));
//G219: __E_bool_option_46(135) => 'a' 'd'.
	p(NT(135), (T(3)+T(21)));
//G220: __E_bool_option_46(135) => 'a' 'u' 't' 'o' '-' 'd' 'i' 's' 'a' 'm' 'b' 'i' 'g' 'u' 'a' 't' 'e'.
	p(NT(135), (T(3)+T(16)+T(14)+T(20)+T(15)+T(21)+T(9)+T(5)+T(3)+T(12)+T(19)+T(9)+T(11)+T(16)+T(3)+T(14)+T(6)));
//G221: auto_disambiguate_opt(45) => __E_bool_option_46(135).
	p(NT(45), (NT(135)));
//G222: bool_option(107)     => auto_disambiguate_opt(45).
	p(NT(107), (NT(45)));
//G223: __E_bool_option_47(136) => 't' 't'.
	p(NT(136), (T(14)+T(14)));
//G224: __E_bool_option_47(136) => 't' 'r' 'i' 'm' '-' 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
	p(NT(136), (T(14)+T(4)+T(9)+T(12)+T(15)+T(14)+T(6)+T(4)+T(12)+T(9)+T(13)+T(3)+T(10)+T(5)));
//G225: trim_terminals_opt(46) => __E_bool_option_47(136).
	p(NT(46), (NT(136)));
//G226: bool_option(107)     => trim_terminals_opt(46).
	p(NT(107), (NT(46)));
//G227: __E_bool_option_48(137) => 'i' 'c' 'c'.
	p(NT(137), (T(9)+T(17)+T(17)));
//G228: __E_bool_option_48(137) => 'i' 'n' 'l' 'i' 'n' 'e' '-' 'c' 'c'.
	p(NT(137), (T(9)+T(13)+T(10)+T(9)+T(13)+T(6)+T(15)+T(17)+T(17)));
//G229: __E_bool_option_48(137) => 'i' 'n' 'l' 'i' 'n' 'e' '-' 'c' 'h' 'a' 'r' '-' 'c' 'l' 'a' 's' 's' 'e' 's'.
	p(NT(137), (T(9)+T(13)+T(10)+T(9)+T(13)+T(6)+T(15)+T(17)+T(18)+T(3)+T(4)+T(15)+T(17)+T(10)+T(3)+T(5)+T(5)+T(6)+T(5)));
//G230: inline_cc_opt(47)    => __E_bool_option_48(137).
	p(NT(47), (NT(137)));
//G231: bool_option(107)     => inline_cc_opt(47).
	p(NT(107), (NT(47)));
//G232: __E_list_option_49(138) => 'n' 'd'.
	p(NT(138), (T(13)+T(21)));
//G233: __E_list_option_49(138) => 'n' 'd' 'l'.
	p(NT(138), (T(13)+T(21)+T(10)));
//G234: __E_list_option_49(138) => 'n' 'o' 'd' 'i' 's' 'a' 'm' 'b' 'i' 'g' '-' 'l' 'i' 's' 't'.
	p(NT(138), (T(13)+T(20)+T(21)+T(9)+T(5)+T(3)+T(12)+T(19)+T(9)+T(11)+T(15)+T(10)+T(9)+T(5)+T(14)));
//G235: nodisambig_list_opt(48) => __E_list_option_49(138).
	p(NT(48), (NT(138)));
//G236: list_option(100)     => nodisambig_list_opt(48).
	p(NT(100), (NT(48)));
//G237: trim_opt(49)         => 't' 'r' 'i' 'm'.
	p(NT(49), (T(14)+T(4)+T(9)+T(12)));
//G238: list_option(100)     => trim_opt(49).
	p(NT(100), (NT(49)));
//G239: __E_list_option_50(139) => 't' 'c'.
	p(NT(139), (T(14)+T(17)));
//G240: __E_list_option_50(139) => 't' 'r' 'i' 'm' '-' 'c' 'h' 'i' 'l' 'd' 'r' 'e' 'n'.
	p(NT(139), (T(14)+T(4)+T(9)+T(12)+T(15)+T(17)+T(18)+T(9)+T(10)+T(21)+T(4)+T(6)+T(13)));
//G241: trim_children_opt(50) => __E_list_option_50(139).
	p(NT(50), (NT(139)));
//G242: list_option(100)     => trim_children_opt(50).
	p(NT(100), (NT(50)));
//G243: __E_list_option_51(141) => 't' 'c' 't'.
	p(NT(141), (T(14)+T(17)+T(14)));
//G244: __E_list_option_51(141) => 't' 'r' 'i' 'm' '-' 'c' 'h' 'i' 'l' 'd' 'r' 'e' 'n' '-' 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
	p(NT(141), (T(14)+T(4)+T(9)+T(12)+T(15)+T(17)+T(18)+T(9)+T(10)+T(21)+T(4)+T(6)+T(13)+T(15)+T(14)+T(6)+T(4)+T(12)+T(9)+T(13)+T(3)+T(10)+T(5)));
//G245: trim_children_terminals_opt(140) => __E_list_option_51(141).
	p(NT(140), (NT(141)));
//G246: list_option(100)     => trim_children_terminals_opt(140).
	p(NT(100), (NT(140)));
//G247: __E_treepaths_option_52(142) => 'i'.
	p(NT(142), (T(9)));
//G248: __E_treepaths_option_52(142) => 'i' 'n' 'l' 'i' 'n' 'e'.
	p(NT(142), (T(9)+T(13)+T(10)+T(9)+T(13)+T(6)));
//G249: inline_opt(51)       => __E_treepaths_option_52(142).
	p(NT(51), (NT(142)));
//G250: treepaths_option(102) => inline_opt(51).
	p(NT(102), (NT(51)));
//G251: __E_bool_value_53(143) => 't'.
	p(NT(143), (T(14)));
//G252: __E_bool_value_53(143) => 't' 'r' 'u' 'e'.
	p(NT(143), (T(14)+T(4)+T(16)+T(6)));
//G253: __E_bool_value_53(143) => 'o' 'n'.
	p(NT(143), (T(20)+T(13)));
//G254: __E_bool_value_53(143) => '1'.
	p(NT(143), (T(28)));
//G255: __E_bool_value_53(143) => 'y'.
	p(NT(143), (T(27)));
//G256: __E_bool_value_53(143) => 'y' 'e' 's'.
	p(NT(143), (T(27)+T(6)+T(5)));
//G257: true_value(27)       => __E_bool_value_53(143).
	p(NT(27), (NT(143)));
//G258: bool_value(112)      => true_value(27).
	p(NT(112), (NT(27)));
//G259: __E_bool_value_54(144) => 'f'.
	p(NT(144), (T(7)));
//G260: __E_bool_value_54(144) => 'f' 'a' 'l' 's' 'e'.
	p(NT(144), (T(7)+T(3)+T(10)+T(5)+T(6)));
//G261: __E_bool_value_54(144) => 'o' 'f' 'f'.
	p(NT(144), (T(20)+T(7)+T(7)));
//G262: __E_bool_value_54(144) => '0'.
	p(NT(144), (T(29)));
//G263: __E_bool_value_54(144) => 'n'.
	p(NT(144), (T(13)));
//G264: __E_bool_value_54(144) => 'n' 'o'.
	p(NT(144), (T(13)+T(20)));
//G265: false_value(28)      => __E_bool_value_54(144).
	p(NT(28), (NT(144)));
//G266: bool_value(112)      => false_value(28).
	p(NT(112), (NT(28)));
//G267: __E_error_verbosity_55(145) => 'b'.
	p(NT(145), (T(19)));
//G268: __E_error_verbosity_55(145) => 'b' 'a' 's' 'i' 'c'.
	p(NT(145), (T(19)+T(3)+T(5)+T(9)+T(17)));
//G269: basic_sym(29)        => __E_error_verbosity_55(145).
	p(NT(29), (NT(145)));
//G270: error_verbosity(117) => basic_sym(29).
	p(NT(117), (NT(29)));
//G271: __E_error_verbosity_56(146) => 'd'.
	p(NT(146), (T(21)));
//G272: __E_error_verbosity_56(146) => 'd' 'e' 't' 'a' 'i' 'l' 'e' 'd'.
	p(NT(146), (T(21)+T(6)+T(14)+T(3)+T(9)+T(10)+T(6)+T(21)));
//G273: detailed_sym(30)     => __E_error_verbosity_56(146).
	p(NT(30), (NT(146)));
//G274: error_verbosity(117) => detailed_sym(30).
	p(NT(117), (NT(30)));
//G275: __E_error_verbosity_57(147) => 'r'.
	p(NT(147), (T(4)));
//G276: __E_error_verbosity_57(147) => 'r' 'c'.
	p(NT(147), (T(4)+T(17)));
//G277: __E_error_verbosity_57(147) => 'r' 'o' 'o' 't' '-' 'c' 'a' 'u' 's' 'e'.
	p(NT(147), (T(4)+T(20)+T(20)+T(14)+T(15)+T(17)+T(3)+T(16)+T(5)+T(6)));
//G278: root_cause_sym(31)   => __E_error_verbosity_57(147).
	p(NT(31), (NT(147)));
//G279: error_verbosity(117) => root_cause_sym(31).
	p(NT(117), (NT(31)));
//G280: __E_symbol_58(148)   => alpha(3).
	p(NT(148), (NT(3)));
//G281: __E_symbol_58(148)   => '_'.
	p(NT(148), (T(30)));
//G282: __E_symbol_59(149)   => alnum(2).
	p(NT(149), (NT(2)));
//G283: __E_symbol_59(149)   => '_'.
	p(NT(149), (T(30)));
//G284: __E_symbol_60(150)   => null.
	p(NT(150), (nul));
//G285: __E_symbol_60(150)   => __E_symbol_59(149) __E_symbol_60(150).
	p(NT(150), (NT(149)+NT(150)));
//G286: symbol(84)           => __E_symbol_58(148) __E_symbol_60(150).
	p(NT(84), (NT(148)+NT(150)));
//G287: __E_symbol_list_61(151) => _(6) ',' _(6) symbol(84).
	p(NT(151), (NT(6)+T(31)+NT(6)+NT(84)));
//G288: __E_symbol_list_62(152) => null.
	p(NT(152), (nul));
//G289: __E_symbol_list_62(152) => __E_symbol_list_61(151) __E_symbol_list_62(152).
	p(NT(152), (NT(151)+NT(152)));
//G290: symbol_list(101)     => symbol(84) __E_symbol_list_62(152).
	p(NT(101), (NT(84)+NT(152)));
//G291: __E_treepath_63(154) => _(6) '>' _(6) symbol(84).
	p(NT(154), (NT(6)+T(32)+NT(6)+NT(84)));
//G292: __E_treepath_64(155) => null.
	p(NT(155), (nul));
//G293: __E_treepath_64(155) => __E_treepath_63(154) __E_treepath_64(155).
	p(NT(155), (NT(154)+NT(155)));
//G294: treepath(153)        => symbol(84) __E_treepath_64(155).
	p(NT(153), (NT(84)+NT(155)));
//G295: __E_treepath_list_65(156) => _(6) ',' _(6) treepath(153).
	p(NT(156), (NT(6)+T(31)+NT(6)+NT(153)));
//G296: __E_treepath_list_66(157) => null.
	p(NT(157), (nul));
//G297: __E_treepath_list_66(157) => __E_treepath_list_65(156) __E_treepath_list_66(157).
	p(NT(157), (NT(156)+NT(157)));
//G298: treepath_list(103)   => treepath(153) __E_treepath_list_66(157).
	p(NT(103), (NT(153)+NT(157)));
//G299: filename(79)         => quoted_string(118).
	p(NT(79), (NT(118)));
//G300: __E_quoted_string_67(159) => null.
	p(NT(159), (nul));
//G301: __E_quoted_string_67(159) => quoted_string_char(158) __E_quoted_string_67(159).
	p(NT(159), (NT(158)+NT(159)));
//G302: quoted_string(118)   => '"' __E_quoted_string_67(159) '"'.
	p(NT(118), (T(33)+NT(159)+T(33)));
//G303: quoted_string_char(158) => unescaped_s(160).
	p(NT(158), (NT(160)));
//G304: quoted_string_char(158) => escaped_s(161).
	p(NT(158), (NT(161)));
//G305: __E_unescaped_s_68(162) => space(4).
	p(NT(162), (NT(4)));
//G306: __E_unescaped_s_68(162) => printable(5).
	p(NT(162), (NT(5)));
//G307: __E_unescaped_s_69(163) => '"'.
	p(NT(163), (T(33)));
//G308: __E_unescaped_s_69(163) => '\\'.
	p(NT(163), (T(34)));
//G309: __N_0(171)           => __E_unescaped_s_69(163).
	p(NT(171), (NT(163)));
//G310: unescaped_s(160)     => __E_unescaped_s_68(162) & ~( __N_0(171) ).	 # conjunctive
	p(NT(160), (NT(162)) & ~(NT(171)));
//G311: __E_escaped_s_70(164) => '"'.
	p(NT(164), (T(33)));
//G312: __E_escaped_s_70(164) => '\\'.
	p(NT(164), (T(34)));
//G313: __E_escaped_s_70(164) => '/'.
	p(NT(164), (T(35)));
//G314: __E_escaped_s_70(164) => 'b'.
	p(NT(164), (T(19)));
//G315: __E_escaped_s_70(164) => 'f'.
	p(NT(164), (T(7)));
//G316: __E_escaped_s_70(164) => 'n'.
	p(NT(164), (T(13)));
//G317: __E_escaped_s_70(164) => 'r'.
	p(NT(164), (T(4)));
//G318: __E_escaped_s_70(164) => 't'.
	p(NT(164), (T(14)));
//G319: escaped_s(161)       => '\\' __E_escaped_s_70(164).
	p(NT(161), (T(34)+NT(164)));
//G320: __E___71(165)        => __(7).
	p(NT(165), (NT(7)));
//G321: __E___71(165)        => null.
	p(NT(165), (nul));
//G322: _(6)                 => __E___71(165).
	p(NT(6), (NT(165)));
//G323: __E____72(166)       => space(4).
	p(NT(166), (NT(4)));
//G324: __E____72(166)       => comment(167).
	p(NT(166), (NT(167)));
//G325: __(7)                => __E____72(166) _(6).
	p(NT(7), (NT(166)+NT(6)));
//G326: __E_comment_73(168)  => printable(5).
	p(NT(168), (NT(5)));
//G327: __E_comment_73(168)  => '\t'.
	p(NT(168), (T(26)));
//G328: __E_comment_74(169)  => null.
	p(NT(169), (nul));
//G329: __E_comment_74(169)  => __E_comment_73(168) __E_comment_74(169).
	p(NT(169), (NT(168)+NT(169)));
//G330: __E_comment_75(170)  => '\r'.
	p(NT(170), (T(36)));
//G331: __E_comment_75(170)  => '\n'.
	p(NT(170), (T(37)));
//G332: __E_comment_75(170)  => eof(1).
	p(NT(170), (NT(1)));
//G333: comment(167)         => '#' __E_comment_74(169) __E_comment_75(170).
	p(NT(167), (T(38)+NT(169)+NT(170)));
	#undef T
	#undef NT
	return loaded = true, p;
}

inline ::idni::grammar<char_type, terminal_type> grammar(
	nts, productions(), start_symbol, char_classes, grammar_options);

} // namespace tgf_repl_parser_data

struct tgf_repl_parser : public idni::parser<char, char> {
	enum nonterminal {
		nul, eof, alnum, alpha, space, printable, _, __, grammar_sym, internal_grammar_sym, 
		unreachable_sym, start_sym, parse_sym, parse_file_sym, load_sym, reload_sym, help_sym, quit_sym, version_sym, clear_sym, 
		get_sym, set_sym, add_sym, del_sym, toggle_sym, enable_sym, disable_sym, true_value, false_value, basic_sym, 
		detailed_sym, root_cause_sym, error_verbosity_opt, status_opt, colors_opt, print_ambiguity_opt, print_graphs_opt, print_rules_opt, print_facts_opt, print_terminals_opt, 
		measure_parsing_opt, measure_each_pos_opt, measure_forest_opt, measure_preprocess_opt, debug_opt, auto_disambiguate_opt, trim_terminals_opt, inline_cc_opt, nodisambig_list_opt, trim_opt, 
		trim_children_opt, inline_opt, start, __E_start_0, statement, __E___E_start_0_1, __E___E_start_0_2, grammar_cmd, igrammar_cmd, unreachable_cmd, 
		reload_cmd, load_cmd, start_cmd, help, version, quit, clear, get, set, toggle, 
		enable, disable, add, del, parse_file_cmd, parse_cmd, __E_parse_cmd_3, parse_input, __E_parse_file_cmd_4, filename, 
		__E_grammar_cmd_5, igrammar_sym, __E_igrammar_cmd_6, __E_igrammar_cmd_7, symbol, __E_start_cmd_8, __E_start_cmd_9, __E_unreachable_cmd_10, __E_unreachable_cmd_11, __E_reload_cmd_12, 
		__E_load_cmd_13, __E_help_14, __E_help_15, help_arg, __E_version_16, __E_quit_17, __E_clear_18, __E_get_19, option, __E_add_20, 
		list_option, symbol_list, treepaths_option, treepath_list, __E_del_21, __E_del_22, __E_toggle_23, bool_option, __E_enable_24, __E_disable_25, 
		__E_set_26, __E___E_set_26_27, bool_value, __E___E_set_26_28, __E___E_set_26_29, enum_ev_option, __E___E_set_26_30, error_verbosity, quoted_string, parse_input_char_seq, 
		__E_parse_input_31, __E_parse_input_32, __E_enum_ev_option_33, __E_bool_option_34, __E_bool_option_35, __E_bool_option_36, __E_bool_option_37, __E_bool_option_38, __E_bool_option_39, __E_bool_option_40, 
		__E_bool_option_41, __E_bool_option_42, __E_bool_option_43, __E_bool_option_44, __E_bool_option_45, __E_bool_option_46, __E_bool_option_47, __E_bool_option_48, __E_list_option_49, __E_list_option_50, 
		trim_children_terminals_opt, __E_list_option_51, __E_treepaths_option_52, __E_bool_value_53, __E_bool_value_54, __E_error_verbosity_55, __E_error_verbosity_56, __E_error_verbosity_57, __E_symbol_58, __E_symbol_59, 
		__E_symbol_60, __E_symbol_list_61, __E_symbol_list_62, treepath, __E_treepath_63, __E_treepath_64, __E_treepath_list_65, __E_treepath_list_66, quoted_string_char, __E_quoted_string_67, 
		unescaped_s, escaped_s, __E_unescaped_s_68, __E_unescaped_s_69, __E_escaped_s_70, __E___71, __E____72, comment, __E_comment_73, __E_comment_74, 
		__E_comment_75, __N_0, 
	};
	static tgf_repl_parser& instance() {
		static tgf_repl_parser inst;
		return inst;
	}
	tgf_repl_parser() : idni::parser<char_type, terminal_type>(
		tgf_repl_parser_data::grammar,
		tgf_repl_parser_data::parser_options) {}
	size_t id(const std::basic_string<char_type>& name) {
		return tgf_repl_parser_data::nts.get(name);
	}
	const std::basic_string<char_type>& name(size_t id) {
		return tgf_repl_parser_data::nts.get(id);
	}
	symbol_type literal(const nonterminal& nt) {
		return symbol_type(nt, &tgf_repl_parser_data::nts);
	}
};

#endif // __TGF_REPL_PARSER_H__
