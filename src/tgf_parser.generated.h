// This file is generated from a file src/tgf.tgf by
//       https://github.com/IDNI/parser/tools/tgf
//
#ifndef __TGF_PARSER_H__
#define __TGF_PARSER_H__

#include <string.h>

#include "parser.h"

struct tgf_parser {
	using char_type       = char;
	using terminal_type   = char;
	using traits_type     = std::char_traits<char_type>;
	using int_type        = typename traits_type::int_type;
	using grammar_type    = idni::grammar<char_type, terminal_type>;
	using grammar_options = grammar_type::options;
	using symbol_type     = idni::lit<char_type, terminal_type>;
	using location_type   = std::array<size_t, 2>;
	using node_type       = std::pair<symbol_type, location_type>;
	using parser_type     = idni::parser<char_type, terminal_type>;
	using options         = parser_type::options;
	using parse_options   = parser_type::parse_options;
	using forest_type     = parser_type::pforest;
	using input_type      = parser_type::input;
	using decoder_type    = parser_type::input::decoder_type;
	using encoder_type    = std::function<std::basic_string<char_type>(
			const std::vector<terminal_type>&)>;
	tgf_parser() :
		nts(load_nonterminals()), cc(load_cc()),
		g(nts, load_prods(), nt(6), cc, load_grammar_opts()),
		p(g, load_opts()) {}
	std::unique_ptr<forest_type> parse(const char_type* data, size_t size,
		parse_options po = {}) { return p.parse(data, size, po); }
	std::unique_ptr<forest_type> parse(std::basic_istream<char_type>& is,
		parse_options po = {}) { return p.parse(is, po); }
	std::unique_ptr<forest_type> parse(const std::string& fn,
		parse_options po = {}) { return p.parse(fn, po); }
#ifndef WIN32
	std::unique_ptr<forest_type> parse(int fd, parse_options po = {})
		{ return p.parse(fd, po); }
#endif //WIN32
	bool found(size_t start = SIZE_MAX) { return p.found(start); }
	typename parser_type::error get_error() { return p.get_error(); }
	enum nonterminal {
		nul, eof, alnum, alpha, space, printable, start, __E_start_0, _, statement, 
		__E_start_1, directive, production, start_statement, __E___2, __, __E____3, comment, __E_comment_4, __E_comment_5, 
		__E_comment_6, sep, __E_sep_7, sep_required, terminal_char, __E_terminal_char_8, unescaped_c, escaped_c, __E_unescaped_c_9, __E_escaped_c_10, 
		terminal_string, terminal_string_char, __E_terminal_string_11, unescaped_s, escaped_s, __E_unescaped_s_12, __E_escaped_s_13, terminal, sym, __E_sym_14, 
		__E_sym_15, __E_sym_16, alternation, conjunction, __E_alternation_17, __E_alternation_18, concatenation, __E_conjunction_19, __E_conjunction_20, factor, 
		__E_concatenation_21, __E_concatenation_22, optional, repeat, none_or_repeat, neg, term, group, optional_group, repeat_group, 
		directive_body, start_dir, inline_dir, trim_children_dir, trim_terminals, trim_dir, use_dir, boolean_dir, nodisambig_dir, __E_trim_dir_23, 
		__E_trim_dir_24, trim_terminals_dir, trim_arg, __E_trim_children_dir_25, __E_trim_children_dir_26, all_terminals_sym, inline_arg, __E_inline_dir_27, __E_inline_dir_28, char_classes_sym, 
		__E_char_classes_sym_29, use_from, use_param, __E_use_dir_30, __E_use_dir_31, __E_use_dir_32, __E_use_dir_33, char_class_name, __E_boolean_dir_34, boolean_action, 
		boolean_dir_name, autodisambig_sym, enable_sym, disable_sym, __E_autodisambig_sym_35, disambig_sym, __E_disambig_sym_36, __E___E_disambig_sym_36_37, __E___E___E_disambig_sym_36_37_38, nodisambig_sym, 
		__E_nodisambig_dir_39, __E_nodisambig_dir_40, __E_nodisambig_sym_41, __E___E_nodisambig_sym_41_42, __E___E_nodisambig_sym_41_43, __E_nodisambig_sym_44, __N_0, __N_1, 
	};
	size_t id(const std::basic_string<char_type>& name) {
		return nts.get(name);
	}
	const std::basic_string<char_type>& name(size_t id) {
		return nts.get(id);
	}
	symbol_type literal(const nonterminal& nt) {
		return symbol_type(nt, &nts);
	}
private:
	std::vector<terminal_type> ts{
		'\0', '\t', '\r', '\n', '#', '-', '_', '\'', '\\', 
		'/', 'b', 'f', 'n', 'r', 't', '"', '=', '>', '.', 
		'|', '&', '~', '?', '+', '*', '(', ')', '[', ']', 
		'{', '}', '@', 's', 'a', ',', 'i', 'm', 'l', 'e', 
		'c', 'h', 'd', 'u', 'o', 'p', 'k', 'g', 'w', 'x', 
	};
	idni::nonterminals<char_type, terminal_type> nts{};
	idni::char_class_fns<terminal_type> cc;
	idni::grammar<char_type, terminal_type> g;
	parser_type p;
	idni::prods<char_type, terminal_type> t(size_t tid) {
		return idni::prods<char_type, terminal_type>(ts[tid]);
	}
	idni::prods<char_type, terminal_type> nt(size_t ntid) {
		return idni::prods<char_type, terminal_type>(
			symbol_type(ntid, &nts));
	}
	idni::nonterminals<char_type, terminal_type> load_nonterminals() const {
		idni::nonterminals<char_type, terminal_type> nts{};
		for (const auto& nt : {
			"", "eof", "alnum", "alpha", "space", "printable", "start", "__E_start_0", "_", "statement", 
			"__E_start_1", "directive", "production", "start_statement", "__E___2", "__", "__E____3", "comment", "__E_comment_4", "__E_comment_5", 
			"__E_comment_6", "sep", "__E_sep_7", "sep_required", "terminal_char", "__E_terminal_char_8", "unescaped_c", "escaped_c", "__E_unescaped_c_9", "__E_escaped_c_10", 
			"terminal_string", "terminal_string_char", "__E_terminal_string_11", "unescaped_s", "escaped_s", "__E_unescaped_s_12", "__E_escaped_s_13", "terminal", "sym", "__E_sym_14", 
			"__E_sym_15", "__E_sym_16", "alternation", "conjunction", "__E_alternation_17", "__E_alternation_18", "concatenation", "__E_conjunction_19", "__E_conjunction_20", "factor", 
			"__E_concatenation_21", "__E_concatenation_22", "optional", "repeat", "none_or_repeat", "neg", "term", "group", "optional_group", "repeat_group", 
			"directive_body", "start_dir", "inline_dir", "trim_children_dir", "trim_terminals", "trim_dir", "use_dir", "boolean_dir", "nodisambig_dir", "__E_trim_dir_23", 
			"__E_trim_dir_24", "trim_terminals_dir", "trim_arg", "__E_trim_children_dir_25", "__E_trim_children_dir_26", "all_terminals_sym", "inline_arg", "__E_inline_dir_27", "__E_inline_dir_28", "char_classes_sym", 
			"__E_char_classes_sym_29", "use_from", "use_param", "__E_use_dir_30", "__E_use_dir_31", "__E_use_dir_32", "__E_use_dir_33", "char_class_name", "__E_boolean_dir_34", "boolean_action", 
			"boolean_dir_name", "autodisambig_sym", "enable_sym", "disable_sym", "__E_autodisambig_sym_35", "disambig_sym", "__E_disambig_sym_36", "__E___E_disambig_sym_36_37", "__E___E___E_disambig_sym_36_37_38", "nodisambig_sym", 
			"__E_nodisambig_dir_39", "__E_nodisambig_dir_40", "__E_nodisambig_sym_41", "__E___E_nodisambig_sym_41_42", "__E___E_nodisambig_sym_41_43", "__E_nodisambig_sym_44", "__N_0", "__N_1", 
		}) nts.get(nt);
		return nts;
	}
	idni::char_class_fns<terminal_type> load_cc() {
		return idni::predefined_char_classes<char_type, terminal_type>({
			"eof",
			"alnum",
			"alpha",
			"space",
			"printable",
		}, nts);
	}
	grammar_type::options load_grammar_opts() {
		grammar_type::options o;
		o.auto_disambiguate = true;
		return o;
	}
	options load_opts() {
		options o;
		return o;
	}
	idni::prods<char_type, terminal_type> load_prods() {
		idni::prods<char_type, terminal_type> q,
			nul(symbol_type{});
		//       __E_start_0(7)       => _(8) statement(9).
		q(nt(7), (nt(8)+nt(9)));
		//       __E_start_1(10)      => null.
		q(nt(10), (nul));
		//       __E_start_1(10)      => __E_start_0(7).
		q(nt(10), (nt(7)));
		//       __E_start_1(10)      => __E_start_0(7) __E_start_1(10).
		q(nt(10), (nt(7)+nt(10)));
		//       start(6)             => __E_start_1(10) _(8).
		q(nt(6), (nt(10)+nt(8)));
		//       statement(9)         => directive(11).
		q(nt(9), (nt(11)));
		//       statement(9)         => production(12).
		q(nt(9), (nt(12)));
		//       start_statement(13)  => _(8) statement(9) _(8).
		q(nt(13), (nt(8)+nt(9)+nt(8)));
		//       __E___2(14)          => __(15).
		q(nt(14), (nt(15)));
		//       __E___2(14)          => null.
		q(nt(14), (nul));
		//       _(8)                 => __E___2(14).
		q(nt(8), (nt(14)));
		//       __E____3(16)         => space(4).
		q(nt(16), (nt(4)));
		//       __E____3(16)         => comment(17).
		q(nt(16), (nt(17)));
		//       __(15)               => __E____3(16) _(8).
		q(nt(15), (nt(16)+nt(8)));
		//       __E_comment_4(18)    => printable(5).
		q(nt(18), (nt(5)));
		//       __E_comment_4(18)    => '\t'.
		q(nt(18), (t(1)));
		//       __E_comment_5(19)    => null.
		q(nt(19), (nul));
		//       __E_comment_5(19)    => __E_comment_4(18).
		q(nt(19), (nt(18)));
		//       __E_comment_5(19)    => __E_comment_4(18) __E_comment_5(19).
		q(nt(19), (nt(18)+nt(19)));
		//       __E_comment_6(20)    => '\r'.
		q(nt(20), (t(2)));
		//       __E_comment_6(20)    => '\n'.
		q(nt(20), (t(3)));
		//       __E_comment_6(20)    => eof(1).
		q(nt(20), (nt(1)));
		//       comment(17)          => '#' __E_comment_5(19) __E_comment_6(20).
		q(nt(17), (t(4)+nt(19)+nt(20)));
		//       __E_sep_7(22)        => sep_required(23).
		q(nt(22), (nt(23)));
		//       __E_sep_7(22)        => null.
		q(nt(22), (nul));
		//       sep(21)              => __E_sep_7(22).
		q(nt(21), (nt(22)));
		//       sep_required(23)     => '-'.
		q(nt(23), (t(5)));
		//       sep_required(23)     => '_'.
		q(nt(23), (t(6)));
		//       sep_required(23)     => __(15).
		q(nt(23), (nt(15)));
		//       __E_terminal_char_8(25) => unescaped_c(26).
		q(nt(25), (nt(26)));
		//       __E_terminal_char_8(25) => escaped_c(27).
		q(nt(25), (nt(27)));
		//       terminal_char(24)    => '\'' __E_terminal_char_8(25) '\''.
		q(nt(24), (t(7)+nt(25)+t(7)));
		//       __E_unescaped_c_9(28) => '\''.
		q(nt(28), (t(7)));
		//       __E_unescaped_c_9(28) => '\\'.
		q(nt(28), (t(8)));
		//       __N_0(106)           => __E_unescaped_c_9(28).
		q(nt(106), (nt(28)));
		//       unescaped_c(26)      => printable(5) & ~( __N_0(106) ).	 # conjunctive
		q(nt(26), (nt(5)) & ~(nt(106)));
		//       __E_escaped_c_10(29) => '\''.
		q(nt(29), (t(7)));
		//       __E_escaped_c_10(29) => '\\'.
		q(nt(29), (t(8)));
		//       __E_escaped_c_10(29) => '/'.
		q(nt(29), (t(9)));
		//       __E_escaped_c_10(29) => 'b'.
		q(nt(29), (t(10)));
		//       __E_escaped_c_10(29) => 'f'.
		q(nt(29), (t(11)));
		//       __E_escaped_c_10(29) => 'n'.
		q(nt(29), (t(12)));
		//       __E_escaped_c_10(29) => 'r'.
		q(nt(29), (t(13)));
		//       __E_escaped_c_10(29) => 't'.
		q(nt(29), (t(14)));
		//       escaped_c(27)        => '\\' __E_escaped_c_10(29).
		q(nt(27), (t(8)+nt(29)));
		//       __E_terminal_string_11(32) => null.
		q(nt(32), (nul));
		//       __E_terminal_string_11(32) => terminal_string_char(31).
		q(nt(32), (nt(31)));
		//       __E_terminal_string_11(32) => terminal_string_char(31) __E_terminal_string_11(32).
		q(nt(32), (nt(31)+nt(32)));
		//       terminal_string(30)  => '"' __E_terminal_string_11(32) '"'.
		q(nt(30), (t(15)+nt(32)+t(15)));
		//       terminal_string_char(31) => unescaped_s(33).
		q(nt(31), (nt(33)));
		//       terminal_string_char(31) => escaped_s(34).
		q(nt(31), (nt(34)));
		//       __E_unescaped_s_12(35) => '"'.
		q(nt(35), (t(15)));
		//       __E_unescaped_s_12(35) => '\\'.
		q(nt(35), (t(8)));
		//       __N_1(107)           => __E_unescaped_s_12(35).
		q(nt(107), (nt(35)));
		//       unescaped_s(33)      => printable(5) & ~( __N_1(107) ).	 # conjunctive
		q(nt(33), (nt(5)) & ~(nt(107)));
		//       __E_escaped_s_13(36) => '"'.
		q(nt(36), (t(15)));
		//       __E_escaped_s_13(36) => '\\'.
		q(nt(36), (t(8)));
		//       __E_escaped_s_13(36) => '/'.
		q(nt(36), (t(9)));
		//       __E_escaped_s_13(36) => 'b'.
		q(nt(36), (t(10)));
		//       __E_escaped_s_13(36) => 'f'.
		q(nt(36), (t(11)));
		//       __E_escaped_s_13(36) => 'n'.
		q(nt(36), (t(12)));
		//       __E_escaped_s_13(36) => 'r'.
		q(nt(36), (t(13)));
		//       __E_escaped_s_13(36) => 't'.
		q(nt(36), (t(14)));
		//       escaped_s(34)        => '\\' __E_escaped_s_13(36).
		q(nt(34), (t(8)+nt(36)));
		//       terminal(37)         => terminal_char(24).
		q(nt(37), (nt(24)));
		//       terminal(37)         => terminal_string(30).
		q(nt(37), (nt(30)));
		//       __E_sym_14(39)       => alpha(3).
		q(nt(39), (nt(3)));
		//       __E_sym_14(39)       => '_'.
		q(nt(39), (t(6)));
		//       __E_sym_15(40)       => alnum(2).
		q(nt(40), (nt(2)));
		//       __E_sym_15(40)       => '_'.
		q(nt(40), (t(6)));
		//       __E_sym_16(41)       => null.
		q(nt(41), (nul));
		//       __E_sym_16(41)       => __E_sym_15(40).
		q(nt(41), (nt(40)));
		//       __E_sym_16(41)       => __E_sym_15(40) __E_sym_16(41).
		q(nt(41), (nt(40)+nt(41)));
		//       sym(38)              => __E_sym_14(39) __E_sym_16(41).
		q(nt(38), (nt(39)+nt(41)));
		//       production(12)       => sym(38) _(8) '=' '>' _(8) alternation(42) _(8) '.'.
		q(nt(12), (nt(38)+nt(8)+t(16)+t(17)+nt(8)+nt(42)+nt(8)+t(18)));
		//       __E_alternation_17(44) => _(8) '|' _(8) conjunction(43).
		q(nt(44), (nt(8)+t(19)+nt(8)+nt(43)));
		//       __E_alternation_18(45) => null.
		q(nt(45), (nul));
		//       __E_alternation_18(45) => __E_alternation_17(44).
		q(nt(45), (nt(44)));
		//       __E_alternation_18(45) => __E_alternation_17(44) __E_alternation_18(45).
		q(nt(45), (nt(44)+nt(45)));
		//       alternation(42)      => conjunction(43) __E_alternation_18(45).
		q(nt(42), (nt(43)+nt(45)));
		//       __E_conjunction_19(47) => _(8) '&' _(8) concatenation(46).
		q(nt(47), (nt(8)+t(20)+nt(8)+nt(46)));
		//       __E_conjunction_20(48) => null.
		q(nt(48), (nul));
		//       __E_conjunction_20(48) => __E_conjunction_19(47).
		q(nt(48), (nt(47)));
		//       __E_conjunction_20(48) => __E_conjunction_19(47) __E_conjunction_20(48).
		q(nt(48), (nt(47)+nt(48)));
		//       conjunction(43)      => concatenation(46) __E_conjunction_20(48).
		q(nt(43), (nt(46)+nt(48)));
		//       __E_concatenation_21(50) => __(15) factor(49).
		q(nt(50), (nt(15)+nt(49)));
		//       __E_concatenation_22(51) => null.
		q(nt(51), (nul));
		//       __E_concatenation_22(51) => __E_concatenation_21(50).
		q(nt(51), (nt(50)));
		//       __E_concatenation_22(51) => __E_concatenation_21(50) __E_concatenation_22(51).
		q(nt(51), (nt(50)+nt(51)));
		//       concatenation(46)    => factor(49) __E_concatenation_22(51).
		q(nt(46), (nt(49)+nt(51)));
		//       factor(49)           => optional(52).
		q(nt(49), (nt(52)));
		//       factor(49)           => repeat(53).
		q(nt(49), (nt(53)));
		//       factor(49)           => none_or_repeat(54).
		q(nt(49), (nt(54)));
		//       factor(49)           => neg(55).
		q(nt(49), (nt(55)));
		//       factor(49)           => term(56).
		q(nt(49), (nt(56)));
		//       term(56)             => group(57).
		q(nt(56), (nt(57)));
		//       term(56)             => optional_group(58).
		q(nt(56), (nt(58)));
		//       term(56)             => repeat_group(59).
		q(nt(56), (nt(59)));
		//       term(56)             => terminal(37).
		q(nt(56), (nt(37)));
		//       term(56)             => sym(38).
		q(nt(56), (nt(38)));
		//       neg(55)              => '~' term(56).
		q(nt(55), (t(21)+nt(56)));
		//       optional(52)         => term(56) '?'.
		q(nt(52), (nt(56)+t(22)));
		//       repeat(53)           => term(56) '+'.
		q(nt(53), (nt(56)+t(23)));
		//       none_or_repeat(54)   => term(56) '*'.
		q(nt(54), (nt(56)+t(24)));
		//       group(57)            => '(' _(8) alternation(42) _(8) ')'.
		q(nt(57), (t(25)+nt(8)+nt(42)+nt(8)+t(26)));
		//       optional_group(58)   => '[' _(8) alternation(42) _(8) ']'.
		q(nt(58), (t(27)+nt(8)+nt(42)+nt(8)+t(28)));
		//       repeat_group(59)     => '{' _(8) alternation(42) _(8) '}'.
		q(nt(59), (t(29)+nt(8)+nt(42)+nt(8)+t(30)));
		//       directive(11)        => '@' _(8) directive_body(60) _(8) '.'.
		q(nt(11), (t(31)+nt(8)+nt(60)+nt(8)+t(18)));
		//       directive_body(60)   => start_dir(61).
		q(nt(60), (nt(61)));
		//       directive_body(60)   => inline_dir(62).
		q(nt(60), (nt(62)));
		//       directive_body(60)   => trim_children_dir(63).
		q(nt(60), (nt(63)));
		//       directive_body(60)   => trim_terminals(64).
		q(nt(60), (nt(64)));
		//       directive_body(60)   => trim_dir(65).
		q(nt(60), (nt(65)));
		//       directive_body(60)   => use_dir(66).
		q(nt(60), (nt(66)));
		//       directive_body(60)   => boolean_dir(67).
		q(nt(60), (nt(67)));
		//       directive_body(60)   => nodisambig_dir(68).
		q(nt(60), (nt(68)));
		//       start_dir(61)        => 's' 't' 'a' 'r' 't' __(15) sym(38).
		q(nt(61), (t(32)+t(14)+t(33)+t(13)+t(14)+nt(15)+nt(38)));
		//       __E_trim_dir_23(69)  => _(8) ',' _(8) sym(38).
		q(nt(69), (nt(8)+t(34)+nt(8)+nt(38)));
		//       __E_trim_dir_24(70)  => null.
		q(nt(70), (nul));
		//       __E_trim_dir_24(70)  => __E_trim_dir_23(69).
		q(nt(70), (nt(69)));
		//       __E_trim_dir_24(70)  => __E_trim_dir_23(69) __E_trim_dir_24(70).
		q(nt(70), (nt(69)+nt(70)));
		//       trim_dir(65)         => 't' 'r' 'i' 'm' __(15) sym(38) __E_trim_dir_24(70).
		q(nt(65), (t(14)+t(13)+t(35)+t(36)+nt(15)+nt(38)+nt(70)));
		//       trim_terminals_dir(71) => 't' 'r' 'i' 'm' sep(21) 'a' 'l' 'l' sep(21) 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
		q(nt(71), (t(14)+t(13)+t(35)+t(36)+nt(21)+t(33)+t(37)+t(37)+nt(21)+t(14)+t(38)+t(13)+t(36)+t(35)+t(12)+t(33)+t(37)+t(32)));
		//       __E_trim_children_dir_25(73) => _(8) ',' _(8) trim_arg(72).
		q(nt(73), (nt(8)+t(34)+nt(8)+nt(72)));
		//       __E_trim_children_dir_26(74) => null.
		q(nt(74), (nul));
		//       __E_trim_children_dir_26(74) => __E_trim_children_dir_25(73).
		q(nt(74), (nt(73)));
		//       __E_trim_children_dir_26(74) => __E_trim_children_dir_25(73) __E_trim_children_dir_26(74).
		q(nt(74), (nt(73)+nt(74)));
		//       trim_children_dir(63) => 't' 'r' 'i' 'm' sep(21) 'c' 'h' 'i' 'l' 'd' 'r' 'e' 'n' __(15) trim_arg(72) __E_trim_children_dir_26(74).
		q(nt(63), (t(14)+t(13)+t(35)+t(36)+nt(21)+t(39)+t(40)+t(35)+t(37)+t(41)+t(13)+t(38)+t(12)+nt(15)+nt(72)+nt(74)));
		//       trim_arg(72)         => sym(38).
		q(nt(72), (nt(38)));
		//       trim_arg(72)         => all_terminals_sym(75).
		q(nt(72), (nt(75)));
		//       all_terminals_sym(75) => 'a' 'l' 'l' sep(21) 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
		q(nt(75), (t(33)+t(37)+t(37)+nt(21)+t(14)+t(38)+t(13)+t(36)+t(35)+t(12)+t(33)+t(37)+t(32)));
		//       __E_inline_dir_27(77) => _(8) ',' _(8) inline_arg(76).
		q(nt(77), (nt(8)+t(34)+nt(8)+nt(76)));
		//       __E_inline_dir_28(78) => null.
		q(nt(78), (nul));
		//       __E_inline_dir_28(78) => __E_inline_dir_27(77).
		q(nt(78), (nt(77)));
		//       __E_inline_dir_28(78) => __E_inline_dir_27(77) __E_inline_dir_28(78).
		q(nt(78), (nt(77)+nt(78)));
		//       inline_dir(62)       => 'i' 'n' 'l' 'i' 'n' 'e' __(15) inline_arg(76) __E_inline_dir_28(78).
		q(nt(62), (t(35)+t(12)+t(37)+t(35)+t(12)+t(38)+nt(15)+nt(76)+nt(78)));
		//       inline_arg(76)       => sym(38).
		q(nt(76), (nt(38)));
		//       inline_arg(76)       => char_classes_sym(79).
		q(nt(76), (nt(79)));
		//       __E_char_classes_sym_29(80) => 'e' 's'.
		q(nt(80), (t(38)+t(32)));
		//       __E_char_classes_sym_29(80) => null.
		q(nt(80), (nul));
		//       char_classes_sym(79) => 'c' 'h' 'a' 'r' sep(21) 'c' 'l' 'a' 's' 's' __E_char_classes_sym_29(80).
		q(nt(79), (t(39)+t(40)+t(33)+t(13)+nt(21)+t(39)+t(37)+t(33)+t(32)+t(32)+nt(80)));
		//       __E_use_dir_30(83)   => _(8) ',' _(8) use_param(82).
		q(nt(83), (nt(8)+t(34)+nt(8)+nt(82)));
		//       __E_use_dir_31(84)   => null.
		q(nt(84), (nul));
		//       __E_use_dir_31(84)   => __E_use_dir_30(83).
		q(nt(84), (nt(83)));
		//       __E_use_dir_31(84)   => __E_use_dir_30(83) __E_use_dir_31(84).
		q(nt(84), (nt(83)+nt(84)));
		//       use_dir(66)          => 'u' 's' 'e' __(15) use_from(81) __(15) use_param(82) __E_use_dir_31(84).
		q(nt(66), (t(42)+t(32)+t(38)+nt(15)+nt(81)+nt(15)+nt(82)+nt(84)));
		//       __E_use_dir_32(85)   => _(8) ',' _(8) use_param(82).
		q(nt(85), (nt(8)+t(34)+nt(8)+nt(82)));
		//       __E_use_dir_33(86)   => null.
		q(nt(86), (nul));
		//       __E_use_dir_33(86)   => __E_use_dir_32(85).
		q(nt(86), (nt(85)));
		//       __E_use_dir_33(86)   => __E_use_dir_32(85) __E_use_dir_33(86).
		q(nt(86), (nt(85)+nt(86)));
		//       use_dir(66)          => 'u' 's' 'e' '_' 'c' 'h' 'a' 'r' '_' 'c' 'l' 'a' 's' 's' __(15) use_param(82) __E_use_dir_33(86).
		q(nt(66), (t(42)+t(32)+t(38)+t(6)+t(39)+t(40)+t(33)+t(13)+t(6)+t(39)+t(37)+t(33)+t(32)+t(32)+nt(15)+nt(82)+nt(86)));
		//       use_from(81)         => char_classes_sym(79).
		q(nt(81), (nt(79)));
		//       use_param(82)        => char_class_name(87).
		q(nt(82), (nt(87)));
		//       char_class_name(87)  => 'e' 'o' 'f'.
		q(nt(87), (t(38)+t(43)+t(11)));
		//       char_class_name(87)  => 'a' 'l' 'n' 'u' 'm'.
		q(nt(87), (t(33)+t(37)+t(12)+t(42)+t(36)));
		//       char_class_name(87)  => 'a' 'l' 'p' 'h' 'a'.
		q(nt(87), (t(33)+t(37)+t(44)+t(40)+t(33)));
		//       char_class_name(87)  => 'b' 'l' 'a' 'n' 'k'.
		q(nt(87), (t(10)+t(37)+t(33)+t(12)+t(45)));
		//       char_class_name(87)  => 'c' 'n' 't' 'r' 'l'.
		q(nt(87), (t(39)+t(12)+t(14)+t(13)+t(37)));
		//       char_class_name(87)  => 'd' 'i' 'g' 'i' 't'.
		q(nt(87), (t(41)+t(35)+t(46)+t(35)+t(14)));
		//       char_class_name(87)  => 'g' 'r' 'a' 'p' 'h'.
		q(nt(87), (t(46)+t(13)+t(33)+t(44)+t(40)));
		//       char_class_name(87)  => 'l' 'o' 'w' 'e' 'r'.
		q(nt(87), (t(37)+t(43)+t(47)+t(38)+t(13)));
		//       char_class_name(87)  => 'p' 'r' 'i' 'n' 't' 'a' 'b' 'l' 'e'.
		q(nt(87), (t(44)+t(13)+t(35)+t(12)+t(14)+t(33)+t(10)+t(37)+t(38)));
		//       char_class_name(87)  => 'p' 'u' 'n' 'c' 't'.
		q(nt(87), (t(44)+t(42)+t(12)+t(39)+t(14)));
		//       char_class_name(87)  => 's' 'p' 'a' 'c' 'e'.
		q(nt(87), (t(32)+t(44)+t(33)+t(39)+t(38)));
		//       char_class_name(87)  => 'u' 'p' 'p' 'e' 'r'.
		q(nt(87), (t(42)+t(44)+t(44)+t(38)+t(13)));
		//       char_class_name(87)  => 'x' 'd' 'i' 'g' 'i' 't'.
		q(nt(87), (t(48)+t(41)+t(35)+t(46)+t(35)+t(14)));
		//       __E_boolean_dir_34(88) => boolean_action(89) __(15).
		q(nt(88), (nt(89)+nt(15)));
		//       __E_boolean_dir_34(88) => null.
		q(nt(88), (nul));
		//       boolean_dir(67)      => __E_boolean_dir_34(88) boolean_dir_name(90).
		q(nt(67), (nt(88)+nt(90)));
		//       boolean_dir_name(90) => autodisambig_sym(91).
		q(nt(90), (nt(91)));
		//       boolean_action(89)   => enable_sym(92).
		q(nt(89), (nt(92)));
		//       boolean_action(89)   => disable_sym(93).
		q(nt(89), (nt(93)));
		//       enable_sym(92)       => 'e' 'n' 'a' 'b' 'l' 'e'.
		q(nt(92), (t(38)+t(12)+t(33)+t(10)+t(37)+t(38)));
		//       disable_sym(93)      => 'd' 'i' 's' 'a' 'b' 'l' 'e'.
		q(nt(93), (t(41)+t(35)+t(32)+t(33)+t(10)+t(37)+t(38)));
		//       __E_autodisambig_sym_35(94) => 'a' 'u' 't' 'o' sep(21).
		q(nt(94), (t(33)+t(42)+t(14)+t(43)+nt(21)));
		//       __E_autodisambig_sym_35(94) => null.
		q(nt(94), (nul));
		//       autodisambig_sym(91) => __E_autodisambig_sym_35(94) disambig_sym(95).
		q(nt(91), (nt(94)+nt(95)));
		//       __E___E_disambig_sym_36_37(97) => 'e'.
		q(nt(97), (t(38)));
		//       __E___E___E_disambig_sym_36_37_38(98) => 'o' 'n'.
		q(nt(98), (t(43)+t(12)));
		//       __E___E___E_disambig_sym_36_37_38(98) => 'n' 'g'.
		q(nt(98), (t(12)+t(46)));
		//       __E___E_disambig_sym_36_37(97) => 'i' __E___E___E_disambig_sym_36_37_38(98).
		q(nt(97), (t(35)+nt(98)));
		//       __E_disambig_sym_36(96) => 'u' 'a' 't' __E___E_disambig_sym_36_37(97).
		q(nt(96), (t(42)+t(33)+t(14)+nt(97)));
		//       __E_disambig_sym_36(96) => null.
		q(nt(96), (nul));
		//       disambig_sym(95)     => 'd' 'i' 's' 'a' 'm' 'b' 'i' 'g' __E_disambig_sym_36(96).
		q(nt(95), (t(41)+t(35)+t(32)+t(33)+t(36)+t(10)+t(35)+t(46)+nt(96)));
		//       __E_nodisambig_dir_39(100) => _(8) ',' _(8) sym(38).
		q(nt(100), (nt(8)+t(34)+nt(8)+nt(38)));
		//       __E_nodisambig_dir_40(101) => null.
		q(nt(101), (nul));
		//       __E_nodisambig_dir_40(101) => __E_nodisambig_dir_39(100).
		q(nt(101), (nt(100)));
		//       __E_nodisambig_dir_40(101) => __E_nodisambig_dir_39(100) __E_nodisambig_dir_40(101).
		q(nt(101), (nt(100)+nt(101)));
		//       nodisambig_dir(68)   => nodisambig_sym(99) _(8) sym(38) __E_nodisambig_dir_40(101).
		q(nt(68), (nt(99)+nt(8)+nt(38)+nt(101)));
		//       __E___E_nodisambig_sym_41_42(103) => 'n'.
		q(nt(103), (t(12)));
		//       __E___E_nodisambig_sym_41_42(103) => null.
		q(nt(103), (nul));
		//       __E_nodisambig_sym_41(102) => 'n' 'o' __E___E_nodisambig_sym_41_42(103).
		q(nt(102), (t(12)+t(43)+nt(103)));
		//       __E___E_nodisambig_sym_41_43(104) => '\''.
		q(nt(104), (t(7)));
		//       __E___E_nodisambig_sym_41_43(104) => null.
		q(nt(104), (nul));
		//       __E_nodisambig_sym_41(102) => 'd' 'o' 'n' __E___E_nodisambig_sym_41_43(104) 't'.
		q(nt(102), (t(41)+t(43)+t(12)+nt(104)+t(14)));
		//       __E_nodisambig_sym_44(105) => sep(21) 'l' 'i' 's' 't'.
		q(nt(105), (nt(21)+t(37)+t(35)+t(32)+t(14)));
		//       __E_nodisambig_sym_44(105) => null.
		q(nt(105), (nul));
		//       nodisambig_sym(99)   => __E_nodisambig_sym_41(102) sep(21) disambig_sym(95) __E_nodisambig_sym_44(105).
		q(nt(99), (nt(102)+nt(21)+nt(95)+nt(105)));
		return q;
	}
};

#endif // __TGF_PARSER_H__
