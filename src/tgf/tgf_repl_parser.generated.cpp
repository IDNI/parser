// This file is generated from a file src/tgf/tgf_repl.tgf by
//       https://github.com/IDNI/parser/src/tgf
//
// productions() lives here so the table is compiled once, not per TU.
//
#include "parser.h"
#include "tgf_repl_parser.generated.h"

namespace tgf_repl_parser_data {

#ifndef TAU_PARSER_BUILD_HEADER_ONLY
idni::prods<char_type, terminal_type>& productions() {
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
//G2:   __E___E_start_0_2(22) => __E___E_start_0_2(22) __E___E_start_0_1(21).
	p(NT(22), (NT(22)+NT(21)));
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
//G123: __E___E___E_set_cmd_27_29_30(100) => symbol_list(82).
	p(NT(100), (NT(82)));
//G124: __E___E___E_set_cmd_27_29_30(100) => null.
	p(NT(100), (nul));
//G125: __E___E_set_cmd_27_29(99) => _(7) '=' _(7) __E___E___E_set_cmd_27_29_30(100).
	p(NT(99), (NT(7)+T(26)+NT(7)+NT(100)));
//G126: __E___E___E_set_cmd_27_29_31(101) => __(8) symbol_list(82).
	p(NT(101), (NT(8)+NT(82)));
//G127: __E___E___E_set_cmd_27_29_31(101) => null.
	p(NT(101), (nul));
//G128: __E___E_set_cmd_27_29(99) => __E___E___E_set_cmd_27_29_31(101).
	p(NT(99), (NT(101)));
//G129: __E_set_cmd_27(96)   => list_option(81) __E___E_set_cmd_27_29(99).
	p(NT(96), (NT(81)+NT(99)));
//G130: __E___E___E_set_cmd_27_32_33(103) => treepath_list(84).
	p(NT(103), (NT(84)));
//G131: __E___E___E_set_cmd_27_32_33(103) => null.
	p(NT(103), (nul));
//G132: __E___E_set_cmd_27_32(102) => _(7) '=' _(7) __E___E___E_set_cmd_27_32_33(103).
	p(NT(102), (NT(7)+T(26)+NT(7)+NT(103)));
//G133: __E___E___E_set_cmd_27_32_34(104) => __(8) treepath_list(84).
	p(NT(104), (NT(8)+NT(84)));
//G134: __E___E___E_set_cmd_27_32_34(104) => null.
	p(NT(104), (nul));
//G135: __E___E_set_cmd_27_32(102) => __E___E___E_set_cmd_27_32_34(104).
	p(NT(102), (NT(104)));
//G136: __E_set_cmd_27(96)   => treepaths_option(83) __E___E_set_cmd_27_32(102).
	p(NT(96), (NT(83)+NT(102)));
//G137: __E___E_set_cmd_27_35(106) => __(8).
	p(NT(106), (NT(8)));
//G138: __E___E_set_cmd_27_35(106) => _(7) '=' _(7).
	p(NT(106), (NT(7)+T(26)+NT(7)));
//G139: __E_set_cmd_27(96)   => enum_ev_option(105) __E___E_set_cmd_27_35(106) error_verbosity(107).
	p(NT(96), (NT(105)+NT(106)+NT(107)));
//G140: __E___E_set_cmd_27_36(109) => __(8).
	p(NT(109), (NT(8)));
//G141: __E___E_set_cmd_27_36(109) => _(7) '=' _(7).
	p(NT(109), (NT(7)+T(26)+NT(7)));
//G142: __E_set_cmd_27(96)   => symbol_option(108) __E___E_set_cmd_27_36(109) symbol(9).
	p(NT(96), (NT(108)+NT(109)+NT(9)));
//G143: set_cmd(35)          => set_sym(95) __(8) __E_set_cmd_27(96).
	p(NT(35), (NT(95)+NT(8)+NT(96)));
//G144: parse_input(45)      => quoted_string(10).
	p(NT(45), (NT(10)));
//G145: __E_parse_input_37(110) => printable(5).
	p(NT(110), (NT(5)));
//G146: __E_parse_input_37(110) => '\t'.
	p(NT(110), (T(27)));
//G147: __E_parse_input_38(111) => __E_parse_input_37(110).
	p(NT(111), (NT(110)));
//G148: __E_parse_input_38(111) => __E_parse_input_38(111) __E_parse_input_37(110).
	p(NT(111), (NT(111)+NT(110)));
//G149: parse_input_char_seq(13) => __E_parse_input_38(111).
	p(NT(13), (NT(111)));
//G150: parse_input(45)      => parse_input_char_seq(13).
	p(NT(45), (NT(13)));
//G151: help_arg(67)         => grammar_sym(49).
	p(NT(67), (NT(49)));
//G152: help_arg(67)         => igrammar_sym(51).
	p(NT(67), (NT(51)));
//G153: help_arg(67)         => unreachable_sym(57).
	p(NT(67), (NT(57)));
//G154: help_arg(67)         => start_sym(54).
	p(NT(67), (NT(54)));
//G155: help_arg(67)         => parse_sym(43).
	p(NT(67), (NT(43)));
//G156: help_arg(67)         => parse_file_sym(46).
	p(NT(67), (NT(46)));
//G157: help_arg(67)         => load_sym(62).
	p(NT(67), (NT(62)));
//G158: help_arg(67)         => reload_sym(60).
	p(NT(67), (NT(60)));
//G159: help_arg(67)         => clear_sym(74).
	p(NT(67), (NT(74)));
//G160: help_arg(67)         => help_sym(64).
	p(NT(67), (NT(64)));
//G161: help_arg(67)         => quit_sym(72).
	p(NT(67), (NT(72)));
//G162: help_arg(67)         => version_sym(68).
	p(NT(67), (NT(68)));
//G163: help_arg(67)         => license_sym(70).
	p(NT(67), (NT(70)));
//G164: help_arg(67)         => get_sym(76).
	p(NT(67), (NT(76)));
//G165: help_arg(67)         => set_sym(95).
	p(NT(67), (NT(95)));
//G166: help_arg(67)         => add_sym(79).
	p(NT(67), (NT(79)));
//G167: help_arg(67)         => del_sym(85).
	p(NT(67), (NT(85)));
//G168: help_arg(67)         => toggle_sym(88).
	p(NT(67), (NT(88)));
//G169: help_arg(67)         => enable_sym(91).
	p(NT(67), (NT(91)));
//G170: help_arg(67)         => disable_sym(93).
	p(NT(67), (NT(93)));
//G171: option(78)           => bool_option(90).
	p(NT(78), (NT(90)));
//G172: option(78)           => enum_ev_option(105).
	p(NT(78), (NT(105)));
//G173: option(78)           => list_option(81).
	p(NT(78), (NT(81)));
//G174: option(78)           => treepaths_option(83).
	p(NT(78), (NT(83)));
//G175: option(78)           => symbol_option(108).
	p(NT(78), (NT(108)));
//G176: __E_enum_ev_option_39(113) => 'e'.
	p(NT(113), (T(6)));
//G177: __E_enum_ev_option_39(113) => 'e' 'r' 'r' 'o' 'r' '-' 'v' 'e' 'r' 'b' 'o' 's' 'i' 't' 'y'.
	p(NT(113), (T(6)+T(4)+T(4)+T(20)+T(4)+T(15)+T(22)+T(6)+T(4)+T(19)+T(20)+T(5)+T(9)+T(14)+T(28)));
//G178: error_verbosity_opt(112) => __E_enum_ev_option_39(113).
	p(NT(112), (NT(113)));
//G179: enum_ev_option(105)  => error_verbosity_opt(112).
	p(NT(105), (NT(112)));
//G180: start_opt(114)       => 's' 't' 'a' 'r' 't'.
	p(NT(114), (T(5)+T(14)+T(3)+T(4)+T(14)));
//G181: symbol_option(108)   => start_opt(114).
	p(NT(108), (NT(114)));
//G182: __E_bool_option_40(116) => 's'.
	p(NT(116), (T(5)));
//G183: __E_bool_option_40(116) => 's' 't' 'a' 't' 'u' 's'.
	p(NT(116), (T(5)+T(14)+T(3)+T(14)+T(16)+T(5)));
//G184: status_opt(115)      => __E_bool_option_40(116).
	p(NT(115), (NT(116)));
//G185: bool_option(90)      => status_opt(115).
	p(NT(90), (NT(115)));
//G186: __E_bool_option_41(118) => 'c'.
	p(NT(118), (T(17)));
//G187: __E_bool_option_41(118) => 'c' 'o' 'l' 'o' 'r'.
	p(NT(118), (T(17)+T(20)+T(10)+T(20)+T(4)));
//G188: __E_bool_option_41(118) => 'c' 'o' 'l' 'o' 'r' 's'.
	p(NT(118), (T(17)+T(20)+T(10)+T(20)+T(4)+T(5)));
//G189: colors_opt(117)      => __E_bool_option_41(118).
	p(NT(117), (NT(118)));
//G190: bool_option(90)      => colors_opt(117).
	p(NT(90), (NT(117)));
//G191: __E_bool_option_42(120) => 'a'.
	p(NT(120), (T(3)));
//G192: __E_bool_option_42(120) => 'a' 'm' 'b' 'i' 'g' 'u' 'i' 't' 'y'.
	p(NT(120), (T(3)+T(12)+T(19)+T(9)+T(11)+T(16)+T(9)+T(14)+T(28)));
//G193: __E_bool_option_42(120) => 'p' 'r' 'i' 'n' 't' '-' 'a' 'm' 'b' 'i' 'g' 'u' 'i' 't' 'y'.
	p(NT(120), (T(2)+T(4)+T(9)+T(13)+T(14)+T(15)+T(3)+T(12)+T(19)+T(9)+T(11)+T(16)+T(9)+T(14)+T(28)));
//G194: print_ambiguity_opt(119) => __E_bool_option_42(120).
	p(NT(119), (NT(120)));
//G195: bool_option(90)      => print_ambiguity_opt(119).
	p(NT(90), (NT(119)));
//G196: __E_bool_option_43(122) => 'g'.
	p(NT(122), (T(11)));
//G197: __E_bool_option_43(122) => 'g' 'r' 'a' 'p' 'h' 's'.
	p(NT(122), (T(11)+T(4)+T(3)+T(2)+T(18)+T(5)));
//G198: __E_bool_option_43(122) => 'p' 'r' 'i' 'n' 't' '-' 'g' 'r' 'a' 'p' 'h' 's'.
	p(NT(122), (T(2)+T(4)+T(9)+T(13)+T(14)+T(15)+T(11)+T(4)+T(3)+T(2)+T(18)+T(5)));
//G199: print_graphs_opt(121) => __E_bool_option_43(122).
	p(NT(121), (NT(122)));
//G200: bool_option(90)      => print_graphs_opt(121).
	p(NT(90), (NT(121)));
//G201: __E_bool_option_44(124) => 'r'.
	p(NT(124), (T(4)));
//G202: __E_bool_option_44(124) => 'r' 'u' 'l' 'e' 's'.
	p(NT(124), (T(4)+T(16)+T(10)+T(6)+T(5)));
//G203: __E_bool_option_44(124) => 'p' 'r' 'i' 'n' 't' '-' 'r' 'u' 'l' 'e' 's'.
	p(NT(124), (T(2)+T(4)+T(9)+T(13)+T(14)+T(15)+T(4)+T(16)+T(10)+T(6)+T(5)));
//G204: print_rules_opt(123) => __E_bool_option_44(124).
	p(NT(123), (NT(124)));
//G205: bool_option(90)      => print_rules_opt(123).
	p(NT(90), (NT(123)));
//G206: __E_bool_option_45(126) => 'f'.
	p(NT(126), (T(7)));
//G207: __E_bool_option_45(126) => 'f' 'a' 'c' 't' 's'.
	p(NT(126), (T(7)+T(3)+T(17)+T(14)+T(5)));
//G208: __E_bool_option_45(126) => 'p' 'r' 'i' 'n' 't' '-' 'f' 'a' 'c' 't' 's'.
	p(NT(126), (T(2)+T(4)+T(9)+T(13)+T(14)+T(15)+T(7)+T(3)+T(17)+T(14)+T(5)));
//G209: print_facts_opt(125) => __E_bool_option_45(126).
	p(NT(125), (NT(126)));
//G210: bool_option(90)      => print_facts_opt(125).
	p(NT(90), (NT(125)));
//G211: __E_bool_option_46(128) => 't'.
	p(NT(128), (T(14)));
//G212: __E_bool_option_46(128) => 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
	p(NT(128), (T(14)+T(6)+T(4)+T(12)+T(9)+T(13)+T(3)+T(10)+T(5)));
//G213: __E_bool_option_46(128) => 'p' 'r' 'i' 'n' 't' '-' 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
	p(NT(128), (T(2)+T(4)+T(9)+T(13)+T(14)+T(15)+T(14)+T(6)+T(4)+T(12)+T(9)+T(13)+T(3)+T(10)+T(5)));
//G214: print_terminals_opt(127) => __E_bool_option_46(128).
	p(NT(127), (NT(128)));
//G215: bool_option(90)      => print_terminals_opt(127).
	p(NT(90), (NT(127)));
//G216: __E_bool_option_47(130) => 'm'.
	p(NT(130), (T(12)));
//G217: __E_bool_option_47(130) => 'm' 'e' 'a' 's' 'u' 'r' 'e'.
	p(NT(130), (T(12)+T(6)+T(3)+T(5)+T(16)+T(4)+T(6)));
//G218: __E_bool_option_47(130) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'p' 'a' 'r' 's' 'i' 'n' 'g'.
	p(NT(130), (T(12)+T(6)+T(3)+T(5)+T(16)+T(4)+T(6)+T(15)+T(2)+T(3)+T(4)+T(5)+T(9)+T(13)+T(11)));
//G219: measure_parsing_opt(129) => __E_bool_option_47(130).
	p(NT(129), (NT(130)));
//G220: bool_option(90)      => measure_parsing_opt(129).
	p(NT(90), (NT(129)));
//G221: __E_bool_option_48(132) => 'm' 'e'.
	p(NT(132), (T(12)+T(6)));
//G222: __E_bool_option_48(132) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'e' 'a' 'c' 'h'.
	p(NT(132), (T(12)+T(6)+T(3)+T(5)+T(16)+T(4)+T(6)+T(15)+T(6)+T(3)+T(17)+T(18)));
//G223: __E_bool_option_48(132) => 'm' 'e' 'p'.
	p(NT(132), (T(12)+T(6)+T(2)));
//G224: __E_bool_option_48(132) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'e' 'a' 'c' 'h' '-' 'p' 'o' 's'.
	p(NT(132), (T(12)+T(6)+T(3)+T(5)+T(16)+T(4)+T(6)+T(15)+T(6)+T(3)+T(17)+T(18)+T(15)+T(2)+T(20)+T(5)));
//G225: measure_each_pos_opt(131) => __E_bool_option_48(132).
	p(NT(131), (NT(132)));
//G226: bool_option(90)      => measure_each_pos_opt(131).
	p(NT(90), (NT(131)));
//G227: __E_bool_option_49(134) => 'm' 'f'.
	p(NT(134), (T(12)+T(7)));
//G228: __E_bool_option_49(134) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'f' 'o' 'r' 'e' 's' 't'.
	p(NT(134), (T(12)+T(6)+T(3)+T(5)+T(16)+T(4)+T(6)+T(15)+T(7)+T(20)+T(4)+T(6)+T(5)+T(14)));
//G229: measure_forest_opt(133) => __E_bool_option_49(134).
	p(NT(133), (NT(134)));
//G230: bool_option(90)      => measure_forest_opt(133).
	p(NT(90), (NT(133)));
//G231: __E_bool_option_50(136) => 'm' 'p'.
	p(NT(136), (T(12)+T(2)));
//G232: __E_bool_option_50(136) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'p' 'r' 'e' 'p' 'r' 'o' 'c' 'e' 's' 's'.
	p(NT(136), (T(12)+T(6)+T(3)+T(5)+T(16)+T(4)+T(6)+T(15)+T(2)+T(4)+T(6)+T(2)+T(4)+T(20)+T(17)+T(6)+T(5)+T(5)));
//G233: measure_preprocess_opt(135) => __E_bool_option_50(136).
	p(NT(135), (NT(136)));
//G234: bool_option(90)      => measure_preprocess_opt(135).
	p(NT(90), (NT(135)));
//G235: __E_bool_option_51(138) => 'g' 'c'.
	p(NT(138), (T(11)+T(17)));
//G236: gc_opt(137)          => __E_bool_option_51(138).
	p(NT(137), (NT(138)));
//G237: bool_option(90)      => gc_opt(137).
	p(NT(90), (NT(137)));
//G238: __E_bool_option_52(140) => 'd'.
	p(NT(140), (T(21)));
//G239: __E_bool_option_52(140) => 'd' 'e' 'b' 'u' 'g'.
	p(NT(140), (T(21)+T(6)+T(19)+T(16)+T(11)));
//G240: debug_opt(139)       => __E_bool_option_52(140).
	p(NT(139), (NT(140)));
//G241: bool_option(90)      => debug_opt(139).
	p(NT(90), (NT(139)));
//G242: __E_bool_option_53(142) => 'a' 'd'.
	p(NT(142), (T(3)+T(21)));
//G243: __E_bool_option_53(142) => 'a' 'u' 't' 'o' '-' 'd' 'i' 's' 'a' 'm' 'b' 'i' 'g' 'u' 'a' 't' 'e'.
	p(NT(142), (T(3)+T(16)+T(14)+T(20)+T(15)+T(21)+T(9)+T(5)+T(3)+T(12)+T(19)+T(9)+T(11)+T(16)+T(3)+T(14)+T(6)));
//G244: auto_disambiguate_opt(141) => __E_bool_option_53(142).
	p(NT(141), (NT(142)));
//G245: bool_option(90)      => auto_disambiguate_opt(141).
	p(NT(90), (NT(141)));
//G246: __E_bool_option_54(144) => 't' 't'.
	p(NT(144), (T(14)+T(14)));
//G247: __E_bool_option_54(144) => 't' 'r' 'i' 'm' '-' 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
	p(NT(144), (T(14)+T(4)+T(9)+T(12)+T(15)+T(14)+T(6)+T(4)+T(12)+T(9)+T(13)+T(3)+T(10)+T(5)));
//G248: trim_terminals_opt(143) => __E_bool_option_54(144).
	p(NT(143), (NT(144)));
//G249: bool_option(90)      => trim_terminals_opt(143).
	p(NT(90), (NT(143)));
//G250: __E_bool_option_55(146) => 'i' 'c' 'c'.
	p(NT(146), (T(9)+T(17)+T(17)));
//G251: __E_bool_option_55(146) => 'i' 'n' 'l' 'i' 'n' 'e' '-' 'c' 'c'.
	p(NT(146), (T(9)+T(13)+T(10)+T(9)+T(13)+T(6)+T(15)+T(17)+T(17)));
//G252: __E_bool_option_55(146) => 'i' 'n' 'l' 'i' 'n' 'e' '-' 'c' 'h' 'a' 'r' '-' 'c' 'l' 'a' 's' 's' 'e' 's'.
	p(NT(146), (T(9)+T(13)+T(10)+T(9)+T(13)+T(6)+T(15)+T(17)+T(18)+T(3)+T(4)+T(15)+T(17)+T(10)+T(3)+T(5)+T(5)+T(6)+T(5)));
//G253: inline_cc_opt(145)   => __E_bool_option_55(146).
	p(NT(145), (NT(146)));
//G254: bool_option(90)      => inline_cc_opt(145).
	p(NT(90), (NT(145)));
//G255: __E_bool_option_56(148) => 'd' 'c' 'c'.
	p(NT(148), (T(21)+T(17)+T(17)));
//G256: __E_bool_option_56(148) => 'd' 'e' 'r' 'i' 'v' 'e' '-' 'c' 'h' 'a' 'r' '-' 'c' 'l' 'a' 's' 's' 'e' 's'.
	p(NT(148), (T(21)+T(6)+T(4)+T(9)+T(22)+T(6)+T(15)+T(17)+T(18)+T(3)+T(4)+T(15)+T(17)+T(10)+T(3)+T(5)+T(5)+T(6)+T(5)));
//G257: derive_char_classes_opt(147) => __E_bool_option_56(148).
	p(NT(147), (NT(148)));
//G258: bool_option(90)      => derive_char_classes_opt(147).
	p(NT(90), (NT(147)));
//G259: __E_list_option_57(150) => 'n' 'd'.
	p(NT(150), (T(13)+T(21)));
//G260: __E_list_option_57(150) => 'n' 'd' 'l'.
	p(NT(150), (T(13)+T(21)+T(10)));
//G261: __E_list_option_57(150) => 'n' 'o' 'd' 'i' 's' 'a' 'm' 'b' 'i' 'g' '-' 'l' 'i' 's' 't'.
	p(NT(150), (T(13)+T(20)+T(21)+T(9)+T(5)+T(3)+T(12)+T(19)+T(9)+T(11)+T(15)+T(10)+T(9)+T(5)+T(14)));
//G262: nodisambig_list_opt(149) => __E_list_option_57(150).
	p(NT(149), (NT(150)));
//G263: list_option(81)      => nodisambig_list_opt(149).
	p(NT(81), (NT(149)));
//G264: __E_list_option_58(152) => 'g' 'u' 'a' 'r' 'd' 's'.
	p(NT(152), (T(11)+T(16)+T(3)+T(4)+T(21)+T(5)));
//G265: __E_list_option_58(152) => 'e' 'n' 'a' 'b' 'l' 'e' 'd' '-' 'p' 'r' 'o' 'd' 'u' 'c' 't' 'i' 'o' 'n' 's'.
	p(NT(152), (T(6)+T(13)+T(3)+T(19)+T(10)+T(6)+T(21)+T(15)+T(2)+T(4)+T(20)+T(21)+T(16)+T(17)+T(14)+T(9)+T(20)+T(13)+T(5)));
//G266: enabled_prods_opt(151) => __E_list_option_58(152).
	p(NT(151), (NT(152)));
//G267: list_option(81)      => enabled_prods_opt(151).
	p(NT(81), (NT(151)));
//G268: trim_opt(153)        => 't' 'r' 'i' 'm'.
	p(NT(153), (T(14)+T(4)+T(9)+T(12)));
//G269: list_option(81)      => trim_opt(153).
	p(NT(81), (NT(153)));
//G270: __E_list_option_59(155) => 't' 'c'.
	p(NT(155), (T(14)+T(17)));
//G271: __E_list_option_59(155) => 't' 'r' 'i' 'm' '-' 'c' 'h' 'i' 'l' 'd' 'r' 'e' 'n'.
	p(NT(155), (T(14)+T(4)+T(9)+T(12)+T(15)+T(17)+T(18)+T(9)+T(10)+T(21)+T(4)+T(6)+T(13)));
//G272: trim_children_opt(154) => __E_list_option_59(155).
	p(NT(154), (NT(155)));
//G273: list_option(81)      => trim_children_opt(154).
	p(NT(81), (NT(154)));
//G274: __E_list_option_60(157) => 't' 'c' 't'.
	p(NT(157), (T(14)+T(17)+T(14)));
//G275: __E_list_option_60(157) => 't' 'r' 'i' 'm' '-' 'c' 'h' 'i' 'l' 'd' 'r' 'e' 'n' '-' 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
	p(NT(157), (T(14)+T(4)+T(9)+T(12)+T(15)+T(17)+T(18)+T(9)+T(10)+T(21)+T(4)+T(6)+T(13)+T(15)+T(14)+T(6)+T(4)+T(12)+T(9)+T(13)+T(3)+T(10)+T(5)));
//G276: trim_children_terminals_opt(156) => __E_list_option_60(157).
	p(NT(156), (NT(157)));
//G277: list_option(81)      => trim_children_terminals_opt(156).
	p(NT(81), (NT(156)));
//G278: __E_treepaths_option_61(159) => 'i'.
	p(NT(159), (T(9)));
//G279: __E_treepaths_option_61(159) => 'i' 'n' 'l' 'i' 'n' 'e'.
	p(NT(159), (T(9)+T(13)+T(10)+T(9)+T(13)+T(6)));
//G280: inline_opt(158)      => __E_treepaths_option_61(159).
	p(NT(158), (NT(159)));
//G281: treepaths_option(83) => inline_opt(158).
	p(NT(83), (NT(158)));
//G282: __E_bool_value_62(161) => 't'.
	p(NT(161), (T(14)));
//G283: __E_bool_value_62(161) => 't' 'r' 'u' 'e'.
	p(NT(161), (T(14)+T(4)+T(16)+T(6)));
//G284: __E_bool_value_62(161) => 'o' 'n'.
	p(NT(161), (T(20)+T(13)));
//G285: __E_bool_value_62(161) => '1'.
	p(NT(161), (T(29)));
//G286: __E_bool_value_62(161) => 'y'.
	p(NT(161), (T(28)));
//G287: __E_bool_value_62(161) => 'y' 'e' 's'.
	p(NT(161), (T(28)+T(6)+T(5)));
//G288: true_value(160)      => __E_bool_value_62(161).
	p(NT(160), (NT(161)));
//G289: bool_value(98)       => true_value(160).
	p(NT(98), (NT(160)));
//G290: __E_bool_value_63(163) => 'f'.
	p(NT(163), (T(7)));
//G291: __E_bool_value_63(163) => 'f' 'a' 'l' 's' 'e'.
	p(NT(163), (T(7)+T(3)+T(10)+T(5)+T(6)));
//G292: __E_bool_value_63(163) => 'o' 'f' 'f'.
	p(NT(163), (T(20)+T(7)+T(7)));
//G293: __E_bool_value_63(163) => '0'.
	p(NT(163), (T(30)));
//G294: __E_bool_value_63(163) => 'n'.
	p(NT(163), (T(13)));
//G295: __E_bool_value_63(163) => 'n' 'o'.
	p(NT(163), (T(13)+T(20)));
//G296: false_value(162)     => __E_bool_value_63(163).
	p(NT(162), (NT(163)));
//G297: bool_value(98)       => false_value(162).
	p(NT(98), (NT(162)));
//G298: __E_error_verbosity_64(165) => 'b'.
	p(NT(165), (T(19)));
//G299: __E_error_verbosity_64(165) => 'b' 'a' 's' 'i' 'c'.
	p(NT(165), (T(19)+T(3)+T(5)+T(9)+T(17)));
//G300: basic_sym(164)       => __E_error_verbosity_64(165).
	p(NT(164), (NT(165)));
//G301: error_verbosity(107) => basic_sym(164).
	p(NT(107), (NT(164)));
//G302: __E_error_verbosity_65(167) => 'd'.
	p(NT(167), (T(21)));
//G303: __E_error_verbosity_65(167) => 'd' 'e' 't' 'a' 'i' 'l' 'e' 'd'.
	p(NT(167), (T(21)+T(6)+T(14)+T(3)+T(9)+T(10)+T(6)+T(21)));
//G304: detailed_sym(166)    => __E_error_verbosity_65(167).
	p(NT(166), (NT(167)));
//G305: error_verbosity(107) => detailed_sym(166).
	p(NT(107), (NT(166)));
//G306: __E_error_verbosity_66(169) => 'r'.
	p(NT(169), (T(4)));
//G307: __E_error_verbosity_66(169) => 'r' 'c'.
	p(NT(169), (T(4)+T(17)));
//G308: __E_error_verbosity_66(169) => 'r' 'o' 'o' 't' '-' 'c' 'a' 'u' 's' 'e'.
	p(NT(169), (T(4)+T(20)+T(20)+T(14)+T(15)+T(17)+T(3)+T(16)+T(5)+T(6)));
//G309: root_cause_sym(168)  => __E_error_verbosity_66(169).
	p(NT(168), (NT(169)));
//G310: error_verbosity(107) => root_cause_sym(168).
	p(NT(107), (NT(168)));
//G311: __E_symbol_67(170)   => alpha(3).
	p(NT(170), (NT(3)));
//G312: __E_symbol_67(170)   => '_'.
	p(NT(170), (T(31)));
//G313: __E_symbol_68(171)   => alnum(2).
	p(NT(171), (NT(2)));
//G314: __E_symbol_68(171)   => '_'.
	p(NT(171), (T(31)));
//G315: __E_symbol_69(172)   => null.
	p(NT(172), (nul));
//G316: __E_symbol_69(172)   => __E_symbol_69(172) __E_symbol_68(171).
	p(NT(172), (NT(172)+NT(171)));
//G317: symbol(9)            => __E_symbol_67(170) __E_symbol_69(172).
	p(NT(9), (NT(170)+NT(172)));
//G318: __E_symbol_list_70(173) => _(7) ',' _(7) symbol(9).
	p(NT(173), (NT(7)+T(32)+NT(7)+NT(9)));
//G319: __E_symbol_list_71(174) => null.
	p(NT(174), (nul));
//G320: __E_symbol_list_71(174) => __E_symbol_list_71(174) __E_symbol_list_70(173).
	p(NT(174), (NT(174)+NT(173)));
//G321: symbol_list(82)      => symbol(9) __E_symbol_list_71(174).
	p(NT(82), (NT(9)+NT(174)));
//G322: __E_treepath_72(176) => _(7) '>' _(7) symbol(9).
	p(NT(176), (NT(7)+T(33)+NT(7)+NT(9)));
//G323: __E_treepath_73(177) => null.
	p(NT(177), (nul));
//G324: __E_treepath_73(177) => __E_treepath_73(177) __E_treepath_72(176).
	p(NT(177), (NT(177)+NT(176)));
//G325: treepath(175)        => symbol(9) __E_treepath_73(177).
	p(NT(175), (NT(9)+NT(177)));
//G326: __E_treepath_list_74(178) => _(7) ',' _(7) treepath(175).
	p(NT(178), (NT(7)+T(32)+NT(7)+NT(175)));
//G327: __E_treepath_list_75(179) => null.
	p(NT(179), (nul));
//G328: __E_treepath_list_75(179) => __E_treepath_list_75(179) __E_treepath_list_74(178).
	p(NT(179), (NT(179)+NT(178)));
//G329: treepath_list(84)    => treepath(175) __E_treepath_list_75(179).
	p(NT(84), (NT(175)+NT(179)));
//G330: filename(48)         => quoted_string(10).
	p(NT(48), (NT(10)));
//G331: __E_quoted_string_76(181) => null.
	p(NT(181), (nul));
//G332: __E_quoted_string_76(181) => __E_quoted_string_76(181) quoted_string_char(180).
	p(NT(181), (NT(181)+NT(180)));
//G333: quoted_string(10)    => '"' __E_quoted_string_76(181) '"'.
	p(NT(10), (T(34)+NT(181)+T(34)));
//G334: quoted_string_char(180) => unescaped_s(11).
	p(NT(180), (NT(11)));
//G335: quoted_string_char(180) => escaped_s(12).
	p(NT(180), (NT(12)));
//G336: __E_unescaped_s_77(182) => space(4).
	p(NT(182), (NT(4)));
//G337: __E_unescaped_s_77(182) => printable(5).
	p(NT(182), (NT(5)));
//G338: __E_unescaped_s_78(183) => '"'.
	p(NT(183), (T(34)));
//G339: __E_unescaped_s_78(183) => '\\'.
	p(NT(183), (T(35)));
//G340: __N_0(193)           => __E_unescaped_s_78(183).
	p(NT(193), (NT(183)));
//G341: unescaped_s(11)      => __E_unescaped_s_77(182) & ~( __N_0(193) ).	 # conjunctive
	p(NT(11), (NT(182)) & ~(NT(193)));
//G342: __E_escaped_s_79(184) => '"'.
	p(NT(184), (T(34)));
//G343: __E_escaped_s_79(184) => escape_char(14).
	p(NT(184), (NT(14)));
//G344: escaped_s(12)        => '\\' __E_escaped_s_79(184).
	p(NT(12), (T(35)+NT(184)));
//G345: escape_char(14)      => 'a'.
	p(NT(14), (T(3)));
//G346: escape_char(14)      => 'b'.
	p(NT(14), (T(19)));
//G347: escape_char(14)      => 'f'.
	p(NT(14), (T(7)));
//G348: escape_char(14)      => 'n'.
	p(NT(14), (T(13)));
//G349: escape_char(14)      => 'r'.
	p(NT(14), (T(4)));
//G350: escape_char(14)      => 't'.
	p(NT(14), (T(14)));
//G351: escape_char(14)      => 'v'.
	p(NT(14), (T(22)));
//G352: escape_char(14)      => '\\'.
	p(NT(14), (T(35)));
//G353: escape_char(14)      => '/'.
	p(NT(14), (T(36)));
//G354: escape_char(14)      => esc_hex(15).
	p(NT(14), (NT(15)));
//G355: escape_char(14)      => esc_u4(16).
	p(NT(14), (NT(16)));
//G356: escape_char(14)      => esc_U8(17).
	p(NT(14), (NT(17)));
//G357: __E_esc_hex_80(185)  => 'x'.
	p(NT(185), (T(25)));
//G358: __E_esc_hex_80(185)  => 'X'.
	p(NT(185), (T(37)));
//G359: __E_esc_hex_81(186)  => xdigit(6).
	p(NT(186), (NT(6)));
//G360: __E_esc_hex_81(186)  => null.
	p(NT(186), (nul));
//G361: esc_hex(15)          => __E_esc_hex_80(185) xdigit(6) __E_esc_hex_81(186).
	p(NT(15), (NT(185)+NT(6)+NT(186)));
//G362: esc_u4(16)           => 'u' xdigit(6) xdigit(6) xdigit(6) xdigit(6).
	p(NT(16), (T(16)+NT(6)+NT(6)+NT(6)+NT(6)));
//G363: esc_U8(17)           => 'U' xdigit(6) xdigit(6) xdigit(6) xdigit(6) xdigit(6) xdigit(6) xdigit(6) xdigit(6).
	p(NT(17), (T(38)+NT(6)+NT(6)+NT(6)+NT(6)+NT(6)+NT(6)+NT(6)+NT(6)));
//G364: __E___82(187)        => __(8).
	p(NT(187), (NT(8)));
//G365: __E___82(187)        => null.
	p(NT(187), (nul));
//G366: _(7)                 => __E___82(187).
	p(NT(7), (NT(187)));
//G367: __E____83(188)       => space(4).
	p(NT(188), (NT(4)));
//G368: __E____83(188)       => comment(189).
	p(NT(188), (NT(189)));
//G369: __(8)                => __E____83(188) _(7).
	p(NT(8), (NT(188)+NT(7)));
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
#endif

} // namespace tgf_repl_parser_data
