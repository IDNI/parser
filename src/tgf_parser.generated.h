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
	using parse_result    = parser_type::result;
	using forest_type     = parser_type::pforest;
	using sptree_type     = parser_type::psptree;
	using input_type      = parser_type::input;
	using decoder_type    = parser_type::input::decoder_type;
	using encoder_type    = std::function<std::basic_string<char_type>(
			const std::vector<terminal_type>&)>;
	static tgf_parser& instance() { static tgf_parser i; return i; }	tgf_parser() :
		nts(load_nonterminals()), cc(load_cc()),
		g(nts, load_prods(), nt(11), cc, load_grammar_opts()),
		p(g, load_opts()) {}
	parse_result parse(const char_type* data, size_t size,
		parse_options po = {}) { return p.parse(data, size, po); }
	parse_result parse(std::basic_istream<char_type>& is,
		parse_options po = {}) { return p.parse(is, po); }
	parse_result parse(const std::string& fn,
		parse_options po = {}) { return p.parse(fn, po); }
#ifndef WIN32
	parse_result parse(int fd, parse_options po = {})
		{ return p.parse(fd, po); }
#endif //WIN32
	sptree_type shape(const parse_result& r, const node_type& n) {
		return r.get_shaped_tree(n, g.opt.shaping);
	}
	sptree_type parse_and_shape(const char_type* data, size_t size,
		const node_type& n, parse_options po = {})
	{
		return shape(p.parse(data, size, po), n);
	}
	sptree_type parse_and_shape(const char_type* data, size_t size,
		parse_options po = {})
	{
		auto r = p.parse(data, size, po);
		return shape(r, r.get_forest()->root());
	}
	sptree_type parse_and_shape(std::basic_istream<char_type>& is,
		const node_type& n, parse_options po = {})
	{
		return shape(p.parse(is, po), n);
	}
	sptree_type parse_and_shape(std::basic_istream<char_type>& is,
		parse_options po = {})
	{
		auto r = p.parse(is, po);
		return shape(r, r.get_forest()->root());
	}
	sptree_type parse_and_shape(const std::string& fn,
		const node_type& n, parse_options po = {})
	{
		return shape(p.parse(fn, po), n);
	}
	sptree_type parse_and_shape(const std::string& fn,
		parse_options po = {})
	{
		auto r = p.parse(fn, po);
		return shape(r, r.get_forest()->root());
	}
#ifndef WIN32
	sptree_type parse_and_shape(int fd, const node_type& n, parse_options po = {})
	{
		return shape(p.parse(fd, po), n);
	}
	sptree_type parse_and_shape(int fd, parse_options po = {})
	{
		auto r = p.parse(fd, po);
		return shape(r, r.get_forest()->root());
	}
