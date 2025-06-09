// This file is generated from a file src/tgf/tgf_repl.tgf by
//       https://github.com/IDNI/parser/src/tgf
//
#ifndef __TGF_REPL_PARSER_H__
#define __TGF_REPL_PARSER_H__

#include "parser.h"

namespace tgf_repl_parser_data {

using char_type     = char;
using terminal_type = char;

inline std::vector<std::string> symbol_names{
	"", "eof", "alnum", "alpha", "space", "printable", "_", "__", "symbol", "quoted_string", 
	"unescaped_s", "escaped_s", "parse_input_char_seq", "start", "__E_start_0", "statement", "__E___E_start_0_1", "__E___E_start_0_2", "grammar_cmd", "igrammar_cmd", 
	"unreachable_cmd", "reload_cmd", "load_cmd", "start_cmd", "help", "version", "quit", "license", "clear", "get", 
	"set", "toggle", "enable", "disable", "add", "del", "parse_file_cmd", "parse_cmd", "parse_sym", "__E_parse_cmd_3", 
	"parse_input", "parse_file_sym", "__E_parse_file_cmd_4", "filename", "grammar_sym", "__E_grammar_cmd_5", "igrammar_sym", "__E_igrammar_cmd_6", "__E_igrammar_cmd_7", "start_sym", 
	"__E_start_cmd_8", "__E_start_cmd_9", "unreachable_sym", "__E_unreachable_cmd_10", "__E_unreachable_cmd_11", "reload_sym", "__E_reload_cmd_12", "load_sym", "__E_load_cmd_13", "help_sym", 
	"__E_help_14", "__E_help_15", "help_arg", "version_sym", "__E_version_16", "license_sym", "__E_license_17", "quit_sym", "__E_quit_18", "clear_sym", 
	"__E_clear_19", "get_sym", "__E_get_20", "option", "add_sym", "__E_add_21", "list_option", "symbol_list", "treepaths_option", "treepath_list", 
	"del_sym", "__E_del_22", "__E_del_23", "toggle_sym", "__E_toggle_24", "bool_option", "enable_sym", "__E_enable_25", "disable_sym", "__E_disable_26", 
	"set_sym", "__E_set_27", "__E___E_set_27_28", "bool_value", "__E___E_set_27_29", "__E___E_set_27_30", "enum_ev_option", "__E___E_set_27_31", "error_verbosity", "__E_parse_input_32", 
	"__E_parse_input_33", "error_verbosity_opt", "__E_enum_ev_option_34", "status_opt", "__E_bool_option_35", "colors_opt", "__E_bool_option_36", "print_ambiguity_opt", "__E_bool_option_37", "print_graphs_opt", 
	"__E_bool_option_38", "print_rules_opt", "__E_bool_option_39", "print_facts_opt", "__E_bool_option_40", "print_terminals_opt", "__E_bool_option_41", "measure_parsing_opt", "__E_bool_option_42", "measure_each_pos_opt", 
	"__E_bool_option_43", "measure_forest_opt", "__E_bool_option_44", "measure_preprocess_opt", "__E_bool_option_45", "debug_opt", "__E_bool_option_46", "auto_disambiguate_opt", "__E_bool_option_47", "trim_terminals_opt", 
	"__E_bool_option_48", "inline_cc_opt", "__E_bool_option_49", "nodisambig_list_opt", "__E_list_option_50", "enabled_prods_opt", "__E_list_option_51", "trim_opt", "trim_children_opt", "__E_list_option_52", 
	"trim_children_terminals_opt", "__E_list_option_53", "inline_opt", "__E_treepaths_option_54", "true_value", "__E_bool_value_55", "false_value", "__E_bool_value_56", "basic_sym", "__E_error_verbosity_57", 
	"detailed_sym", "__E_error_verbosity_58", "root_cause_sym", "__E_error_verbosity_59", "__E_symbol_60", "__E_symbol_61", "__E_symbol_62", "__E_symbol_list_63", "__E_symbol_list_64", "treepath", 
	"__E_treepath_65", "__E_treepath_66", "__E_treepath_list_67", "__E_treepath_list_68", "quoted_string_char", "__E_quoted_string_69", "__E_unescaped_s_70", "__E_unescaped_s_71", "__E_escaped_s_72", "__E___73", 
	"__E____74", "comment", "__E_comment_75", "__E_comment_76", "__E_comment_77", "__N_0", 
};

inline ::idni::nonterminals<char_type, terminal_type> nts{symbol_names};

inline std::vector<terminal_type> terminals{
	'\0', '.', 'p', 'a', 'r', 's', 'e', 'f', ' ', 
	'i', 'l', 'g', 'm', 'n', 't', '-', 'u', 'c', 'h', 
	'b', 'o', 'd', 'v', 'L', 'q', 'x', '=', '\t', 'y', 
	'1', '0', '_', ',', '>', '"', '\\', '/', '\r', '\n', 
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
			6, 7
		},
		.trim_terminals = true,
		.dont_trim_terminals_of = {
			8, 9, 10, 11, 12
		},
		.inline_char_classes = true
	}
};

inline ::idni::parser<char_type, terminal_type>::options parser_options{
};

inline ::idni::prods<char_type, terminal_type> start_symbol{ nts(13) };

