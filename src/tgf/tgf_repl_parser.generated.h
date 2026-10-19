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

inline static constexpr size_t nt_bits = 7;

inline const std::vector<std::string>& symbol_names() {
	static const std::vector<std::string> v{
	"", "eof", "alnum", "alpha", "space", "printable", "xdigit", "start", "_", "__E_start_0", 
	"statement", "__E___E_start_0_1", "__E___E_start_0_2", "grammar_cmd", "igrammar_cmd", "unreachable_cmd", "reload_cmd", "load_cmd", "start_cmd", "help_cmd", 
	"version_cmd", "license_cmd", "quit_cmd", "clear_cmd", "get_cmd", "set_cmd", "toggle_cmd", "enable_cmd", "disable_cmd", "add_cmd", 
	"del_cmd", "parse_file_cmd", "parse_cmd", "parse_sym", "__E_parse_cmd_3", "__", "parse_input", "parse_file_sym", "__E_parse_file_cmd_4", "filename", 
	"grammar_sym", "__E_grammar_cmd_5", "igrammar_sym", "__E_igrammar_cmd_6", "__E_igrammar_cmd_7", "symbol", "start_sym", "__E_start_cmd_8", "__E_start_cmd_9", "unreachable_sym", 
	"__E_unreachable_cmd_10", "__E_unreachable_cmd_11", "reload_sym", "__E_reload_cmd_12", "load_sym", "__E_load_cmd_13", "help_sym", "__E_help_cmd_14", "__E_help_cmd_15", "help_arg", 
	"version_sym", "__E_version_cmd_16", "license_sym", "__E_license_cmd_17", "quit_sym", "__E_quit_cmd_18", "clear_sym", "__E_clear_cmd_19", "get_sym", "__E_get_cmd_20", 
	"option_name", "set_sym", "__E_set_cmd_21", "__E_set_cmd_22", "option_value", "add_sym", "del_sym", "__E_del_cmd_23", "toggle_sym", "__E_toggle_cmd_24", 
	"enable_sym", "__E_enable_cmd_25", "disable_sym", "__E_disable_cmd_26", "quoted_string", "parse_input_char_seq", "__E_parse_input_27", "__E_parse_input_28", "__E_option_name_29", "__E_option_name_30", 
	"__E_option_name_31", "option_word", "__E_option_value_32", "__E_option_value_33", "__E_option_word_34", "option_char", "__E_option_word_35", "__E_option_char_36", "__E_symbol_37", "__E_symbol_38", 
	"__E_symbol_39", "quoted_string_char", "__E_quoted_string_40", "unescaped_s", "escaped_s", "__E_unescaped_s_41", "__E_unescaped_s_42", "__E_escaped_s_43", "escape_char", "esc_hex", 
	"esc_u4", "esc_U8", "__E_esc_hex_44", "__E_esc_hex_45", "__E___46", "__E____47", "comment", "__E_comment_48", "__E_comment_49", "__E_comment_50", 
	"__N_0", "__N_1", 
};
	return v;
}

inline ::idni::nonterminals<char_type, terminal_type>& nts() {
	static ::idni::nonterminals<char_type, terminal_type> v{
		symbol_names() };
	return v;
}

inline std::vector<terminal_type>& terminals() {
	static std::vector<terminal_type> v{
	U'\0', U'.', U'p', U'a', U'r', U's', U'e', U'f', U' ', 
	U'i', U'l', U'g', U'm', U'n', U't', U'-', U'u', U'c', U'h', 
	U'b', U'o', U'd', U'v', U'L', U'q', U'x', U'=', U'\t', U'_', 
	U'#', U'"', U'\\', U'/', U'X', U'U', U'\r', U'\n', 
};
	return v;
}

inline ::idni::char_class_fns<terminal_type>& char_classes() {
	static ::idni::char_class_fns<terminal_type> v =
	::idni::predefined_char_classes<char_type, terminal_type>({
		"eof",
		"alnum",
		"alpha",
		"space",
		"xdigit",
	}, {
		{ "unicode_printable", "printable" },
	}, nts());
	return v;
}

