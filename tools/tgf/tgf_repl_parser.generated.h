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
	using input_type      = parser_type::input;
	using decoder_type    = parser_type::input::decoder_type;
	using encoder_type    = std::function<std::basic_string<char_type>(
			const std::vector<terminal_type>&)>;
	tgf_repl_parser() :
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
		nul, alnum, alpha, eof, printable, space, start, _, statement, _Rstart_0, 
		_Rstart_1, _Rstart_2, _Rstart_3, __, _R__4, comment, _R___5, _Rcomment_6, _Rcomment_7, _Rcomment_8, 
		quoted_string, unescaped_s, escaped_s, _Rquoted_string_9, _Rquoted_string_10, _Runescaped_s_11, _Rescaped_s_12, filename, symbol, _Rsymbol_13, 
		_Rsymbol_14, _Rsymbol_15, parse_input_char, parse_input, _Rparse_input_16, grammar_cmd, internal_grammar_cmd, unreachable_cmd, reload_cmd, load_cmd, 
		start_cmd, help, version, quit, get, set, toggle, enable, disable, add, 
		del, parse_file_cmd, parse_cmd, parse_sym, parse_file_sym, grammar_sym, internal_grammar_sym, _Rinternal_grammar_cmd_17, start_sym, _Rstart_cmd_18, 
		unreachable_sym, _Runreachable_cmd_19, reload_sym, load_sym, help_sym, cmd_symbol, _Rhelp_20, version_sym, quit_sym, add_sym, 
		list_option, symbol_list, del_sym, get_sym, option, _Rget_21, set_sym, _Rset_22, option_value, toggle_sym, 
		bool_option, enable_sym, disable_sym, error_verbosity_opt, status_opt, colors_opt, print_ambiguity_opt, print_graphs_opt, print_rules_opt, print_facts_opt, 
		print_terminals_opt, measure_parsing_opt, measure_each_pos_opt, measure_forest_opt, measure_preprocess_opt, debug_opt, auto_disambiguate_opt, nodisambig_list_opt, option_value_true, option_value_false, 
		error_verbosity, basic_sym, detailed_sym, root_cause_sym, _Rsymbol_list_23, _Rsymbol_list_24, __neg_0, 
	};
	size_t id(const std::basic_string<char_type>& name) {
		return nts.get(name);
	}
	const std::basic_string<char_type>& name(size_t id) {
		return nts.get(id);
	}
