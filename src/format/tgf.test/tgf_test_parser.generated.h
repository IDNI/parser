// This file is generated from a file src/format/tgf.test/tgf.test.tgf by
//       https://github.com/IDNI/parser/src/tgf
//
#ifndef __TGF_TEST_PARSER_H__
#define __TGF_TEST_PARSER_H__

#include "parser.h"
#include "recoders.h"

namespace tgf_test_parser_data {

using char_type     = char;
using terminal_type = char32_t;

inline static constexpr size_t nt_bits = 7;

inline const std::vector<std::string>& symbol_names() {
	static const std::vector<std::string> v{
	"", "eof", "alpha", "alnum", "space", "graph", "printable", "xdigit", "start", "_", 
	"__E_start_0", "entry", "__E_start_1", "negate", "__E_entry_2", "__E_entry_3", "words", "__", "__E_entry_4", "matcher", 
	"entry_tail", "item_list", "nested_block", "__E_entry_tail_5", "entry_terminator", "__E_entry_tail_6", "__E_entry_tail_7", "__E_entry_tail_8", "__E_nested_block_9", "__E_nested_block_10", 
	"item", "__E_item_list_11", "__E_item_list_12", "__E_item_13", "input", "__E_item_14", "__E_item_15", "__E_item_16", "__E_item_17", "word", 
	"__E_words_18", "__E_words_19", "__E_word_20", "w_forbid", "w_unique", "w_all", "w_any", "w_raw", "quoted_string", "name", 
	"m_unit", "__E_matcher_21", "m_space", "__E___E_matcher_21_22", "__E_matcher_23", "char_lit", "m_char", "__E_m_char_24", "__E_name_25", "__E_name_26", 
	"__E_name_27", "char_lit_char", "unescaped_c", "escaped_c", "__E_unescaped_c_28", "__E_unescaped_c_29", "__E_escaped_c_30", "escape_char", "quoted_string_char", "__E_quoted_string_31", 
	"unescaped_s", "escaped_s", "__E_unescaped_s_32", "__E_unescaped_s_33", "__E_escaped_s_34", "esc_hex", "esc_u4", "esc_U8", "__E_esc_hex_35", "__E___36", 
	"__E____37", "comment", "__E_comment_38", "__E_comment_39", "__E_comment_40", "__N_0", "__N_1", "__N_2", 
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
	U'\0', U':', U'~', U'{', U'}', U'.', U',', U'@', U'f', 
	U'o', U'r', U'b', U'i', U'd', U'u', U'n', U'q', U'e', U'a', 
	U'l', U'y', U'w', U'\'', U'"', U'#', U' ', U'\t', U'_', U'\\', 
	U't', U'v', U'/', U'x', U'X', U'U', U'\r', U'\n', 
};
	return v;
}

inline ::idni::char_class_fns<terminal_type>& char_classes() {
	static ::idni::char_class_fns<terminal_type> v =
	::idni::predefined_char_classes<char_type, terminal_type>({
		"eof",
		"alpha",
		"alnum",
		"space",
		"graph",
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
			9, 17
		},
		.trim_terminals = true,
		.dont_trim_terminals_of = {
			48, 49, 52, 55, 56, 62, 63, 67, 70, 71,
			75, 76, 77
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
	static ::idni::prods<char_type, terminal_type> v{ nts()(8) };
	return v;
}

#ifdef TAU_PARSER_BUILD_HEADER_ONLY
inline idni::prods<char_type, terminal_type>& productions() {
	static bool loaded = false;
	static idni::prods<char_type, terminal_type>
		p, nul(idni::lit<char_type, terminal_type>{});
	if (loaded) return p;
	const auto& ts = terminals();
	auto& ns = nts();
	#define  T(x) (idni::prods<char_type, terminal_type>{ ts[x] })
	#define NT(x) (idni::prods<char_type, terminal_type>{ ns(x) })
//G0:   __E_start_0(10)      => entry(11) _(9).
	p(NT(10), (NT(11)+NT(9)));
//G1:   __E_start_1(12)      => null.
	p(NT(12), (nul));
//G2:   __E_start_1(12)      => __E_start_1(12) __E_start_0(10).
	p(NT(12), (NT(12)+NT(10)));
//G3:   start(8)             => _(9) __E_start_1(12).
	p(NT(8), (NT(9)+NT(12)));
//G4:   __E_entry_2(14)      => negate(13).
	p(NT(14), (NT(13)));
//G5:   __E_entry_2(14)      => null.
	p(NT(14), (nul));
//G6:   __E_entry_3(15)      => words(16) __(17).
	p(NT(15), (NT(16)+NT(17)));
//G7:   __E_entry_4(18)      => __E_entry_3(15).
	p(NT(18), (NT(15)));
//G8:   __E_entry_4(18)      => null.
	p(NT(18), (nul));
//G9:   entry(11)            => __E_entry_2(14) __E_entry_4(18) matcher(19) entry_tail(20).
	p(NT(11), (NT(14)+NT(18)+NT(19)+NT(20)));
//G10:  __E_entry_tail_5(23) => _(9) entry_terminator(24).
	p(NT(23), (NT(9)+NT(24)));
//G11:  __E_entry_tail_6(25) => __E_entry_tail_5(23).
	p(NT(25), (NT(23)));
//G12:  __E_entry_tail_6(25) => null.
	p(NT(25), (nul));
//G13:  entry_tail(20)       => _(9) ':' _(9) item_list(21) _(9) nested_block(22) __E_entry_tail_6(25).
	p(NT(20), (NT(9)+T(1)+NT(9)+NT(21)+NT(9)+NT(22)+NT(25)));
//G14:  entry_tail(20)       => _(9) ':' _(9) item_list(21) _(9) entry_terminator(24).
	p(NT(20), (NT(9)+T(1)+NT(9)+NT(21)+NT(9)+NT(24)));
//G15:  __E_entry_tail_7(26) => _(9) entry_terminator(24).
	p(NT(26), (NT(9)+NT(24)));
//G16:  __E_entry_tail_8(27) => __E_entry_tail_7(26).
	p(NT(27), (NT(26)));
//G17:  __E_entry_tail_8(27) => null.
	p(NT(27), (nul));
//G18:  entry_tail(20)       => _(9) nested_block(22) __E_entry_tail_8(27).
	p(NT(20), (NT(9)+NT(22)+NT(27)));
//G19:  entry_tail(20)       => _(9) entry_terminator(24).
	p(NT(20), (NT(9)+NT(24)));
//G20:  negate(13)           => '~' _(9).
	p(NT(13), (T(2)+NT(9)));
//G21:  __E_nested_block_9(28) => entry(11) _(9).
	p(NT(28), (NT(11)+NT(9)));
//G22:  __E_nested_block_10(29) => null.
	p(NT(29), (nul));
//G23:  __E_nested_block_10(29) => __E_nested_block_10(29) __E_nested_block_9(28).
	p(NT(29), (NT(29)+NT(28)));
//G24:  nested_block(22)     => '{' _(9) __E_nested_block_10(29) '}'.
	p(NT(22), (T(3)+NT(9)+NT(29)+T(4)));
//G25:  entry_terminator(24) => '.'.
	p(NT(24), (T(5)));
//G26:  __E_item_list_11(31) => _(9) ',' _(9) item(30).
	p(NT(31), (NT(9)+T(6)+NT(9)+NT(30)));
//G27:  __E_item_list_12(32) => null.
	p(NT(32), (nul));
//G28:  __E_item_list_12(32) => __E_item_list_12(32) __E_item_list_11(31).
	p(NT(32), (NT(32)+NT(31)));
//G29:  item_list(21)        => item(30) __E_item_list_12(32).
	p(NT(21), (NT(30)+NT(32)));
//G30:  __E_item_13(33)      => negate(13).
	p(NT(33), (NT(13)));
//G31:  __E_item_13(33)      => null.
	p(NT(33), (nul));
//G32:  __E_item_14(35)      => __(17) words(16).
	p(NT(35), (NT(17)+NT(16)));
//G33:  __E_item_15(36)      => __E_item_14(35).
	p(NT(36), (NT(35)));
//G34:  __E_item_15(36)      => null.
	p(NT(36), (nul));
//G35:  __E_item_16(37)      => _(9) ':' _(9) matcher(19).
	p(NT(37), (NT(9)+T(1)+NT(9)+NT(19)));
//G36:  __E_item_17(38)      => __E_item_16(37).
	p(NT(38), (NT(37)));
//G37:  __E_item_17(38)      => null.
	p(NT(38), (nul));
//G38:  item(30)             => __E_item_13(33) input(34) __E_item_15(36) __E_item_17(38).
	p(NT(30), (NT(33)+NT(34)+NT(36)+NT(38)));
//G39:  __E_words_18(40)     => __(17) word(39).
	p(NT(40), (NT(17)+NT(39)));
//G40:  __E_words_19(41)     => null.
	p(NT(41), (nul));
//G41:  __E_words_19(41)     => __E_words_19(41) __E_words_18(40).
	p(NT(41), (NT(41)+NT(40)));
//G42:  words(16)            => word(39) __E_words_19(41).
	p(NT(16), (NT(39)+NT(41)));
//G43:  __E_word_20(42)      => w_forbid(43).
	p(NT(42), (NT(43)));
//G44:  __E_word_20(42)      => w_unique(44).
	p(NT(42), (NT(44)));
//G45:  __E_word_20(42)      => w_all(45).
	p(NT(42), (NT(45)));
//G46:  __E_word_20(42)      => w_any(46).
	p(NT(42), (NT(46)));
//G47:  __E_word_20(42)      => w_raw(47).
	p(NT(42), (NT(47)));
//G48:  word(39)             => '@' __E_word_20(42).
	p(NT(39), (T(7)+NT(42)));
//G49:  w_forbid(43)         => 'f' 'o' 'r' 'b' 'i' 'd'.
	p(NT(43), (T(8)+T(9)+T(10)+T(11)+T(12)+T(13)));
//G50:  w_unique(44)         => 'u' 'n' 'i' 'q' 'u' 'e'.
	p(NT(44), (T(14)+T(15)+T(12)+T(16)+T(14)+T(17)));
//G51:  w_all(45)            => 'a' 'l' 'l'.
	p(NT(45), (T(18)+T(19)+T(19)));
//G52:  w_any(46)            => 'a' 'n' 'y'.
	p(NT(46), (T(18)+T(15)+T(20)));
//G53:  w_raw(47)            => 'r' 'a' 'w'.
	p(NT(47), (T(10)+T(18)+T(21)));
//G54:  input(34)            => quoted_string(48).
	p(NT(34), (NT(48)));
//G55:  input(34)            => name(49).
	p(NT(34), (NT(49)));
//G56:  __E___E_matcher_21_22(53) => null.
	p(NT(53), (nul));
//G57:  __E___E_matcher_21_22(53) => __E___E_matcher_21_22(53) m_space(52).
	p(NT(53), (NT(53)+NT(52)));
//G58:  __E_matcher_21(51)   => __E___E_matcher_21_22(53) m_unit(50).
	p(NT(51), (NT(53)+NT(50)));
//G59:  __E_matcher_23(54)   => null.
	p(NT(54), (nul));
//G60:  __E_matcher_23(54)   => __E_matcher_23(54) __E_matcher_21(51).
	p(NT(54), (NT(54)+NT(51)));
//G61:  matcher(19)          => m_unit(50) __E_matcher_23(54).
	p(NT(19), (NT(50)+NT(54)));
//G62:  m_unit(50)           => quoted_string(48).
	p(NT(50), (NT(48)));
//G63:  m_unit(50)           => char_lit(55).
	p(NT(50), (NT(55)));
//G64:  m_unit(50)           => m_char(56).
	p(NT(50), (NT(56)));
//G65:  __E_m_char_24(57)    => ':'.
	p(NT(57), (T(1)));
//G66:  __E_m_char_24(57)    => '{'.
	p(NT(57), (T(3)));
//G67:  __E_m_char_24(57)    => '}'.
	p(NT(57), (T(4)));
//G68:  __E_m_char_24(57)    => '.'.
	p(NT(57), (T(5)));
//G69:  __E_m_char_24(57)    => ','.
	p(NT(57), (T(6)));
//G70:  __E_m_char_24(57)    => '\''.
	p(NT(57), (T(22)));
//G71:  __E_m_char_24(57)    => '"'.
	p(NT(57), (T(23)));
//G72:  __E_m_char_24(57)    => '@'.
	p(NT(57), (T(7)));
//G73:  __E_m_char_24(57)    => '#'.
	p(NT(57), (T(24)));
//G74:  __E_m_char_24(57)    => '~'.
	p(NT(57), (T(2)));
//G75:  __N_0(85)            => __E_m_char_24(57).
	p(NT(85), (NT(57)));
//G76:  m_char(56)           => graph(5) & ~( __N_0(85) ).	 # conjunctive
	p(NT(56), (NT(5)) & ~(NT(85)));
//G77:  m_space(52)          => ' '.
	p(NT(52), (T(25)));
//G78:  m_space(52)          => '\t'.
	p(NT(52), (T(26)));
//G79:  __E_name_25(58)      => alpha(2).
	p(NT(58), (NT(2)));
//G80:  __E_name_25(58)      => '_'.
	p(NT(58), (T(27)));
//G81:  __E_name_26(59)      => alnum(3).
	p(NT(59), (NT(3)));
//G82:  __E_name_26(59)      => '_'.
	p(NT(59), (T(27)));
//G83:  __E_name_27(60)      => null.
	p(NT(60), (nul));
//G84:  __E_name_27(60)      => __E_name_27(60) __E_name_26(59).
	p(NT(60), (NT(60)+NT(59)));
//G85:  name(49)             => __E_name_25(58) __E_name_27(60).
	p(NT(49), (NT(58)+NT(60)));
//G86:  char_lit(55)         => '\'' char_lit_char(61) '\''.
	p(NT(55), (T(22)+NT(61)+T(22)));
//G87:  char_lit_char(61)    => unescaped_c(62).
	p(NT(61), (NT(62)));
//G88:  char_lit_char(61)    => escaped_c(63).
	p(NT(61), (NT(63)));
//G89:  __E_unescaped_c_28(64) => space(4).
	p(NT(64), (NT(4)));
//G90:  __E_unescaped_c_28(64) => printable(6).
	p(NT(64), (NT(6)));
//G91:  __E_unescaped_c_29(65) => '\''.
	p(NT(65), (T(22)));
//G92:  __E_unescaped_c_29(65) => '\\'.
	p(NT(65), (T(28)));
//G93:  __N_1(86)            => __E_unescaped_c_29(65).
	p(NT(86), (NT(65)));
//G94:  unescaped_c(62)      => __E_unescaped_c_28(64) & ~( __N_1(86) ).	 # conjunctive
	p(NT(62), (NT(64)) & ~(NT(86)));
//G95:  __E_escaped_c_30(66) => '\''.
	p(NT(66), (T(22)));
//G96:  __E_escaped_c_30(66) => escape_char(67).
	p(NT(66), (NT(67)));
//G97:  escaped_c(63)        => '\\' __E_escaped_c_30(66).
	p(NT(63), (T(28)+NT(66)));
//G98:  __E_quoted_string_31(69) => null.
	p(NT(69), (nul));
//G99:  __E_quoted_string_31(69) => __E_quoted_string_31(69) quoted_string_char(68).
	p(NT(69), (NT(69)+NT(68)));
//G100: quoted_string(48)    => '"' __E_quoted_string_31(69) '"'.
	p(NT(48), (T(23)+NT(69)+T(23)));
//G101: quoted_string_char(68) => unescaped_s(70).
	p(NT(68), (NT(70)));
//G102: quoted_string_char(68) => escaped_s(71).
	p(NT(68), (NT(71)));
//G103: __E_unescaped_s_32(72) => space(4).
	p(NT(72), (NT(4)));
//G104: __E_unescaped_s_32(72) => printable(6).
	p(NT(72), (NT(6)));
//G105: __E_unescaped_s_33(73) => '"'.
	p(NT(73), (T(23)));
//G106: __E_unescaped_s_33(73) => '\\'.
	p(NT(73), (T(28)));
//G107: __N_2(87)            => __E_unescaped_s_33(73).
	p(NT(87), (NT(73)));
//G108: unescaped_s(70)      => __E_unescaped_s_32(72) & ~( __N_2(87) ).	 # conjunctive
	p(NT(70), (NT(72)) & ~(NT(87)));
//G109: __E_escaped_s_34(74) => '"'.
	p(NT(74), (T(23)));
//G110: __E_escaped_s_34(74) => escape_char(67).
	p(NT(74), (NT(67)));
//G111: escaped_s(71)        => '\\' __E_escaped_s_34(74).
	p(NT(71), (T(28)+NT(74)));
//G112: escape_char(67)      => 'a'.
	p(NT(67), (T(18)));
//G113: escape_char(67)      => 'b'.
	p(NT(67), (T(11)));
//G114: escape_char(67)      => 'f'.
	p(NT(67), (T(8)));
//G115: escape_char(67)      => 'n'.
	p(NT(67), (T(15)));
//G116: escape_char(67)      => 'r'.
	p(NT(67), (T(10)));
//G117: escape_char(67)      => 't'.
	p(NT(67), (T(29)));
//G118: escape_char(67)      => 'v'.
	p(NT(67), (T(30)));
//G119: escape_char(67)      => '\\'.
	p(NT(67), (T(28)));
//G120: escape_char(67)      => '/'.
	p(NT(67), (T(31)));
//G121: escape_char(67)      => esc_hex(75).
	p(NT(67), (NT(75)));
//G122: escape_char(67)      => esc_u4(76).
	p(NT(67), (NT(76)));
//G123: escape_char(67)      => esc_U8(77).
	p(NT(67), (NT(77)));
//G124: __E_esc_hex_35(78)   => 'x'.
	p(NT(78), (T(32)));
//G125: __E_esc_hex_35(78)   => 'X'.
	p(NT(78), (T(33)));
//G126: esc_hex(75)          => __E_esc_hex_35(78) xdigit(7) xdigit(7).
	p(NT(75), (NT(78)+NT(7)+NT(7)));
//G127: esc_u4(76)           => 'u' xdigit(7) xdigit(7) xdigit(7) xdigit(7).
	p(NT(76), (T(14)+NT(7)+NT(7)+NT(7)+NT(7)));
//G128: esc_U8(77)           => 'U' xdigit(7) xdigit(7) xdigit(7) xdigit(7) xdigit(7) xdigit(7) xdigit(7) xdigit(7).
	p(NT(77), (T(34)+NT(7)+NT(7)+NT(7)+NT(7)+NT(7)+NT(7)+NT(7)+NT(7)));
//G129: __E___36(79)         => __(17).
	p(NT(79), (NT(17)));
//G130: __E___36(79)         => null.
	p(NT(79), (nul));
//G131: _(9)                 => __E___36(79).
	p(NT(9), (NT(79)));
//G132: __E____37(80)        => space(4).
	p(NT(80), (NT(4)));
//G133: __E____37(80)        => comment(81).
	p(NT(80), (NT(81)));
//G134: __(17)               => __E____37(80) _(9).
	p(NT(17), (NT(80)+NT(9)));
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
#else
idni::prods<char_type, terminal_type>& productions();
#endif

inline ::idni::grammar<char_type, terminal_type>& grammar() {
	static ::idni::grammar<char_type, terminal_type> v(nts(),
		productions(), start_symbol(), char_classes(),
		grammar_options());
	return v;
}

} // namespace tgf_test_parser_data

struct tgf_test_parser_nonterminals {
	enum nonterminal {
		nul, eof, alpha, alnum, space, graph, printable, xdigit, start, _, 
		__E_start_0, entry, __E_start_1, negate, __E_entry_2, __E_entry_3, words, __, __E_entry_4, matcher, 
		entry_tail, item_list, nested_block, __E_entry_tail_5, entry_terminator, __E_entry_tail_6, __E_entry_tail_7, __E_entry_tail_8, __E_nested_block_9, __E_nested_block_10, 
		item, __E_item_list_11, __E_item_list_12, __E_item_13, input, __E_item_14, __E_item_15, __E_item_16, __E_item_17, word, 
		__E_words_18, __E_words_19, __E_word_20, w_forbid, w_unique, w_all, w_any, w_raw, quoted_string, name, 
		m_unit, __E_matcher_21, m_space, __E___E_matcher_21_22, __E_matcher_23, char_lit, m_char, __E_m_char_24, __E_name_25, __E_name_26, 
		__E_name_27, char_lit_char, unescaped_c, escaped_c, __E_unescaped_c_28, __E_unescaped_c_29, __E_escaped_c_30, escape_char, quoted_string_char, __E_quoted_string_31, 
		unescaped_s, escaped_s, __E_unescaped_s_32, __E_unescaped_s_33, __E_escaped_s_34, esc_hex, esc_u4, esc_U8, __E_esc_hex_35, __E___36, 
		__E____37, comment, __E_comment_38, __E_comment_39, __E_comment_40, __N_0, __N_1, __N_2, 
	};
};

#ifdef _MSC_VER
// the base parser puts its chart indexes on a cache line, so the padding is wanted
#	pragma warning(push)
#	pragma warning(disable : 4324)
#endif
struct tgf_test_parser : public idni::parser<char, char32_t>, public tgf_test_parser_nonterminals {
	static tgf_test_parser& instance() {
		static tgf_test_parser inst;
		return inst;
	}
	tgf_test_parser() : idni::parser<char_type, terminal_type>(
		tgf_test_parser_data::grammar(),
		tgf_test_parser_data::make_parser_options()) {}
	size_t id(const std::basic_string<char_type>& name) {
		return tgf_test_parser_data::nts().get(name);
	}
	const std::basic_string<char_type>& name(size_t id) {
		return tgf_test_parser_data::nts().get(id);
	}
	symbol_type literal(const nonterminal& nt) {
		return symbol_type(nt, &tgf_test_parser_data::nts());
	}
};
#ifdef _MSC_VER
#	pragma warning(pop)
#endif

#endif // __TGF_TEST_PARSER_H__
