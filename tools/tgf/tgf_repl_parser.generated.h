// This file is generated from a file tools/tgf/tgf_repl.tgf by
//       https://github.com/IDNI/parser/tools/tgf
//
#ifndef __TGF_REPL_PARSER_H__
#define __TGF_REPL_PARSER_H__

#include <string.h>
#include <ranges>

#include "parser.h"
#include "rewriting.h"

namespace tgf_repl_parsing {

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
	using rw_symbol_type  = std::variant<symbol_type, size_t>;
	using rw_node_type    = idni::rewriter::node<rw_symbol_type>;
	using sp_rw_node_type = idni::rewriter::sp_node<rw_symbol_type>;
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
		_Rsymbol_14, parse_input_char, parse_input, _Rparse_input_15, grammar_cmd, internal_grammar_cmd, start_cmd, reload_cmd, file_cmd, help, 
		version, quit, get, set, toggle, enable, disable, parse_cmd, grammar_sym, internal_grammar_sym, 
		_Rinternal_grammar_cmd_16, start_sym, _Rstart_cmd_17, reload_sym, file_sym, help_sym, cmd_symbol, _Rhelp_18, version_sym, quit_sym, 
		get_sym, option, _Rget_19, set_sym, _Rset_20, option_value, toggle_sym, bool_option, enable_sym, disable_sym, 
		parse_sym, error_verbosity_opt, status_opt, colors_opt, print_ambiguity_opt, print_graphs_opt, print_rules_opt, print_facts_opt, debug_opt, option_value_true, 
		option_value_false, error_verbosity, basic_sym, detailed_sym, root_cause_sym, __neg_0, 
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
		'o', 'l', 'a', 'm', 'i', '-', 'p', 'd', 'h', 'v', 
		'x', 'q', 'u', '+', 'c', 'y', '1', '0', 
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
			"_Rsymbol_14", "parse_input_char", "parse_input", "_Rparse_input_15", "grammar_cmd", "internal_grammar_cmd", "start_cmd", "reload_cmd", "file_cmd", "help", 
			"version", "quit", "get", "set", "toggle", "enable", "disable", "parse_cmd", "grammar_sym", "internal_grammar_sym", 
			"_Rinternal_grammar_cmd_16", "start_sym", "_Rstart_cmd_17", "reload_sym", "file_sym", "help_sym", "cmd_symbol", "_Rhelp_18", "version_sym", "quit_sym", 
			"get_sym", "option", "_Rget_19", "set_sym", "_Rset_20", "option_value", "toggle_sym", "bool_option", "enable_sym", "disable_sym", 
			"parse_sym", "error_verbosity_opt", "status_opt", "colors_opt", "print_ambiguity_opt", "print_graphs_opt", "print_rules_opt", "print_facts_opt", "debug_opt", "option_value_true", 
			"option_value_false", "error_verbosity", "basic_sym", "detailed_sym", "root_cause_sym", "__neg_0", 
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
		return o;
	}
	idni::prods<char_type, terminal_type> load_prods() {
		idni::prods<char_type, terminal_type> q,
			nul(symbol_type{});
		// _Rstart_0 => _ '.' _ statement.
		q(nt(9), (nt(7)+t(1)+nt(7)+nt(8)));
		// _Rstart_1 => null.
		q(nt(10), (nul));
		// _Rstart_1 => _Rstart_0 _Rstart_1.
		q(nt(10), (nt(9)+nt(10)));
		// _Rstart_2 => null.
		q(nt(11), (nul));
		// _Rstart_2 => _ '.'.
		q(nt(11), (nt(7)+t(1)));
		// _Rstart_3 => null.
		q(nt(12), (nul));
		// _Rstart_3 => _Rstart_2 _.
		q(nt(12), (nt(11)+nt(7)));
		// start => _ statement _Rstart_1 _Rstart_3.
		q(nt(6), (nt(7)+nt(8)+nt(10)+nt(12)));
		// _R__4 => null.
		q(nt(14), (nul));
		// _R__4 => __.
		q(nt(14), (nt(13)));
		// _ => _R__4.
		q(nt(7), (nt(14)));
		// _R___5 => space.
		q(nt(16), (nt(4)));
		// _R___5 => comment.
		q(nt(16), (nt(15)));
		// __ => _R___5 _.
		q(nt(13), (nt(16)+nt(7)));
		// _Rcomment_6 => '\t'.
		q(nt(17), (t(2)));
		// _Rcomment_6 => printable.
		q(nt(17), (nt(5)));
		// _Rcomment_7 => null.
		q(nt(18), (nul));
		// _Rcomment_7 => _Rcomment_6 _Rcomment_7.
		q(nt(18), (nt(17)+nt(18)));
		// _Rcomment_8 => '\n'.
		q(nt(19), (t(3)));
		// _Rcomment_8 => '\r'.
		q(nt(19), (t(4)));
		// _Rcomment_8 => eof.
		q(nt(19), (nt(1)));
		// comment => '#' _Rcomment_7 _Rcomment_8.
		q(nt(15), (t(5)+nt(18)+nt(19)));
		// _Rquoted_string_9 => unescaped_s.
		q(nt(23), (nt(21)));
		// _Rquoted_string_9 => escaped_s.
		q(nt(23), (nt(22)));
		// _Rquoted_string_10 => null.
		q(nt(24), (nul));
		// _Rquoted_string_10 => _Rquoted_string_9 _Rquoted_string_10.
		q(nt(24), (nt(23)+nt(24)));
		// quoted_string => '"' _Rquoted_string_10 '"'.
		q(nt(20), (t(6)+nt(24)+t(6)));
		// _Runescaped_s_11 => '"'.
		q(nt(25), (t(6)));
		// _Runescaped_s_11 => '\\'.
		q(nt(25), (t(7)));
		// __neg_0 => _Runescaped_s_11.
		q(nt(85), (nt(25)));
		// unescaped_s => printable & ~( __neg_0 ).
		q(nt(21), (nt(5)) & ~(nt(85)));
		// _Rescaped_s_12 => '"'.
		q(nt(26), (t(6)));
		// _Rescaped_s_12 => '/'.
		q(nt(26), (t(8)));
		// _Rescaped_s_12 => '\\'.
		q(nt(26), (t(7)));
		// _Rescaped_s_12 => 'b'.
		q(nt(26), (t(9)));
		// _Rescaped_s_12 => 'f'.
		q(nt(26), (t(10)));
		// _Rescaped_s_12 => 'n'.
		q(nt(26), (t(11)));
		// _Rescaped_s_12 => 'r'.
		q(nt(26), (t(12)));
		// _Rescaped_s_12 => 't'.
		q(nt(26), (t(13)));
		// escaped_s => '\\' _Rescaped_s_12.
		q(nt(22), (t(7)+nt(26)));
		// filename => quoted_string.
		q(nt(27), (nt(20)));
		// _Rsymbol_13 => '_'.
		q(nt(29), (t(14)));
		// _Rsymbol_13 => alnum.
		q(nt(29), (nt(2)));
		// _Rsymbol_14 => _Rsymbol_13.
		q(nt(30), (nt(29)));
		// _Rsymbol_14 => _Rsymbol_13 _Rsymbol_14.
		q(nt(30), (nt(29)+nt(30)));
		// symbol => _Rsymbol_14.
		q(nt(28), (nt(30)));
		// parse_input_char => '\t'.
		q(nt(31), (t(2)));
		// parse_input_char => printable.
		q(nt(31), (nt(5)));
		// _Rparse_input_15 => null.
		q(nt(33), (nul));
		// _Rparse_input_15 => parse_input_char _Rparse_input_15.
		q(nt(33), (nt(31)+nt(33)));
		// parse_input => _Rparse_input_15.
		q(nt(32), (nt(33)));
		// statement => grammar_cmd.
		q(nt(8), (nt(34)));
		// statement => internal_grammar_cmd.
		q(nt(8), (nt(35)));
		// statement => start_cmd.
		q(nt(8), (nt(36)));
		// statement => reload_cmd.
		q(nt(8), (nt(37)));
		// statement => file_cmd.
		q(nt(8), (nt(38)));
		// statement => help.
		q(nt(8), (nt(39)));
		// statement => version.
		q(nt(8), (nt(40)));
		// statement => quit.
		q(nt(8), (nt(41)));
		// statement => get.
		q(nt(8), (nt(42)));
		// statement => set.
		q(nt(8), (nt(43)));
		// statement => toggle.
		q(nt(8), (nt(44)));
		// statement => enable.
		q(nt(8), (nt(45)));
		// statement => disable.
		q(nt(8), (nt(46)));
		// statement => parse_cmd.
		q(nt(8), (nt(47)));
		// parse_cmd => parse_input.
		q(nt(47), (nt(32)));
		// grammar_cmd => grammar_sym.
		q(nt(34), (nt(48)));
		// _Rinternal_grammar_cmd_16 => null.
		q(nt(50), (nul));
		// _Rinternal_grammar_cmd_16 => __ symbol.
		q(nt(50), (nt(13)+nt(28)));
		// internal_grammar_cmd => internal_grammar_sym _Rinternal_grammar_cmd_16.
		q(nt(35), (nt(49)+nt(50)));
		// _Rstart_cmd_17 => null.
		q(nt(52), (nul));
		// _Rstart_cmd_17 => __ symbol.
		q(nt(52), (nt(13)+nt(28)));
		// start_cmd => start_sym _Rstart_cmd_17.
		q(nt(36), (nt(51)+nt(52)));
		// reload_cmd => reload_sym.
		q(nt(37), (nt(53)));
		// file_cmd => file_sym __ filename.
		q(nt(38), (nt(54)+nt(13)+nt(27)));
		// _Rhelp_18 => null.
		q(nt(57), (nul));
		// _Rhelp_18 => __ cmd_symbol.
		q(nt(57), (nt(13)+nt(56)));
		// help => help_sym _Rhelp_18.
		q(nt(39), (nt(55)+nt(57)));
		// version => version_sym.
		q(nt(40), (nt(58)));
		// quit => quit_sym.
		q(nt(41), (nt(59)));
		// _Rget_19 => null.
		q(nt(62), (nul));
		// _Rget_19 => __ option.
		q(nt(62), (nt(13)+nt(61)));
		// get => get_sym _Rget_19.
		q(nt(42), (nt(60)+nt(62)));
		// _Rset_20 => _ '=' _.
		q(nt(64), (nt(7)+t(15)+nt(7)));
		// _Rset_20 => __.
		q(nt(64), (nt(13)));
		// set => set_sym __ option _Rset_20 option_value.
		q(nt(43), (nt(63)+nt(13)+nt(61)+nt(64)+nt(65)));
		// toggle => toggle_sym __ bool_option.
		q(nt(44), (nt(66)+nt(13)+nt(67)));
		// enable => enable_sym bool_option.
		q(nt(45), (nt(68)+nt(67)));
		// disable => disable_sym bool_option.
		q(nt(46), (nt(69)+nt(67)));
		// get_sym => 'g' 'e' 't'.
		q(nt(60), (t(16)+t(17)+t(13)));
		// set_sym => 's' 'e' 't'.
		q(nt(63), (t(18)+t(17)+t(13)));
		// toggle_sym => 't' 'o' 'g' 'g' 'l' 'e'.
		q(nt(66), (t(13)+t(19)+t(16)+t(16)+t(20)+t(17)));
		// grammar_sym => 'g'.
		q(nt(48), (t(16)));
		// grammar_sym => 'g' 'r' 'a' 'm' 'm' 'a' 'r'.
		q(nt(48), (t(16)+t(12)+t(21)+t(22)+t(22)+t(21)+t(12)));
		// internal_grammar_sym => 'i'.
		q(nt(49), (t(23)));
		// internal_grammar_sym => 'i' 'g'.
		q(nt(49), (t(23)+t(16)));
		// internal_grammar_sym => 'i' 'n' 't' 'e' 'r' 'n' 'a' 'l' '-' 'g' 'r' 'a' 'm' 'm' 'a' 'r'.
		q(nt(49), (t(23)+t(11)+t(13)+t(17)+t(12)+t(11)+t(21)+t(20)+t(24)+t(16)+t(12)+t(21)+t(22)+t(22)+t(21)+t(12)));
		// parse_sym => 'p'.
		q(nt(70), (t(25)));
		// parse_sym => 'p' 'a' 'r' 's' 'e'.
		q(nt(70), (t(25)+t(21)+t(12)+t(18)+t(17)));
		// start_sym => 's'.
		q(nt(51), (t(18)));
		// start_sym => 's' 't' 'a' 'r' 't'.
		q(nt(51), (t(18)+t(13)+t(21)+t(12)+t(13)));
		// reload_sym => 'r'.
		q(nt(53), (t(12)));
		// reload_sym => 'r' 'e' 'l' 'o' 'a' 'd'.
		q(nt(53), (t(12)+t(17)+t(20)+t(19)+t(21)+t(26)));
		// file_sym => 'f'.
		q(nt(54), (t(10)));
		// file_sym => 'f' 'i' 'l' 'e'.
		q(nt(54), (t(10)+t(23)+t(20)+t(17)));
		// help_sym => 'h'.
		q(nt(55), (t(27)));
		// help_sym => 'h' 'e' 'l' 'p'.
		q(nt(55), (t(27)+t(17)+t(20)+t(25)));
		// version_sym => 'v'.
		q(nt(58), (t(28)));
		// version_sym => 'v' 'e' 'r' 's' 'i' 'o' 'n'.
		q(nt(58), (t(28)+t(17)+t(12)+t(18)+t(23)+t(19)+t(11)));
		// quit_sym => 'e'.
		q(nt(59), (t(17)));
		// quit_sym => 'e' 'x' 'i' 't'.
		q(nt(59), (t(17)+t(29)+t(23)+t(13)));
		// quit_sym => 'q'.
		q(nt(59), (t(30)));
		// quit_sym => 'q' 'u' 'i' 't'.
		q(nt(59), (t(30)+t(31)+t(23)+t(13)));
		// get_sym => 'g' 'e' 't'.
		q(nt(60), (t(16)+t(17)+t(13)));
		// set_sym => 's' 'e' 't'.
		q(nt(63), (t(18)+t(17)+t(13)));
		// toggle_sym => 't' 'o' 'g'.
		q(nt(66), (t(13)+t(19)+t(16)));
		// toggle_sym => 't' 'o' 'g' 'g' 'l' 'e'.
		q(nt(66), (t(13)+t(19)+t(16)+t(16)+t(20)+t(17)));
		// enable_sym => '+'.
		q(nt(68), (t(32)));
		// enable_sym => 'e' 'n' 'a' 'b' 'l' 'e' __.
		q(nt(68), (t(17)+t(11)+t(21)+t(9)+t(20)+t(17)+nt(13)));
		// enable_sym => 'e' 'n' __.
		q(nt(68), (t(17)+t(11)+nt(13)));
		// enable_sym => 'o' 'n' __.
		q(nt(68), (t(19)+t(11)+nt(13)));
		// disable_sym => '-'.
		q(nt(69), (t(24)));
		// disable_sym => 'd' 'i' 's' 'a' 'b' 'l' 'e' __.
		q(nt(69), (t(26)+t(23)+t(18)+t(21)+t(9)+t(20)+t(17)+nt(13)));
		// disable_sym => 'd' 'i' 's' __.
		q(nt(69), (t(26)+t(23)+t(18)+nt(13)));
		// disable_sym => 'o' 'f' 'f' __.
		q(nt(69), (t(19)+t(10)+t(10)+nt(13)));
		// cmd_symbol => grammar_sym.
		q(nt(56), (nt(48)));
		// cmd_symbol => internal_grammar_sym.
		q(nt(56), (nt(49)));
		// cmd_symbol => start_sym.
		q(nt(56), (nt(51)));
		// cmd_symbol => reload_sym.
		q(nt(56), (nt(53)));
		// cmd_symbol => file_sym.
		q(nt(56), (nt(54)));
		// cmd_symbol => help_sym.
		q(nt(56), (nt(55)));
		// cmd_symbol => quit_sym.
		q(nt(56), (nt(59)));
		// cmd_symbol => get_sym.
		q(nt(56), (nt(60)));
		// cmd_symbol => set_sym.
		q(nt(56), (nt(63)));
		// cmd_symbol => toggle_sym.
		q(nt(56), (nt(66)));
		// cmd_symbol => enable_sym.
		q(nt(56), (nt(68)));
		// cmd_symbol => disable_sym.
		q(nt(56), (nt(69)));
		// cmd_symbol => parse_sym.
		q(nt(56), (nt(70)));
		// option => bool_option.
		q(nt(61), (nt(67)));
		// option => error_verbosity_opt.
		q(nt(61), (nt(71)));
		// bool_option => status_opt.
		q(nt(67), (nt(72)));
		// bool_option => colors_opt.
		q(nt(67), (nt(73)));
		// bool_option => print_ambiguity_opt.
		q(nt(67), (nt(74)));
		// bool_option => print_graphs_opt.
		q(nt(67), (nt(75)));
		// bool_option => print_rules_opt.
		q(nt(67), (nt(76)));
		// bool_option => print_facts_opt.
		q(nt(67), (nt(77)));
		// bool_option => debug_opt.
		q(nt(67), (nt(78)));
		// debug_opt => 'd'.
		q(nt(78), (t(26)));
		// debug_opt => 'd' 'e' 'b' 'u' 'g'.
		q(nt(78), (t(26)+t(17)+t(9)+t(31)+t(16)));
		// status_opt => 's'.
		q(nt(72), (t(18)));
		// status_opt => 's' 't' 'a' 't' 'u' 's'.
		q(nt(72), (t(18)+t(13)+t(21)+t(13)+t(31)+t(18)));
		// colors_opt => 'c'.
		q(nt(73), (t(33)));
		// colors_opt => 'c' 'o' 'l' 'o' 'r'.
		q(nt(73), (t(33)+t(19)+t(20)+t(19)+t(12)));
		// colors_opt => 'c' 'o' 'l' 'o' 'r' 's'.
		q(nt(73), (t(33)+t(19)+t(20)+t(19)+t(12)+t(18)));
		// print_ambiguity_opt => 'a'.
		q(nt(74), (t(21)));
		// print_ambiguity_opt => 'a' 'm' 'b' 'i' 'g' 'u' 'i' 't' 'y'.
		q(nt(74), (t(21)+t(22)+t(9)+t(23)+t(16)+t(31)+t(23)+t(13)+t(34)));
		// print_ambiguity_opt => 'p' 'r' 'i' 'n' 't' '-' 'a' 'm' 'b' 'i' 'g' 'u' 'i' 't' 'y'.
		q(nt(74), (t(25)+t(12)+t(23)+t(11)+t(13)+t(24)+t(21)+t(22)+t(9)+t(23)+t(16)+t(31)+t(23)+t(13)+t(34)));
		// print_graphs_opt => 'g'.
		q(nt(75), (t(16)));
		// print_graphs_opt => 'g' 'r' 'a' 'p' 'h' 's'.
		q(nt(75), (t(16)+t(12)+t(21)+t(25)+t(27)+t(18)));
		// print_graphs_opt => 'p' 'r' 'i' 'n' 't' '-' 'g' 'r' 'a' 'p' 'h' 's'.
		q(nt(75), (t(25)+t(12)+t(23)+t(11)+t(13)+t(24)+t(16)+t(12)+t(21)+t(25)+t(27)+t(18)));
		// print_rules_opt => 'p' 'r' 'i' 'n' 't' '-' 'r' 'u' 'l' 'e' 's'.
		q(nt(76), (t(25)+t(12)+t(23)+t(11)+t(13)+t(24)+t(12)+t(31)+t(20)+t(17)+t(18)));
		// print_rules_opt => 'r'.
		q(nt(76), (t(12)));
		// print_rules_opt => 'r' 'u' 'l' 'e' 's'.
		q(nt(76), (t(12)+t(31)+t(20)+t(17)+t(18)));
		// print_facts_opt => 'f'.
		q(nt(77), (t(10)));
		// print_facts_opt => 'f' 'a' 'c' 't' 's'.
		q(nt(77), (t(10)+t(21)+t(33)+t(13)+t(18)));
		// print_facts_opt => 'p' 'r' 'i' 'n' 't' '-' 'f' 'a' 'c' 't' 's'.
		q(nt(77), (t(25)+t(12)+t(23)+t(11)+t(13)+t(24)+t(10)+t(21)+t(33)+t(13)+t(18)));
		// error_verbosity_opt => 'e'.
		q(nt(71), (t(17)));
		// error_verbosity_opt => 'e' 'r' 'r' 'o' 'r' '-' 'v' 'e' 'r' 'b' 'o' 's' 'i' 't' 'y'.
		q(nt(71), (t(17)+t(12)+t(12)+t(19)+t(12)+t(24)+t(28)+t(17)+t(12)+t(9)+t(19)+t(18)+t(23)+t(13)+t(34)));
		// option_value => option_value_true.
		q(nt(65), (nt(79)));
		// option_value => option_value_false.
		q(nt(65), (nt(80)));
		// option_value => error_verbosity.
		q(nt(65), (nt(81)));
		// option_value_true => '1'.
		q(nt(79), (t(35)));
		// option_value_true => 'o' 'n'.
		q(nt(79), (t(19)+t(11)));
		// option_value_true => 't'.
		q(nt(79), (t(13)));
		// option_value_true => 't' 'r' 'u' 'e'.
		q(nt(79), (t(13)+t(12)+t(31)+t(17)));
		// option_value_true => 'y'.
		q(nt(79), (t(34)));
		// option_value_true => 'y' 'e' 's'.
		q(nt(79), (t(34)+t(17)+t(18)));
		// option_value_false => '0'.
		q(nt(80), (t(36)));
		// option_value_false => 'f'.
		q(nt(80), (t(10)));
		// option_value_false => 'f' 'a' 'l' 's' 'e'.
		q(nt(80), (t(10)+t(21)+t(20)+t(18)+t(17)));
		// option_value_false => 'n'.
		q(nt(80), (t(11)));
		// option_value_false => 'n' 'o'.
		q(nt(80), (t(11)+t(19)));
		// option_value_false => 'o' 'f' 'f'.
		q(nt(80), (t(19)+t(10)+t(10)));
		// error_verbosity => basic_sym.
		q(nt(81), (nt(82)));
		// error_verbosity => detailed_sym.
		q(nt(81), (nt(83)));
		// error_verbosity => root_cause_sym.
		q(nt(81), (nt(84)));
		// basic_sym => 'b'.
		q(nt(82), (t(9)));
		// basic_sym => 'b' 'a' 's' 'i' 'c'.
		q(nt(82), (t(9)+t(21)+t(18)+t(23)+t(33)));
		// detailed_sym => 'd'.
		q(nt(83), (t(26)));
		// detailed_sym => 'd' 'e' 't' 'a' 'i' 'l' 'e' 'd'.
		q(nt(83), (t(26)+t(17)+t(13)+t(21)+t(23)+t(20)+t(17)+t(26)));
		// root_cause_sym => 'r'.
		q(nt(84), (t(12)));
		// root_cause_sym => 'r' 'c'.
		q(nt(84), (t(12)+t(33)));
		// root_cause_sym => 'r' 'o' 'o' 't' '-' 'c' 'a' 'u' 's' 'e'.
		q(nt(84), (t(12)+t(19)+t(19)+t(13)+t(24)+t(33)+t(21)+t(31)+t(18)+t(17)));
		return q;
	}
};