private:
	std::vector<terminal_type> ts{
		'\0', '.', '\t', '\n', '\r', '#', '"', '\\', '/', 
		'b', 'f', 'n', 'r', 't', '_', '=', 'g', 'e', 's', 
		'o', 'l', 'a', 'm', 'i', '-', 'u', 'c', 'h', 'p', 
		' ', 'd', 'v', 'x', 'q', 'y', '1', '0', ',', 
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
			"", "alnum", "alpha", "eof", "printable", "space", "start", "_", "statement", "_Rstart_0", 
			"_Rstart_1", "_Rstart_2", "_Rstart_3", "__", "_R__4", "comment", "_R___5", "_Rcomment_6", "_Rcomment_7", "_Rcomment_8", 
			"quoted_string", "unescaped_s", "escaped_s", "_Rquoted_string_9", "_Rquoted_string_10", "_Runescaped_s_11", "_Rescaped_s_12", "filename", "symbol", "_Rsymbol_13", 
			"_Rsymbol_14", "_Rsymbol_15", "parse_input_char", "parse_input", "_Rparse_input_16", "grammar_cmd", "internal_grammar_cmd", "unreachable_cmd", "reload_cmd", "load_cmd", 
			"start_cmd", "help", "version", "quit", "get", "set", "toggle", "enable", "disable", "add", 
			"del", "parse_file_cmd", "parse_cmd", "parse_sym", "parse_file_sym", "grammar_sym", "internal_grammar_sym", "_Rinternal_grammar_cmd_17", "start_sym", "_Rstart_cmd_18", 
			"unreachable_sym", "_Runreachable_cmd_19", "reload_sym", "load_sym", "help_sym", "cmd_symbol", "_Rhelp_20", "version_sym", "quit_sym", "add_sym", 
			"list_option", "symbol_list", "del_sym", "get_sym", "option", "_Rget_21", "set_sym", "_Rset_22", "option_value", "toggle_sym", 
			"bool_option", "enable_sym", "disable_sym", "error_verbosity_opt", "status_opt", "colors_opt", "print_ambiguity_opt", "print_graphs_opt", "print_rules_opt", "print_facts_opt", 
			"print_terminals_opt", "measure_parsing_opt", "measure_each_pos_opt", "measure_forest_opt", "measure_preprocess_opt", "debug_opt", "auto_disambiguate_opt", "nodisambig_list_opt", "option_value_true", "option_value_false", 
			"error_verbosity", "basic_sym", "detailed_sym", "root_cause_sym", "_Rsymbol_list_23", "_Rsymbol_list_24", "__neg_0", 
		}) nts.get(nt);
		return nts;
	}
	idni::char_class_fns<terminal_type> load_cc() {
		return idni::predefined_char_classes<char_type, terminal_type>({
			"alnum",
			"alpha",
			"eof",
			"printable",
			"space",
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
		//       _Rstart_0(9)         => _(7) '.' _(7) statement(8).
		q(nt(9), (nt(7)+t(1)+nt(7)+nt(8)));
		//       _Rstart_1(10)        => null.
		q(nt(10), (nul));
		//       _Rstart_1(10)        => _Rstart_0(9) _Rstart_1(10).
		q(nt(10), (nt(9)+nt(10)));
		//       _Rstart_2(11)        => null.
		q(nt(11), (nul));
		//       _Rstart_2(11)        => _(7) '.'.
		q(nt(11), (nt(7)+t(1)));
		//       _Rstart_3(12)        => null.
		q(nt(12), (nul));
		//       _Rstart_3(12)        => _Rstart_2(11) _(7).
		q(nt(12), (nt(11)+nt(7)));
		//       start(6)             => _(7) statement(8) _Rstart_1(10) _Rstart_3(12).
		q(nt(6), (nt(7)+nt(8)+nt(10)+nt(12)));
		//       _R__4(14)            => null.
		q(nt(14), (nul));
		//       _R__4(14)            => __(13).
		q(nt(14), (nt(13)));
		//       _(7)                 => _R__4(14).
		q(nt(7), (nt(14)));
		//       _R___5(16)           => space(5).
		q(nt(16), (nt(5)));
		//       _R___5(16)           => comment(15).
		q(nt(16), (nt(15)));
		//       __(13)               => _R___5(16) _(7).
		q(nt(13), (nt(16)+nt(7)));
		//       _Rcomment_6(17)      => '\t'.
		q(nt(17), (t(2)));
		//       _Rcomment_6(17)      => printable(4).
		q(nt(17), (nt(4)));
		//       _Rcomment_7(18)      => null.
		q(nt(18), (nul));
		//       _Rcomment_7(18)      => _Rcomment_6(17) _Rcomment_7(18).
		q(nt(18), (nt(17)+nt(18)));
		//       _Rcomment_8(19)      => '\n'.
		q(nt(19), (t(3)));
		//       _Rcomment_8(19)      => '\r'.
		q(nt(19), (t(4)));
		//       _Rcomment_8(19)      => eof(3).
		q(nt(19), (nt(3)));
		//       comment(15)          => '#' _Rcomment_7(18) _Rcomment_8(19).
		q(nt(15), (t(5)+nt(18)+nt(19)));
		//       _Rquoted_string_9(23) => unescaped_s(21).
		q(nt(23), (nt(21)));
		//       _Rquoted_string_9(23) => escaped_s(22).
		q(nt(23), (nt(22)));
		//       _Rquoted_string_10(24) => null.
		q(nt(24), (nul));
		//       _Rquoted_string_10(24) => _Rquoted_string_9(23) _Rquoted_string_10(24).
		q(nt(24), (nt(23)+nt(24)));
		//       quoted_string(20)    => '"' _Rquoted_string_10(24) '"'.
		q(nt(20), (t(6)+nt(24)+t(6)));
		//       _Runescaped_s_11(25) => '"'.
		q(nt(25), (t(6)));
		//       _Runescaped_s_11(25) => '\\'.
		q(nt(25), (t(7)));
		//       __neg_0(106)         => _Runescaped_s_11(25).
		q(nt(106), (nt(25)));
		//       unescaped_s(21)      => printable(4) & ~( __neg_0(106) ).	 # conjunctive
		q(nt(21), (nt(4)) & ~(nt(106)));
		//       _Rescaped_s_12(26)   => '"'.
		q(nt(26), (t(6)));
		//       _Rescaped_s_12(26)   => '/'.
		q(nt(26), (t(8)));
		//       _Rescaped_s_12(26)   => '\\'.
		q(nt(26), (t(7)));
		//       _Rescaped_s_12(26)   => 'b'.
		q(nt(26), (t(9)));
		//       _Rescaped_s_12(26)   => 'f'.
		q(nt(26), (t(10)));
		//       _Rescaped_s_12(26)   => 'n'.
		q(nt(26), (t(11)));
		//       _Rescaped_s_12(26)   => 'r'.
		q(nt(26), (t(12)));
		//       _Rescaped_s_12(26)   => 't'.
		q(nt(26), (t(13)));
		//       escaped_s(22)        => '\\' _Rescaped_s_12(26).
		q(nt(22), (t(7)+nt(26)));
		//       filename(27)         => quoted_string(20).
		q(nt(27), (nt(20)));
		//       _Rsymbol_13(29)      => '_'.
		q(nt(29), (t(14)));
		//       _Rsymbol_13(29)      => alpha(2).
		q(nt(29), (nt(2)));
		//       _Rsymbol_14(30)      => '_'.
		q(nt(30), (t(14)));
		//       _Rsymbol_14(30)      => alnum(1).
		q(nt(30), (nt(1)));
		//       _Rsymbol_15(31)      => null.
		q(nt(31), (nul));
		//       _Rsymbol_15(31)      => _Rsymbol_14(30) _Rsymbol_15(31).
		q(nt(31), (nt(30)+nt(31)));
		//       symbol(28)           => _Rsymbol_13(29) _Rsymbol_15(31).
		q(nt(28), (nt(29)+nt(31)));
		//       parse_input_char(32) => '\t'.
		q(nt(32), (t(2)));
		//       parse_input_char(32) => printable(4).
		q(nt(32), (nt(4)));
		//       _Rparse_input_16(34) => null.
		q(nt(34), (nul));
		//       _Rparse_input_16(34) => parse_input_char(32) _Rparse_input_16(34).
		q(nt(34), (nt(32)+nt(34)));
		//       parse_input(33)      => _Rparse_input_16(34).
		q(nt(33), (nt(34)));
		//       statement(8)         => grammar_cmd(35).
		q(nt(8), (nt(35)));
		//       statement(8)         => internal_grammar_cmd(36).
		q(nt(8), (nt(36)));
		//       statement(8)         => unreachable_cmd(37).
		q(nt(8), (nt(37)));
		//       statement(8)         => reload_cmd(38).
		q(nt(8), (nt(38)));
		//       statement(8)         => load_cmd(39).
		q(nt(8), (nt(39)));
		//       statement(8)         => start_cmd(40).
		q(nt(8), (nt(40)));
		//       statement(8)         => help(41).
		q(nt(8), (nt(41)));
		//       statement(8)         => version(42).
		q(nt(8), (nt(42)));
		//       statement(8)         => quit(43).
		q(nt(8), (nt(43)));
		//       statement(8)         => get(44).
		q(nt(8), (nt(44)));
		//       statement(8)         => set(45).
		q(nt(8), (nt(45)));
		//       statement(8)         => toggle(46).
		q(nt(8), (nt(46)));
		//       statement(8)         => enable(47).
		q(nt(8), (nt(47)));
		//       statement(8)         => disable(48).
		q(nt(8), (nt(48)));
		//       statement(8)         => add(49).
		q(nt(8), (nt(49)));
		//       statement(8)         => del(50).
		q(nt(8), (nt(50)));
		//       statement(8)         => parse_file_cmd(51).
		q(nt(8), (nt(51)));
		//       statement(8)         => parse_cmd(52).
		q(nt(8), (nt(52)));
		//       parse_cmd(52)        => parse_sym(53) __(13) parse_input(33).
		q(nt(52), (nt(53)+nt(13)+nt(33)));
		//       parse_file_cmd(51)   => parse_file_sym(54) __(13) filename(27).
		q(nt(51), (nt(54)+nt(13)+nt(27)));
		//       grammar_cmd(35)      => grammar_sym(55).
		q(nt(35), (nt(55)));
		//       _Rinternal_grammar_cmd_17(57) => null.
		q(nt(57), (nul));
		//       _Rinternal_grammar_cmd_17(57) => __(13) symbol(28).
		q(nt(57), (nt(13)+nt(28)));
		//       internal_grammar_cmd(36) => internal_grammar_sym(56) _Rinternal_grammar_cmd_17(57).
		q(nt(36), (nt(56)+nt(57)));
		//       _Rstart_cmd_18(59)   => null.
		q(nt(59), (nul));
		//       _Rstart_cmd_18(59)   => __(13) symbol(28).
		q(nt(59), (nt(13)+nt(28)));
		//       start_cmd(40)        => start_sym(58) _Rstart_cmd_18(59).
		q(nt(40), (nt(58)+nt(59)));
		//       _Runreachable_cmd_19(61) => null.
		q(nt(61), (nul));
		//       _Runreachable_cmd_19(61) => __(13) symbol(28).
		q(nt(61), (nt(13)+nt(28)));
		//       unreachable_cmd(37)  => unreachable_sym(60) _Runreachable_cmd_19(61).
		q(nt(37), (nt(60)+nt(61)));
		//       reload_cmd(38)       => reload_sym(62).
		q(nt(38), (nt(62)));
		//       load_cmd(39)         => load_sym(63) __(13) filename(27).
		q(nt(39), (nt(63)+nt(13)+nt(27)));
		//       _Rhelp_20(66)        => null.
		q(nt(66), (nul));
		//       _Rhelp_20(66)        => __(13) cmd_symbol(65).
		q(nt(66), (nt(13)+nt(65)));
		//       help(41)             => help_sym(64) _Rhelp_20(66).
		q(nt(41), (nt(64)+nt(66)));
		//       version(42)          => version_sym(67).
		q(nt(42), (nt(67)));
		//       quit(43)             => quit_sym(68).
		q(nt(43), (nt(68)));
		//       add(49)              => add_sym(69) __(13) list_option(70) __(13) symbol_list(71).
		q(nt(49), (nt(69)+nt(13)+nt(70)+nt(13)+nt(71)));
		//       del(50)              => del_sym(72) __(13) list_option(70) __(13) symbol_list(71).
		q(nt(50), (nt(72)+nt(13)+nt(70)+nt(13)+nt(71)));
		//       _Rget_21(75)         => null.
		q(nt(75), (nul));
		//       _Rget_21(75)         => __(13) option(74).
		q(nt(75), (nt(13)+nt(74)));
		//       get(44)              => get_sym(73) _Rget_21(75).
		q(nt(44), (nt(73)+nt(75)));
		//       _Rset_22(77)         => _(7) '=' _(7).
		q(nt(77), (nt(7)+t(15)+nt(7)));
		//       _Rset_22(77)         => __(13).
		q(nt(77), (nt(13)));
		//       set(45)              => set_sym(76) __(13) option(74) _Rset_22(77) option_value(78).
		q(nt(45), (nt(76)+nt(13)+nt(74)+nt(77)+nt(78)));
		//       toggle(46)           => toggle_sym(79) __(13) bool_option(80).
		q(nt(46), (nt(79)+nt(13)+nt(80)));
		//       enable(47)           => enable_sym(81) bool_option(80).
		q(nt(47), (nt(81)+nt(80)));
		//       disable(48)          => disable_sym(82) bool_option(80).
		q(nt(48), (nt(82)+nt(80)));
		//       get_sym(73)          => 'g' 'e' 't'.
		q(nt(73), (t(16)+t(17)+t(13)));
		//       set_sym(76)          => 's' 'e' 't'.
		q(nt(76), (t(18)+t(17)+t(13)));
		//       toggle_sym(79)       => 't' 'o' 'g' 'g' 'l' 'e'.
		q(nt(79), (t(13)+t(19)+t(16)+t(16)+t(20)+t(17)));
		//       grammar_sym(55)      => 'g'.
		q(nt(55), (t(16)));
		//       grammar_sym(55)      => 'g' 'r' 'a' 'm' 'm' 'a' 'r'.
		q(nt(55), (t(16)+t(12)+t(21)+t(22)+t(22)+t(21)+t(12)));
		//       internal_grammar_sym(56) => 'i'.
		q(nt(56), (t(23)));
		//       internal_grammar_sym(56) => 'i' 'g'.
		q(nt(56), (t(23)+t(16)));
		//       internal_grammar_sym(56) => 'i' 'n' 't' 'e' 'r' 'n' 'a' 'l' '-' 'g' 'r' 'a' 'm' 'm' 'a' 'r'.
		q(nt(56), (t(23)+t(11)+t(13)+t(17)+t(12)+t(11)+t(21)+t(20)+t(24)+t(16)+t(12)+t(21)+t(22)+t(22)+t(21)+t(12)));
		//       unreachable_sym(60)  => 'u'.
		q(nt(60), (t(25)));
		//       unreachable_sym(60)  => 'u' 'n' 'r' 'e' 'a' 'c' 'h' 'a' 'b' 'l' 'e'.
		q(nt(60), (t(25)+t(11)+t(12)+t(17)+t(21)+t(26)+t(27)+t(21)+t(9)+t(20)+t(17)));
		//       parse_sym(53)        => 'p'.
		q(nt(53), (t(28)));
		//       parse_sym(53)        => 'p' 'a' 'r' 's' 'e'.
		q(nt(53), (t(28)+t(21)+t(12)+t(18)+t(17)));
		//       parse_file_sym(54)   => 'f'.
		q(nt(54), (t(10)));
		//       parse_file_sym(54)   => 'p' 'a' 'r' 's' 'e' ' ' 'f' 'i' 'l' 'e'.
		q(nt(54), (t(28)+t(21)+t(12)+t(18)+t(17)+t(29)+t(10)+t(23)+t(20)+t(17)));
		//       parse_file_sym(54)   => 'p' 'f'.
		q(nt(54), (t(28)+t(10)));
		//       start_sym(58)        => 's'.
		q(nt(58), (t(18)));
		//       start_sym(58)        => 's' 't' 'a' 'r' 't'.
		q(nt(58), (t(18)+t(13)+t(21)+t(12)+t(13)));
		//       reload_sym(62)       => 'r'.
		q(nt(62), (t(12)));
		//       reload_sym(62)       => 'r' 'e' 'l' 'o' 'a' 'd'.
		q(nt(62), (t(12)+t(17)+t(20)+t(19)+t(21)+t(30)));
		//       load_sym(63)         => 'l'.
		q(nt(63), (t(20)));
		//       load_sym(63)         => 'l' 'o' 'a' 'd'.
		q(nt(63), (t(20)+t(19)+t(21)+t(30)));
		//       help_sym(64)         => 'h'.
		q(nt(64), (t(27)));
		//       help_sym(64)         => 'h' 'e' 'l' 'p'.
		q(nt(64), (t(27)+t(17)+t(20)+t(28)));
		//       version_sym(67)      => 'v'.
		q(nt(67), (t(31)));
		//       version_sym(67)      => 'v' 'e' 'r' 's' 'i' 'o' 'n'.
		q(nt(67), (t(31)+t(17)+t(12)+t(18)+t(23)+t(19)+t(11)));
		//       quit_sym(68)         => 'e'.
		q(nt(68), (t(17)));
		//       quit_sym(68)         => 'e' 'x' 'i' 't'.
		q(nt(68), (t(17)+t(32)+t(23)+t(13)));
		//       quit_sym(68)         => 'q'.
		q(nt(68), (t(33)));
		//       quit_sym(68)         => 'q' 'u' 'i' 't'.
		q(nt(68), (t(33)+t(25)+t(23)+t(13)));
		//       get_sym(73)          => 'g' 'e' 't'.
		q(nt(73), (t(16)+t(17)+t(13)));
		//       set_sym(76)          => 's' 'e' 't'.
		q(nt(76), (t(18)+t(17)+t(13)));
		//       toggle_sym(79)       => 't' 'o' 'g'.
		q(nt(79), (t(13)+t(19)+t(16)));
		//       toggle_sym(79)       => 't' 'o' 'g' 'g' 'l' 'e'.
		q(nt(79), (t(13)+t(19)+t(16)+t(16)+t(20)+t(17)));
		//       add_sym(69)          => 'a' 'd' 'd'.
		q(nt(69), (t(21)+t(30)+t(30)));
		//       del_sym(72)          => 'd' 'e' 'l'.
		q(nt(72), (t(30)+t(17)+t(20)));
		//       del_sym(72)          => 'd' 'e' 'l' 'e' 't' 'e'.
		q(nt(72), (t(30)+t(17)+t(20)+t(17)+t(13)+t(17)));
		//       del_sym(72)          => 'r' 'e' 'm'.
		q(nt(72), (t(12)+t(17)+t(22)));
		//       del_sym(72)          => 'r' 'e' 'm' 'o' 'v' 'e'.
		q(nt(72), (t(12)+t(17)+t(22)+t(19)+t(31)+t(17)));
		//       del_sym(72)          => 'r' 'm'.
		q(nt(72), (t(12)+t(22)));
		//       enable_sym(81)       => 'e' 'n' 'a' 'b' 'l' 'e' __(13).
		q(nt(81), (t(17)+t(11)+t(21)+t(9)+t(20)+t(17)+nt(13)));
		//       enable_sym(81)       => 'e' 'n' __(13).
		q(nt(81), (t(17)+t(11)+nt(13)));
		//       disable_sym(82)      => 'd' 'i' 's' 'a' 'b' 'l' 'e' __(13).
		q(nt(82), (t(30)+t(23)+t(18)+t(21)+t(9)+t(20)+t(17)+nt(13)));
		//       disable_sym(82)      => 'd' 'i' 's' __(13).
		q(nt(82), (t(30)+t(23)+t(18)+nt(13)));
		//       cmd_symbol(65)       => parse_sym(53).
		q(nt(65), (nt(53)));
		//       cmd_symbol(65)       => parse_file_sym(54).
		q(nt(65), (nt(54)));
		//       cmd_symbol(65)       => grammar_sym(55).
		q(nt(65), (nt(55)));
		//       cmd_symbol(65)       => internal_grammar_sym(56).
		q(nt(65), (nt(56)));
		//       cmd_symbol(65)       => start_sym(58).
		q(nt(65), (nt(58)));
		//       cmd_symbol(65)       => unreachable_sym(60).
		q(nt(65), (nt(60)));
		//       cmd_symbol(65)       => reload_sym(62).
		q(nt(65), (nt(62)));
		//       cmd_symbol(65)       => load_sym(63).
		q(nt(65), (nt(63)));
		//       cmd_symbol(65)       => help_sym(64).
		q(nt(65), (nt(64)));
		//       cmd_symbol(65)       => version_sym(67).
		q(nt(65), (nt(67)));
		//       cmd_symbol(65)       => quit_sym(68).
		q(nt(65), (nt(68)));
		//       cmd_symbol(65)       => add_sym(69).
		q(nt(65), (nt(69)));
		//       cmd_symbol(65)       => del_sym(72).
		q(nt(65), (nt(72)));
		//       cmd_symbol(65)       => get_sym(73).
		q(nt(65), (nt(73)));
		//       cmd_symbol(65)       => set_sym(76).
		q(nt(65), (nt(76)));
		//       cmd_symbol(65)       => toggle_sym(79).
		q(nt(65), (nt(79)));
		//       cmd_symbol(65)       => enable_sym(81).
		q(nt(65), (nt(81)));
		//       cmd_symbol(65)       => disable_sym(82).
		q(nt(65), (nt(82)));
		//       option(74)           => list_option(70).
		q(nt(74), (nt(70)));
		//       option(74)           => bool_option(80).
		q(nt(74), (nt(80)));
		//       option(74)           => error_verbosity_opt(83).
		q(nt(74), (nt(83)));
		//       bool_option(80)      => status_opt(84).
		q(nt(80), (nt(84)));
		//       bool_option(80)      => colors_opt(85).
		q(nt(80), (nt(85)));
		//       bool_option(80)      => print_ambiguity_opt(86).
		q(nt(80), (nt(86)));
		//       bool_option(80)      => print_graphs_opt(87).
		q(nt(80), (nt(87)));
		//       bool_option(80)      => print_rules_opt(88).
		q(nt(80), (nt(88)));
		//       bool_option(80)      => print_facts_opt(89).
		q(nt(80), (nt(89)));
		//       bool_option(80)      => print_terminals_opt(90).
		q(nt(80), (nt(90)));
		//       bool_option(80)      => measure_parsing_opt(91).
		q(nt(80), (nt(91)));
		//       bool_option(80)      => measure_each_pos_opt(92).
		q(nt(80), (nt(92)));
		//       bool_option(80)      => measure_forest_opt(93).
		q(nt(80), (nt(93)));
		//       bool_option(80)      => measure_preprocess_opt(94).
		q(nt(80), (nt(94)));
		//       bool_option(80)      => debug_opt(95).
		q(nt(80), (nt(95)));
		//       bool_option(80)      => auto_disambiguate_opt(96).
		q(nt(80), (nt(96)));
		//       list_option(70)      => nodisambig_list_opt(97).
		q(nt(70), (nt(97)));
		//       debug_opt(95)        => 'd'.
		q(nt(95), (t(30)));
		//       debug_opt(95)        => 'd' 'e' 'b' 'u' 'g'.
		q(nt(95), (t(30)+t(17)+t(9)+t(25)+t(16)));
		//       status_opt(84)       => 's'.
		q(nt(84), (t(18)));
		//       status_opt(84)       => 's' 't' 'a' 't' 'u' 's'.
		q(nt(84), (t(18)+t(13)+t(21)+t(13)+t(25)+t(18)));
		//       colors_opt(85)       => 'c'.
		q(nt(85), (t(26)));
		//       colors_opt(85)       => 'c' 'o' 'l' 'o' 'r'.
		q(nt(85), (t(26)+t(19)+t(20)+t(19)+t(12)));
		//       colors_opt(85)       => 'c' 'o' 'l' 'o' 'r' 's'.
		q(nt(85), (t(26)+t(19)+t(20)+t(19)+t(12)+t(18)));
		//       print_ambiguity_opt(86) => 'a'.
		q(nt(86), (t(21)));
		//       print_ambiguity_opt(86) => 'a' 'm' 'b' 'i' 'g' 'u' 'i' 't' 'y'.
		q(nt(86), (t(21)+t(22)+t(9)+t(23)+t(16)+t(25)+t(23)+t(13)+t(34)));
		//       print_ambiguity_opt(86) => 'p' 'r' 'i' 'n' 't' '-' 'a' 'm' 'b' 'i' 'g' 'u' 'i' 't' 'y'.
		q(nt(86), (t(28)+t(12)+t(23)+t(11)+t(13)+t(24)+t(21)+t(22)+t(9)+t(23)+t(16)+t(25)+t(23)+t(13)+t(34)));
		//       print_terminals_opt(90) => 'p' 'r' 'i' 'n' 't' '-' 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
		q(nt(90), (t(28)+t(12)+t(23)+t(11)+t(13)+t(24)+t(13)+t(17)+t(12)+t(22)+t(23)+t(11)+t(21)+t(20)+t(18)));
		//       print_terminals_opt(90) => 't'.
		q(nt(90), (t(13)));
		//       print_terminals_opt(90) => 't' 'e' 'r' 'm' 'i' 'n' 'a' 'l' 's'.
		q(nt(90), (t(13)+t(17)+t(12)+t(22)+t(23)+t(11)+t(21)+t(20)+t(18)));
		//       print_graphs_opt(87) => 'g'.
		q(nt(87), (t(16)));
		//       print_graphs_opt(87) => 'g' 'r' 'a' 'p' 'h' 's'.
		q(nt(87), (t(16)+t(12)+t(21)+t(28)+t(27)+t(18)));
		//       print_graphs_opt(87) => 'p' 'r' 'i' 'n' 't' '-' 'g' 'r' 'a' 'p' 'h' 's'.
		q(nt(87), (t(28)+t(12)+t(23)+t(11)+t(13)+t(24)+t(16)+t(12)+t(21)+t(28)+t(27)+t(18)));
		//       print_rules_opt(88)  => 'p' 'r' 'i' 'n' 't' '-' 'r' 'u' 'l' 'e' 's'.
		q(nt(88), (t(28)+t(12)+t(23)+t(11)+t(13)+t(24)+t(12)+t(25)+t(20)+t(17)+t(18)));
		//       print_rules_opt(88)  => 'r'.
		q(nt(88), (t(12)));
		//       print_rules_opt(88)  => 'r' 'u' 'l' 'e' 's'.
		q(nt(88), (t(12)+t(25)+t(20)+t(17)+t(18)));
		//       print_facts_opt(89)  => 'f'.
		q(nt(89), (t(10)));
		//       print_facts_opt(89)  => 'f' 'a' 'c' 't' 's'.
		q(nt(89), (t(10)+t(21)+t(26)+t(13)+t(18)));
		//       print_facts_opt(89)  => 'p' 'r' 'i' 'n' 't' '-' 'f' 'a' 'c' 't' 's'.
		q(nt(89), (t(28)+t(12)+t(23)+t(11)+t(13)+t(24)+t(10)+t(21)+t(26)+t(13)+t(18)));
		//       measure_parsing_opt(91) => 'm'.
		q(nt(91), (t(22)));
		//       measure_parsing_opt(91) => 'm' 'e' 'a' 's' 'u' 'r' 'e'.
		q(nt(91), (t(22)+t(17)+t(21)+t(18)+t(25)+t(12)+t(17)));
		//       measure_parsing_opt(91) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'p' 'a' 'r' 's' 'i' 'n' 'g'.
		q(nt(91), (t(22)+t(17)+t(21)+t(18)+t(25)+t(12)+t(17)+t(24)+t(28)+t(21)+t(12)+t(18)+t(23)+t(11)+t(16)));
		//       measure_each_pos_opt(92) => 'm' 'e'.
		q(nt(92), (t(22)+t(17)));
		//       measure_each_pos_opt(92) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'e' 'a' 'c' 'h'.
		q(nt(92), (t(22)+t(17)+t(21)+t(18)+t(25)+t(12)+t(17)+t(24)+t(17)+t(21)+t(26)+t(27)));
		//       measure_each_pos_opt(92) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'e' 'a' 'c' 'h' '-' 'p' 'o' 's'.
		q(nt(92), (t(22)+t(17)+t(21)+t(18)+t(25)+t(12)+t(17)+t(24)+t(17)+t(21)+t(26)+t(27)+t(24)+t(28)+t(19)+t(18)));
		//       measure_each_pos_opt(92) => 'm' 'e' 'p'.
		q(nt(92), (t(22)+t(17)+t(28)));
		//       measure_forest_opt(93) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'f' 'o' 'r' 'e' 's' 't'.
		q(nt(93), (t(22)+t(17)+t(21)+t(18)+t(25)+t(12)+t(17)+t(24)+t(10)+t(19)+t(12)+t(17)+t(18)+t(13)));
		//       measure_forest_opt(93) => 'm' 'f'.
		q(nt(93), (t(22)+t(10)));
		//       measure_preprocess_opt(94) => 'm' 'e' 'a' 's' 'u' 'r' 'e' '-' 'p' 'r' 'e' 'p' 'r' 'o' 'c' 'e' 's' 's'.
		q(nt(94), (t(22)+t(17)+t(21)+t(18)+t(25)+t(12)+t(17)+t(24)+t(28)+t(12)+t(17)+t(28)+t(12)+t(19)+t(26)+t(17)+t(18)+t(18)));
		//       measure_preprocess_opt(94) => 'm' 'p'.
		q(nt(94), (t(22)+t(28)));
		//       auto_disambiguate_opt(96) => 'a' 'd'.
		q(nt(96), (t(21)+t(30)));
		//       auto_disambiguate_opt(96) => 'a' 'u' 't' 'o' '-' 'd' 'i' 's' 'a' 'm' 'b' 'i' 'g' 'u' 'a' 't' 'e'.
		q(nt(96), (t(21)+t(25)+t(13)+t(19)+t(24)+t(30)+t(23)+t(18)+t(21)+t(22)+t(9)+t(23)+t(16)+t(25)+t(21)+t(13)+t(17)));
		//       error_verbosity_opt(83) => 'e'.
		q(nt(83), (t(17)));
		//       error_verbosity_opt(83) => 'e' 'r' 'r' 'o' 'r' '-' 'v' 'e' 'r' 'b' 'o' 's' 'i' 't' 'y'.
		q(nt(83), (t(17)+t(12)+t(12)+t(19)+t(12)+t(24)+t(31)+t(17)+t(12)+t(9)+t(19)+t(18)+t(23)+t(13)+t(34)));
		//       nodisambig_list_opt(97) => 'n' 'd'.
		q(nt(97), (t(11)+t(30)));
		//       nodisambig_list_opt(97) => 'n' 'd' 'l'.
		q(nt(97), (t(11)+t(30)+t(20)));
		//       nodisambig_list_opt(97) => 'n' 'o' 'd' 'i' 's' 'a' 'm' 'b' 'i' 'g' '-' 'l' 'i' 's' 't'.
		q(nt(97), (t(11)+t(19)+t(30)+t(23)+t(18)+t(21)+t(22)+t(9)+t(23)+t(16)+t(24)+t(20)+t(23)+t(18)+t(13)));
		//       option_value(78)     => option_value_true(98).
		q(nt(78), (nt(98)));
		//       option_value(78)     => option_value_false(99).
		q(nt(78), (nt(99)));
		//       option_value(78)     => error_verbosity(100).
		q(nt(78), (nt(100)));
		//       option_value(78)     => symbol_list(71).
		q(nt(78), (nt(71)));
		//       option_value_true(98) => '1'.
		q(nt(98), (t(35)));
		//       option_value_true(98) => 'o' 'n'.
		q(nt(98), (t(19)+t(11)));
		//       option_value_true(98) => 't'.
		q(nt(98), (t(13)));
		//       option_value_true(98) => 't' 'r' 'u' 'e'.
		q(nt(98), (t(13)+t(12)+t(25)+t(17)));
		//       option_value_true(98) => 'y'.
		q(nt(98), (t(34)));
		//       option_value_true(98) => 'y' 'e' 's'.
		q(nt(98), (t(34)+t(17)+t(18)));
		//       option_value_false(99) => '0'.
		q(nt(99), (t(36)));
		//       option_value_false(99) => 'f'.
		q(nt(99), (t(10)));
		//       option_value_false(99) => 'f' 'a' 'l' 's' 'e'.
		q(nt(99), (t(10)+t(21)+t(20)+t(18)+t(17)));
		//       option_value_false(99) => 'n'.
		q(nt(99), (t(11)));
		//       option_value_false(99) => 'n' 'o'.
		q(nt(99), (t(11)+t(19)));
		//       option_value_false(99) => 'o' 'f' 'f'.
		q(nt(99), (t(19)+t(10)+t(10)));
		//       error_verbosity(100) => basic_sym(101).
		q(nt(100), (nt(101)));
		//       error_verbosity(100) => detailed_sym(102).
		q(nt(100), (nt(102)));
		//       error_verbosity(100) => root_cause_sym(103).
		q(nt(100), (nt(103)));
		//       basic_sym(101)       => 'b'.
		q(nt(101), (t(9)));
		//       basic_sym(101)       => 'b' 'a' 's' 'i' 'c'.
		q(nt(101), (t(9)+t(21)+t(18)+t(23)+t(26)));
		//       detailed_sym(102)    => 'd'.
		q(nt(102), (t(30)));
		//       detailed_sym(102)    => 'd' 'e' 't' 'a' 'i' 'l' 'e' 'd'.
		q(nt(102), (t(30)+t(17)+t(13)+t(21)+t(23)+t(20)+t(17)+t(30)));
		//       root_cause_sym(103)  => 'r'.
		q(nt(103), (t(12)));
		//       root_cause_sym(103)  => 'r' 'c'.
		q(nt(103), (t(12)+t(26)));
		//       root_cause_sym(103)  => 'r' 'o' 'o' 't' '-' 'c' 'a' 'u' 's' 'e'.
		q(nt(103), (t(12)+t(19)+t(19)+t(13)+t(24)+t(26)+t(21)+t(25)+t(18)+t(17)));
		//       _Rsymbol_list_23(104) => _(7) ',' _(7) symbol(28).
		q(nt(104), (nt(7)+t(37)+nt(7)+nt(28)));
		//       _Rsymbol_list_24(105) => null.
		q(nt(105), (nul));
		//       _Rsymbol_list_24(105) => _Rsymbol_list_23(104) _Rsymbol_list_24(105).
		q(nt(105), (nt(104)+nt(105)));
		//       symbol_list(71)      => symbol(28) _Rsymbol_list_24(105).
		q(nt(71), (nt(28)+nt(105)));
		return q;
	}
};

#endif // __TGF_REPL_PARSER_H__
