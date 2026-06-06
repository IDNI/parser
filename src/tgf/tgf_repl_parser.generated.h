// This file is generated from a file src/tgf/tgf_repl.tgf by
//       https://github.com/IDNI/parser/src/tgf
//
#ifndef __TGF_REPL_PARSER_H__
#define __TGF_REPL_PARSER_H__

#include "parser.h"
#include "recoders.h"

namespace tgf_repl_parser_data {

using char_type     = char;
using terminal_type = char32_t;

inline static constexpr size_t nt_bits = 8;
inline const std::vector<std::string> symbol_names{
	"", "eof", "alnum", "alpha", "space", "printable", "xdigit", "_", "__", "symbol", 
	"quoted_string", "unescaped_s", "escaped_s", "parse_input_char_seq", "escape_char", "esc_hex", "esc_u4", "esc_U8", "start", "__E_start_0", 
	"statement", "__E___E_start_0_1", "__E___E_start_0_2", "grammar_cmd", "igrammar_cmd", "unreachable_cmd", "reload_cmd", "load_cmd", "start_cmd", "help_cmd", 
	"version_cmd", "license_cmd", "quit_cmd", "clear_cmd", "get_cmd", "set_cmd", "toggle_cmd", "enable_cmd", "disable_cmd", "add_cmd", 
	"del_cmd", "parse_file_cmd", "parse_cmd", "parse_sym", "__E_parse_cmd_3", "parse_input", "parse_file_sym", "__E_parse_file_cmd_4", "filename", "grammar_sym", 
	"__E_grammar_cmd_5", "igrammar_sym", "__E_igrammar_cmd_6", "__E_igrammar_cmd_7", "start_sym", "__E_start_cmd_8", "__E_start_cmd_9", "unreachable_sym", "__E_unreachable_cmd_10", "__E_unreachable_cmd_11", 
	"reload_sym", "__E_reload_cmd_12", "load_sym", "__E_load_cmd_13", "help_sym", "__E_help_cmd_14", "__E_help_cmd_15", "help_arg", "version_sym", "__E_version_cmd_16", 
	"license_sym", "__E_license_cmd_17", "quit_sym", "__E_quit_cmd_18", "clear_sym", "__E_clear_cmd_19", "get_sym", "__E_get_cmd_20", "option", "add_sym", 
	"__E_add_cmd_21", "list_option", "symbol_list", "treepaths_option", "treepath_list", "del_sym", "__E_del_cmd_22", "__E_del_cmd_23", "toggle_sym", "__E_toggle_cmd_24", 
	"bool_option", "enable_sym", "__E_enable_cmd_25", "disable_sym", "__E_disable_cmd_26", "set_sym", "__E_set_cmd_27", "__E___E_set_cmd_27_28", "bool_value", "__E___E_set_cmd_27_29", 
	"__E___E_set_cmd_27_30", "enum_ev_option", "__E___E_set_cmd_27_31", "error_verbosity", "__E_parse_input_32", "__E_parse_input_33", "error_verbosity_opt", "__E_enum_ev_option_34", "status_opt", "__E_bool_option_35", 
	"colors_opt", "__E_bool_option_36", "print_ambiguity_opt", "__E_bool_option_37", "print_graphs_opt", "__E_bool_option_38", "print_rules_opt", "__E_bool_option_39", "print_facts_opt", "__E_bool_option_40", 
	"print_terminals_opt", "__E_bool_option_41", "measure_parsing_opt", "__E_bool_option_42", "measure_each_pos_opt", "__E_bool_option_43", "measure_forest_opt", "__E_bool_option_44", "measure_preprocess_opt", "__E_bool_option_45", 
	"gc_opt", "__E_bool_option_46", "debug_opt", "__E_bool_option_47", "auto_disambiguate_opt", "__E_bool_option_48", "trim_terminals_opt", "__E_bool_option_49", "inline_cc_opt", "__E_bool_option_50", 
	"nodisambig_list_opt", "__E_list_option_51", "enabled_prods_opt", "__E_list_option_52", "trim_opt", "trim_children_opt", "__E_list_option_53", "trim_children_terminals_opt", "__E_list_option_54", "inline_opt", 
	"__E_treepaths_option_55", "true_value", "__E_bool_value_56", "false_value", "__E_bool_value_57", "basic_sym", "__E_error_verbosity_58", "detailed_sym", "__E_error_verbosity_59", "root_cause_sym", 
	"__E_error_verbosity_60", "__E_symbol_61", "__E_symbol_62", "__E_symbol_63", "__E_symbol_list_64", "__E_symbol_list_65", "treepath", "__E_treepath_66", "__E_treepath_67", "__E_treepath_list_68", 
	"__E_treepath_list_69", "quoted_string_char", "__E_quoted_string_70", "__E_unescaped_s_71", "__E_unescaped_s_72", "__E_escaped_s_73", "__E_esc_hex_74", "__E_esc_hex_75", "__E___76", "__E____77", 
	"comment", "__E_comment_78", "__E_comment_79", "__E_comment_80", "__N_0", 
};

inline ::idni::nonterminals<char_type, terminal_type> nts{symbol_names};

inline std::vector<terminal_type> terminals{
	U'\0', U'.', U'p', U'a', U'r', U's', U'e', U'f', U' ', 
	U'i', U'l', U'g', U'm', U'n', U't', U'-', U'u', U'c', U'h', 
	U'b', U'o', U'd', U'v', U'L', U'q', U'x', U'=', U'\t', U'y', 
	U'1', U'0', U'_', U',', U'>', U'"', U'\\', U'/', U'X', U'U', 
	U'\r', U'\n', U'#', 
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
			7, 8
		},
		.trim_terminals = true,
		.dont_trim_terminals_of = {
			9, 10, 11, 12, 13, 14, 15, 16, 17
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

inline ::idni::prods<char_type, terminal_type> start_symbol{ nts(18) };

inline idni::prods<char_type, terminal_type>& productions() {
	static bool loaded = false;
	static idni::prods<char_type, terminal_type>
		p, nul(idni::lit<char_type, terminal_type>{});
	if (loaded) return p;
	#define  T(x) (idni::prods<char_type, terminal_type>{ terminals[x] })
	#define NT(x) (idni::prods<char_type, terminal_type>{ nts(x) })
//G0:   __E___E_start_0_1(21) => _(7) '.' _(7) statement(20).
	p(NT(21), (NT(7)+T(1)+NT(7)+NT(20)));
//G1:   __E___E_start_0_2(22) => null.
	p(NT(22), (nul));
//G2:   __E___E_start_0_2(22) => __E___E_start_0_1(21) __E___E_start_0_2(22).
	p(NT(22), (NT(21)+NT(22)));
//G3:   __E_start_0(19)      => statement(20) __E___E_start_0_2(22) _(7).
	p(NT(19), (NT(20)+NT(22)+NT(7)));
//G4:   __E_start_0(19)      => null.
	p(NT(19), (nul));
//G5:   start(18)            => _(7) __E_start_0(19).
	p(NT(18), (NT(7)+NT(19)));
//G6:   statement(20)        => grammar_cmd(23).
	p(NT(20), (NT(23)));
//G7:   statement(20)        => igrammar_cmd(24).
	p(NT(20), (NT(24)));
//G8:   statement(20)        => unreachable_cmd(25).
	p(NT(20), (NT(25)));
//G9:   statement(20)        => reload_cmd(26).
	p(NT(20), (NT(26)));
//G10:  statement(20)        => load_cmd(27).
	p(NT(20), (NT(27)));
//G11:  statement(20)        => start_cmd(28).
	p(NT(20), (NT(28)));
//G12:  statement(20)        => help_cmd(29).
	p(NT(20), (NT(29)));
//G13:  statement(20)        => version_cmd(30).
	p(NT(20), (NT(30)));
//G14:  statement(20)        => license_cmd(31).
	p(NT(20), (NT(31)));
//G15:  statement(20)        => quit_cmd(32).
	p(NT(20), (NT(32)));
//G16:  statement(20)        => clear_cmd(33).
	p(NT(20), (NT(33)));
//G17:  statement(20)        => get_cmd(34).
	p(NT(20), (NT(34)));
//G18:  statement(20)        => set_cmd(35).
	p(NT(20), (NT(35)));
//G19:  statement(20)        => toggle_cmd(36).
	p(NT(20), (NT(36)));
//G20:  statement(20)        => enable_cmd(37).
	p(NT(20), (NT(37)));
//G21:  statement(20)        => disable_cmd(38).
	p(NT(20), (NT(38)));
//G22:  statement(20)        => add_cmd(39).
	p(NT(20), (NT(39)));
//G23:  statement(20)        => del_cmd(40).
	p(NT(20), (NT(40)));
//G24:  statement(20)        => parse_file_cmd(41).
	p(NT(20), (NT(41)));
//G25:  statement(20)        => parse_cmd(42).
	p(NT(20), (NT(42)));
//G26:  __E_parse_cmd_3(44)  => 'p'.
	p(NT(44), (T(2)));
//G27:  __E_parse_cmd_3(44)  => 'p' 'a' 'r' 's' 'e'.
	p(NT(44), (T(2)+T(3)+T(4)+T(5)+T(6)));
//G28:  parse_sym(43)        => __E_parse_cmd_3(44).
	p(NT(43), (NT(44)));
//G29:  parse_cmd(42)        => parse_sym(43) __(8) parse_input(45).
	p(NT(42), (NT(43)+NT(8)+NT(45)));
//G30:  __E_parse_file_cmd_4(47) => 'f'.
	p(NT(47), (T(7)));
//G31:  __E_parse_file_cmd_4(47) => 'p' 'f'.
	p(NT(47), (T(2)+T(7)));
//G32:  __E_parse_file_cmd_4(47) => 'p' 'a' 'r' 's' 'e' ' ' 'f' 'i' 'l' 'e'.
	p(NT(47), (T(2)+T(3)+T(4)+T(5)+T(6)+T(8)+T(7)+T(9)+T(10)+T(6)));
//G33:  parse_file_sym(46)   => __E_parse_file_cmd_4(47).
	p(NT(46), (NT(47)));
//G34:  parse_file_cmd(41)   => parse_file_sym(46) __(8) filename(48).
	p(NT(41), (NT(46)+NT(8)+NT(48)));
//G35:  __E_grammar_cmd_5(50) => 'g'.
	p(NT(50), (T(11)));
//G36:  __E_grammar_cmd_5(50) => 'g' 'r' 'a' 'm' 'm' 'a' 'r'.
	p(NT(50), (T(11)+T(4)+T(3)+T(12)+T(12)+T(3)+T(4)));
//G37:  grammar_sym(49)      => __E_grammar_cmd_5(50).
	p(NT(49), (NT(50)));
//G38:  grammar_cmd(23)      => grammar_sym(49).
	p(NT(23), (NT(49)));
//G39:  __E_igrammar_cmd_6(52) => 'i'.
	p(NT(52), (T(9)));
//G40:  __E_igrammar_cmd_6(52) => 'i' 'g'.
	p(NT(52), (T(9)+T(11)));
//G41:  __E_igrammar_cmd_6(52) => 'i' 'n' 't' 'e' 'r' 'n' 'a' 'l' '-' 'g' 'r' 'a' 'm' 'm' 'a' 'r'.
	p(NT(52), (T(9)+T(13)+T(14)+T(6)+T(4)+T(13)+T(3)+T(10)+T(15)+T(11)+T(4)+T(3)+T(12)+T(12)+T(3)+T(4)));
//G42:  igrammar_sym(51)     => __E_igrammar_cmd_6(52).
	p(NT(51), (NT(52)));
//G43:  __E_igrammar_cmd_7(53) => __(8) symbol(9).
	p(NT(53), (NT(8)+NT(9)));
//G44:  __E_igrammar_cmd_7(53) => null.
	p(NT(53), (nul));
//G45:  igrammar_cmd(24)     => igrammar_sym(51) __E_igrammar_cmd_7(53).
	p(NT(24), (NT(51)+NT(53)));
//G46:  __E_start_cmd_8(55)  => 's'.
	p(NT(55), (T(5)));
//G47:  __E_start_cmd_8(55)  => 's' 't' 'a' 'r' 't'.
	p(NT(55), (T(5)+T(14)+T(3)+T(4)+T(14)));
//G48:  start_sym(54)        => __E_start_cmd_8(55).
	p(NT(54), (NT(55)));
//G49:  __E_start_cmd_9(56)  => __(8) symbol(9).
	p(NT(56), (NT(8)+NT(9)));
//G50:  __E_start_cmd_9(56)  => null.
	p(NT(56), (nul));
//G51:  start_cmd(28)        => start_sym(54) __E_start_cmd_9(56).
	p(NT(28), (NT(54)+NT(56)));
//G52:  __E_unreachable_cmd_10(58) => 'u'.
	p(NT(58), (T(16)));
//G53:  __E_unreachable_cmd_10(58) => 'u' 'n' 'r' 'e' 'a' 'c' 'h' 'a' 'b' 'l' 'e'.
	p(NT(58), (T(16)+T(13)+T(4)+T(6)+T(3)+T(17)+T(18)+T(3)+T(19)+T(10)+T(6)));
//G54:  unreachable_sym(57)  => __E_unreachable_cmd_10(58).
	p(NT(57), (NT(58)));
//G55:  __E_unreachable_cmd_11(59) => __(8) symbol(9).
	p(NT(59), (NT(8)+NT(9)));
//G56:  __E_unreachable_cmd_11(59) => null.
	p(NT(59), (nul));
//G57:  unreachable_cmd(25)  => unreachable_sym(57) __E_unreachable_cmd_11(59).
	p(NT(25), (NT(57)+NT(59)));
//G58:  __E_reload_cmd_12(61) => 'r'.
	p(NT(61), (T(4)));
//G59:  __E_reload_cmd_12(61) => 'r' 'e' 'l' 'o' 'a' 'd'.
	p(NT(61), (T(4)+T(6)+T(10)+T(20)+T(3)+T(21)));
//G60:  reload_sym(60)       => __E_reload_cmd_12(61).
	p(NT(60), (NT(61)));
//G61:  reload_cmd(26)       => reload_sym(60).
	p(NT(26), (NT(60)));
//G62:  __E_load_cmd_13(63)  => 'l'.
	p(NT(63), (T(10)));
//G63:  __E_load_cmd_13(63)  => 'l' 'o' 'a' 'd'.
	p(NT(63), (T(10)+T(20)+T(3)+T(21)));
//G64:  load_sym(62)         => __E_load_cmd_13(63).
	p(NT(62), (NT(63)));
//G65:  load_cmd(27)         => load_sym(62) __(8) filename(48).
	p(NT(27), (NT(62)+NT(8)+NT(48)));
//G66:  __E_help_cmd_14(65)  => 'h'.
	p(NT(65), (T(18)));
//G67:  __E_help_cmd_14(65)  => 'h' 'e' 'l' 'p'.
	p(NT(65), (T(18)+T(6)+T(10)+T(2)));
//G68:  help_sym(64)         => __E_help_cmd_14(65).
	p(NT(64), (NT(65)));
//G69:  __E_help_cmd_15(66)  => __(8) help_arg(67).
	p(NT(66), (NT(8)+NT(67)));
//G70:  __E_help_cmd_15(66)  => null.
	p(NT(66), (nul));
//G71:  help_cmd(29)         => help_sym(64) __E_help_cmd_15(66).
	p(NT(29), (NT(64)+NT(66)));
//G72:  __E_version_cmd_16(69) => 'v'.
	p(NT(69), (T(22)));
//G73:  __E_version_cmd_16(69) => 'v' 'e' 'r' 's' 'i' 'o' 'n'.
	p(NT(69), (T(22)+T(6)+T(4)+T(5)+T(9)+T(20)+T(13)));
//G74:  version_sym(68)      => __E_version_cmd_16(69).
	p(NT(68), (NT(69)));
//G75:  version_cmd(30)      => version_sym(68).
	p(NT(30), (NT(68)));
//G76:  __E_license_cmd_17(71) => 'L'.
	p(NT(71), (T(23)));
//G77:  __E_license_cmd_17(71) => 'l' 'i' 'c' 'e' 'n' 's' 'e'.
	p(NT(71), (T(10)+T(9)+T(17)+T(6)+T(13)+T(5)+T(6)));
//G78:  license_sym(70)      => __E_license_cmd_17(71).
	p(NT(70), (NT(71)));
//G79:  license_cmd(31)      => license_sym(70).
	p(NT(31), (NT(70)));
//G80:  __E_quit_cmd_18(73)  => 'q'.
	p(NT(73), (T(24)));
//G81:  __E_quit_cmd_18(73)  => 'q' 'u' 'i' 't'.
	p(NT(73), (T(24)+T(16)+T(9)+T(14)));
//G82:  __E_quit_cmd_18(73)  => 'e'.
	p(NT(73), (T(6)));
//G83:  __E_quit_cmd_18(73)  => 'e' 'x' 'i' 't'.
	p(NT(73), (T(6)+T(25)+T(9)+T(14)));
//G84:  quit_sym(72)         => __E_quit_cmd_18(73).
	p(NT(72), (NT(73)));
//G85:  quit_cmd(32)         => quit_sym(72).
	p(NT(32), (NT(72)));
//G86:  __E_clear_cmd_19(75) => 'c' 'l' 's'.
	p(NT(75), (T(17)+T(10)+T(5)));
//G87:  __E_clear_cmd_19(75) => 'c' 'l' 'e' 'a' 'r'.
	p(NT(75), (T(17)+T(10)+T(6)+T(3)+T(4)));
//G88:  clear_sym(74)        => __E_clear_cmd_19(75).
	p(NT(74), (NT(75)));
//G89:  clear_cmd(33)        => clear_sym(74).
	p(NT(33), (NT(74)));
//G90:  get_sym(76)          => 'g' 'e' 't'.
	p(NT(76), (T(11)+T(6)+T(14)));
//G91:  __E_get_cmd_20(77)   => __(8) option(78).
	p(NT(77), (NT(8)+NT(78)));
//G92:  __E_get_cmd_20(77)   => null.
	p(NT(77), (nul));
//G93:  get_cmd(34)          => get_sym(76) __E_get_cmd_20(77).
	p(NT(34), (NT(76)+NT(77)));
//G94:  add_sym(79)          => 'a' 'd' 'd'.
	p(NT(79), (T(3)+T(21)+T(21)));
//G95:  __E_add_cmd_21(80)   => list_option(81) __(8) symbol_list(82).
	p(NT(80), (NT(81)+NT(8)+NT(82)));
//G96:  __E_add_cmd_21(80)   => treepaths_option(83) __(8) treepath_list(84).
	p(NT(80), (NT(83)+NT(8)+NT(84)));
//G97:  add_cmd(39)          => add_sym(79) __(8) __E_add_cmd_21(80).
	p(NT(39), (NT(79)+NT(8)+NT(80)));
//G98:  __E_del_cmd_22(86)   => 'd' 'e' 'l'.
	p(NT(86), (T(21)+T(6)+T(10)));
//G99:  __E_del_cmd_22(86)   => 'd' 'e' 'l' 'e' 't' 'e'.
	p(NT(86), (T(21)+T(6)+T(10)+T(6)+T(14)+T(6)));
//G100: __E_del_cmd_22(86)   => 'r' 'm'.
	p(NT(86), (T(4)+T(12)));
//G101: __E_del_cmd_22(86)   => 'r' 'e' 'm'.
	p(NT(86), (T(4)+T(6)+T(12)));
//G102: __E_del_cmd_22(86)   => 'r' 'e' 'm' 'o' 'v' 'e'.
	p(NT(86), (T(4)+T(6)+T(12)+T(20)+T(22)+T(6)));
//G103: del_sym(85)          => __E_del_cmd_22(86).
	p(NT(85), (NT(86)));
//G104: __E_del_cmd_23(87)   => list_option(81) __(8) symbol_list(82).
	p(NT(87), (NT(81)+NT(8)+NT(82)));
//G105: __E_del_cmd_23(87)   => treepaths_option(83) __(8) treepath_list(84).
	p(NT(87), (NT(83)+NT(8)+NT(84)));
//G106: del_cmd(40)          => del_sym(85) __(8) __E_del_cmd_23(87).
	p(NT(40), (NT(85)+NT(8)+NT(87)));
//G107: __E_toggle_cmd_24(89) => 't' 'o' 'g'.
	p(NT(89), (T(14)+T(20)+T(11)));
//G108: __E_toggle_cmd_24(89) => 't' 'o' 'g' 'g' 'l' 'e'.
	p(NT(89), (T(14)+T(20)+T(11)+T(11)+T(10)+T(6)));
//G109: toggle_sym(88)       => __E_toggle_cmd_24(89).
	p(NT(88), (NT(89)));
//G110: toggle_cmd(36)       => toggle_sym(88) __(8) bool_option(90).
	p(NT(36), (NT(88)+NT(8)+NT(90)));
//G111: __E_enable_cmd_25(92) => 'e' 'n' __(8).
	p(NT(92), (T(6)+T(13)+NT(8)));
//G112: __E_enable_cmd_25(92) => 'e' 'n' 'a' 'b' 'l' 'e' __(8).
	p(NT(92), (T(6)+T(13)+T(3)+T(19)+T(10)+T(6)+NT(8)));
//G113: enable_sym(91)       => __E_enable_cmd_25(92).
	p(NT(91), (NT(92)));
//G114: enable_cmd(37)       => enable_sym(91) bool_option(90).
	p(NT(37), (NT(91)+NT(90)));
//G115: __E_disable_cmd_26(94) => 'd' 'i' 's' __(8).
	p(NT(94), (T(21)+T(9)+T(5)+NT(8)));
//G116: __E_disable_cmd_26(94) => 'd' 'i' 's' 'a' 'b' 'l' 'e' __(8).
	p(NT(94), (T(21)+T(9)+T(5)+T(3)+T(19)+T(10)+T(6)+NT(8)));
//G117: disable_sym(93)      => __E_disable_cmd_26(94).
	p(NT(93), (NT(94)));
//G118: disable_cmd(38)      => disable_sym(93) bool_option(90).
	p(NT(38), (NT(93)+NT(90)));
//G119: set_sym(95)          => 's' 'e' 't'.
	p(NT(95), (T(5)+T(6)+T(14)));
//G120: __E___E_set_cmd_27_28(97) => __(8).
	p(NT(97), (NT(8)));
//G121: __E___E_set_cmd_27_28(97) => _(7) '=' _(7).
	p(NT(97), (NT(7)+T(26)+NT(7)));
//G122: __E_set_cmd_27(96)   => bool_option(90) __E___E_set_cmd_27_28(97) bool_value(98).
	p(NT(96), (NT(90)+NT(97)+NT(98)));
//G123: __E___E_set_cmd_27_29(99) => __(8).
	p(NT(99), (NT(8)));
//G124: __E___E_set_cmd_27_29(99) => _(7) '=' _(7).
	p(NT(99), (NT(7)+T(26)+NT(7)));
//G125: __E_set_cmd_27(96)   => list_option(81) __E___E_set_cmd_27_29(99) symbol_list(82).
	p(NT(96), (NT(81)+NT(99)+NT(82)));
//G126: __E___E_set_cmd_27_30(100) => __(8).
	p(NT(100), (NT(8)));
//G127: __E___E_set_cmd_27_30(100) => _(7) '=' _(7).
	p(NT(100), (NT(7)+T(26)+NT(7)));
//G128: __E_set_cmd_27(96)   => treepaths_option(83) __E___E_set_cmd_27_30(100) treepath_list(84).
	p(NT(96), (NT(83)+NT(100)+NT(84)));
//G129: __E___E_set_cmd_27_31(102) => __(8).
	p(NT(102), (NT(8)));
//G130: __E___E_set_cmd_27_31(102) => _(7) '=' _(7).
	p(NT(102), (NT(7)+T(26)+NT(7)));
//G131: __E_set_cmd_27(96)   => enum_ev_option(101) __E___E_set_cmd_27_31(102) error_verbosity(103).
	p(NT(96), (NT(101)+NT(102)+NT(103)));
//G132: set_cmd(35)          => set_sym(95) __(8) __E_set_cmd_27(96).
	p(NT(35), (NT(95)+NT(8)+NT(96)));
//G133: parse_input(45)      => quoted_string(10).
	p(NT(45), (NT(10)));
//G134: __E_parse_input_32(104) => printable(5).
	p(NT(104), (NT(5)));
//G135: __E_parse_input_32(104) => '\t'.
	p(NT(104), (T(27)));
//G136: __E_parse_input_33(105) => __E_parse_input_32(104).
	p(NT(105), (NT(104)));
//G137: __E_parse_input_33(105) => __E_parse_input_32(104) __E_parse_input_33(105).
	p(NT(105), (NT(104)+NT(105)));
//G138: parse_input_char_seq(13) => __E_parse_input_33(105).
	p(NT(13), (NT(105)));
//G139: parse_input(45)      => parse_input_char_seq(13).
	p(NT(45), (NT(13)));
//G140: help_arg(67)         => grammar_sym(49).
	p(NT(67), (NT(49)));
//G141: help_arg(67)         => igrammar_sym(51).
	p(NT(67), (NT(51)));
//G142: help_arg(67)         => unreachable_sym(57).
	p(NT(67), (NT(57)));
//G143: help_arg(67)         => start_sym(54).
	p(NT(67), (NT(54)));
//G144: help_arg(67)         => parse_sym(43).
	p(NT(67), (NT(43)));
//G145: help_arg(67)         => parse_file_sym(46).
	p(NT(67), (NT(46)));
//G146: help_arg(67)         => load_sym(62).
	p(NT(67), (NT(62)));
//G147: help_arg(67)         => reload_sym(60).
	p(NT(67), (NT(60)));
//G148: help_arg(67)         => clear_sym(74).
	p(NT(67), (NT(74)));
//G149: help_arg(67)         => help_sym(64).
	p(NT(67), (NT(64)));
//G150: help_arg(67)         => quit_sym(72).
	p(NT(67), (NT(72)));
//G151: help_arg(67)         => version_sym(68).
	p(NT(67), (NT(68)));
//G152: help_arg(67)         => license_sym(70).
	p(NT(67), (NT(70)));
//G153: help_arg(67)         => get_sym(76).
	p(NT(67), (NT(76)));
//G154: help_arg(67)         => set_sym(95).
	p(NT(67), (NT(95)));
//G155: help_arg(67)         => add_sym(79).
	p(NT(67), (NT(79)));
//G156: help_arg(67)         => del_sym(85).
	p(NT(67), (NT(85)));
//G157: help_arg(67)         => toggle_sym(88).
	p(NT(67), (NT(88)));
//G158: help_arg(67)         => enable_sym(91).
	p(NT(67), (NT(91)));
//G159: help_arg(67)         => disable_sym(93).
	p(NT(67), (NT(93)));
//G160: option(78)           => bool_option(90).
	p(NT(78), (NT(90)));
//G161: option(78)           => enum_ev_option(101).
	p(NT(78), (NT(101)));
//G162: option(78)           => list_option(81).
	p(NT(78), (NT(81)));
//G163: option(78)           => treepaths_option(83).
	p(NT(78), (NT(83)));
//G164: __E_enum_ev_option_34(107) => 'e'.
	p(NT(107), (T(6)));
//G165: __E_enum_ev_option_34(107) => 'e' 'r' 'r' 'o' 'r' '-' 'v' 'e' 'r' 'b' 'o' 's' 'i' 't' 'y'.
	p(NT(107), (T(6)+T(4)+T(4)+T(20)+T(4)+T(15)+T(22)+T(6)+T(4)+T(19)+T(20)+T(5)+T(9)+T(14)+T(28)));
//G166: error_verbosity_opt(106) => __E_enum_ev_option_34(107).
	p(NT(106), (NT(107)));
//G167: enum_ev_option(101)  => error_verbosity_opt(106).
	p(NT(101), (NT(106)));
//G168: __E_bool_option_35(109) => 's'.
	p(NT(109), (T(5)));
//G169: __E_bool_option_35(109) => 's' 't' 'a' 't' 'u' 's'.
	p(NT(109), (T(5)+T(14)+T(3)+T(14)+T(16)+T(5)));
//G170: status_opt(108)      => __E_bool_option_35(109).
	p(NT(108), (NT(109)));
//G171: bool_option(90)      => status_opt(108).
	p(NT(90), (NT(108)));
//G172: __E_bool_option_36(111) => 'c'.
	p(NT(111), (T(17)));
//G173: __E_bool_option_36(111) => 'c' 'o' 'l' 'o' 'r'.
	p(NT(111), (T(17)+T(20)+T(10)+T(20)+T(4)));
//G174: __E_bool_option_36(111) => 'c' 'o' 'l' 'o' 'r' 's'.
	p(NT(111), (T(17)+T(20)+T(10)+T(20)+T(4)+T(5)));
//G175: colors_opt(110)      => __E_bool_option_36(111).
	p(NT(110), (NT(111)));
//G176: bool_option(90)      => colors_opt(110).
	p(NT(90), (NT(110)));
//G177: __E_bool_option_37(113) => 'a'.
	p(NT(113), (T(3)));
//G178: __E_bool_option_37(113) => 'a' 'm' 'b' 'i' 'g' 'u' 'i' 't' 'y'.
	p(NT(113), (T(3)+T(12)+T(19)+T(9)+T(11)+T(16)+T(9)+T(14)+T(28)));
//G179: __E_bool_option_37(113) => 'p' 'r' 'i' 'n' 't' '-' 'a' 'm' 'b' 'i' 'g' 'u' 'i' 't' 'y'.
	p(NT(113), (T(2)+T(4)+T(9)+T(13)+T(14)+T(15)+T(3)+T(12)+T(19)+T(9)+T(11)+T(16)+T(9)+T(14)+T(28)));
//G180: print_ambiguity_opt(112) => __E_bool_option_37(113).
	p(NT(112), (NT(113)));
//G181: bool_option(90)      => print_ambiguity_opt(112).
	p(NT(90), (NT(112)));
//G182: __E_bool_option_38(115) => 'g'.
	p(NT(115), (T(11)));
//G183: __E_bool_option_38(115) => 'g' 'r' 'a' 'p' 'h' 's'.
	p(NT(115), (T(11)+T(4)+T(3)+T(2)+T(18)+T(5)));
//G184: __E_bool_option_38(115) => 'p' 'r' 'i' 'n' 't' '-' 'g' 'r' 'a' 'p' 'h' 's'.
	p(NT(115), (T(2)+T(4)+T(9)+T(13)+T(14)+T(15)+T(11)+T(4)+T(3)+T(2)+T(18)+T(5)));
//G185: print_graphs_opt(114) => __E_bool_option_38(115).
	p(NT(114), (NT(115)));
//G186: bool_option(90)      => print_graphs_opt(114).
	p(NT(90), (NT(114)));
//G187: __E_bool_option_39(117) => 'r'.
	p(NT(117), (T(4)));
//G188: __E_bool_option_39(117) => 'r' 'u' 'l' 'e' 's'.
	p(NT(117), (T(4)+T(16)+T(10)+T(6)+T(5)));
//G189: __E_bool_option_39(117) => 'p' 'r' 'i' 'n' 't' '-' 'r' 'u' 'l' 'e' 's'.
	p(NT(117), (T(2)+T(4)+T(9)+T(13)+T(14)+T(15)+T(4)+T(16)+T(10)+T(6)+T(5)));
//G190: print_rules_opt(116) => __E_bool_option_39(117).
	p(NT(116), (NT(117)));
//G191: bool_option(90)      => print_rules_opt(116).
	p(NT(90), (NT(116)));
//G192: __E_bool_option_40(119) => 'f'.
	p(NT(119), (T(7)));
//G193: __E_bool_option_40(119) => 'f' 'a' 'c' 't' 's'.
	p(NT(119), (T(7)+T(3)+T(17)+T(14)+T(5)));
//G194: __E_bool_option_40(119) => 'p' 'r' 'i' 'n' 't' '-' 'f' 'a' 'c' 't' 's'.
	p(NT(119), (T(2)+T(4)+T(9)+T(13)+T(14)+T(15)+T(7)+T(3)+T(17)+T(14)+T(5)));
//G195: print_facts_opt(118) => __E_bool_option_40(119).
	p(NT(118), (NT(119)));
//G196: bool_option(90)      => print_facts_opt(118).
	p(NT(90), (NT(118)));
//G197: __E_bool_option_41(121) => 't'.
	p(NT(121), (T(14)));
//G198: __E_bool_option_41(121) => 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
	p(NT(121), (T(14)+T(6)+T(4)+T(12)+T(9)+T(13)+T(3)+T(10)+T(5)));
//G199: __E_bool_option_41(121) => 'p' 'r' 'i' 'n' 't' '-' 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
	p(NT(121), (T(2)+T(4)+T(9)+T(13)+T(14)+T(15)+T(14)+T(6)+T(4)+T(12)+T(9)+T(13)+T(3)+T(10)+T(5)));
//G200: print_terminals_opt(120) => __E_bool_option_41(121).
	p(NT(120), (NT(121)));
//G201: bool_option(90)      => print_terminals_opt(120).
	p(NT(90), (NT(120)));
//G202: __E_bool_option_42(123) => 'm'.
	p(NT(123), (T(12)));
//G203: __E_bool_option_42(123) => 'm' 'e' 'a' 's' 'u' 'r' 'e'.
	p(NT(123), (T(12)+T(6)+T(3)+T(5)+T(16)+T(4)+T(6)));
//G204: __E_bool_option_42(123) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'p' 'a' 'r' 's' 'i' 'n' 'g'.
	p(NT(123), (T(12)+T(6)+T(3)+T(5)+T(16)+T(4)+T(6)+T(15)+T(2)+T(3)+T(4)+T(5)+T(9)+T(13)+T(11)));
//G205: measure_parsing_opt(122) => __E_bool_option_42(123).
	p(NT(122), (NT(123)));
//G206: bool_option(90)      => measure_parsing_opt(122).
	p(NT(90), (NT(122)));
//G207: __E_bool_option_43(125) => 'm' 'e'.
	p(NT(125), (T(12)+T(6)));
//G208: __E_bool_option_43(125) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'e' 'a' 'c' 'h'.
	p(NT(125), (T(12)+T(6)+T(3)+T(5)+T(16)+T(4)+T(6)+T(15)+T(6)+T(3)+T(17)+T(18)));
//G209: __E_bool_option_43(125) => 'm' 'e' 'p'.
	p(NT(125), (T(12)+T(6)+T(2)));
//G210: __E_bool_option_43(125) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'e' 'a' 'c' 'h' '-' 'p' 'o' 's'.
	p(NT(125), (T(12)+T(6)+T(3)+T(5)+T(16)+T(4)+T(6)+T(15)+T(6)+T(3)+T(17)+T(18)+T(15)+T(2)+T(20)+T(5)));
//G211: measure_each_pos_opt(124) => __E_bool_option_43(125).
	p(NT(124), (NT(125)));
//G212: bool_option(90)      => measure_each_pos_opt(124).
	p(NT(90), (NT(124)));
//G213: __E_bool_option_44(127) => 'm' 'f'.
	p(NT(127), (T(12)+T(7)));
//G214: __E_bool_option_44(127) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'f' 'o' 'r' 'e' 's' 't'.
	p(NT(127), (T(12)+T(6)+T(3)+T(5)+T(16)+T(4)+T(6)+T(15)+T(7)+T(20)+T(4)+T(6)+T(5)+T(14)));
//G215: measure_forest_opt(126) => __E_bool_option_44(127).
	p(NT(126), (NT(127)));
//G216: bool_option(90)      => measure_forest_opt(126).
	p(NT(90), (NT(126)));
//G217: __E_bool_option_45(129) => 'm' 'p'.
	p(NT(129), (T(12)+T(2)));
//G218: __E_bool_option_45(129) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'p' 'r' 'e' 'p' 'r' 'o' 'c' 'e' 's' 's'.
	p(NT(129), (T(12)+T(6)+T(3)+T(5)+T(16)+T(4)+T(6)+T(15)+T(2)+T(4)+T(6)+T(2)+T(4)+T(20)+T(17)+T(6)+T(5)+T(5)));
//G219: measure_preprocess_opt(128) => __E_bool_option_45(129).
	p(NT(128), (NT(129)));
//G220: bool_option(90)      => measure_preprocess_opt(128).
	p(NT(90), (NT(128)));
//G221: __E_bool_option_46(131) => 'g' 'c'.
	p(NT(131), (T(11)+T(17)));
//G222: gc_opt(130)          => __E_bool_option_46(131).
	p(NT(130), (NT(131)));
//G223: bool_option(90)      => gc_opt(130).
	p(NT(90), (NT(130)));
//G224: __E_bool_option_47(133) => 'd'.
	p(NT(133), (T(21)));
//G225: __E_bool_option_47(133) => 'd' 'e' 'b' 'u' 'g'.
	p(NT(133), (T(21)+T(6)+T(19)+T(16)+T(11)));
//G226: debug_opt(132)       => __E_bool_option_47(133).
	p(NT(132), (NT(133)));
//G227: bool_option(90)      => debug_opt(132).
	p(NT(90), (NT(132)));
//G228: __E_bool_option_48(135) => 'a' 'd'.
	p(NT(135), (T(3)+T(21)));
//G229: __E_bool_option_48(135) => 'a' 'u' 't' 'o' '-' 'd' 'i' 's' 'a' 'm' 'b' 'i' 'g' 'u' 'a' 't' 'e'.
	p(NT(135), (T(3)+T(16)+T(14)+T(20)+T(15)+T(21)+T(9)+T(5)+T(3)+T(12)+T(19)+T(9)+T(11)+T(16)+T(3)+T(14)+T(6)));
//G230: auto_disambiguate_opt(134) => __E_bool_option_48(135).
	p(NT(134), (NT(135)));
//G231: bool_option(90)      => auto_disambiguate_opt(134).
	p(NT(90), (NT(134)));
//G232: __E_bool_option_49(137) => 't' 't'.
	p(NT(137), (T(14)+T(14)));
//G233: __E_bool_option_49(137) => 't' 'r' 'i' 'm' '-' 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
	p(NT(137), (T(14)+T(4)+T(9)+T(12)+T(15)+T(14)+T(6)+T(4)+T(12)+T(9)+T(13)+T(3)+T(10)+T(5)));
//G234: trim_terminals_opt(136) => __E_bool_option_49(137).
	p(NT(136), (NT(137)));
//G235: bool_option(90)      => trim_terminals_opt(136).
	p(NT(90), (NT(136)));
//G236: __E_bool_option_50(139) => 'i' 'c' 'c'.
	p(NT(139), (T(9)+T(17)+T(17)));
//G237: __E_bool_option_50(139) => 'i' 'n' 'l' 'i' 'n' 'e' '-' 'c' 'c'.
	p(NT(139), (T(9)+T(13)+T(10)+T(9)+T(13)+T(6)+T(15)+T(17)+T(17)));
//G238: __E_bool_option_50(139) => 'i' 'n' 'l' 'i' 'n' 'e' '-' 'c' 'h' 'a' 'r' '-' 'c' 'l' 'a' 's' 's' 'e' 's'.
	p(NT(139), (T(9)+T(13)+T(10)+T(9)+T(13)+T(6)+T(15)+T(17)+T(18)+T(3)+T(4)+T(15)+T(17)+T(10)+T(3)+T(5)+T(5)+T(6)+T(5)));
//G239: inline_cc_opt(138)   => __E_bool_option_50(139).
	p(NT(138), (NT(139)));
//G240: bool_option(90)      => inline_cc_opt(138).
	p(NT(90), (NT(138)));
//G241: __E_list_option_51(141) => 'n' 'd'.
	p(NT(141), (T(13)+T(21)));
//G242: __E_list_option_51(141) => 'n' 'd' 'l'.
	p(NT(141), (T(13)+T(21)+T(10)));
//G243: __E_list_option_51(141) => 'n' 'o' 'd' 'i' 's' 'a' 'm' 'b' 'i' 'g' '-' 'l' 'i' 's' 't'.
	p(NT(141), (T(13)+T(20)+T(21)+T(9)+T(5)+T(3)+T(12)+T(19)+T(9)+T(11)+T(15)+T(10)+T(9)+T(5)+T(14)));
//G244: nodisambig_list_opt(140) => __E_list_option_51(141).
	p(NT(140), (NT(141)));
//G245: list_option(81)      => nodisambig_list_opt(140).
	p(NT(81), (NT(140)));
//G246: __E_list_option_52(143) => 'g' 'u' 'a' 'r' 'd' 's'.
	p(NT(143), (T(11)+T(16)+T(3)+T(4)+T(21)+T(5)));
//G247: __E_list_option_52(143) => 'e' 'n' 'a' 'b' 'l' 'e' 'd' '-' 'p' 'r' 'o' 'd' 'u' 'c' 't' 'i' 'o' 'n' 's'.
	p(NT(143), (T(6)+T(13)+T(3)+T(19)+T(10)+T(6)+T(21)+T(15)+T(2)+T(4)+T(20)+T(21)+T(16)+T(17)+T(14)+T(9)+T(20)+T(13)+T(5)));
//G248: enabled_prods_opt(142) => __E_list_option_52(143).
	p(NT(142), (NT(143)));
//G249: list_option(81)      => enabled_prods_opt(142).
	p(NT(81), (NT(142)));
//G250: trim_opt(144)        => 't' 'r' 'i' 'm'.
	p(NT(144), (T(14)+T(4)+T(9)+T(12)));
//G251: list_option(81)      => trim_opt(144).
	p(NT(81), (NT(144)));
//G252: __E_list_option_53(146) => 't' 'c'.
	p(NT(146), (T(14)+T(17)));
//G253: __E_list_option_53(146) => 't' 'r' 'i' 'm' '-' 'c' 'h' 'i' 'l' 'd' 'r' 'e' 'n'.
	p(NT(146), (T(14)+T(4)+T(9)+T(12)+T(15)+T(17)+T(18)+T(9)+T(10)+T(21)+T(4)+T(6)+T(13)));
//G254: trim_children_opt(145) => __E_list_option_53(146).
	p(NT(145), (NT(146)));
//G255: list_option(81)      => trim_children_opt(145).
	p(NT(81), (NT(145)));
//G256: __E_list_option_54(148) => 't' 'c' 't'.
	p(NT(148), (T(14)+T(17)+T(14)));
//G257: __E_list_option_54(148) => 't' 'r' 'i' 'm' '-' 'c' 'h' 'i' 'l' 'd' 'r' 'e' 'n' '-' 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
	p(NT(148), (T(14)+T(4)+T(9)+T(12)+T(15)+T(17)+T(18)+T(9)+T(10)+T(21)+T(4)+T(6)+T(13)+T(15)+T(14)+T(6)+T(4)+T(12)+T(9)+T(13)+T(3)+T(10)+T(5)));
//G258: trim_children_terminals_opt(147) => __E_list_option_54(148).
	p(NT(147), (NT(148)));
//G259: list_option(81)      => trim_children_terminals_opt(147).
	p(NT(81), (NT(147)));
//G260: __E_treepaths_option_55(150) => 'i'.
	p(NT(150), (T(9)));
//G261: __E_treepaths_option_55(150) => 'i' 'n' 'l' 'i' 'n' 'e'.
	p(NT(150), (T(9)+T(13)+T(10)+T(9)+T(13)+T(6)));
//G262: inline_opt(149)      => __E_treepaths_option_55(150).
	p(NT(149), (NT(150)));
//G263: treepaths_option(83) => inline_opt(149).
	p(NT(83), (NT(149)));
//G264: __E_bool_value_56(152) => 't'.
	p(NT(152), (T(14)));
//G265: __E_bool_value_56(152) => 't' 'r' 'u' 'e'.
	p(NT(152), (T(14)+T(4)+T(16)+T(6)));
//G266: __E_bool_value_56(152) => 'o' 'n'.
	p(NT(152), (T(20)+T(13)));
//G267: __E_bool_value_56(152) => '1'.
	p(NT(152), (T(29)));
//G268: __E_bool_value_56(152) => 'y'.
	p(NT(152), (T(28)));
//G269: __E_bool_value_56(152) => 'y' 'e' 's'.
	p(NT(152), (T(28)+T(6)+T(5)));
//G270: true_value(151)      => __E_bool_value_56(152).
	p(NT(151), (NT(152)));
//G271: bool_value(98)       => true_value(151).
	p(NT(98), (NT(151)));
//G272: __E_bool_value_57(154) => 'f'.
	p(NT(154), (T(7)));
//G273: __E_bool_value_57(154) => 'f' 'a' 'l' 's' 'e'.
	p(NT(154), (T(7)+T(3)+T(10)+T(5)+T(6)));
//G274: __E_bool_value_57(154) => 'o' 'f' 'f'.
	p(NT(154), (T(20)+T(7)+T(7)));
//G275: __E_bool_value_57(154) => '0'.
	p(NT(154), (T(30)));
//G276: __E_bool_value_57(154) => 'n'.
	p(NT(154), (T(13)));
//G277: __E_bool_value_57(154) => 'n' 'o'.
	p(NT(154), (T(13)+T(20)));
//G278: false_value(153)     => __E_bool_value_57(154).
	p(NT(153), (NT(154)));
//G279: bool_value(98)       => false_value(153).
	p(NT(98), (NT(153)));
//G280: __E_error_verbosity_58(156) => 'b'.
	p(NT(156), (T(19)));
//G281: __E_error_verbosity_58(156) => 'b' 'a' 's' 'i' 'c'.
	p(NT(156), (T(19)+T(3)+T(5)+T(9)+T(17)));
//G282: basic_sym(155)       => __E_error_verbosity_58(156).
	p(NT(155), (NT(156)));
//G283: error_verbosity(103) => basic_sym(155).
	p(NT(103), (NT(155)));
//G284: __E_error_verbosity_59(158) => 'd'.
	p(NT(158), (T(21)));
//G285: __E_error_verbosity_59(158) => 'd' 'e' 't' 'a' 'i' 'l' 'e' 'd'.
	p(NT(158), (T(21)+T(6)+T(14)+T(3)+T(9)+T(10)+T(6)+T(21)));
//G286: detailed_sym(157)    => __E_error_verbosity_59(158).
	p(NT(157), (NT(158)));
//G287: error_verbosity(103) => detailed_sym(157).
	p(NT(103), (NT(157)));
//G288: __E_error_verbosity_60(160) => 'r'.
	p(NT(160), (T(4)));
//G289: __E_error_verbosity_60(160) => 'r' 'c'.
	p(NT(160), (T(4)+T(17)));
//G290: __E_error_verbosity_60(160) => 'r' 'o' 'o' 't' '-' 'c' 'a' 'u' 's' 'e'.
	p(NT(160), (T(4)+T(20)+T(20)+T(14)+T(15)+T(17)+T(3)+T(16)+T(5)+T(6)));
//G291: root_cause_sym(159)  => __E_error_verbosity_60(160).
	p(NT(159), (NT(160)));
//G292: error_verbosity(103) => root_cause_sym(159).
	p(NT(103), (NT(159)));
//G293: __E_symbol_61(161)   => alpha(3).
	p(NT(161), (NT(3)));
//G294: __E_symbol_61(161)   => '_'.
	p(NT(161), (T(31)));
//G295: __E_symbol_62(162)   => alnum(2).
	p(NT(162), (NT(2)));
//G296: __E_symbol_62(162)   => '_'.
	p(NT(162), (T(31)));
//G297: __E_symbol_63(163)   => null.
	p(NT(163), (nul));
//G298: __E_symbol_63(163)   => __E_symbol_62(162) __E_symbol_63(163).
	p(NT(163), (NT(162)+NT(163)));
//G299: symbol(9)            => __E_symbol_61(161) __E_symbol_63(163).
	p(NT(9), (NT(161)+NT(163)));
//G300: __E_symbol_list_64(164) => _(7) ',' _(7) symbol(9).
	p(NT(164), (NT(7)+T(32)+NT(7)+NT(9)));
//G301: __E_symbol_list_65(165) => null.
	p(NT(165), (nul));
//G302: __E_symbol_list_65(165) => __E_symbol_list_64(164) __E_symbol_list_65(165).
	p(NT(165), (NT(164)+NT(165)));
//G303: symbol_list(82)      => symbol(9) __E_symbol_list_65(165).
	p(NT(82), (NT(9)+NT(165)));
//G304: __E_treepath_66(167) => _(7) '>' _(7) symbol(9).
	p(NT(167), (NT(7)+T(33)+NT(7)+NT(9)));
//G305: __E_treepath_67(168) => null.
	p(NT(168), (nul));
//G306: __E_treepath_67(168) => __E_treepath_66(167) __E_treepath_67(168).
	p(NT(168), (NT(167)+NT(168)));
//G307: treepath(166)        => symbol(9) __E_treepath_67(168).
	p(NT(166), (NT(9)+NT(168)));
//G308: __E_treepath_list_68(169) => _(7) ',' _(7) treepath(166).
	p(NT(169), (NT(7)+T(32)+NT(7)+NT(166)));
//G309: __E_treepath_list_69(170) => null.
	p(NT(170), (nul));
//G310: __E_treepath_list_69(170) => __E_treepath_list_68(169) __E_treepath_list_69(170).
	p(NT(170), (NT(169)+NT(170)));
//G311: treepath_list(84)    => treepath(166) __E_treepath_list_69(170).
	p(NT(84), (NT(166)+NT(170)));
//G312: filename(48)         => quoted_string(10).
	p(NT(48), (NT(10)));
//G313: __E_quoted_string_70(172) => null.
	p(NT(172), (nul));
//G314: __E_quoted_string_70(172) => quoted_string_char(171) __E_quoted_string_70(172).
	p(NT(172), (NT(171)+NT(172)));
//G315: quoted_string(10)    => '"' __E_quoted_string_70(172) '"'.
	p(NT(10), (T(34)+NT(172)+T(34)));
//G316: quoted_string_char(171) => unescaped_s(11).
	p(NT(171), (NT(11)));
//G317: quoted_string_char(171) => escaped_s(12).
	p(NT(171), (NT(12)));
//G318: __E_unescaped_s_71(173) => space(4).
	p(NT(173), (NT(4)));
//G319: __E_unescaped_s_71(173) => printable(5).
	p(NT(173), (NT(5)));
//G320: __E_unescaped_s_72(174) => '"'.
	p(NT(174), (T(34)));
//G321: __E_unescaped_s_72(174) => '\\'.
	p(NT(174), (T(35)));
//G322: __N_0(184)           => __E_unescaped_s_72(174).
	p(NT(184), (NT(174)));
//G323: unescaped_s(11)      => __E_unescaped_s_71(173) & ~( __N_0(184) ).	 # conjunctive
	p(NT(11), (NT(173)) & ~(NT(184)));
//G324: __E_escaped_s_73(175) => '"'.
	p(NT(175), (T(34)));
//G325: __E_escaped_s_73(175) => escape_char(14).
	p(NT(175), (NT(14)));
//G326: escaped_s(12)        => '\\' __E_escaped_s_73(175).
	p(NT(12), (T(35)+NT(175)));
//G327: escape_char(14)      => 'a'.
	p(NT(14), (T(3)));
//G328: escape_char(14)      => 'b'.
	p(NT(14), (T(19)));
//G329: escape_char(14)      => 'f'.
	p(NT(14), (T(7)));
//G330: escape_char(14)      => 'n'.
	p(NT(14), (T(13)));
//G331: escape_char(14)      => 'r'.
	p(NT(14), (T(4)));
//G332: escape_char(14)      => 't'.
	p(NT(14), (T(14)));
//G333: escape_char(14)      => 'v'.
	p(NT(14), (T(22)));
//G334: escape_char(14)      => '\\'.
	p(NT(14), (T(35)));
//G335: escape_char(14)      => '/'.
	p(NT(14), (T(36)));
//G336: escape_char(14)      => esc_hex(15).
	p(NT(14), (NT(15)));
//G337: escape_char(14)      => esc_u4(16).
	p(NT(14), (NT(16)));
//G338: escape_char(14)      => esc_U8(17).
	p(NT(14), (NT(17)));
//G339: __E_esc_hex_74(176)  => 'x'.
	p(NT(176), (T(25)));
//G340: __E_esc_hex_74(176)  => 'X'.
	p(NT(176), (T(37)));
//G341: __E_esc_hex_75(177)  => xdigit(6).
	p(NT(177), (NT(6)));
//G342: __E_esc_hex_75(177)  => null.
	p(NT(177), (nul));
//G343: esc_hex(15)          => __E_esc_hex_74(176) xdigit(6) __E_esc_hex_75(177).
	p(NT(15), (NT(176)+NT(6)+NT(177)));
//G344: esc_u4(16)           => 'u' xdigit(6) xdigit(6) xdigit(6) xdigit(6).
	p(NT(16), (T(16)+NT(6)+NT(6)+NT(6)+NT(6)));
//G345: esc_U8(17)           => 'U' xdigit(6) xdigit(6) xdigit(6) xdigit(6) xdigit(6) xdigit(6) xdigit(6) xdigit(6).
	p(NT(17), (T(38)+NT(6)+NT(6)+NT(6)+NT(6)+NT(6)+NT(6)+NT(6)+NT(6)));
//G346: __E___76(178)        => __(8).
	p(NT(178), (NT(8)));
//G347: __E___76(178)        => null.
	p(NT(178), (nul));
//G348: _(7)                 => __E___76(178).
	p(NT(7), (NT(178)));
//G349: __E____77(179)       => space(4).
	p(NT(179), (NT(4)));
//G350: __E____77(179)       => comment(180).
	p(NT(179), (NT(180)));
//G351: __(8)                => __E____77(179) _(7).
	p(NT(8), (NT(179)+NT(7)));
//G352: __E_comment_78(181)  => printable(5).
	p(NT(181), (NT(5)));
//G353: __E_comment_78(181)  => '\t'.
	p(NT(181), (T(27)));
//G354: __E_comment_79(182)  => null.
	p(NT(182), (nul));
//G355: __E_comment_79(182)  => __E_comment_78(181) __E_comment_79(182).
	p(NT(182), (NT(181)+NT(182)));
//G356: __E_comment_80(183)  => '\r'.
	p(NT(183), (T(39)));
//G357: __E_comment_80(183)  => '\n'.
	p(NT(183), (T(40)));
//G358: __E_comment_80(183)  => eof(1).
	p(NT(183), (NT(1)));
//G359: comment(180)         => '#' __E_comment_79(182) __E_comment_80(183).
	p(NT(180), (T(41)+NT(182)+NT(183)));
	#undef T
	#undef NT
	return loaded = true, p;
}

inline ::idni::grammar<char_type, terminal_type> grammar(
	nts, productions(), start_symbol, char_classes, grammar_options);

} // namespace tgf_repl_parser_data

struct tgf_repl_parser_nonterminals {
	enum nonterminal {
		nul, eof, alnum, alpha, space, printable, xdigit, _, __, symbol, 
		quoted_string, unescaped_s, escaped_s, parse_input_char_seq, escape_char, esc_hex, esc_u4, esc_U8, start, __E_start_0, 
		statement, __E___E_start_0_1, __E___E_start_0_2, grammar_cmd, igrammar_cmd, unreachable_cmd, reload_cmd, load_cmd, start_cmd, help_cmd, 
		version_cmd, license_cmd, quit_cmd, clear_cmd, get_cmd, set_cmd, toggle_cmd, enable_cmd, disable_cmd, add_cmd, 
		del_cmd, parse_file_cmd, parse_cmd, parse_sym, __E_parse_cmd_3, parse_input, parse_file_sym, __E_parse_file_cmd_4, filename, grammar_sym, 
		__E_grammar_cmd_5, igrammar_sym, __E_igrammar_cmd_6, __E_igrammar_cmd_7, start_sym, __E_start_cmd_8, __E_start_cmd_9, unreachable_sym, __E_unreachable_cmd_10, __E_unreachable_cmd_11, 
		reload_sym, __E_reload_cmd_12, load_sym, __E_load_cmd_13, help_sym, __E_help_cmd_14, __E_help_cmd_15, help_arg, version_sym, __E_version_cmd_16, 
		license_sym, __E_license_cmd_17, quit_sym, __E_quit_cmd_18, clear_sym, __E_clear_cmd_19, get_sym, __E_get_cmd_20, option, add_sym, 
		__E_add_cmd_21, list_option, symbol_list, treepaths_option, treepath_list, del_sym, __E_del_cmd_22, __E_del_cmd_23, toggle_sym, __E_toggle_cmd_24, 
		bool_option, enable_sym, __E_enable_cmd_25, disable_sym, __E_disable_cmd_26, set_sym, __E_set_cmd_27, __E___E_set_cmd_27_28, bool_value, __E___E_set_cmd_27_29, 
		__E___E_set_cmd_27_30, enum_ev_option, __E___E_set_cmd_27_31, error_verbosity, __E_parse_input_32, __E_parse_input_33, error_verbosity_opt, __E_enum_ev_option_34, status_opt, __E_bool_option_35, 
		colors_opt, __E_bool_option_36, print_ambiguity_opt, __E_bool_option_37, print_graphs_opt, __E_bool_option_38, print_rules_opt, __E_bool_option_39, print_facts_opt, __E_bool_option_40, 
		print_terminals_opt, __E_bool_option_41, measure_parsing_opt, __E_bool_option_42, measure_each_pos_opt, __E_bool_option_43, measure_forest_opt, __E_bool_option_44, measure_preprocess_opt, __E_bool_option_45, 
		gc_opt, __E_bool_option_46, debug_opt, __E_bool_option_47, auto_disambiguate_opt, __E_bool_option_48, trim_terminals_opt, __E_bool_option_49, inline_cc_opt, __E_bool_option_50, 
		nodisambig_list_opt, __E_list_option_51, enabled_prods_opt, __E_list_option_52, trim_opt, trim_children_opt, __E_list_option_53, trim_children_terminals_opt, __E_list_option_54, inline_opt, 
		__E_treepaths_option_55, true_value, __E_bool_value_56, false_value, __E_bool_value_57, basic_sym, __E_error_verbosity_58, detailed_sym, __E_error_verbosity_59, root_cause_sym, 
		__E_error_verbosity_60, __E_symbol_61, __E_symbol_62, __E_symbol_63, __E_symbol_list_64, __E_symbol_list_65, treepath, __E_treepath_66, __E_treepath_67, __E_treepath_list_68, 
		__E_treepath_list_69, quoted_string_char, __E_quoted_string_70, __E_unescaped_s_71, __E_unescaped_s_72, __E_escaped_s_73, __E_esc_hex_74, __E_esc_hex_75, __E___76, __E____77, 
		comment, __E_comment_78, __E_comment_79, __E_comment_80, __N_0, 
	};
};

struct tgf_repl_parser : public idni::parser<char, char32_t>, public tgf_repl_parser_nonterminals {
	static tgf_repl_parser& instance() {
		static tgf_repl_parser inst;
		return inst;
	}
	tgf_repl_parser() : idni::parser<char_type, terminal_type>(
		tgf_repl_parser_data::grammar,
		tgf_repl_parser_data::make_parser_options()) {}
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