inline ::idni::grammar<char_type, terminal_type>::options&
	grammar_options()
{
	static struct ::idni::grammar<char_type, terminal_type>::options v
{
	.transform_negation = false,
	.auto_disambiguate = true,
	.shaping = {
		.to_trim = {
			8, 35
		},
		.trim_terminals = true,
		.dont_trim_terminals_of = {
			45, 70, 84, 85, 95, 103, 104, 108, 109, 110,
			111
		},
		.inline_char_classes = true
	}
};
	return v;
}

inline auto make_parser_options() {
	auto o = ::idni::default_parser_options<char_type, terminal_type>();
	o.codec.decode = idni::utf8_to_u32_conv;
	o.codec.encode = idni::u32_to_utf8_conv;
	return o;
}

inline ::idni::prods<char_type, terminal_type>& start_symbol() {
	static ::idni::prods<char_type, terminal_type> v{ nts()(7) };
	return v;
}

#ifdef TAU_PARSER_BUILD_HEADER_ONLY
inline idni::prods<char_type, terminal_type>& productions() {
	static idni::prods<char_type, terminal_type> p = [] {
	idni::prods<char_type, terminal_type>
		p, nul(idni::lit<char_type, terminal_type>{});
	const auto& ts = terminals();
	auto& ns = nts();
	#define  T(x) (idni::prods<char_type, terminal_type>{ ts[x] })
	#define NT(x) (idni::prods<char_type, terminal_type>{ ns(x) })
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
//G91:  __E_get_cmd_20(69)   => __(35) option_name(70).
	p(NT(69), (NT(35)+NT(70)));
//G92:  __E_get_cmd_20(69)   => null.
	p(NT(69), (nul));
//G93:  get_cmd(24)          => get_sym(68) __E_get_cmd_20(69).
	p(NT(24), (NT(68)+NT(69)));
//G94:  set_sym(71)          => 's' 'e' 't'.
	p(NT(71), (T(5)+T(6)+T(14)));
//G95:  __E_set_cmd_21(72)   => _(8) '=' _(8).
	p(NT(72), (NT(8)+T(26)+NT(8)));
//G96:  __E_set_cmd_21(72)   => __(35).
	p(NT(72), (NT(35)));
//G97:  __E_set_cmd_22(73)   => option_value(74).
	p(NT(73), (NT(74)));
//G98:  __E_set_cmd_22(73)   => null.
	p(NT(73), (nul));
//G99:  set_cmd(25)          => set_sym(71) __(35) option_name(70) __E_set_cmd_21(72) __E_set_cmd_22(73).
	p(NT(25), (NT(71)+NT(35)+NT(70)+NT(72)+NT(73)));
//G100: add_sym(75)          => 'a' 'd' 'd'.
	p(NT(75), (T(3)+T(21)+T(21)));
//G101: add_cmd(29)          => add_sym(75) __(35) option_name(70) __(35) option_value(74).
	p(NT(29), (NT(75)+NT(35)+NT(70)+NT(35)+NT(74)));
//G102: __E_del_cmd_23(77)   => 'd' 'e' 'l'.
	p(NT(77), (T(21)+T(6)+T(10)));
//G103: __E_del_cmd_23(77)   => 'd' 'e' 'l' 'e' 't' 'e'.
	p(NT(77), (T(21)+T(6)+T(10)+T(6)+T(14)+T(6)));
//G104: __E_del_cmd_23(77)   => 'r' 'm'.
	p(NT(77), (T(4)+T(12)));
//G105: __E_del_cmd_23(77)   => 'r' 'e' 'm'.
	p(NT(77), (T(4)+T(6)+T(12)));
//G106: __E_del_cmd_23(77)   => 'r' 'e' 'm' 'o' 'v' 'e'.
	p(NT(77), (T(4)+T(6)+T(12)+T(20)+T(22)+T(6)));
//G107: del_sym(76)          => __E_del_cmd_23(77).
	p(NT(76), (NT(77)));
//G108: del_cmd(30)          => del_sym(76) __(35) option_name(70) __(35) option_value(74).
	p(NT(30), (NT(76)+NT(35)+NT(70)+NT(35)+NT(74)));
//G109: __E_toggle_cmd_24(79) => 't' 'o' 'g'.
	p(NT(79), (T(14)+T(20)+T(11)));
//G110: __E_toggle_cmd_24(79) => 't' 'o' 'g' 'g' 'l' 'e'.
	p(NT(79), (T(14)+T(20)+T(11)+T(11)+T(10)+T(6)));
//G111: toggle_sym(78)       => __E_toggle_cmd_24(79).
	p(NT(78), (NT(79)));
//G112: toggle_cmd(26)       => toggle_sym(78) __(35) option_name(70).
	p(NT(26), (NT(78)+NT(35)+NT(70)));
//G113: __E_enable_cmd_25(81) => 'e' 'n'.
	p(NT(81), (T(6)+T(13)));
//G114: __E_enable_cmd_25(81) => 'e' 'n' 'a' 'b' 'l' 'e'.
	p(NT(81), (T(6)+T(13)+T(3)+T(19)+T(10)+T(6)));
//G115: enable_sym(80)       => __E_enable_cmd_25(81).
	p(NT(80), (NT(81)));
//G116: enable_cmd(27)       => enable_sym(80) __(35) option_name(70).
	p(NT(27), (NT(80)+NT(35)+NT(70)));
//G117: __E_disable_cmd_26(83) => 'd' 'i' 's'.
	p(NT(83), (T(21)+T(9)+T(5)));
//G118: __E_disable_cmd_26(83) => 'd' 'i' 's' 'a' 'b' 'l' 'e'.
	p(NT(83), (T(21)+T(9)+T(5)+T(3)+T(19)+T(10)+T(6)));
//G119: disable_sym(82)      => __E_disable_cmd_26(83).
	p(NT(82), (NT(83)));
//G120: disable_cmd(28)      => disable_sym(82) __(35) option_name(70).
	p(NT(28), (NT(82)+NT(35)+NT(70)));
//G121: parse_input(36)      => quoted_string(84).
	p(NT(36), (NT(84)));
//G122: __E_parse_input_27(86) => printable(5).
	p(NT(86), (NT(5)));
//G123: __E_parse_input_27(86) => '\t'.
	p(NT(86), (T(27)));
//G124: __E_parse_input_28(87) => __E_parse_input_27(86).
	p(NT(87), (NT(86)));
//G125: __E_parse_input_28(87) => __E_parse_input_28(87) __E_parse_input_27(86).
	p(NT(87), (NT(87)+NT(86)));
//G126: parse_input_char_seq(85) => __E_parse_input_28(87).
	p(NT(85), (NT(87)));
//G127: parse_input(36)      => parse_input_char_seq(85).
	p(NT(36), (NT(85)));
//G128: help_arg(59)         => grammar_sym(40).
	p(NT(59), (NT(40)));
//G129: help_arg(59)         => igrammar_sym(42).
	p(NT(59), (NT(42)));
//G130: help_arg(59)         => unreachable_sym(49).
	p(NT(59), (NT(49)));
//G131: help_arg(59)         => start_sym(46).
	p(NT(59), (NT(46)));
//G132: help_arg(59)         => parse_sym(33).
	p(NT(59), (NT(33)));
//G133: help_arg(59)         => parse_file_sym(37).
	p(NT(59), (NT(37)));
//G134: help_arg(59)         => load_sym(54).
	p(NT(59), (NT(54)));
//G135: help_arg(59)         => reload_sym(52).
	p(NT(59), (NT(52)));
//G136: help_arg(59)         => clear_sym(66).
	p(NT(59), (NT(66)));
//G137: help_arg(59)         => help_sym(56).
	p(NT(59), (NT(56)));
//G138: help_arg(59)         => quit_sym(64).
	p(NT(59), (NT(64)));
//G139: help_arg(59)         => version_sym(60).
	p(NT(59), (NT(60)));
//G140: help_arg(59)         => license_sym(62).
	p(NT(59), (NT(62)));
//G141: help_arg(59)         => get_sym(68).
	p(NT(59), (NT(68)));
//G142: help_arg(59)         => set_sym(71).
	p(NT(59), (NT(71)));
//G143: help_arg(59)         => add_sym(75).
	p(NT(59), (NT(75)));
//G144: help_arg(59)         => del_sym(76).
	p(NT(59), (NT(76)));
//G145: help_arg(59)         => toggle_sym(78).
	p(NT(59), (NT(78)));
//G146: help_arg(59)         => enable_sym(80).
	p(NT(59), (NT(80)));
//G147: help_arg(59)         => disable_sym(82).
	p(NT(59), (NT(82)));
//G148: __E_option_name_29(88) => alpha(3).
	p(NT(88), (NT(3)));
//G149: __E_option_name_29(88) => '_'.
	p(NT(88), (T(28)));
//G150: __E_option_name_30(89) => alnum(2).
	p(NT(89), (NT(2)));
//G151: __E_option_name_30(89) => '_'.
	p(NT(89), (T(28)));
//G152: __E_option_name_30(89) => '-'.
	p(NT(89), (T(15)));
//G153: __E_option_name_31(90) => null.
	p(NT(90), (nul));
//G154: __E_option_name_31(90) => __E_option_name_31(90) __E_option_name_30(89).
	p(NT(90), (NT(90)+NT(89)));
//G155: option_name(70)      => __E_option_name_29(88) __E_option_name_31(90).
	p(NT(70), (NT(88)+NT(90)));
//G156: __E_option_value_32(92) => __(35) option_word(91).
	p(NT(92), (NT(35)+NT(91)));
//G157: __E_option_value_33(93) => null.
	p(NT(93), (nul));
//G158: __E_option_value_33(93) => __E_option_value_33(93) __E_option_value_32(92).
	p(NT(93), (NT(93)+NT(92)));
//G159: option_value(74)     => option_word(91) __E_option_value_33(93).
	p(NT(74), (NT(91)+NT(93)));
//G160: __E_option_word_34(94) => quoted_string(84).
	p(NT(94), (NT(84)));
//G161: __E_option_word_34(94) => option_char(95).
	p(NT(94), (NT(95)));
//G162: __E_option_word_35(96) => __E_option_word_34(94).
	p(NT(96), (NT(94)));
//G163: __E_option_word_35(96) => __E_option_word_35(96) __E_option_word_34(94).
	p(NT(96), (NT(96)+NT(94)));
//G164: option_word(91)      => __E_option_word_35(96).
	p(NT(91), (NT(96)));
//G165: __E_option_char_36(97) => space(4).
	p(NT(97), (NT(4)));
//G166: __E_option_char_36(97) => '.'.
	p(NT(97), (T(1)));
//G167: __E_option_char_36(97) => '='.
	p(NT(97), (T(26)));
//G168: __E_option_char_36(97) => '#'.
	p(NT(97), (T(29)));
//G169: __E_option_char_36(97) => '"'.
	p(NT(97), (T(30)));
//G170: __N_0(120)           => __E_option_char_36(97).
	p(NT(120), (NT(97)));
//G171: option_char(95)      => printable(5) & ~( __N_0(120) ).	 # conjunctive
	p(NT(95), (NT(5)) & ~(NT(120)));
//G172: __E_symbol_37(98)    => alpha(3).
	p(NT(98), (NT(3)));
//G173: __E_symbol_37(98)    => '_'.
	p(NT(98), (T(28)));
//G174: __E_symbol_38(99)    => alnum(2).
	p(NT(99), (NT(2)));
//G175: __E_symbol_38(99)    => '_'.
	p(NT(99), (T(28)));
//G176: __E_symbol_39(100)   => null.
	p(NT(100), (nul));
//G177: __E_symbol_39(100)   => __E_symbol_39(100) __E_symbol_38(99).
	p(NT(100), (NT(100)+NT(99)));
//G178: symbol(45)           => __E_symbol_37(98) __E_symbol_39(100).
	p(NT(45), (NT(98)+NT(100)));
//G179: filename(39)         => quoted_string(84).
	p(NT(39), (NT(84)));
//G180: __E_quoted_string_40(102) => null.
	p(NT(102), (nul));
//G181: __E_quoted_string_40(102) => __E_quoted_string_40(102) quoted_string_char(101).
	p(NT(102), (NT(102)+NT(101)));
//G182: quoted_string(84)    => '"' __E_quoted_string_40(102) '"'.
	p(NT(84), (T(30)+NT(102)+T(30)));
//G183: quoted_string_char(101) => unescaped_s(103).
	p(NT(101), (NT(103)));
//G184: quoted_string_char(101) => escaped_s(104).
	p(NT(101), (NT(104)));
//G185: __E_unescaped_s_41(105) => space(4).
	p(NT(105), (NT(4)));
//G186: __E_unescaped_s_41(105) => printable(5).
	p(NT(105), (NT(5)));
//G187: __E_unescaped_s_42(106) => '"'.
	p(NT(106), (T(30)));
//G188: __E_unescaped_s_42(106) => '\\'.
	p(NT(106), (T(31)));
//G189: __N_1(121)           => __E_unescaped_s_42(106).
	p(NT(121), (NT(106)));
//G190: unescaped_s(103)     => __E_unescaped_s_41(105) & ~( __N_1(121) ).	 # conjunctive
	p(NT(103), (NT(105)) & ~(NT(121)));
//G191: __E_escaped_s_43(107) => '"'.
	p(NT(107), (T(30)));
//G192: __E_escaped_s_43(107) => escape_char(108).
	p(NT(107), (NT(108)));
//G193: escaped_s(104)       => '\\' __E_escaped_s_43(107).
	p(NT(104), (T(31)+NT(107)));
//G194: escape_char(108)     => 'a'.
	p(NT(108), (T(3)));
//G195: escape_char(108)     => 'b'.
	p(NT(108), (T(19)));
//G196: escape_char(108)     => 'f'.
	p(NT(108), (T(7)));
//G197: escape_char(108)     => 'n'.
	p(NT(108), (T(13)));
//G198: escape_char(108)     => 'r'.
	p(NT(108), (T(4)));
//G199: escape_char(108)     => 't'.
	p(NT(108), (T(14)));
//G200: escape_char(108)     => 'v'.
	p(NT(108), (T(22)));
//G201: escape_char(108)     => '\\'.
	p(NT(108), (T(31)));
//G202: escape_char(108)     => '/'.
	p(NT(108), (T(32)));
//G203: escape_char(108)     => esc_hex(109).
	p(NT(108), (NT(109)));
//G204: escape_char(108)     => esc_u4(110).
	p(NT(108), (NT(110)));
//G205: escape_char(108)     => esc_U8(111).
	p(NT(108), (NT(111)));
//G206: __E_esc_hex_44(112)  => 'x'.
	p(NT(112), (T(25)));
//G207: __E_esc_hex_44(112)  => 'X'.
	p(NT(112), (T(33)));
//G208: __E_esc_hex_45(113)  => xdigit(6).
	p(NT(113), (NT(6)));
//G209: __E_esc_hex_45(113)  => null.
	p(NT(113), (nul));
//G210: esc_hex(109)         => __E_esc_hex_44(112) xdigit(6) __E_esc_hex_45(113).
	p(NT(109), (NT(112)+NT(6)+NT(113)));
//G211: esc_u4(110)          => 'u' xdigit(6) xdigit(6) xdigit(6) xdigit(6).
	p(NT(110), (T(16)+NT(6)+NT(6)+NT(6)+NT(6)));
//G212: esc_U8(111)          => 'U' xdigit(6) xdigit(6) xdigit(6) xdigit(6) xdigit(6) xdigit(6) xdigit(6) xdigit(6).
	p(NT(111), (T(34)+NT(6)+NT(6)+NT(6)+NT(6)+NT(6)+NT(6)+NT(6)+NT(6)));
//G213: __E___46(114)        => __(35).
	p(NT(114), (NT(35)));
//G214: __E___46(114)        => null.
	p(NT(114), (nul));
//G215: _(8)                 => __E___46(114).
	p(NT(8), (NT(114)));
//G216: __E____47(115)       => space(4).
	p(NT(115), (NT(4)));
//G217: __E____47(115)       => comment(116).
	p(NT(115), (NT(116)));
//G218: __(35)               => __E____47(115) _(8).
	p(NT(35), (NT(115)+NT(8)));
//G219: __E_comment_48(117)  => printable(5).
	p(NT(117), (NT(5)));
//G220: __E_comment_48(117)  => '\t'.
	p(NT(117), (T(27)));
//G221: __E_comment_49(118)  => null.
	p(NT(118), (nul));
//G222: __E_comment_49(118)  => __E_comment_49(118) __E_comment_48(117).
	p(NT(118), (NT(118)+NT(117)));
//G223: __E_comment_50(119)  => '\r'.
	p(NT(119), (T(35)));
//G224: __E_comment_50(119)  => '\n'.
	p(NT(119), (T(36)));
//G225: __E_comment_50(119)  => eof(1).
	p(NT(119), (NT(1)));
//G226: comment(116)         => '#' __E_comment_49(118) __E_comment_50(119).
	p(NT(116), (T(29)+NT(118)+NT(119)));
	#undef T
	#undef NT
	return p;
	}();
	return p;
}
#else
idni::prods<char_type, terminal_type>& productions();
#endif

