// This file is generated from a file src/format/treemr/treemr.tgf by
//       https://github.com/IDNI/parser/src/tgf
//
#ifndef __TREEMR_PARSER_H__
#define __TREEMR_PARSER_H__

#include "parser.h"
#include "recoders.h"

namespace treemr_parser_data {

using char_type     = char;
using terminal_type = char32_t;

inline static constexpr size_t nt_bits = 7;
inline const std::vector<std::string> symbol_names{
	"", "eof", "alpha", "alnum", "space", "printable", "xdigit", "__", "_", "name", 
	"unescaped_s", "escaped_s", "unescaped_c", "escaped_c", "escape_char", "esc_hex", "esc_u4", "esc_U8", "pattern", "sib_seq", 
	"atom", "terminal", "quantifier", "edge", "start", "__E_pattern_0", "root_anchor", "slot_chain", "simple_slots", "__E_slot_chain_1", 
	"edge_slot", "simple_slot", "__E_simple_slots_2", "__E_simple_slots_3", "left_anchor", "right_anchor", "__E_simple_slot_4", "leaf_anchor", "__E_simple_slot_5", "star", 
	"plus", "opt", "__E_edge_slot_6", "__E_edge_slot_7", "wildcard", "capture", "alt_seq", "__E_alt_seq_8", "__E_alt_seq_9", "descendant_edge", 
	"direct_edge", "__E_name_10", "__E_name_11", "__E_name_12", "char_lit", "str_lit", "char_lit_char", "__E_unescaped_c_13", "__E_unescaped_c_14", "__E_escaped_c_15", 
	"str_lit_char", "__E_str_lit_16", "__E_unescaped_s_17", "__E_unescaped_s_18", "__E_escaped_s_19", "__E_esc_hex_20", "__E___21", "__E____22", "comment", "__E_comment_23", 
	"__E_comment_24", "__E_comment_25", "__N_0", "__N_1", 
};

inline ::idni::nonterminals<char_type, terminal_type> nts{symbol_names};

inline std::vector<terminal_type> terminals{
	U'\0', U'/', U'^', U'$', U'!', U'*', U'+', U'?', U'%', 
	U'(', U')', U'|', U'>', U'_', U'\'', U'\\', U'"', U'a', U'b', 
	U'f', U'n', U'r', U't', U'v', U'x', U'X', U'u', U'U', U'\t', 
	U'\r', U'\n', U'#', 
};

inline ::idni::char_class_fns<terminal_type> char_classes =
	::idni::predefined_char_classes<char_type, terminal_type>({
		"eof",
		"alpha",
		"alnum",
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
		.to_inline = {
			{ 18 },
			{ 19 },
			{ 20 },
			{ 21 },
			{ 22 },
			{ 23 }
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

inline ::idni::prods<char_type, terminal_type> start_symbol{ nts(24) };

#ifdef TAU_PARSER_BUILD_HEADER_ONLY
inline idni::prods<char_type, terminal_type>& productions() {
	static bool loaded = false;
	static idni::prods<char_type, terminal_type>
		p, nul(idni::lit<char_type, terminal_type>{});
	if (loaded) return p;
	#define  T(x) (idni::prods<char_type, terminal_type>{ terminals[x] })
	#define NT(x) (idni::prods<char_type, terminal_type>{ nts(x) })
//G0:   start(24)            => _(8) pattern(18) _(8).
	p(NT(24), (NT(8)+NT(18)+NT(8)));
//G1:   __E_pattern_0(25)    => root_anchor(26) _(8).
	p(NT(25), (NT(26)+NT(8)));
//G2:   __E_pattern_0(25)    => null.
	p(NT(25), (nul));
//G3:   pattern(18)          => __E_pattern_0(25) sib_seq(19).
	p(NT(18), (NT(25)+NT(19)));
//G4:   root_anchor(26)      => '/'.
	p(NT(26), (T(1)));
//G5:   sib_seq(19)          => slot_chain(27).
	p(NT(19), (NT(27)));
//G6:   __E_slot_chain_1(29) => __(7) edge_slot(30).
	p(NT(29), (NT(7)+NT(30)));
//G7:   __E_slot_chain_1(29) => null.
	p(NT(29), (nul));
//G8:   slot_chain(27)       => simple_slots(28) __E_slot_chain_1(29).
	p(NT(27), (NT(28)+NT(29)));
//G9:   slot_chain(27)       => edge_slot(30).
	p(NT(27), (NT(30)));
//G10:  __E_simple_slots_2(32) => __(7) simple_slot(31).
	p(NT(32), (NT(7)+NT(31)));
//G11:  __E_simple_slots_3(33) => null.
	p(NT(33), (nul));
//G12:  __E_simple_slots_3(33) => __E_simple_slots_3(33) __E_simple_slots_2(32).
	p(NT(33), (NT(33)+NT(32)));
//G13:  simple_slots(28)     => simple_slot(31) __E_simple_slots_3(33).
	p(NT(28), (NT(31)+NT(33)));
//G14:  simple_slot(31)      => left_anchor(34).
	p(NT(31), (NT(34)));
//G15:  simple_slot(31)      => right_anchor(35).
	p(NT(31), (NT(35)));
//G16:  __E_simple_slot_4(36) => _(8) leaf_anchor(37).
	p(NT(36), (NT(8)+NT(37)));
//G17:  __E_simple_slot_4(36) => null.
	p(NT(36), (nul));
//G18:  __E_simple_slot_5(38) => _(8) quantifier(22).
	p(NT(38), (NT(8)+NT(22)));
//G19:  __E_simple_slot_5(38) => null.
	p(NT(38), (nul));
//G20:  simple_slot(31)      => atom(20) __E_simple_slot_4(36) __E_simple_slot_5(38).
	p(NT(31), (NT(20)+NT(36)+NT(38)));
//G21:  left_anchor(34)      => '^'.
	p(NT(34), (T(2)));
//G22:  right_anchor(35)     => '$'.
	p(NT(35), (T(3)));
//G23:  leaf_anchor(37)      => '!'.
	p(NT(37), (T(4)));
//G24:  quantifier(22)       => star(39).
	p(NT(22), (NT(39)));
//G25:  quantifier(22)       => plus(40).
	p(NT(22), (NT(40)));
//G26:  quantifier(22)       => opt(41).
	p(NT(22), (NT(41)));
//G27:  star(39)             => '*'.
	p(NT(39), (T(5)));
//G28:  plus(40)             => '+'.
	p(NT(40), (T(6)));
//G29:  opt(41)              => '?'.
	p(NT(41), (T(7)));
//G30:  __E_edge_slot_6(42)  => _(8) leaf_anchor(37).
	p(NT(42), (NT(8)+NT(37)));
//G31:  __E_edge_slot_6(42)  => null.
	p(NT(42), (nul));
//G32:  __E_edge_slot_7(43)  => _(8) quantifier(22).
	p(NT(43), (NT(8)+NT(22)));
//G33:  __E_edge_slot_7(43)  => null.
	p(NT(43), (nul));
//G34:  edge_slot(30)        => atom(20) __E_edge_slot_6(42) __E_edge_slot_7(43) _(8) edge(23) _(8) sib_seq(19).
	p(NT(30), (NT(20)+NT(42)+NT(43)+NT(8)+NT(23)+NT(8)+NT(19)));
//G35:  atom(20)             => name(9).
	p(NT(20), (NT(9)));
//G36:  atom(20)             => wildcard(44).
	p(NT(20), (NT(44)));
//G37:  atom(20)             => terminal(21).
	p(NT(20), (NT(21)));
//G38:  atom(20)             => capture(45).
	p(NT(20), (NT(45)));
//G39:  wildcard(44)         => '%'.
	p(NT(44), (T(8)));
//G40:  capture(45)          => '(' _(8) alt_seq(46) _(8) ')'.
	p(NT(45), (T(9)+NT(8)+NT(46)+NT(8)+T(10)));
//G41:  __E_alt_seq_8(47)    => _(8) '|' _(8) sib_seq(19).
	p(NT(47), (NT(8)+T(11)+NT(8)+NT(19)));
//G42:  __E_alt_seq_9(48)    => null.
	p(NT(48), (nul));
//G43:  __E_alt_seq_9(48)    => __E_alt_seq_9(48) __E_alt_seq_8(47).
	p(NT(48), (NT(48)+NT(47)));
//G44:  alt_seq(46)          => sib_seq(19) __E_alt_seq_9(48).
	p(NT(46), (NT(19)+NT(48)));
//G45:  edge(23)             => descendant_edge(49).
	p(NT(23), (NT(49)));
//G46:  edge(23)             => direct_edge(50).
	p(NT(23), (NT(50)));
//G47:  descendant_edge(49)  => '>' '>'.
	p(NT(49), (T(12)+T(12)));
//G48:  direct_edge(50)      => '>'.
	p(NT(50), (T(12)));
//G49:  __E_name_10(51)      => alpha(2).
	p(NT(51), (NT(2)));
//G50:  __E_name_10(51)      => '_'.
	p(NT(51), (T(13)));
//G51:  __E_name_11(52)      => alnum(3).
	p(NT(52), (NT(3)));
//G52:  __E_name_11(52)      => '_'.
	p(NT(52), (T(13)));
//G53:  __E_name_12(53)      => null.
	p(NT(53), (nul));
//G54:  __E_name_12(53)      => __E_name_12(53) __E_name_11(52).
	p(NT(53), (NT(53)+NT(52)));
//G55:  name(9)              => __E_name_10(51) __E_name_12(53).
	p(NT(9), (NT(51)+NT(53)));
//G56:  terminal(21)         => char_lit(54).
	p(NT(21), (NT(54)));
//G57:  terminal(21)         => str_lit(55).
	p(NT(21), (NT(55)));
//G58:  char_lit(54)         => '\'' char_lit_char(56) '\''.
	p(NT(54), (T(14)+NT(56)+T(14)));
//G59:  char_lit_char(56)    => unescaped_c(12).
	p(NT(56), (NT(12)));
//G60:  char_lit_char(56)    => escaped_c(13).
	p(NT(56), (NT(13)));
//G61:  __E_unescaped_c_13(57) => space(4).
	p(NT(57), (NT(4)));
//G62:  __E_unescaped_c_13(57) => printable(5).
	p(NT(57), (NT(5)));
//G63:  __E_unescaped_c_14(58) => '\''.
	p(NT(58), (T(14)));
//G64:  __E_unescaped_c_14(58) => '\\'.
	p(NT(58), (T(15)));
//G65:  __N_0(72)            => __E_unescaped_c_14(58).
	p(NT(72), (NT(58)));
//G66:  unescaped_c(12)      => __E_unescaped_c_13(57) & ~( __N_0(72) ).	 # conjunctive
	p(NT(12), (NT(57)) & ~(NT(72)));
//G67:  __E_escaped_c_15(59) => '\''.
	p(NT(59), (T(14)));
//G68:  __E_escaped_c_15(59) => escape_char(14).
	p(NT(59), (NT(14)));
//G69:  escaped_c(13)        => '\\' __E_escaped_c_15(59).
	p(NT(13), (T(15)+NT(59)));
//G70:  __E_str_lit_16(61)   => null.
	p(NT(61), (nul));
//G71:  __E_str_lit_16(61)   => __E_str_lit_16(61) str_lit_char(60).
	p(NT(61), (NT(61)+NT(60)));
//G72:  str_lit(55)          => '"' __E_str_lit_16(61) '"'.
	p(NT(55), (T(16)+NT(61)+T(16)));
//G73:  str_lit_char(60)     => unescaped_s(10).
	p(NT(60), (NT(10)));
//G74:  str_lit_char(60)     => escaped_s(11).
	p(NT(60), (NT(11)));
//G75:  __E_unescaped_s_17(62) => space(4).
	p(NT(62), (NT(4)));
//G76:  __E_unescaped_s_17(62) => printable(5).
	p(NT(62), (NT(5)));
//G77:  __E_unescaped_s_18(63) => '"'.
	p(NT(63), (T(16)));
//G78:  __E_unescaped_s_18(63) => '\\'.
	p(NT(63), (T(15)));
//G79:  __N_1(73)            => __E_unescaped_s_18(63).
	p(NT(73), (NT(63)));
//G80:  unescaped_s(10)      => __E_unescaped_s_17(62) & ~( __N_1(73) ).	 # conjunctive
	p(NT(10), (NT(62)) & ~(NT(73)));
//G81:  __E_escaped_s_19(64) => '"'.
	p(NT(64), (T(16)));
//G82:  __E_escaped_s_19(64) => escape_char(14).
	p(NT(64), (NT(14)));
//G83:  escaped_s(11)        => '\\' __E_escaped_s_19(64).
	p(NT(11), (T(15)+NT(64)));
//G84:  escape_char(14)      => 'a'.
	p(NT(14), (T(17)));
//G85:  escape_char(14)      => 'b'.
	p(NT(14), (T(18)));
//G86:  escape_char(14)      => 'f'.
	p(NT(14), (T(19)));
//G87:  escape_char(14)      => 'n'.
	p(NT(14), (T(20)));
//G88:  escape_char(14)      => 'r'.
	p(NT(14), (T(21)));
//G89:  escape_char(14)      => 't'.
	p(NT(14), (T(22)));
//G90:  escape_char(14)      => 'v'.
	p(NT(14), (T(23)));
//G91:  escape_char(14)      => '\\'.
	p(NT(14), (T(15)));
//G92:  escape_char(14)      => '/'.
	p(NT(14), (T(1)));
//G93:  escape_char(14)      => esc_hex(15).
	p(NT(14), (NT(15)));
//G94:  escape_char(14)      => esc_u4(16).
	p(NT(14), (NT(16)));
//G95:  escape_char(14)      => esc_U8(17).
	p(NT(14), (NT(17)));
//G96:  __E_esc_hex_20(65)   => 'x'.
	p(NT(65), (T(24)));
//G97:  __E_esc_hex_20(65)   => 'X'.
	p(NT(65), (T(25)));
//G98:  esc_hex(15)          => __E_esc_hex_20(65) xdigit(6) xdigit(6).
	p(NT(15), (NT(65)+NT(6)+NT(6)));
//G99:  esc_u4(16)           => 'u' xdigit(6) xdigit(6) xdigit(6) xdigit(6).
	p(NT(16), (T(26)+NT(6)+NT(6)+NT(6)+NT(6)));
//G100: esc_U8(17)           => 'U' xdigit(6) xdigit(6) xdigit(6) xdigit(6) xdigit(6) xdigit(6) xdigit(6) xdigit(6).
	p(NT(17), (T(27)+NT(6)+NT(6)+NT(6)+NT(6)+NT(6)+NT(6)+NT(6)+NT(6)));
//G101: __E___21(66)         => __(7).
	p(NT(66), (NT(7)));
//G102: __E___21(66)         => null.
	p(NT(66), (nul));
//G103: _(8)                 => __E___21(66).
	p(NT(8), (NT(66)));
//G104: __E____22(67)        => space(4).
	p(NT(67), (NT(4)));
//G105: __E____22(67)        => comment(68).
	p(NT(67), (NT(68)));
//G106: __(7)                => __E____22(67) _(8).
	p(NT(7), (NT(67)+NT(8)));
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
#else
idni::prods<char_type, terminal_type>& productions();
#endif

inline ::idni::grammar<char_type, terminal_type> grammar(
	nts, productions(), start_symbol, char_classes, grammar_options);

} // namespace treemr_parser_data

struct treemr_parser_nonterminals {
	enum nonterminal {
		nul, eof, alpha, alnum, space, printable, xdigit, __, _, name, 
		unescaped_s, escaped_s, unescaped_c, escaped_c, escape_char, esc_hex, esc_u4, esc_U8, pattern, sib_seq, 
		atom, terminal, quantifier, edge, start, __E_pattern_0, root_anchor, slot_chain, simple_slots, __E_slot_chain_1, 
		edge_slot, simple_slot, __E_simple_slots_2, __E_simple_slots_3, left_anchor, right_anchor, __E_simple_slot_4, leaf_anchor, __E_simple_slot_5, star, 
		plus, opt, __E_edge_slot_6, __E_edge_slot_7, wildcard, capture, alt_seq, __E_alt_seq_8, __E_alt_seq_9, descendant_edge, 
		direct_edge, __E_name_10, __E_name_11, __E_name_12, char_lit, str_lit, char_lit_char, __E_unescaped_c_13, __E_unescaped_c_14, __E_escaped_c_15, 
		str_lit_char, __E_str_lit_16, __E_unescaped_s_17, __E_unescaped_s_18, __E_escaped_s_19, __E_esc_hex_20, __E___21, __E____22, comment, __E_comment_23, 
		__E_comment_24, __E_comment_25, __N_0, __N_1, 
	};
};

struct treemr_parser : public idni::parser<char, char32_t>, public treemr_parser_nonterminals {
	static treemr_parser& instance() {
		static treemr_parser inst;
		return inst;
	}
	treemr_parser() : idni::parser<char_type, terminal_type>(
		treemr_parser_data::grammar,
		treemr_parser_data::make_parser_options()) {}
	size_t id(const std::basic_string<char_type>& name) {
		return treemr_parser_data::nts.get(name);
	}
	const std::basic_string<char_type>& name(size_t id) {
		return treemr_parser_data::nts.get(id);
	}
	symbol_type literal(const nonterminal& nt) {
		return symbol_type(nt, &treemr_parser_data::nts);
	}
};

#endif // __TREEMR_PARSER_H__
