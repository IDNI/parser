// This file is generated from a file src/format/treemr/treemr.tgf by
//       https://github.com/IDNI/parser/src/tgf
//
// productions() lives here so the table is compiled once, not per TU.
//
#include "parser.h"
#include "treemr_parser.generated.h"

namespace treemr_parser_data {

#ifndef TAU_PARSER_BUILD_HEADER_ONLY
idni::prods<char_type, terminal_type>& productions() {
	static bool loaded = false;
	static idni::prods<char_type, terminal_type>
		p, nul(idni::lit<char_type, terminal_type>{});
	if (loaded) return p;
	const auto& ts = terminals();
	auto& ns = nts();
	#define  T(x) (idni::prods<char_type, terminal_type>{ ts[x] })
	#define NT(x) (idni::prods<char_type, terminal_type>{ ns(x) })
//G0:   start(7)             => _(8) pattern(9) _(8).
	p(NT(7), (NT(8)+NT(9)+NT(8)));
//G1:   __E_pattern_0(10)    => root_anchor(11) _(8).
	p(NT(10), (NT(11)+NT(8)));
//G2:   __E_pattern_0(10)    => null.
	p(NT(10), (nul));
//G3:   pattern(9)           => __E_pattern_0(10) sib_seq(12).
	p(NT(9), (NT(10)+NT(12)));
//G4:   root_anchor(11)      => '/'.
	p(NT(11), (T(1)));
//G5:   sib_seq(12)          => slot_chain(13).
	p(NT(12), (NT(13)));
//G6:   __E_slot_chain_1(15) => __(16) edge_slot(17).
	p(NT(15), (NT(16)+NT(17)));
//G7:   __E_slot_chain_1(15) => null.
	p(NT(15), (nul));
//G8:   slot_chain(13)       => simple_slots(14) __E_slot_chain_1(15).
	p(NT(13), (NT(14)+NT(15)));
//G9:   slot_chain(13)       => edge_slot(17).
	p(NT(13), (NT(17)));
//G10:  __E_simple_slots_2(19) => __(16) simple_slot(18).
	p(NT(19), (NT(16)+NT(18)));
//G11:  __E_simple_slots_3(20) => null.
	p(NT(20), (nul));
//G12:  __E_simple_slots_3(20) => __E_simple_slots_3(20) __E_simple_slots_2(19).
	p(NT(20), (NT(20)+NT(19)));
//G13:  simple_slots(14)     => simple_slot(18) __E_simple_slots_3(20).
	p(NT(14), (NT(18)+NT(20)));
//G14:  simple_slot(18)      => left_anchor(21).
	p(NT(18), (NT(21)));
//G15:  simple_slot(18)      => right_anchor(22).
	p(NT(18), (NT(22)));
//G16:  __E_simple_slot_4(24) => _(8) leaf_anchor(25).
	p(NT(24), (NT(8)+NT(25)));
//G17:  __E_simple_slot_4(24) => null.
	p(NT(24), (nul));
//G18:  __E_simple_slot_5(26) => _(8) quantifier(27).
	p(NT(26), (NT(8)+NT(27)));
//G19:  __E_simple_slot_5(26) => null.
	p(NT(26), (nul));
//G20:  simple_slot(18)      => atom(23) __E_simple_slot_4(24) __E_simple_slot_5(26).
	p(NT(18), (NT(23)+NT(24)+NT(26)));
//G21:  left_anchor(21)      => '^'.
	p(NT(21), (T(2)));
//G22:  right_anchor(22)     => '$'.
	p(NT(22), (T(3)));
//G23:  leaf_anchor(25)      => '!'.
	p(NT(25), (T(4)));
//G24:  quantifier(27)       => star(28).
	p(NT(27), (NT(28)));
//G25:  quantifier(27)       => plus(29).
	p(NT(27), (NT(29)));
//G26:  quantifier(27)       => opt(30).
	p(NT(27), (NT(30)));
//G27:  star(28)             => '*'.
	p(NT(28), (T(5)));
//G28:  plus(29)             => '+'.
	p(NT(29), (T(6)));
//G29:  opt(30)              => '?'.
	p(NT(30), (T(7)));
//G30:  __E_edge_slot_6(31)  => _(8) leaf_anchor(25).
	p(NT(31), (NT(8)+NT(25)));
//G31:  __E_edge_slot_6(31)  => null.
	p(NT(31), (nul));
//G32:  __E_edge_slot_7(32)  => _(8) quantifier(27).
	p(NT(32), (NT(8)+NT(27)));
//G33:  __E_edge_slot_7(32)  => null.
	p(NT(32), (nul));
//G34:  edge_slot(17)        => atom(23) __E_edge_slot_6(31) __E_edge_slot_7(32) _(8) edge(33) _(8) sib_seq(12).
	p(NT(17), (NT(23)+NT(31)+NT(32)+NT(8)+NT(33)+NT(8)+NT(12)));
//G35:  atom(23)             => name(34).
	p(NT(23), (NT(34)));
//G36:  atom(23)             => wildcard(35).
	p(NT(23), (NT(35)));
//G37:  atom(23)             => terminal(36).
	p(NT(23), (NT(36)));
//G38:  atom(23)             => capture(37).
	p(NT(23), (NT(37)));
//G39:  wildcard(35)         => '%'.
	p(NT(35), (T(8)));
//G40:  capture(37)          => '(' _(8) alt_seq(38) _(8) ')'.
	p(NT(37), (T(9)+NT(8)+NT(38)+NT(8)+T(10)));
//G41:  __E_alt_seq_8(39)    => _(8) '|' _(8) sib_seq(12).
	p(NT(39), (NT(8)+T(11)+NT(8)+NT(12)));
//G42:  __E_alt_seq_9(40)    => null.
	p(NT(40), (nul));
//G43:  __E_alt_seq_9(40)    => __E_alt_seq_9(40) __E_alt_seq_8(39).
	p(NT(40), (NT(40)+NT(39)));
//G44:  alt_seq(38)          => sib_seq(12) __E_alt_seq_9(40).
	p(NT(38), (NT(12)+NT(40)));
//G45:  edge(33)             => descendant_edge(41).
	p(NT(33), (NT(41)));
//G46:  edge(33)             => direct_edge(42).
	p(NT(33), (NT(42)));
//G47:  descendant_edge(41)  => '>' '>'.
	p(NT(41), (T(12)+T(12)));
//G48:  direct_edge(42)      => '>'.
	p(NT(42), (T(12)));
//G49:  __E_name_10(43)      => alpha(2).
	p(NT(43), (NT(2)));
//G50:  __E_name_10(43)      => '_'.
	p(NT(43), (T(13)));
//G51:  __E_name_11(44)      => alnum(3).
	p(NT(44), (NT(3)));
//G52:  __E_name_11(44)      => '_'.
	p(NT(44), (T(13)));
//G53:  __E_name_12(45)      => null.
	p(NT(45), (nul));
//G54:  __E_name_12(45)      => __E_name_12(45) __E_name_11(44).
	p(NT(45), (NT(45)+NT(44)));
//G55:  name(34)             => __E_name_10(43) __E_name_12(45).
	p(NT(34), (NT(43)+NT(45)));
//G56:  terminal(36)         => char_lit(46).
	p(NT(36), (NT(46)));
//G57:  terminal(36)         => str_lit(47).
	p(NT(36), (NT(47)));
//G58:  char_lit(46)         => '\'' char_lit_char(48) '\''.
	p(NT(46), (T(14)+NT(48)+T(14)));
//G59:  char_lit_char(48)    => unescaped_c(49).
	p(NT(48), (NT(49)));
//G60:  char_lit_char(48)    => escaped_c(50).
	p(NT(48), (NT(50)));
//G61:  __E_unescaped_c_13(51) => space(4).
	p(NT(51), (NT(4)));
//G62:  __E_unescaped_c_13(51) => printable(5).
	p(NT(51), (NT(5)));
//G63:  __E_unescaped_c_14(52) => '\''.
	p(NT(52), (T(14)));
//G64:  __E_unescaped_c_14(52) => '\\'.
	p(NT(52), (T(15)));
//G65:  __N_0(72)            => __E_unescaped_c_14(52).
	p(NT(72), (NT(52)));
//G66:  unescaped_c(49)      => __E_unescaped_c_13(51) & ~( __N_0(72) ).	 # conjunctive
	p(NT(49), (NT(51)) & ~(NT(72)));
//G67:  __E_escaped_c_15(53) => '\''.
	p(NT(53), (T(14)));
//G68:  __E_escaped_c_15(53) => escape_char(54).
	p(NT(53), (NT(54)));
//G69:  escaped_c(50)        => '\\' __E_escaped_c_15(53).
	p(NT(50), (T(15)+NT(53)));
//G70:  __E_str_lit_16(56)   => null.
	p(NT(56), (nul));
//G71:  __E_str_lit_16(56)   => __E_str_lit_16(56) str_lit_char(55).
	p(NT(56), (NT(56)+NT(55)));
//G72:  str_lit(47)          => '"' __E_str_lit_16(56) '"'.
	p(NT(47), (T(16)+NT(56)+T(16)));
//G73:  str_lit_char(55)     => unescaped_s(57).
	p(NT(55), (NT(57)));
//G74:  str_lit_char(55)     => escaped_s(58).
	p(NT(55), (NT(58)));
//G75:  __E_unescaped_s_17(59) => space(4).
	p(NT(59), (NT(4)));
//G76:  __E_unescaped_s_17(59) => printable(5).
	p(NT(59), (NT(5)));
//G77:  __E_unescaped_s_18(60) => '"'.
	p(NT(60), (T(16)));
//G78:  __E_unescaped_s_18(60) => '\\'.
	p(NT(60), (T(15)));
//G79:  __N_1(73)            => __E_unescaped_s_18(60).
	p(NT(73), (NT(60)));
//G80:  unescaped_s(57)      => __E_unescaped_s_17(59) & ~( __N_1(73) ).	 # conjunctive
	p(NT(57), (NT(59)) & ~(NT(73)));
//G81:  __E_escaped_s_19(61) => '"'.
	p(NT(61), (T(16)));
//G82:  __E_escaped_s_19(61) => escape_char(54).
	p(NT(61), (NT(54)));
//G83:  escaped_s(58)        => '\\' __E_escaped_s_19(61).
	p(NT(58), (T(15)+NT(61)));
//G84:  escape_char(54)      => 'a'.
	p(NT(54), (T(17)));
//G85:  escape_char(54)      => 'b'.
	p(NT(54), (T(18)));
//G86:  escape_char(54)      => 'f'.
	p(NT(54), (T(19)));
//G87:  escape_char(54)      => 'n'.
	p(NT(54), (T(20)));
//G88:  escape_char(54)      => 'r'.
	p(NT(54), (T(21)));
//G89:  escape_char(54)      => 't'.
	p(NT(54), (T(22)));
//G90:  escape_char(54)      => 'v'.
	p(NT(54), (T(23)));
//G91:  escape_char(54)      => '\\'.
	p(NT(54), (T(15)));
//G92:  escape_char(54)      => '/'.
	p(NT(54), (T(1)));
//G93:  escape_char(54)      => esc_hex(62).
	p(NT(54), (NT(62)));
//G94:  escape_char(54)      => esc_u4(63).
	p(NT(54), (NT(63)));
//G95:  escape_char(54)      => esc_U8(64).
	p(NT(54), (NT(64)));
//G96:  __E_esc_hex_20(65)   => 'x'.
	p(NT(65), (T(24)));
//G97:  __E_esc_hex_20(65)   => 'X'.
	p(NT(65), (T(25)));
//G98:  esc_hex(62)          => __E_esc_hex_20(65) xdigit(6) xdigit(6).
	p(NT(62), (NT(65)+NT(6)+NT(6)));
//G99:  esc_u4(63)           => 'u' xdigit(6) xdigit(6) xdigit(6) xdigit(6).
	p(NT(63), (T(26)+NT(6)+NT(6)+NT(6)+NT(6)));
//G100: esc_U8(64)           => 'U' xdigit(6) xdigit(6) xdigit(6) xdigit(6) xdigit(6) xdigit(6) xdigit(6) xdigit(6).
	p(NT(64), (T(27)+NT(6)+NT(6)+NT(6)+NT(6)+NT(6)+NT(6)+NT(6)+NT(6)));
//G101: __E___21(66)         => __(16).
	p(NT(66), (NT(16)));
//G102: __E___21(66)         => null.
	p(NT(66), (nul));
//G103: _(8)                 => __E___21(66).
	p(NT(8), (NT(66)));
//G104: __E____22(67)        => space(4).
	p(NT(67), (NT(4)));
//G105: __E____22(67)        => comment(68).
	p(NT(67), (NT(68)));
//G106: __(16)               => __E____22(67) _(8).
	p(NT(16), (NT(67)+NT(8)));
//G107: __E_comment_23(69)   => printable(5).
	p(NT(69), (NT(5)));
//G108: __E_comment_23(69)   => '\t'.
	p(NT(69), (T(28)));
//G109: __E_comment_24(70)   => null.
	p(NT(70), (nul));
//G110: __E_comment_24(70)   => __E_comment_24(70) __E_comment_23(69).
	p(NT(70), (NT(70)+NT(69)));
//G111: __E_comment_25(71)   => '\r'.
	p(NT(71), (T(29)));
//G112: __E_comment_25(71)   => '\n'.
	p(NT(71), (T(30)));
//G113: __E_comment_25(71)   => eof(1).
	p(NT(71), (NT(1)));
//G114: comment(68)          => '#' __E_comment_24(70) __E_comment_25(71).
	p(NT(68), (T(31)+NT(70)+NT(71)));
	#undef T
	#undef NT
	return loaded = true, p;
}
#endif

} // namespace treemr_parser_data