#endif //WIN32
	enum nonterminal {
		nul, eof, alnum, alpha, space, printable, __, _, digits, chars, 
		charvar, start, __E_start_0, statement, __E_start_1, directive, production, start_statement, __E___2, __E____3, 
		comment, __E_comment_4, __E_comment_5, __E_comment_6, sep, __E_sep_7, sep_required, terminal_char, __E_terminal_char_8, unescaped_c, 
		escaped_c, __E_unescaped_c_9, __E_escaped_c_10, terminal_string, terminal_string_char, __E_terminal_string_11, unescaped_s, escaped_s, __E_unescaped_s_12, __E_escaped_s_13, 
		terminal, sym, __E_sym_14, __E_sym_15, __E_sym_16, alternation, conjunction, __E_alternation_17, __E_alternation_18, concatenation, 
		__E_conjunction_19, __E_conjunction_20, factor, __E_concatenation_21, __E_concatenation_22, shorthand_rule, optional, repeat, none_or_repeat, neg, 
		term, group, optional_group, repeat_group, directive_body, start_dir, inline_dir, trim_children_dir, trim_terminals, trim_dir, 
		use_dir, boolean_dir, nodisambig_dir, __E_trim_dir_23, __E_trim_dir_24, trim_terminals_dir, __E_trim_children_dir_25, __E_trim_children_dir_26, inline_arg, __E_inline_dir_27, 
		__E_inline_dir_28, tree_path, char_classes_sym, __E_tree_path_29, __E_tree_path_30, __E_char_classes_sym_31, use_from, use_param, __E_use_dir_32, __E_use_dir_33, 
		__E_use_dir_34, __E_use_dir_35, char_class_name, __E_boolean_dir_36, boolean_action, boolean_dir_name, autodisambig_sym, enable_sym, disable_sym, __E_autodisambig_sym_37, 
		disambig_sym, __E_disambig_sym_38, __E___E_disambig_sym_38_39, __E___E___E_disambig_sym_38_39_40, nodisambig_sym, __E_nodisambig_dir_41, __E_nodisambig_dir_42, __E_nodisambig_sym_43, __E___E_nodisambig_sym_43_44, __E___E_nodisambig_sym_43_45, 
		__E_nodisambig_sym_46, __N_0, __N_1, 
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
		'|', '&', ':', '~', '?', '+', '*', '(', ')', '[', 
		']', '{', '}', '@', 's', 'a', ',', 'i', 'm', 'l', 
		'e', 'c', 'h', 'd', 'u', 'o', 'p', 'k', 'g', 'w', 
		'x', 
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
			"", "eof", "alnum", "alpha", "space", "printable", "__", "_", "digits", "chars", 
			"charvar", "start", "__E_start_0", "statement", "__E_start_1", "directive", "production", "start_statement", "__E___2", "__E____3", 
			"comment", "__E_comment_4", "__E_comment_5", "__E_comment_6", "sep", "__E_sep_7", "sep_required", "terminal_char", "__E_terminal_char_8", "unescaped_c", 
			"escaped_c", "__E_unescaped_c_9", "__E_escaped_c_10", "terminal_string", "terminal_string_char", "__E_terminal_string_11", "unescaped_s", "escaped_s", "__E_unescaped_s_12", "__E_escaped_s_13", 
			"terminal", "sym", "__E_sym_14", "__E_sym_15", "__E_sym_16", "alternation", "conjunction", "__E_alternation_17", "__E_alternation_18", "concatenation", 
			"__E_conjunction_19", "__E_conjunction_20", "factor", "__E_concatenation_21", "__E_concatenation_22", "shorthand_rule", "optional", "repeat", "none_or_repeat", "neg", 
			"term", "group", "optional_group", "repeat_group", "directive_body", "start_dir", "inline_dir", "trim_children_dir", "trim_terminals", "trim_dir", 
			"use_dir", "boolean_dir", "nodisambig_dir", "__E_trim_dir_23", "__E_trim_dir_24", "trim_terminals_dir", "__E_trim_children_dir_25", "__E_trim_children_dir_26", "inline_arg", "__E_inline_dir_27", 
			"__E_inline_dir_28", "tree_path", "char_classes_sym", "__E_tree_path_29", "__E_tree_path_30", "__E_char_classes_sym_31", "use_from", "use_param", "__E_use_dir_32", "__E_use_dir_33", 
			"__E_use_dir_34", "__E_use_dir_35", "char_class_name", "__E_boolean_dir_36", "boolean_action", "boolean_dir_name", "autodisambig_sym", "enable_sym", "disable_sym", "__E_autodisambig_sym_37", 
			"disambig_sym", "__E_disambig_sym_38", "__E___E_disambig_sym_38_39", "__E___E___E_disambig_sym_38_39_40", "nodisambig_sym", "__E_nodisambig_dir_41", "__E_nodisambig_dir_42", "__E_nodisambig_sym_43", "__E___E_nodisambig_sym_43_44", "__E___E_nodisambig_sym_43_45", 
			"__E_nodisambig_sym_46", "__N_0", "__N_1", 
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
		o.transform_negation = false;
		o.auto_disambiguate = true;
		o.shaping.to_trim = {
			6, 7
		};
		o.shaping.trim_terminals = false;
		o.shaping.inline_char_classes = true;
		o.shaping.to_inline = {
			{ 8 },
			{ 9 },
			{ 10 }
		};
		return o;
	}
	options load_opts() {
		options o;
		return o;
	}
	idni::prods<char_type, terminal_type> load_prods() {
		idni::prods<char_type, terminal_type> q,
			nul(symbol_type{});
		//       __E_start_0(12)      => _(7) statement(13).
		q(nt(12), (nt(7)+nt(13)));
		//       __E_start_1(14)      => null.
		q(nt(14), (nul));
		//       __E_start_1(14)      => __E_start_0(12) __E_start_1(14).
		q(nt(14), (nt(12)+nt(14)));
		//       start(11)            => __E_start_1(14) _(7).
		q(nt(11), (nt(14)+nt(7)));
		//       statement(13)        => directive(15).
		q(nt(13), (nt(15)));
		//       statement(13)        => production(16).
		q(nt(13), (nt(16)));
		//       start_statement(17)  => _(7) statement(13) _(7).
		q(nt(17), (nt(7)+nt(13)+nt(7)));
		//       __E___2(18)          => __(6).
		q(nt(18), (nt(6)));
		//       __E___2(18)          => null.
		q(nt(18), (nul));
		//       _(7)                 => __E___2(18).
		q(nt(7), (nt(18)));
		//       __E____3(19)         => space(4).
		q(nt(19), (nt(4)));
		//       __E____3(19)         => comment(20).
		q(nt(19), (nt(20)));
		//       __(6)                => __E____3(19) _(7).
		q(nt(6), (nt(19)+nt(7)));
		//       __E_comment_4(21)    => printable(5).
		q(nt(21), (nt(5)));
		//       __E_comment_4(21)    => '\t'.
		q(nt(21), (t(1)));
		//       __E_comment_5(22)    => null.
		q(nt(22), (nul));
		//       __E_comment_5(22)    => __E_comment_4(21) __E_comment_5(22).
		q(nt(22), (nt(21)+nt(22)));
		//       __E_comment_6(23)    => '\r'.
		q(nt(23), (t(2)));
		//       __E_comment_6(23)    => '\n'.
		q(nt(23), (t(3)));
		//       __E_comment_6(23)    => eof(1).
		q(nt(23), (nt(1)));
		//       comment(20)          => '#' __E_comment_5(22) __E_comment_6(23).
		q(nt(20), (t(4)+nt(22)+nt(23)));
		//       __E_sep_7(25)        => sep_required(26).
		q(nt(25), (nt(26)));
		//       __E_sep_7(25)        => null.
		q(nt(25), (nul));
		//       sep(24)              => __E_sep_7(25).
		q(nt(24), (nt(25)));
		//       sep_required(26)     => '-'.
		q(nt(26), (t(5)));
		//       sep_required(26)     => '_'.
		q(nt(26), (t(6)));
		//       sep_required(26)     => __(6).
		q(nt(26), (nt(6)));
		//       __E_terminal_char_8(28) => unescaped_c(29).
		q(nt(28), (nt(29)));
		//       __E_terminal_char_8(28) => escaped_c(30).
		q(nt(28), (nt(30)));
		//       terminal_char(27)    => '\'' __E_terminal_char_8(28) '\''.
		q(nt(27), (t(7)+nt(28)+t(7)));
		//       __E_unescaped_c_9(31) => '\''.
		q(nt(31), (t(7)));
		//       __E_unescaped_c_9(31) => '\\'.
		q(nt(31), (t(8)));
		//       __N_0(111)           => __E_unescaped_c_9(31).
		q(nt(111), (nt(31)));
		//       unescaped_c(29)      => printable(5) & ~( __N_0(111) ).	 # conjunctive
		q(nt(29), (nt(5)) & ~(nt(111)));
		//       __E_escaped_c_10(32) => '\''.
		q(nt(32), (t(7)));
		//       __E_escaped_c_10(32) => '\\'.
		q(nt(32), (t(8)));
		//       __E_escaped_c_10(32) => '/'.
		q(nt(32), (t(9)));
		//       __E_escaped_c_10(32) => 'b'.
		q(nt(32), (t(10)));
		//       __E_escaped_c_10(32) => 'f'.
		q(nt(32), (t(11)));
		//       __E_escaped_c_10(32) => 'n'.
		q(nt(32), (t(12)));
		//       __E_escaped_c_10(32) => 'r'.
		q(nt(32), (t(13)));
		//       __E_escaped_c_10(32) => 't'.
		q(nt(32), (t(14)));
		//       escaped_c(30)        => '\\' __E_escaped_c_10(32).
		q(nt(30), (t(8)+nt(32)));
		//       __E_terminal_string_11(35) => null.
		q(nt(35), (nul));
		//       __E_terminal_string_11(35) => terminal_string_char(34) __E_terminal_string_11(35).
		q(nt(35), (nt(34)+nt(35)));
		//       terminal_string(33)  => '"' __E_terminal_string_11(35) '"'.
		q(nt(33), (t(15)+nt(35)+t(15)));
		//       terminal_string_char(34) => unescaped_s(36).
		q(nt(34), (nt(36)));
		//       terminal_string_char(34) => escaped_s(37).
		q(nt(34), (nt(37)));
		//       __E_unescaped_s_12(38) => '"'.
		q(nt(38), (t(15)));
		//       __E_unescaped_s_12(38) => '\\'.
		q(nt(38), (t(8)));
		//       __N_1(112)           => __E_unescaped_s_12(38).
		q(nt(112), (nt(38)));
		//       unescaped_s(36)      => printable(5) & ~( __N_1(112) ).	 # conjunctive
		q(nt(36), (nt(5)) & ~(nt(112)));
		//       __E_escaped_s_13(39) => '"'.
		q(nt(39), (t(15)));
		//       __E_escaped_s_13(39) => '\\'.
		q(nt(39), (t(8)));
		//       __E_escaped_s_13(39) => '/'.
		q(nt(39), (t(9)));
		//       __E_escaped_s_13(39) => 'b'.
		q(nt(39), (t(10)));
		//       __E_escaped_s_13(39) => 'f'.
		q(nt(39), (t(11)));
		//       __E_escaped_s_13(39) => 'n'.
		q(nt(39), (t(12)));
		//       __E_escaped_s_13(39) => 'r'.
		q(nt(39), (t(13)));
		//       __E_escaped_s_13(39) => 't'.
		q(nt(39), (t(14)));
		//       escaped_s(37)        => '\\' __E_escaped_s_13(39).
		q(nt(37), (t(8)+nt(39)));
		//       terminal(40)         => terminal_char(27).
		q(nt(40), (nt(27)));
		//       terminal(40)         => terminal_string(33).
		q(nt(40), (nt(33)));
		//       __E_sym_14(42)       => alpha(3).
		q(nt(42), (nt(3)));
		//       __E_sym_14(42)       => '_'.
		q(nt(42), (t(6)));
		//       __E_sym_15(43)       => alnum(2).
		q(nt(43), (nt(2)));
		//       __E_sym_15(43)       => '_'.
		q(nt(43), (t(6)));
		//       __E_sym_16(44)       => null.
		q(nt(44), (nul));
		//       __E_sym_16(44)       => __E_sym_15(43) __E_sym_16(44).
		q(nt(44), (nt(43)+nt(44)));
		//       sym(41)              => __E_sym_14(42) __E_sym_16(44).
		q(nt(41), (nt(42)+nt(44)));
		//       production(16)       => sym(41) _(7) '=' '>' _(7) alternation(45) _(7) '.'.
		q(nt(16), (nt(41)+nt(7)+t(16)+t(17)+nt(7)+nt(45)+nt(7)+t(18)));
		//       __E_alternation_17(47) => _(7) '|' _(7) conjunction(46).
		q(nt(47), (nt(7)+t(19)+nt(7)+nt(46)));
		//       __E_alternation_18(48) => null.
		q(nt(48), (nul));
		//       __E_alternation_18(48) => __E_alternation_17(47) __E_alternation_18(48).
		q(nt(48), (nt(47)+nt(48)));
		//       alternation(45)      => conjunction(46) __E_alternation_18(48).
		q(nt(45), (nt(46)+nt(48)));
		//       __E_conjunction_19(50) => _(7) '&' _(7) concatenation(49).
		q(nt(50), (nt(7)+t(20)+nt(7)+nt(49)));
		//       __E_conjunction_20(51) => null.
		q(nt(51), (nul));
		//       __E_conjunction_20(51) => __E_conjunction_19(50) __E_conjunction_20(51).
		q(nt(51), (nt(50)+nt(51)));
		//       conjunction(46)      => concatenation(49) __E_conjunction_20(51).
		q(nt(46), (nt(49)+nt(51)));
		//       __E_concatenation_21(53) => __(6) factor(52).
		q(nt(53), (nt(6)+nt(52)));
		//       __E_concatenation_22(54) => null.
		q(nt(54), (nul));
		//       __E_concatenation_22(54) => __E_concatenation_21(53) __E_concatenation_22(54).
		q(nt(54), (nt(53)+nt(54)));
		//       concatenation(49)    => factor(52) __E_concatenation_22(54).
		q(nt(49), (nt(52)+nt(54)));
		//       factor(52)           => shorthand_rule(55).
		q(nt(52), (nt(55)));
		//       factor(52)           => optional(56).
		q(nt(52), (nt(56)));
		//       factor(52)           => repeat(57).
		q(nt(52), (nt(57)));
		//       factor(52)           => none_or_repeat(58).
		q(nt(52), (nt(58)));
		//       factor(52)           => neg(59).
		q(nt(52), (nt(59)));
		//       factor(52)           => term(60).
		q(nt(52), (nt(60)));
		//       term(60)             => group(61).
		q(nt(60), (nt(61)));
		//       term(60)             => optional_group(62).
		q(nt(60), (nt(62)));
		//       term(60)             => repeat_group(63).
		q(nt(60), (nt(63)));
		//       term(60)             => terminal(40).
		q(nt(60), (nt(40)));
		//       term(60)             => sym(41).
		q(nt(60), (nt(41)));
		//       shorthand_rule(55)   => factor(52) _(7) ':' sym(41).
		q(nt(55), (nt(52)+nt(7)+t(21)+nt(41)));
		//       neg(59)              => '~' term(60).
		q(nt(59), (t(22)+nt(60)));
		//       optional(56)         => term(60) '?'.
		q(nt(56), (nt(60)+t(23)));
		//       repeat(57)           => term(60) '+'.
		q(nt(57), (nt(60)+t(24)));
		//       none_or_repeat(58)   => term(60) '*'.
		q(nt(58), (nt(60)+t(25)));
		//       group(61)            => '(' _(7) alternation(45) _(7) ')'.
		q(nt(61), (t(26)+nt(7)+nt(45)+nt(7)+t(27)));
		//       optional_group(62)   => '[' _(7) alternation(45) _(7) ']'.
		q(nt(62), (t(28)+nt(7)+nt(45)+nt(7)+t(29)));
		//       repeat_group(63)     => '{' _(7) alternation(45) _(7) '}'.
		q(nt(63), (t(30)+nt(7)+nt(45)+nt(7)+t(31)));
		//       directive(15)        => '@' _(7) directive_body(64) _(7) '.'.
		q(nt(15), (t(32)+nt(7)+nt(64)+nt(7)+t(18)));
		//       directive_body(64)   => start_dir(65).
		q(nt(64), (nt(65)));
		//       directive_body(64)   => inline_dir(66).
		q(nt(64), (nt(66)));
		//       directive_body(64)   => trim_children_dir(67).
		q(nt(64), (nt(67)));
		//       directive_body(64)   => trim_terminals(68).
		q(nt(64), (nt(68)));
		//       directive_body(64)   => trim_dir(69).
		q(nt(64), (nt(69)));
		//       directive_body(64)   => use_dir(70).
		q(nt(64), (nt(70)));
		//       directive_body(64)   => boolean_dir(71).
		q(nt(64), (nt(71)));
		//       directive_body(64)   => nodisambig_dir(72).
		q(nt(64), (nt(72)));
		//       start_dir(65)        => 's' 't' 'a' 'r' 't' __(6) sym(41).
		q(nt(65), (t(33)+t(14)+t(34)+t(13)+t(14)+nt(6)+nt(41)));
		//       __E_trim_dir_23(73)  => _(7) ',' _(7) sym(41).
		q(nt(73), (nt(7)+t(35)+nt(7)+nt(41)));
		//       __E_trim_dir_24(74)  => null.
		q(nt(74), (nul));
		//       __E_trim_dir_24(74)  => __E_trim_dir_23(73) __E_trim_dir_24(74).
		q(nt(74), (nt(73)+nt(74)));
		//       trim_dir(69)         => 't' 'r' 'i' 'm' __(6) sym(41) __E_trim_dir_24(74).
		q(nt(69), (t(14)+t(13)+t(36)+t(37)+nt(6)+nt(41)+nt(74)));
		//       trim_terminals_dir(75) => 't' 'r' 'i' 'm' sep(24) 'a' 'l' 'l' sep(24) 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
		q(nt(75), (t(14)+t(13)+t(36)+t(37)+nt(24)+t(34)+t(38)+t(38)+nt(24)+t(14)+t(39)+t(13)+t(37)+t(36)+t(12)+t(34)+t(38)+t(33)));
		//       __E_trim_children_dir_25(76) => _(7) ',' _(7) sym(41).
		q(nt(76), (nt(7)+t(35)+nt(7)+nt(41)));
		//       __E_trim_children_dir_26(77) => null.
		q(nt(77), (nul));
		//       __E_trim_children_dir_26(77) => __E_trim_children_dir_25(76) __E_trim_children_dir_26(77).
		q(nt(77), (nt(76)+nt(77)));
		//       trim_children_dir(67) => 't' 'r' 'i' 'm' sep(24) 'c' 'h' 'i' 'l' 'd' 'r' 'e' 'n' __(6) sym(41) __E_trim_children_dir_26(77).
		q(nt(67), (t(14)+t(13)+t(36)+t(37)+nt(24)+t(40)+t(41)+t(36)+t(38)+t(42)+t(13)+t(39)+t(12)+nt(6)+nt(41)+nt(77)));
		//       __E_inline_dir_27(79) => _(7) ',' _(7) inline_arg(78).
		q(nt(79), (nt(7)+t(35)+nt(7)+nt(78)));
		//       __E_inline_dir_28(80) => null.
		q(nt(80), (nul));
		//       __E_inline_dir_28(80) => __E_inline_dir_27(79) __E_inline_dir_28(80).
		q(nt(80), (nt(79)+nt(80)));
		//       inline_dir(66)       => 'i' 'n' 'l' 'i' 'n' 'e' __(6) inline_arg(78) __E_inline_dir_28(80).
		q(nt(66), (t(36)+t(12)+t(38)+t(36)+t(12)+t(39)+nt(6)+nt(78)+nt(80)));
		//       inline_arg(78)       => tree_path(81).
		q(nt(78), (nt(81)));
		//       inline_arg(78)       => char_classes_sym(82).
		q(nt(78), (nt(82)));
		//       __E_tree_path_29(83) => _(7) '>' _(7) sym(41).
		q(nt(83), (nt(7)+t(17)+nt(7)+nt(41)));
		//       __E_tree_path_30(84) => null.
		q(nt(84), (nul));
		//       __E_tree_path_30(84) => __E_tree_path_29(83) __E_tree_path_30(84).
		q(nt(84), (nt(83)+nt(84)));
		//       tree_path(81)        => sym(41) __E_tree_path_30(84).
		q(nt(81), (nt(41)+nt(84)));
		//       __E_char_classes_sym_31(85) => 'e' 's'.
		q(nt(85), (t(39)+t(33)));
		//       __E_char_classes_sym_31(85) => null.
		q(nt(85), (nul));
		//       char_classes_sym(82) => 'c' 'h' 'a' 'r' sep(24) 'c' 'l' 'a' 's' 's' __E_char_classes_sym_31(85).
		q(nt(82), (t(40)+t(41)+t(34)+t(13)+nt(24)+t(40)+t(38)+t(34)+t(33)+t(33)+nt(85)));
		//       __E_use_dir_32(88)   => _(7) ',' _(7) use_param(87).
		q(nt(88), (nt(7)+t(35)+nt(7)+nt(87)));
		//       __E_use_dir_33(89)   => null.
		q(nt(89), (nul));
		//       __E_use_dir_33(89)   => __E_use_dir_32(88) __E_use_dir_33(89).
		q(nt(89), (nt(88)+nt(89)));
		//       use_dir(70)          => 'u' 's' 'e' __(6) use_from(86) __(6) use_param(87) __E_use_dir_33(89).
		q(nt(70), (t(43)+t(33)+t(39)+nt(6)+nt(86)+nt(6)+nt(87)+nt(89)));
		//       __E_use_dir_34(90)   => _(7) ',' _(7) use_param(87).
		q(nt(90), (nt(7)+t(35)+nt(7)+nt(87)));
		//       __E_use_dir_35(91)   => null.
		q(nt(91), (nul));
		//       __E_use_dir_35(91)   => __E_use_dir_34(90) __E_use_dir_35(91).
		q(nt(91), (nt(90)+nt(91)));
		//       use_dir(70)          => 'u' 's' 'e' '_' 'c' 'h' 'a' 'r' '_' 'c' 'l' 'a' 's' 's' __(6) use_param(87) __E_use_dir_35(91).
		q(nt(70), (t(43)+t(33)+t(39)+t(6)+t(40)+t(41)+t(34)+t(13)+t(6)+t(40)+t(38)+t(34)+t(33)+t(33)+nt(6)+nt(87)+nt(91)));
		//       use_from(86)         => char_classes_sym(82).
		q(nt(86), (nt(82)));
		//       use_param(87)        => char_class_name(92).
		q(nt(87), (nt(92)));
		//       char_class_name(92)  => 'e' 'o' 'f'.
		q(nt(92), (t(39)+t(44)+t(11)));
		//       char_class_name(92)  => 'a' 'l' 'n' 'u' 'm'.
		q(nt(92), (t(34)+t(38)+t(12)+t(43)+t(37)));
		//       char_class_name(92)  => 'a' 'l' 'p' 'h' 'a'.
		q(nt(92), (t(34)+t(38)+t(45)+t(41)+t(34)));
		//       char_class_name(92)  => 'b' 'l' 'a' 'n' 'k'.
		q(nt(92), (t(10)+t(38)+t(34)+t(12)+t(46)));
		//       char_class_name(92)  => 'c' 'n' 't' 'r' 'l'.
		q(nt(92), (t(40)+t(12)+t(14)+t(13)+t(38)));
		//       char_class_name(92)  => 'd' 'i' 'g' 'i' 't'.
		q(nt(92), (t(42)+t(36)+t(47)+t(36)+t(14)));
		//       char_class_name(92)  => 'g' 'r' 'a' 'p' 'h'.
		q(nt(92), (t(47)+t(13)+t(34)+t(45)+t(41)));
		//       char_class_name(92)  => 'l' 'o' 'w' 'e' 'r'.
		q(nt(92), (t(38)+t(44)+t(48)+t(39)+t(13)));
		//       char_class_name(92)  => 'p' 'r' 'i' 'n' 't' 'a' 'b' 'l' 'e'.
		q(nt(92), (t(45)+t(13)+t(36)+t(12)+t(14)+t(34)+t(10)+t(38)+t(39)));
		//       char_class_name(92)  => 'p' 'u' 'n' 'c' 't'.
		q(nt(92), (t(45)+t(43)+t(12)+t(40)+t(14)));
		//       char_class_name(92)  => 's' 'p' 'a' 'c' 'e'.
		q(nt(92), (t(33)+t(45)+t(34)+t(40)+t(39)));
		//       char_class_name(92)  => 'u' 'p' 'p' 'e' 'r'.
		q(nt(92), (t(43)+t(45)+t(45)+t(39)+t(13)));
		//       char_class_name(92)  => 'x' 'd' 'i' 'g' 'i' 't'.
		q(nt(92), (t(49)+t(42)+t(36)+t(47)+t(36)+t(14)));
		//       __E_boolean_dir_36(93) => boolean_action(94) __(6).
		q(nt(93), (nt(94)+nt(6)));
		//       __E_boolean_dir_36(93) => null.
		q(nt(93), (nul));
		//       boolean_dir(71)      => __E_boolean_dir_36(93) boolean_dir_name(95).
		q(nt(71), (nt(93)+nt(95)));
		//       boolean_dir_name(95) => autodisambig_sym(96).
		q(nt(95), (nt(96)));
		//       boolean_action(94)   => enable_sym(97).
		q(nt(94), (nt(97)));
		//       boolean_action(94)   => disable_sym(98).
		q(nt(94), (nt(98)));
		//       enable_sym(97)       => 'e' 'n' 'a' 'b' 'l' 'e'.
		q(nt(97), (t(39)+t(12)+t(34)+t(10)+t(38)+t(39)));
		//       disable_sym(98)      => 'd' 'i' 's' 'a' 'b' 'l' 'e'.
		q(nt(98), (t(42)+t(36)+t(33)+t(34)+t(10)+t(38)+t(39)));
		//       __E_autodisambig_sym_37(99) => 'a' 'u' 't' 'o' sep(24).
		q(nt(99), (t(34)+t(43)+t(14)+t(44)+nt(24)));
		//       __E_autodisambig_sym_37(99) => null.
		q(nt(99), (nul));
		//       autodisambig_sym(96) => __E_autodisambig_sym_37(99) disambig_sym(100).
		q(nt(96), (nt(99)+nt(100)));
		//       __E___E_disambig_sym_38_39(102) => 'e'.
		q(nt(102), (t(39)));
		//       __E___E___E_disambig_sym_38_39_40(103) => 'o' 'n'.
		q(nt(103), (t(44)+t(12)));
		//       __E___E___E_disambig_sym_38_39_40(103) => 'n' 'g'.
		q(nt(103), (t(12)+t(47)));
		//       __E___E_disambig_sym_38_39(102) => 'i' __E___E___E_disambig_sym_38_39_40(103).
		q(nt(102), (t(36)+nt(103)));
		//       __E_disambig_sym_38(101) => 'u' 'a' 't' __E___E_disambig_sym_38_39(102).
		q(nt(101), (t(43)+t(34)+t(14)+nt(102)));
		//       __E_disambig_sym_38(101) => null.
		q(nt(101), (nul));
		//       disambig_sym(100)    => 'd' 'i' 's' 'a' 'm' 'b' 'i' 'g' __E_disambig_sym_38(101).
		q(nt(100), (t(42)+t(36)+t(33)+t(34)+t(37)+t(10)+t(36)+t(47)+nt(101)));
		//       __E_nodisambig_dir_41(105) => _(7) ',' _(7) sym(41).
		q(nt(105), (nt(7)+t(35)+nt(7)+nt(41)));
		//       __E_nodisambig_dir_42(106) => null.
		q(nt(106), (nul));
		//       __E_nodisambig_dir_42(106) => __E_nodisambig_dir_41(105) __E_nodisambig_dir_42(106).
		q(nt(106), (nt(105)+nt(106)));
		//       nodisambig_dir(72)   => nodisambig_sym(104) _(7) sym(41) __E_nodisambig_dir_42(106).
		q(nt(72), (nt(104)+nt(7)+nt(41)+nt(106)));
		//       __E___E_nodisambig_sym_43_44(108) => 'n'.
		q(nt(108), (t(12)));
		//       __E___E_nodisambig_sym_43_44(108) => null.
		q(nt(108), (nul));
		//       __E_nodisambig_sym_43(107) => 'n' 'o' __E___E_nodisambig_sym_43_44(108).
		q(nt(107), (t(12)+t(44)+nt(108)));
		//       __E___E_nodisambig_sym_43_45(109) => '\''.
		q(nt(109), (t(7)));
		//       __E___E_nodisambig_sym_43_45(109) => null.
		q(nt(109), (nul));
		//       __E_nodisambig_sym_43(107) => 'd' 'o' 'n' __E___E_nodisambig_sym_43_45(109) 't'.
		q(nt(107), (t(42)+t(44)+t(12)+nt(109)+t(14)));
		//       __E_nodisambig_sym_46(110) => sep(24) 'l' 'i' 's' 't'.
		q(nt(110), (nt(24)+t(38)+t(36)+t(33)+t(14)));
		//       __E_nodisambig_sym_46(110) => null.
		q(nt(110), (nul));
		//       nodisambig_sym(104)  => __E_nodisambig_sym_43(107) sep(24) disambig_sym(100) __E_nodisambig_sym_46(110).
		q(nt(104), (nt(107)+nt(24)+nt(100)+nt(110)));
		return q;
	}
};

#endif // __TGF_PARSER_H__
