// This file is generated from a file tools/tgf/tgf_repl.tgf by
//       https://github.com/IDNI/parser/tools/tgf
//
#ifndef __TGF_REPL_PARSER_H__
#define __TGF_REPL_PARSER_H__

#include <string.h>

#include "parser.h"

struct tgf_repl_parser {
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
	using sptree_type     = parser_type::psptree;
	using input_type      = parser_type::input;
	using decoder_type    = parser_type::input::decoder_type;
	using encoder_type    = std::function<std::basic_string<char_type>(
			const std::vector<terminal_type>&)>;
	tgf_repl_parser() :
		nts(load_nonterminals()), cc(load_cc()),
		g(nts, load_prods(), nt(9), cc, load_grammar_opts()),
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
	sptree_type shape(forest_type* f, const node_type& n) {
		idni::tree_shaping_options opt;
		opt.to_trim = g.opt.to_trim;
		opt.to_trim_children = g.opt.to_trim_children;
		opt.trim_terminals = g.opt.trim_terminals;
		opt.to_inline = g.opt.to_inline;
		opt.inline_char_classes = g.opt.inline_char_classes;
		return f->get_shaped_tree(n, opt);
	}
	sptree_type parse_and_shape(const char_type* data, size_t size,
		const node_type& n, parse_options po = {})
	{
		return shape(p.parse(data, size, po).get(), n);
	}
	sptree_type parse_and_shape(const char_type* data, size_t size,
		parse_options po = {})
	{
		auto f = p.parse(data, size, po);
		return shape(f.get(), f->root());
	}
	sptree_type parse_and_shape(std::basic_istream<char_type>& is,
		const node_type& n, parse_options po = {})
	{
		return shape(p.parse(is, po).get(), n);
	}
	sptree_type parse_and_shape(std::basic_istream<char_type>& is,
		parse_options po = {})
	{
		auto f = p.parse(is, po);
		return shape(f.get(), f->root());
	}
	sptree_type parse_and_shape(const std::string& fn,
		const node_type& n, parse_options po = {})
	{
		return shape(p.parse(fn, po).get(), n);
	}
	sptree_type parse_and_shape(const std::string& fn,
		parse_options po = {})
	{
		auto f = p.parse(fn, po);
		return shape(f.get(), f->root());
	}
#ifndef WIN32
	sptree_type parse_and_shape(int fd, const node_type& n, parse_options po = {})
	{
		return shape(p.parse(fd, po).get(), n);
	}
	sptree_type parse_and_shape(int fd, parse_options po = {})
	{
		auto f = p.parse(fd, po);
		return shape(f.get(), f->root());
	}
#endif //WIN32
	bool found(size_t start = SIZE_MAX) { return p.found(start); }
	typename parser_type::error get_error() { return p.get_error(); }
	enum nonterminal {
		nul, eof, alnum, alpha, space, printable, _, __, parse_input_char, start, 
		__E_start_0, statement, __E___E_start_0_1, __E___E_start_0_2, __E___3, __E____4, comment, __E_comment_5, __E_comment_6, __E_comment_7, 
		quoted_string, quoted_string_char, __E_quoted_string_8, unescaped_s, escaped_s, __E_unescaped_s_9, __E_unescaped_s_10, __E_escaped_s_11, filename, symbol, 
		__E_symbol_12, __E_symbol_13, __E_symbol_14, parse_input, parse_input_char_seq, __E_parse_input_char_seq_15, grammar_cmd, internal_grammar_cmd, unreachable_cmd, reload_cmd, 
		load_cmd, start_cmd, help, version, quit, clear, get, set, toggle, enable, 
		disable, add, del, parse_file_cmd, parse_cmd, parse_sym, parse_file_sym, grammar_sym, internal_grammar_sym, __E_internal_grammar_cmd_16, 
		start_sym, __E_start_cmd_17, unreachable_sym, __E_unreachable_cmd_18, reload_sym, load_sym, help_sym, __E_help_19, cmd_symbol, version_sym, 
		quit_sym, clear_sym, add_sym, list_option, symbol_list, del_sym, get_sym, __E_get_20, option, set_sym, 
		__E_set_21, option_value, toggle_sym, bool_option, enable_sym, disable_sym, error_verbosity_opt, status_opt, colors_opt, print_ambiguity_opt, 
		print_graphs_opt, print_rules_opt, print_facts_opt, print_terminals_opt, measure_parsing_opt, measure_each_pos_opt, measure_forest_opt, measure_preprocess_opt, debug_opt, auto_disambiguate_opt, 
		trim_terminals_opt, inline_cc_opt, nodisambig_list_opt, trim_opt, trim_children_opt, inline_opt, option_value_true, option_value_false, error_verbosity, basic_sym, 
		detailed_sym, root_cause_sym, __E_symbol_list_22, __E_symbol_list_23, __N_0, 
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
		'\0', '.', '\t', '\r', '\n', '#', '"', '\\', '/', 
		'b', 'f', 'n', 'r', 't', '_', '=', 'g', 'a', 'm', 
		'i', 'e', 'l', '-', 'u', 'c', 'h', 'p', 's', ' ', 
		'o', 'd', 'v', 'q', 'x', 'y', '1', '0', ',', 
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
			"", "eof", "alnum", "alpha", "space", "printable", "_", "__", "parse_input_char", "start", 
			"__E_start_0", "statement", "__E___E_start_0_1", "__E___E_start_0_2", "__E___3", "__E____4", "comment", "__E_comment_5", "__E_comment_6", "__E_comment_7", 
			"quoted_string", "quoted_string_char", "__E_quoted_string_8", "unescaped_s", "escaped_s", "__E_unescaped_s_9", "__E_unescaped_s_10", "__E_escaped_s_11", "filename", "symbol", 
			"__E_symbol_12", "__E_symbol_13", "__E_symbol_14", "parse_input", "parse_input_char_seq", "__E_parse_input_char_seq_15", "grammar_cmd", "internal_grammar_cmd", "unreachable_cmd", "reload_cmd", 
			"load_cmd", "start_cmd", "help", "version", "quit", "clear", "get", "set", "toggle", "enable", 
			"disable", "add", "del", "parse_file_cmd", "parse_cmd", "parse_sym", "parse_file_sym", "grammar_sym", "internal_grammar_sym", "__E_internal_grammar_cmd_16", 
			"start_sym", "__E_start_cmd_17", "unreachable_sym", "__E_unreachable_cmd_18", "reload_sym", "load_sym", "help_sym", "__E_help_19", "cmd_symbol", "version_sym", 
			"quit_sym", "clear_sym", "add_sym", "list_option", "symbol_list", "del_sym", "get_sym", "__E_get_20", "option", "set_sym", 
			"__E_set_21", "option_value", "toggle_sym", "bool_option", "enable_sym", "disable_sym", "error_verbosity_opt", "status_opt", "colors_opt", "print_ambiguity_opt", 
			"print_graphs_opt", "print_rules_opt", "print_facts_opt", "print_terminals_opt", "measure_parsing_opt", "measure_each_pos_opt", "measure_forest_opt", "measure_preprocess_opt", "debug_opt", "auto_disambiguate_opt", 
			"trim_terminals_opt", "inline_cc_opt", "nodisambig_list_opt", "trim_opt", "trim_children_opt", "inline_opt", "option_value_true", "option_value_false", "error_verbosity", "basic_sym", 
			"detailed_sym", "root_cause_sym", "__E_symbol_list_22", "__E_symbol_list_23", "__N_0", 
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
		o.trim_terminals = false;
		o.inline_char_classes = true;
		o.auto_disambiguate = true;
		o.to_trim = { 6, 7 };
		o.to_inline = { 8 };
		return o;
	}
	options load_opts() {
		options o;
		return o;
	}
	idni::prods<char_type, terminal_type> load_prods() {
		idni::prods<char_type, terminal_type> q,
			nul(symbol_type{});
		//       __E___E_start_0_1(12) => _(6) '.' _(6) statement(11).
		q(nt(12), (nt(6)+t(1)+nt(6)+nt(11)));
		//       __E___E_start_0_2(13) => null.
		q(nt(13), (nul));
		//       __E___E_start_0_2(13) => __E___E_start_0_1(12) __E___E_start_0_2(13).
		q(nt(13), (nt(12)+nt(13)));
		//       __E_start_0(10)      => statement(11) __E___E_start_0_2(13) _(6).
		q(nt(10), (nt(11)+nt(13)+nt(6)));
		//       __E_start_0(10)      => null.
		q(nt(10), (nul));
		//       start(9)             => _(6) __E_start_0(10).
		q(nt(9), (nt(6)+nt(10)));
		//       __E___3(14)          => __(7).
		q(nt(14), (nt(7)));
		//       __E___3(14)          => null.
		q(nt(14), (nul));
		//       _(6)                 => __E___3(14).
		q(nt(6), (nt(14)));
		//       __E____4(15)         => space(4).
		q(nt(15), (nt(4)));
		//       __E____4(15)         => comment(16).
		q(nt(15), (nt(16)));
		//       __(7)                => __E____4(15) _(6).
		q(nt(7), (nt(15)+nt(6)));
		//       __E_comment_5(17)    => printable(5).
		q(nt(17), (nt(5)));
		//       __E_comment_5(17)    => '\t'.
		q(nt(17), (t(2)));
		//       __E_comment_6(18)    => null.
		q(nt(18), (nul));
		//       __E_comment_6(18)    => __E_comment_5(17) __E_comment_6(18).
		q(nt(18), (nt(17)+nt(18)));
		//       __E_comment_7(19)    => '\r'.
		q(nt(19), (t(3)));
		//       __E_comment_7(19)    => '\n'.
		q(nt(19), (t(4)));
		//       __E_comment_7(19)    => eof(1).
		q(nt(19), (nt(1)));
		//       comment(16)          => '#' __E_comment_6(18) __E_comment_7(19).
		q(nt(16), (t(5)+nt(18)+nt(19)));
		//       __E_quoted_string_8(22) => null.
		q(nt(22), (nul));
		//       __E_quoted_string_8(22) => quoted_string_char(21) __E_quoted_string_8(22).
		q(nt(22), (nt(21)+nt(22)));
		//       quoted_string(20)    => '"' __E_quoted_string_8(22) '"'.
		q(nt(20), (t(6)+nt(22)+t(6)));
		//       quoted_string_char(21) => unescaped_s(23).
		q(nt(21), (nt(23)));
		//       quoted_string_char(21) => escaped_s(24).
		q(nt(21), (nt(24)));
		//       __E_unescaped_s_9(25) => space(4).
		q(nt(25), (nt(4)));
		//       __E_unescaped_s_9(25) => printable(5).
		q(nt(25), (nt(5)));
		//       __E_unescaped_s_10(26) => '"'.
		q(nt(26), (t(6)));
		//       __E_unescaped_s_10(26) => '\\'.
		q(nt(26), (t(7)));
		//       __N_0(114)           => __E_unescaped_s_10(26).
		q(nt(114), (nt(26)));
		//       unescaped_s(23)      => __E_unescaped_s_9(25) & ~( __N_0(114) ).	 # conjunctive
		q(nt(23), (nt(25)) & ~(nt(114)));
		//       __E_escaped_s_11(27) => '"'.
		q(nt(27), (t(6)));
		//       __E_escaped_s_11(27) => '\\'.
		q(nt(27), (t(7)));
		//       __E_escaped_s_11(27) => '/'.
		q(nt(27), (t(8)));
		//       __E_escaped_s_11(27) => 'b'.
		q(nt(27), (t(9)));
		//       __E_escaped_s_11(27) => 'f'.
		q(nt(27), (t(10)));
		//       __E_escaped_s_11(27) => 'n'.
		q(nt(27), (t(11)));
		//       __E_escaped_s_11(27) => 'r'.
		q(nt(27), (t(12)));
		//       __E_escaped_s_11(27) => 't'.
		q(nt(27), (t(13)));
		//       escaped_s(24)        => '\\' __E_escaped_s_11(27).
		q(nt(24), (t(7)+nt(27)));
		//       filename(28)         => quoted_string(20).
		q(nt(28), (nt(20)));
		//       __E_symbol_12(30)    => alpha(3).
		q(nt(30), (nt(3)));
		//       __E_symbol_12(30)    => '_'.
		q(nt(30), (t(14)));
		//       __E_symbol_13(31)    => alnum(2).
		q(nt(31), (nt(2)));
		//       __E_symbol_13(31)    => '_'.
		q(nt(31), (t(14)));
		//       __E_symbol_14(32)    => null.
		q(nt(32), (nul));
		//       __E_symbol_14(32)    => __E_symbol_13(31) __E_symbol_14(32).
		q(nt(32), (nt(31)+nt(32)));
		//       symbol(29)           => __E_symbol_12(30) __E_symbol_14(32).
		q(nt(29), (nt(30)+nt(32)));
		//       parse_input(33)      => quoted_string(20).
		q(nt(33), (nt(20)));
		//       parse_input(33)      => parse_input_char_seq(34).
		q(nt(33), (nt(34)));
		//       __E_parse_input_char_seq_15(35) => parse_input_char(8).
		q(nt(35), (nt(8)));
		//       __E_parse_input_char_seq_15(35) => parse_input_char(8) __E_parse_input_char_seq_15(35).
		q(nt(35), (nt(8)+nt(35)));
		//       parse_input_char_seq(34) => __E_parse_input_char_seq_15(35).
		q(nt(34), (nt(35)));
		//       parse_input_char(8)  => printable(5).
		q(nt(8), (nt(5)));
		//       parse_input_char(8)  => '\t'.
		q(nt(8), (t(2)));
		//       statement(11)        => grammar_cmd(36).
		q(nt(11), (nt(36)));
		//       statement(11)        => internal_grammar_cmd(37).
		q(nt(11), (nt(37)));
		//       statement(11)        => unreachable_cmd(38).
		q(nt(11), (nt(38)));
		//       statement(11)        => reload_cmd(39).
		q(nt(11), (nt(39)));
		//       statement(11)        => load_cmd(40).
		q(nt(11), (nt(40)));
		//       statement(11)        => start_cmd(41).
		q(nt(11), (nt(41)));
		//       statement(11)        => help(42).
		q(nt(11), (nt(42)));
		//       statement(11)        => version(43).
		q(nt(11), (nt(43)));
		//       statement(11)        => quit(44).
		q(nt(11), (nt(44)));
		//       statement(11)        => clear(45).
		q(nt(11), (nt(45)));
		//       statement(11)        => get(46).
		q(nt(11), (nt(46)));
		//       statement(11)        => set(47).
		q(nt(11), (nt(47)));
		//       statement(11)        => toggle(48).
		q(nt(11), (nt(48)));
		//       statement(11)        => enable(49).
		q(nt(11), (nt(49)));
		//       statement(11)        => disable(50).
		q(nt(11), (nt(50)));
		//       statement(11)        => add(51).
		q(nt(11), (nt(51)));
		//       statement(11)        => del(52).
		q(nt(11), (nt(52)));
		//       statement(11)        => parse_file_cmd(53).
		q(nt(11), (nt(53)));
		//       statement(11)        => parse_cmd(54).
		q(nt(11), (nt(54)));
		//       parse_cmd(54)        => parse_sym(55) __(7) parse_input(33).
		q(nt(54), (nt(55)+nt(7)+nt(33)));
		//       parse_file_cmd(53)   => parse_file_sym(56) __(7) filename(28).
		q(nt(53), (nt(56)+nt(7)+nt(28)));
		//       grammar_cmd(36)      => grammar_sym(57).
		q(nt(36), (nt(57)));
		//       __E_internal_grammar_cmd_16(59) => __(7) symbol(29).
		q(nt(59), (nt(7)+nt(29)));
		//       __E_internal_grammar_cmd_16(59) => null.
		q(nt(59), (nul));
		//       internal_grammar_cmd(37) => internal_grammar_sym(58) __E_internal_grammar_cmd_16(59).
		q(nt(37), (nt(58)+nt(59)));
		//       __E_start_cmd_17(61) => __(7) symbol(29).
		q(nt(61), (nt(7)+nt(29)));
		//       __E_start_cmd_17(61) => null.
		q(nt(61), (nul));
		//       start_cmd(41)        => start_sym(60) __E_start_cmd_17(61).
		q(nt(41), (nt(60)+nt(61)));
		//       __E_unreachable_cmd_18(63) => __(7) symbol(29).
		q(nt(63), (nt(7)+nt(29)));
		//       __E_unreachable_cmd_18(63) => null.
		q(nt(63), (nul));
		//       unreachable_cmd(38)  => unreachable_sym(62) __E_unreachable_cmd_18(63).
		q(nt(38), (nt(62)+nt(63)));
		//       reload_cmd(39)       => reload_sym(64).
		q(nt(39), (nt(64)));
		//       load_cmd(40)         => load_sym(65) __(7) filename(28).
		q(nt(40), (nt(65)+nt(7)+nt(28)));
		//       __E_help_19(67)      => __(7) cmd_symbol(68).
		q(nt(67), (nt(7)+nt(68)));
		//       __E_help_19(67)      => null.
		q(nt(67), (nul));
		//       help(42)             => help_sym(66) __E_help_19(67).
		q(nt(42), (nt(66)+nt(67)));
		//       version(43)          => version_sym(69).
		q(nt(43), (nt(69)));
		//       quit(44)             => quit_sym(70).
		q(nt(44), (nt(70)));
		//       clear(45)            => clear_sym(71).
		q(nt(45), (nt(71)));
		//       add(51)              => add_sym(72) __(7) list_option(73) __(7) symbol_list(74).
		q(nt(51), (nt(72)+nt(7)+nt(73)+nt(7)+nt(74)));
		//       del(52)              => del_sym(75) __(7) list_option(73) __(7) symbol_list(74).
		q(nt(52), (nt(75)+nt(7)+nt(73)+nt(7)+nt(74)));
		//       __E_get_20(77)       => __(7) option(78).
		q(nt(77), (nt(7)+nt(78)));
		//       __E_get_20(77)       => null.
		q(nt(77), (nul));
		//       get(46)              => get_sym(76) __E_get_20(77).
		q(nt(46), (nt(76)+nt(77)));
		//       __E_set_21(80)       => __(7).
		q(nt(80), (nt(7)));
		//       __E_set_21(80)       => _(6) '=' _(6).
		q(nt(80), (nt(6)+t(15)+nt(6)));
		//       set(47)              => set_sym(79) __(7) option(78) __E_set_21(80) option_value(81).
		q(nt(47), (nt(79)+nt(7)+nt(78)+nt(80)+nt(81)));
		//       toggle(48)           => toggle_sym(82) __(7) bool_option(83).
		q(nt(48), (nt(82)+nt(7)+nt(83)));
		//       enable(49)           => enable_sym(84) bool_option(83).
		q(nt(49), (nt(84)+nt(83)));
		//       disable(50)          => disable_sym(85) bool_option(83).
		q(nt(50), (nt(85)+nt(83)));
		//       grammar_sym(57)      => 'g'.
		q(nt(57), (t(16)));
		//       grammar_sym(57)      => 'g' 'r' 'a' 'm' 'm' 'a' 'r'.
		q(nt(57), (t(16)+t(12)+t(17)+t(18)+t(18)+t(17)+t(12)));
		//       internal_grammar_sym(58) => 'i'.
		q(nt(58), (t(19)));
		//       internal_grammar_sym(58) => 'i' 'g'.
		q(nt(58), (t(19)+t(16)));
		//       internal_grammar_sym(58) => 'i' 'n' 't' 'e' 'r' 'n' 'a' 'l' '-' 'g' 'r' 'a' 'm' 'm' 'a' 'r'.
		q(nt(58), (t(19)+t(11)+t(13)+t(20)+t(12)+t(11)+t(17)+t(21)+t(22)+t(16)+t(12)+t(17)+t(18)+t(18)+t(17)+t(12)));
		//       unreachable_sym(62)  => 'u'.
		q(nt(62), (t(23)));
		//       unreachable_sym(62)  => 'u' 'n' 'r' 'e' 'a' 'c' 'h' 'a' 'b' 'l' 'e'.
		q(nt(62), (t(23)+t(11)+t(12)+t(20)+t(17)+t(24)+t(25)+t(17)+t(9)+t(21)+t(20)));
		//       parse_sym(55)        => 'p'.
		q(nt(55), (t(26)));
		//       parse_sym(55)        => 'p' 'a' 'r' 's' 'e'.
		q(nt(55), (t(26)+t(17)+t(12)+t(27)+t(20)));
		//       parse_file_sym(56)   => 'f'.
		q(nt(56), (t(10)));
		//       parse_file_sym(56)   => 'p' 'f'.
		q(nt(56), (t(26)+t(10)));
		//       parse_file_sym(56)   => 'p' 'a' 'r' 's' 'e' ' ' 'f' 'i' 'l' 'e'.
		q(nt(56), (t(26)+t(17)+t(12)+t(27)+t(20)+t(28)+t(10)+t(19)+t(21)+t(20)));
		//       start_sym(60)        => 's'.
		q(nt(60), (t(27)));
		//       start_sym(60)        => 's' 't' 'a' 'r' 't'.
		q(nt(60), (t(27)+t(13)+t(17)+t(12)+t(13)));
		//       reload_sym(64)       => 'r'.
		q(nt(64), (t(12)));
		//       reload_sym(64)       => 'r' 'e' 'l' 'o' 'a' 'd'.
		q(nt(64), (t(12)+t(20)+t(21)+t(29)+t(17)+t(30)));
		//       load_sym(65)         => 'l'.
		q(nt(65), (t(21)));
		//       load_sym(65)         => 'l' 'o' 'a' 'd'.
		q(nt(65), (t(21)+t(29)+t(17)+t(30)));
		//       help_sym(66)         => 'h'.
		q(nt(66), (t(25)));
		//       help_sym(66)         => 'h' 'e' 'l' 'p'.
		q(nt(66), (t(25)+t(20)+t(21)+t(26)));
		//       version_sym(69)      => 'v'.
		q(nt(69), (t(31)));
		//       version_sym(69)      => 'v' 'e' 'r' 's' 'i' 'o' 'n'.
		q(nt(69), (t(31)+t(20)+t(12)+t(27)+t(19)+t(29)+t(11)));
		//       quit_sym(70)         => 'q'.
		q(nt(70), (t(32)));
		//       quit_sym(70)         => 'q' 'u' 'i' 't'.
		q(nt(70), (t(32)+t(23)+t(19)+t(13)));
		//       quit_sym(70)         => 'e'.
		q(nt(70), (t(20)));
		//       quit_sym(70)         => 'e' 'x' 'i' 't'.
		q(nt(70), (t(20)+t(33)+t(19)+t(13)));
		//       clear_sym(71)        => 'c' 'l' 's'.
		q(nt(71), (t(24)+t(21)+t(27)));
		//       clear_sym(71)        => 'c' 'l' 'e' 'a' 'r'.
		q(nt(71), (t(24)+t(21)+t(20)+t(17)+t(12)));
		//       get_sym(76)          => 'g' 'e' 't'.
		q(nt(76), (t(16)+t(20)+t(13)));
		//       set_sym(79)          => 's' 'e' 't'.
		q(nt(79), (t(27)+t(20)+t(13)));
		//       toggle_sym(82)       => 't' 'o' 'g'.
		q(nt(82), (t(13)+t(29)+t(16)));
		//       toggle_sym(82)       => 't' 'o' 'g' 'g' 'l' 'e'.
		q(nt(82), (t(13)+t(29)+t(16)+t(16)+t(21)+t(20)));
		//       add_sym(72)          => 'a' 'd' 'd'.
		q(nt(72), (t(17)+t(30)+t(30)));
		//       del_sym(75)          => 'd' 'e' 'l'.
		q(nt(75), (t(30)+t(20)+t(21)));
		//       del_sym(75)          => 'd' 'e' 'l' 'e' 't' 'e'.
		q(nt(75), (t(30)+t(20)+t(21)+t(20)+t(13)+t(20)));
		//       del_sym(75)          => 'r' 'm'.
		q(nt(75), (t(12)+t(18)));
		//       del_sym(75)          => 'r' 'e' 'm'.
		q(nt(75), (t(12)+t(20)+t(18)));
		//       del_sym(75)          => 'r' 'e' 'm' 'o' 'v' 'e'.
		q(nt(75), (t(12)+t(20)+t(18)+t(29)+t(31)+t(20)));
		//       enable_sym(84)       => 'e' 'n' __(7).
		q(nt(84), (t(20)+t(11)+nt(7)));
		//       enable_sym(84)       => 'e' 'n' 'a' 'b' 'l' 'e' __(7).
		q(nt(84), (t(20)+t(11)+t(17)+t(9)+t(21)+t(20)+nt(7)));
		//       disable_sym(85)      => 'd' 'i' 's' __(7).
		q(nt(85), (t(30)+t(19)+t(27)+nt(7)));
		//       disable_sym(85)      => 'd' 'i' 's' 'a' 'b' 'l' 'e' __(7).
		q(nt(85), (t(30)+t(19)+t(27)+t(17)+t(9)+t(21)+t(20)+nt(7)));
		//       cmd_symbol(68)       => grammar_sym(57).
		q(nt(68), (nt(57)));
		//       cmd_symbol(68)       => internal_grammar_sym(58).
		q(nt(68), (nt(58)));
		//       cmd_symbol(68)       => unreachable_sym(62).
		q(nt(68), (nt(62)));
		//       cmd_symbol(68)       => start_sym(60).
		q(nt(68), (nt(60)));
		//       cmd_symbol(68)       => parse_sym(55).
		q(nt(68), (nt(55)));
		//       cmd_symbol(68)       => parse_file_sym(56).
		q(nt(68), (nt(56)));
		//       cmd_symbol(68)       => load_sym(65).
		q(nt(68), (nt(65)));
		//       cmd_symbol(68)       => reload_sym(64).
		q(nt(68), (nt(64)));
		//       cmd_symbol(68)       => help_sym(66).
		q(nt(68), (nt(66)));
		//       cmd_symbol(68)       => quit_sym(70).
		q(nt(68), (nt(70)));
		//       cmd_symbol(68)       => version_sym(69).
		q(nt(68), (nt(69)));
		//       cmd_symbol(68)       => clear_sym(71).
		q(nt(68), (nt(71)));
		//       cmd_symbol(68)       => get_sym(76).
		q(nt(68), (nt(76)));
		//       cmd_symbol(68)       => set_sym(79).
		q(nt(68), (nt(79)));
		//       cmd_symbol(68)       => add_sym(72).
		q(nt(68), (nt(72)));
		//       cmd_symbol(68)       => del_sym(75).
		q(nt(68), (nt(75)));
		//       cmd_symbol(68)       => toggle_sym(82).
		q(nt(68), (nt(82)));
		//       cmd_symbol(68)       => enable_sym(84).
		q(nt(68), (nt(84)));
		//       cmd_symbol(68)       => disable_sym(85).
		q(nt(68), (nt(85)));
		//       option(78)           => bool_option(83).
		q(nt(78), (nt(83)));
		//       option(78)           => error_verbosity_opt(86).
		q(nt(78), (nt(86)));
		//       option(78)           => list_option(73).
		q(nt(78), (nt(73)));
		//       bool_option(83)      => status_opt(87).
		q(nt(83), (nt(87)));
		//       bool_option(83)      => colors_opt(88).
		q(nt(83), (nt(88)));
		//       bool_option(83)      => print_ambiguity_opt(89).
		q(nt(83), (nt(89)));
		//       bool_option(83)      => print_graphs_opt(90).
		q(nt(83), (nt(90)));
		//       bool_option(83)      => print_rules_opt(91).
		q(nt(83), (nt(91)));
		//       bool_option(83)      => print_facts_opt(92).
		q(nt(83), (nt(92)));
		//       bool_option(83)      => print_terminals_opt(93).
		q(nt(83), (nt(93)));
		//       bool_option(83)      => measure_parsing_opt(94).
		q(nt(83), (nt(94)));
		//       bool_option(83)      => measure_each_pos_opt(95).
		q(nt(83), (nt(95)));
		//       bool_option(83)      => measure_forest_opt(96).
		q(nt(83), (nt(96)));
		//       bool_option(83)      => measure_preprocess_opt(97).
		q(nt(83), (nt(97)));
		//       bool_option(83)      => debug_opt(98).
		q(nt(83), (nt(98)));
		//       bool_option(83)      => auto_disambiguate_opt(99).
		q(nt(83), (nt(99)));
		//       bool_option(83)      => trim_terminals_opt(100).
		q(nt(83), (nt(100)));
		//       bool_option(83)      => inline_cc_opt(101).
		q(nt(83), (nt(101)));
		//       list_option(73)      => nodisambig_list_opt(102).
		q(nt(73), (nt(102)));
		//       list_option(73)      => trim_opt(103).
		q(nt(73), (nt(103)));
		//       list_option(73)      => trim_children_opt(104).
		q(nt(73), (nt(104)));
		//       list_option(73)      => inline_opt(105).
		q(nt(73), (nt(105)));
		//       debug_opt(98)        => 'd'.
		q(nt(98), (t(30)));
		//       debug_opt(98)        => 'd' 'e' 'b' 'u' 'g'.
		q(nt(98), (t(30)+t(20)+t(9)+t(23)+t(16)));
		//       status_opt(87)       => 's'.
		q(nt(87), (t(27)));
		//       status_opt(87)       => 's' 't' 'a' 't' 'u' 's'.
		q(nt(87), (t(27)+t(13)+t(17)+t(13)+t(23)+t(27)));
		//       colors_opt(88)       => 'c'.
		q(nt(88), (t(24)));
		//       colors_opt(88)       => 'c' 'o' 'l' 'o' 'r'.
		q(nt(88), (t(24)+t(29)+t(21)+t(29)+t(12)));
		//       colors_opt(88)       => 'c' 'o' 'l' 'o' 'r' 's'.
		q(nt(88), (t(24)+t(29)+t(21)+t(29)+t(12)+t(27)));
		//       print_ambiguity_opt(89) => 'a'.
		q(nt(89), (t(17)));
		//       print_ambiguity_opt(89) => 'a' 'm' 'b' 'i' 'g' 'u' 'i' 't' 'y'.
		q(nt(89), (t(17)+t(18)+t(9)+t(19)+t(16)+t(23)+t(19)+t(13)+t(34)));
		//       print_ambiguity_opt(89) => 'p' 'r' 'i' 'n' 't' '-' 'a' 'm' 'b' 'i' 'g' 'u' 'i' 't' 'y'.
		q(nt(89), (t(26)+t(12)+t(19)+t(11)+t(13)+t(22)+t(17)+t(18)+t(9)+t(19)+t(16)+t(23)+t(19)+t(13)+t(34)));
		//       print_terminals_opt(93) => 't'.
		q(nt(93), (t(13)));
		//       print_terminals_opt(93) => 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
		q(nt(93), (t(13)+t(20)+t(12)+t(18)+t(19)+t(11)+t(17)+t(21)+t(27)));
		//       print_terminals_opt(93) => 'p' 'r' 'i' 'n' 't' '-' 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
		q(nt(93), (t(26)+t(12)+t(19)+t(11)+t(13)+t(22)+t(13)+t(20)+t(12)+t(18)+t(19)+t(11)+t(17)+t(21)+t(27)));
		//       print_graphs_opt(90) => 'g'.
		q(nt(90), (t(16)));
		//       print_graphs_opt(90) => 'g' 'r' 'a' 'p' 'h' 's'.
		q(nt(90), (t(16)+t(12)+t(17)+t(26)+t(25)+t(27)));
		//       print_graphs_opt(90) => 'p' 'r' 'i' 'n' 't' '-' 'g' 'r' 'a' 'p' 'h' 's'.
		q(nt(90), (t(26)+t(12)+t(19)+t(11)+t(13)+t(22)+t(16)+t(12)+t(17)+t(26)+t(25)+t(27)));
		//       print_rules_opt(91)  => 'r'.
		q(nt(91), (t(12)));
		//       print_rules_opt(91)  => 'r' 'u' 'l' 'e' 's'.
		q(nt(91), (t(12)+t(23)+t(21)+t(20)+t(27)));
		//       print_rules_opt(91)  => 'p' 'r' 'i' 'n' 't' '-' 'r' 'u' 'l' 'e' 's'.
		q(nt(91), (t(26)+t(12)+t(19)+t(11)+t(13)+t(22)+t(12)+t(23)+t(21)+t(20)+t(27)));
		//       print_facts_opt(92)  => 'f'.
		q(nt(92), (t(10)));
		//       print_facts_opt(92)  => 'f' 'a' 'c' 't' 's'.
		q(nt(92), (t(10)+t(17)+t(24)+t(13)+t(27)));
		//       print_facts_opt(92)  => 'p' 'r' 'i' 'n' 't' '-' 'f' 'a' 'c' 't' 's'.
		q(nt(92), (t(26)+t(12)+t(19)+t(11)+t(13)+t(22)+t(10)+t(17)+t(24)+t(13)+t(27)));
		//       measure_parsing_opt(94) => 'm'.
		q(nt(94), (t(18)));
		//       measure_parsing_opt(94) => 'm' 'e' 'a' 's' 'u' 'r' 'e'.
		q(nt(94), (t(18)+t(20)+t(17)+t(27)+t(23)+t(12)+t(20)));
		//       measure_parsing_opt(94) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'p' 'a' 'r' 's' 'i' 'n' 'g'.
		q(nt(94), (t(18)+t(20)+t(17)+t(27)+t(23)+t(12)+t(20)+t(22)+t(26)+t(17)+t(12)+t(27)+t(19)+t(11)+t(16)));
		//       measure_each_pos_opt(95) => 'm' 'e'.
		q(nt(95), (t(18)+t(20)));
		//       measure_each_pos_opt(95) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'e' 'a' 'c' 'h'.
		q(nt(95), (t(18)+t(20)+t(17)+t(27)+t(23)+t(12)+t(20)+t(22)+t(20)+t(17)+t(24)+t(25)));
		//       measure_each_pos_opt(95) => 'm' 'e' 'p'.
		q(nt(95), (t(18)+t(20)+t(26)));
		//       measure_each_pos_opt(95) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'e' 'a' 'c' 'h' '-' 'p' 'o' 's'.
		q(nt(95), (t(18)+t(20)+t(17)+t(27)+t(23)+t(12)+t(20)+t(22)+t(20)+t(17)+t(24)+t(25)+t(22)+t(26)+t(29)+t(27)));
		//       measure_forest_opt(96) => 'm' 'f'.
		q(nt(96), (t(18)+t(10)));
		//       measure_forest_opt(96) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'f' 'o' 'r' 'e' 's' 't'.
		q(nt(96), (t(18)+t(20)+t(17)+t(27)+t(23)+t(12)+t(20)+t(22)+t(10)+t(29)+t(12)+t(20)+t(27)+t(13)));
		//       measure_preprocess_opt(97) => 'm' 'p'.
		q(nt(97), (t(18)+t(26)));
		//       measure_preprocess_opt(97) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'p' 'r' 'e' 'p' 'r' 'o' 'c' 'e' 's' 's'.
		q(nt(97), (t(18)+t(20)+t(17)+t(27)+t(23)+t(12)+t(20)+t(22)+t(26)+t(12)+t(20)+t(26)+t(12)+t(29)+t(24)+t(20)+t(27)+t(27)));
		//       auto_disambiguate_opt(99) => 'a' 'd'.
		q(nt(99), (t(17)+t(30)));
		//       auto_disambiguate_opt(99) => 'a' 'u' 't' 'o' '-' 'd' 'i' 's' 'a' 'm' 'b' 'i' 'g' 'u' 'a' 't' 'e'.
		q(nt(99), (t(17)+t(23)+t(13)+t(29)+t(22)+t(30)+t(19)+t(27)+t(17)+t(18)+t(9)+t(19)+t(16)+t(23)+t(17)+t(13)+t(20)));
		//       error_verbosity_opt(86) => 'e'.
		q(nt(86), (t(20)));
		//       error_verbosity_opt(86) => 'e' 'r' 'r' 'o' 'r' '-' 'v' 'e' 'r' 'b' 'o' 's' 'i' 't' 'y'.
		q(nt(86), (t(20)+t(12)+t(12)+t(29)+t(12)+t(22)+t(31)+t(20)+t(12)+t(9)+t(29)+t(27)+t(19)+t(13)+t(34)));
		//       nodisambig_list_opt(102) => 'n' 'd'.
		q(nt(102), (t(11)+t(30)));
		//       nodisambig_list_opt(102) => 'n' 'd' 'l'.
		q(nt(102), (t(11)+t(30)+t(21)));
		//       nodisambig_list_opt(102) => 'n' 'o' 'd' 'i' 's' 'a' 'm' 'b' 'i' 'g' '-' 'l' 'i' 's' 't'.
		q(nt(102), (t(11)+t(29)+t(30)+t(19)+t(27)+t(17)+t(18)+t(9)+t(19)+t(16)+t(22)+t(21)+t(19)+t(27)+t(13)));
		//       trim_terminals_opt(100) => 't' 't'.
		q(nt(100), (t(13)+t(13)));
		//       trim_terminals_opt(100) => 't' 'r' 'i' 'm' '-' 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
		q(nt(100), (t(13)+t(12)+t(19)+t(18)+t(22)+t(13)+t(20)+t(12)+t(18)+t(19)+t(11)+t(17)+t(21)+t(27)));
		//       trim_opt(103)        => 't' 'r' 'i' 'm'.
		q(nt(103), (t(13)+t(12)+t(19)+t(18)));
		//       trim_children_opt(104) => 't' 'c'.
		q(nt(104), (t(13)+t(24)));
		//       trim_children_opt(104) => 't' 'r' 'i' 'm' '-' 'c' 'h' 'i' 'l' 'd' 'r' 'e' 'n'.
		q(nt(104), (t(13)+t(12)+t(19)+t(18)+t(22)+t(24)+t(25)+t(19)+t(21)+t(30)+t(12)+t(20)+t(11)));
		//       inline_cc_opt(101)   => 'i' 'c' 'c'.
		q(nt(101), (t(19)+t(24)+t(24)));
		//       inline_cc_opt(101)   => 'i' 'n' 'l' 'i' 'n' 'e' '-' 'c' 'c'.
		q(nt(101), (t(19)+t(11)+t(21)+t(19)+t(11)+t(20)+t(22)+t(24)+t(24)));
		//       inline_cc_opt(101)   => 'i' 'n' 'l' 'i' 'n' 'e' '-' 'c' 'h' 'a' 'r' '-' 'c' 'l' 'a' 's' 's' 'e' 's'.
		q(nt(101), (t(19)+t(11)+t(21)+t(19)+t(11)+t(20)+t(22)+t(24)+t(25)+t(17)+t(12)+t(22)+t(24)+t(21)+t(17)+t(27)+t(27)+t(20)+t(27)));
		//       inline_opt(105)      => 'i'.
		q(nt(105), (t(19)));
		//       inline_opt(105)      => 'i' 'n' 'l' 'i' 'n' 'e'.
		q(nt(105), (t(19)+t(11)+t(21)+t(19)+t(11)+t(20)));
		//       option_value(81)     => option_value_true(106).
		q(nt(81), (nt(106)));
		//       option_value(81)     => option_value_false(107).
		q(nt(81), (nt(107)));
		//       option_value(81)     => error_verbosity(108).
		q(nt(81), (nt(108)));
		//       option_value(81)     => symbol_list(74).
		q(nt(81), (nt(74)));
		//       option_value_true(106) => 't'.
		q(nt(106), (t(13)));
		//       option_value_true(106) => 't' 'r' 'u' 'e'.
		q(nt(106), (t(13)+t(12)+t(23)+t(20)));
		//       option_value_true(106) => 'o' 'n'.
		q(nt(106), (t(29)+t(11)));
		//       option_value_true(106) => '1'.
		q(nt(106), (t(35)));
		//       option_value_true(106) => 'y'.
		q(nt(106), (t(34)));
		//       option_value_true(106) => 'y' 'e' 's'.
		q(nt(106), (t(34)+t(20)+t(27)));
		//       option_value_false(107) => 'f'.
		q(nt(107), (t(10)));
		//       option_value_false(107) => 'f' 'a' 'l' 's' 'e'.
		q(nt(107), (t(10)+t(17)+t(21)+t(27)+t(20)));
		//       option_value_false(107) => 'o' 'f' 'f'.
		q(nt(107), (t(29)+t(10)+t(10)));
		//       option_value_false(107) => '0'.
		q(nt(107), (t(36)));
		//       option_value_false(107) => 'n'.
		q(nt(107), (t(11)));
		//       option_value_false(107) => 'n' 'o'.
		q(nt(107), (t(11)+t(29)));
		//       error_verbosity(108) => basic_sym(109).
		q(nt(108), (nt(109)));
		//       error_verbosity(108) => detailed_sym(110).
		q(nt(108), (nt(110)));
		//       error_verbosity(108) => root_cause_sym(111).
		q(nt(108), (nt(111)));
		//       basic_sym(109)       => 'b'.
		q(nt(109), (t(9)));
		//       basic_sym(109)       => 'b' 'a' 's' 'i' 'c'.
		q(nt(109), (t(9)+t(17)+t(27)+t(19)+t(24)));
		//       detailed_sym(110)    => 'd'.
		q(nt(110), (t(30)));
		//       detailed_sym(110)    => 'd' 'e' 't' 'a' 'i' 'l' 'e' 'd'.
		q(nt(110), (t(30)+t(20)+t(13)+t(17)+t(19)+t(21)+t(20)+t(30)));
		//       root_cause_sym(111)  => 'r'.
		q(nt(111), (t(12)));
		//       root_cause_sym(111)  => 'r' 'c'.
		q(nt(111), (t(12)+t(24)));
		//       root_cause_sym(111)  => 'r' 'o' 'o' 't' '-' 'c' 'a' 'u' 's' 'e'.
		q(nt(111), (t(12)+t(29)+t(29)+t(13)+t(22)+t(24)+t(17)+t(23)+t(27)+t(20)));
		//       __E_symbol_list_22(112) => _(6) ',' _(6) symbol(29).
		q(nt(112), (nt(6)+t(37)+nt(6)+nt(29)));
		//       __E_symbol_list_23(113) => null.
		q(nt(113), (nul));
		//       __E_symbol_list_23(113) => __E_symbol_list_22(112) __E_symbol_list_23(113).
		q(nt(113), (nt(112)+nt(113)));
		//       symbol_list(74)      => symbol(29) __E_symbol_list_23(113).
		q(nt(74), (nt(29)+nt(113)));
		return q;
	}
};

#endif // __TGF_REPL_PARSER_H__