using sp_node_type = tgf_repl_parser::sp_rw_node_type;
using rw_symbol_type = tgf_repl_parser::rw_symbol_type;
using symbol_type = tgf_repl_parser::symbol_type;
using nonterminal_type = tgf_repl_parser::nonterminal;

static inline bool is_non_terminal_node(const sp_node_type& n) {
	return std::holds_alternative<symbol_type>(n->value)
		&& get<symbol_type>(n->value).nt();
};

static inline std::function<bool(const sp_node_type&)> is_non_terminal_node() {
	return [](const sp_node_type& n) { return is_non_terminal_node(n); };
}

static inline bool is_non_terminal(const nonterminal_type nt, const sp_node_type& n) {
	return is_non_terminal_node(n)
		&& std::get<symbol_type>(n->value).n() == nt;
}

static inline std::function<bool(const sp_node_type&)> is_non_terminal(
	const nonterminal_type nt)
{
	return [nt](const sp_node_type& n) { return is_non_terminal(nt, n); };
}

static inline std::optional<sp_node_type> operator|(const sp_node_type& n,
	const nonterminal_type nt)
{
	auto v = n->child
		| std::ranges::views::filter(is_non_terminal(nt))
		| std::ranges::views::take(1);
	return v.empty() ? std::optional<sp_node_type>()
		: std::optional<sp_node_type>(v.front());
}