inline idni::prods<char_type, terminal_type>& productions() {
	static bool loaded = false;
	static idni::prods<char_type, terminal_type>
		p, nul(idni::lit<char_type, terminal_type>{});
	if (loaded) return p;
	#define  T(x) (idni::prods<char_type, terminal_type>{ terminals[x] })
	#define NT(x) (idni::prods<char_type, terminal_type>{ nts(x) })
//G0:   __E___E_start_0_1(16) => _(6) '.' _(6) statement(15).
	p(NT(16), (NT(6)+T(1)+NT(6)+NT(15)));
//G1:   __E___E_start_0_2(17) => null.
	p(NT(17), (nul));
//G2:   __E___E_start_0_2(17) => __E___E_start_0_1(16) __E___E_start_0_2(17).
	p(NT(17), (NT(16)+NT(17)));
//G3:   __E_start_0(14)      => statement(15) __E___E_start_0_2(17) _(6).
	p(NT(14), (NT(15)+NT(17)+NT(6)));
//G4:   __E_start_0(14)      => null.
	p(NT(14), (nul));
//G5:   start(13)            => _(6) __E_start_0(14).
	p(NT(13), (NT(6)+NT(14)));
//G6:   statement(15)        => grammar_cmd(18).
	p(NT(15), (NT(18)));
//G7:   statement(15)        => igrammar_cmd(19).
	p(NT(15), (NT(19)));
//G8:   statement(15)        => unreachable_cmd(20).
	p(NT(15), (NT(20)));
//G9:   statement(15)        => reload_cmd(21).
	p(NT(15), (NT(21)));
//G10:  statement(15)        => load_cmd(22).
	p(NT(15), (NT(22)));
//G11:  statement(15)        => start_cmd(23).
	p(NT(15), (NT(23)));
//G12:  statement(15)        => help(24).
	p(NT(15), (NT(24)));
//G13:  statement(15)        => version(25).
	p(NT(15), (NT(25)));
//G14:  statement(15)        => quit(26).
	p(NT(15), (NT(26)));
//G15:  statement(15)        => license(27).
	p(NT(15), (NT(27)));
//G16:  statement(15)        => clear(28).
	p(NT(15), (NT(28)));
//G17:  statement(15)        => get(29).
	p(NT(15), (NT(29)));
//G18:  statement(15)        => set(30).
	p(NT(15), (NT(30)));
//G19:  statement(15)        => toggle(31).
	p(NT(15), (NT(31)));
//G20:  statement(15)        => enable(32).
	p(NT(15), (NT(32)));
//G21:  statement(15)        => disable(33).
	p(NT(15), (NT(33)));
//G22:  statement(15)        => add(34).
	p(NT(15), (NT(34)));
//G23:  statement(15)        => del(35).
	p(NT(15), (NT(35)));
//G24:  statement(15)        => parse_file_cmd(36).
	p(NT(15), (NT(36)));
//G25:  statement(15)        => parse_cmd(37).
	p(NT(15), (NT(37)));
//G26:  __E_parse_cmd_3(39)  => 'p'.
	p(NT(39), (T(2)));
//G27:  __E_parse_cmd_3(39)  => 'p' 'a' 'r' 's' 'e'.
	p(NT(39), (T(2)+T(3)+T(4)+T(5)+T(6)));
//G28:  parse_sym(38)        => __E_parse_cmd_3(39).
	p(NT(38), (NT(39)));
//G29:  parse_cmd(37)        => parse_sym(38) __(7) parse_input(40).
	p(NT(37), (NT(38)+NT(7)+NT(40)));
//G30:  __E_parse_file_cmd_4(42) => 'f'.
	p(NT(42), (T(7)));
//G31:  __E_parse_file_cmd_4(42) => 'p' 'f'.
	p(NT(42), (T(2)+T(7)));
//G32:  __E_parse_file_cmd_4(42) => 'p' 'a' 'r' 's' 'e' ' ' 'f' 'i' 'l' 'e'.
	p(NT(42), (T(2)+T(3)+T(4)+T(5)+T(6)+T(8)+T(7)+T(9)+T(10)+T(6)));
//G33:  parse_file_sym(41)   => __E_parse_file_cmd_4(42).
	p(NT(41), (NT(42)));
//G34:  parse_file_cmd(36)   => parse_file_sym(41) __(7) filename(43).
	p(NT(36), (NT(41)+NT(7)+NT(43)));
//G35:  __E_grammar_cmd_5(45) => 'g'.
	p(NT(45), (T(11)));
//G36:  __E_grammar_cmd_5(45) => 'g' 'r' 'a' 'm' 'm' 'a' 'r'.
	p(NT(45), (T(11)+T(4)+T(3)+T(12)+T(12)+T(3)+T(4)));
//G37:  grammar_sym(44)      => __E_grammar_cmd_5(45).
	p(NT(44), (NT(45)));
//G38:  grammar_cmd(18)      => grammar_sym(44).
	p(NT(18), (NT(44)));
//G39:  __E_igrammar_cmd_6(47) => 'i'.
	p(NT(47), (T(9)));
//G40:  __E_igrammar_cmd_6(47) => 'i' 'g'.
	p(NT(47), (T(9)+T(11)));
//G41:  __E_igrammar_cmd_6(47) => 'i' 'n' 't' 'e' 'r' 'n' 'a' 'l' '-' 'g' 'r' 'a' 'm' 'm' 'a' 'r'.
	p(NT(47), (T(9)+T(13)+T(14)+T(6)+T(4)+T(13)+T(3)+T(10)+T(15)+T(11)+T(4)+T(3)+T(12)+T(12)+T(3)+T(4)));
//G42:  igrammar_sym(46)     => __E_igrammar_cmd_6(47).
	p(NT(46), (NT(47)));
//G43:  __E_igrammar_cmd_7(48) => __(7) symbol(8).
	p(NT(48), (NT(7)+NT(8)));
//G44:  __E_igrammar_cmd_7(48) => null.
	p(NT(48), (nul));
//G45:  igrammar_cmd(19)     => igrammar_sym(46) __E_igrammar_cmd_7(48).
	p(NT(19), (NT(46)+NT(48)));
//G46:  __E_start_cmd_8(50)  => 's'.
	p(NT(50), (T(5)));
//G47:  __E_start_cmd_8(50)  => 's' 't' 'a' 'r' 't'.
	p(NT(50), (T(5)+T(14)+T(3)+T(4)+T(14)));
//G48:  start_sym(49)        => __E_start_cmd_8(50).
	p(NT(49), (NT(50)));
//G49:  __E_start_cmd_9(51)  => __(7) symbol(8).
	p(NT(51), (NT(7)+NT(8)));
//G50:  __E_start_cmd_9(51)  => null.
	p(NT(51), (nul));
//G51:  start_cmd(23)        => start_sym(49) __E_start_cmd_9(51).
	p(NT(23), (NT(49)+NT(51)));
//G52:  __E_unreachable_cmd_10(53) => 'u'.
	p(NT(53), (T(16)));
//G53:  __E_unreachable_cmd_10(53) => 'u' 'n' 'r' 'e' 'a' 'c' 'h' 'a' 'b' 'l' 'e'.
	p(NT(53), (T(16)+T(13)+T(4)+T(6)+T(3)+T(17)+T(18)+T(3)+T(19)+T(10)+T(6)));
//G54:  unreachable_sym(52)  => __E_unreachable_cmd_10(53).
	p(NT(52), (NT(53)));
//G55:  __E_unreachable_cmd_11(54) => __(7) symbol(8).
	p(NT(54), (NT(7)+NT(8)));
//G56:  __E_unreachable_cmd_11(54) => null.
	p(NT(54), (nul));
//G57:  unreachable_cmd(20)  => unreachable_sym(52) __E_unreachable_cmd_11(54).
	p(NT(20), (NT(52)+NT(54)));
//G58:  __E_reload_cmd_12(56) => 'r'.
	p(NT(56), (T(4)));
//G59:  __E_reload_cmd_12(56) => 'r' 'e' 'l' 'o' 'a' 'd'.
	p(NT(56), (T(4)+T(6)+T(10)+T(20)+T(3)+T(21)));
//G60:  reload_sym(55)       => __E_reload_cmd_12(56).
	p(NT(55), (NT(56)));
//G61:  reload_cmd(21)       => reload_sym(55).
	p(NT(21), (NT(55)));
//G62:  __E_load_cmd_13(58)  => 'l'.
	p(NT(58), (T(10)));
//G63:  __E_load_cmd_13(58)  => 'l' 'o' 'a' 'd'.
	p(NT(58), (T(10)+T(20)+T(3)+T(21)));
//G64:  load_sym(57)         => __E_load_cmd_13(58).
	p(NT(57), (NT(58)));
//G65:  load_cmd(22)         => load_sym(57) __(7) filename(43).
	p(NT(22), (NT(57)+NT(7)+NT(43)));
//G66:  __E_help_14(60)      => 'h'.
	p(NT(60), (T(18)));
//G67:  __E_help_14(60)      => 'h' 'e' 'l' 'p'.
	p(NT(60), (T(18)+T(6)+T(10)+T(2)));
//G68:  help_sym(59)         => __E_help_14(60).
	p(NT(59), (NT(60)));
//G69:  __E_help_15(61)      => __(7) help_arg(62).
	p(NT(61), (NT(7)+NT(62)));
//G70:  __E_help_15(61)      => null.
	p(NT(61), (nul));
//G71:  help(24)             => help_sym(59) __E_help_15(61).
	p(NT(24), (NT(59)+NT(61)));
//G72:  __E_version_16(64)   => 'v'.
	p(NT(64), (T(22)));
//G73:  __E_version_16(64)   => 'v' 'e' 'r' 's' 'i' 'o' 'n'.
	p(NT(64), (T(22)+T(6)+T(4)+T(5)+T(9)+T(20)+T(13)));
//G74:  version_sym(63)      => __E_version_16(64).
	p(NT(63), (NT(64)));
//G75:  version(25)          => version_sym(63).
	p(NT(25), (NT(63)));
//G76:  __E_license_17(66)   => 'L'.
	p(NT(66), (T(23)));
//G77:  __E_license_17(66)   => 'l' 'i' 'c' 'e' 'n' 's' 'e'.
	p(NT(66), (T(10)+T(9)+T(17)+T(6)+T(13)+T(5)+T(6)));
//G78:  license_sym(65)      => __E_license_17(66).
	p(NT(65), (NT(66)));
//G79:  license(27)          => license_sym(65).
	p(NT(27), (NT(65)));
//G80:  __E_quit_18(68)      => 'q'.
	p(NT(68), (T(24)));
//G81:  __E_quit_18(68)      => 'q' 'u' 'i' 't'.
	p(NT(68), (T(24)+T(16)+T(9)+T(14)));
//G82:  __E_quit_18(68)      => 'e'.
	p(NT(68), (T(6)));
//G83:  __E_quit_18(68)      => 'e' 'x' 'i' 't'.
	p(NT(68), (T(6)+T(25)+T(9)+T(14)));
//G84:  quit_sym(67)         => __E_quit_18(68).
	p(NT(67), (NT(68)));
//G85:  quit(26)             => quit_sym(67).
	p(NT(26), (NT(67)));
//G86:  __E_clear_19(70)     => 'c' 'l' 's'.
	p(NT(70), (T(17)+T(10)+T(5)));
//G87:  __E_clear_19(70)     => 'c' 'l' 'e' 'a' 'r'.
	p(NT(70), (T(17)+T(10)+T(6)+T(3)+T(4)));
//G88:  clear_sym(69)        => __E_clear_19(70).
	p(NT(69), (NT(70)));
//G89:  clear(28)            => clear_sym(69).
	p(NT(28), (NT(69)));
//G90:  get_sym(71)          => 'g' 'e' 't'.
	p(NT(71), (T(11)+T(6)+T(14)));
//G91:  __E_get_20(72)       => __(7) option(73).
	p(NT(72), (NT(7)+NT(73)));
//G92:  __E_get_20(72)       => null.
	p(NT(72), (nul));
//G93:  get(29)              => get_sym(71) __E_get_20(72).
	p(NT(29), (NT(71)+NT(72)));
//G94:  add_sym(74)          => 'a' 'd' 'd'.
	p(NT(74), (T(3)+T(21)+T(21)));
//G95:  __E_add_21(75)       => list_option(76) __(7) symbol_list(77).
	p(NT(75), (NT(76)+NT(7)+NT(77)));
//G96:  __E_add_21(75)       => treepaths_option(78) __(7) treepath_list(79).
	p(NT(75), (NT(78)+NT(7)+NT(79)));
//G97:  add(34)              => add_sym(74) __(7) __E_add_21(75).
	p(NT(34), (NT(74)+NT(7)+NT(75)));
//G98:  __E_del_22(81)       => 'd' 'e' 'l'.
	p(NT(81), (T(21)+T(6)+T(10)));
//G99:  __E_del_22(81)       => 'd' 'e' 'l' 'e' 't' 'e'.
	p(NT(81), (T(21)+T(6)+T(10)+T(6)+T(14)+T(6)));
//G100: __E_del_22(81)       => 'r' 'm'.
	p(NT(81), (T(4)+T(12)));
//G101: __E_del_22(81)       => 'r' 'e' 'm'.
	p(NT(81), (T(4)+T(6)+T(12)));
//G102: __E_del_22(81)       => 'r' 'e' 'm' 'o' 'v' 'e'.
	p(NT(81), (T(4)+T(6)+T(12)+T(20)+T(22)+T(6)));
//G103: del_sym(80)          => __E_del_22(81).
	p(NT(80), (NT(81)));
//G104: __E_del_23(82)       => list_option(76) __(7) symbol_list(77).
	p(NT(82), (NT(76)+NT(7)+NT(77)));
//G105: __E_del_23(82)       => treepaths_option(78) __(7) treepath_list(79).
	p(NT(82), (NT(78)+NT(7)+NT(79)));
//G106: del(35)              => del_sym(80) __(7) __E_del_23(82).
	p(NT(35), (NT(80)+NT(7)+NT(82)));
//G107: __E_toggle_24(84)    => 't' 'o' 'g'.
	p(NT(84), (T(14)+T(20)+T(11)));
//G108: __E_toggle_24(84)    => 't' 'o' 'g' 'g' 'l' 'e'.
	p(NT(84), (T(14)+T(20)+T(11)+T(11)+T(10)+T(6)));
//G109: toggle_sym(83)       => __E_toggle_24(84).
	p(NT(83), (NT(84)));
//G110: toggle(31)           => toggle_sym(83) __(7) bool_option(85).
	p(NT(31), (NT(83)+NT(7)+NT(85)));
//G111: __E_enable_25(87)    => 'e' 'n' __(7).
	p(NT(87), (T(6)+T(13)+NT(7)));
//G112: __E_enable_25(87)    => 'e' 'n' 'a' 'b' 'l' 'e' __(7).
	p(NT(87), (T(6)+T(13)+T(3)+T(19)+T(10)+T(6)+NT(7)));
//G113: enable_sym(86)       => __E_enable_25(87).
	p(NT(86), (NT(87)));
//G114: enable(32)           => enable_sym(86) bool_option(85).
	p(NT(32), (NT(86)+NT(85)));
//G115: __E_disable_26(89)   => 'd' 'i' 's' __(7).
	p(NT(89), (T(21)+T(9)+T(5)+NT(7)));
//G116: __E_disable_26(89)   => 'd' 'i' 's' 'a' 'b' 'l' 'e' __(7).
	p(NT(89), (T(21)+T(9)+T(5)+T(3)+T(19)+T(10)+T(6)+NT(7)));
//G117: disable_sym(88)      => __E_disable_26(89).
	p(NT(88), (NT(89)));
//G118: disable(33)          => disable_sym(88) bool_option(85).
	p(NT(33), (NT(88)+NT(85)));
//G119: set_sym(90)          => 's' 'e' 't'.
	p(NT(90), (T(5)+T(6)+T(14)));
//G120: __E___E_set_27_28(92) => __(7).
	p(NT(92), (NT(7)));
//G121: __E___E_set_27_28(92) => _(6) '=' _(6).
	p(NT(92), (NT(6)+T(26)+NT(6)));
//G122: __E_set_27(91)       => bool_option(85) __E___E_set_27_28(92) bool_value(93).
	p(NT(91), (NT(85)+NT(92)+NT(93)));
//G123: __E___E_set_27_29(94) => __(7).
	p(NT(94), (NT(7)));
//G124: __E___E_set_27_29(94) => _(6) '=' _(6).
	p(NT(94), (NT(6)+T(26)+NT(6)));
//G125: __E_set_27(91)       => list_option(76) __E___E_set_27_29(94) symbol_list(77).
	p(NT(91), (NT(76)+NT(94)+NT(77)));
//G126: __E___E_set_27_30(95) => __(7).
	p(NT(95), (NT(7)));
//G127: __E___E_set_27_30(95) => _(6) '=' _(6).
	p(NT(95), (NT(6)+T(26)+NT(6)));
//G128: __E_set_27(91)       => treepaths_option(78) __E___E_set_27_30(95) treepath_list(79).
	p(NT(91), (NT(78)+NT(95)+NT(79)));
//G129: __E___E_set_27_31(97) => __(7).
	p(NT(97), (NT(7)));
//G130: __E___E_set_27_31(97) => _(6) '=' _(6).
	p(NT(97), (NT(6)+T(26)+NT(6)));
//G131: __E_set_27(91)       => enum_ev_option(96) __E___E_set_27_31(97) error_verbosity(98).
	p(NT(91), (NT(96)+NT(97)+NT(98)));
//G132: set(30)              => set_sym(90) __(7) __E_set_27(91).
	p(NT(30), (NT(90)+NT(7)+NT(91)));
//G133: parse_input(40)      => quoted_string(9).
	p(NT(40), (NT(9)));
//G134: __E_parse_input_32(99) => printable(5).
	p(NT(99), (NT(5)));
//G135: __E_parse_input_32(99) => '\t'.
	p(NT(99), (T(27)));
//G136: __E_parse_input_33(100) => __E_parse_input_32(99).
	p(NT(100), (NT(99)));
//G137: __E_parse_input_33(100) => __E_parse_input_32(99) __E_parse_input_33(100).
	p(NT(100), (NT(99)+NT(100)));
//G138: parse_input_char_seq(12) => __E_parse_input_33(100).
	p(NT(12), (NT(100)));
//G139: parse_input(40)      => parse_input_char_seq(12).
	p(NT(40), (NT(12)));
//G140: help_arg(62)         => grammar_sym(44).
	p(NT(62), (NT(44)));
//G141: help_arg(62)         => igrammar_sym(46).
	p(NT(62), (NT(46)));
//G142: help_arg(62)         => unreachable_sym(52).
	p(NT(62), (NT(52)));
//G143: help_arg(62)         => start_sym(49).
	p(NT(62), (NT(49)));
//G144: help_arg(62)         => parse_sym(38).
	p(NT(62), (NT(38)));
//G145: help_arg(62)         => parse_file_sym(41).
	p(NT(62), (NT(41)));
//G146: help_arg(62)         => load_sym(57).
	p(NT(62), (NT(57)));
//G147: help_arg(62)         => reload_sym(55).
	p(NT(62), (NT(55)));
//G148: help_arg(62)         => clear_sym(69).
	p(NT(62), (NT(69)));
//G149: help_arg(62)         => help_sym(59).
	p(NT(62), (NT(59)));
//G150: help_arg(62)         => quit_sym(67).
	p(NT(62), (NT(67)));
//G151: help_arg(62)         => version_sym(63).
	p(NT(62), (NT(63)));
//G152: help_arg(62)         => license_sym(65).
	p(NT(62), (NT(65)));
//G153: help_arg(62)         => get_sym(71).
	p(NT(62), (NT(71)));
//G154: help_arg(62)         => set_sym(90).
	p(NT(62), (NT(90)));
//G155: help_arg(62)         => add_sym(74).
	p(NT(62), (NT(74)));
//G156: help_arg(62)         => del_sym(80).
	p(NT(62), (NT(80)));
//G157: help_arg(62)         => toggle_sym(83).
	p(NT(62), (NT(83)));
//G158: help_arg(62)         => enable_sym(86).
	p(NT(62), (NT(86)));
//G159: help_arg(62)         => disable_sym(88).
	p(NT(62), (NT(88)));
//G160: option(73)           => bool_option(85).
	p(NT(73), (NT(85)));
//G161: option(73)           => enum_ev_option(96).
	p(NT(73), (NT(96)));
//G162: option(73)           => list_option(76).
	p(NT(73), (NT(76)));
//G163: option(73)           => treepaths_option(78).
	p(NT(73), (NT(78)));
//G164: __E_enum_ev_option_34(102) => 'e'.
	p(NT(102), (T(6)));
//G165: __E_enum_ev_option_34(102) => 'e' 'r' 'r' 'o' 'r' '-' 'v' 'e' 'r' 'b' 'o' 's' 'i' 't' 'y'.
	p(NT(102), (T(6)+T(4)+T(4)+T(20)+T(4)+T(15)+T(22)+T(6)+T(4)+T(19)+T(20)+T(5)+T(9)+T(14)+T(28)));
//G166: error_verbosity_opt(101) => __E_enum_ev_option_34(102).
	p(NT(101), (NT(102)));
//G167: enum_ev_option(96)   => error_verbosity_opt(101).
	p(NT(96), (NT(101)));
//G168: __E_bool_option_35(104) => 's'.
	p(NT(104), (T(5)));
//G169: __E_bool_option_35(104) => 's' 't' 'a' 't' 'u' 's'.
	p(NT(104), (T(5)+T(14)+T(3)+T(14)+T(16)+T(5)));
//G170: status_opt(103)      => __E_bool_option_35(104).
	p(NT(103), (NT(104)));
//G171: bool_option(85)      => status_opt(103).
	p(NT(85), (NT(103)));
//G172: __E_bool_option_36(106) => 'c'.
	p(NT(106), (T(17)));
//G173: __E_bool_option_36(106) => 'c' 'o' 'l' 'o' 'r'.
	p(NT(106), (T(17)+T(20)+T(10)+T(20)+T(4)));
//G174: __E_bool_option_36(106) => 'c' 'o' 'l' 'o' 'r' 's'.
	p(NT(106), (T(17)+T(20)+T(10)+T(20)+T(4)+T(5)));
//G175: colors_opt(105)      => __E_bool_option_36(106).
	p(NT(105), (NT(106)));
//G176: bool_option(85)      => colors_opt(105).
	p(NT(85), (NT(105)));
//G177: __E_bool_option_37(108) => 'a'.
	p(NT(108), (T(3)));
//G178: __E_bool_option_37(108) => 'a' 'm' 'b' 'i' 'g' 'u' 'i' 't' 'y'.
	p(NT(108), (T(3)+T(12)+T(19)+T(9)+T(11)+T(16)+T(9)+T(14)+T(28)));
//G179: __E_bool_option_37(108) => 'p' 'r' 'i' 'n' 't' '-' 'a' 'm' 'b' 'i' 'g' 'u' 'i' 't' 'y'.
	p(NT(108), (T(2)+T(4)+T(9)+T(13)+T(14)+T(15)+T(3)+T(12)+T(19)+T(9)+T(11)+T(16)+T(9)+T(14)+T(28)));
//G180: print_ambiguity_opt(107) => __E_bool_option_37(108).
	p(NT(107), (NT(108)));
//G181: bool_option(85)      => print_ambiguity_opt(107).
	p(NT(85), (NT(107)));
//G182: __E_bool_option_38(110) => 'g'.
	p(NT(110), (T(11)));
//G183: __E_bool_option_38(110) => 'g' 'r' 'a' 'p' 'h' 's'.
	p(NT(110), (T(11)+T(4)+T(3)+T(2)+T(18)+T(5)));
//G184: __E_bool_option_38(110) => 'p' 'r' 'i' 'n' 't' '-' 'g' 'r' 'a' 'p' 'h' 's'.
	p(NT(110), (T(2)+T(4)+T(9)+T(13)+T(14)+T(15)+T(11)+T(4)+T(3)+T(2)+T(18)+T(5)));
//G185: print_graphs_opt(109) => __E_bool_option_38(110).
	p(NT(109), (NT(110)));
//G186: bool_option(85)      => print_graphs_opt(109).
	p(NT(85), (NT(109)));
//G187: __E_bool_option_39(112) => 'r'.
	p(NT(112), (T(4)));
//G188: __E_bool_option_39(112) => 'r' 'u' 'l' 'e' 's'.
	p(NT(112), (T(4)+T(16)+T(10)+T(6)+T(5)));
//G189: __E_bool_option_39(112) => 'p' 'r' 'i' 'n' 't' '-' 'r' 'u' 'l' 'e' 's'.
	p(NT(112), (T(2)+T(4)+T(9)+T(13)+T(14)+T(15)+T(4)+T(16)+T(10)+T(6)+T(5)));
//G190: print_rules_opt(111) => __E_bool_option_39(112).
	p(NT(111), (NT(112)));
//G191: bool_option(85)      => print_rules_opt(111).
	p(NT(85), (NT(111)));
//G192: __E_bool_option_40(114) => 'f'.
	p(NT(114), (T(7)));
//G193: __E_bool_option_40(114) => 'f' 'a' 'c' 't' 's'.
	p(NT(114), (T(7)+T(3)+T(17)+T(14)+T(5)));
//G194: __E_bool_option_40(114) => 'p' 'r' 'i' 'n' 't' '-' 'f' 'a' 'c' 't' 's'.
	p(NT(114), (T(2)+T(4)+T(9)+T(13)+T(14)+T(15)+T(7)+T(3)+T(17)+T(14)+T(5)));
//G195: print_facts_opt(113) => __E_bool_option_40(114).
	p(NT(113), (NT(114)));
//G196: bool_option(85)      => print_facts_opt(113).
	p(NT(85), (NT(113)));
//G197: __E_bool_option_41(116) => 't'.
	p(NT(116), (T(14)));
//G198: __E_bool_option_41(116) => 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
	p(NT(116), (T(14)+T(6)+T(4)+T(12)+T(9)+T(13)+T(3)+T(10)+T(5)));
//G199: __E_bool_option_41(116) => 'p' 'r' 'i' 'n' 't' '-' 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
	p(NT(116), (T(2)+T(4)+T(9)+T(13)+T(14)+T(15)+T(14)+T(6)+T(4)+T(12)+T(9)+T(13)+T(3)+T(10)+T(5)));
//G200: print_terminals_opt(115) => __E_bool_option_41(116).
	p(NT(115), (NT(116)));
//G201: bool_option(85)      => print_terminals_opt(115).
	p(NT(85), (NT(115)));
//G202: __E_bool_option_42(118) => 'm'.
	p(NT(118), (T(12)));
//G203: __E_bool_option_42(118) => 'm' 'e' 'a' 's' 'u' 'r' 'e'.
	p(NT(118), (T(12)+T(6)+T(3)+T(5)+T(16)+T(4)+T(6)));
//G204: __E_bool_option_42(118) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'p' 'a' 'r' 's' 'i' 'n' 'g'.
	p(NT(118), (T(12)+T(6)+T(3)+T(5)+T(16)+T(4)+T(6)+T(15)+T(2)+T(3)+T(4)+T(5)+T(9)+T(13)+T(11)));
//G205: measure_parsing_opt(117) => __E_bool_option_42(118).
	p(NT(117), (NT(118)));
//G206: bool_option(85)      => measure_parsing_opt(117).
	p(NT(85), (NT(117)));
//G207: __E_bool_option_43(120) => 'm' 'e'.
	p(NT(120), (T(12)+T(6)));
//G208: __E_bool_option_43(120) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'e' 'a' 'c' 'h'.
	p(NT(120), (T(12)+T(6)+T(3)+T(5)+T(16)+T(4)+T(6)+T(15)+T(6)+T(3)+T(17)+T(18)));
//G209: __E_bool_option_43(120) => 'm' 'e' 'p'.
	p(NT(120), (T(12)+T(6)+T(2)));
//G210: __E_bool_option_43(120) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'e' 'a' 'c' 'h' '-' 'p' 'o' 's'.
	p(NT(120), (T(12)+T(6)+T(3)+T(5)+T(16)+T(4)+T(6)+T(15)+T(6)+T(3)+T(17)+T(18)+T(15)+T(2)+T(20)+T(5)));
//G211: measure_each_pos_opt(119) => __E_bool_option_43(120).
	p(NT(119), (NT(120)));
//G212: bool_option(85)      => measure_each_pos_opt(119).
	p(NT(85), (NT(119)));
//G213: __E_bool_option_44(122) => 'm' 'f'.
	p(NT(122), (T(12)+T(7)));
//G214: __E_bool_option_44(122) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'f' 'o' 'r' 'e' 's' 't'.
	p(NT(122), (T(12)+T(6)+T(3)+T(5)+T(16)+T(4)+T(6)+T(15)+T(7)+T(20)+T(4)+T(6)+T(5)+T(14)));
//G215: measure_forest_opt(121) => __E_bool_option_44(122).
	p(NT(121), (NT(122)));
//G216: bool_option(85)      => measure_forest_opt(121).
	p(NT(85), (NT(121)));
//G217: __E_bool_option_45(124) => 'm' 'p'.
	p(NT(124), (T(12)+T(2)));
//G218: __E_bool_option_45(124) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'p' 'r' 'e' 'p' 'r' 'o' 'c' 'e' 's' 's'.
	p(NT(124), (T(12)+T(6)+T(3)+T(5)+T(16)+T(4)+T(6)+T(15)+T(2)+T(4)+T(6)+T(2)+T(4)+T(20)+T(17)+T(6)+T(5)+T(5)));
//G219: measure_preprocess_opt(123) => __E_bool_option_45(124).
	p(NT(123), (NT(124)));
//G220: bool_option(85)      => measure_preprocess_opt(123).
	p(NT(85), (NT(123)));
//G221: __E_bool_option_46(126) => 'd'.
	p(NT(126), (T(21)));
//G222: __E_bool_option_46(126) => 'd' 'e' 'b' 'u' 'g'.
	p(NT(126), (T(21)+T(6)+T(19)+T(16)+T(11)));
//G223: debug_opt(125)       => __E_bool_option_46(126).
	p(NT(125), (NT(126)));
//G224: bool_option(85)      => debug_opt(125).
	p(NT(85), (NT(125)));
//G225: __E_bool_option_47(128) => 'a' 'd'.
	p(NT(128), (T(3)+T(21)));
//G226: __E_bool_option_47(128) => 'a' 'u' 't' 'o' '-' 'd' 'i' 's' 'a' 'm' 'b' 'i' 'g' 'u' 'a' 't' 'e'.
	p(NT(128), (T(3)+T(16)+T(14)+T(20)+T(15)+T(21)+T(9)+T(5)+T(3)+T(12)+T(19)+T(9)+T(11)+T(16)+T(3)+T(14)+T(6)));
//G227: auto_disambiguate_opt(127) => __E_bool_option_47(128).
	p(NT(127), (NT(128)));
//G228: bool_option(85)      => auto_disambiguate_opt(127).
	p(NT(85), (NT(127)));
//G229: __E_bool_option_48(130) => 't' 't'.
	p(NT(130), (T(14)+T(14)));
//G230: __E_bool_option_48(130) => 't' 'r' 'i' 'm' '-' 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
	p(NT(130), (T(14)+T(4)+T(9)+T(12)+T(15)+T(14)+T(6)+T(4)+T(12)+T(9)+T(13)+T(3)+T(10)+T(5)));
//G231: trim_terminals_opt(129) => __E_bool_option_48(130).
	p(NT(129), (NT(130)));
//G232: bool_option(85)      => trim_terminals_opt(129).
	p(NT(85), (NT(129)));
//G233: __E_bool_option_49(132) => 'i' 'c' 'c'.
	p(NT(132), (T(9)+T(17)+T(17)));
//G234: __E_bool_option_49(132) => 'i' 'n' 'l' 'i' 'n' 'e' '-' 'c' 'c'.
	p(NT(132), (T(9)+T(13)+T(10)+T(9)+T(13)+T(6)+T(15)+T(17)+T(17)));
//G235: __E_bool_option_49(132) => 'i' 'n' 'l' 'i' 'n' 'e' '-' 'c' 'h' 'a' 'r' '-' 'c' 'l' 'a' 's' 's' 'e' 's'.
	p(NT(132), (T(9)+T(13)+T(10)+T(9)+T(13)+T(6)+T(15)+T(17)+T(18)+T(3)+T(4)+T(15)+T(17)+T(10)+T(3)+T(5)+T(5)+T(6)+T(5)));
//G236: inline_cc_opt(131)   => __E_bool_option_49(132).
	p(NT(131), (NT(132)));
//G237: bool_option(85)      => inline_cc_opt(131).
	p(NT(85), (NT(131)));
//G238: __E_list_option_50(134) => 'n' 'd'.
	p(NT(134), (T(13)+T(21)));
//G239: __E_list_option_50(134) => 'n' 'd' 'l'.
	p(NT(134), (T(13)+T(21)+T(10)));
//G240: __E_list_option_50(134) => 'n' 'o' 'd' 'i' 's' 'a' 'm' 'b' 'i' 'g' '-' 'l' 'i' 's' 't'.
	p(NT(134), (T(13)+T(20)+T(21)+T(9)+T(5)+T(3)+T(12)+T(19)+T(9)+T(11)+T(15)+T(10)+T(9)+T(5)+T(14)));
//G241: nodisambig_list_opt(133) => __E_list_option_50(134).
	p(NT(133), (NT(134)));
//G242: list_option(76)      => nodisambig_list_opt(133).
	p(NT(76), (NT(133)));
//G243: __E_list_option_51(136) => 'g' 'u' 'a' 'r' 'd' 's'.
	p(NT(136), (T(11)+T(16)+T(3)+T(4)+T(21)+T(5)));
//G244: __E_list_option_51(136) => 'e' 'n' 'a' 'b' 'l' 'e' 'd' '-' 'p' 'r' 'o' 'd' 'u' 'c' 't' 'i' 'o' 'n' 's'.
	p(NT(136), (T(6)+T(13)+T(3)+T(19)+T(10)+T(6)+T(21)+T(15)+T(2)+T(4)+T(20)+T(21)+T(16)+T(17)+T(14)+T(9)+T(20)+T(13)+T(5)));
//G245: enabled_prods_opt(135) => __E_list_option_51(136).
	p(NT(135), (NT(136)));
//G246: list_option(76)      => enabled_prods_opt(135).
	p(NT(76), (NT(135)));
//G247: trim_opt(137)        => 't' 'r' 'i' 'm'.
	p(NT(137), (T(14)+T(4)+T(9)+T(12)));
//G248: list_option(76)      => trim_opt(137).
	p(NT(76), (NT(137)));
//G249: __E_list_option_52(139) => 't' 'c'.
	p(NT(139), (T(14)+T(17)));
//G250: __E_list_option_52(139) => 't' 'r' 'i' 'm' '-' 'c' 'h' 'i' 'l' 'd' 'r' 'e' 'n'.
	p(NT(139), (T(14)+T(4)+T(9)+T(12)+T(15)+T(17)+T(18)+T(9)+T(10)+T(21)+T(4)+T(6)+T(13)));
//G251: trim_children_opt(138) => __E_list_option_52(139).
	p(NT(138), (NT(139)));
//G252: list_option(76)      => trim_children_opt(138).
	p(NT(76), (NT(138)));
//G253: __E_list_option_53(141) => 't' 'c' 't'.
	p(NT(141), (T(14)+T(17)+T(14)));
//G254: __E_list_option_53(141) => 't' 'r' 'i' 'm' '-' 'c' 'h' 'i' 'l' 'd' 'r' 'e' 'n' '-' 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
	p(NT(141), (T(14)+T(4)+T(9)+T(12)+T(15)+T(17)+T(18)+T(9)+T(10)+T(21)+T(4)+T(6)+T(13)+T(15)+T(14)+T(6)+T(4)+T(12)+T(9)+T(13)+T(3)+T(10)+T(5)));
//G255: trim_children_terminals_opt(140) => __E_list_option_53(141).
	p(NT(140), (NT(141)));
//G256: list_option(76)      => trim_children_terminals_opt(140).
	p(NT(76), (NT(140)));
//G257: __E_treepaths_option_54(143) => 'i'.
	p(NT(143), (T(9)));
//G258: __E_treepaths_option_54(143) => 'i' 'n' 'l' 'i' 'n' 'e'.
	p(NT(143), (T(9)+T(13)+T(10)+T(9)+T(13)+T(6)));
//G259: inline_opt(142)      => __E_treepaths_option_54(143).
	p(NT(142), (NT(143)));
//G260: treepaths_option(78) => inline_opt(142).
	p(NT(78), (NT(142)));
//G261: __E_bool_value_55(145) => 't'.
	p(NT(145), (T(14)));
//G262: __E_bool_value_55(145) => 't' 'r' 'u' 'e'.
	p(NT(145), (T(14)+T(4)+T(16)+T(6)));
//G263: __E_bool_value_55(145) => 'o' 'n'.
	p(NT(145), (T(20)+T(13)));
//G264: __E_bool_value_55(145) => '1'.
	p(NT(145), (T(29)));
//G265: __E_bool_value_55(145) => 'y'.
	p(NT(145), (T(28)));
//G266: __E_bool_value_55(145) => 'y' 'e' 's'.
	p(NT(145), (T(28)+T(6)+T(5)));
//G267: true_value(144)      => __E_bool_value_55(145).
	p(NT(144), (NT(145)));
//G268: bool_value(93)       => true_value(144).
	p(NT(93), (NT(144)));
//G269: __E_bool_value_56(147) => 'f'.
	p(NT(147), (T(7)));
//G270: __E_bool_value_56(147) => 'f' 'a' 'l' 's' 'e'.
	p(NT(147), (T(7)+T(3)+T(10)+T(5)+T(6)));
//G271: __E_bool_value_56(147) => 'o' 'f' 'f'.
	p(NT(147), (T(20)+T(7)+T(7)));
//G272: __E_bool_value_56(147) => '0'.
	p(NT(147), (T(30)));
//G273: __E_bool_value_56(147) => 'n'.
	p(NT(147), (T(13)));
//G274: __E_bool_value_56(147) => 'n' 'o'.
	p(NT(147), (T(13)+T(20)));
//G275: false_value(146)     => __E_bool_value_56(147).
	p(NT(146), (NT(147)));
//G276: bool_value(93)       => false_value(146).
	p(NT(93), (NT(146)));
//G277: __E_error_verbosity_57(149) => 'b'.
	p(NT(149), (T(19)));
//G278: __E_error_verbosity_57(149) => 'b' 'a' 's' 'i' 'c'.
	p(NT(149), (T(19)+T(3)+T(5)+T(9)+T(17)));
//G279: basic_sym(148)       => __E_error_verbosity_57(149).
	p(NT(148), (NT(149)));
//G280: error_verbosity(98)  => basic_sym(148).
	p(NT(98), (NT(148)));
//G281: __E_error_verbosity_58(151) => 'd'.
	p(NT(151), (T(21)));
//G282: __E_error_verbosity_58(151) => 'd' 'e' 't' 'a' 'i' 'l' 'e' 'd'.
	p(NT(151), (T(21)+T(6)+T(14)+T(3)+T(9)+T(10)+T(6)+T(21)));
//G283: detailed_sym(150)    => __E_error_verbosity_58(151).
	p(NT(150), (NT(151)));
//G284: error_verbosity(98)  => detailed_sym(150).
	p(NT(98), (NT(150)));
//G285: __E_error_verbosity_59(153) => 'r'.
	p(NT(153), (T(4)));
//G286: __E_error_verbosity_59(153) => 'r' 'c'.
	p(NT(153), (T(4)+T(17)));
//G287: __E_error_verbosity_59(153) => 'r' 'o' 'o' 't' '-' 'c' 'a' 'u' 's' 'e'.
	p(NT(153), (T(4)+T(20)+T(20)+T(14)+T(15)+T(17)+T(3)+T(16)+T(5)+T(6)));
//G288: root_cause_sym(152)  => __E_error_verbosity_59(153).
	p(NT(152), (NT(153)));
//G289: error_verbosity(98)  => root_cause_sym(152).
	p(NT(98), (NT(152)));
//G290: __E_symbol_60(154)   => alpha(3).
	p(NT(154), (NT(3)));
//G291: __E_symbol_60(154)   => '_'.
	p(NT(154), (T(31)));
//G292: __E_symbol_61(155)   => alnum(2).
	p(NT(155), (NT(2)));
//G293: __E_symbol_61(155)   => '_'.
	p(NT(155), (T(31)));
//G294: __E_symbol_62(156)   => null.
	p(NT(156), (nul));
//G295: __E_symbol_62(156)   => __E_symbol_61(155) __E_symbol_62(156).
	p(NT(156), (NT(155)+NT(156)));
//G296: symbol(8)            => __E_symbol_60(154) __E_symbol_62(156).
	p(NT(8), (NT(154)+NT(156)));
//G297: __E_symbol_list_63(157) => _(6) ',' _(6) symbol(8).
	p(NT(157), (NT(6)+T(32)+NT(6)+NT(8)));
//G298: __E_symbol_list_64(158) => null.
	p(NT(158), (nul));
//G299: __E_symbol_list_64(158) => __E_symbol_list_63(157) __E_symbol_list_64(158).
	p(NT(158), (NT(157)+NT(158)));
//G300: symbol_list(77)      => symbol(8) __E_symbol_list_64(158).
	p(NT(77), (NT(8)+NT(158)));
//G301: __E_treepath_65(160) => _(6) '>' _(6) symbol(8).
	p(NT(160), (NT(6)+T(33)+NT(6)+NT(8)));
//G302: __E_treepath_66(161) => null.
	p(NT(161), (nul));
//G303: __E_treepath_66(161) => __E_treepath_65(160) __E_treepath_66(161).
	p(NT(161), (NT(160)+NT(161)));
//G304: treepath(159)        => symbol(8) __E_treepath_66(161).
	p(NT(159), (NT(8)+NT(161)));
//G305: __E_treepath_list_67(162) => _(6) ',' _(6) treepath(159).
	p(NT(162), (NT(6)+T(32)+NT(6)+NT(159)));
//G306: __E_treepath_list_68(163) => null.
	p(NT(163), (nul));
//G307: __E_treepath_list_68(163) => __E_treepath_list_67(162) __E_treepath_list_68(163).
	p(NT(163), (NT(162)+NT(163)));
//G308: treepath_list(79)    => treepath(159) __E_treepath_list_68(163).
	p(NT(79), (NT(159)+NT(163)));
//G309: filename(43)         => quoted_string(9).
	p(NT(43), (NT(9)));
//G310: __E_quoted_string_69(165) => null.
	p(NT(165), (nul));
//G311: __E_quoted_string_69(165) => quoted_string_char(164) __E_quoted_string_69(165).
	p(NT(165), (NT(164)+NT(165)));
//G312: quoted_string(9)     => '"' __E_quoted_string_69(165) '"'.
	p(NT(9), (T(34)+NT(165)+T(34)));
//G313: quoted_string_char(164) => unescaped_s(10).
	p(NT(164), (NT(10)));
//G314: quoted_string_char(164) => escaped_s(11).
	p(NT(164), (NT(11)));
//G315: __E_unescaped_s_70(166) => space(4).
	p(NT(166), (NT(4)));
//G316: __E_unescaped_s_70(166) => printable(5).
	p(NT(166), (NT(5)));
//G317: __E_unescaped_s_71(167) => '"'.
	p(NT(167), (T(34)));
//G318: __E_unescaped_s_71(167) => '\\'.
	p(NT(167), (T(35)));
//G319: __N_0(175)           => __E_unescaped_s_71(167).
	p(NT(175), (NT(167)));
//G320: unescaped_s(10)      => __E_unescaped_s_70(166) & ~( __N_0(175) ).	 # conjunctive
	p(NT(10), (NT(166)) & ~(NT(175)));
//G321: __E_escaped_s_72(168) => '"'.
	p(NT(168), (T(34)));
//G322: __E_escaped_s_72(168) => '\\'.
	p(NT(168), (T(35)));
//G323: __E_escaped_s_72(168) => '/'.
	p(NT(168), (T(36)));
//G324: __E_escaped_s_72(168) => 'b'.
	p(NT(168), (T(19)));
//G325: __E_escaped_s_72(168) => 'f'.
	p(NT(168), (T(7)));
//G326: __E_escaped_s_72(168) => 'n'.
	p(NT(168), (T(13)));
//G327: __E_escaped_s_72(168) => 'r'.
	p(NT(168), (T(4)));
//G328: __E_escaped_s_72(168) => 't'.
	p(NT(168), (T(14)));
//G329: escaped_s(11)        => '\\' __E_escaped_s_72(168).
	p(NT(11), (T(35)+NT(168)));
//G330: __E___73(169)        => __(7).
	p(NT(169), (NT(7)));
//G331: __E___73(169)        => null.
	p(NT(169), (nul));
//G332: _(6)                 => __E___73(169).
	p(NT(6), (NT(169)));
//G333: __E____74(170)       => space(4).
	p(NT(170), (NT(4)));
//G334: __E____74(170)       => comment(171).
	p(NT(170), (NT(171)));
//G335: __(7)                => __E____74(170) _(6).
	p(NT(7), (NT(170)+NT(6)));
//G336: __E_comment_75(172)  => printable(5).
	p(NT(172), (NT(5)));
//G337: __E_comment_75(172)  => '\t'.
	p(NT(172), (T(27)));
//G338: __E_comment_76(173)  => null.
	p(NT(173), (nul));
//G339: __E_comment_76(173)  => __E_comment_75(172) __E_comment_76(173).
	p(NT(173), (NT(172)+NT(173)));
//G340: __E_comment_77(174)  => '\r'.
	p(NT(174), (T(37)));
//G341: __E_comment_77(174)  => '\n'.
	p(NT(174), (T(38)));
//G342: __E_comment_77(174)  => eof(1).
	p(NT(174), (NT(1)));
//G343: comment(171)         => '#' __E_comment_76(173) __E_comment_77(174).
	p(NT(171), (T(39)+NT(173)+NT(174)));
	#undef T
	#undef NT
	return loaded = true, p;
}

