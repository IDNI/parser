// This file is generated from a file tools/tgf/tgf_repl.tgf by
//       https://github.com/IDNI/parser/tools/tgf
//
#ifndef __TGF_REPL_PARSER_H__
#define __TGF_REPL_PARSER_H__

#include <string.h>

#include "parser.h"

struct tgf_repl_parser {
	using char_type     = char;
	using terminal_type = char;
	using traits_type   = std::char_traits<char_type>;
	using int_type      = typename traits_type::int_type;
	using symbol_type   = idni::lit<char_type, terminal_type>;
	using location_type = std::array<size_t, 2>;
	using node_type     = std::pair<symbol_type, location_type>;
	using parser_type   = idni::parser<char_type, terminal_type>;
	using options       = parser_type::options;
	using parse_options = parser_type::parse_options;
	using forest_type   = parser_type::pforest;
	using input_type    = parser_type::input;
	using decoder_type  = parser_type::input::decoder_type;
	using encoder_type  = std::function<std::basic_string<char_type>(
			const std::vector<terminal_type>&)>;
	tgf_repl_parser() :
		nts(load_nonterminals()), cc(load_cc()),
		g(nts, load_prods(), nt(6), cc), p(g, load_opts()) {}
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
	bool found(int start = -1) { return p.found(start); }
	typename parser_type::error get_error() { return p.get_error(); }
	enum nonterminal {
		nul, eof, alnum, alpha, space, printable, start, _, statement, _Rstart_0, 
		_Rstart_1, _Rstart_2, _Rstart_3, __, _R__4, comment, _R___5, _Rcomment_6, _Rcomment_7, _Rcomment_8, 
		quoted_string, unescaped_s, escaped_s, _Rquoted_string_9, _Rquoted_string_10, _Runescaped_s_11, _Rescaped_s_12, filename, symbol, _Rsymbol_13, 
		_Rsymbol_14, parse_input_char, parse_input, _Rparse_input_15, grammar_cmd, internal_grammar_cmd, start_cmd, unreachable_cmd, reload_cmd, file_cmd, 
		help, version, quit, get, set, toggle, enable, disable, parse_cmd, grammar_sym, 
		internal_grammar_sym, _Rinternal_grammar_cmd_16, start_sym, _Rstart_cmd_17, unreachable_sym, _Runreachable_cmd_18, reload_sym, file_sym, help_sym, cmd_symbol, 
		_Rhelp_19, version_sym, quit_sym, get_sym, option, _Rget_20, set_sym, _Rset_21, option_value, toggle_sym, 
		bool_option, enable_sym, disable_sym, parse_sym, error_verbosity_opt, status_opt, colors_opt, print_ambiguity_opt, print_graphs_opt, print_rules_opt, 
		print_facts_opt, debug_opt, option_value_true, option_value_false, error_verbosity, basic_sym, detailed_sym, root_cause_sym, __neg_0, 
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
		'd', 'v', 'x', 'q', '+', 'y', '1', '0', 
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
			"", "eof", "alnum", "alpha", "space", "printable", "start", "_", "statement", "_Rstart_0", 
			"_Rstart_1", "_Rstart_2", "_Rstart_3", "__", "_R__4", "comment", "_R___5", "_Rcomment_6", "_Rcomment_7", "_Rcomment_8", 
			"quoted_string", "unescaped_s", "escaped_s", "_Rquoted_string_9", "_Rquoted_string_10", "_Runescaped_s_11", "_Rescaped_s_12", "filename", "symbol", "_Rsymbol_13", 
			"_Rsymbol_14", "parse_input_char", "parse_input", "_Rparse_input_15", "grammar_cmd", "internal_grammar_cmd", "start_cmd", "unreachable_cmd", "reload_cmd", "file_cmd", 
			"help", "version", "quit", "get", "set", "toggle", "enable", "disable", "parse_cmd", "grammar_sym", 
			"internal_grammar_sym", "_Rinternal_grammar_cmd_16", "start_sym", "_Rstart_cmd_17", "unreachable_sym", "_Runreachable_cmd_18", "reload_sym", "file_sym", "help_sym", "cmd_symbol", 
			"_Rhelp_19", "version_sym", "quit_sym", "get_sym", "option", "_Rget_20", "set_sym", "_Rset_21", "option_value", "toggle_sym", 
			"bool_option", "enable_sym", "disable_sym", "parse_sym", "error_verbosity_opt", "status_opt", "colors_opt", "print_ambiguity_opt", "print_graphs_opt", "print_rules_opt", 
			"print_facts_opt", "debug_opt", "option_value_true", "option_value_false", "error_verbosity", "basic_sym", "detailed_sym", "root_cause_sym", "__neg_0", 
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
	options load_opts() {
		options o;
		o.auto_disambiguate = true;
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
		//       _R___5(16)           => space(4).
		q(nt(16), (nt(4)));
		//       _R___5(16)           => comment(15).
		q(nt(16), (nt(15)));
		//       __(13)               => _R___5(16) _(7).
		q(nt(13), (nt(16)+nt(7)));
		//       _Rcomment_6(17)      => '\t'.
		q(nt(17), (t(2)));
		//       _Rcomment_6(17)      => printable(5).
		q(nt(17), (nt(5)));
		//       _Rcomment_7(18)      => null.
		q(nt(18), (nul));
		//       _Rcomment_7(18)      => _Rcomment_6(17) _Rcomment_7(18).
		q(nt(18), (nt(17)+nt(18)));
		//       _Rcomment_8(19)      => '\n'.
		q(nt(19), (t(3)));
		//       _Rcomment_8(19)      => '\r'.
		q(nt(19), (t(4)));
		//       _Rcomment_8(19)      => eof(1).
		q(nt(19), (nt(1)));
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
		//       __neg_0(88)          => _Runescaped_s_11(25).
		q(nt(88), (nt(25)));
		//       unescaped_s(21)      => printable(5) & ~( __neg_0(88) ).	 # conjunctive
		q(nt(21), (nt(5)) & ~(nt(88)));
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
		//       _Rsymbol_13(29)      => alnum(2).
		q(nt(29), (nt(2)));
		//       _Rsymbol_14(30)      => _Rsymbol_13(29).
		q(nt(30), (nt(29)));
		//       _Rsymbol_14(30)      => _Rsymbol_13(29) _Rsymbol_14(30).
		q(nt(30), (nt(29)+nt(30)));
		//       symbol(28)           => _Rsymbol_14(30).
		q(nt(28), (nt(30)));
		//       parse_input_char(31) => '\t'.
		q(nt(31), (t(2)));
		//       parse_input_char(31) => printable(5).
		q(nt(31), (nt(5)));
		//       _Rparse_input_15(33) => null.
		q(nt(33), (nul));
		//       _Rparse_input_15(33) => parse_input_char(31) _Rparse_input_15(33).
		q(nt(33), (nt(31)+nt(33)));
		//       parse_input(32)      => _Rparse_input_15(33).
		q(nt(32), (nt(33)));
		//       statement(8)         => grammar_cmd(34).
		q(nt(8), (nt(34)));
		//       statement(8)         => internal_grammar_cmd(35).
		q(nt(8), (nt(35)));
		//       statement(8)         => start_cmd(36).
		q(nt(8), (nt(36)));
		//       statement(8)         => unreachable_cmd(37).
		q(nt(8), (nt(37)));
		//       statement(8)         => reload_cmd(38).
		q(nt(8), (nt(38)));
		//       statement(8)         => file_cmd(39).
		q(nt(8), (nt(39)));
		//       statement(8)         => help(40).
		q(nt(8), (nt(40)));
		//       statement(8)         => version(41).
		q(nt(8), (nt(41)));
		//       statement(8)         => quit(42).
		q(nt(8), (nt(42)));
		//       statement(8)         => get(43).
		q(nt(8), (nt(43)));
		//       statement(8)         => set(44).
		q(nt(8), (nt(44)));
		//       statement(8)         => toggle(45).
		q(nt(8), (nt(45)));
		//       statement(8)         => enable(46).
		q(nt(8), (nt(46)));
		//       statement(8)         => disable(47).
		q(nt(8), (nt(47)));
		//       statement(8)         => parse_cmd(48).
		q(nt(8), (nt(48)));
		//       parse_cmd(48)        => parse_input(32).
		q(nt(48), (nt(32)));
		//       grammar_cmd(34)      => grammar_sym(49).
		q(nt(34), (nt(49)));
		//       _Rinternal_grammar_cmd_16(51) => null.
		q(nt(51), (nul));
		//       _Rinternal_grammar_cmd_16(51) => __(13) symbol(28).
		q(nt(51), (nt(13)+nt(28)));
		//       internal_grammar_cmd(35) => internal_grammar_sym(50) _Rinternal_grammar_cmd_16(51).
		q(nt(35), (nt(50)+nt(51)));
		//       _Rstart_cmd_17(53)   => null.
		q(nt(53), (nul));
		//       _Rstart_cmd_17(53)   => __(13) symbol(28).
		q(nt(53), (nt(13)+nt(28)));
		//       start_cmd(36)        => start_sym(52) _Rstart_cmd_17(53).
		q(nt(36), (nt(52)+nt(53)));
		//       _Runreachable_cmd_18(55) => null.
		q(nt(55), (nul));
		//       _Runreachable_cmd_18(55) => __(13) symbol(28).
		q(nt(55), (nt(13)+nt(28)));
		//       unreachable_cmd(37)  => unreachable_sym(54) _Runreachable_cmd_18(55).
		q(nt(37), (nt(54)+nt(55)));
		//       reload_cmd(38)       => reload_sym(56).
		q(nt(38), (nt(56)));
		//       file_cmd(39)         => file_sym(57) __(13) filename(27).
		q(nt(39), (nt(57)+nt(13)+nt(27)));
		//       _Rhelp_19(60)        => null.
		q(nt(60), (nul));
		//       _Rhelp_19(60)        => __(13) cmd_symbol(59).
		q(nt(60), (nt(13)+nt(59)));
		//       help(40)             => help_sym(58) _Rhelp_19(60).
		q(nt(40), (nt(58)+nt(60)));
		//       version(41)          => version_sym(61).
		q(nt(41), (nt(61)));
		//       quit(42)             => quit_sym(62).
		q(nt(42), (nt(62)));
		//       _Rget_20(65)         => null.
		q(nt(65), (nul));
		//       _Rget_20(65)         => __(13) option(64).
		q(nt(65), (nt(13)+nt(64)));
		//       get(43)              => get_sym(63) _Rget_20(65).
		q(nt(43), (nt(63)+nt(65)));
		//       _Rset_21(67)         => _(7) '=' _(7).
		q(nt(67), (nt(7)+t(15)+nt(7)));
		//       _Rset_21(67)         => __(13).
		q(nt(67), (nt(13)));
		//       set(44)              => set_sym(66) __(13) option(64) _Rset_21(67) option_value(68).
		q(nt(44), (nt(66)+nt(13)+nt(64)+nt(67)+nt(68)));
		//       toggle(45)           => toggle_sym(69) __(13) bool_option(70).
		q(nt(45), (nt(69)+nt(13)+nt(70)));
		//       enable(46)           => enable_sym(71) bool_option(70).
		q(nt(46), (nt(71)+nt(70)));
		//       disable(47)          => disable_sym(72) bool_option(70).
		q(nt(47), (nt(72)+nt(70)));
		//       get_sym(63)          => 'g' 'e' 't'.
		q(nt(63), (t(16)+t(17)+t(13)));
		//       set_sym(66)          => 's' 'e' 't'.
		q(nt(66), (t(18)+t(17)+t(13)));
		//       toggle_sym(69)       => 't' 'o' 'g' 'g' 'l' 'e'.
		q(nt(69), (t(13)+t(19)+t(16)+t(16)+t(20)+t(17)));
		//       grammar_sym(49)      => 'g'.
		q(nt(49), (t(16)));
		//       grammar_sym(49)      => 'g' 'r' 'a' 'm' 'm' 'a' 'r'.
		q(nt(49), (t(16)+t(12)+t(21)+t(22)+t(22)+t(21)+t(12)));
		//       internal_grammar_sym(50) => 'i'.
		q(nt(50), (t(23)));
		//       internal_grammar_sym(50) => 'i' 'g'.
		q(nt(50), (t(23)+t(16)));
		//       internal_grammar_sym(50) => 'i' 'n' 't' 'e' 'r' 'n' 'a' 'l' '-' 'g' 'r' 'a' 'm' 'm' 'a' 'r'.
		q(nt(50), (t(23)+t(11)+t(13)+t(17)+t(12)+t(11)+t(21)+t(20)+t(24)+t(16)+t(12)+t(21)+t(22)+t(22)+t(21)+t(12)));
		//       unreachable_sym(54)  => 'u'.
		q(nt(54), (t(25)));
		//       unreachable_sym(54)  => 'u' 'n' 'r' 'e' 'a' 'c' 'h' 'a' 'b' 'l' 'e'.
		q(nt(54), (t(25)+t(11)+t(12)+t(17)+t(21)+t(26)+t(27)+t(21)+t(9)+t(20)+t(17)));
		//       parse_sym(73)        => 'p'.
		q(nt(73), (t(28)));
		//       parse_sym(73)        => 'p' 'a' 'r' 's' 'e'.
		q(nt(73), (t(28)+t(21)+t(12)+t(18)+t(17)));
		//       start_sym(52)        => 's'.
		q(nt(52), (t(18)));
		//       start_sym(52)        => 's' 't' 'a' 'r' 't'.
		q(nt(52), (t(18)+t(13)+t(21)+t(12)+t(13)));
		//       reload_sym(56)       => 'r'.
		q(nt(56), (t(12)));
		//       reload_sym(56)       => 'r' 'e' 'l' 'o' 'a' 'd'.
		q(nt(56), (t(12)+t(17)+t(20)+t(19)+t(21)+t(29)));
		//       file_sym(57)         => 'f'.
		q(nt(57), (t(10)));
		//       file_sym(57)         => 'f' 'i' 'l' 'e'.
		q(nt(57), (t(10)+t(23)+t(20)+t(17)));
		//       help_sym(58)         => 'h'.
		q(nt(58), (t(27)));
		//       help_sym(58)         => 'h' 'e' 'l' 'p'.
		q(nt(58), (t(27)+t(17)+t(20)+t(28)));
		//       version_sym(61)      => 'v'.
		q(nt(61), (t(30)));
		//       version_sym(61)      => 'v' 'e' 'r' 's' 'i' 'o' 'n'.
		q(nt(61), (t(30)+t(17)+t(12)+t(18)+t(23)+t(19)+t(11)));
		//       quit_sym(62)         => 'e'.
		q(nt(62), (t(17)));
		//       quit_sym(62)         => 'e' 'x' 'i' 't'.
		q(nt(62), (t(17)+t(31)+t(23)+t(13)));
		//       quit_sym(62)         => 'q'.
		q(nt(62), (t(32)));
		//       quit_sym(62)         => 'q' 'u' 'i' 't'.
		q(nt(62), (t(32)+t(25)+t(23)+t(13)));
		//       get_sym(63)          => 'g' 'e' 't'.
		q(nt(63), (t(16)+t(17)+t(13)));
		//       set_sym(66)          => 's' 'e' 't'.
		q(nt(66), (t(18)+t(17)+t(13)));
		//       toggle_sym(69)       => 't' 'o' 'g'.
		q(nt(69), (t(13)+t(19)+t(16)));
		//       toggle_sym(69)       => 't' 'o' 'g' 'g' 'l' 'e'.
		q(nt(69), (t(13)+t(19)+t(16)+t(16)+t(20)+t(17)));
		//       enable_sym(71)       => '+'.
		q(nt(71), (t(33)));
		//       enable_sym(71)       => 'e' 'n' 'a' 'b' 'l' 'e' __(13).
		q(nt(71), (t(17)+t(11)+t(21)+t(9)+t(20)+t(17)+nt(13)));
		//       enable_sym(71)       => 'e' 'n' __(13).
		q(nt(71), (t(17)+t(11)+nt(13)));
		//       enable_sym(71)       => 'o' 'n' __(13).
		q(nt(71), (t(19)+t(11)+nt(13)));
		//       disable_sym(72)      => '-'.
		q(nt(72), (t(24)));
		//       disable_sym(72)      => 'd' 'i' 's' 'a' 'b' 'l' 'e' __(13).
		q(nt(72), (t(29)+t(23)+t(18)+t(21)+t(9)+t(20)+t(17)+nt(13)));
		//       disable_sym(72)      => 'd' 'i' 's' __(13).
		q(nt(72), (t(29)+t(23)+t(18)+nt(13)));
		//       disable_sym(72)      => 'o' 'f' 'f' __(13).
		q(nt(72), (t(19)+t(10)+t(10)+nt(13)));
		//       cmd_symbol(59)       => grammar_sym(49).
		q(nt(59), (nt(49)));
		//       cmd_symbol(59)       => internal_grammar_sym(50).
		q(nt(59), (nt(50)));
		//       cmd_symbol(59)       => start_sym(52).
		q(nt(59), (nt(52)));
		//       cmd_symbol(59)       => unreachable_sym(54).
		q(nt(59), (nt(54)));
		//       cmd_symbol(59)       => reload_sym(56).
		q(nt(59), (nt(56)));
		//       cmd_symbol(59)       => file_sym(57).
		q(nt(59), (nt(57)));
		//       cmd_symbol(59)       => help_sym(58).
		q(nt(59), (nt(58)));
		//       cmd_symbol(59)       => quit_sym(62).
		q(nt(59), (nt(62)));
		//       cmd_symbol(59)       => get_sym(63).
		q(nt(59), (nt(63)));
		//       cmd_symbol(59)       => set_sym(66).
		q(nt(59), (nt(66)));
		//       cmd_symbol(59)       => toggle_sym(69).
		q(nt(59), (nt(69)));
		//       cmd_symbol(59)       => enable_sym(71).
		q(nt(59), (nt(71)));
		//       cmd_symbol(59)       => disable_sym(72).
		q(nt(59), (nt(72)));
		//       cmd_symbol(59)       => parse_sym(73).
		q(nt(59), (nt(73)));
		//       option(64)           => bool_option(70).
		q(nt(64), (nt(70)));
		//       option(64)           => error_verbosity_opt(74).
		q(nt(64), (nt(74)));
		//       bool_option(70)      => status_opt(75).
		q(nt(70), (nt(75)));
		//       bool_option(70)      => colors_opt(76).
		q(nt(70), (nt(76)));
		//       bool_option(70)      => print_ambiguity_opt(77).
		q(nt(70), (nt(77)));
		//       bool_option(70)      => print_graphs_opt(78).
		q(nt(70), (nt(78)));
		//       bool_option(70)      => print_rules_opt(79).
		q(nt(70), (nt(79)));
		//       bool_option(70)      => print_facts_opt(80).
		q(nt(70), (nt(80)));
		//       bool_option(70)      => debug_opt(81).
		q(nt(70), (nt(81)));
		//       debug_opt(81)        => 'd'.
		q(nt(81), (t(29)));
		//       debug_opt(81)        => 'd' 'e' 'b' 'u' 'g'.
		q(nt(81), (t(29)+t(17)+t(9)+t(25)+t(16)));
		//       status_opt(75)       => 's'.
		q(nt(75), (t(18)));
		//       status_opt(75)       => 's' 't' 'a' 't' 'u' 's'.
		q(nt(75), (t(18)+t(13)+t(21)+t(13)+t(25)+t(18)));
		//       colors_opt(76)       => 'c'.
		q(nt(76), (t(26)));
		//       colors_opt(76)       => 'c' 'o' 'l' 'o' 'r'.
		q(nt(76), (t(26)+t(19)+t(20)+t(19)+t(12)));
		//       colors_opt(76)       => 'c' 'o' 'l' 'o' 'r' 's'.
		q(nt(76), (t(26)+t(19)+t(20)+t(19)+t(12)+t(18)));
		//       print_ambiguity_opt(77) => 'a'.
		q(nt(77), (t(21)));
		//       print_ambiguity_opt(77) => 'a' 'm' 'b' 'i' 'g' 'u' 'i' 't' 'y'.
		q(nt(77), (t(21)+t(22)+t(9)+t(23)+t(16)+t(25)+t(23)+t(13)+t(34)));
		//       print_ambiguity_opt(77) => 'p' 'r' 'i' 'n' 't' '-' 'a' 'm' 'b' 'i' 'g' 'u' 'i' 't' 'y'.
		q(nt(77), (t(28)+t(12)+t(23)+t(11)+t(13)+t(24)+t(21)+t(22)+t(9)+t(23)+t(16)+t(25)+t(23)+t(13)+t(34)));
		//       print_graphs_opt(78) => 'g'.
		q(nt(78), (t(16)));
		//       print_graphs_opt(78) => 'g' 'r' 'a' 'p' 'h' 's'.
		q(nt(78), (t(16)+t(12)+t(21)+t(28)+t(27)+t(18)));
		//       print_graphs_opt(78) => 'p' 'r' 'i' 'n' 't' '-' 'g' 'r' 'a' 'p' 'h' 's'.
		q(nt(78), (t(28)+t(12)+t(23)+t(11)+t(13)+t(24)+t(16)+t(12)+t(21)+t(28)+t(27)+t(18)));
		//       print_rules_opt(79)  => 'p' 'r' 'i' 'n' 't' '-' 'r' 'u' 'l' 'e' 's'.
		q(nt(79), (t(28)+t(12)+t(23)+t(11)+t(13)+t(24)+t(12)+t(25)+t(20)+t(17)+t(18)));
		//       print_rules_opt(79)  => 'r'.
		q(nt(79), (t(12)));
		//       print_rules_opt(79)  => 'r' 'u' 'l' 'e' 's'.
		q(nt(79), (t(12)+t(25)+t(20)+t(17)+t(18)));
		//       print_facts_opt(80)  => 'f'.
		q(nt(80), (t(10)));
		//       print_facts_opt(80)  => 'f' 'a' 'c' 't' 's'.
		q(nt(80), (t(10)+t(21)+t(26)+t(13)+t(18)));
		//       print_facts_opt(80)  => 'p' 'r' 'i' 'n' 't' '-' 'f' 'a' 'c' 't' 's'.
		q(nt(80), (t(28)+t(12)+t(23)+t(11)+t(13)+t(24)+t(10)+t(21)+t(26)+t(13)+t(18)));
		//       error_verbosity_opt(74) => 'e'.
		q(nt(74), (t(17)));
		//       error_verbosity_opt(74) => 'e' 'r' 'r' 'o' 'r' '-' 'v' 'e' 'r' 'b' 'o' 's' 'i' 't' 'y'.
		q(nt(74), (t(17)+t(12)+t(12)+t(19)+t(12)+t(24)+t(30)+t(17)+t(12)+t(9)+t(19)+t(18)+t(23)+t(13)+t(34)));
		//       option_value(68)     => option_value_true(82).
		q(nt(68), (nt(82)));
		//       option_value(68)     => option_value_false(83).
		q(nt(68), (nt(83)));
		//       option_value(68)     => error_verbosity(84).
		q(nt(68), (nt(84)));
		//       option_value_true(82) => '1'.
		q(nt(82), (t(35)));
		//       option_value_true(82) => 'o' 'n'.
		q(nt(82), (t(19)+t(11)));
		//       option_value_true(82) => 't'.
		q(nt(82), (t(13)));
		//       option_value_true(82) => 't' 'r' 'u' 'e'.
		q(nt(82), (t(13)+t(12)+t(25)+t(17)));
		//       option_value_true(82) => 'y'.
		q(nt(82), (t(34)));
		//       option_value_true(82) => 'y' 'e' 's'.
		q(nt(82), (t(34)+t(17)+t(18)));
		//       option_value_false(83) => '0'.
		q(nt(83), (t(36)));
		//       option_value_false(83) => 'f'.
		q(nt(83), (t(10)));
		//       option_value_false(83) => 'f' 'a' 'l' 's' 'e'.
		q(nt(83), (t(10)+t(21)+t(20)+t(18)+t(17)));
		//       option_value_false(83) => 'n'.
		q(nt(83), (t(11)));
		//       option_value_false(83) => 'n' 'o'.
		q(nt(83), (t(11)+t(19)));
		//       option_value_false(83) => 'o' 'f' 'f'.
		q(nt(83), (t(19)+t(10)+t(10)));
		//       error_verbosity(84)  => basic_sym(85).
		q(nt(84), (nt(85)));
		//       error_verbosity(84)  => detailed_sym(86).
		q(nt(84), (nt(86)));
		//       error_verbosity(84)  => root_cause_sym(87).
		q(nt(84), (nt(87)));
		//       basic_sym(85)        => 'b'.
		q(nt(85), (t(9)));
		//       basic_sym(85)        => 'b' 'a' 's' 'i' 'c'.
		q(nt(85), (t(9)+t(21)+t(18)+t(23)+t(26)));
		//       detailed_sym(86)     => 'd'.
		q(nt(86), (t(29)));
		//       detailed_sym(86)     => 'd' 'e' 't' 'a' 'i' 'l' 'e' 'd'.
		q(nt(86), (t(29)+t(17)+t(13)+t(21)+t(23)+t(20)+t(17)+t(29)));
		//       root_cause_sym(87)   => 'r'.
		q(nt(87), (t(12)));
		//       root_cause_sym(87)   => 'r' 'c'.
		q(nt(87), (t(12)+t(26)));
		//       root_cause_sym(87)   => 'r' 'o' 'o' 't' '-' 'c' 'a' 'u' 's' 'e'.
		q(nt(87), (t(12)+t(19)+t(19)+t(13)+t(24)+t(26)+t(21)+t(25)+t(18)+t(17)));
		return q;
	}
};

#endif // __TGF_REPL_PARSER_H__