static inline std::optional<sp_node_type> operator|(const std::optional<sp_node_type>& n,
	const nonterminal_type nt)
{
	return n ? n.value() | nt : n;
}

static inline std::vector<sp_node_type> operator||(const sp_node_type& n,
	const nonterminal_type nt)
{
	std::vector<sp_node_type> nv;
	nv.reserve(n->child.size());
	for (const auto& c : n->child | std::ranges::views::filter(
					is_non_terminal(nt))) nv.push_back(c);
	return nv;
}

static inline std::vector<sp_node_type> operator||(const std::optional<sp_node_type>& n,
	const nonterminal_type nt)
{
	if (n) return n.value() || nt;
	return {};
}

static const auto only_child_extractor = [](const sp_node_type& n)
	-> std::optional<sp_node_type>
{
	if (n->child.size() != 1) return std::optional<sp_node_type>();
	return std::optional<sp_node_type>(n->child[0]);
};
using only_child_extractor_t = decltype(only_child_extractor);

static inline std::vector<sp_node_type> operator||(const std::vector<sp_node_type>& v,
	const only_child_extractor_t e)
{
	std::vector<sp_node_type> nv;
	for (const auto& n : v | std::ranges::views::transform(e))
		if (n.has_value()) nv.push_back(n.value());
	return nv;
}