inline ::idni::grammar<char_type, terminal_type> grammar(
	nts, productions(), start_symbol, char_classes, grammar_options);

} // namespace tgf_repl_parser_data

struct tgf_repl_parser : public idni::parser<char, char> {
	enum nonterminal {
		nul, eof, alnum, alpha, space, printable, _, __, symbol, quoted_string, 
		unescaped_s, escaped_s, parse_input_char_seq, start, __E_start_0, statement, __E___E_start_0_1, __E___E_start_0_2, grammar_cmd, igrammar_cmd, 
		unreachable_cmd, reload_cmd, load_cmd, start_cmd, help, version, quit, license, clear, get, 
		set, toggle, enable, disable, add, del, parse_file_cmd, parse_cmd, parse_sym, __E_parse_cmd_3, 
		parse_input, parse_file_sym, __E_parse_file_cmd_4, filename, grammar_sym, __E_grammar_cmd_5, igrammar_sym, __E_igrammar_cmd_6, __E_igrammar_cmd_7, start_sym, 
		__E_start_cmd_8, __E_start_cmd_9, unreachable_sym, __E_unreachable_cmd_10, __E_unreachable_cmd_11, reload_sym, __E_reload_cmd_12, load_sym, __E_load_cmd_13, help_sym, 
		__E_help_14, __E_help_15, help_arg, version_sym, __E_version_16, license_sym, __E_license_17, quit_sym, __E_quit_18, clear_sym, 
		__E_clear_19, get_sym, __E_get_20, option, add_sym, __E_add_21, list_option, symbol_list, treepaths_option, treepath_list, 
		del_sym, __E_del_22, __E_del_23, toggle_sym, __E_toggle_24, bool_option, enable_sym, __E_enable_25, disable_sym, __E_disable_26, 
		set_sym, __E_set_27, __E___E_set_27_28, bool_value, __E___E_set_27_29, __E___E_set_27_30, enum_ev_option, __E___E_set_27_31, error_verbosity, __E_parse_input_32, 
		__E_parse_input_33, error_verbosity_opt, __E_enum_ev_option_34, status_opt, __E_bool_option_35, colors_opt, __E_bool_option_36, print_ambiguity_opt, __E_bool_option_37, print_graphs_opt, 
		__E_bool_option_38, print_rules_opt, __E_bool_option_39, print_facts_opt, __E_bool_option_40, print_terminals_opt, __E_bool_option_41, measure_parsing_opt, __E_bool_option_42, measure_each_pos_opt, 
		__E_bool_option_43, measure_forest_opt, __E_bool_option_44, measure_preprocess_opt, __E_bool_option_45, debug_opt, __E_bool_option_46, auto_disambiguate_opt, __E_bool_option_47, trim_terminals_opt, 
		__E_bool_option_48, inline_cc_opt, __E_bool_option_49, nodisambig_list_opt, __E_list_option_50, enabled_prods_opt, __E_list_option_51, trim_opt, trim_children_opt, __E_list_option_52, 
		trim_children_terminals_opt, __E_list_option_53, inline_opt, __E_treepaths_option_54, true_value, __E_bool_value_55, false_value, __E_bool_value_56, basic_sym, __E_error_verbosity_57, 
		detailed_sym, __E_error_verbosity_58, root_cause_sym, __E_error_verbosity_59, __E_symbol_60, __E_symbol_61, __E_symbol_62, __E_symbol_list_63, __E_symbol_list_64, treepath, 
		__E_treepath_65, __E_treepath_66, __E_treepath_list_67, __E_treepath_list_68, quoted_string_char, __E_quoted_string_69, __E_unescaped_s_70, __E_unescaped_s_71, __E_escaped_s_72, __E___73, 
		__E____74, comment, __E_comment_75, __E_comment_76, __E_comment_77, __N_0, 
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
