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
	"", "eof", "alnum", "alpha", "space", "printable", "xdigit", "start", "_", "__E_start_0", 
	"statement", "__E___E_start_0_1", "__E___E_start_0_2", "grammar_cmd", "igrammar_cmd", "unreachable_cmd", "reload_cmd", "load_cmd", "start_cmd", "help_cmd", 
	"version_cmd", "license_cmd", "quit_cmd", "clear_cmd", "get_cmd", "set_cmd", "toggle_cmd", "enable_cmd", "disable_cmd", "add_cmd", 
	"del_cmd", "parse_file_cmd", "parse_cmd", "parse_sym", "__E_parse_cmd_3", "__", "parse_input", "parse_file_sym", "__E_parse_file_cmd_4", "filename", 
	"grammar_sym", "__E_grammar_cmd_5", "igrammar_sym", "__E_igrammar_cmd_6", "__E_igrammar_cmd_7", "symbol", "start_sym", "__E_start_cmd_8", "__E_start_cmd_9", "unreachable_sym", 
	"__E_unreachable_cmd_10", "__E_unreachable_cmd_11", "reload_sym", "__E_reload_cmd_12", "load_sym", "__E_load_cmd_13", "help_sym", "__E_help_cmd_14", "__E_help_cmd_15", "help_arg", 
	"version_sym", "__E_version_cmd_16", "license_sym", "__E_license_cmd_17", "quit_sym", "__E_quit_cmd_18", "clear_sym", "__E_clear_cmd_19", "get_sym", "__E_get_cmd_20", 
	"option", "add_sym", "__E_add_cmd_21", "list_option", "symbol_list", "treepaths_option", "treepath_list", "del_sym", "__E_del_cmd_22", "__E_del_cmd_23", 
	"toggle_sym", "__E_toggle_cmd_24", "bool_option", "enable_sym", "__E_enable_cmd_25", "disable_sym", "__E_disable_cmd_26", "set_sym", "__E_set_cmd_27", "__E___E_set_cmd_27_28", 
	"bool_value", "__E___E_set_cmd_27_29", "__E___E___E_set_cmd_27_29_30", "__E___E___E_set_cmd_27_29_31", "__E___E_set_cmd_27_32", "__E___E___E_set_cmd_27_32_33", "__E___E___E_set_cmd_27_32_34", "enum_ev_option", "__E___E_set_cmd_27_35", "error_verbosity", 
	"symbol_option", "__E___E_set_cmd_27_36", "quoted_string", "parse_input_char_seq", "__E_parse_input_37", "__E_parse_input_38", "error_verbosity_opt", "__E_enum_ev_option_39", "start_opt", "status_opt", 
	"__E_bool_option_40", "colors_opt", "__E_bool_option_41", "print_ambiguity_opt", "__E_bool_option_42", "print_graphs_opt", "__E_bool_option_43", "print_rules_opt", "__E_bool_option_44", "print_facts_opt", 
	"__E_bool_option_45", "print_terminals_opt", "__E_bool_option_46", "measure_parsing_opt", "__E_bool_option_47", "measure_each_pos_opt", "__E_bool_option_48", "measure_forest_opt", "__E_bool_option_49", "measure_preprocess_opt", 
	"__E_bool_option_50", "gc_opt", "__E_bool_option_51", "debug_opt", "__E_bool_option_52", "auto_disambiguate_opt", "__E_bool_option_53", "trim_terminals_opt", "__E_bool_option_54", "inline_cc_opt", 
	"__E_bool_option_55", "derive_char_classes_opt", "__E_bool_option_56", "nodisambig_list_opt", "__E_list_option_57", "enabled_prods_opt", "__E_list_option_58", "trim_opt", "trim_children_opt", "__E_list_option_59", 
	"trim_children_terminals_opt", "__E_list_option_60", "inline_opt", "__E_treepaths_option_61", "true_value", "__E_bool_value_62", "false_value", "__E_bool_value_63", "basic_sym", "__E_error_verbosity_64", 
	"detailed_sym", "__E_error_verbosity_65", "root_cause_sym", "__E_error_verbosity_66", "__E_symbol_67", "__E_symbol_68", "__E_symbol_69", "__E_symbol_list_70", "__E_symbol_list_71", "treepath", 
	"__E_treepath_72", "__E_treepath_73", "__E_treepath_list_74", "__E_treepath_list_75", "quoted_string_char", "__E_quoted_string_76", "unescaped_s", "escaped_s", "__E_unescaped_s_77", "__E_unescaped_s_78", 
	"__E_escaped_s_79", "escape_char", "esc_hex", "esc_u4", "esc_U8", "__E_esc_hex_80", "__E_esc_hex_81", "__E___82", "__E____83", "comment", 
	"__E_comment_84", "__E_comment_85", "__E_comment_86", "__N_0", 
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
		"xdigit",
	}, {
		{ "unicode_printable", "printable" },
	}, nts);