static inline std::optional<sp_node_type> operator|(const std::optional<sp_node_type>& o,
	const only_child_extractor_t e)
{
	return o.has_value() ? e(o.value()) : std::optional<sp_node_type>();
}

static inline std::optional<sp_node_type> operator|(const sp_node_type& o,
	const only_child_extractor_t e)
{
	return e(o);
}

static const auto terminal_extractor = [](const sp_node_type& n)
	-> std::optional<char>
{
	if (std::holds_alternative<symbol_type>(n->value)) {
		auto& v = std::get<symbol_type>(n->value);
		if (!v.nt() && !v.is_null()) return std::optional<char>(v.t());
	}
	return std::optional<char>();
};

using terminal_extractor_t = decltype(terminal_extractor);

static inline std::optional<char> operator|(const std::optional<sp_node_type>& o,
	const terminal_extractor_t e)
{
	return o.has_value() ? e(o.value()) : std::optional<char>();
}

static const auto non_terminal_extractor = [](const sp_node_type& n)
	-> std::optional<size_t>
{
	if (std::holds_alternative<symbol_type>(n->value)) {
		auto& v = std::get<symbol_type>(n->value);
		if (v.nt()) return std::optional<char>(v.n());
	}
	return std::optional<size_t>();
};

using non_terminal_extractor_t = decltype(non_terminal_extractor);

