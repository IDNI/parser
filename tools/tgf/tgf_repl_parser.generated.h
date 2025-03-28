// This file is generated from a file tools/tgf/tgf_repl.tgf by
//       https://github.com/IDNI/parser/tools/tgf
//
#ifndef __TGF_REPL_PARSER_H__
#define __TGF_REPL_PARSER_H__

#include "parser.h"

namespace tgf_repl_parser_data {

using char_type     = char;
using terminal_type = char;

inline static constexpr size_t nt_bits = 8;
inline const std::vector<std::string> symbol_names{
	"", "eof", "alnum", "alpha", "space", "printable", "_", "__", "symbol", "quoted_string", 
	"unescaped_s", "escaped_s", "parse_input_char_seq", "start", "__E_start_0", "statement", "__E___E_start_0_1", "__E___E_start_0_2", "grammar_cmd", "igrammar_cmd", 
	"unreachable_cmd", "reload_cmd", "load_cmd", "start_cmd", "help", "version", "quit", "clear", "get", "set", 
	"toggle", "enable", "disable", "add", "del", "parse_file_cmd", "parse_cmd", "parse_sym", "__E_parse_cmd_3", "parse_input", 
	"parse_file_sym", "__E_parse_file_cmd_4", "filename", "grammar_sym", "__E_grammar_cmd_5", "igrammar_sym", "__E_igrammar_cmd_6", "__E_igrammar_cmd_7", "start_sym", "__E_start_cmd_8", 
	"__E_start_cmd_9", "unreachable_sym", "__E_unreachable_cmd_10", "__E_unreachable_cmd_11", "reload_sym", "__E_reload_cmd_12", "load_sym", "__E_load_cmd_13", "help_sym", "__E_help_14", 
	"__E_help_15", "help_arg", "version_sym", "__E_version_16", "quit_sym", "__E_quit_17", "clear_sym", "__E_clear_18", "get_sym", "__E_get_19", 
	"option", "add_sym", "__E_add_20", "list_option", "symbol_list", "treepaths_option", "treepath_list", "del_sym", "__E_del_21", "__E_del_22", 
	"toggle_sym", "__E_toggle_23", "bool_option", "enable_sym", "__E_enable_24", "disable_sym", "__E_disable_25", "set_sym", "__E_set_26", "__E___E_set_26_27", 
	"bool_value", "__E___E_set_26_28", "__E___E_set_26_29", "enum_ev_option", "__E___E_set_26_30", "error_verbosity", "__E_parse_input_31", "__E_parse_input_32", "error_verbosity_opt", "__E_enum_ev_option_33", 
	"status_opt", "__E_bool_option_34", "colors_opt", "__E_bool_option_35", "print_ambiguity_opt", "__E_bool_option_36", "print_graphs_opt", "__E_bool_option_37", "print_rules_opt", "__E_bool_option_38", 
	"print_facts_opt", "__E_bool_option_39", "print_terminals_opt", "__E_bool_option_40", "measure_parsing_opt", "__E_bool_option_41", "measure_each_pos_opt", "__E_bool_option_42", "measure_forest_opt", "__E_bool_option_43", 
	"measure_preprocess_opt", "__E_bool_option_44", "debug_opt", "__E_bool_option_45", "auto_disambiguate_opt", "__E_bool_option_46", "trim_terminals_opt", "__E_bool_option_47", "inline_cc_opt", "__E_bool_option_48", 
	"nodisambig_list_opt", "__E_list_option_49", "enabled_prods_opt", "__E_list_option_50", "trim_opt", "trim_children_opt", "__E_list_option_51", "trim_children_terminals_opt", "__E_list_option_52", "inline_opt", 
	"__E_treepaths_option_53", "true_value", "__E_bool_value_54", "false_value", "__E_bool_value_55", "basic_sym", "__E_error_verbosity_56", "detailed_sym", "__E_error_verbosity_57", "root_cause_sym", 
	"__E_error_verbosity_58", "__E_symbol_59", "__E_symbol_60", "__E_symbol_61", "__E_symbol_list_62", "__E_symbol_list_63", "treepath", "__E_treepath_64", "__E_treepath_65", "__E_treepath_list_66", 
	"__E_treepath_list_67", "quoted_string_char", "__E_quoted_string_68", "__E_unescaped_s_69", "__E_unescaped_s_70", "__E_escaped_s_71", "__E___72", "__E____73", "comment", "__E_comment_74", 
	"__E_comment_75", "__E_comment_76", "__N_0", 
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
//G15:  statement(15)        => clear(27).
	p(NT(15), (NT(27)));
//G16:  statement(15)        => get(28).
	p(NT(15), (NT(28)));
//G17:  statement(15)        => set(29).
	p(NT(15), (NT(29)));
//G18:  statement(15)        => toggle(30).
	p(NT(15), (NT(30)));
//G19:  statement(15)        => enable(31).
	p(NT(15), (NT(31)));
//G20:  statement(15)        => disable(32).
	p(NT(15), (NT(32)));
//G21:  statement(15)        => add(33).
	p(NT(15), (NT(33)));
//G22:  statement(15)        => del(34).
	p(NT(15), (NT(34)));
//G23:  statement(15)        => parse_file_cmd(35).
	p(NT(15), (NT(35)));
//G24:  statement(15)        => parse_cmd(36).
	p(NT(15), (NT(36)));
//G25:  __E_parse_cmd_3(38)  => 'p'.
	p(NT(38), (T(2)));
//G26:  __E_parse_cmd_3(38)  => 'p' 'a' 'r' 's' 'e'.
	p(NT(38), (T(2)+T(3)+T(4)+T(5)+T(6)));
//G27:  parse_sym(37)        => __E_parse_cmd_3(38).
	p(NT(37), (NT(38)));
//G28:  parse_cmd(36)        => parse_sym(37) __(7) parse_input(39).
	p(NT(36), (NT(37)+NT(7)+NT(39)));
//G29:  __E_parse_file_cmd_4(41) => 'f'.
	p(NT(41), (T(7)));
//G30:  __E_parse_file_cmd_4(41) => 'p' 'f'.
	p(NT(41), (T(2)+T(7)));
//G31:  __E_parse_file_cmd_4(41) => 'p' 'a' 'r' 's' 'e' ' ' 'f' 'i' 'l' 'e'.
	p(NT(41), (T(2)+T(3)+T(4)+T(5)+T(6)+T(8)+T(7)+T(9)+T(10)+T(6)));
//G32:  parse_file_sym(40)   => __E_parse_file_cmd_4(41).
	p(NT(40), (NT(41)));
//G33:  parse_file_cmd(35)   => parse_file_sym(40) __(7) filename(42).
	p(NT(35), (NT(40)+NT(7)+NT(42)));
//G34:  __E_grammar_cmd_5(44) => 'g'.
	p(NT(44), (T(11)));
//G35:  __E_grammar_cmd_5(44) => 'g' 'r' 'a' 'm' 'm' 'a' 'r'.
	p(NT(44), (T(11)+T(4)+T(3)+T(12)+T(12)+T(3)+T(4)));
//G36:  grammar_sym(43)      => __E_grammar_cmd_5(44).
	p(NT(43), (NT(44)));
//G37:  grammar_cmd(18)      => grammar_sym(43).
	p(NT(18), (NT(43)));
//G38:  __E_igrammar_cmd_6(46) => 'i'.
	p(NT(46), (T(9)));
//G39:  __E_igrammar_cmd_6(46) => 'i' 'g'.
	p(NT(46), (T(9)+T(11)));
//G40:  __E_igrammar_cmd_6(46) => 'i' 'n' 't' 'e' 'r' 'n' 'a' 'l' '-' 'g' 'r' 'a' 'm' 'm' 'a' 'r'.
	p(NT(46), (T(9)+T(13)+T(14)+T(6)+T(4)+T(13)+T(3)+T(10)+T(15)+T(11)+T(4)+T(3)+T(12)+T(12)+T(3)+T(4)));
//G41:  igrammar_sym(45)     => __E_igrammar_cmd_6(46).
	p(NT(45), (NT(46)));
//G42:  __E_igrammar_cmd_7(47) => __(7) symbol(8).
	p(NT(47), (NT(7)+NT(8)));
//G43:  __E_igrammar_cmd_7(47) => null.
	p(NT(47), (nul));
//G44:  igrammar_cmd(19)     => igrammar_sym(45) __E_igrammar_cmd_7(47).
	p(NT(19), (NT(45)+NT(47)));
//G45:  __E_start_cmd_8(49)  => 's'.
	p(NT(49), (T(5)));
//G46:  __E_start_cmd_8(49)  => 's' 't' 'a' 'r' 't'.
	p(NT(49), (T(5)+T(14)+T(3)+T(4)+T(14)));
//G47:  start_sym(48)        => __E_start_cmd_8(49).
	p(NT(48), (NT(49)));
//G48:  __E_start_cmd_9(50)  => __(7) symbol(8).
	p(NT(50), (NT(7)+NT(8)));
//G49:  __E_start_cmd_9(50)  => null.
	p(NT(50), (nul));
//G50:  start_cmd(23)        => start_sym(48) __E_start_cmd_9(50).
	p(NT(23), (NT(48)+NT(50)));
//G51:  __E_unreachable_cmd_10(52) => 'u'.
	p(NT(52), (T(16)));
//G52:  __E_unreachable_cmd_10(52) => 'u' 'n' 'r' 'e' 'a' 'c' 'h' 'a' 'b' 'l' 'e'.
	p(NT(52), (T(16)+T(13)+T(4)+T(6)+T(3)+T(17)+T(18)+T(3)+T(19)+T(10)+T(6)));
//G53:  unreachable_sym(51)  => __E_unreachable_cmd_10(52).
	p(NT(51), (NT(52)));
//G54:  __E_unreachable_cmd_11(53) => __(7) symbol(8).
	p(NT(53), (NT(7)+NT(8)));
//G55:  __E_unreachable_cmd_11(53) => null.
	p(NT(53), (nul));
//G56:  unreachable_cmd(20)  => unreachable_sym(51) __E_unreachable_cmd_11(53).
	p(NT(20), (NT(51)+NT(53)));
//G57:  __E_reload_cmd_12(55) => 'r'.
	p(NT(55), (T(4)));
//G58:  __E_reload_cmd_12(55) => 'r' 'e' 'l' 'o' 'a' 'd'.
	p(NT(55), (T(4)+T(6)+T(10)+T(20)+T(3)+T(21)));
//G59:  reload_sym(54)       => __E_reload_cmd_12(55).
	p(NT(54), (NT(55)));
//G60:  reload_cmd(21)       => reload_sym(54).
	p(NT(21), (NT(54)));
//G61:  __E_load_cmd_13(57)  => 'l'.
	p(NT(57), (T(10)));
//G62:  __E_load_cmd_13(57)  => 'l' 'o' 'a' 'd'.
	p(NT(57), (T(10)+T(20)+T(3)+T(21)));
//G63:  load_sym(56)         => __E_load_cmd_13(57).
	p(NT(56), (NT(57)));
//G64:  load_cmd(22)         => load_sym(56) __(7) filename(42).
	p(NT(22), (NT(56)+NT(7)+NT(42)));
//G65:  __E_help_14(59)      => 'h'.
	p(NT(59), (T(18)));
//G66:  __E_help_14(59)      => 'h' 'e' 'l' 'p'.
	p(NT(59), (T(18)+T(6)+T(10)+T(2)));
//G67:  help_sym(58)         => __E_help_14(59).
	p(NT(58), (NT(59)));
//G68:  __E_help_15(60)      => __(7) help_arg(61).
	p(NT(60), (NT(7)+NT(61)));
//G69:  __E_help_15(60)      => null.
	p(NT(60), (nul));
//G70:  help(24)             => help_sym(58) __E_help_15(60).
	p(NT(24), (NT(58)+NT(60)));
//G71:  __E_version_16(63)   => 'v'.
	p(NT(63), (T(22)));
//G72:  __E_version_16(63)   => 'v' 'e' 'r' 's' 'i' 'o' 'n'.
	p(NT(63), (T(22)+T(6)+T(4)+T(5)+T(9)+T(20)+T(13)));
//G73:  version_sym(62)      => __E_version_16(63).
	p(NT(62), (NT(63)));
//G74:  version(25)          => version_sym(62).
	p(NT(25), (NT(62)));
//G75:  __E_quit_17(65)      => 'q'.
	p(NT(65), (T(23)));
//G76:  __E_quit_17(65)      => 'q' 'u' 'i' 't'.
	p(NT(65), (T(23)+T(16)+T(9)+T(14)));
//G77:  __E_quit_17(65)      => 'e'.
	p(NT(65), (T(6)));
//G78:  __E_quit_17(65)      => 'e' 'x' 'i' 't'.
	p(NT(65), (T(6)+T(24)+T(9)+T(14)));
//G79:  quit_sym(64)         => __E_quit_17(65).
	p(NT(64), (NT(65)));
//G80:  quit(26)             => quit_sym(64).
	p(NT(26), (NT(64)));
//G81:  __E_clear_18(67)     => 'c' 'l' 's'.
	p(NT(67), (T(17)+T(10)+T(5)));
//G82:  __E_clear_18(67)     => 'c' 'l' 'e' 'a' 'r'.
	p(NT(67), (T(17)+T(10)+T(6)+T(3)+T(4)));
//G83:  clear_sym(66)        => __E_clear_18(67).
	p(NT(66), (NT(67)));
//G84:  clear(27)            => clear_sym(66).
	p(NT(27), (NT(66)));
//G85:  get_sym(68)          => 'g' 'e' 't'.
	p(NT(68), (T(11)+T(6)+T(14)));
//G86:  __E_get_19(69)       => __(7) option(70).
	p(NT(69), (NT(7)+NT(70)));
//G87:  __E_get_19(69)       => null.
	p(NT(69), (nul));
//G88:  get(28)              => get_sym(68) __E_get_19(69).
	p(NT(28), (NT(68)+NT(69)));
//G89:  add_sym(71)          => 'a' 'd' 'd'.
	p(NT(71), (T(3)+T(21)+T(21)));
//G90:  __E_add_20(72)       => list_option(73) __(7) symbol_list(74).
	p(NT(72), (NT(73)+NT(7)+NT(74)));
//G91:  __E_add_20(72)       => treepaths_option(75) __(7) treepath_list(76).
	p(NT(72), (NT(75)+NT(7)+NT(76)));
//G92:  add(33)              => add_sym(71) __(7) __E_add_20(72).
	p(NT(33), (NT(71)+NT(7)+NT(72)));
//G93:  __E_del_21(78)       => 'd' 'e' 'l'.
	p(NT(78), (T(21)+T(6)+T(10)));
//G94:  __E_del_21(78)       => 'd' 'e' 'l' 'e' 't' 'e'.
	p(NT(78), (T(21)+T(6)+T(10)+T(6)+T(14)+T(6)));
//G95:  __E_del_21(78)       => 'r' 'm'.
	p(NT(78), (T(4)+T(12)));
//G96:  __E_del_21(78)       => 'r' 'e' 'm'.
	p(NT(78), (T(4)+T(6)+T(12)));
//G97:  __E_del_21(78)       => 'r' 'e' 'm' 'o' 'v' 'e'.
	p(NT(78), (T(4)+T(6)+T(12)+T(20)+T(22)+T(6)));
//G98:  del_sym(77)          => __E_del_21(78).
	p(NT(77), (NT(78)));
//G99:  __E_del_22(79)       => list_option(73) __(7) symbol_list(74).
	p(NT(79), (NT(73)+NT(7)+NT(74)));
//G100: __E_del_22(79)       => treepaths_option(75) __(7) treepath_list(76).
	p(NT(79), (NT(75)+NT(7)+NT(76)));
//G101: del(34)              => del_sym(77) __(7) __E_del_22(79).
	p(NT(34), (NT(77)+NT(7)+NT(79)));
//G102: __E_toggle_23(81)    => 't' 'o' 'g'.
	p(NT(81), (T(14)+T(20)+T(11)));
//G103: __E_toggle_23(81)    => 't' 'o' 'g' 'g' 'l' 'e'.
	p(NT(81), (T(14)+T(20)+T(11)+T(11)+T(10)+T(6)));
//G104: toggle_sym(80)       => __E_toggle_23(81).
	p(NT(80), (NT(81)));
//G105: toggle(30)           => toggle_sym(80) __(7) bool_option(82).
	p(NT(30), (NT(80)+NT(7)+NT(82)));
//G106: __E_enable_24(84)    => 'e' 'n' __(7).
	p(NT(84), (T(6)+T(13)+NT(7)));
//G107: __E_enable_24(84)    => 'e' 'n' 'a' 'b' 'l' 'e' __(7).
	p(NT(84), (T(6)+T(13)+T(3)+T(19)+T(10)+T(6)+NT(7)));
//G108: enable_sym(83)       => __E_enable_24(84).
	p(NT(83), (NT(84)));
//G109: enable(31)           => enable_sym(83) bool_option(82).
	p(NT(31), (NT(83)+NT(82)));
//G110: __E_disable_25(86)   => 'd' 'i' 's' __(7).
	p(NT(86), (T(21)+T(9)+T(5)+NT(7)));
//G111: __E_disable_25(86)   => 'd' 'i' 's' 'a' 'b' 'l' 'e' __(7).
	p(NT(86), (T(21)+T(9)+T(5)+T(3)+T(19)+T(10)+T(6)+NT(7)));
//G112: disable_sym(85)      => __E_disable_25(86).
	p(NT(85), (NT(86)));
//G113: disable(32)          => disable_sym(85) bool_option(82).
	p(NT(32), (NT(85)+NT(82)));
//G114: set_sym(87)          => 's' 'e' 't'.
	p(NT(87), (T(5)+T(6)+T(14)));
//G115: __E___E_set_26_27(89) => __(7).
	p(NT(89), (NT(7)));
//G116: __E___E_set_26_27(89) => _(6) '=' _(6).
	p(NT(89), (NT(6)+T(25)+NT(6)));
//G117: __E_set_26(88)       => bool_option(82) __E___E_set_26_27(89) bool_value(90).
	p(NT(88), (NT(82)+NT(89)+NT(90)));
//G118: __E___E_set_26_28(91) => __(7).
	p(NT(91), (NT(7)));
//G119: __E___E_set_26_28(91) => _(6) '=' _(6).
	p(NT(91), (NT(6)+T(25)+NT(6)));
//G120: __E_set_26(88)       => list_option(73) __E___E_set_26_28(91) symbol_list(74).
	p(NT(88), (NT(73)+NT(91)+NT(74)));
//G121: __E___E_set_26_29(92) => __(7).
	p(NT(92), (NT(7)));
//G122: __E___E_set_26_29(92) => _(6) '=' _(6).
	p(NT(92), (NT(6)+T(25)+NT(6)));
//G123: __E_set_26(88)       => treepaths_option(75) __E___E_set_26_29(92) treepath_list(76).
	p(NT(88), (NT(75)+NT(92)+NT(76)));
//G124: __E___E_set_26_30(94) => __(7).
	p(NT(94), (NT(7)));
//G125: __E___E_set_26_30(94) => _(6) '=' _(6).
	p(NT(94), (NT(6)+T(25)+NT(6)));
//G126: __E_set_26(88)       => enum_ev_option(93) __E___E_set_26_30(94) error_verbosity(95).
	p(NT(88), (NT(93)+NT(94)+NT(95)));
//G127: set(29)              => set_sym(87) __(7) __E_set_26(88).
	p(NT(29), (NT(87)+NT(7)+NT(88)));
//G128: parse_input(39)      => quoted_string(9).
	p(NT(39), (NT(9)));
//G129: __E_parse_input_31(96) => printable(5).
	p(NT(96), (NT(5)));
//G130: __E_parse_input_31(96) => '\t'.
	p(NT(96), (T(26)));
//G131: __E_parse_input_32(97) => __E_parse_input_31(96).
	p(NT(97), (NT(96)));
//G132: __E_parse_input_32(97) => __E_parse_input_31(96) __E_parse_input_32(97).
	p(NT(97), (NT(96)+NT(97)));
//G133: parse_input_char_seq(12) => __E_parse_input_32(97).
	p(NT(12), (NT(97)));
//G134: parse_input(39)      => parse_input_char_seq(12).
	p(NT(39), (NT(12)));
//G135: help_arg(61)         => grammar_sym(43).
	p(NT(61), (NT(43)));
//G136: help_arg(61)         => igrammar_sym(45).
	p(NT(61), (NT(45)));
//G137: help_arg(61)         => unreachable_sym(51).
	p(NT(61), (NT(51)));
//G138: help_arg(61)         => start_sym(48).
	p(NT(61), (NT(48)));
//G139: help_arg(61)         => parse_sym(37).
	p(NT(61), (NT(37)));
//G140: help_arg(61)         => parse_file_sym(40).
	p(NT(61), (NT(40)));
//G141: help_arg(61)         => load_sym(56).
	p(NT(61), (NT(56)));
//G142: help_arg(61)         => reload_sym(54).
	p(NT(61), (NT(54)));
//G143: help_arg(61)         => help_sym(58).
	p(NT(61), (NT(58)));
//G144: help_arg(61)         => quit_sym(64).
	p(NT(61), (NT(64)));
//G145: help_arg(61)         => version_sym(62).
	p(NT(61), (NT(62)));
//G146: help_arg(61)         => clear_sym(66).
	p(NT(61), (NT(66)));
//G147: help_arg(61)         => get_sym(68).
	p(NT(61), (NT(68)));
//G148: help_arg(61)         => set_sym(87).
	p(NT(61), (NT(87)));
//G149: help_arg(61)         => add_sym(71).
	p(NT(61), (NT(71)));
//G150: help_arg(61)         => del_sym(77).
	p(NT(61), (NT(77)));
//G151: help_arg(61)         => toggle_sym(80).
	p(NT(61), (NT(80)));
//G152: help_arg(61)         => enable_sym(83).
	p(NT(61), (NT(83)));
//G153: help_arg(61)         => disable_sym(85).
	p(NT(61), (NT(85)));
//G154: option(70)           => bool_option(82).
	p(NT(70), (NT(82)));
//G155: option(70)           => enum_ev_option(93).
	p(NT(70), (NT(93)));
//G156: option(70)           => list_option(73).
	p(NT(70), (NT(73)));
//G157: option(70)           => treepaths_option(75).
	p(NT(70), (NT(75)));
//G158: __E_enum_ev_option_33(99) => 'e'.
	p(NT(99), (T(6)));
//G159: __E_enum_ev_option_33(99) => 'e' 'r' 'r' 'o' 'r' '-' 'v' 'e' 'r' 'b' 'o' 's' 'i' 't' 'y'.
	p(NT(99), (T(6)+T(4)+T(4)+T(20)+T(4)+T(15)+T(22)+T(6)+T(4)+T(19)+T(20)+T(5)+T(9)+T(14)+T(27)));
//G160: error_verbosity_opt(98) => __E_enum_ev_option_33(99).
	p(NT(98), (NT(99)));
//G161: enum_ev_option(93)   => error_verbosity_opt(98).
	p(NT(93), (NT(98)));
//G162: __E_bool_option_34(101) => 's'.
	p(NT(101), (T(5)));
//G163: __E_bool_option_34(101) => 's' 't' 'a' 't' 'u' 's'.
	p(NT(101), (T(5)+T(14)+T(3)+T(14)+T(16)+T(5)));
//G164: status_opt(100)      => __E_bool_option_34(101).
	p(NT(100), (NT(101)));
//G165: bool_option(82)      => status_opt(100).
	p(NT(82), (NT(100)));
//G166: __E_bool_option_35(103) => 'c'.
	p(NT(103), (T(17)));
//G167: __E_bool_option_35(103) => 'c' 'o' 'l' 'o' 'r'.
	p(NT(103), (T(17)+T(20)+T(10)+T(20)+T(4)));
//G168: __E_bool_option_35(103) => 'c' 'o' 'l' 'o' 'r' 's'.
	p(NT(103), (T(17)+T(20)+T(10)+T(20)+T(4)+T(5)));
//G169: colors_opt(102)      => __E_bool_option_35(103).
	p(NT(102), (NT(103)));
//G170: bool_option(82)      => colors_opt(102).
	p(NT(82), (NT(102)));
//G171: __E_bool_option_36(105) => 'a'.
	p(NT(105), (T(3)));
//G172: __E_bool_option_36(105) => 'a' 'm' 'b' 'i' 'g' 'u' 'i' 't' 'y'.
	p(NT(105), (T(3)+T(12)+T(19)+T(9)+T(11)+T(16)+T(9)+T(14)+T(27)));
//G173: __E_bool_option_36(105) => 'p' 'r' 'i' 'n' 't' '-' 'a' 'm' 'b' 'i' 'g' 'u' 'i' 't' 'y'.
	p(NT(105), (T(2)+T(4)+T(9)+T(13)+T(14)+T(15)+T(3)+T(12)+T(19)+T(9)+T(11)+T(16)+T(9)+T(14)+T(27)));
//G174: print_ambiguity_opt(104) => __E_bool_option_36(105).
	p(NT(104), (NT(105)));
//G175: bool_option(82)      => print_ambiguity_opt(104).
	p(NT(82), (NT(104)));
//G176: __E_bool_option_37(107) => 'g'.
	p(NT(107), (T(11)));
//G177: __E_bool_option_37(107) => 'g' 'r' 'a' 'p' 'h' 's'.
	p(NT(107), (T(11)+T(4)+T(3)+T(2)+T(18)+T(5)));
//G178: __E_bool_option_37(107) => 'p' 'r' 'i' 'n' 't' '-' 'g' 'r' 'a' 'p' 'h' 's'.
	p(NT(107), (T(2)+T(4)+T(9)+T(13)+T(14)+T(15)+T(11)+T(4)+T(3)+T(2)+T(18)+T(5)));
//G179: print_graphs_opt(106) => __E_bool_option_37(107).
	p(NT(106), (NT(107)));
//G180: bool_option(82)      => print_graphs_opt(106).
	p(NT(82), (NT(106)));
//G181: __E_bool_option_38(109) => 'r'.
	p(NT(109), (T(4)));
//G182: __E_bool_option_38(109) => 'r' 'u' 'l' 'e' 's'.
	p(NT(109), (T(4)+T(16)+T(10)+T(6)+T(5)));
//G183: __E_bool_option_38(109) => 'p' 'r' 'i' 'n' 't' '-' 'r' 'u' 'l' 'e' 's'.
	p(NT(109), (T(2)+T(4)+T(9)+T(13)+T(14)+T(15)+T(4)+T(16)+T(10)+T(6)+T(5)));
//G184: print_rules_opt(108) => __E_bool_option_38(109).
	p(NT(108), (NT(109)));
//G185: bool_option(82)      => print_rules_opt(108).
	p(NT(82), (NT(108)));
//G186: __E_bool_option_39(111) => 'f'.
	p(NT(111), (T(7)));
//G187: __E_bool_option_39(111) => 'f' 'a' 'c' 't' 's'.
	p(NT(111), (T(7)+T(3)+T(17)+T(14)+T(5)));
//G188: __E_bool_option_39(111) => 'p' 'r' 'i' 'n' 't' '-' 'f' 'a' 'c' 't' 's'.
	p(NT(111), (T(2)+T(4)+T(9)+T(13)+T(14)+T(15)+T(7)+T(3)+T(17)+T(14)+T(5)));
//G189: print_facts_opt(110) => __E_bool_option_39(111).
	p(NT(110), (NT(111)));
//G190: bool_option(82)      => print_facts_opt(110).
	p(NT(82), (NT(110)));
//G191: __E_bool_option_40(113) => 't'.
	p(NT(113), (T(14)));
//G192: __E_bool_option_40(113) => 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
	p(NT(113), (T(14)+T(6)+T(4)+T(12)+T(9)+T(13)+T(3)+T(10)+T(5)));
//G193: __E_bool_option_40(113) => 'p' 'r' 'i' 'n' 't' '-' 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
	p(NT(113), (T(2)+T(4)+T(9)+T(13)+T(14)+T(15)+T(14)+T(6)+T(4)+T(12)+T(9)+T(13)+T(3)+T(10)+T(5)));
//G194: print_terminals_opt(112) => __E_bool_option_40(113).
	p(NT(112), (NT(113)));
//G195: bool_option(82)      => print_terminals_opt(112).
	p(NT(82), (NT(112)));
//G196: __E_bool_option_41(115) => 'm'.
	p(NT(115), (T(12)));
//G197: __E_bool_option_41(115) => 'm' 'e' 'a' 's' 'u' 'r' 'e'.
	p(NT(115), (T(12)+T(6)+T(3)+T(5)+T(16)+T(4)+T(6)));
//G198: __E_bool_option_41(115) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'p' 'a' 'r' 's' 'i' 'n' 'g'.
	p(NT(115), (T(12)+T(6)+T(3)+T(5)+T(16)+T(4)+T(6)+T(15)+T(2)+T(3)+T(4)+T(5)+T(9)+T(13)+T(11)));
//G199: measure_parsing_opt(114) => __E_bool_option_41(115).
	p(NT(114), (NT(115)));
//G200: bool_option(82)      => measure_parsing_opt(114).
	p(NT(82), (NT(114)));
//G201: __E_bool_option_42(117) => 'm' 'e'.
	p(NT(117), (T(12)+T(6)));
//G202: __E_bool_option_42(117) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'e' 'a' 'c' 'h'.
	p(NT(117), (T(12)+T(6)+T(3)+T(5)+T(16)+T(4)+T(6)+T(15)+T(6)+T(3)+T(17)+T(18)));
//G203: __E_bool_option_42(117) => 'm' 'e' 'p'.
	p(NT(117), (T(12)+T(6)+T(2)));
//G204: __E_bool_option_42(117) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'e' 'a' 'c' 'h' '-' 'p' 'o' 's'.
	p(NT(117), (T(12)+T(6)+T(3)+T(5)+T(16)+T(4)+T(6)+T(15)+T(6)+T(3)+T(17)+T(18)+T(15)+T(2)+T(20)+T(5)));
//G205: measure_each_pos_opt(116) => __E_bool_option_42(117).
	p(NT(116), (NT(117)));
//G206: bool_option(82)      => measure_each_pos_opt(116).
	p(NT(82), (NT(116)));
//G207: __E_bool_option_43(119) => 'm' 'f'.
	p(NT(119), (T(12)+T(7)));
//G208: __E_bool_option_43(119) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'f' 'o' 'r' 'e' 's' 't'.
	p(NT(119), (T(12)+T(6)+T(3)+T(5)+T(16)+T(4)+T(6)+T(15)+T(7)+T(20)+T(4)+T(6)+T(5)+T(14)));
//G209: measure_forest_opt(118) => __E_bool_option_43(119).
	p(NT(118), (NT(119)));
//G210: bool_option(82)      => measure_forest_opt(118).
	p(NT(82), (NT(118)));
//G211: __E_bool_option_44(121) => 'm' 'p'.
	p(NT(121), (T(12)+T(2)));
//G212: __E_bool_option_44(121) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'p' 'r' 'e' 'p' 'r' 'o' 'c' 'e' 's' 's'.
	p(NT(121), (T(12)+T(6)+T(3)+T(5)+T(16)+T(4)+T(6)+T(15)+T(2)+T(4)+T(6)+T(2)+T(4)+T(20)+T(17)+T(6)+T(5)+T(5)));
//G213: measure_preprocess_opt(120) => __E_bool_option_44(121).
	p(NT(120), (NT(121)));
//G214: bool_option(82)      => measure_preprocess_opt(120).
	p(NT(82), (NT(120)));
//G215: __E_bool_option_45(123) => 'd'.
	p(NT(123), (T(21)));
//G216: __E_bool_option_45(123) => 'd' 'e' 'b' 'u' 'g'.
	p(NT(123), (T(21)+T(6)+T(19)+T(16)+T(11)));
//G217: debug_opt(122)       => __E_bool_option_45(123).
	p(NT(122), (NT(123)));
//G218: bool_option(82)      => debug_opt(122).
	p(NT(82), (NT(122)));
//G219: __E_bool_option_46(125) => 'a' 'd'.
	p(NT(125), (T(3)+T(21)));
//G220: __E_bool_option_46(125) => 'a' 'u' 't' 'o' '-' 'd' 'i' 's' 'a' 'm' 'b' 'i' 'g' 'u' 'a' 't' 'e'.
	p(NT(125), (T(3)+T(16)+T(14)+T(20)+T(15)+T(21)+T(9)+T(5)+T(3)+T(12)+T(19)+T(9)+T(11)+T(16)+T(3)+T(14)+T(6)));
//G221: auto_disambiguate_opt(124) => __E_bool_option_46(125).
	p(NT(124), (NT(125)));
//G222: bool_option(82)      => auto_disambiguate_opt(124).
	p(NT(82), (NT(124)));
//G223: __E_bool_option_47(127) => 't' 't'.
	p(NT(127), (T(14)+T(14)));
//G224: __E_bool_option_47(127) => 't' 'r' 'i' 'm' '-' 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
	p(NT(127), (T(14)+T(4)+T(9)+T(12)+T(15)+T(14)+T(6)+T(4)+T(12)+T(9)+T(13)+T(3)+T(10)+T(5)));
//G225: trim_terminals_opt(126) => __E_bool_option_47(127).
	p(NT(126), (NT(127)));
//G226: bool_option(82)      => trim_terminals_opt(126).
	p(NT(82), (NT(126)));
//G227: __E_bool_option_48(129) => 'i' 'c' 'c'.
	p(NT(129), (T(9)+T(17)+T(17)));
//G228: __E_bool_option_48(129) => 'i' 'n' 'l' 'i' 'n' 'e' '-' 'c' 'c'.
	p(NT(129), (T(9)+T(13)+T(10)+T(9)+T(13)+T(6)+T(15)+T(17)+T(17)));
//G229: __E_bool_option_48(129) => 'i' 'n' 'l' 'i' 'n' 'e' '-' 'c' 'h' 'a' 'r' '-' 'c' 'l' 'a' 's' 's' 'e' 's'.
	p(NT(129), (T(9)+T(13)+T(10)+T(9)+T(13)+T(6)+T(15)+T(17)+T(18)+T(3)+T(4)+T(15)+T(17)+T(10)+T(3)+T(5)+T(5)+T(6)+T(5)));
//G230: inline_cc_opt(128)   => __E_bool_option_48(129).
	p(NT(128), (NT(129)));
//G231: bool_option(82)      => inline_cc_opt(128).
	p(NT(82), (NT(128)));
//G232: __E_list_option_49(131) => 'n' 'd'.
	p(NT(131), (T(13)+T(21)));
//G233: __E_list_option_49(131) => 'n' 'd' 'l'.
	p(NT(131), (T(13)+T(21)+T(10)));
//G234: __E_list_option_49(131) => 'n' 'o' 'd' 'i' 's' 'a' 'm' 'b' 'i' 'g' '-' 'l' 'i' 's' 't'.
	p(NT(131), (T(13)+T(20)+T(21)+T(9)+T(5)+T(3)+T(12)+T(19)+T(9)+T(11)+T(15)+T(10)+T(9)+T(5)+T(14)));
//G235: nodisambig_list_opt(130) => __E_list_option_49(131).
	p(NT(130), (NT(131)));
//G236: list_option(73)      => nodisambig_list_opt(130).
	p(NT(73), (NT(130)));
//G237: __E_list_option_50(133) => 'g' 'u' 'a' 'r' 'd' 's'.
	p(NT(133), (T(11)+T(16)+T(3)+T(4)+T(21)+T(5)));
//G238: __E_list_option_50(133) => 'e' 'n' 'a' 'b' 'l' 'e' 'd' '-' 'p' 'r' 'o' 'd' 'u' 'c' 't' 'i' 'o' 'n' 's'.
	p(NT(133), (T(6)+T(13)+T(3)+T(19)+T(10)+T(6)+T(21)+T(15)+T(2)+T(4)+T(20)+T(21)+T(16)+T(17)+T(14)+T(9)+T(20)+T(13)+T(5)));
//G239: enabled_prods_opt(132) => __E_list_option_50(133).
	p(NT(132), (NT(133)));
//G240: list_option(73)      => enabled_prods_opt(132).
	p(NT(73), (NT(132)));
//G241: trim_opt(134)        => 't' 'r' 'i' 'm'.
	p(NT(134), (T(14)+T(4)+T(9)+T(12)));
//G242: list_option(73)      => trim_opt(134).
	p(NT(73), (NT(134)));
//G243: __E_list_option_51(136) => 't' 'c'.
	p(NT(136), (T(14)+T(17)));
//G244: __E_list_option_51(136) => 't' 'r' 'i' 'm' '-' 'c' 'h' 'i' 'l' 'd' 'r' 'e' 'n'.
	p(NT(136), (T(14)+T(4)+T(9)+T(12)+T(15)+T(17)+T(18)+T(9)+T(10)+T(21)+T(4)+T(6)+T(13)));
//G245: trim_children_opt(135) => __E_list_option_51(136).
	p(NT(135), (NT(136)));
//G246: list_option(73)      => trim_children_opt(135).
	p(NT(73), (NT(135)));
//G247: __E_list_option_52(138) => 't' 'c' 't'.
	p(NT(138), (T(14)+T(17)+T(14)));
//G248: __E_list_option_52(138) => 't' 'r' 'i' 'm' '-' 'c' 'h' 'i' 'l' 'd' 'r' 'e' 'n' '-' 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
	p(NT(138), (T(14)+T(4)+T(9)+T(12)+T(15)+T(17)+T(18)+T(9)+T(10)+T(21)+T(4)+T(6)+T(13)+T(15)+T(14)+T(6)+T(4)+T(12)+T(9)+T(13)+T(3)+T(10)+T(5)));
//G249: trim_children_terminals_opt(137) => __E_list_option_52(138).
	p(NT(137), (NT(138)));
//G250: list_option(73)      => trim_children_terminals_opt(137).
	p(NT(73), (NT(137)));
//G251: __E_treepaths_option_53(140) => 'i'.
	p(NT(140), (T(9)));
//G252: __E_treepaths_option_53(140) => 'i' 'n' 'l' 'i' 'n' 'e'.
	p(NT(140), (T(9)+T(13)+T(10)+T(9)+T(13)+T(6)));
//G253: inline_opt(139)      => __E_treepaths_option_53(140).
	p(NT(139), (NT(140)));
//G254: treepaths_option(75) => inline_opt(139).
	p(NT(75), (NT(139)));
//G255: __E_bool_value_54(142) => 't'.
	p(NT(142), (T(14)));
//G256: __E_bool_value_54(142) => 't' 'r' 'u' 'e'.
	p(NT(142), (T(14)+T(4)+T(16)+T(6)));
//G257: __E_bool_value_54(142) => 'o' 'n'.
	p(NT(142), (T(20)+T(13)));
//G258: __E_bool_value_54(142) => '1'.
	p(NT(142), (T(28)));
//G259: __E_bool_value_54(142) => 'y'.
	p(NT(142), (T(27)));
//G260: __E_bool_value_54(142) => 'y' 'e' 's'.
	p(NT(142), (T(27)+T(6)+T(5)));
//G261: true_value(141)      => __E_bool_value_54(142).
	p(NT(141), (NT(142)));
//G262: bool_value(90)       => true_value(141).
	p(NT(90), (NT(141)));
//G263: __E_bool_value_55(144) => 'f'.
	p(NT(144), (T(7)));
//G264: __E_bool_value_55(144) => 'f' 'a' 'l' 's' 'e'.
	p(NT(144), (T(7)+T(3)+T(10)+T(5)+T(6)));
//G265: __E_bool_value_55(144) => 'o' 'f' 'f'.
	p(NT(144), (T(20)+T(7)+T(7)));
//G266: __E_bool_value_55(144) => '0'.
	p(NT(144), (T(29)));
//G267: __E_bool_value_55(144) => 'n'.
	p(NT(144), (T(13)));
//G268: __E_bool_value_55(144) => 'n' 'o'.
	p(NT(144), (T(13)+T(20)));
//G269: false_value(143)     => __E_bool_value_55(144).
	p(NT(143), (NT(144)));
//G270: bool_value(90)       => false_value(143).
	p(NT(90), (NT(143)));
//G271: __E_error_verbosity_56(146) => 'b'.
	p(NT(146), (T(19)));
//G272: __E_error_verbosity_56(146) => 'b' 'a' 's' 'i' 'c'.
	p(NT(146), (T(19)+T(3)+T(5)+T(9)+T(17)));
//G273: basic_sym(145)       => __E_error_verbosity_56(146).
	p(NT(145), (NT(146)));
//G274: error_verbosity(95)  => basic_sym(145).
	p(NT(95), (NT(145)));
//G275: __E_error_verbosity_57(148) => 'd'.
	p(NT(148), (T(21)));
//G276: __E_error_verbosity_57(148) => 'd' 'e' 't' 'a' 'i' 'l' 'e' 'd'.
	p(NT(148), (T(21)+T(6)+T(14)+T(3)+T(9)+T(10)+T(6)+T(21)));
//G277: detailed_sym(147)    => __E_error_verbosity_57(148).
	p(NT(147), (NT(148)));
//G278: error_verbosity(95)  => detailed_sym(147).
	p(NT(95), (NT(147)));
//G279: __E_error_verbosity_58(150) => 'r'.
	p(NT(150), (T(4)));
//G280: __E_error_verbosity_58(150) => 'r' 'c'.
	p(NT(150), (T(4)+T(17)));
//G281: __E_error_verbosity_58(150) => 'r' 'o' 'o' 't' '-' 'c' 'a' 'u' 's' 'e'.
	p(NT(150), (T(4)+T(20)+T(20)+T(14)+T(15)+T(17)+T(3)+T(16)+T(5)+T(6)));
//G282: root_cause_sym(149)  => __E_error_verbosity_58(150).
	p(NT(149), (NT(150)));
//G283: error_verbosity(95)  => root_cause_sym(149).
	p(NT(95), (NT(149)));
//G284: __E_symbol_59(151)   => alpha(3).
	p(NT(151), (NT(3)));
//G285: __E_symbol_59(151)   => '_'.
	p(NT(151), (T(30)));
//G286: __E_symbol_60(152)   => alnum(2).
	p(NT(152), (NT(2)));
//G287: __E_symbol_60(152)   => '_'.
	p(NT(152), (T(30)));
//G288: __E_symbol_61(153)   => null.
	p(NT(153), (nul));
//G289: __E_symbol_61(153)   => __E_symbol_60(152) __E_symbol_61(153).
	p(NT(153), (NT(152)+NT(153)));
//G290: symbol(8)            => __E_symbol_59(151) __E_symbol_61(153).
	p(NT(8), (NT(151)+NT(153)));
//G291: __E_symbol_list_62(154) => _(6) ',' _(6) symbol(8).
	p(NT(154), (NT(6)+T(31)+NT(6)+NT(8)));
//G292: __E_symbol_list_63(155) => null.
	p(NT(155), (nul));
//G293: __E_symbol_list_63(155) => __E_symbol_list_62(154) __E_symbol_list_63(155).
	p(NT(155), (NT(154)+NT(155)));
//G294: symbol_list(74)      => symbol(8) __E_symbol_list_63(155).
	p(NT(74), (NT(8)+NT(155)));
//G295: __E_treepath_64(157) => _(6) '>' _(6) symbol(8).
	p(NT(157), (NT(6)+T(32)+NT(6)+NT(8)));
//G296: __E_treepath_65(158) => null.
	p(NT(158), (nul));
//G297: __E_treepath_65(158) => __E_treepath_64(157) __E_treepath_65(158).
	p(NT(158), (NT(157)+NT(158)));
//G298: treepath(156)        => symbol(8) __E_treepath_65(158).
	p(NT(156), (NT(8)+NT(158)));
//G299: __E_treepath_list_66(159) => _(6) ',' _(6) treepath(156).
	p(NT(159), (NT(6)+T(31)+NT(6)+NT(156)));
//G300: __E_treepath_list_67(160) => null.
	p(NT(160), (nul));
//G301: __E_treepath_list_67(160) => __E_treepath_list_66(159) __E_treepath_list_67(160).
	p(NT(160), (NT(159)+NT(160)));
//G302: treepath_list(76)    => treepath(156) __E_treepath_list_67(160).
	p(NT(76), (NT(156)+NT(160)));
//G303: filename(42)         => quoted_string(9).
	p(NT(42), (NT(9)));
//G304: __E_quoted_string_68(162) => null.
	p(NT(162), (nul));
//G305: __E_quoted_string_68(162) => quoted_string_char(161) __E_quoted_string_68(162).
	p(NT(162), (NT(161)+NT(162)));
//G306: quoted_string(9)     => '"' __E_quoted_string_68(162) '"'.
	p(NT(9), (T(33)+NT(162)+T(33)));
//G307: quoted_string_char(161) => unescaped_s(10).
	p(NT(161), (NT(10)));
//G308: quoted_string_char(161) => escaped_s(11).
	p(NT(161), (NT(11)));
//G309: __E_unescaped_s_69(163) => space(4).
	p(NT(163), (NT(4)));
//G310: __E_unescaped_s_69(163) => printable(5).
	p(NT(163), (NT(5)));
//G311: __E_unescaped_s_70(164) => '"'.
	p(NT(164), (T(33)));
//G312: __E_unescaped_s_70(164) => '\\'.
	p(NT(164), (T(34)));
//G313: __N_0(172)           => __E_unescaped_s_70(164).
	p(NT(172), (NT(164)));
//G314: unescaped_s(10)      => __E_unescaped_s_69(163) & ~( __N_0(172) ).	 # conjunctive
	p(NT(10), (NT(163)) & ~(NT(172)));
//G315: __E_escaped_s_71(165) => '"'.
	p(NT(165), (T(33)));
//G316: __E_escaped_s_71(165) => '\\'.
	p(NT(165), (T(34)));
//G317: __E_escaped_s_71(165) => '/'.
	p(NT(165), (T(35)));
//G318: __E_escaped_s_71(165) => 'b'.
	p(NT(165), (T(19)));
//G319: __E_escaped_s_71(165) => 'f'.
	p(NT(165), (T(7)));
//G320: __E_escaped_s_71(165) => 'n'.
	p(NT(165), (T(13)));
//G321: __E_escaped_s_71(165) => 'r'.
	p(NT(165), (T(4)));
//G322: __E_escaped_s_71(165) => 't'.
	p(NT(165), (T(14)));
//G323: escaped_s(11)        => '\\' __E_escaped_s_71(165).
	p(NT(11), (T(34)+NT(165)));
//G324: __E___72(166)        => __(7).
	p(NT(166), (NT(7)));
//G325: __E___72(166)        => null.
	p(NT(166), (nul));
//G326: _(6)                 => __E___72(166).
	p(NT(6), (NT(166)));
//G327: __E____73(167)       => space(4).
	p(NT(167), (NT(4)));
//G328: __E____73(167)       => comment(168).
	p(NT(167), (NT(168)));
//G329: __(7)                => __E____73(167) _(6).
	p(NT(7), (NT(167)+NT(6)));
//G330: __E_comment_74(169)  => printable(5).
	p(NT(169), (NT(5)));
//G331: __E_comment_74(169)  => '\t'.
	p(NT(169), (T(26)));
//G332: __E_comment_75(170)  => null.
	p(NT(170), (nul));
//G333: __E_comment_75(170)  => __E_comment_74(169) __E_comment_75(170).
	p(NT(170), (NT(169)+NT(170)));
//G334: __E_comment_76(171)  => '\r'.
	p(NT(171), (T(36)));
//G335: __E_comment_76(171)  => '\n'.
	p(NT(171), (T(37)));
//G336: __E_comment_76(171)  => eof(1).
	p(NT(171), (NT(1)));
//G337: comment(168)         => '#' __E_comment_75(170) __E_comment_76(171).
	p(NT(168), (T(38)+NT(170)+NT(171)));
	#undef T
	#undef NT
	return loaded = true, p;
}

inline ::idni::grammar<char_type, terminal_type> grammar(
	nts, productions(), start_symbol, char_classes, grammar_options);

} // namespace tgf_repl_parser_data

