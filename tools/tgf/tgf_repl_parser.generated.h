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
		g(nts, load_prods(), nt(53), cc, load_grammar_opts()),
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
		nul, eof, alnum, alpha, space, printable, parse_input_char, _, __, grammar_sym, 
		internal_grammar_sym, unreachable_sym, start_sym, parse_sym, parse_file_sym, load_sym, reload_sym, help_sym, quit_sym, version_sym, 
		clear_sym, get_sym, set_sym, add_sym, del_sym, toggle_sym, enable_sym, disable_sym, true_value, false_value, 
		basic_sym, detailed_sym, root_cause_sym, error_verbosity_opt, status_opt, colors_opt, print_ambiguity_opt, print_graphs_opt, print_rules_opt, print_facts_opt, 
		print_terminals_opt, measure_parsing_opt, measure_each_pos_opt, measure_forest_opt, measure_preprocess_opt, debug_opt, auto_disambiguate_opt, trim_terminals_opt, inline_cc_opt, nodisambig_list_opt, 
		trim_opt, trim_children_opt, inline_opt, start, __E_start_0, statement, __E___E_start_0_1, __E___E_start_0_2, __E___3, __E____4, 
		comment, __E_comment_5, __E_comment_6, __E_comment_7, quoted_string, quoted_string_char, __E_quoted_string_8, unescaped_s, escaped_s, __E_unescaped_s_9, 
		__E_unescaped_s_10, __E_escaped_s_11, filename, symbol, __E_symbol_12, __E_symbol_13, __E_symbol_14, parse_input, parse_input_char_seq, __E_parse_input_char_seq_15, 
		grammar_cmd, internal_grammar_cmd, unreachable_cmd, reload_cmd, load_cmd, start_cmd, help, version, quit, clear, 
		get, set, toggle, enable, disable, add, del, parse_file_cmd, parse_cmd, __E_parse_cmd_16, 
		__E_parse_file_cmd_17, __E_grammar_cmd_18, __E_internal_grammar_cmd_19, __E_internal_grammar_cmd_20, __E_start_cmd_21, __E_start_cmd_22, __E_unreachable_cmd_23, __E_unreachable_cmd_24, __E_reload_cmd_25, __E_load_cmd_26, 
		__E_help_27, __E_help_28, cmd_symbol, __E_version_29, __E_quit_30, __E_clear_31, __E_get_32, option, __E_add_33, list_option, 
		symbol_list, treepaths_option, treepath_list, __E_del_34, __E_del_35, __E_set_36, bool_option, __E___E_set_36_37, bool_value, __E___E_set_36_38, 
		__E___E_set_36_39, enum_ev_option, __E___E_set_36_40, error_verbosity, __E_toggle_41, __E_enable_42, __E_disable_43, __E_enum_ev_option_44, __E_bool_option_45, __E_bool_option_46, 
		__E_bool_option_47, __E_bool_option_48, __E_bool_option_49, __E_bool_option_50, __E_bool_option_51, __E_bool_option_52, __E_bool_option_53, __E_bool_option_54, __E_bool_option_55, __E_bool_option_56, 
		__E_bool_option_57, __E_bool_option_58, __E_bool_option_59, __E_list_option_60, __E_list_option_61, __E_treepaths_option_62, __E_bool_value_63, __E_bool_value_64, __E_symbol_list_65, __E_symbol_list_66, 
		treepath, __E_treepath_list_67, __E_treepath_list_68, __E_treepath_69, __E_treepath_70, __E_error_verbosity_71, __E_error_verbosity_72, __E_error_verbosity_73, __N_0, 
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
		'd', 'v', 'q', 'x', '=', 'y', '1', '0', ',', '>', 
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
			"", "eof", "alnum", "alpha", "space", "printable", "parse_input_char", "_", "__", "grammar_sym", 
			"internal_grammar_sym", "unreachable_sym", "start_sym", "parse_sym", "parse_file_sym", "load_sym", "reload_sym", "help_sym", "quit_sym", "version_sym", 
			"clear_sym", "get_sym", "set_sym", "add_sym", "del_sym", "toggle_sym", "enable_sym", "disable_sym", "true_value", "false_value", 
			"basic_sym", "detailed_sym", "root_cause_sym", "error_verbosity_opt", "status_opt", "colors_opt", "print_ambiguity_opt", "print_graphs_opt", "print_rules_opt", "print_facts_opt", 
			"print_terminals_opt", "measure_parsing_opt", "measure_each_pos_opt", "measure_forest_opt", "measure_preprocess_opt", "debug_opt", "auto_disambiguate_opt", "trim_terminals_opt", "inline_cc_opt", "nodisambig_list_opt", 
			"trim_opt", "trim_children_opt", "inline_opt", "start", "__E_start_0", "statement", "__E___E_start_0_1", "__E___E_start_0_2", "__E___3", "__E____4", 
			"comment", "__E_comment_5", "__E_comment_6", "__E_comment_7", "quoted_string", "quoted_string_char", "__E_quoted_string_8", "unescaped_s", "escaped_s", "__E_unescaped_s_9", 
			"__E_unescaped_s_10", "__E_escaped_s_11", "filename", "symbol", "__E_symbol_12", "__E_symbol_13", "__E_symbol_14", "parse_input", "parse_input_char_seq", "__E_parse_input_char_seq_15", 
			"grammar_cmd", "internal_grammar_cmd", "unreachable_cmd", "reload_cmd", "load_cmd", "start_cmd", "help", "version", "quit", "clear", 
			"get", "set", "toggle", "enable", "disable", "add", "del", "parse_file_cmd", "parse_cmd", "__E_parse_cmd_16", 
			"__E_parse_file_cmd_17", "__E_grammar_cmd_18", "__E_internal_grammar_cmd_19", "__E_internal_grammar_cmd_20", "__E_start_cmd_21", "__E_start_cmd_22", "__E_unreachable_cmd_23", "__E_unreachable_cmd_24", "__E_reload_cmd_25", "__E_load_cmd_26", 
			"__E_help_27", "__E_help_28", "cmd_symbol", "__E_version_29", "__E_quit_30", "__E_clear_31", "__E_get_32", "option", "__E_add_33", "list_option", 
			"symbol_list", "treepaths_option", "treepath_list", "__E_del_34", "__E_del_35", "__E_set_36", "bool_option", "__E___E_set_36_37", "bool_value", "__E___E_set_36_38", 
			"__E___E_set_36_39", "enum_ev_option", "__E___E_set_36_40", "error_verbosity", "__E_toggle_41", "__E_enable_42", "__E_disable_43", "__E_enum_ev_option_44", "__E_bool_option_45", "__E_bool_option_46", 
			"__E_bool_option_47", "__E_bool_option_48", "__E_bool_option_49", "__E_bool_option_50", "__E_bool_option_51", "__E_bool_option_52", "__E_bool_option_53", "__E_bool_option_54", "__E_bool_option_55", "__E_bool_option_56", 
			"__E_bool_option_57", "__E_bool_option_58", "__E_bool_option_59", "__E_list_option_60", "__E_list_option_61", "__E_treepaths_option_62", "__E_bool_value_63", "__E_bool_value_64", "__E_symbol_list_65", "__E_symbol_list_66", 
			"treepath", "__E_treepath_list_67", "__E_treepath_list_68", "__E_treepath_69", "__E_treepath_70", "__E_error_verbosity_71", "__E_error_verbosity_72", "__E_error_verbosity_73", "__N_0", 
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
		o.to_trim = {
			7, 8
		};
		o.to_trim_children = {
			9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
			19, 20, 21, 22, 23, 24, 25, 26, 27, 28,
			29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
			39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
			49, 50, 51, 52
		};
		o.to_inline = {
			{ 6 }
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
		//       __E___E_start_0_1(56) => _(7) '.' _(7) statement(55).
		q(nt(56), (nt(7)+t(1)+nt(7)+nt(55)));
		//       __E___E_start_0_2(57) => null.
		q(nt(57), (nul));
		//       __E___E_start_0_2(57) => __E___E_start_0_1(56) __E___E_start_0_2(57).
		q(nt(57), (nt(56)+nt(57)));
		//       __E_start_0(54)      => statement(55) __E___E_start_0_2(57) _(7).
		q(nt(54), (nt(55)+nt(57)+nt(7)));
		//       __E_start_0(54)      => null.
		q(nt(54), (nul));
		//       start(53)            => _(7) __E_start_0(54).
		q(nt(53), (nt(7)+nt(54)));
		//       __E___3(58)          => __(8).
		q(nt(58), (nt(8)));
		//       __E___3(58)          => null.
		q(nt(58), (nul));
		//       _(7)                 => __E___3(58).
		q(nt(7), (nt(58)));
		//       __E____4(59)         => space(4).
		q(nt(59), (nt(4)));
		//       __E____4(59)         => comment(60).
		q(nt(59), (nt(60)));
		//       __(8)                => __E____4(59) _(7).
		q(nt(8), (nt(59)+nt(7)));
		//       __E_comment_5(61)    => printable(5).
		q(nt(61), (nt(5)));
		//       __E_comment_5(61)    => '\t'.
		q(nt(61), (t(2)));
		//       __E_comment_6(62)    => null.
		q(nt(62), (nul));
		//       __E_comment_6(62)    => __E_comment_5(61) __E_comment_6(62).
		q(nt(62), (nt(61)+nt(62)));
		//       __E_comment_7(63)    => '\r'.
		q(nt(63), (t(3)));
		//       __E_comment_7(63)    => '\n'.
		q(nt(63), (t(4)));
		//       __E_comment_7(63)    => eof(1).
		q(nt(63), (nt(1)));
		//       comment(60)          => '#' __E_comment_6(62) __E_comment_7(63).
		q(nt(60), (t(5)+nt(62)+nt(63)));
		//       __E_quoted_string_8(66) => null.
		q(nt(66), (nul));
		//       __E_quoted_string_8(66) => quoted_string_char(65) __E_quoted_string_8(66).
		q(nt(66), (nt(65)+nt(66)));
		//       quoted_string(64)    => '"' __E_quoted_string_8(66) '"'.
		q(nt(64), (t(6)+nt(66)+t(6)));
		//       quoted_string_char(65) => unescaped_s(67).
		q(nt(65), (nt(67)));
		//       quoted_string_char(65) => escaped_s(68).
		q(nt(65), (nt(68)));
		//       __E_unescaped_s_9(69) => space(4).
		q(nt(69), (nt(4)));
		//       __E_unescaped_s_9(69) => printable(5).
		q(nt(69), (nt(5)));
		//       __E_unescaped_s_10(70) => '"'.
		q(nt(70), (t(6)));
		//       __E_unescaped_s_10(70) => '\\'.
		q(nt(70), (t(7)));
		//       __N_0(168)           => __E_unescaped_s_10(70).
		q(nt(168), (nt(70)));
		//       unescaped_s(67)      => __E_unescaped_s_9(69) & ~( __N_0(168) ).	 # conjunctive
		q(nt(67), (nt(69)) & ~(nt(168)));
		//       __E_escaped_s_11(71) => '"'.
		q(nt(71), (t(6)));
		//       __E_escaped_s_11(71) => '\\'.
		q(nt(71), (t(7)));
		//       __E_escaped_s_11(71) => '/'.
		q(nt(71), (t(8)));
		//       __E_escaped_s_11(71) => 'b'.
		q(nt(71), (t(9)));
		//       __E_escaped_s_11(71) => 'f'.
		q(nt(71), (t(10)));
		//       __E_escaped_s_11(71) => 'n'.
		q(nt(71), (t(11)));
		//       __E_escaped_s_11(71) => 'r'.
		q(nt(71), (t(12)));
		//       __E_escaped_s_11(71) => 't'.
		q(nt(71), (t(13)));
		//       escaped_s(68)        => '\\' __E_escaped_s_11(71).
		q(nt(68), (t(7)+nt(71)));
		//       filename(72)         => quoted_string(64).
		q(nt(72), (nt(64)));
		//       __E_symbol_12(74)    => alpha(3).
		q(nt(74), (nt(3)));
		//       __E_symbol_12(74)    => '_'.
		q(nt(74), (t(14)));
		//       __E_symbol_13(75)    => alnum(2).
		q(nt(75), (nt(2)));
		//       __E_symbol_13(75)    => '_'.
		q(nt(75), (t(14)));
		//       __E_symbol_14(76)    => null.
		q(nt(76), (nul));
		//       __E_symbol_14(76)    => __E_symbol_13(75) __E_symbol_14(76).
		q(nt(76), (nt(75)+nt(76)));
		//       symbol(73)           => __E_symbol_12(74) __E_symbol_14(76).
		q(nt(73), (nt(74)+nt(76)));
		//       parse_input(77)      => quoted_string(64).
		q(nt(77), (nt(64)));
		//       parse_input(77)      => parse_input_char_seq(78).
		q(nt(77), (nt(78)));
		//       __E_parse_input_char_seq_15(79) => parse_input_char(6).
		q(nt(79), (nt(6)));
		//       __E_parse_input_char_seq_15(79) => parse_input_char(6) __E_parse_input_char_seq_15(79).
		q(nt(79), (nt(6)+nt(79)));
		//       parse_input_char_seq(78) => __E_parse_input_char_seq_15(79).
		q(nt(78), (nt(79)));
		//       parse_input_char(6)  => printable(5).
		q(nt(6), (nt(5)));
		//       parse_input_char(6)  => '\t'.
		q(nt(6), (t(2)));
		//       statement(55)        => grammar_cmd(80).
		q(nt(55), (nt(80)));
		//       statement(55)        => internal_grammar_cmd(81).
		q(nt(55), (nt(81)));
		//       statement(55)        => unreachable_cmd(82).
		q(nt(55), (nt(82)));
		//       statement(55)        => reload_cmd(83).
		q(nt(55), (nt(83)));
		//       statement(55)        => load_cmd(84).
		q(nt(55), (nt(84)));
		//       statement(55)        => start_cmd(85).
		q(nt(55), (nt(85)));
		//       statement(55)        => help(86).
		q(nt(55), (nt(86)));
		//       statement(55)        => version(87).
		q(nt(55), (nt(87)));
		//       statement(55)        => quit(88).
		q(nt(55), (nt(88)));
		//       statement(55)        => clear(89).
		q(nt(55), (nt(89)));
		//       statement(55)        => get(90).
		q(nt(55), (nt(90)));
		//       statement(55)        => set(91).
		q(nt(55), (nt(91)));
		//       statement(55)        => toggle(92).
		q(nt(55), (nt(92)));
		//       statement(55)        => enable(93).
		q(nt(55), (nt(93)));
		//       statement(55)        => disable(94).
		q(nt(55), (nt(94)));
		//       statement(55)        => add(95).
		q(nt(55), (nt(95)));
		//       statement(55)        => del(96).
		q(nt(55), (nt(96)));
		//       statement(55)        => parse_file_cmd(97).
		q(nt(55), (nt(97)));
		//       statement(55)        => parse_cmd(98).
		q(nt(55), (nt(98)));
		//       __E_parse_cmd_16(99) => 'p'.
		q(nt(99), (t(15)));
		//       __E_parse_cmd_16(99) => 'p' 'a' 'r' 's' 'e'.
		q(nt(99), (t(15)+t(16)+t(12)+t(17)+t(18)));
		//       parse_sym(13)        => __E_parse_cmd_16(99).
		q(nt(13), (nt(99)));
		//       parse_cmd(98)        => parse_sym(13) __(8) parse_input(77).
		q(nt(98), (nt(13)+nt(8)+nt(77)));
		//       __E_parse_file_cmd_17(100) => 'f'.
		q(nt(100), (t(10)));
		//       __E_parse_file_cmd_17(100) => 'p' 'f'.
		q(nt(100), (t(15)+t(10)));
		//       __E_parse_file_cmd_17(100) => 'p' 'a' 'r' 's' 'e' ' ' 'f' 'i' 'l' 'e'.
		q(nt(100), (t(15)+t(16)+t(12)+t(17)+t(18)+t(19)+t(10)+t(20)+t(21)+t(18)));
		//       parse_file_sym(14)   => __E_parse_file_cmd_17(100).
		q(nt(14), (nt(100)));
		//       parse_file_cmd(97)   => parse_file_sym(14) __(8) filename(72).
		q(nt(97), (nt(14)+nt(8)+nt(72)));
		//       __E_grammar_cmd_18(101) => 'g'.
		q(nt(101), (t(22)));
		//       __E_grammar_cmd_18(101) => 'g' 'r' 'a' 'm' 'm' 'a' 'r'.
		q(nt(101), (t(22)+t(12)+t(16)+t(23)+t(23)+t(16)+t(12)));
		//       grammar_sym(9)       => __E_grammar_cmd_18(101).
		q(nt(9), (nt(101)));
		//       grammar_cmd(80)      => grammar_sym(9).
		q(nt(80), (nt(9)));
		//       __E_internal_grammar_cmd_19(102) => 'i'.
		q(nt(102), (t(20)));
		//       __E_internal_grammar_cmd_19(102) => 'i' 'g'.
		q(nt(102), (t(20)+t(22)));
		//       __E_internal_grammar_cmd_19(102) => 'i' 'n' 't' 'e' 'r' 'n' 'a' 'l' '-' 'g' 'r' 'a' 'm' 'm' 'a' 'r'.
		q(nt(102), (t(20)+t(11)+t(13)+t(18)+t(12)+t(11)+t(16)+t(21)+t(24)+t(22)+t(12)+t(16)+t(23)+t(23)+t(16)+t(12)));
		//       internal_grammar_sym(10) => __E_internal_grammar_cmd_19(102).
		q(nt(10), (nt(102)));
		//       __E_internal_grammar_cmd_20(103) => __(8) symbol(73).
		q(nt(103), (nt(8)+nt(73)));
		//       __E_internal_grammar_cmd_20(103) => null.
		q(nt(103), (nul));
		//       internal_grammar_cmd(81) => internal_grammar_sym(10) __E_internal_grammar_cmd_20(103).
		q(nt(81), (nt(10)+nt(103)));
		//       __E_start_cmd_21(104) => 's'.
		q(nt(104), (t(17)));
		//       __E_start_cmd_21(104) => 's' 't' 'a' 'r' 't'.
		q(nt(104), (t(17)+t(13)+t(16)+t(12)+t(13)));
		//       start_sym(12)        => __E_start_cmd_21(104).
		q(nt(12), (nt(104)));
		//       __E_start_cmd_22(105) => __(8) symbol(73).
		q(nt(105), (nt(8)+nt(73)));
		//       __E_start_cmd_22(105) => null.
		q(nt(105), (nul));
		//       start_cmd(85)        => start_sym(12) __E_start_cmd_22(105).
		q(nt(85), (nt(12)+nt(105)));
		//       __E_unreachable_cmd_23(106) => 'u'.
		q(nt(106), (t(25)));
		//       __E_unreachable_cmd_23(106) => 'u' 'n' 'r' 'e' 'a' 'c' 'h' 'a' 'b' 'l' 'e'.
		q(nt(106), (t(25)+t(11)+t(12)+t(18)+t(16)+t(26)+t(27)+t(16)+t(9)+t(21)+t(18)));
		//       unreachable_sym(11)  => __E_unreachable_cmd_23(106).
		q(nt(11), (nt(106)));
		//       __E_unreachable_cmd_24(107) => __(8) symbol(73).
		q(nt(107), (nt(8)+nt(73)));
		//       __E_unreachable_cmd_24(107) => null.
		q(nt(107), (nul));
		//       unreachable_cmd(82)  => unreachable_sym(11) __E_unreachable_cmd_24(107).
		q(nt(82), (nt(11)+nt(107)));
		//       __E_reload_cmd_25(108) => 'r'.
		q(nt(108), (t(12)));
		//       __E_reload_cmd_25(108) => 'r' 'e' 'l' 'o' 'a' 'd'.
		q(nt(108), (t(12)+t(18)+t(21)+t(28)+t(16)+t(29)));
		//       reload_sym(16)       => __E_reload_cmd_25(108).
		q(nt(16), (nt(108)));
		//       reload_cmd(83)       => reload_sym(16).
		q(nt(83), (nt(16)));
		//       __E_load_cmd_26(109) => 'l'.
		q(nt(109), (t(21)));
		//       __E_load_cmd_26(109) => 'l' 'o' 'a' 'd'.
		q(nt(109), (t(21)+t(28)+t(16)+t(29)));
		//       load_sym(15)         => __E_load_cmd_26(109).
		q(nt(15), (nt(109)));
		//       load_cmd(84)         => load_sym(15) __(8) filename(72).
		q(nt(84), (nt(15)+nt(8)+nt(72)));
		//       __E_help_27(110)     => 'h'.
		q(nt(110), (t(27)));
		//       __E_help_27(110)     => 'h' 'e' 'l' 'p'.
		q(nt(110), (t(27)+t(18)+t(21)+t(15)));
		//       help_sym(17)         => __E_help_27(110).
		q(nt(17), (nt(110)));
		//       __E_help_28(111)     => __(8) cmd_symbol(112).
		q(nt(111), (nt(8)+nt(112)));
		//       __E_help_28(111)     => null.
		q(nt(111), (nul));
		//       help(86)             => help_sym(17) __E_help_28(111).
		q(nt(86), (nt(17)+nt(111)));
		//       __E_version_29(113)  => 'v'.
		q(nt(113), (t(30)));
		//       __E_version_29(113)  => 'v' 'e' 'r' 's' 'i' 'o' 'n'.
		q(nt(113), (t(30)+t(18)+t(12)+t(17)+t(20)+t(28)+t(11)));
		//       version_sym(19)      => __E_version_29(113).
		q(nt(19), (nt(113)));
		//       version(87)          => version_sym(19).
		q(nt(87), (nt(19)));
		//       __E_quit_30(114)     => 'q'.
		q(nt(114), (t(31)));
		//       __E_quit_30(114)     => 'q' 'u' 'i' 't'.
		q(nt(114), (t(31)+t(25)+t(20)+t(13)));
		//       __E_quit_30(114)     => 'e'.
		q(nt(114), (t(18)));
		//       __E_quit_30(114)     => 'e' 'x' 'i' 't'.
		q(nt(114), (t(18)+t(32)+t(20)+t(13)));
		//       quit_sym(18)         => __E_quit_30(114).
		q(nt(18), (nt(114)));
		//       quit(88)             => quit_sym(18).
		q(nt(88), (nt(18)));
		//       __E_clear_31(115)    => 'c' 'l' 's'.
		q(nt(115), (t(26)+t(21)+t(17)));
		//       __E_clear_31(115)    => 'c' 'l' 'e' 'a' 'r'.
		q(nt(115), (t(26)+t(21)+t(18)+t(16)+t(12)));
		//       clear_sym(20)        => __E_clear_31(115).
		q(nt(20), (nt(115)));
		//       clear(89)            => clear_sym(20).
		q(nt(89), (nt(20)));
		//       get_sym(21)          => 'g' 'e' 't'.
		q(nt(21), (t(22)+t(18)+t(13)));
		//       __E_get_32(116)      => __(8) option(117).
		q(nt(116), (nt(8)+nt(117)));
		//       __E_get_32(116)      => null.
		q(nt(116), (nul));
		//       get(90)              => get_sym(21) __E_get_32(116).
		q(nt(90), (nt(21)+nt(116)));
		//       add_sym(23)          => 'a' 'd' 'd'.
		q(nt(23), (t(16)+t(29)+t(29)));
		//       __E_add_33(118)      => list_option(119) __(8) symbol_list(120).
		q(nt(118), (nt(119)+nt(8)+nt(120)));
		//       __E_add_33(118)      => treepaths_option(121) __(8) treepath_list(122).
		q(nt(118), (nt(121)+nt(8)+nt(122)));
		//       add(95)              => add_sym(23) __(8) __E_add_33(118).
		q(nt(95), (nt(23)+nt(8)+nt(118)));
		//       __E_del_34(123)      => 'd' 'e' 'l'.
		q(nt(123), (t(29)+t(18)+t(21)));
		//       __E_del_34(123)      => 'd' 'e' 'l' 'e' 't' 'e'.
		q(nt(123), (t(29)+t(18)+t(21)+t(18)+t(13)+t(18)));
		//       __E_del_34(123)      => 'r' 'm'.
		q(nt(123), (t(12)+t(23)));
		//       __E_del_34(123)      => 'r' 'e' 'm'.
		q(nt(123), (t(12)+t(18)+t(23)));
		//       __E_del_34(123)      => 'r' 'e' 'm' 'o' 'v' 'e'.
		q(nt(123), (t(12)+t(18)+t(23)+t(28)+t(30)+t(18)));
		//       del_sym(24)          => __E_del_34(123).
		q(nt(24), (nt(123)));
		//       __E_del_35(124)      => list_option(119) __(8) symbol_list(120).
		q(nt(124), (nt(119)+nt(8)+nt(120)));
		//       __E_del_35(124)      => treepaths_option(121) __(8) treepath_list(122).
		q(nt(124), (nt(121)+nt(8)+nt(122)));
		//       del(96)              => del_sym(24) __(8) __E_del_35(124).
		q(nt(96), (nt(24)+nt(8)+nt(124)));
		//       set_sym(22)          => 's' 'e' 't'.
		q(nt(22), (t(17)+t(18)+t(13)));
		//       __E___E_set_36_37(127) => __(8).
		q(nt(127), (nt(8)));
		//       __E___E_set_36_37(127) => _(7) '=' _(7).
		q(nt(127), (nt(7)+t(33)+nt(7)));
		//       __E_set_36(125)      => bool_option(126) __E___E_set_36_37(127) bool_value(128).
		q(nt(125), (nt(126)+nt(127)+nt(128)));
		//       __E___E_set_36_38(129) => __(8).
		q(nt(129), (nt(8)));
		//       __E___E_set_36_38(129) => _(7) '=' _(7).
		q(nt(129), (nt(7)+t(33)+nt(7)));
		//       __E_set_36(125)      => list_option(119) __E___E_set_36_38(129) symbol_list(120).
		q(nt(125), (nt(119)+nt(129)+nt(120)));
		//       __E___E_set_36_39(130) => __(8).
		q(nt(130), (nt(8)));
		//       __E___E_set_36_39(130) => _(7) '=' _(7).
		q(nt(130), (nt(7)+t(33)+nt(7)));
		//       __E_set_36(125)      => treepaths_option(121) __E___E_set_36_39(130) treepath_list(122).
		q(nt(125), (nt(121)+nt(130)+nt(122)));
		//       __E___E_set_36_40(132) => __(8).
		q(nt(132), (nt(8)));
		//       __E___E_set_36_40(132) => _(7) '=' _(7).
		q(nt(132), (nt(7)+t(33)+nt(7)));
		//       __E_set_36(125)      => enum_ev_option(131) __E___E_set_36_40(132) error_verbosity(133).
		q(nt(125), (nt(131)+nt(132)+nt(133)));
		//       set(91)              => set_sym(22) __(8) __E_set_36(125).
		q(nt(91), (nt(22)+nt(8)+nt(125)));
		//       __E_toggle_41(134)   => 't' 'o' 'g'.
		q(nt(134), (t(13)+t(28)+t(22)));
		//       __E_toggle_41(134)   => 't' 'o' 'g' 'g' 'l' 'e'.
		q(nt(134), (t(13)+t(28)+t(22)+t(22)+t(21)+t(18)));
		//       toggle_sym(25)       => __E_toggle_41(134).
		q(nt(25), (nt(134)));
		//       toggle(92)           => toggle_sym(25) __(8) bool_option(126).
		q(nt(92), (nt(25)+nt(8)+nt(126)));
		//       __E_enable_42(135)   => 'e' 'n' __(8).
		q(nt(135), (t(18)+t(11)+nt(8)));
		//       __E_enable_42(135)   => 'e' 'n' 'a' 'b' 'l' 'e' __(8).
		q(nt(135), (t(18)+t(11)+t(16)+t(9)+t(21)+t(18)+nt(8)));
		//       enable_sym(26)       => __E_enable_42(135).
		q(nt(26), (nt(135)));
		//       enable(93)           => enable_sym(26) bool_option(126).
		q(nt(93), (nt(26)+nt(126)));
		//       __E_disable_43(136)  => 'd' 'i' 's' __(8).
		q(nt(136), (t(29)+t(20)+t(17)+nt(8)));
		//       __E_disable_43(136)  => 'd' 'i' 's' 'a' 'b' 'l' 'e' __(8).
		q(nt(136), (t(29)+t(20)+t(17)+t(16)+t(9)+t(21)+t(18)+nt(8)));
		//       disable_sym(27)      => __E_disable_43(136).
		q(nt(27), (nt(136)));
		//       disable(94)          => disable_sym(27) bool_option(126).
		q(nt(94), (nt(27)+nt(126)));
		//       cmd_symbol(112)      => grammar_sym(9).
		q(nt(112), (nt(9)));
		//       cmd_symbol(112)      => internal_grammar_sym(10).
		q(nt(112), (nt(10)));
		//       cmd_symbol(112)      => unreachable_sym(11).
		q(nt(112), (nt(11)));
		//       cmd_symbol(112)      => start_sym(12).
		q(nt(112), (nt(12)));
		//       cmd_symbol(112)      => parse_sym(13).
		q(nt(112), (nt(13)));
		//       cmd_symbol(112)      => parse_file_sym(14).
		q(nt(112), (nt(14)));
		//       cmd_symbol(112)      => load_sym(15).
		q(nt(112), (nt(15)));
		//       cmd_symbol(112)      => reload_sym(16).
		q(nt(112), (nt(16)));
		//       cmd_symbol(112)      => help_sym(17).
		q(nt(112), (nt(17)));
		//       cmd_symbol(112)      => quit_sym(18).
		q(nt(112), (nt(18)));
		//       cmd_symbol(112)      => version_sym(19).
		q(nt(112), (nt(19)));
		//       cmd_symbol(112)      => clear_sym(20).
		q(nt(112), (nt(20)));
		//       cmd_symbol(112)      => get_sym(21).
		q(nt(112), (nt(21)));
		//       cmd_symbol(112)      => set_sym(22).
		q(nt(112), (nt(22)));
		//       cmd_symbol(112)      => add_sym(23).
		q(nt(112), (nt(23)));
		//       cmd_symbol(112)      => del_sym(24).
		q(nt(112), (nt(24)));
		//       cmd_symbol(112)      => toggle_sym(25).
		q(nt(112), (nt(25)));
		//       cmd_symbol(112)      => enable_sym(26).
		q(nt(112), (nt(26)));
		//       cmd_symbol(112)      => disable_sym(27).
		q(nt(112), (nt(27)));
		//       option(117)          => bool_option(126).
		q(nt(117), (nt(126)));
		//       option(117)          => enum_ev_option(131).
		q(nt(117), (nt(131)));
		//       option(117)          => list_option(119).
		q(nt(117), (nt(119)));
		//       option(117)          => treepaths_option(121).
		q(nt(117), (nt(121)));
		//       __E_enum_ev_option_44(137) => 'e'.
		q(nt(137), (t(18)));
		//       __E_enum_ev_option_44(137) => 'e' 'r' 'r' 'o' 'r' '-' 'v' 'e' 'r' 'b' 'o' 's' 'i' 't' 'y'.
		q(nt(137), (t(18)+t(12)+t(12)+t(28)+t(12)+t(24)+t(30)+t(18)+t(12)+t(9)+t(28)+t(17)+t(20)+t(13)+t(34)));
		//       error_verbosity_opt(33) => __E_enum_ev_option_44(137).
		q(nt(33), (nt(137)));
		//       enum_ev_option(131)  => error_verbosity_opt(33).
		q(nt(131), (nt(33)));
		//       __E_bool_option_45(138) => 's'.
		q(nt(138), (t(17)));
		//       __E_bool_option_45(138) => 's' 't' 'a' 't' 'u' 's'.
		q(nt(138), (t(17)+t(13)+t(16)+t(13)+t(25)+t(17)));
		//       status_opt(34)       => __E_bool_option_45(138).
		q(nt(34), (nt(138)));
		//       bool_option(126)     => status_opt(34).
		q(nt(126), (nt(34)));
		//       __E_bool_option_46(139) => 'c'.
		q(nt(139), (t(26)));
		//       __E_bool_option_46(139) => 'c' 'o' 'l' 'o' 'r'.
		q(nt(139), (t(26)+t(28)+t(21)+t(28)+t(12)));
		//       __E_bool_option_46(139) => 'c' 'o' 'l' 'o' 'r' 's'.
		q(nt(139), (t(26)+t(28)+t(21)+t(28)+t(12)+t(17)));
		//       colors_opt(35)       => __E_bool_option_46(139).
		q(nt(35), (nt(139)));
		//       bool_option(126)     => colors_opt(35).
		q(nt(126), (nt(35)));
		//       __E_bool_option_47(140) => 'a'.
		q(nt(140), (t(16)));
		//       __E_bool_option_47(140) => 'a' 'm' 'b' 'i' 'g' 'u' 'i' 't' 'y'.
		q(nt(140), (t(16)+t(23)+t(9)+t(20)+t(22)+t(25)+t(20)+t(13)+t(34)));
		//       __E_bool_option_47(140) => 'p' 'r' 'i' 'n' 't' '-' 'a' 'm' 'b' 'i' 'g' 'u' 'i' 't' 'y'.
		q(nt(140), (t(15)+t(12)+t(20)+t(11)+t(13)+t(24)+t(16)+t(23)+t(9)+t(20)+t(22)+t(25)+t(20)+t(13)+t(34)));
		//       print_ambiguity_opt(36) => __E_bool_option_47(140).
		q(nt(36), (nt(140)));
		//       bool_option(126)     => print_ambiguity_opt(36).
		q(nt(126), (nt(36)));
		//       __E_bool_option_48(141) => 'g'.
		q(nt(141), (t(22)));
		//       __E_bool_option_48(141) => 'g' 'r' 'a' 'p' 'h' 's'.
		q(nt(141), (t(22)+t(12)+t(16)+t(15)+t(27)+t(17)));
		//       __E_bool_option_48(141) => 'p' 'r' 'i' 'n' 't' '-' 'g' 'r' 'a' 'p' 'h' 's'.
		q(nt(141), (t(15)+t(12)+t(20)+t(11)+t(13)+t(24)+t(22)+t(12)+t(16)+t(15)+t(27)+t(17)));
		//       print_graphs_opt(37) => __E_bool_option_48(141).
		q(nt(37), (nt(141)));
		//       bool_option(126)     => print_graphs_opt(37).
		q(nt(126), (nt(37)));
		//       __E_bool_option_49(142) => 'r'.
		q(nt(142), (t(12)));
		//       __E_bool_option_49(142) => 'r' 'u' 'l' 'e' 's'.
		q(nt(142), (t(12)+t(25)+t(21)+t(18)+t(17)));
		//       __E_bool_option_49(142) => 'p' 'r' 'i' 'n' 't' '-' 'r' 'u' 'l' 'e' 's'.
		q(nt(142), (t(15)+t(12)+t(20)+t(11)+t(13)+t(24)+t(12)+t(25)+t(21)+t(18)+t(17)));
		//       print_rules_opt(38)  => __E_bool_option_49(142).
		q(nt(38), (nt(142)));
		//       bool_option(126)     => print_rules_opt(38).
		q(nt(126), (nt(38)));
		//       __E_bool_option_50(143) => 'f'.
		q(nt(143), (t(10)));
		//       __E_bool_option_50(143) => 'f' 'a' 'c' 't' 's'.
		q(nt(143), (t(10)+t(16)+t(26)+t(13)+t(17)));
		//       __E_bool_option_50(143) => 'p' 'r' 'i' 'n' 't' '-' 'f' 'a' 'c' 't' 's'.
		q(nt(143), (t(15)+t(12)+t(20)+t(11)+t(13)+t(24)+t(10)+t(16)+t(26)+t(13)+t(17)));
		//       print_facts_opt(39)  => __E_bool_option_50(143).
		q(nt(39), (nt(143)));
		//       bool_option(126)     => print_facts_opt(39).
		q(nt(126), (nt(39)));
		//       __E_bool_option_51(144) => 't'.
		q(nt(144), (t(13)));
		//       __E_bool_option_51(144) => 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
		q(nt(144), (t(13)+t(18)+t(12)+t(23)+t(20)+t(11)+t(16)+t(21)+t(17)));
		//       __E_bool_option_51(144) => 'p' 'r' 'i' 'n' 't' '-' 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
		q(nt(144), (t(15)+t(12)+t(20)+t(11)+t(13)+t(24)+t(13)+t(18)+t(12)+t(23)+t(20)+t(11)+t(16)+t(21)+t(17)));
		//       print_terminals_opt(40) => __E_bool_option_51(144).
		q(nt(40), (nt(144)));
		//       bool_option(126)     => print_terminals_opt(40).
		q(nt(126), (nt(40)));
		//       __E_bool_option_52(145) => 'm'.
		q(nt(145), (t(23)));
		//       __E_bool_option_52(145) => 'm' 'e' 'a' 's' 'u' 'r' 'e'.
		q(nt(145), (t(23)+t(18)+t(16)+t(17)+t(25)+t(12)+t(18)));
		//       __E_bool_option_52(145) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'p' 'a' 'r' 's' 'i' 'n' 'g'.
		q(nt(145), (t(23)+t(18)+t(16)+t(17)+t(25)+t(12)+t(18)+t(24)+t(15)+t(16)+t(12)+t(17)+t(20)+t(11)+t(22)));
		//       measure_parsing_opt(41) => __E_bool_option_52(145).
		q(nt(41), (nt(145)));
		//       bool_option(126)     => measure_parsing_opt(41).
		q(nt(126), (nt(41)));
		//       __E_bool_option_53(146) => 'm' 'e'.
		q(nt(146), (t(23)+t(18)));
		//       __E_bool_option_53(146) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'e' 'a' 'c' 'h'.
		q(nt(146), (t(23)+t(18)+t(16)+t(17)+t(25)+t(12)+t(18)+t(24)+t(18)+t(16)+t(26)+t(27)));
		//       __E_bool_option_53(146) => 'm' 'e' 'p'.
		q(nt(146), (t(23)+t(18)+t(15)));
		//       __E_bool_option_53(146) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'e' 'a' 'c' 'h' '-' 'p' 'o' 's'.
		q(nt(146), (t(23)+t(18)+t(16)+t(17)+t(25)+t(12)+t(18)+t(24)+t(18)+t(16)+t(26)+t(27)+t(24)+t(15)+t(28)+t(17)));
		//       measure_each_pos_opt(42) => __E_bool_option_53(146).
		q(nt(42), (nt(146)));
		//       bool_option(126)     => measure_each_pos_opt(42).
		q(nt(126), (nt(42)));
		//       __E_bool_option_54(147) => 'm' 'f'.
		q(nt(147), (t(23)+t(10)));
		//       __E_bool_option_54(147) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'f' 'o' 'r' 'e' 's' 't'.
		q(nt(147), (t(23)+t(18)+t(16)+t(17)+t(25)+t(12)+t(18)+t(24)+t(10)+t(28)+t(12)+t(18)+t(17)+t(13)));
		//       measure_forest_opt(43) => __E_bool_option_54(147).
		q(nt(43), (nt(147)));
		//       bool_option(126)     => measure_forest_opt(43).
		q(nt(126), (nt(43)));
		//       __E_bool_option_55(148) => 'm' 'p'.
		q(nt(148), (t(23)+t(15)));
		//       __E_bool_option_55(148) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'p' 'r' 'e' 'p' 'r' 'o' 'c' 'e' 's' 's'.
		q(nt(148), (t(23)+t(18)+t(16)+t(17)+t(25)+t(12)+t(18)+t(24)+t(15)+t(12)+t(18)+t(15)+t(12)+t(28)+t(26)+t(18)+t(17)+t(17)));
		//       measure_preprocess_opt(44) => __E_bool_option_55(148).
		q(nt(44), (nt(148)));
		//       bool_option(126)     => measure_preprocess_opt(44).
		q(nt(126), (nt(44)));
		//       __E_bool_option_56(149) => 'd'.
		q(nt(149), (t(29)));
		//       __E_bool_option_56(149) => 'd' 'e' 'b' 'u' 'g'.
		q(nt(149), (t(29)+t(18)+t(9)+t(25)+t(22)));
		//       debug_opt(45)        => __E_bool_option_56(149).
		q(nt(45), (nt(149)));
		//       bool_option(126)     => debug_opt(45).
		q(nt(126), (nt(45)));
		//       __E_bool_option_57(150) => 'a' 'd'.
		q(nt(150), (t(16)+t(29)));
		//       __E_bool_option_57(150) => 'a' 'u' 't' 'o' '-' 'd' 'i' 's' 'a' 'm' 'b' 'i' 'g' 'u' 'a' 't' 'e'.
		q(nt(150), (t(16)+t(25)+t(13)+t(28)+t(24)+t(29)+t(20)+t(17)+t(16)+t(23)+t(9)+t(20)+t(22)+t(25)+t(16)+t(13)+t(18)));
		//       auto_disambiguate_opt(46) => __E_bool_option_57(150).
		q(nt(46), (nt(150)));
		//       bool_option(126)     => auto_disambiguate_opt(46).
		q(nt(126), (nt(46)));
		//       __E_bool_option_58(151) => 't' 't'.
		q(nt(151), (t(13)+t(13)));
		//       __E_bool_option_58(151) => 't' 'r' 'i' 'm' '-' 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
		q(nt(151), (t(13)+t(12)+t(20)+t(23)+t(24)+t(13)+t(18)+t(12)+t(23)+t(20)+t(11)+t(16)+t(21)+t(17)));
		//       trim_terminals_opt(47) => __E_bool_option_58(151).
		q(nt(47), (nt(151)));
		//       bool_option(126)     => trim_terminals_opt(47).
		q(nt(126), (nt(47)));
		//       __E_bool_option_59(152) => 'i' 'c' 'c'.
		q(nt(152), (t(20)+t(26)+t(26)));
		//       __E_bool_option_59(152) => 'i' 'n' 'l' 'i' 'n' 'e' '-' 'c' 'c'.
		q(nt(152), (t(20)+t(11)+t(21)+t(20)+t(11)+t(18)+t(24)+t(26)+t(26)));
		//       __E_bool_option_59(152) => 'i' 'n' 'l' 'i' 'n' 'e' '-' 'c' 'h' 'a' 'r' '-' 'c' 'l' 'a' 's' 's' 'e' 's'.
		q(nt(152), (t(20)+t(11)+t(21)+t(20)+t(11)+t(18)+t(24)+t(26)+t(27)+t(16)+t(12)+t(24)+t(26)+t(21)+t(16)+t(17)+t(17)+t(18)+t(17)));
		//       inline_cc_opt(48)    => __E_bool_option_59(152).
		q(nt(48), (nt(152)));
		//       bool_option(126)     => inline_cc_opt(48).
		q(nt(126), (nt(48)));
		//       __E_list_option_60(153) => 'n' 'd'.
		q(nt(153), (t(11)+t(29)));
		//       __E_list_option_60(153) => 'n' 'd' 'l'.
		q(nt(153), (t(11)+t(29)+t(21)));
		//       __E_list_option_60(153) => 'n' 'o' 'd' 'i' 's' 'a' 'm' 'b' 'i' 'g' '-' 'l' 'i' 's' 't'.
		q(nt(153), (t(11)+t(28)+t(29)+t(20)+t(17)+t(16)+t(23)+t(9)+t(20)+t(22)+t(24)+t(21)+t(20)+t(17)+t(13)));
		//       nodisambig_list_opt(49) => __E_list_option_60(153).
		q(nt(49), (nt(153)));
		//       list_option(119)     => nodisambig_list_opt(49).
		q(nt(119), (nt(49)));
		//       trim_opt(50)         => 't' 'r' 'i' 'm'.
		q(nt(50), (t(13)+t(12)+t(20)+t(23)));
		//       list_option(119)     => trim_opt(50).
		q(nt(119), (nt(50)));
		//       __E_list_option_61(154) => 't' 'c'.
		q(nt(154), (t(13)+t(26)));
		//       __E_list_option_61(154) => 't' 'r' 'i' 'm' '-' 'c' 'h' 'i' 'l' 'd' 'r' 'e' 'n'.
		q(nt(154), (t(13)+t(12)+t(20)+t(23)+t(24)+t(26)+t(27)+t(20)+t(21)+t(29)+t(12)+t(18)+t(11)));
		//       trim_children_opt(51) => __E_list_option_61(154).
		q(nt(51), (nt(154)));
		//       list_option(119)     => trim_children_opt(51).
		q(nt(119), (nt(51)));
		//       __E_treepaths_option_62(155) => 'i'.
		q(nt(155), (t(20)));
		//       __E_treepaths_option_62(155) => 'i' 'n' 'l' 'i' 'n' 'e'.
		q(nt(155), (t(20)+t(11)+t(21)+t(20)+t(11)+t(18)));
		//       inline_opt(52)       => __E_treepaths_option_62(155).
		q(nt(52), (nt(155)));
		//       treepaths_option(121) => inline_opt(52).
		q(nt(121), (nt(52)));
		//       __E_bool_value_63(156) => 't'.
		q(nt(156), (t(13)));
		//       __E_bool_value_63(156) => 't' 'r' 'u' 'e'.
		q(nt(156), (t(13)+t(12)+t(25)+t(18)));
		//       __E_bool_value_63(156) => 'o' 'n'.
		q(nt(156), (t(28)+t(11)));
		//       __E_bool_value_63(156) => '1'.
		q(nt(156), (t(35)));
		//       __E_bool_value_63(156) => 'y'.
		q(nt(156), (t(34)));
		//       __E_bool_value_63(156) => 'y' 'e' 's'.
		q(nt(156), (t(34)+t(18)+t(17)));
		//       true_value(28)       => __E_bool_value_63(156).
		q(nt(28), (nt(156)));
		//       bool_value(128)      => true_value(28).
		q(nt(128), (nt(28)));
		//       __E_bool_value_64(157) => 'f'.
		q(nt(157), (t(10)));
		//       __E_bool_value_64(157) => 'f' 'a' 'l' 's' 'e'.
		q(nt(157), (t(10)+t(16)+t(21)+t(17)+t(18)));
		//       __E_bool_value_64(157) => 'o' 'f' 'f'.
		q(nt(157), (t(28)+t(10)+t(10)));
		//       __E_bool_value_64(157) => '0'.
		q(nt(157), (t(36)));
		//       __E_bool_value_64(157) => 'n'.
		q(nt(157), (t(11)));
		//       __E_bool_value_64(157) => 'n' 'o'.
		q(nt(157), (t(11)+t(28)));
		//       false_value(29)      => __E_bool_value_64(157).
		q(nt(29), (nt(157)));
		//       bool_value(128)      => false_value(29).
		q(nt(128), (nt(29)));
		//       __E_symbol_list_65(158) => _(7) ',' _(7) symbol(73).
		q(nt(158), (nt(7)+t(37)+nt(7)+nt(73)));
		//       __E_symbol_list_66(159) => null.
		q(nt(159), (nul));
		//       __E_symbol_list_66(159) => __E_symbol_list_65(158) __E_symbol_list_66(159).
		q(nt(159), (nt(158)+nt(159)));
		//       symbol_list(120)     => symbol(73) __E_symbol_list_66(159).
		q(nt(120), (nt(73)+nt(159)));
		//       __E_treepath_list_67(161) => _(7) ',' _(7) treepath(160).
		q(nt(161), (nt(7)+t(37)+nt(7)+nt(160)));
		//       __E_treepath_list_68(162) => null.
		q(nt(162), (nul));
		//       __E_treepath_list_68(162) => __E_treepath_list_67(161) __E_treepath_list_68(162).
		q(nt(162), (nt(161)+nt(162)));
		//       treepath_list(122)   => treepath(160) __E_treepath_list_68(162).
		q(nt(122), (nt(160)+nt(162)));
		//       __E_treepath_69(163) => _(7) '>' _(7) symbol(73).
		q(nt(163), (nt(7)+t(38)+nt(7)+nt(73)));
		//       __E_treepath_70(164) => null.
		q(nt(164), (nul));
		//       __E_treepath_70(164) => __E_treepath_69(163) __E_treepath_70(164).
		q(nt(164), (nt(163)+nt(164)));
		//       treepath(160)        => symbol(73) __E_treepath_70(164).
		q(nt(160), (nt(73)+nt(164)));
		//       __E_error_verbosity_71(165) => 'b'.
		q(nt(165), (t(9)));
		//       __E_error_verbosity_71(165) => 'b' 'a' 's' 'i' 'c'.
		q(nt(165), (t(9)+t(16)+t(17)+t(20)+t(26)));
		//       basic_sym(30)        => __E_error_verbosity_71(165).
		q(nt(30), (nt(165)));
		//       error_verbosity(133) => basic_sym(30).
		q(nt(133), (nt(30)));
		//       __E_error_verbosity_72(166) => 'd'.
		q(nt(166), (t(29)));
		//       __E_error_verbosity_72(166) => 'd' 'e' 't' 'a' 'i' 'l' 'e' 'd'.
		q(nt(166), (t(29)+t(18)+t(13)+t(16)+t(20)+t(21)+t(18)+t(29)));
		//       detailed_sym(31)     => __E_error_verbosity_72(166).
		q(nt(31), (nt(166)));
		//       error_verbosity(133) => detailed_sym(31).
		q(nt(133), (nt(31)));
		//       __E_error_verbosity_73(167) => 'r'.
		q(nt(167), (t(12)));
		//       __E_error_verbosity_73(167) => 'r' 'c'.
		q(nt(167), (t(12)+t(26)));
		//       __E_error_verbosity_73(167) => 'r' 'o' 'o' 't' '-' 'c' 'a' 'u' 's' 'e'.
		q(nt(167), (t(12)+t(28)+t(28)+t(13)+t(24)+t(26)+t(16)+t(25)+t(17)+t(18)));
		//       root_cause_sym(32)   => __E_error_verbosity_73(167).
		q(nt(32), (nt(167)));
		//       error_verbosity(133) => root_cause_sym(32).
		q(nt(133), (nt(32)));
		return q;
	}
};

#endif // __TGF_REPL_PARSER_H__