static inline std::optional<size_t> operator|(const std::optional<sp_node_type>& o,
	const non_terminal_extractor_t e)
{
	return o.has_value() ? e(o.value()) : std::optional<size_t>();
}

static inline std::optional<size_t> operator|(const sp_node_type& o,
	const non_terminal_extractor_t e)
{
	return e(o);
}

static const auto size_t_extractor = [](const sp_node_type& n)
	-> std::optional<size_t>
{
	if (std::holds_alternative<size_t>(n->value))
		return std::optional<size_t>(std::get<size_t>(n->value));
	return std::optional<size_t>();
};
using size_t_extractor_t = decltype(size_t_extractor);

static inline std::optional<size_t> operator|(const std::optional<sp_node_type>& o,
	const size_t_extractor_t e)
{
	return o.has_value() ? e(o.value()) : std::optional<size_t>();
}

static inline std::optional<size_t> operator|(const sp_node_type& o,
	const size_t_extractor_t e)
{
	return e(o);
}

// is not a whitespace predicate
static const auto not_whitespace_predicate = [](const sp_node_type& n) {
	if (std::holds_alternative<symbol_type>(n->value)) {
		auto& v = std::get<symbol_type>(n->value);
		return !v.nt() || (v.n() != tgf_repl_parser::_ &&
				v.n() != tgf_repl_parser::__);
	}
	return true;
};

using not_whitespace_predicate_t = decltype(not_whitespace_predicate);

template <typename extractor_t>
struct stringify {
	stringify(const extractor_t& extractor,
		std::basic_stringstream<char>& stream)
		: stream(stream), extractor(extractor) {}
	sp_node_type operator()(const sp_node_type& n) {
		if (auto str = extractor(n); str) stream << str.value();
		return n;
	}
	std::basic_stringstream<char>& stream;
	const extractor_t& extractor;
};

template <typename extractor_t, typename predicate_t>
std::string make_string_with_skip(const extractor_t& extractor,
	predicate_t& skip, const sp_node_type& n)
{
	std::basic_stringstream<char> ss;
	stringify<extractor_t> sy(extractor, ss);
	idni::rewriter::post_order_tree_traverser<
		stringify<extractor_t>, predicate_t, sp_node_type>(sy, skip)(n);
	return ss.str();
}
} // tgf_repl_parsing namespace
#endif // __TGF_REPL_PARSER_H__
