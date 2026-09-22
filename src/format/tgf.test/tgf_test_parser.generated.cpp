// This file is generated from a file src/format/tgf.test/tgf.test.tgf by
//       https://github.com/IDNI/parser/src/tgf
//
// productions() lives here so the table is compiled once, not per TU.
//
#include "parser.h"
#include "tgf_test_parser.generated.h"

namespace tgf_test_parser_data {

#ifndef TAU_PARSER_BUILD_HEADER_ONLY
idni::prods<char_type, terminal_type>& productions() {
	static bool loaded = false;
	static idni::prods<char_type, terminal_type>
		p, nul(idni::lit<char_type, terminal_type>{});
	if (loaded) return p;
	#define  T(x) (idni::prods<char_type, terminal_type>{ terminals[x] })
	#define NT(x) (idni::prods<char_type, terminal_type>{ nts(x) })
//G0:   __E_start_0(24)      => entry(25) _(9).
	p(NT(24), (NT(25)+NT(9)));
//G1:   __E_start_1(26)      => null.
	p(NT(26), (nul));
//G2:   __E_start_1(26)      => __E_start_1(26) __E_start_0(24).
	p(NT(26), (NT(26)+NT(24)));
//G3:   start(23)            => _(9) __E_start_1(26).
	p(NT(23), (NT(9)+NT(26)));
//G4:   __E_entry_2(28)      => negate(27).
	p(NT(28), (NT(27)));
//G5:   __E_entry_2(28)      => null.
	p(NT(28), (nul));
//G6:   __E_entry_3(29)      => words(30) __(8).
	p(NT(29), (NT(30)+NT(8)));
//G7:   __E_entry_4(31)      => __E_entry_3(29).
	p(NT(31), (NT(29)));
//G8:   __E_entry_4(31)      => null.
	p(NT(31), (nul));
//G9:   entry(25)            => __E_entry_2(28) __E_entry_4(31) matcher(32) entry_tail(33).
	p(NT(25), (NT(28)+NT(31)+NT(32)+NT(33)));
//G10:  __E_entry_tail_5(36) => _(9) entry_terminator(37).
	p(NT(36), (NT(9)+NT(37)));
//G11:  __E_entry_tail_6(38) => __E_entry_tail_5(36).
	p(NT(38), (NT(36)));
//G12:  __E_entry_tail_6(38) => null.
	p(NT(38), (nul));
//G13:  entry_tail(33)       => _(9) ':' _(9) item_list(34) _(9) nested_block(35) __E_entry_tail_6(38).
	p(NT(33), (NT(9)+T(1)+NT(9)+NT(34)+NT(9)+NT(35)+NT(38)));
//G14:  entry_tail(33)       => _(9) ':' _(9) item_list(34) _(9) entry_terminator(37).
	p(NT(33), (NT(9)+T(1)+NT(9)+NT(34)+NT(9)+NT(37)));
//G15:  __E_entry_tail_7(39) => _(9) entry_terminator(37).
	p(NT(39), (NT(9)+NT(37)));
//G16:  __E_entry_tail_8(40) => __E_entry_tail_7(39).
	p(NT(40), (NT(39)));
//G17:  __E_entry_tail_8(40) => null.
	p(NT(40), (nul));
//G18:  entry_tail(33)       => _(9) nested_block(35) __E_entry_tail_8(40).
	p(NT(33), (NT(9)+NT(35)+NT(40)));
//G19:  entry_tail(33)       => _(9) entry_terminator(37).
	p(NT(33), (NT(9)+NT(37)));
//G20:  negate(27)           => '~' _(9).
	p(NT(27), (T(2)+NT(9)));
//G21:  __E_nested_block_9(41) => entry(25) _(9).
	p(NT(41), (NT(25)+NT(9)));
//G22:  __E_nested_block_10(42) => null.
	p(NT(42), (nul));
//G23:  __E_nested_block_10(42) => __E_nested_block_10(42) __E_nested_block_9(41).
	p(NT(42), (NT(42)+NT(41)));
//G24:  nested_block(35)     => '{' _(9) __E_nested_block_10(42) '}'.
	p(NT(35), (T(3)+NT(9)+NT(42)+T(4)));
//G25:  entry_terminator(37) => '.'.
	p(NT(37), (T(5)));
//G26:  __E_item_list_11(44) => _(9) ',' _(9) item(43).
	p(NT(44), (NT(9)+T(6)+NT(9)+NT(43)));
//G27:  __E_item_list_12(45) => null.
	p(NT(45), (nul));
//G28:  __E_item_list_12(45) => __E_item_list_12(45) __E_item_list_11(44).
	p(NT(45), (NT(45)+NT(44)));
//G29:  item_list(34)        => item(43) __E_item_list_12(45).
	p(NT(34), (NT(43)+NT(45)));
//G30:  __E_item_13(46)      => negate(27).
	p(NT(46), (NT(27)));
//G31:  __E_item_13(46)      => null.
	p(NT(46), (nul));
//G32:  __E_item_14(48)      => __(8) words(30).
	p(NT(48), (NT(8)+NT(30)));
//G33:  __E_item_15(49)      => __E_item_14(48).
	p(NT(49), (NT(48)));
//G34:  __E_item_15(49)      => null.
	p(NT(49), (nul));
//G35:  __E_item_16(50)      => _(9) ':' _(9) matcher(32).
	p(NT(50), (NT(9)+T(1)+NT(9)+NT(32)));
//G36:  __E_item_17(51)      => __E_item_16(50).
	p(NT(51), (NT(50)));
//G37:  __E_item_17(51)      => null.
	p(NT(51), (nul));
//G38:  item(43)             => __E_item_13(46) input(47) __E_item_15(49) __E_item_17(51).
	p(NT(43), (NT(46)+NT(47)+NT(49)+NT(51)));
//G39:  __E_words_18(53)     => __(8) word(52).
	p(NT(53), (NT(8)+NT(52)));
//G40:  __E_words_19(54)     => null.
	p(NT(54), (nul));
//G41:  __E_words_19(54)     => __E_words_19(54) __E_words_18(53).
	p(NT(54), (NT(54)+NT(53)));
//G42:  words(30)            => word(52) __E_words_19(54).
	p(NT(30), (NT(52)+NT(54)));
//G43:  __E_word_20(55)      => w_forbid(56).
	p(NT(55), (NT(56)));
//G44:  __E_word_20(55)      => w_unique(57).
	p(NT(55), (NT(57)));
//G45:  __E_word_20(55)      => w_all(58).
	p(NT(55), (NT(58)));
//G46:  __E_word_20(55)      => w_any(59).
	p(NT(55), (NT(59)));
//G47:  __E_word_20(55)      => w_raw(60).
	p(NT(55), (NT(60)));
//G48:  word(52)             => '@' __E_word_20(55).
	p(NT(52), (T(7)+NT(55)));
//G49:  w_forbid(56)         => 'f' 'o' 'r' 'b' 'i' 'd'.
	p(NT(56), (T(8)+T(9)+T(10)+T(11)+T(12)+T(13)));
//G50:  w_unique(57)         => 'u' 'n' 'i' 'q' 'u' 'e'.
	p(NT(57), (T(14)+T(15)+T(12)+T(16)+T(14)+T(17)));
//G51:  w_all(58)            => 'a' 'l' 'l'.
	p(NT(58), (T(18)+T(19)+T(19)));
//G52:  w_any(59)            => 'a' 'n' 'y'.
	p(NT(59), (T(18)+T(15)+T(20)));
//G53:  w_raw(60)            => 'r' 'a' 'w'.
	p(NT(60), (T(10)+T(18)+T(21)));
//G54:  input(47)            => quoted_string(13).
	p(NT(47), (NT(13)));
//G55:  input(47)            => name(10).
	p(NT(47), (NT(10)));
//G56:  __E___E_matcher_21_22(63) => null.
	p(NT(63), (nul));
//G57:  __E___E_matcher_21_22(63) => __E___E_matcher_21_22(63) m_space(12).
	p(NT(63), (NT(63)+NT(12)));
//G58:  __E_matcher_21(62)   => __E___E_matcher_21_22(63) m_unit(61).
	p(NT(62), (NT(63)+NT(61)));
//G59:  __E_matcher_23(64)   => null.
	p(NT(64), (nul));
//G60:  __E_matcher_23(64)   => __E_matcher_23(64) __E_matcher_21(62).
	p(NT(64), (NT(64)+NT(62)));
//G61:  matcher(32)          => m_unit(61) __E_matcher_23(64).
	p(NT(32), (NT(61)+NT(64)));
//G62:  m_unit(61)           => quoted_string(13).
	p(NT(61), (NT(13)));
//G63:  m_unit(61)           => char_lit(14).
	p(NT(61), (NT(14)));
//G64:  m_unit(61)           => m_char(11).
	p(NT(61), (NT(11)));
//G65:  __E_m_char_24(65)    => ':'.
	p(NT(65), (T(1)));
//G66:  __E_m_char_24(65)    => '{'.
	p(NT(65), (T(3)));
//G67:  __E_m_char_24(65)    => '}'.
	p(NT(65), (T(4)));
//G68:  __E_m_char_24(65)    => '.'.
	p(NT(65), (T(5)));
//G69:  __E_m_char_24(65)    => ','.
	p(NT(65), (T(6)));
//G70:  __E_m_char_24(65)    => '\''.
	p(NT(65), (T(22)));
//G71:  __E_m_char_24(65)    => '"'.
	p(NT(65), (T(23)));
//G72:  __E_m_char_24(65)    => '@'.
	p(NT(65), (T(7)));
//G73:  __E_m_char_24(65)    => '#'.
	p(NT(65), (T(24)));
//G74:  __E_m_char_24(65)    => '~'.
	p(NT(65), (T(2)));
//G75:  __N_0(85)            => __E_m_char_24(65).
	p(NT(85), (NT(65)));
//G76:  m_char(11)           => graph(5) & ~( __N_0(85) ).	 # conjunctive
	p(NT(11), (NT(5)) & ~(NT(85)));
//G77:  m_space(12)          => ' '.
	p(NT(12), (T(25)));
//G78:  m_space(12)          => '\t'.
	p(NT(12), (T(26)));
//G79:  __E_name_25(66)      => alpha(2).
	p(NT(66), (NT(2)));
//G80:  __E_name_25(66)      => '_'.
	p(NT(66), (T(27)));
//G81:  __E_name_26(67)      => alnum(3).
	p(NT(67), (NT(3)));
//G82:  __E_name_26(67)      => '_'.
	p(NT(67), (T(27)));
//G83:  __E_name_27(68)      => null.
	p(NT(68), (nul));
//G84:  __E_name_27(68)      => __E_name_27(68) __E_name_26(67).
	p(NT(68), (NT(68)+NT(67)));
//G85:  name(10)             => __E_name_25(66) __E_name_27(68).
	p(NT(10), (NT(66)+NT(68)));
//G86:  char_lit(14)         => '\'' char_lit_char(69) '\''.
	p(NT(14), (T(22)+NT(69)+T(22)));
//G87:  char_lit_char(69)    => unescaped_c(17).
	p(NT(69), (NT(17)));
//G88:  char_lit_char(69)    => escaped_c(18).
	p(NT(69), (NT(18)));
//G89:  __E_unescaped_c_28(70) => space(4).
	p(NT(70), (NT(4)));
//G90:  __E_unescaped_c_28(70) => printable(6).
	p(NT(70), (NT(6)));
//G91:  __E_unescaped_c_29(71) => '\''.
	p(NT(71), (T(22)));
//G92:  __E_unescaped_c_29(71) => '\\'.
	p(NT(71), (T(28)));
//G93:  __N_1(86)            => __E_unescaped_c_29(71).
	p(NT(86), (NT(71)));
//G94:  unescaped_c(17)      => __E_unescaped_c_28(70) & ~( __N_1(86) ).	 # conjunctive
	p(NT(17), (NT(70)) & ~(NT(86)));
//G95:  __E_escaped_c_30(72) => '\''.
	p(NT(72), (T(22)));
//G96:  __E_escaped_c_30(72) => escape_char(19).
	p(NT(72), (NT(19)));
//G97:  escaped_c(18)        => '\\' __E_escaped_c_30(72).
	p(NT(18), (T(28)+NT(72)));
//G98:  __E_quoted_string_31(74) => null.
	p(NT(74), (nul));
//G99:  __E_quoted_string_31(74) => __E_quoted_string_31(74) quoted_string_char(73).
	p(NT(74), (NT(74)+NT(73)));
//G100: quoted_string(13)    => '"' __E_quoted_string_31(74) '"'.
	p(NT(13), (T(23)+NT(74)+T(23)));
//G101: quoted_string_char(73) => unescaped_s(15).
	p(NT(73), (NT(15)));
//G102: quoted_string_char(73) => escaped_s(16).
	p(NT(73), (NT(16)));
//G103: __E_unescaped_s_32(75) => space(4).
	p(NT(75), (NT(4)));
//G104: __E_unescaped_s_32(75) => printable(6).
	p(NT(75), (NT(6)));
//G105: __E_unescaped_s_33(76) => '"'.
	p(NT(76), (T(23)));
//G106: __E_unescaped_s_33(76) => '\\'.
	p(NT(76), (T(28)));
//G107: __N_2(87)            => __E_unescaped_s_33(76).
	p(NT(87), (NT(76)));
//G108: unescaped_s(15)      => __E_unescaped_s_32(75) & ~( __N_2(87) ).	 # conjunctive
	p(NT(15), (NT(75)) & ~(NT(87)));
//G109: __E_escaped_s_34(77) => '"'.
	p(NT(77), (T(23)));
//G110: __E_escaped_s_34(77) => escape_char(19).
	p(NT(77), (NT(19)));
//G111: escaped_s(16)        => '\\' __E_escaped_s_34(77).
	p(NT(16), (T(28)+NT(77)));
//G112: escape_char(19)      => 'a'.
	p(NT(19), (T(18)));
//G113: escape_char(19)      => 'b'.
	p(NT(19), (T(11)));
//G114: escape_char(19)      => 'f'.
	p(NT(19), (T(8)));
//G115: escape_char(19)      => 'n'.
	p(NT(19), (T(15)));
//G116: escape_char(19)      => 'r'.
	p(NT(19), (T(10)));
//G117: escape_char(19)      => 't'.
	p(NT(19), (T(29)));
//G118: escape_char(19)      => 'v'.
	p(NT(19), (T(30)));
//G119: escape_char(19)      => '\\'.
	p(NT(19), (T(28)));
//G120: escape_char(19)      => '/'.
	p(NT(19), (T(31)));
//G121: escape_char(19)      => esc_hex(20).
	p(NT(19), (NT(20)));
//G122: escape_char(19)      => esc_u4(21).
	p(NT(19), (NT(21)));
//G123: escape_char(19)      => esc_U8(22).
	p(NT(19), (NT(22)));
//G124: __E_esc_hex_35(78)   => 'x'.
	p(NT(78), (T(32)));
//G125: __E_esc_hex_35(78)   => 'X'.
	p(NT(78), (T(33)));
//G126: esc_hex(20)          => __E_esc_hex_35(78) xdigit(7) xdigit(7).
	p(NT(20), (NT(78)+NT(7)+NT(7)));
//G127: esc_u4(21)           => 'u' xdigit(7) xdigit(7) xdigit(7) xdigit(7).
	p(NT(21), (T(14)+NT(7)+NT(7)+NT(7)+NT(7)));
//G128: esc_U8(22)           => 'U' xdigit(7) xdigit(7) xdigit(7) xdigit(7) xdigit(7) xdigit(7) xdigit(7) xdigit(7).
	p(NT(22), (T(34)+NT(7)+NT(7)+NT(7)+NT(7)+NT(7)+NT(7)+NT(7)+NT(7)));
//G129: __E___36(79)         => __(8).
	p(NT(79), (NT(8)));
//G130: __E___36(79)         => null.
	p(NT(79), (nul));
//G131: _(9)                 => __E___36(79).
	p(NT(9), (NT(79)));
//G132: __E____37(80)        => space(4).
	p(NT(80), (NT(4)));
//G133: __E____37(80)        => comment(81).
	p(NT(80), (NT(81)));
//G134: __(8)                => __E____37(80) _(9).
	p(NT(8), (NT(80)+NT(9)));
//G135: __E_comment_38(82)   => printable(6).
	p(NT(82), (NT(6)));
//G136: __E_comment_38(82)   => '\t'.
	p(NT(82), (T(26)));
//G137: __E_comment_39(83)   => null.
	p(NT(83), (nul));
//G138: __E_comment_39(83)   => __E_comment_39(83) __E_comment_38(82).
	p(NT(83), (NT(83)+NT(82)));
//G139: __E_comment_40(84)   => '\r'.
	p(NT(84), (T(35)));
//G140: __E_comment_40(84)   => '\n'.
	p(NT(84), (T(36)));
//G141: __E_comment_40(84)   => eof(1).
	p(NT(84), (NT(1)));
//G142: comment(81)          => '#' __E_comment_39(83) __E_comment_40(84).
	p(NT(81), (T(24)+NT(83)+NT(84)));
	#undef T
	#undef NT
	return loaded = true, p;
}
#endif

} // namespace tgf_test_parser_data