inline ::idni::grammar<char_type, terminal_type>& grammar() {
	static ::idni::grammar<char_type, terminal_type> v(nts(),
		productions(), start_symbol(), char_classes(),
		grammar_options());
	return v;
}

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
		option_name, set_sym, __E_set_cmd_21, __E_set_cmd_22, option_value, add_sym, del_sym, __E_del_cmd_23, toggle_sym, __E_toggle_cmd_24, 
		enable_sym, __E_enable_cmd_25, disable_sym, __E_disable_cmd_26, quoted_string, parse_input_char_seq, __E_parse_input_27, __E_parse_input_28, __E_option_name_29, __E_option_name_30, 
		__E_option_name_31, option_word, __E_option_value_32, __E_option_value_33, __E_option_word_34, option_char, __E_option_word_35, __E_option_char_36, __E_symbol_37, __E_symbol_38, 
		__E_symbol_39, quoted_string_char, __E_quoted_string_40, unescaped_s, escaped_s, __E_unescaped_s_41, __E_unescaped_s_42, __E_escaped_s_43, escape_char, esc_hex, 
		esc_u4, esc_U8, __E_esc_hex_44, __E_esc_hex_45, __E___46, __E____47, comment, __E_comment_48, __E_comment_49, __E_comment_50, 
		__N_0, __N_1, 
	};
};

#ifdef _MSC_VER
// the base parser puts its chart indexes on a cache line, so the padding is wanted
#	pragma warning(push)
#	pragma warning(disable : 4324)
#endif
struct tgf_repl_parser : public idni::parser<char, char32_t>, public tgf_repl_parser_nonterminals {
	static tgf_repl_parser& instance() {
		static tgf_repl_parser inst;
		return inst;
	}
	tgf_repl_parser() : idni::parser<char_type, terminal_type>(
		tgf_repl_parser_data::grammar(),
		tgf_repl_parser_data::make_parser_options()) {}
	size_t id(const std::basic_string<char_type>& name) {
		return tgf_repl_parser_data::nts().get(name);
	}
	const std::basic_string<char_type>& name(size_t id) {
		return tgf_repl_parser_data::nts().get(id);
	}
	symbol_type literal(const nonterminal& nt) {
		return symbol_type(nt, &tgf_repl_parser_data::nts());
	}
};
#ifdef _MSC_VER
#	pragma warning(pop)
#endif

#endif // __TGF_REPL_PARSER_H__
