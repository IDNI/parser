// This file is generated from a file src/format/tgf/tgf.tgf by
//       https://github.com/IDNI/parser/src/tgf
//
// productions() lives here so the table is compiled once, not per TU.
//
#include "parser.h"
#include "tgf_parser.generated.h"

namespace tgf_parser_data {

#ifndef TAU_PARSER_BUILD_HEADER_ONLY
idni::prods<char_type, terminal_type>& productions() {
	static bool loaded = false;
	static idni::prods<char_type, terminal_type>
		p, nul(idni::lit<char_type, terminal_type>{});
	if (loaded) return p;
	#define  T(x) (idni::prods<char_type, terminal_type>{ terminals[x] })
	#define NT(x) (idni::prods<char_type, terminal_type>{ nts(x) })
//G0:   __E_start_0(24)      => _(8) statement(25).
	p(NT(24), (NT(8)+NT(25)));
//G1:   __E_start_1(26)      => null.
	p(NT(26), (nul));
//G2:   __E_start_1(26)      => __E_start_1(26) __E_start_0(24).
	p(NT(26), (NT(26)+NT(24)));
//G3:   start(23)            => __E_start_1(26) _(8).
	p(NT(23), (NT(26)+NT(8)));
//G4:   statement(25)        => directive(27).
	p(NT(25), (NT(27)));
//G5:   statement(25)        => production(28).
	p(NT(25), (NT(28)));
//G6:   start_statement(29)  => _(8) statement(25) _(8).
	p(NT(29), (NT(8)+NT(25)+NT(8)));
//G7:   production_guard(31) => sym(10).
	p(NT(31), (NT(10)));
//G8:   __E_production_2(30) => _(8) '[' _(8) production_guard(31) _(8) ']'.
	p(NT(30), (NT(8)+T(1)+NT(8)+NT(31)+NT(8)+T(2)));
//G9:   __E_production_2(30) => null.
	p(NT(30), (nul));
//G10:  production(28)       => sym(10) __E_production_2(30) _(8) '=' '>' _(8) alternation(32) _(8) '.'.
	p(NT(28), (NT(10)+NT(30)+NT(8)+T(3)+T(4)+NT(8)+NT(32)+NT(8)+T(5)));
//G11:  __E_alternation_3(34) => _(8) '|' _(8) conjunction(33).
	p(NT(34), (NT(8)+T(6)+NT(8)+NT(33)));
//G12:  __E_alternation_4(35) => null.
	p(NT(35), (nul));
//G13:  __E_alternation_4(35) => __E_alternation_4(35) __E_alternation_3(34).
	p(NT(35), (NT(35)+NT(34)));
//G14:  alternation(32)      => conjunction(33) __E_alternation_4(35).
	p(NT(32), (NT(33)+NT(35)));
//G15:  __E_conjunction_5(37) => _(8) '&' _(8) concatenation(36).
	p(NT(37), (NT(8)+T(7)+NT(8)+NT(36)));
//G16:  __E_conjunction_6(38) => null.
	p(NT(38), (nul));
//G17:  __E_conjunction_6(38) => __E_conjunction_6(38) __E_conjunction_5(37).
	p(NT(38), (NT(38)+NT(37)));
//G18:  conjunction(33)      => concatenation(36) __E_conjunction_6(38).
	p(NT(33), (NT(36)+NT(38)));
//G19:  __E_concatenation_7(40) => __(7) factor(39).
	p(NT(40), (NT(7)+NT(39)));
//G20:  __E_concatenation_8(41) => null.
	p(NT(41), (nul));
//G21:  __E_concatenation_8(41) => __E_concatenation_8(41) __E_concatenation_7(40).
	p(NT(41), (NT(41)+NT(40)));
//G22:  concatenation(36)    => factor(39) __E_concatenation_8(41).
	p(NT(36), (NT(39)+NT(41)));
//G23:  __E_factor_9(43)     => factor(39) _(8) ':' sym(10).
	p(NT(43), (NT(39)+NT(8)+T(8)+NT(10)));
//G24:  shorthand_rule(42)   => __E_factor_9(43).
	p(NT(42), (NT(43)));
//G25:  factor(39)           => shorthand_rule(42).
	p(NT(39), (NT(42)));
//G26:  __E_factor_10(45)    => term(46) '?'.
	p(NT(45), (NT(46)+T(9)));
//G27:  optional(44)         => __E_factor_10(45).
	p(NT(44), (NT(45)));
//G28:  factor(39)           => optional(44).
	p(NT(39), (NT(44)));
//G29:  __E_factor_11(48)    => term(46) '+'.
	p(NT(48), (NT(46)+T(10)));
//G30:  repeat(47)           => __E_factor_11(48).
	p(NT(47), (NT(48)));
//G31:  factor(39)           => repeat(47).
	p(NT(39), (NT(47)));
//G32:  __E_factor_12(50)    => term(46) '*'.
	p(NT(50), (NT(46)+T(11)));
//G33:  none_or_repeat(49)   => __E_factor_12(50).
	p(NT(49), (NT(50)));
//G34:  factor(39)           => none_or_repeat(49).
	p(NT(39), (NT(49)));
//G35:  __E_factor_13(52)    => '~' term(46).
	p(NT(52), (T(12)+NT(46)));
//G36:  neg(51)              => __E_factor_13(52).
	p(NT(51), (NT(52)));
//G37:  factor(39)           => neg(51).
	p(NT(39), (NT(51)));
//G38:  factor(39)           => term(46).
	p(NT(39), (NT(46)));
//G39:  __E_term_14(54)      => '(' _(8) alternation(32) _(8) ')'.
	p(NT(54), (T(13)+NT(8)+NT(32)+NT(8)+T(14)));
//G40:  group(53)            => __E_term_14(54).
	p(NT(53), (NT(54)));
//G41:  term(46)             => group(53).
	p(NT(46), (NT(53)));
//G42:  __E_term_15(56)      => '[' _(8) alternation(32) _(8) ']'.
	p(NT(56), (T(1)+NT(8)+NT(32)+NT(8)+T(2)));
//G43:  optional_group(55)   => __E_term_15(56).
	p(NT(55), (NT(56)));
//G44:  term(46)             => optional_group(55).
	p(NT(46), (NT(55)));
//G45:  __E_term_16(58)      => '{' _(8) alternation(32) _(8) '}'.
	p(NT(58), (T(15)+NT(8)+NT(32)+NT(8)+T(16)));
//G46:  repeat_group(57)     => __E_term_16(58).
	p(NT(57), (NT(58)));
//G47:  term(46)             => repeat_group(57).
	p(NT(46), (NT(57)));
//G48:  term(46)             => terminal(59).
	p(NT(46), (NT(59)));
//G49:  term(46)             => sym(10).
	p(NT(46), (NT(10)));
//G50:  terminal(59)         => terminal_char(60).
	p(NT(59), (NT(60)));
//G51:  terminal(59)         => terminal_string(61).
	p(NT(59), (NT(61)));
//G52:  terminal(59)         => terminal_hex(17).
	p(NT(59), (NT(17)));
//G53:  __E_terminal_hex_17(62) => xdigit(6).
	p(NT(62), (NT(6)));
//G54:  __E___E_terminal_hex_17_18(63) => xdigit(6) xdigit(6).
	p(NT(63), (NT(6)+NT(6)));
//G55:  __E___E_terminal_hex_17_19(64) => __E___E_terminal_hex_17_18(63).
	p(NT(64), (NT(63)));
//G56:  __E___E_terminal_hex_17_19(64) => __E___E_terminal_hex_17_19(64) __E___E_terminal_hex_17_18(63).
	p(NT(64), (NT(64)+NT(63)));
//G57:  __E_terminal_hex_17(62) => __E___E_terminal_hex_17_19(64).
	p(NT(62), (NT(64)));
//G58:  hex_bytes(18)        => __E_terminal_hex_17(62).
	p(NT(18), (NT(62)));
//G59:  terminal_hex(17)     => '0' 'x' hex_bytes(18).
	p(NT(17), (T(17)+T(18)+NT(18)));
//G60:  __E_sym_20(65)       => alpha(3).
	p(NT(65), (NT(3)));
//G61:  __E_sym_20(65)       => '_'.
	p(NT(65), (T(19)));
//G62:  __E_sym_21(66)       => alnum(2).
	p(NT(66), (NT(2)));
//G63:  __E_sym_21(66)       => '_'.
	p(NT(66), (T(19)));
//G64:  __E_sym_22(67)       => null.
	p(NT(67), (nul));
//G65:  __E_sym_22(67)       => __E_sym_22(67) __E_sym_21(66).
	p(NT(67), (NT(67)+NT(66)));
//G66:  sym(10)              => __E_sym_20(65) __E_sym_22(67).
	p(NT(10), (NT(65)+NT(67)));
//G67:  __E_terminal_char_23(68) => unescaped_c(16).
	p(NT(68), (NT(16)));
//G68:  __E_terminal_char_23(68) => escaped_c(15).
	p(NT(68), (NT(15)));
//G69:  terminal_char(60)    => '\'' __E_terminal_char_23(68) '\''.
	p(NT(60), (T(20)+NT(68)+T(20)));
//G70:  __E_unescaped_c_24(69) => '\''.
	p(NT(69), (T(20)));
//G71:  __E_unescaped_c_24(69) => '\\'.
	p(NT(69), (T(21)));
//G72:  __N_0(117)           => __E_unescaped_c_24(69).
	p(NT(117), (NT(69)));
//G73:  unescaped_c(16)      => printable(5) & ~( __N_0(117) ).	 # conjunctive
	p(NT(16), (NT(5)) & ~(NT(117)));
//G74:  __E_escaped_c_25(70) => '\''.
	p(NT(70), (T(20)));
//G75:  __E_escaped_c_25(70) => escape_char(19).
	p(NT(70), (NT(19)));
//G76:  escaped_c(15)        => '\\' __E_escaped_c_25(70).
	p(NT(15), (T(21)+NT(70)));
//G77:  __E_terminal_string_26(71) => unescaped_s(14).
	p(NT(71), (NT(14)));
//G78:  __E_terminal_string_26(71) => escaped_s(13).
	p(NT(71), (NT(13)));
//G79:  __E_terminal_string_27(72) => null.
	p(NT(72), (nul));
//G80:  __E_terminal_string_27(72) => __E_terminal_string_27(72) __E_terminal_string_26(71).
	p(NT(72), (NT(72)+NT(71)));
//G81:  terminal_string(61)  => '"' __E_terminal_string_27(72) '"'.
	p(NT(61), (T(22)+NT(72)+T(22)));
//G82:  __E_unescaped_s_28(73) => '"'.
	p(NT(73), (T(22)));
//G83:  __E_unescaped_s_28(73) => '\\'.
	p(NT(73), (T(21)));
//G84:  __N_1(118)           => __E_unescaped_s_28(73).
	p(NT(118), (NT(73)));
//G85:  unescaped_s(14)      => printable(5) & ~( __N_1(118) ).	 # conjunctive
	p(NT(14), (NT(5)) & ~(NT(118)));
//G86:  __E_escaped_s_29(74) => '"'.
	p(NT(74), (T(22)));
//G87:  __E_escaped_s_29(74) => escape_char(19).
	p(NT(74), (NT(19)));
//G88:  escaped_s(13)        => '\\' __E_escaped_s_29(74).
	p(NT(13), (T(21)+NT(74)));
//G89:  escape_char(19)      => 'a'.
	p(NT(19), (T(23)));
//G90:  escape_char(19)      => 'b'.
	p(NT(19), (T(24)));
//G91:  escape_char(19)      => 'f'.
	p(NT(19), (T(25)));
//G92:  escape_char(19)      => 'n'.
	p(NT(19), (T(26)));
//G93:  escape_char(19)      => 'r'.
	p(NT(19), (T(27)));
//G94:  escape_char(19)      => 't'.
	p(NT(19), (T(28)));
//G95:  escape_char(19)      => 'v'.
	p(NT(19), (T(29)));
//G96:  escape_char(19)      => '\\'.
	p(NT(19), (T(21)));
//G97:  escape_char(19)      => '/'.
	p(NT(19), (T(30)));
//G98:  escape_char(19)      => esc_hex(20).
	p(NT(19), (NT(20)));
//G99:  escape_char(19)      => esc_u4(21).
	p(NT(19), (NT(21)));
//G100: escape_char(19)      => esc_U8(22).
	p(NT(19), (NT(22)));
//G101: __E_esc_hex_30(75)   => 'x'.
	p(NT(75), (T(18)));
//G102: __E_esc_hex_30(75)   => 'X'.
	p(NT(75), (T(31)));
//G103: __E_esc_hex_31(76)   => xdigit(6).
	p(NT(76), (NT(6)));
//G104: __E_esc_hex_31(76)   => null.
	p(NT(76), (nul));
//G105: esc_hex(20)          => __E_esc_hex_30(75) xdigit(6) __E_esc_hex_31(76).
	p(NT(20), (NT(75)+NT(6)+NT(76)));
//G106: esc_u4(21)           => 'u' xdigit(6) xdigit(6) xdigit(6) xdigit(6).
	p(NT(21), (T(32)+NT(6)+NT(6)+NT(6)+NT(6)));
//G107: esc_U8(22)           => 'U' xdigit(6) xdigit(6) xdigit(6) xdigit(6) xdigit(6) xdigit(6) xdigit(6) xdigit(6).
	p(NT(22), (T(33)+NT(6)+NT(6)+NT(6)+NT(6)+NT(6)+NT(6)+NT(6)+NT(6)));
//G108: directive_name(79)   => sym(10).
	p(NT(79), (NT(10)));
//G109: __E_directive_32(78) => '@' _(8) directive_name(79) _(8).
	p(NT(78), (T(34)+NT(8)+NT(79)+NT(8)));
//G110: directive_token(77)  => __E_directive_32(78).
	p(NT(77), (NT(78)));
//G111: __E___E___E_directive_33_34_35(83) => sep(9) sym(10).
	p(NT(83), (NT(9)+NT(10)));
//G112: __E___E___E_directive_33_34_36(84) => null.
	p(NT(84), (nul));
//G113: __E___E___E_directive_33_34_36(84) => __E___E___E_directive_33_34_36(84) __E___E___E_directive_33_34_35(83).
	p(NT(84), (NT(84)+NT(83)));
//G114: __E___E_directive_33_34(82) => sep(9) sym(10) __E___E___E_directive_33_34_36(84).
	p(NT(82), (NT(9)+NT(10)+NT(84)));
//G115: directive_cmd(81)    => __E___E_directive_33_34(82).
	p(NT(81), (NT(82)));
//G116: __E_directive_33(80) => directive_cmd(81).
	p(NT(80), (NT(81)));
//G117: __E_directive_37(85) => null.
	p(NT(85), (nul));
//G118: __E_directive_37(85) => __E_directive_37(85) __E_directive_33(80).
	p(NT(85), (NT(85)+NT(80)));
//G119: __E___E_directive_38_39(88) => _(8) ';' _(8) dir_pair(87).
	p(NT(88), (NT(8)+T(35)+NT(8)+NT(87)));
//G120: __E___E_directive_38_40(89) => null.
	p(NT(89), (nul));
//G121: __E___E_directive_38_40(89) => __E___E_directive_38_40(89) __E___E_directive_38_39(88).
	p(NT(89), (NT(89)+NT(88)));
//G122: __E_directive_38(86) => __(7) dir_pair(87) __E___E_directive_38_40(89).
	p(NT(86), (NT(7)+NT(87)+NT(89)));
//G123: __E_directive_38(86) => null.
	p(NT(86), (nul));
//G124: directive(27)        => directive_token(77) __E_directive_37(85) __E_directive_38(86) _(8) '.'.
	p(NT(27), (NT(77)+NT(85)+NT(86)+NT(8)+T(5)));
//G125: sep(9)               => '-'.
	p(NT(9), (T(36)));
//G126: sep(9)               => '_'.
	p(NT(9), (T(19)));
//G127: sep(9)               => __(7).
	p(NT(9), (NT(7)));
//G128: __E_dir_pair_41(91)  => _(8) ':' _(8) dir_list(90).
	p(NT(91), (NT(8)+T(8)+NT(8)+NT(90)));
//G129: __E_dir_pair_41(91)  => null.
	p(NT(91), (nul));
//G130: dir_pair(87)         => dir_list(90) __E_dir_pair_41(91).
	p(NT(87), (NT(90)+NT(91)));
//G131: __E_dir_list_42(93)  => _(8) ',' _(8) dir_arg(92).
	p(NT(93), (NT(8)+T(37)+NT(8)+NT(92)));
//G132: __E_dir_list_43(94)  => null.
	p(NT(94), (nul));
//G133: __E_dir_list_43(94)  => __E_dir_list_43(94) __E_dir_list_42(93).
	p(NT(94), (NT(94)+NT(93)));
//G134: dir_list(90)         => dir_arg(92) __E_dir_list_43(94).
	p(NT(90), (NT(92)+NT(94)));
//G135: dir_arg(92)          => tree_path(95).
	p(NT(92), (NT(95)));
//G136: dir_arg(92)          => dir_sym(11).
	p(NT(92), (NT(11)));
//G137: dir_arg(92)          => terminal_string(61).
	p(NT(92), (NT(61)));
//G138: __E___E___E_dir_arg_44_45_46(100) => pat_space(99).
	p(NT(100), (NT(99)));
//G139: __E___E___E_dir_arg_44_45_46(100) => __E___E___E_dir_arg_44_45_46(100) pat_space(99).
	p(NT(100), (NT(100)+NT(99)));
//G140: __E___E_dir_arg_44_45(98) => __E___E___E_dir_arg_44_45_46(100) dir_sym(11).
	p(NT(98), (NT(100)+NT(11)));
//G141: __E___E_dir_arg_44_47(101) => __E___E_dir_arg_44_45(98).
	p(NT(101), (NT(98)));
//G142: __E___E_dir_arg_44_47(101) => __E___E_dir_arg_44_47(101) __E___E_dir_arg_44_45(98).
	p(NT(101), (NT(101)+NT(98)));
//G143: __E_dir_arg_44(97)   => dir_sym(11) __E___E_dir_arg_44_47(101).
	p(NT(97), (NT(11)+NT(101)));
//G144: __N_2(119)           => __E_dir_arg_44(97).
	p(NT(119), (NT(97)));
//G145: dir_arg(92)          => treemr_pattern(96) & ~( __N_2(119) ).	 # conjunctive
	p(NT(92), (NT(96)) & ~(NT(119)));
//G146: __E_tree_path_48(102) => _(8) '>' _(8) dir_sym(11).
	p(NT(102), (NT(8)+T(4)+NT(8)+NT(11)));
//G147: __E_tree_path_49(103) => null.
	p(NT(103), (nul));
//G148: __E_tree_path_49(103) => __E_tree_path_49(103) __E_tree_path_48(102).
	p(NT(103), (NT(103)+NT(102)));
//G149: tree_path(95)        => dir_sym(11) _(8) '>' _(8) dir_sym(11) __E_tree_path_49(103).
	p(NT(95), (NT(11)+NT(8)+T(4)+NT(8)+NT(11)+NT(103)));
//G150: __E_dir_sym_50(104)  => alpha(3).
	p(NT(104), (NT(3)));
//G151: __E_dir_sym_50(104)  => '_'.
	p(NT(104), (T(19)));
//G152: __E_dir_sym_50(104)  => '*'.
	p(NT(104), (T(11)));
//G153: __E_dir_sym_51(105)  => alnum(2).
	p(NT(105), (NT(2)));
//G154: __E_dir_sym_51(105)  => '_'.
	p(NT(105), (T(19)));
//G155: __E_dir_sym_51(105)  => '*'.
	p(NT(105), (T(11)));
//G156: __E_dir_sym_52(106)  => null.
	p(NT(106), (nul));
//G157: __E_dir_sym_52(106)  => __E_dir_sym_52(106) __E_dir_sym_51(105).
	p(NT(106), (NT(106)+NT(105)));
//G158: dir_sym(11)          => __E_dir_sym_50(104) __E_dir_sym_52(106).
	p(NT(11), (NT(104)+NT(106)));
//G159: __E___E_treemr_pattern_53_54(109) => null.
	p(NT(109), (nul));
//G160: __E___E_treemr_pattern_53_54(109) => __E___E_treemr_pattern_53_54(109) pat_space(99).
	p(NT(109), (NT(109)+NT(99)));
//G161: __E_treemr_pattern_53(108) => __E___E_treemr_pattern_53_54(109) pat_unit(107).
	p(NT(108), (NT(109)+NT(107)));
//G162: __E_treemr_pattern_55(110) => null.
	p(NT(110), (nul));
//G163: __E_treemr_pattern_55(110) => __E_treemr_pattern_55(110) __E_treemr_pattern_53(108).
	p(NT(110), (NT(110)+NT(108)));
//G164: treemr_pattern(96)   => pat_unit(107) __E_treemr_pattern_55(110).
	p(NT(96), (NT(107)+NT(110)));
//G165: pat_unit(107)        => terminal_string(61).
	p(NT(107), (NT(61)));
//G166: pat_unit(107)        => terminal_char(60).
	p(NT(107), (NT(60)));
//G167: pat_unit(107)        => pat_char(111).
	p(NT(107), (NT(111)));
//G168: __E_pat_char_56(112) => space(4).
	p(NT(112), (NT(4)));
//G169: __E_pat_char_56(112) => ','.
	p(NT(112), (T(37)));
//G170: __E_pat_char_56(112) => '.'.
	p(NT(112), (T(5)));
//G171: __E_pat_char_56(112) => ':'.
	p(NT(112), (T(8)));
//G172: __E_pat_char_56(112) => ';'.
	p(NT(112), (T(35)));
//G173: __E_pat_char_56(112) => '#'.
	p(NT(112), (T(38)));
//G174: __E_pat_char_56(112) => '"'.
	p(NT(112), (T(22)));
//G175: __E_pat_char_56(112) => '\''.
	p(NT(112), (T(20)));
//G176: __N_3(120)           => __E_pat_char_56(112).
	p(NT(120), (NT(112)));
//G177: pat_char(111)        => printable(5) & ~( __N_3(120) ).	 # conjunctive
	p(NT(111), (NT(5)) & ~(NT(120)));
//G178: pat_space(99)        => ' '.
	p(NT(99), (T(39)));
//G179: pat_space(99)        => '\t'.
	p(NT(99), (T(40)));
//G180: _(8)                 => __(7).
	p(NT(8), (NT(7)));
//G181: _(8)                 => null.
	p(NT(8), (nul));
//G182: __(7)                => space(4).
	p(NT(7), (NT(4)));
//G183: __(7)                => comment(113).
	p(NT(7), (NT(113)));
//G184: __(7)                => __(7) space(4).
	p(NT(7), (NT(7)+NT(4)));
//G185: __(7)                => __(7) comment(113).
	p(NT(7), (NT(7)+NT(113)));
//G186: __E_comment_57(114)  => printable(5).
	p(NT(114), (NT(5)));
//G187: __E_comment_57(114)  => '\t'.
	p(NT(114), (T(40)));
//G188: __E_comment_58(115)  => null.
	p(NT(115), (nul));
//G189: __E_comment_58(115)  => __E_comment_58(115) __E_comment_57(114).
	p(NT(115), (NT(115)+NT(114)));
//G190: __E_comment_59(116)  => '\r'.
	p(NT(116), (T(41)));
//G191: __E_comment_59(116)  => '\n'.
	p(NT(116), (T(42)));
//G192: __E_comment_59(116)  => eof(1).
	p(NT(116), (NT(1)));
//G193: comment(113)         => '#' __E_comment_58(115) __E_comment_59(116).
	p(NT(113), (T(38)+NT(115)+NT(116)));
	#undef T
	#undef NT
	return loaded = true, p;
}
#endif

} // namespace tgf_parser_data