struct tgf_repl_parser_nonterminals {
	enum nonterminal {
		nul, eof, alnum, alpha, space, printable, _, __, symbol, quoted_string, 
		unescaped_s, escaped_s, parse_input_char_seq, start, __E_start_0, statement, __E___E_start_0_1, __E___E_start_0_2, grammar_cmd, igrammar_cmd, 
		unreachable_cmd, reload_cmd, load_cmd, start_cmd, help, version, quit, clear, get, set, 
		toggle, enable, disable, add, del, parse_file_cmd, parse_cmd, parse_sym, __E_parse_cmd_3, parse_input, 
		parse_file_sym, __E_parse_file_cmd_4, filename, grammar_sym, __E_grammar_cmd_5, igrammar_sym, __E_igrammar_cmd_6, __E_igrammar_cmd_7, start_sym, __E_start_cmd_8, 
		__E_start_cmd_9, unreachable_sym, __E_unreachable_cmd_10, __E_unreachable_cmd_11, reload_sym, __E_reload_cmd_12, load_sym, __E_load_cmd_13, help_sym, __E_help_14, 
		__E_help_15, help_arg, version_sym, __E_version_16, quit_sym, __E_quit_17, clear_sym, __E_clear_18, get_sym, __E_get_19, 
		option, add_sym, __E_add_20, list_option, symbol_list, treepaths_option, treepath_list, del_sym, __E_del_21, __E_del_22, 
		toggle_sym, __E_toggle_23, bool_option, enable_sym, __E_enable_24, disable_sym, __E_disable_25, set_sym, __E_set_26, __E___E_set_26_27, 
		bool_value, __E___E_set_26_28, __E___E_set_26_29, enum_ev_option, __E___E_set_26_30, error_verbosity, __E_parse_input_31, __E_parse_input_32, error_verbosity_opt, __E_enum_ev_option_33, 
		status_opt, __E_bool_option_34, colors_opt, __E_bool_option_35, print_ambiguity_opt, __E_bool_option_36, print_graphs_opt, __E_bool_option_37, print_rules_opt, __E_bool_option_38, 
		print_facts_opt, __E_bool_option_39, print_terminals_opt, __E_bool_option_40, measure_parsing_opt, __E_bool_option_41, measure_each_pos_opt, __E_bool_option_42, measure_forest_opt, __E_bool_option_43, 
		measure_preprocess_opt, __E_bool_option_44, debug_opt, __E_bool_option_45, auto_disambiguate_opt, __E_bool_option_46, trim_terminals_opt, __E_bool_option_47, inline_cc_opt, __E_bool_option_48, 
		nodisambig_list_opt, __E_list_option_49, enabled_prods_opt, __E_list_option_50, trim_opt, trim_children_opt, __E_list_option_51, trim_children_terminals_opt, __E_list_option_52, inline_opt, 
		__E_treepaths_option_53, true_value, __E_bool_value_54, false_value, __E_bool_value_55, basic_sym, __E_error_verbosity_56, detailed_sym, __E_error_verbosity_57, root_cause_sym, 
		__E_error_verbosity_58, __E_symbol_59, __E_symbol_60, __E_symbol_61, __E_symbol_list_62, __E_symbol_list_63, treepath, __E_treepath_64, __E_treepath_65, __E_treepath_list_66, 
		__E_treepath_list_67, quoted_string_char, __E_quoted_string_68, __E_unescaped_s_69, __E_unescaped_s_70, __E_escaped_s_71, __E___72, __E____73, comment, __E_comment_74, 
		__E_comment_75, __E_comment_76, __N_0, 
	};
};

struct tgf_repl_parser : public idni::parser<char, char>, public tgf_repl_parser_nonterminals {
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