inline struct ::idni::grammar<char_type, terminal_type>::options
	grammar_options
{
	.transform_negation = false,
	.auto_disambiguate = true,
	.shaping = {
		.to_trim = {
			8, 35
		},
		.trim_terminals = true,
		.dont_trim_terminals_of = {
			45, 102, 103, 176, 177, 181, 182, 183, 184
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

inline ::idni::prods<char_type, terminal_type> start_symbol{ nts(7) };

#ifdef TAU_PARSER_BUILD_HEADER_ONLY
inline idni::prods<char_type, terminal_type>& productions() {
	static bool loaded = false;
	static idni::prods<char_type, terminal_type>
		p, nul(idni::lit<char_type, terminal_type>{});
	if (loaded) return p;
	#define  T(x) (idni::prods<char_type, terminal_type>{ terminals[x] })
	#define NT(x) (idni::prods<char_type, terminal_type>{ nts(x) })
//G0:   __E___E_start_0_1(11) => _(8) '.' _(8) statement(10).
	p(NT(11), (NT(8)+T(1)+NT(8)+NT(10)));
//G1:   __E___E_start_0_2(12) => null.
	p(NT(12), (nul));
//G2:   __E___E_start_0_2(12) => __E___E_start_0_2(12) __E___E_start_0_1(11).
	p(NT(12), (NT(12)+NT(11)));
//G3:   __E_start_0(9)       => statement(10) __E___E_start_0_2(12) _(8).
	p(NT(9), (NT(10)+NT(12)+NT(8)));
//G4:   __E_start_0(9)       => null.
	p(NT(9), (nul));
//G5:   start(7)             => _(8) __E_start_0(9).
	p(NT(7), (NT(8)+NT(9)));
//G6:   statement(10)        => grammar_cmd(13).
	p(NT(10), (NT(13)));
//G7:   statement(10)        => igrammar_cmd(14).
	p(NT(10), (NT(14)));
//G8:   statement(10)        => unreachable_cmd(15).
	p(NT(10), (NT(15)));
//G9:   statement(10)        => reload_cmd(16).
	p(NT(10), (NT(16)));
//G10:  statement(10)        => load_cmd(17).
	p(NT(10), (NT(17)));
//G11:  statement(10)        => start_cmd(18).
	p(NT(10), (NT(18)));
//G12:  statement(10)        => help_cmd(19).
	p(NT(10), (NT(19)));
//G13:  statement(10)        => version_cmd(20).
	p(NT(10), (NT(20)));
//G14:  statement(10)        => license_cmd(21).
	p(NT(10), (NT(21)));
//G15:  statement(10)        => quit_cmd(22).
	p(NT(10), (NT(22)));
//G16:  statement(10)        => clear_cmd(23).
	p(NT(10), (NT(23)));
//G17:  statement(10)        => get_cmd(24).
	p(NT(10), (NT(24)));
//G18:  statement(10)        => set_cmd(25).
	p(NT(10), (NT(25)));
//G19:  statement(10)        => toggle_cmd(26).
	p(NT(10), (NT(26)));
//G20:  statement(10)        => enable_cmd(27).
	p(NT(10), (NT(27)));
//G21:  statement(10)        => disable_cmd(28).
	p(NT(10), (NT(28)));
//G22:  statement(10)        => add_cmd(29).
	p(NT(10), (NT(29)));
//G23:  statement(10)        => del_cmd(30).
	p(NT(10), (NT(30)));
//G24:  statement(10)        => parse_file_cmd(31).
	p(NT(10), (NT(31)));
//G25:  statement(10)        => parse_cmd(32).
	p(NT(10), (NT(32)));
//G26:  __E_parse_cmd_3(34)  => 'p'.
	p(NT(34), (T(2)));
//G27:  __E_parse_cmd_3(34)  => 'p' 'a' 'r' 's' 'e'.
	p(NT(34), (T(2)+T(3)+T(4)+T(5)+T(6)));
//G28:  parse_sym(33)        => __E_parse_cmd_3(34).
	p(NT(33), (NT(34)));
//G29:  parse_cmd(32)        => parse_sym(33) __(35) parse_input(36).
	p(NT(32), (NT(33)+NT(35)+NT(36)));
//G30:  __E_parse_file_cmd_4(38) => 'f'.
	p(NT(38), (T(7)));
//G31:  __E_parse_file_cmd_4(38) => 'p' 'f'.
	p(NT(38), (T(2)+T(7)));
//G32:  __E_parse_file_cmd_4(38) => 'p' 'a' 'r' 's' 'e' ' ' 'f' 'i' 'l' 'e'.
	p(NT(38), (T(2)+T(3)+T(4)+T(5)+T(6)+T(8)+T(7)+T(9)+T(10)+T(6)));
//G33:  parse_file_sym(37)   => __E_parse_file_cmd_4(38).
	p(NT(37), (NT(38)));
//G34:  parse_file_cmd(31)   => parse_file_sym(37) __(35) filename(39).
	p(NT(31), (NT(37)+NT(35)+NT(39)));
//G35:  __E_grammar_cmd_5(41) => 'g'.
	p(NT(41), (T(11)));
//G36:  __E_grammar_cmd_5(41) => 'g' 'r' 'a' 'm' 'm' 'a' 'r'.
	p(NT(41), (T(11)+T(4)+T(3)+T(12)+T(12)+T(3)+T(4)));
//G37:  grammar_sym(40)      => __E_grammar_cmd_5(41).
	p(NT(40), (NT(41)));
//G38:  grammar_cmd(13)      => grammar_sym(40).
	p(NT(13), (NT(40)));
//G39:  __E_igrammar_cmd_6(43) => 'i'.
	p(NT(43), (T(9)));
//G40:  __E_igrammar_cmd_6(43) => 'i' 'g'.
	p(NT(43), (T(9)+T(11)));
//G41:  __E_igrammar_cmd_6(43) => 'i' 'n' 't' 'e' 'r' 'n' 'a' 'l' '-' 'g' 'r' 'a' 'm' 'm' 'a' 'r'.
	p(NT(43), (T(9)+T(13)+T(14)+T(6)+T(4)+T(13)+T(3)+T(10)+T(15)+T(11)+T(4)+T(3)+T(12)+T(12)+T(3)+T(4)));
//G42:  igrammar_sym(42)     => __E_igrammar_cmd_6(43).
	p(NT(42), (NT(43)));
//G43:  __E_igrammar_cmd_7(44) => __(35) symbol(45).
	p(NT(44), (NT(35)+NT(45)));
//G44:  __E_igrammar_cmd_7(44) => null.
	p(NT(44), (nul));
//G45:  igrammar_cmd(14)     => igrammar_sym(42) __E_igrammar_cmd_7(44).
	p(NT(14), (NT(42)+NT(44)));
//G46:  __E_start_cmd_8(47)  => 's'.
	p(NT(47), (T(5)));
//G47:  __E_start_cmd_8(47)  => 's' 't' 'a' 'r' 't'.
	p(NT(47), (T(5)+T(14)+T(3)+T(4)+T(14)));
//G48:  start_sym(46)        => __E_start_cmd_8(47).
	p(NT(46), (NT(47)));
//G49:  __E_start_cmd_9(48)  => __(35) symbol(45).
	p(NT(48), (NT(35)+NT(45)));
//G50:  __E_start_cmd_9(48)  => null.
	p(NT(48), (nul));
//G51:  start_cmd(18)        => start_sym(46) __E_start_cmd_9(48).
	p(NT(18), (NT(46)+NT(48)));
//G52:  __E_unreachable_cmd_10(50) => 'u'.
	p(NT(50), (T(16)));
//G53:  __E_unreachable_cmd_10(50) => 'u' 'n' 'r' 'e' 'a' 'c' 'h' 'a' 'b' 'l' 'e'.
	p(NT(50), (T(16)+T(13)+T(4)+T(6)+T(3)+T(17)+T(18)+T(3)+T(19)+T(10)+T(6)));
//G54:  unreachable_sym(49)  => __E_unreachable_cmd_10(50).
	p(NT(49), (NT(50)));
//G55:  __E_unreachable_cmd_11(51) => __(35) symbol(45).
	p(NT(51), (NT(35)+NT(45)));
//G56:  __E_unreachable_cmd_11(51) => null.
	p(NT(51), (nul));
//G57:  unreachable_cmd(15)  => unreachable_sym(49) __E_unreachable_cmd_11(51).
	p(NT(15), (NT(49)+NT(51)));
//G58:  __E_reload_cmd_12(53) => 'r'.
	p(NT(53), (T(4)));
//G59:  __E_reload_cmd_12(53) => 'r' 'e' 'l' 'o' 'a' 'd'.
	p(NT(53), (T(4)+T(6)+T(10)+T(20)+T(3)+T(21)));
//G60:  reload_sym(52)       => __E_reload_cmd_12(53).
	p(NT(52), (NT(53)));
//G61:  reload_cmd(16)       => reload_sym(52).
	p(NT(16), (NT(52)));
//G62:  __E_load_cmd_13(55)  => 'l'.
	p(NT(55), (T(10)));
//G63:  __E_load_cmd_13(55)  => 'l' 'o' 'a' 'd'.
	p(NT(55), (T(10)+T(20)+T(3)+T(21)));
//G64:  load_sym(54)         => __E_load_cmd_13(55).
	p(NT(54), (NT(55)));
//G65:  load_cmd(17)         => load_sym(54) __(35) filename(39).
	p(NT(17), (NT(54)+NT(35)+NT(39)));
//G66:  __E_help_cmd_14(57)  => 'h'.
	p(NT(57), (T(18)));
//G67:  __E_help_cmd_14(57)  => 'h' 'e' 'l' 'p'.
	p(NT(57), (T(18)+T(6)+T(10)+T(2)));
//G68:  help_sym(56)         => __E_help_cmd_14(57).
	p(NT(56), (NT(57)));
//G69:  __E_help_cmd_15(58)  => __(35) help_arg(59).
	p(NT(58), (NT(35)+NT(59)));
//G70:  __E_help_cmd_15(58)  => null.
	p(NT(58), (nul));
//G71:  help_cmd(19)         => help_sym(56) __E_help_cmd_15(58).
	p(NT(19), (NT(56)+NT(58)));
//G72:  __E_version_cmd_16(61) => 'v'.
	p(NT(61), (T(22)));
//G73:  __E_version_cmd_16(61) => 'v' 'e' 'r' 's' 'i' 'o' 'n'.
	p(NT(61), (T(22)+T(6)+T(4)+T(5)+T(9)+T(20)+T(13)));
//G74:  version_sym(60)      => __E_version_cmd_16(61).
	p(NT(60), (NT(61)));
//G75:  version_cmd(20)      => version_sym(60).
	p(NT(20), (NT(60)));
//G76:  __E_license_cmd_17(63) => 'L'.
	p(NT(63), (T(23)));
//G77:  __E_license_cmd_17(63) => 'l' 'i' 'c' 'e' 'n' 's' 'e'.
	p(NT(63), (T(10)+T(9)+T(17)+T(6)+T(13)+T(5)+T(6)));
//G78:  license_sym(62)      => __E_license_cmd_17(63).
	p(NT(62), (NT(63)));
//G79:  license_cmd(21)      => license_sym(62).
	p(NT(21), (NT(62)));
//G80:  __E_quit_cmd_18(65)  => 'q'.
	p(NT(65), (T(24)));
//G81:  __E_quit_cmd_18(65)  => 'q' 'u' 'i' 't'.
	p(NT(65), (T(24)+T(16)+T(9)+T(14)));
//G82:  __E_quit_cmd_18(65)  => 'e'.
	p(NT(65), (T(6)));
//G83:  __E_quit_cmd_18(65)  => 'e' 'x' 'i' 't'.
	p(NT(65), (T(6)+T(25)+T(9)+T(14)));
//G84:  quit_sym(64)         => __E_quit_cmd_18(65).
	p(NT(64), (NT(65)));
//G85:  quit_cmd(22)         => quit_sym(64).
	p(NT(22), (NT(64)));
//G86:  __E_clear_cmd_19(67) => 'c' 'l' 's'.
	p(NT(67), (T(17)+T(10)+T(5)));
//G87:  __E_clear_cmd_19(67) => 'c' 'l' 'e' 'a' 'r'.
	p(NT(67), (T(17)+T(10)+T(6)+T(3)+T(4)));
//G88:  clear_sym(66)        => __E_clear_cmd_19(67).
	p(NT(66), (NT(67)));
//G89:  clear_cmd(23)        => clear_sym(66).
	p(NT(23), (NT(66)));
//G90:  get_sym(68)          => 'g' 'e' 't'.
	p(NT(68), (T(11)+T(6)+T(14)));
//G91:  __E_get_cmd_20(69)   => __(35) option(70).
	p(NT(69), (NT(35)+NT(70)));
//G92:  __E_get_cmd_20(69)   => null.
	p(NT(69), (nul));
//G93:  get_cmd(24)          => get_sym(68) __E_get_cmd_20(69).
	p(NT(24), (NT(68)+NT(69)));
//G94:  add_sym(71)          => 'a' 'd' 'd'.
	p(NT(71), (T(3)+T(21)+T(21)));
//G95:  __E_add_cmd_21(72)   => list_option(73) __(35) symbol_list(74).
	p(NT(72), (NT(73)+NT(35)+NT(74)));
//G96:  __E_add_cmd_21(72)   => treepaths_option(75) __(35) treepath_list(76).
	p(NT(72), (NT(75)+NT(35)+NT(76)));
//G97:  add_cmd(29)          => add_sym(71) __(35) __E_add_cmd_21(72).
	p(NT(29), (NT(71)+NT(35)+NT(72)));
//G98:  __E_del_cmd_22(78)   => 'd' 'e' 'l'.
	p(NT(78), (T(21)+T(6)+T(10)));
//G99:  __E_del_cmd_22(78)   => 'd' 'e' 'l' 'e' 't' 'e'.
	p(NT(78), (T(21)+T(6)+T(10)+T(6)+T(14)+T(6)));
//G100: __E_del_cmd_22(78)   => 'r' 'm'.
	p(NT(78), (T(4)+T(12)));
//G101: __E_del_cmd_22(78)   => 'r' 'e' 'm'.
	p(NT(78), (T(4)+T(6)+T(12)));
//G102: __E_del_cmd_22(78)   => 'r' 'e' 'm' 'o' 'v' 'e'.
	p(NT(78), (T(4)+T(6)+T(12)+T(20)+T(22)+T(6)));
//G103: del_sym(77)          => __E_del_cmd_22(78).
	p(NT(77), (NT(78)));
//G104: __E_del_cmd_23(79)   => list_option(73) __(35) symbol_list(74).
	p(NT(79), (NT(73)+NT(35)+NT(74)));
//G105: __E_del_cmd_23(79)   => treepaths_option(75) __(35) treepath_list(76).
	p(NT(79), (NT(75)+NT(35)+NT(76)));
//G106: del_cmd(30)          => del_sym(77) __(35) __E_del_cmd_23(79).
	p(NT(30), (NT(77)+NT(35)+NT(79)));
//G107: __E_toggle_cmd_24(81) => 't' 'o' 'g'.
	p(NT(81), (T(14)+T(20)+T(11)));
//G108: __E_toggle_cmd_24(81) => 't' 'o' 'g' 'g' 'l' 'e'.
	p(NT(81), (T(14)+T(20)+T(11)+T(11)+T(10)+T(6)));
//G109: toggle_sym(80)       => __E_toggle_cmd_24(81).
	p(NT(80), (NT(81)));
//G110: toggle_cmd(26)       => toggle_sym(80) __(35) bool_option(82).
	p(NT(26), (NT(80)+NT(35)+NT(82)));
//G111: __E_enable_cmd_25(84) => 'e' 'n' __(35).
	p(NT(84), (T(6)+T(13)+NT(35)));
//G112: __E_enable_cmd_25(84) => 'e' 'n' 'a' 'b' 'l' 'e' __(35).
	p(NT(84), (T(6)+T(13)+T(3)+T(19)+T(10)+T(6)+NT(35)));
//G113: enable_sym(83)       => __E_enable_cmd_25(84).
	p(NT(83), (NT(84)));
//G114: enable_cmd(27)       => enable_sym(83) bool_option(82).
	p(NT(27), (NT(83)+NT(82)));
//G115: __E_disable_cmd_26(86) => 'd' 'i' 's' __(35).
	p(NT(86), (T(21)+T(9)+T(5)+NT(35)));
//G116: __E_disable_cmd_26(86) => 'd' 'i' 's' 'a' 'b' 'l' 'e' __(35).
	p(NT(86), (T(21)+T(9)+T(5)+T(3)+T(19)+T(10)+T(6)+NT(35)));
//G117: disable_sym(85)      => __E_disable_cmd_26(86).
	p(NT(85), (NT(86)));
//G118: disable_cmd(28)      => disable_sym(85) bool_option(82).
	p(NT(28), (NT(85)+NT(82)));
//G119: set_sym(87)          => 's' 'e' 't'.
	p(NT(87), (T(5)+T(6)+T(14)));
//G120: __E___E_set_cmd_27_28(89) => __(35).
	p(NT(89), (NT(35)));
//G121: __E___E_set_cmd_27_28(89) => _(8) '=' _(8).
	p(NT(89), (NT(8)+T(26)+NT(8)));
//G122: __E_set_cmd_27(88)   => bool_option(82) __E___E_set_cmd_27_28(89) bool_value(90).
	p(NT(88), (NT(82)+NT(89)+NT(90)));
//G123: __E___E___E_set_cmd_27_29_30(92) => symbol_list(74).
	p(NT(92), (NT(74)));
//G124: __E___E___E_set_cmd_27_29_30(92) => null.
	p(NT(92), (nul));
//G125: __E___E_set_cmd_27_29(91) => _(8) '=' _(8) __E___E___E_set_cmd_27_29_30(92).
	p(NT(91), (NT(8)+T(26)+NT(8)+NT(92)));
//G126: __E___E___E_set_cmd_27_29_31(93) => __(35) symbol_list(74).
	p(NT(93), (NT(35)+NT(74)));
//G127: __E___E___E_set_cmd_27_29_31(93) => null.
	p(NT(93), (nul));
//G128: __E___E_set_cmd_27_29(91) => __E___E___E_set_cmd_27_29_31(93).
	p(NT(91), (NT(93)));
//G129: __E_set_cmd_27(88)   => list_option(73) __E___E_set_cmd_27_29(91).
	p(NT(88), (NT(73)+NT(91)));
//G130: __E___E___E_set_cmd_27_32_33(95) => treepath_list(76).
	p(NT(95), (NT(76)));
//G131: __E___E___E_set_cmd_27_32_33(95) => null.
	p(NT(95), (nul));
//G132: __E___E_set_cmd_27_32(94) => _(8) '=' _(8) __E___E___E_set_cmd_27_32_33(95).
	p(NT(94), (NT(8)+T(26)+NT(8)+NT(95)));
//G133: __E___E___E_set_cmd_27_32_34(96) => __(35) treepath_list(76).
	p(NT(96), (NT(35)+NT(76)));
//G134: __E___E___E_set_cmd_27_32_34(96) => null.
	p(NT(96), (nul));
//G135: __E___E_set_cmd_27_32(94) => __E___E___E_set_cmd_27_32_34(96).
	p(NT(94), (NT(96)));
//G136: __E_set_cmd_27(88)   => treepaths_option(75) __E___E_set_cmd_27_32(94).
	p(NT(88), (NT(75)+NT(94)));
//G137: __E___E_set_cmd_27_35(98) => __(35).
	p(NT(98), (NT(35)));
//G138: __E___E_set_cmd_27_35(98) => _(8) '=' _(8).
	p(NT(98), (NT(8)+T(26)+NT(8)));
//G139: __E_set_cmd_27(88)   => enum_ev_option(97) __E___E_set_cmd_27_35(98) error_verbosity(99).
	p(NT(88), (NT(97)+NT(98)+NT(99)));
//G140: __E___E_set_cmd_27_36(101) => __(35).
	p(NT(101), (NT(35)));
//G141: __E___E_set_cmd_27_36(101) => _(8) '=' _(8).
	p(NT(101), (NT(8)+T(26)+NT(8)));
//G142: __E_set_cmd_27(88)   => symbol_option(100) __E___E_set_cmd_27_36(101) symbol(45).
	p(NT(88), (NT(100)+NT(101)+NT(45)));
//G143: set_cmd(25)          => set_sym(87) __(35) __E_set_cmd_27(88).
	p(NT(25), (NT(87)+NT(35)+NT(88)));
//G144: parse_input(36)      => quoted_string(102).
	p(NT(36), (NT(102)));
//G145: __E_parse_input_37(104) => printable(5).
	p(NT(104), (NT(5)));
//G146: __E_parse_input_37(104) => '\t'.
	p(NT(104), (T(27)));
//G147: __E_parse_input_38(105) => __E_parse_input_37(104).
	p(NT(105), (NT(104)));
//G148: __E_parse_input_38(105) => __E_parse_input_38(105) __E_parse_input_37(104).
	p(NT(105), (NT(105)+NT(104)));
//G149: parse_input_char_seq(103) => __E_parse_input_38(105).
	p(NT(103), (NT(105)));
//G150: parse_input(36)      => parse_input_char_seq(103).
	p(NT(36), (NT(103)));
//G151: help_arg(59)         => grammar_sym(40).
	p(NT(59), (NT(40)));
//G152: help_arg(59)         => igrammar_sym(42).
	p(NT(59), (NT(42)));
//G153: help_arg(59)         => unreachable_sym(49).
	p(NT(59), (NT(49)));
//G154: help_arg(59)         => start_sym(46).
	p(NT(59), (NT(46)));
//G155: help_arg(59)         => parse_sym(33).
	p(NT(59), (NT(33)));
//G156: help_arg(59)         => parse_file_sym(37).
	p(NT(59), (NT(37)));
//G157: help_arg(59)         => load_sym(54).
	p(NT(59), (NT(54)));
//G158: help_arg(59)         => reload_sym(52).
	p(NT(59), (NT(52)));
//G159: help_arg(59)         => clear_sym(66).
	p(NT(59), (NT(66)));
//G160: help_arg(59)         => help_sym(56).
	p(NT(59), (NT(56)));
//G161: help_arg(59)         => quit_sym(64).
	p(NT(59), (NT(64)));
//G162: help_arg(59)         => version_sym(60).
	p(NT(59), (NT(60)));
//G163: help_arg(59)         => license_sym(62).
	p(NT(59), (NT(62)));
//G164: help_arg(59)         => get_sym(68).
	p(NT(59), (NT(68)));
//G165: help_arg(59)         => set_sym(87).
	p(NT(59), (NT(87)));
//G166: help_arg(59)         => add_sym(71).
	p(NT(59), (NT(71)));
//G167: help_arg(59)         => del_sym(77).
	p(NT(59), (NT(77)));
//G168: help_arg(59)         => toggle_sym(80).
	p(NT(59), (NT(80)));
//G169: help_arg(59)         => enable_sym(83).
	p(NT(59), (NT(83)));
//G170: help_arg(59)         => disable_sym(85).
	p(NT(59), (NT(85)));
//G171: option(70)           => bool_option(82).
	p(NT(70), (NT(82)));
//G172: option(70)           => enum_ev_option(97).
	p(NT(70), (NT(97)));
//G173: option(70)           => list_option(73).
	p(NT(70), (NT(73)));
//G174: option(70)           => treepaths_option(75).
	p(NT(70), (NT(75)));
//G175: option(70)           => symbol_option(100).
	p(NT(70), (NT(100)));
//G176: __E_enum_ev_option_39(107) => 'e'.
	p(NT(107), (T(6)));
//G177: __E_enum_ev_option_39(107) => 'e' 'r' 'r' 'o' 'r' '-' 'v' 'e' 'r' 'b' 'o' 's' 'i' 't' 'y'.
	p(NT(107), (T(6)+T(4)+T(4)+T(20)+T(4)+T(15)+T(22)+T(6)+T(4)+T(19)+T(20)+T(5)+T(9)+T(14)+T(28)));
//G178: error_verbosity_opt(106) => __E_enum_ev_option_39(107).
	p(NT(106), (NT(107)));
//G179: enum_ev_option(97)   => error_verbosity_opt(106).
	p(NT(97), (NT(106)));
//G180: start_opt(108)       => 's' 't' 'a' 'r' 't'.
	p(NT(108), (T(5)+T(14)+T(3)+T(4)+T(14)));
//G181: symbol_option(100)   => start_opt(108).
	p(NT(100), (NT(108)));
//G182: __E_bool_option_40(110) => 's'.
	p(NT(110), (T(5)));
//G183: __E_bool_option_40(110) => 's' 't' 'a' 't' 'u' 's'.
	p(NT(110), (T(5)+T(14)+T(3)+T(14)+T(16)+T(5)));
//G184: status_opt(109)      => __E_bool_option_40(110).
	p(NT(109), (NT(110)));
//G185: bool_option(82)      => status_opt(109).
	p(NT(82), (NT(109)));
//G186: __E_bool_option_41(112) => 'c'.
	p(NT(112), (T(17)));
//G187: __E_bool_option_41(112) => 'c' 'o' 'l' 'o' 'r'.
	p(NT(112), (T(17)+T(20)+T(10)+T(20)+T(4)));
//G188: __E_bool_option_41(112) => 'c' 'o' 'l' 'o' 'r' 's'.
	p(NT(112), (T(17)+T(20)+T(10)+T(20)+T(4)+T(5)));
//G189: colors_opt(111)      => __E_bool_option_41(112).
	p(NT(111), (NT(112)));
//G190: bool_option(82)      => colors_opt(111).
	p(NT(82), (NT(111)));
//G191: __E_bool_option_42(114) => 'a'.
	p(NT(114), (T(3)));
//G192: __E_bool_option_42(114) => 'a' 'm' 'b' 'i' 'g' 'u' 'i' 't' 'y'.
	p(NT(114), (T(3)+T(12)+T(19)+T(9)+T(11)+T(16)+T(9)+T(14)+T(28)));
//G193: __E_bool_option_42(114) => 'p' 'r' 'i' 'n' 't' '-' 'a' 'm' 'b' 'i' 'g' 'u' 'i' 't' 'y'.
	p(NT(114), (T(2)+T(4)+T(9)+T(13)+T(14)+T(15)+T(3)+T(12)+T(19)+T(9)+T(11)+T(16)+T(9)+T(14)+T(28)));
//G194: print_ambiguity_opt(113) => __E_bool_option_42(114).
	p(NT(113), (NT(114)));
//G195: bool_option(82)      => print_ambiguity_opt(113).
	p(NT(82), (NT(113)));
//G196: __E_bool_option_43(116) => 'g'.
	p(NT(116), (T(11)));
//G197: __E_bool_option_43(116) => 'g' 'r' 'a' 'p' 'h' 's'.
	p(NT(116), (T(11)+T(4)+T(3)+T(2)+T(18)+T(5)));
//G198: __E_bool_option_43(116) => 'p' 'r' 'i' 'n' 't' '-' 'g' 'r' 'a' 'p' 'h' 's'.
	p(NT(116), (T(2)+T(4)+T(9)+T(13)+T(14)+T(15)+T(11)+T(4)+T(3)+T(2)+T(18)+T(5)));
//G199: print_graphs_opt(115) => __E_bool_option_43(116).
	p(NT(115), (NT(116)));
//G200: bool_option(82)      => print_graphs_opt(115).
	p(NT(82), (NT(115)));
//G201: __E_bool_option_44(118) => 'r'.
	p(NT(118), (T(4)));
//G202: __E_bool_option_44(118) => 'r' 'u' 'l' 'e' 's'.
	p(NT(118), (T(4)+T(16)+T(10)+T(6)+T(5)));
//G203: __E_bool_option_44(118) => 'p' 'r' 'i' 'n' 't' '-' 'r' 'u' 'l' 'e' 's'.
	p(NT(118), (T(2)+T(4)+T(9)+T(13)+T(14)+T(15)+T(4)+T(16)+T(10)+T(6)+T(5)));
//G204: print_rules_opt(117) => __E_bool_option_44(118).
	p(NT(117), (NT(118)));
//G205: bool_option(82)      => print_rules_opt(117).
	p(NT(82), (NT(117)));
//G206: __E_bool_option_45(120) => 'f'.
	p(NT(120), (T(7)));
//G207: __E_bool_option_45(120) => 'f' 'a' 'c' 't' 's'.
	p(NT(120), (T(7)+T(3)+T(17)+T(14)+T(5)));
//G208: __E_bool_option_45(120) => 'p' 'r' 'i' 'n' 't' '-' 'f' 'a' 'c' 't' 's'.
	p(NT(120), (T(2)+T(4)+T(9)+T(13)+T(14)+T(15)+T(7)+T(3)+T(17)+T(14)+T(5)));
//G209: print_facts_opt(119) => __E_bool_option_45(120).
	p(NT(119), (NT(120)));
//G210: bool_option(82)      => print_facts_opt(119).
	p(NT(82), (NT(119)));
//G211: __E_bool_option_46(122) => 't'.
	p(NT(122), (T(14)));
//G212: __E_bool_option_46(122) => 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
	p(NT(122), (T(14)+T(6)+T(4)+T(12)+T(9)+T(13)+T(3)+T(10)+T(5)));
//G213: __E_bool_option_46(122) => 'p' 'r' 'i' 'n' 't' '-' 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
	p(NT(122), (T(2)+T(4)+T(9)+T(13)+T(14)+T(15)+T(14)+T(6)+T(4)+T(12)+T(9)+T(13)+T(3)+T(10)+T(5)));
//G214: print_terminals_opt(121) => __E_bool_option_46(122).
	p(NT(121), (NT(122)));
//G215: bool_option(82)      => print_terminals_opt(121).
	p(NT(82), (NT(121)));
//G216: __E_bool_option_47(124) => 'm'.
	p(NT(124), (T(12)));
//G217: __E_bool_option_47(124) => 'm' 'e' 'a' 's' 'u' 'r' 'e'.
	p(NT(124), (T(12)+T(6)+T(3)+T(5)+T(16)+T(4)+T(6)));
//G218: __E_bool_option_47(124) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'p' 'a' 'r' 's' 'i' 'n' 'g'.
	p(NT(124), (T(12)+T(6)+T(3)+T(5)+T(16)+T(4)+T(6)+T(15)+T(2)+T(3)+T(4)+T(5)+T(9)+T(13)+T(11)));
//G219: measure_parsing_opt(123) => __E_bool_option_47(124).
	p(NT(123), (NT(124)));
//G220: bool_option(82)      => measure_parsing_opt(123).
	p(NT(82), (NT(123)));
//G221: __E_bool_option_48(126) => 'm' 'e'.
	p(NT(126), (T(12)+T(6)));
//G222: __E_bool_option_48(126) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'e' 'a' 'c' 'h'.
	p(NT(126), (T(12)+T(6)+T(3)+T(5)+T(16)+T(4)+T(6)+T(15)+T(6)+T(3)+T(17)+T(18)));
//G223: __E_bool_option_48(126) => 'm' 'e' 'p'.
	p(NT(126), (T(12)+T(6)+T(2)));
//G224: __E_bool_option_48(126) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'e' 'a' 'c' 'h' '-' 'p' 'o' 's'.
	p(NT(126), (T(12)+T(6)+T(3)+T(5)+T(16)+T(4)+T(6)+T(15)+T(6)+T(3)+T(17)+T(18)+T(15)+T(2)+T(20)+T(5)));
//G225: measure_each_pos_opt(125) => __E_bool_option_48(126).
	p(NT(125), (NT(126)));
//G226: bool_option(82)      => measure_each_pos_opt(125).
	p(NT(82), (NT(125)));
//G227: __E_bool_option_49(128) => 'm' 'f'.
	p(NT(128), (T(12)+T(7)));
//G228: __E_bool_option_49(128) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'f' 'o' 'r' 'e' 's' 't'.
	p(NT(128), (T(12)+T(6)+T(3)+T(5)+T(16)+T(4)+T(6)+T(15)+T(7)+T(20)+T(4)+T(6)+T(5)+T(14)));
//G229: measure_forest_opt(127) => __E_bool_option_49(128).
	p(NT(127), (NT(128)));
//G230: bool_option(82)      => measure_forest_opt(127).
	p(NT(82), (NT(127)));
//G231: __E_bool_option_50(130) => 'm' 'p'.
	p(NT(130), (T(12)+T(2)));
//G232: __E_bool_option_50(130) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'p' 'r' 'e' 'p' 'r' 'o' 'c' 'e' 's' 's'.
	p(NT(130), (T(12)+T(6)+T(3)+T(5)+T(16)+T(4)+T(6)+T(15)+T(2)+T(4)+T(6)+T(2)+T(4)+T(20)+T(17)+T(6)+T(5)+T(5)));
//G233: measure_preprocess_opt(129) => __E_bool_option_50(130).
	p(NT(129), (NT(130)));
//G234: bool_option(82)      => measure_preprocess_opt(129).
	p(NT(82), (NT(129)));
//G235: __E_bool_option_51(132) => 'g' 'c'.
	p(NT(132), (T(11)+T(17)));
//G236: gc_opt(131)          => __E_bool_option_51(132).
	p(NT(131), (NT(132)));
//G237: bool_option(82)      => gc_opt(131).
	p(NT(82), (NT(131)));
//G238: __E_bool_option_52(134) => 'd'.
	p(NT(134), (T(21)));
//G239: __E_bool_option_52(134) => 'd' 'e' 'b' 'u' 'g'.
	p(NT(134), (T(21)+T(6)+T(19)+T(16)+T(11)));
//G240: debug_opt(133)       => __E_bool_option_52(134).
	p(NT(133), (NT(134)));
//G241: bool_option(82)      => debug_opt(133).
	p(NT(82), (NT(133)));
//G242: __E_bool_option_53(136) => 'a' 'd'.
	p(NT(136), (T(3)+T(21)));
//G243: __E_bool_option_53(136) => 'a' 'u' 't' 'o' '-' 'd' 'i' 's' 'a' 'm' 'b' 'i' 'g' 'u' 'a' 't' 'e'.
	p(NT(136), (T(3)+T(16)+T(14)+T(20)+T(15)+T(21)+T(9)+T(5)+T(3)+T(12)+T(19)+T(9)+T(11)+T(16)+T(3)+T(14)+T(6)));
//G244: auto_disambiguate_opt(135) => __E_bool_option_53(136).
	p(NT(135), (NT(136)));
//G245: bool_option(82)      => auto_disambiguate_opt(135).
	p(NT(82), (NT(135)));
//G246: __E_bool_option_54(138) => 't' 't'.
	p(NT(138), (T(14)+T(14)));
//G247: __E_bool_option_54(138) => 't' 'r' 'i' 'm' '-' 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
	p(NT(138), (T(14)+T(4)+T(9)+T(12)+T(15)+T(14)+T(6)+T(4)+T(12)+T(9)+T(13)+T(3)+T(10)+T(5)));
//G248: trim_terminals_opt(137) => __E_bool_option_54(138).
	p(NT(137), (NT(138)));
//G249: bool_option(82)      => trim_terminals_opt(137).
	p(NT(82), (NT(137)));
//G250: __E_bool_option_55(140) => 'i' 'c' 'c'.
	p(NT(140), (T(9)+T(17)+T(17)));
//G251: __E_bool_option_55(140) => 'i' 'n' 'l' 'i' 'n' 'e' '-' 'c' 'c'.
	p(NT(140), (T(9)+T(13)+T(10)+T(9)+T(13)+T(6)+T(15)+T(17)+T(17)));
//G252: __E_bool_option_55(140) => 'i' 'n' 'l' 'i' 'n' 'e' '-' 'c' 'h' 'a' 'r' '-' 'c' 'l' 'a' 's' 's' 'e' 's'.
	p(NT(140), (T(9)+T(13)+T(10)+T(9)+T(13)+T(6)+T(15)+T(17)+T(18)+T(3)+T(4)+T(15)+T(17)+T(10)+T(3)+T(5)+T(5)+T(6)+T(5)));
//G253: inline_cc_opt(139)   => __E_bool_option_55(140).
	p(NT(139), (NT(140)));
//G254: bool_option(82)      => inline_cc_opt(139).
	p(NT(82), (NT(139)));
//G255: __E_bool_option_56(142) => 'd' 'c' 'c'.
	p(NT(142), (T(21)+T(17)+T(17)));
//G256: __E_bool_option_56(142) => 'd' 'e' 'r' 'i' 'v' 'e' '-' 'c' 'h' 'a' 'r' '-' 'c' 'l' 'a' 's' 's' 'e' 's'.
	p(NT(142), (T(21)+T(6)+T(4)+T(9)+T(22)+T(6)+T(15)+T(17)+T(18)+T(3)+T(4)+T(15)+T(17)+T(10)+T(3)+T(5)+T(5)+T(6)+T(5)));
//G257: derive_char_classes_opt(141) => __E_bool_option_56(142).
	p(NT(141), (NT(142)));
//G258: bool_option(82)      => derive_char_classes_opt(141).
	p(NT(82), (NT(141)));
//G259: __E_list_option_57(144) => 'n' 'd'.
	p(NT(144), (T(13)+T(21)));
//G260: __E_list_option_57(144) => 'n' 'd' 'l'.
	p(NT(144), (T(13)+T(21)+T(10)));
//G261: __E_list_option_57(144) => 'n' 'o' 'd' 'i' 's' 'a' 'm' 'b' 'i' 'g' '-' 'l' 'i' 's' 't'.
	p(NT(144), (T(13)+T(20)+T(21)+T(9)+T(5)+T(3)+T(12)+T(19)+T(9)+T(11)+T(15)+T(10)+T(9)+T(5)+T(14)));
//G262: nodisambig_list_opt(143) => __E_list_option_57(144).
	p(NT(143), (NT(144)));
//G263: list_option(73)      => nodisambig_list_opt(143).
	p(NT(73), (NT(143)));
//G264: __E_list_option_58(146) => 'g' 'u' 'a' 'r' 'd' 's'.
	p(NT(146), (T(11)+T(16)+T(3)+T(4)+T(21)+T(5)));
//G265: __E_list_option_58(146) => 'e' 'n' 'a' 'b' 'l' 'e' 'd' '-' 'p' 'r' 'o' 'd' 'u' 'c' 't' 'i' 'o' 'n' 's'.
	p(NT(146), (T(6)+T(13)+T(3)+T(19)+T(10)+T(6)+T(21)+T(15)+T(2)+T(4)+T(20)+T(21)+T(16)+T(17)+T(14)+T(9)+T(20)+T(13)+T(5)));
//G266: enabled_prods_opt(145) => __E_list_option_58(146).
	p(NT(145), (NT(146)));
//G267: list_option(73)      => enabled_prods_opt(145).
	p(NT(73), (NT(145)));
//G268: trim_opt(147)        => 't' 'r' 'i' 'm'.
	p(NT(147), (T(14)+T(4)+T(9)+T(12)));
//G269: list_option(73)      => trim_opt(147).
	p(NT(73), (NT(147)));
//G270: __E_list_option_59(149) => 't' 'c'.
	p(NT(149), (T(14)+T(17)));
//G271: __E_list_option_59(149) => 't' 'r' 'i' 'm' '-' 'c' 'h' 'i' 'l' 'd' 'r' 'e' 'n'.
	p(NT(149), (T(14)+T(4)+T(9)+T(12)+T(15)+T(17)+T(18)+T(9)+T(10)+T(21)+T(4)+T(6)+T(13)));
//G272: trim_children_opt(148) => __E_list_option_59(149).
	p(NT(148), (NT(149)));
//G273: list_option(73)      => trim_children_opt(148).
	p(NT(73), (NT(148)));
//G274: __E_list_option_60(151) => 't' 'c' 't'.
	p(NT(151), (T(14)+T(17)+T(14)));
//G275: __E_list_option_60(151) => 't' 'r' 'i' 'm' '-' 'c' 'h' 'i' 'l' 'd' 'r' 'e' 'n' '-' 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
	p(NT(151), (T(14)+T(4)+T(9)+T(12)+T(15)+T(17)+T(18)+T(9)+T(10)+T(21)+T(4)+T(6)+T(13)+T(15)+T(14)+T(6)+T(4)+T(12)+T(9)+T(13)+T(3)+T(10)+T(5)));
//G276: trim_children_terminals_opt(150) => __E_list_option_60(151).
	p(NT(150), (NT(151)));
//G277: list_option(73)      => trim_children_terminals_opt(150).
	p(NT(73), (NT(150)));
//G278: __E_treepaths_option_61(153) => 'i'.
	p(NT(153), (T(9)));
//G279: __E_treepaths_option_61(153) => 'i' 'n' 'l' 'i' 'n' 'e'.
	p(NT(153), (T(9)+T(13)+T(10)+T(9)+T(13)+T(6)));
//G280: inline_opt(152)      => __E_treepaths_option_61(153).
	p(NT(152), (NT(153)));
//G281: treepaths_option(75) => inline_opt(152).
	p(NT(75), (NT(152)));
//G282: __E_bool_value_62(155) => 't'.
	p(NT(155), (T(14)));
//G283: __E_bool_value_62(155) => 't' 'r' 'u' 'e'.
	p(NT(155), (T(14)+T(4)+T(16)+T(6)));
//G284: __E_bool_value_62(155) => 'o' 'n'.
	p(NT(155), (T(20)+T(13)));
//G285: __E_bool_value_62(155) => '1'.
	p(NT(155), (T(29)));
//G286: __E_bool_value_62(155) => 'y'.
	p(NT(155), (T(28)));
//G287: __E_bool_value_62(155) => 'y' 'e' 's'.
	p(NT(155), (T(28)+T(6)+T(5)));
//G288: true_value(154)      => __E_bool_value_62(155).
	p(NT(154), (NT(155)));
//G289: bool_value(90)       => true_value(154).
	p(NT(90), (NT(154)));
//G290: __E_bool_value_63(157) => 'f'.
	p(NT(157), (T(7)));
//G291: __E_bool_value_63(157) => 'f' 'a' 'l' 's' 'e'.
	p(NT(157), (T(7)+T(3)+T(10)+T(5)+T(6)));
//G292: __E_bool_value_63(157) => 'o' 'f' 'f'.
	p(NT(157), (T(20)+T(7)+T(7)));
//G293: __E_bool_value_63(157) => '0'.
	p(NT(157), (T(30)));
//G294: __E_bool_value_63(157) => 'n'.
	p(NT(157), (T(13)));
//G295: __E_bool_value_63(157) => 'n' 'o'.
	p(NT(157), (T(13)+T(20)));
//G296: false_value(156)     => __E_bool_value_63(157).
	p(NT(156), (NT(157)));
//G297: bool_value(90)       => false_value(156).
	p(NT(90), (NT(156)));
//G298: __E_error_verbosity_64(159) => 'b'.
	p(NT(159), (T(19)));
//G299: __E_error_verbosity_64(159) => 'b' 'a' 's' 'i' 'c'.
	p(NT(159), (T(19)+T(3)+T(5)+T(9)+T(17)));
//G300: basic_sym(158)       => __E_error_verbosity_64(159).
	p(NT(158), (NT(159)));
//G301: error_verbosity(99)  => basic_sym(158).
	p(NT(99), (NT(158)));
//G302: __E_error_verbosity_65(161) => 'd'.
	p(NT(161), (T(21)));
//G303: __E_error_verbosity_65(161) => 'd' 'e' 't' 'a' 'i' 'l' 'e' 'd'.
	p(NT(161), (T(21)+T(6)+T(14)+T(3)+T(9)+T(10)+T(6)+T(21)));
//G304: detailed_sym(160)    => __E_error_verbosity_65(161).
	p(NT(160), (NT(161)));
//G305: error_verbosity(99)  => detailed_sym(160).
	p(NT(99), (NT(160)));
//G306: __E_error_verbosity_66(163) => 'r'.
	p(NT(163), (T(4)));
//G307: __E_error_verbosity_66(163) => 'r' 'c'.
	p(NT(163), (T(4)+T(17)));
//G308: __E_error_verbosity_66(163) => 'r' 'o' 'o' 't' '-' 'c' 'a' 'u' 's' 'e'.
	p(NT(163), (T(4)+T(20)+T(20)+T(14)+T(15)+T(17)+T(3)+T(16)+T(5)+T(6)));
//G309: root_cause_sym(162)  => __E_error_verbosity_66(163).
	p(NT(162), (NT(163)));
//G310: error_verbosity(99)  => root_cause_sym(162).
	p(NT(99), (NT(162)));
//G311: __E_symbol_67(164)   => alpha(3).
	p(NT(164), (NT(3)));
//G312: __E_symbol_67(164)   => '_'.
	p(NT(164), (T(31)));
//G313: __E_symbol_68(165)   => alnum(2).
	p(NT(165), (NT(2)));
//G314: __E_symbol_68(165)   => '_'.
	p(NT(165), (T(31)));
//G315: __E_symbol_69(166)   => null.
	p(NT(166), (nul));
//G316: __E_symbol_69(166)   => __E_symbol_69(166) __E_symbol_68(165).
	p(NT(166), (NT(166)+NT(165)));
//G317: symbol(45)           => __E_symbol_67(164) __E_symbol_69(166).
	p(NT(45), (NT(164)+NT(166)));
//G318: __E_symbol_list_70(167) => _(8) ',' _(8) symbol(45).
	p(NT(167), (NT(8)+T(32)+NT(8)+NT(45)));
//G319: __E_symbol_list_71(168) => null.
	p(NT(168), (nul));
//G320: __E_symbol_list_71(168) => __E_symbol_list_71(168) __E_symbol_list_70(167).
	p(NT(168), (NT(168)+NT(167)));
//G321: symbol_list(74)      => symbol(45) __E_symbol_list_71(168).
	p(NT(74), (NT(45)+NT(168)));
//G322: __E_treepath_72(170) => _(8) '>' _(8) symbol(45).
	p(NT(170), (NT(8)+T(33)+NT(8)+NT(45)));
//G323: __E_treepath_73(171) => null.
	p(NT(171), (nul));
//G324: __E_treepath_73(171) => __E_treepath_73(171) __E_treepath_72(170).
	p(NT(171), (NT(171)+NT(170)));
//G325: treepath(169)        => symbol(45) __E_treepath_73(171).
	p(NT(169), (NT(45)+NT(171)));
//G326: __E_treepath_list_74(172) => _(8) ',' _(8) treepath(169).
	p(NT(172), (NT(8)+T(32)+NT(8)+NT(169)));
//G327: __E_treepath_list_75(173) => null.
	p(NT(173), (nul));
//G328: __E_treepath_list_75(173) => __E_treepath_list_75(173) __E_treepath_list_74(172).
	p(NT(173), (NT(173)+NT(172)));
//G329: treepath_list(76)    => treepath(169) __E_treepath_list_75(173).
	p(NT(76), (NT(169)+NT(173)));
//G330: filename(39)         => quoted_string(102).
	p(NT(39), (NT(102)));
//G331: __E_quoted_string_76(175) => null.
	p(NT(175), (nul));
//G332: __E_quoted_string_76(175) => __E_quoted_string_76(175) quoted_string_char(174).
	p(NT(175), (NT(175)+NT(174)));
//G333: quoted_string(102)   => '"' __E_quoted_string_76(175) '"'.
	p(NT(102), (T(34)+NT(175)+T(34)));
//G334: quoted_string_char(174) => unescaped_s(176).
	p(NT(174), (NT(176)));
//G335: quoted_string_char(174) => escaped_s(177).
	p(NT(174), (NT(177)));
//G336: __E_unescaped_s_77(178) => space(4).
	p(NT(178), (NT(4)));
//G337: __E_unescaped_s_77(178) => printable(5).
	p(NT(178), (NT(5)));
//G338: __E_unescaped_s_78(179) => '"'.
	p(NT(179), (T(34)));
//G339: __E_unescaped_s_78(179) => '\\'.
	p(NT(179), (T(35)));
//G340: __N_0(193)           => __E_unescaped_s_78(179).
	p(NT(193), (NT(179)));
//G341: unescaped_s(176)     => __E_unescaped_s_77(178) & ~( __N_0(193) ).	 # conjunctive
	p(NT(176), (NT(178)) & ~(NT(193)));
//G342: __E_escaped_s_79(180) => '"'.
	p(NT(180), (T(34)));
//G343: __E_escaped_s_79(180) => escape_char(181).
	p(NT(180), (NT(181)));
//G344: escaped_s(177)       => '\\' __E_escaped_s_79(180).
	p(NT(177), (T(35)+NT(180)));
//G345: escape_char(181)     => 'a'.
	p(NT(181), (T(3)));
//G346: escape_char(181)     => 'b'.
	p(NT(181), (T(19)));
//G347: escape_char(181)     => 'f'.
	p(NT(181), (T(7)));
//G348: escape_char(181)     => 'n'.
	p(NT(181), (T(13)));
//G349: escape_char(181)     => 'r'.
	p(NT(181), (T(4)));
//G350: escape_char(181)     => 't'.
	p(NT(181), (T(14)));
//G351: escape_char(181)     => 'v'.
	p(NT(181), (T(22)));
//G352: escape_char(181)     => '\\'.
	p(NT(181), (T(35)));
//G353: escape_char(181)     => '/'.
	p(NT(181), (T(36)));
//G354: escape_char(181)     => esc_hex(182).
	p(NT(181), (NT(182)));
//G355: escape_char(181)     => esc_u4(183).
	p(NT(181), (NT(183)));
//G356: escape_char(181)     => esc_U8(184).
	p(NT(181), (NT(184)));
//G357: __E_esc_hex_80(185)  => 'x'.
	p(NT(185), (T(25)));
//G358: __E_esc_hex_80(185)  => 'X'.
	p(NT(185), (T(37)));
//G359: __E_esc_hex_81(186)  => xdigit(6).
	p(NT(186), (NT(6)));
//G360: __E_esc_hex_81(186)  => null.
	p(NT(186), (nul));
//G361: esc_hex(182)         => __E_esc_hex_80(185) xdigit(6) __E_esc_hex_81(186).
	p(NT(182), (NT(185)+NT(6)+NT(186)));
//G362: esc_u4(183)          => 'u' xdigit(6) xdigit(6) xdigit(6) xdigit(6).
	p(NT(183), (T(16)+NT(6)+NT(6)+NT(6)+NT(6)));
//G363: esc_U8(184)          => 'U' xdigit(6) xdigit(6) xdigit(6) xdigit(6) xdigit(6) xdigit(6) xdigit(6) xdigit(6).
	p(NT(184), (T(38)+NT(6)+NT(6)+NT(6)+NT(6)+NT(6)+NT(6)+NT(6)+NT(6)));
//G364: __E___82(187)        => __(35).
	p(NT(187), (NT(35)));
//G365: __E___82(187)        => null.
	p(NT(187), (nul));
//G366: _(8)                 => __E___82(187).
	p(NT(8), (NT(187)));
//G367: __E____83(188)       => space(4).
	p(NT(188), (NT(4)));
//G368: __E____83(188)       => comment(189).
	p(NT(188), (NT(189)));
//G369: __(35)               => __E____83(188) _(8).
	p(NT(35), (NT(188)+NT(8)));
//G370: __E_comment_84(190)  => printable(5).
	p(NT(190), (NT(5)));
//G371: __E_comment_84(190)  => '\t'.
	p(NT(190), (T(27)));
//G372: __E_comment_85(191)  => null.
	p(NT(191), (nul));
//G373: __E_comment_85(191)  => __E_comment_85(191) __E_comment_84(190).
	p(NT(191), (NT(191)+NT(190)));
//G374: __E_comment_86(192)  => '\r'.
	p(NT(192), (T(39)));
//G375: __E_comment_86(192)  => '\n'.
	p(NT(192), (T(40)));
//G376: __E_comment_86(192)  => eof(1).
	p(NT(192), (NT(1)));
//G377: comment(189)         => '#' __E_comment_85(191) __E_comment_86(192).
	p(NT(189), (T(41)+NT(191)+NT(192)));
	#undef T
	#undef NT
	return loaded = true, p;
}
#else
idni::prods<char_type, terminal_type>& productions();
#endif

inline ::idni::grammar<char_type, terminal_type> grammar(
	nts, productions(), start_symbol, char_classes, grammar_options);

} // namespace tgf_repl_parser_data

struct tgf_repl_parser_nonterminals {
	enum nonterminal {
		nul, eof, alnum, alpha, space, printable, xdigit, start, _, __E_start_0, 
		statement, __E___E_start_0_1, __E___E_start_0_2, grammar_cmd, igrammar_cmd, unreachable_cmd, reload_cmd, load_cmd, start_cmd, help_cmd, 
		version_cmd, license_cmd, quit_cmd, clear_cmd, get_cmd, set_cmd, toggle_cmd, enable_cmd, disable_cmd, add_cmd, 
		del_cmd, parse_file_cmd, parse_cmd, parse_sym, __E_parse_cmd_3, __, parse_input, parse_file_sym, __E_parse_file_cmd_4, filename, 
		grammar_sym, __E_grammar_cmd_5, igrammar_sym, __E_igrammar_cmd_6, __E_igrammar_cmd_7, symbol, start_sym, __E_start_cmd_8, __E_start_cmd_9, unreachable_sym, 
		__E_unreachable_cmd_10, __E_unreachable_cmd_11, reload_sym, __E_reload_cmd_12, load_sym, __E_load_cmd_13, help_sym, __E_help_cmd_14, __E_help_cmd_15, help_arg, 
		version_sym, __E_version_cmd_16, license_sym, __E_license_cmd_17, quit_sym, __E_quit_cmd_18, clear_sym, __E_clear_cmd_19, get_sym, __E_get_cmd_20, 
		option, add_sym, __E_add_cmd_21, list_option, symbol_list, treepaths_option, treepath_list, del_sym, __E_del_cmd_22, __E_del_cmd_23, 
		toggle_sym, __E_toggle_cmd_24, bool_option, enable_sym, __E_enable_cmd_25, disable_sym, __E_disable_cmd_26, set_sym, __E_set_cmd_27, __E___E_set_cmd_27_28, 
		bool_value, __E___E_set_cmd_27_29, __E___E___E_set_cmd_27_29_30, __E___E___E_set_cmd_27_29_31, __E___E_set_cmd_27_32, __E___E___E_set_cmd_27_32_33, __E___E___E_set_cmd_27_32_34, enum_ev_option, __E___E_set_cmd_27_35, error_verbosity, 
		symbol_option, __E___E_set_cmd_27_36, quoted_string, parse_input_char_seq, __E_parse_input_37, __E_parse_input_38, error_verbosity_opt, __E_enum_ev_option_39, start_opt, status_opt, 
		__E_bool_option_40, colors_opt, __E_bool_option_41, print_ambiguity_opt, __E_bool_option_42, print_graphs_opt, __E_bool_option_43, print_rules_opt, __E_bool_option_44, print_facts_opt, 
		__E_bool_option_45, print_terminals_opt, __E_bool_option_46, measure_parsing_opt, __E_bool_option_47, measure_each_pos_opt, __E_bool_option_48, measure_forest_opt, __E_bool_option_49, measure_preprocess_opt, 
		__E_bool_option_50, gc_opt, __E_bool_option_51, debug_opt, __E_bool_option_52, auto_disambiguate_opt, __E_bool_option_53, trim_terminals_opt, __E_bool_option_54, inline_cc_opt, 
		__E_bool_option_55, derive_char_classes_opt, __E_bool_option_56, nodisambig_list_opt, __E_list_option_57, enabled_prods_opt, __E_list_option_58, trim_opt, trim_children_opt, __E_list_option_59, 
		trim_children_terminals_opt, __E_list_option_60, inline_opt, __E_treepaths_option_61, true_value, __E_bool_value_62, false_value, __E_bool_value_63, basic_sym, __E_error_verbosity_64, 
		detailed_sym, __E_error_verbosity_65, root_cause_sym, __E_error_verbosity_66, __E_symbol_67, __E_symbol_68, __E_symbol_69, __E_symbol_list_70, __E_symbol_list_71, treepath, 
		__E_treepath_72, __E_treepath_73, __E_treepath_list_74, __E_treepath_list_75, quoted_string_char, __E_quoted_string_76, unescaped_s, escaped_s, __E_unescaped_s_77, __E_unescaped_s_78, 
		__E_escaped_s_79, escape_char, esc_hex, esc_u4, esc_U8, __E_esc_hex_80, __E_esc_hex_81, __E___82, __E____83, comment, 
		__E_comment_84, __E_comment_85, __E_comment_86, __N_0, 
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
