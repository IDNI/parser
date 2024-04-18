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
		disable, add, del, parse_file_cmd, parse_cmd, parse_sym, __E_parse_cmd_16, parse_file_sym, __E_parse_file_cmd_17, grammar_sym, 
		__E_grammar_cmd_18, internal_grammar_sym, __E_internal_grammar_cmd_19, __E_internal_grammar_cmd_20, start_sym, __E_start_cmd_21, __E_start_cmd_22, unreachable_sym, __E_unreachable_cmd_23, __E_unreachable_cmd_24, 
		reload_sym, __E_reload_cmd_25, load_sym, __E_load_cmd_26, help_sym, __E_help_27, __E_help_28, cmd_symbol, version_sym, __E_version_29, 
		quit_sym, __E_quit_30, clear_sym, __E_clear_31, add_sym, list_option, symbol_list, del_sym, __E_del_32, get_sym, 
		__E_get_33, option, set_sym, __E_set_34, option_value, toggle_sym, __E_toggle_35, bool_option, enable_sym, __E_enable_36, 
		disable_sym, __E_disable_37, error_verbosity_opt, __E_option_38, status_opt, __E_bool_option_39, colors_opt, __E_bool_option_40, print_ambiguity_opt, __E_bool_option_41, 
		print_graphs_opt, __E_bool_option_42, print_rules_opt, __E_bool_option_43, print_facts_opt, __E_bool_option_44, print_terminals_opt, __E_bool_option_45, measure_parsing_opt, __E_bool_option_46, 
		measure_each_pos_opt, __E_bool_option_47, measure_forest_opt, __E_bool_option_48, measure_preprocess_opt, __E_bool_option_49, debug_opt, __E_bool_option_50, auto_disambiguate_opt, __E_bool_option_51, 
		trim_terminals_opt, __E_bool_option_52, inline_cc_opt, __E_bool_option_53, nodisambig_list_opt, __E_list_option_54, trim_opt, trim_children_opt, __E_list_option_55, inline_opt, 
		__E_list_option_56, option_value_true, option_value_false, error_verbosity, basic_sym, __E_error_verbosity_57, detailed_sym, __E_error_verbosity_58, root_cause_sym, __E_error_verbosity_59, 
		__E_symbol_list_60, __E_symbol_list_61, __N_0, 
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
		'b', 'f', 'n', 'r', 't', '_', 'p', 'a', 's', 'e', 
		' ', 'i', 'l', 'g', 'm', '-', 'u', 'c', 'h', 'o', 
		'd', 'v', 'q', 'x', '=', 'y', '1', '0', ',', 
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
			"disable", "add", "del", "parse_file_cmd", "parse_cmd", "parse_sym", "__E_parse_cmd_16", "parse_file_sym", "__E_parse_file_cmd_17", "grammar_sym", 
			"__E_grammar_cmd_18", "internal_grammar_sym", "__E_internal_grammar_cmd_19", "__E_internal_grammar_cmd_20", "start_sym", "__E_start_cmd_21", "__E_start_cmd_22", "unreachable_sym", "__E_unreachable_cmd_23", "__E_unreachable_cmd_24", 
			"reload_sym", "__E_reload_cmd_25", "load_sym", "__E_load_cmd_26", "help_sym", "__E_help_27", "__E_help_28", "cmd_symbol", "version_sym", "__E_version_29", 
			"quit_sym", "__E_quit_30", "clear_sym", "__E_clear_31", "add_sym", "list_option", "symbol_list", "del_sym", "__E_del_32", "get_sym", 
			"__E_get_33", "option", "set_sym", "__E_set_34", "option_value", "toggle_sym", "__E_toggle_35", "bool_option", "enable_sym", "__E_enable_36", 
			"disable_sym", "__E_disable_37", "error_verbosity_opt", "__E_option_38", "status_opt", "__E_bool_option_39", "colors_opt", "__E_bool_option_40", "print_ambiguity_opt", "__E_bool_option_41", 
			"print_graphs_opt", "__E_bool_option_42", "print_rules_opt", "__E_bool_option_43", "print_facts_opt", "__E_bool_option_44", "print_terminals_opt", "__E_bool_option_45", "measure_parsing_opt", "__E_bool_option_46", 
			"measure_each_pos_opt", "__E_bool_option_47", "measure_forest_opt", "__E_bool_option_48", "measure_preprocess_opt", "__E_bool_option_49", "debug_opt", "__E_bool_option_50", "auto_disambiguate_opt", "__E_bool_option_51", 
			"trim_terminals_opt", "__E_bool_option_52", "inline_cc_opt", "__E_bool_option_53", "nodisambig_list_opt", "__E_list_option_54", "trim_opt", "trim_children_opt", "__E_list_option_55", "inline_opt", 
			"__E_list_option_56", "option_value_true", "option_value_false", "error_verbosity", "basic_sym", "__E_error_verbosity_57", "detailed_sym", "__E_error_verbosity_58", "root_cause_sym", "__E_error_verbosity_59", 
			"__E_symbol_list_60", "__E_symbol_list_61", "__N_0", 
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
		//       __N_0(152)           => __E_unescaped_s_10(26).
		q(nt(152), (nt(26)));
		//       unescaped_s(23)      => __E_unescaped_s_9(25) & ~( __N_0(152) ).	 # conjunctive
		q(nt(23), (nt(25)) & ~(nt(152)));
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
		//       __E_parse_cmd_16(56) => 'p'.
		q(nt(56), (t(15)));
		//       __E_parse_cmd_16(56) => 'p' 'a' 'r' 's' 'e'.
		q(nt(56), (t(15)+t(16)+t(12)+t(17)+t(18)));
		//       parse_sym(55)        => __E_parse_cmd_16(56).
		q(nt(55), (nt(56)));
		//       parse_cmd(54)        => parse_sym(55) __(7) parse_input(33).
		q(nt(54), (nt(55)+nt(7)+nt(33)));
		//       __E_parse_file_cmd_17(58) => 'f'.
		q(nt(58), (t(10)));
		//       __E_parse_file_cmd_17(58) => 'p' 'f'.
		q(nt(58), (t(15)+t(10)));
		//       __E_parse_file_cmd_17(58) => 'p' 'a' 'r' 's' 'e' ' ' 'f' 'i' 'l' 'e'.
		q(nt(58), (t(15)+t(16)+t(12)+t(17)+t(18)+t(19)+t(10)+t(20)+t(21)+t(18)));
		//       parse_file_sym(57)   => __E_parse_file_cmd_17(58).
		q(nt(57), (nt(58)));
		//       parse_file_cmd(53)   => parse_file_sym(57) __(7) filename(28).
		q(nt(53), (nt(57)+nt(7)+nt(28)));
		//       __E_grammar_cmd_18(60) => 'g'.
		q(nt(60), (t(22)));
		//       __E_grammar_cmd_18(60) => 'g' 'r' 'a' 'm' 'm' 'a' 'r'.
		q(nt(60), (t(22)+t(12)+t(16)+t(23)+t(23)+t(16)+t(12)));
		//       grammar_sym(59)      => __E_grammar_cmd_18(60).
		q(nt(59), (nt(60)));
		//       grammar_cmd(36)      => grammar_sym(59).
		q(nt(36), (nt(59)));
		//       __E_internal_grammar_cmd_19(62) => 'i'.
		q(nt(62), (t(20)));
		//       __E_internal_grammar_cmd_19(62) => 'i' 'g'.
		q(nt(62), (t(20)+t(22)));
		//       __E_internal_grammar_cmd_19(62) => 'i' 'n' 't' 'e' 'r' 'n' 'a' 'l' '-' 'g' 'r' 'a' 'm' 'm' 'a' 'r'.
		q(nt(62), (t(20)+t(11)+t(13)+t(18)+t(12)+t(11)+t(16)+t(21)+t(24)+t(22)+t(12)+t(16)+t(23)+t(23)+t(16)+t(12)));
		//       internal_grammar_sym(61) => __E_internal_grammar_cmd_19(62).
		q(nt(61), (nt(62)));
		//       __E_internal_grammar_cmd_20(63) => __(7) symbol(29).
		q(nt(63), (nt(7)+nt(29)));
		//       __E_internal_grammar_cmd_20(63) => null.
		q(nt(63), (nul));
		//       internal_grammar_cmd(37) => internal_grammar_sym(61) __E_internal_grammar_cmd_20(63).
		q(nt(37), (nt(61)+nt(63)));
		//       __E_start_cmd_21(65) => 's'.
		q(nt(65), (t(17)));
		//       __E_start_cmd_21(65) => 's' 't' 'a' 'r' 't'.
		q(nt(65), (t(17)+t(13)+t(16)+t(12)+t(13)));
		//       start_sym(64)        => __E_start_cmd_21(65).
		q(nt(64), (nt(65)));
		//       __E_start_cmd_22(66) => __(7) symbol(29).
		q(nt(66), (nt(7)+nt(29)));
		//       __E_start_cmd_22(66) => null.
		q(nt(66), (nul));
		//       start_cmd(41)        => start_sym(64) __E_start_cmd_22(66).
		q(nt(41), (nt(64)+nt(66)));
		//       __E_unreachable_cmd_23(68) => 'u'.
		q(nt(68), (t(25)));
		//       __E_unreachable_cmd_23(68) => 'u' 'n' 'r' 'e' 'a' 'c' 'h' 'a' 'b' 'l' 'e'.
		q(nt(68), (t(25)+t(11)+t(12)+t(18)+t(16)+t(26)+t(27)+t(16)+t(9)+t(21)+t(18)));
		//       unreachable_sym(67)  => __E_unreachable_cmd_23(68).
		q(nt(67), (nt(68)));
		//       __E_unreachable_cmd_24(69) => __(7) symbol(29).
		q(nt(69), (nt(7)+nt(29)));
		//       __E_unreachable_cmd_24(69) => null.
		q(nt(69), (nul));
		//       unreachable_cmd(38)  => unreachable_sym(67) __E_unreachable_cmd_24(69).
		q(nt(38), (nt(67)+nt(69)));
		//       __E_reload_cmd_25(71) => 'r'.
		q(nt(71), (t(12)));
		//       __E_reload_cmd_25(71) => 'r' 'e' 'l' 'o' 'a' 'd'.
		q(nt(71), (t(12)+t(18)+t(21)+t(28)+t(16)+t(29)));
		//       reload_sym(70)       => __E_reload_cmd_25(71).
		q(nt(70), (nt(71)));
		//       reload_cmd(39)       => reload_sym(70).
		q(nt(39), (nt(70)));
		//       __E_load_cmd_26(73)  => 'l'.
		q(nt(73), (t(21)));
		//       __E_load_cmd_26(73)  => 'l' 'o' 'a' 'd'.
		q(nt(73), (t(21)+t(28)+t(16)+t(29)));
		//       load_sym(72)         => __E_load_cmd_26(73).
		q(nt(72), (nt(73)));
		//       load_cmd(40)         => load_sym(72) __(7) filename(28).
		q(nt(40), (nt(72)+nt(7)+nt(28)));
		//       __E_help_27(75)      => 'h'.
		q(nt(75), (t(27)));
		//       __E_help_27(75)      => 'h' 'e' 'l' 'p'.
		q(nt(75), (t(27)+t(18)+t(21)+t(15)));
		//       help_sym(74)         => __E_help_27(75).
		q(nt(74), (nt(75)));
		//       __E_help_28(76)      => __(7) cmd_symbol(77).
		q(nt(76), (nt(7)+nt(77)));
		//       __E_help_28(76)      => null.
		q(nt(76), (nul));
		//       help(42)             => help_sym(74) __E_help_28(76).
		q(nt(42), (nt(74)+nt(76)));
		//       __E_version_29(79)   => 'v'.
		q(nt(79), (t(30)));
		//       __E_version_29(79)   => 'v' 'e' 'r' 's' 'i' 'o' 'n'.
		q(nt(79), (t(30)+t(18)+t(12)+t(17)+t(20)+t(28)+t(11)));
		//       version_sym(78)      => __E_version_29(79).
		q(nt(78), (nt(79)));
		//       version(43)          => version_sym(78).
		q(nt(43), (nt(78)));
		//       __E_quit_30(81)      => 'q'.
		q(nt(81), (t(31)));
		//       __E_quit_30(81)      => 'q' 'u' 'i' 't'.
		q(nt(81), (t(31)+t(25)+t(20)+t(13)));
		//       __E_quit_30(81)      => 'e'.
		q(nt(81), (t(18)));
		//       __E_quit_30(81)      => 'e' 'x' 'i' 't'.
		q(nt(81), (t(18)+t(32)+t(20)+t(13)));
		//       quit_sym(80)         => __E_quit_30(81).
		q(nt(80), (nt(81)));
		//       quit(44)             => quit_sym(80).
		q(nt(44), (nt(80)));
		//       __E_clear_31(83)     => 'c' 'l' 's'.
		q(nt(83), (t(26)+t(21)+t(17)));
		//       __E_clear_31(83)     => 'c' 'l' 'e' 'a' 'r'.
		q(nt(83), (t(26)+t(21)+t(18)+t(16)+t(12)));
		//       clear_sym(82)        => __E_clear_31(83).
		q(nt(82), (nt(83)));
		//       clear(45)            => clear_sym(82).
		q(nt(45), (nt(82)));
		//       add_sym(84)          => 'a' 'd' 'd'.
		q(nt(84), (t(16)+t(29)+t(29)));
		//       add(51)              => add_sym(84) __(7) list_option(85) __(7) symbol_list(86).
		q(nt(51), (nt(84)+nt(7)+nt(85)+nt(7)+nt(86)));
		//       __E_del_32(88)       => 'd' 'e' 'l'.
		q(nt(88), (t(29)+t(18)+t(21)));
		//       __E_del_32(88)       => 'd' 'e' 'l' 'e' 't' 'e'.
		q(nt(88), (t(29)+t(18)+t(21)+t(18)+t(13)+t(18)));
		//       __E_del_32(88)       => 'r' 'm'.
		q(nt(88), (t(12)+t(23)));
		//       __E_del_32(88)       => 'r' 'e' 'm'.
		q(nt(88), (t(12)+t(18)+t(23)));
		//       __E_del_32(88)       => 'r' 'e' 'm' 'o' 'v' 'e'.
		q(nt(88), (t(12)+t(18)+t(23)+t(28)+t(30)+t(18)));
		//       del_sym(87)          => __E_del_32(88).
		q(nt(87), (nt(88)));
		//       del(52)              => del_sym(87) __(7) list_option(85) __(7) symbol_list(86).
		q(nt(52), (nt(87)+nt(7)+nt(85)+nt(7)+nt(86)));
		//       get_sym(89)          => 'g' 'e' 't'.
		q(nt(89), (t(22)+t(18)+t(13)));
		//       __E_get_33(90)       => __(7) option(91).
		q(nt(90), (nt(7)+nt(91)));
		//       __E_get_33(90)       => null.
		q(nt(90), (nul));
		//       get(46)              => get_sym(89) __E_get_33(90).
		q(nt(46), (nt(89)+nt(90)));
		//       set_sym(92)          => 's' 'e' 't'.
		q(nt(92), (t(17)+t(18)+t(13)));
		//       __E_set_34(93)       => __(7).
		q(nt(93), (nt(7)));
		//       __E_set_34(93)       => _(6) '=' _(6).
		q(nt(93), (nt(6)+t(33)+nt(6)));
		//       set(47)              => set_sym(92) __(7) option(91) __E_set_34(93) option_value(94).
		q(nt(47), (nt(92)+nt(7)+nt(91)+nt(93)+nt(94)));
		//       __E_toggle_35(96)    => 't' 'o' 'g'.
		q(nt(96), (t(13)+t(28)+t(22)));
		//       __E_toggle_35(96)    => 't' 'o' 'g' 'g' 'l' 'e'.
		q(nt(96), (t(13)+t(28)+t(22)+t(22)+t(21)+t(18)));
		//       toggle_sym(95)       => __E_toggle_35(96).
		q(nt(95), (nt(96)));
		//       toggle(48)           => toggle_sym(95) __(7) bool_option(97).
		q(nt(48), (nt(95)+nt(7)+nt(97)));
		//       __E_enable_36(99)    => 'e' 'n' __(7).
		q(nt(99), (t(18)+t(11)+nt(7)));
		//       __E_enable_36(99)    => 'e' 'n' 'a' 'b' 'l' 'e' __(7).
		q(nt(99), (t(18)+t(11)+t(16)+t(9)+t(21)+t(18)+nt(7)));
		//       enable_sym(98)       => __E_enable_36(99).
		q(nt(98), (nt(99)));
		//       enable(49)           => enable_sym(98) bool_option(97).
		q(nt(49), (nt(98)+nt(97)));
		//       __E_disable_37(101)  => 'd' 'i' 's' __(7).
		q(nt(101), (t(29)+t(20)+t(17)+nt(7)));
		//       __E_disable_37(101)  => 'd' 'i' 's' 'a' 'b' 'l' 'e' __(7).
		q(nt(101), (t(29)+t(20)+t(17)+t(16)+t(9)+t(21)+t(18)+nt(7)));
		//       disable_sym(100)     => __E_disable_37(101).
		q(nt(100), (nt(101)));
		//       disable(50)          => disable_sym(100) bool_option(97).
		q(nt(50), (nt(100)+nt(97)));
		//       cmd_symbol(77)       => grammar_sym(59).
		q(nt(77), (nt(59)));
		//       cmd_symbol(77)       => internal_grammar_sym(61).
		q(nt(77), (nt(61)));
		//       cmd_symbol(77)       => unreachable_sym(67).
		q(nt(77), (nt(67)));
		//       cmd_symbol(77)       => start_sym(64).
		q(nt(77), (nt(64)));
		//       cmd_symbol(77)       => parse_sym(55).
		q(nt(77), (nt(55)));
		//       cmd_symbol(77)       => parse_file_sym(57).
		q(nt(77), (nt(57)));
		//       cmd_symbol(77)       => load_sym(72).
		q(nt(77), (nt(72)));
		//       cmd_symbol(77)       => reload_sym(70).
		q(nt(77), (nt(70)));
		//       cmd_symbol(77)       => help_sym(74).
		q(nt(77), (nt(74)));
		//       cmd_symbol(77)       => quit_sym(80).
		q(nt(77), (nt(80)));
		//       cmd_symbol(77)       => version_sym(78).
		q(nt(77), (nt(78)));
		//       cmd_symbol(77)       => clear_sym(82).
		q(nt(77), (nt(82)));
		//       cmd_symbol(77)       => get_sym(89).
		q(nt(77), (nt(89)));
		//       cmd_symbol(77)       => set_sym(92).
		q(nt(77), (nt(92)));
		//       cmd_symbol(77)       => add_sym(84).
		q(nt(77), (nt(84)));
		//       cmd_symbol(77)       => del_sym(87).
		q(nt(77), (nt(87)));
		//       cmd_symbol(77)       => toggle_sym(95).
		q(nt(77), (nt(95)));
		//       cmd_symbol(77)       => enable_sym(98).
		q(nt(77), (nt(98)));
		//       cmd_symbol(77)       => disable_sym(100).
		q(nt(77), (nt(100)));
		//       option(91)           => bool_option(97).
		q(nt(91), (nt(97)));
		//       __E_option_38(103)   => 'e'.
		q(nt(103), (t(18)));
		//       __E_option_38(103)   => 'e' 'r' 'r' 'o' 'r' '-' 'v' 'e' 'r' 'b' 'o' 's' 'i' 't' 'y'.
		q(nt(103), (t(18)+t(12)+t(12)+t(28)+t(12)+t(24)+t(30)+t(18)+t(12)+t(9)+t(28)+t(17)+t(20)+t(13)+t(34)));
		//       error_verbosity_opt(102) => __E_option_38(103).
		q(nt(102), (nt(103)));
		//       option(91)           => error_verbosity_opt(102).
		q(nt(91), (nt(102)));
		//       option(91)           => list_option(85).
		q(nt(91), (nt(85)));
		//       __E_bool_option_39(105) => 's'.
		q(nt(105), (t(17)));
		//       __E_bool_option_39(105) => 's' 't' 'a' 't' 'u' 's'.
		q(nt(105), (t(17)+t(13)+t(16)+t(13)+t(25)+t(17)));
		//       status_opt(104)      => __E_bool_option_39(105).
		q(nt(104), (nt(105)));
		//       bool_option(97)      => status_opt(104).
		q(nt(97), (nt(104)));
		//       __E_bool_option_40(107) => 'c'.
		q(nt(107), (t(26)));
		//       __E_bool_option_40(107) => 'c' 'o' 'l' 'o' 'r'.
		q(nt(107), (t(26)+t(28)+t(21)+t(28)+t(12)));
		//       __E_bool_option_40(107) => 'c' 'o' 'l' 'o' 'r' 's'.
		q(nt(107), (t(26)+t(28)+t(21)+t(28)+t(12)+t(17)));
		//       colors_opt(106)      => __E_bool_option_40(107).
		q(nt(106), (nt(107)));
		//       bool_option(97)      => colors_opt(106).
		q(nt(97), (nt(106)));
		//       __E_bool_option_41(109) => 'a'.
		q(nt(109), (t(16)));
		//       __E_bool_option_41(109) => 'a' 'm' 'b' 'i' 'g' 'u' 'i' 't' 'y'.
		q(nt(109), (t(16)+t(23)+t(9)+t(20)+t(22)+t(25)+t(20)+t(13)+t(34)));
		//       __E_bool_option_41(109) => 'p' 'r' 'i' 'n' 't' '-' 'a' 'm' 'b' 'i' 'g' 'u' 'i' 't' 'y'.
		q(nt(109), (t(15)+t(12)+t(20)+t(11)+t(13)+t(24)+t(16)+t(23)+t(9)+t(20)+t(22)+t(25)+t(20)+t(13)+t(34)));
		//       print_ambiguity_opt(108) => __E_bool_option_41(109).
		q(nt(108), (nt(109)));
		//       bool_option(97)      => print_ambiguity_opt(108).
		q(nt(97), (nt(108)));
		//       __E_bool_option_42(111) => 'g'.
		q(nt(111), (t(22)));
		//       __E_bool_option_42(111) => 'g' 'r' 'a' 'p' 'h' 's'.
		q(nt(111), (t(22)+t(12)+t(16)+t(15)+t(27)+t(17)));
		//       __E_bool_option_42(111) => 'p' 'r' 'i' 'n' 't' '-' 'g' 'r' 'a' 'p' 'h' 's'.
		q(nt(111), (t(15)+t(12)+t(20)+t(11)+t(13)+t(24)+t(22)+t(12)+t(16)+t(15)+t(27)+t(17)));
		//       print_graphs_opt(110) => __E_bool_option_42(111).
		q(nt(110), (nt(111)));
		//       bool_option(97)      => print_graphs_opt(110).
		q(nt(97), (nt(110)));
		//       __E_bool_option_43(113) => 'r'.
		q(nt(113), (t(12)));
		//       __E_bool_option_43(113) => 'r' 'u' 'l' 'e' 's'.
		q(nt(113), (t(12)+t(25)+t(21)+t(18)+t(17)));
		//       __E_bool_option_43(113) => 'p' 'r' 'i' 'n' 't' '-' 'r' 'u' 'l' 'e' 's'.
		q(nt(113), (t(15)+t(12)+t(20)+t(11)+t(13)+t(24)+t(12)+t(25)+t(21)+t(18)+t(17)));
		//       print_rules_opt(112) => __E_bool_option_43(113).
		q(nt(112), (nt(113)));
		//       bool_option(97)      => print_rules_opt(112).
		q(nt(97), (nt(112)));
		//       __E_bool_option_44(115) => 'f'.
		q(nt(115), (t(10)));
		//       __E_bool_option_44(115) => 'f' 'a' 'c' 't' 's'.
		q(nt(115), (t(10)+t(16)+t(26)+t(13)+t(17)));
		//       __E_bool_option_44(115) => 'p' 'r' 'i' 'n' 't' '-' 'f' 'a' 'c' 't' 's'.
		q(nt(115), (t(15)+t(12)+t(20)+t(11)+t(13)+t(24)+t(10)+t(16)+t(26)+t(13)+t(17)));
		//       print_facts_opt(114) => __E_bool_option_44(115).
		q(nt(114), (nt(115)));
		//       bool_option(97)      => print_facts_opt(114).
		q(nt(97), (nt(114)));
		//       __E_bool_option_45(117) => 't'.
		q(nt(117), (t(13)));
		//       __E_bool_option_45(117) => 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
		q(nt(117), (t(13)+t(18)+t(12)+t(23)+t(20)+t(11)+t(16)+t(21)+t(17)));
		//       __E_bool_option_45(117) => 'p' 'r' 'i' 'n' 't' '-' 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
		q(nt(117), (t(15)+t(12)+t(20)+t(11)+t(13)+t(24)+t(13)+t(18)+t(12)+t(23)+t(20)+t(11)+t(16)+t(21)+t(17)));
		//       print_terminals_opt(116) => __E_bool_option_45(117).
		q(nt(116), (nt(117)));
		//       bool_option(97)      => print_terminals_opt(116).
		q(nt(97), (nt(116)));
		//       __E_bool_option_46(119) => 'm'.
		q(nt(119), (t(23)));
		//       __E_bool_option_46(119) => 'm' 'e' 'a' 's' 'u' 'r' 'e'.
		q(nt(119), (t(23)+t(18)+t(16)+t(17)+t(25)+t(12)+t(18)));
		//       __E_bool_option_46(119) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'p' 'a' 'r' 's' 'i' 'n' 'g'.
		q(nt(119), (t(23)+t(18)+t(16)+t(17)+t(25)+t(12)+t(18)+t(24)+t(15)+t(16)+t(12)+t(17)+t(20)+t(11)+t(22)));
		//       measure_parsing_opt(118) => __E_bool_option_46(119).
		q(nt(118), (nt(119)));
		//       bool_option(97)      => measure_parsing_opt(118).
		q(nt(97), (nt(118)));
		//       __E_bool_option_47(121) => 'm' 'e'.
		q(nt(121), (t(23)+t(18)));
		//       __E_bool_option_47(121) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'e' 'a' 'c' 'h'.
		q(nt(121), (t(23)+t(18)+t(16)+t(17)+t(25)+t(12)+t(18)+t(24)+t(18)+t(16)+t(26)+t(27)));
		//       __E_bool_option_47(121) => 'm' 'e' 'p'.
		q(nt(121), (t(23)+t(18)+t(15)));
		//       __E_bool_option_47(121) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'e' 'a' 'c' 'h' '-' 'p' 'o' 's'.
		q(nt(121), (t(23)+t(18)+t(16)+t(17)+t(25)+t(12)+t(18)+t(24)+t(18)+t(16)+t(26)+t(27)+t(24)+t(15)+t(28)+t(17)));
		//       measure_each_pos_opt(120) => __E_bool_option_47(121).
		q(nt(120), (nt(121)));
		//       bool_option(97)      => measure_each_pos_opt(120).
		q(nt(97), (nt(120)));
		//       __E_bool_option_48(123) => 'm' 'f'.
		q(nt(123), (t(23)+t(10)));
		//       __E_bool_option_48(123) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'f' 'o' 'r' 'e' 's' 't'.
		q(nt(123), (t(23)+t(18)+t(16)+t(17)+t(25)+t(12)+t(18)+t(24)+t(10)+t(28)+t(12)+t(18)+t(17)+t(13)));
		//       measure_forest_opt(122) => __E_bool_option_48(123).
		q(nt(122), (nt(123)));
		//       bool_option(97)      => measure_forest_opt(122).
		q(nt(97), (nt(122)));
		//       __E_bool_option_49(125) => 'm' 'p'.
		q(nt(125), (t(23)+t(15)));
		//       __E_bool_option_49(125) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'p' 'r' 'e' 'p' 'r' 'o' 'c' 'e' 's' 's'.
		q(nt(125), (t(23)+t(18)+t(16)+t(17)+t(25)+t(12)+t(18)+t(24)+t(15)+t(12)+t(18)+t(15)+t(12)+t(28)+t(26)+t(18)+t(17)+t(17)));
		//       measure_preprocess_opt(124) => __E_bool_option_49(125).
		q(nt(124), (nt(125)));
		//       bool_option(97)      => measure_preprocess_opt(124).
		q(nt(97), (nt(124)));
		//       __E_bool_option_50(127) => 'd'.
		q(nt(127), (t(29)));
		//       __E_bool_option_50(127) => 'd' 'e' 'b' 'u' 'g'.
		q(nt(127), (t(29)+t(18)+t(9)+t(25)+t(22)));
		//       debug_opt(126)       => __E_bool_option_50(127).
		q(nt(126), (nt(127)));
		//       bool_option(97)      => debug_opt(126).
		q(nt(97), (nt(126)));
		//       __E_bool_option_51(129) => 'a' 'd'.
		q(nt(129), (t(16)+t(29)));
		//       __E_bool_option_51(129) => 'a' 'u' 't' 'o' '-' 'd' 'i' 's' 'a' 'm' 'b' 'i' 'g' 'u' 'a' 't' 'e'.
		q(nt(129), (t(16)+t(25)+t(13)+t(28)+t(24)+t(29)+t(20)+t(17)+t(16)+t(23)+t(9)+t(20)+t(22)+t(25)+t(16)+t(13)+t(18)));
		//       auto_disambiguate_opt(128) => __E_bool_option_51(129).
		q(nt(128), (nt(129)));
		//       bool_option(97)      => auto_disambiguate_opt(128).
		q(nt(97), (nt(128)));
		//       __E_bool_option_52(131) => 't' 't'.
		q(nt(131), (t(13)+t(13)));
		//       __E_bool_option_52(131) => 't' 'r' 'i' 'm' '-' 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
		q(nt(131), (t(13)+t(12)+t(20)+t(23)+t(24)+t(13)+t(18)+t(12)+t(23)+t(20)+t(11)+t(16)+t(21)+t(17)));
		//       trim_terminals_opt(130) => __E_bool_option_52(131).
		q(nt(130), (nt(131)));
		//       bool_option(97)      => trim_terminals_opt(130).
		q(nt(97), (nt(130)));
		//       __E_bool_option_53(133) => 'i' 'c' 'c'.
		q(nt(133), (t(20)+t(26)+t(26)));
		//       __E_bool_option_53(133) => 'i' 'n' 'l' 'i' 'n' 'e' '-' 'c' 'c'.
		q(nt(133), (t(20)+t(11)+t(21)+t(20)+t(11)+t(18)+t(24)+t(26)+t(26)));
		//       __E_bool_option_53(133) => 'i' 'n' 'l' 'i' 'n' 'e' '-' 'c' 'h' 'a' 'r' '-' 'c' 'l' 'a' 's' 's' 'e' 's'.
		q(nt(133), (t(20)+t(11)+t(21)+t(20)+t(11)+t(18)+t(24)+t(26)+t(27)+t(16)+t(12)+t(24)+t(26)+t(21)+t(16)+t(17)+t(17)+t(18)+t(17)));
		//       inline_cc_opt(132)   => __E_bool_option_53(133).
		q(nt(132), (nt(133)));
		//       bool_option(97)      => inline_cc_opt(132).
		q(nt(97), (nt(132)));
		//       __E_list_option_54(135) => 'n' 'd'.
		q(nt(135), (t(11)+t(29)));
		//       __E_list_option_54(135) => 'n' 'd' 'l'.
		q(nt(135), (t(11)+t(29)+t(21)));
		//       __E_list_option_54(135) => 'n' 'o' 'd' 'i' 's' 'a' 'm' 'b' 'i' 'g' '-' 'l' 'i' 's' 't'.
		q(nt(135), (t(11)+t(28)+t(29)+t(20)+t(17)+t(16)+t(23)+t(9)+t(20)+t(22)+t(24)+t(21)+t(20)+t(17)+t(13)));
		//       nodisambig_list_opt(134) => __E_list_option_54(135).
		q(nt(134), (nt(135)));
		//       list_option(85)      => nodisambig_list_opt(134).
		q(nt(85), (nt(134)));
		//       trim_opt(136)        => 't' 'r' 'i' 'm'.
		q(nt(136), (t(13)+t(12)+t(20)+t(23)));
		//       list_option(85)      => trim_opt(136).
		q(nt(85), (nt(136)));
		//       __E_list_option_55(138) => 't' 'c'.
		q(nt(138), (t(13)+t(26)));
		//       __E_list_option_55(138) => 't' 'r' 'i' 'm' '-' 'c' 'h' 'i' 'l' 'd' 'r' 'e' 'n'.
		q(nt(138), (t(13)+t(12)+t(20)+t(23)+t(24)+t(26)+t(27)+t(20)+t(21)+t(29)+t(12)+t(18)+t(11)));
		//       trim_children_opt(137) => __E_list_option_55(138).
		q(nt(137), (nt(138)));
		//       list_option(85)      => trim_children_opt(137).
		q(nt(85), (nt(137)));
		//       __E_list_option_56(140) => 'i'.
		q(nt(140), (t(20)));
		//       __E_list_option_56(140) => 'i' 'n' 'l' 'i' 'n' 'e'.
		q(nt(140), (t(20)+t(11)+t(21)+t(20)+t(11)+t(18)));
		//       inline_opt(139)      => __E_list_option_56(140).
		q(nt(139), (nt(140)));
		//       list_option(85)      => inline_opt(139).
		q(nt(85), (nt(139)));
		//       option_value(94)     => option_value_true(141).
		q(nt(94), (nt(141)));
		//       option_value(94)     => option_value_false(142).
		q(nt(94), (nt(142)));
		//       option_value(94)     => error_verbosity(143).
		q(nt(94), (nt(143)));
		//       option_value(94)     => symbol_list(86).
		q(nt(94), (nt(86)));
		//       option_value_true(141) => 't'.
		q(nt(141), (t(13)));
		//       option_value_true(141) => 't' 'r' 'u' 'e'.
		q(nt(141), (t(13)+t(12)+t(25)+t(18)));
		//       option_value_true(141) => 'o' 'n'.
		q(nt(141), (t(28)+t(11)));
		//       option_value_true(141) => '1'.
		q(nt(141), (t(35)));
		//       option_value_true(141) => 'y'.
		q(nt(141), (t(34)));
		//       option_value_true(141) => 'y' 'e' 's'.
		q(nt(141), (t(34)+t(18)+t(17)));
		//       option_value_false(142) => 'f'.
		q(nt(142), (t(10)));
		//       option_value_false(142) => 'f' 'a' 'l' 's' 'e'.
		q(nt(142), (t(10)+t(16)+t(21)+t(17)+t(18)));
		//       option_value_false(142) => 'o' 'f' 'f'.
		q(nt(142), (t(28)+t(10)+t(10)));
		//       option_value_false(142) => '0'.
		q(nt(142), (t(36)));
		//       option_value_false(142) => 'n'.
		q(nt(142), (t(11)));
		//       option_value_false(142) => 'n' 'o'.
		q(nt(142), (t(11)+t(28)));
		//       __E_error_verbosity_57(145) => 'b'.
		q(nt(145), (t(9)));
		//       __E_error_verbosity_57(145) => 'b' 'a' 's' 'i' 'c'.
		q(nt(145), (t(9)+t(16)+t(17)+t(20)+t(26)));
		//       basic_sym(144)       => __E_error_verbosity_57(145).
		q(nt(144), (nt(145)));
		//       error_verbosity(143) => basic_sym(144).
		q(nt(143), (nt(144)));
		//       __E_error_verbosity_58(147) => 'd'.
		q(nt(147), (t(29)));
		//       __E_error_verbosity_58(147) => 'd' 'e' 't' 'a' 'i' 'l' 'e' 'd'.
		q(nt(147), (t(29)+t(18)+t(13)+t(16)+t(20)+t(21)+t(18)+t(29)));
		//       detailed_sym(146)    => __E_error_verbosity_58(147).
		q(nt(146), (nt(147)));
		//       error_verbosity(143) => detailed_sym(146).
		q(nt(143), (nt(146)));
		//       __E_error_verbosity_59(149) => 'r'.
		q(nt(149), (t(12)));
		//       __E_error_verbosity_59(149) => 'r' 'c'.
		q(nt(149), (t(12)+t(26)));
		//       __E_error_verbosity_59(149) => 'r' 'o' 'o' 't' '-' 'c' 'a' 'u' 's' 'e'.
		q(nt(149), (t(12)+t(28)+t(28)+t(13)+t(24)+t(26)+t(16)+t(25)+t(17)+t(18)));
		//       root_cause_sym(148)  => __E_error_verbosity_59(149).
		q(nt(148), (nt(149)));
		//       error_verbosity(143) => root_cause_sym(148).
		q(nt(143), (nt(148)));
		//       __E_symbol_list_60(150) => _(6) ',' _(6) symbol(29).
		q(nt(150), (nt(6)+t(37)+nt(6)+nt(29)));
		//       __E_symbol_list_61(151) => null.
		q(nt(151), (nul));
		//       __E_symbol_list_61(151) => __E_symbol_list_60(150) __E_symbol_list_61(151).
		q(nt(151), (nt(150)+nt(151)));
		//       symbol_list(86)      => symbol(29) __E_symbol_list_61(151).
		q(nt(86), (nt(29)+nt(151)));
		return q;
	}
};

#endif // __TGF_REPL_PARSER_H__
