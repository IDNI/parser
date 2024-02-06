// This file is generated from a file src/tgf.tgf by
//       https://github.com/IDNI/parser/tools/tgf
//
#ifndef __TGF_PARSER_H__
#define __TGF_PARSER_H__
#include <string.h>
#include "parser.h"
struct tgf_parser {
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
	tgf_parser() :
		nts(load_nonterminals()), cc(load_cc()),
		g(nts, load_prods(), nt(6), cc), p(g, load_opts()) {}
	std::unique_ptr<forest_type> parse(const char_type* data, size_t size=0,
		parse_options po = {}) { return p.parse(data, size, po); }
	std::unique_ptr<forest_type> parse(std::basic_istream<char_type>& is,
		parse_options po = {}) { return p.parse(is, po); }
	std::unique_ptr<forest_type> parse(std::string fn, mmap_mode m,
		parse_options po = {}) { return p.parse(fn, m, po); }
#ifndef WIN32
	std::unique_ptr<forest_type> parse(int fd, parse_options po = {})
		{ return p.parse(fd, po); }
#endif //WIN32
	bool found() { return p.found(); }
	typename parser_type::error get_error() { return p.get_error(); }
	enum nonterminal {
		nul, eof, alnum, alpha, space, printable, start, ws, statement, _Rstart_0, 
		_Rstart_1, directive, production, ws_required, directive_params, directive_param, _Rdirective_params_2, _Rdirective_params_3, sym, expr1, 
		disjunction, expr2, conjunction, expr3, negation, literals, literal, _Rliterals_4, _Rliterals_5, terminal, 
		nonterminal, terminal_char, terminal_string, nonliteral, sym_start, sym_rest, _Rsym_6, _Rsym_7, group, optional, 
		repeat, plus, multi, unescaped_c, escaped_c, _Rterminal_char_8, _Runescaped_c_9, _Rescaped_c_10, unescaped_s, escaped_s, 
		_Rterminal_string_11, _Rterminal_string_12, _Runescaped_s_13, _Rescaped_s_14, ws_comment, ws_char, _Rws_comment_15, _Rws_comment_16, eol, __neg_0, 
		__neg_1, 
	};
	size_t id(const std::basic_string<char_type>& name) {
		return nts.get(name);
	}
private:
	std::vector<terminal_type> ts{
		'\0', '@', 'u', 's', 'e', '_', 'c', 'h', 'a', 
		'r', 'l', '.', ',', 'n', 'm', 'p', 'b', 'k', 't', 
		'd', 'i', 'g', 'o', 'f', 'w', 'x', '=', '>', '|', 
		'&', '~', '(', ')', '[', ']', '{', '}', '+', '*', 
		'\'', '\\', '/', '"', '#', '\t', '\n', '\r', 
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
			"", "eof", "alnum", "alpha", "space", "printable", "start", "ws", "statement", "_Rstart_0", 
			"_Rstart_1", "directive", "production", "ws_required", "directive_params", "directive_param", "_Rdirective_params_2", "_Rdirective_params_3", "sym", "expr1", 
			"disjunction", "expr2", "conjunction", "expr3", "negation", "literals", "literal", "_Rliterals_4", "_Rliterals_5", "terminal", 
			"nonterminal", "terminal_char", "terminal_string", "nonliteral", "sym_start", "sym_rest", "_Rsym_6", "_Rsym_7", "group", "optional", 
			"repeat", "plus", "multi", "unescaped_c", "escaped_c", "_Rterminal_char_8", "_Runescaped_c_9", "_Rescaped_c_10", "unescaped_s", "escaped_s", 
			"_Rterminal_string_11", "_Rterminal_string_12", "_Runescaped_s_13", "_Rescaped_s_14", "ws_comment", "ws_char", "_Rws_comment_15", "_Rws_comment_16", "eol", "__neg_0", 
			"__neg_1", 
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
		// _Rstart_0 => ws statement.
		q(nt(9), (nt(7)+nt(8)));
		// _Rstart_1 => null.
		q(nt(10), (nul));
		// _Rstart_1 => _Rstart_0 _Rstart_1.
		q(nt(10), (nt(9)+nt(10)));
		// start => _Rstart_1 ws.
		q(nt(6), (nt(10)+nt(7)));
		// statement => directive.
		q(nt(8), (nt(11)));
		// statement => production.
		q(nt(8), (nt(12)));
		// directive => '@' 'u' 's' 'e' '_' 'c' 'h' 'a' 'r' '_' 'c' 'l' 'a' 's' 's' ws_required directive_params ws '.'.
		q(nt(11), (t(1)+t(2)+t(3)+t(4)+t(5)+t(6)+t(7)+t(8)+t(9)+t(5)+t(6)+t(10)+t(8)+t(3)+t(3)+nt(13)+nt(14)+nt(7)+t(11)));
		// _Rdirective_params_2 => ws ',' ws directive_param.
		q(nt(16), (nt(7)+t(12)+nt(7)+nt(15)));
		// _Rdirective_params_3 => null.
		q(nt(17), (nul));
		// _Rdirective_params_3 => _Rdirective_params_2 _Rdirective_params_3.
		q(nt(17), (nt(16)+nt(17)));
		// directive_params => directive_param _Rdirective_params_3.
		q(nt(14), (nt(15)+nt(17)));
		// directive_param => 'a' 'l' 'n' 'u' 'm'.
		q(nt(15), (t(8)+t(10)+t(13)+t(2)+t(14)));
		// directive_param => 'a' 'l' 'p' 'h' 'a'.
		q(nt(15), (t(8)+t(10)+t(15)+t(7)+t(8)));
		// directive_param => 'b' 'l' 'a' 'n' 'k'.
		q(nt(15), (t(16)+t(10)+t(8)+t(13)+t(17)));
		// directive_param => 'c' 'n' 't' 'r' 'l'.
		q(nt(15), (t(6)+t(13)+t(18)+t(9)+t(10)));
		// directive_param => 'd' 'i' 'g' 'i' 't'.
		q(nt(15), (t(19)+t(20)+t(21)+t(20)+t(18)));
		// directive_param => 'e' 'o' 'f'.
		q(nt(15), (t(4)+t(22)+t(23)));
		// directive_param => 'g' 'r' 'a' 'p' 'h'.
		q(nt(15), (t(21)+t(9)+t(8)+t(15)+t(7)));
		// directive_param => 'l' 'o' 'w' 'e' 'r'.
		q(nt(15), (t(10)+t(22)+t(24)+t(4)+t(9)));
		// directive_param => 'p' 'r' 'i' 'n' 't' 'a' 'b' 'l' 'e'.
		q(nt(15), (t(15)+t(9)+t(20)+t(13)+t(18)+t(8)+t(16)+t(10)+t(4)));
		// directive_param => 'p' 'u' 'n' 'c' 't'.
		q(nt(15), (t(15)+t(2)+t(13)+t(6)+t(18)));
		// directive_param => 's' 'p' 'a' 'c' 'e'.
		q(nt(15), (t(3)+t(15)+t(8)+t(6)+t(4)));
		// directive_param => 'u' 'p' 'p' 'e' 'r'.
		q(nt(15), (t(2)+t(15)+t(15)+t(4)+t(9)));
		// directive_param => 'x' 'd' 'i' 'g' 'i' 't'.
		q(nt(15), (t(25)+t(19)+t(20)+t(21)+t(20)+t(18)));
		// production => sym ws '=' '>' ws expr1 ws '.'.
		q(nt(12), (nt(18)+nt(7)+t(26)+t(27)+nt(7)+nt(19)+nt(7)+t(11)));
		// expr1 => disjunction.
		q(nt(19), (nt(20)));
		// expr1 => expr2.
		q(nt(19), (nt(21)));
		// expr2 => conjunction.
		q(nt(21), (nt(22)));
		// expr2 => expr3.
		q(nt(21), (nt(23)));
		// expr3 => negation.
		q(nt(23), (nt(24)));
		// expr3 => literals.
		q(nt(23), (nt(25)));
		// disjunction => expr1 ws '|' ws expr2.
		q(nt(20), (nt(19)+nt(7)+t(28)+nt(7)+nt(21)));
		// conjunction => expr2 ws '&' ws expr3.
		q(nt(22), (nt(21)+nt(7)+t(29)+nt(7)+nt(23)));
		// negation => '~' ws expr3.
		q(nt(24), (t(30)+nt(7)+nt(23)));
		// _Rliterals_4 => ws_required literal.
		q(nt(27), (nt(13)+nt(26)));
		// _Rliterals_5 => null.
		q(nt(28), (nul));
		// _Rliterals_5 => _Rliterals_4 _Rliterals_5.
		q(nt(28), (nt(27)+nt(28)));
		// literals => literal _Rliterals_5.
		q(nt(25), (nt(26)+nt(28)));
		// literal => terminal.
		q(nt(26), (nt(29)));
		// literal => nonterminal.
		q(nt(26), (nt(30)));
		// terminal => terminal_char.
		q(nt(29), (nt(31)));
		// terminal => terminal_string.
		q(nt(29), (nt(32)));
		// nonterminal => sym.
		q(nt(30), (nt(18)));
		// nonterminal => nonliteral.
		q(nt(30), (nt(33)));
		// _Rsym_6 => sym_rest.
		q(nt(36), (nt(35)));
		// _Rsym_7 => null.
		q(nt(37), (nul));
		// _Rsym_7 => _Rsym_6 _Rsym_7.
		q(nt(37), (nt(36)+nt(37)));
		// sym => sym_start _Rsym_7.
		q(nt(18), (nt(34)+nt(37)));
		// sym_start => '_'.
		q(nt(34), (t(5)));
		// sym_start => alpha.
		q(nt(34), (nt(3)));
		// sym_rest => '_'.
		q(nt(35), (t(5)));
		// sym_rest => alnum.
		q(nt(35), (nt(2)));
		// nonliteral => group.
		q(nt(33), (nt(38)));
		// nonliteral => optional.
		q(nt(33), (nt(39)));
		// nonliteral => repeat.
		q(nt(33), (nt(40)));
		// nonliteral => plus.
		q(nt(33), (nt(41)));
		// nonliteral => multi.
		q(nt(33), (nt(42)));
		// group => '(' ws expr1 ws ')'.
		q(nt(38), (t(31)+nt(7)+nt(19)+nt(7)+t(32)));
		// optional => '[' ws expr1 ws ']'.
		q(nt(39), (t(33)+nt(7)+nt(19)+nt(7)+t(34)));
		// repeat => '{' ws expr1 ws '}'.
		q(nt(40), (t(35)+nt(7)+nt(19)+nt(7)+t(36)));
		// plus => literal ws '+'.
		q(nt(41), (nt(26)+nt(7)+t(37)));
		// multi => literal ws '*'.
		q(nt(42), (nt(26)+nt(7)+t(38)));
		// _Rterminal_char_8 => unescaped_c.
		q(nt(45), (nt(43)));
		// _Rterminal_char_8 => escaped_c.
		q(nt(45), (nt(44)));
		// terminal_char => '\'' _Rterminal_char_8 '\''.
		q(nt(31), (t(39)+nt(45)+t(39)));
		// _Runescaped_c_9 => '\''.
		q(nt(46), (t(39)));
		// _Runescaped_c_9 => '\\'.
		q(nt(46), (t(40)));
		// __neg_0 => _Runescaped_c_9.
		q(nt(59), (nt(46)));
		// unescaped_c => printable & ~( __neg_0 ).
		q(nt(43), (nt(5)) & ~(nt(59)));
		// _Rescaped_c_10 => '\''.
		q(nt(47), (t(39)));
		// _Rescaped_c_10 => '/'.
		q(nt(47), (t(41)));
		// _Rescaped_c_10 => '\\'.
		q(nt(47), (t(40)));
		// _Rescaped_c_10 => 'b'.
		q(nt(47), (t(16)));
		// _Rescaped_c_10 => 'f'.
		q(nt(47), (t(23)));
		// _Rescaped_c_10 => 'n'.
		q(nt(47), (t(13)));
		// _Rescaped_c_10 => 'r'.
		q(nt(47), (t(9)));
		// _Rescaped_c_10 => 't'.
		q(nt(47), (t(18)));
		// escaped_c => '\\' _Rescaped_c_10.
		q(nt(44), (t(40)+nt(47)));
		// _Rterminal_string_11 => unescaped_s.
		q(nt(50), (nt(48)));
		// _Rterminal_string_11 => escaped_s.
		q(nt(50), (nt(49)));
		// _Rterminal_string_12 => null.
		q(nt(51), (nul));
		// _Rterminal_string_12 => _Rterminal_string_11 _Rterminal_string_12.
		q(nt(51), (nt(50)+nt(51)));
		// terminal_string => '"' _Rterminal_string_12 '"'.
		q(nt(32), (t(42)+nt(51)+t(42)));
		// _Runescaped_s_13 => '"'.
		q(nt(52), (t(42)));
		// _Runescaped_s_13 => '\\'.
		q(nt(52), (t(40)));
		// __neg_1 => _Runescaped_s_13.
		q(nt(60), (nt(52)));
		// unescaped_s => printable & ~( __neg_1 ).
		q(nt(48), (nt(5)) & ~(nt(60)));
		// _Rescaped_s_14 => '"'.
		q(nt(53), (t(42)));
		// _Rescaped_s_14 => '/'.
		q(nt(53), (t(41)));
		// _Rescaped_s_14 => '\\'.
		q(nt(53), (t(40)));
		// _Rescaped_s_14 => 'b'.
		q(nt(53), (t(16)));
		// _Rescaped_s_14 => 'f'.
		q(nt(53), (t(23)));
		// _Rescaped_s_14 => 'n'.
		q(nt(53), (t(13)));
		// _Rescaped_s_14 => 'r'.
		q(nt(53), (t(9)));
		// _Rescaped_s_14 => 't'.
		q(nt(53), (t(18)));
		// escaped_s => '\\' _Rescaped_s_14.
		q(nt(49), (t(40)+nt(53)));
		// ws => null.
		q(nt(7), (nul));
		// ws => ws_required.
		q(nt(7), (nt(13)));
		// ws_required => space ws.
		q(nt(13), (nt(4)+nt(7)));
		// ws_required => ws_comment ws.
		q(nt(13), (nt(54)+nt(7)));
		// _Rws_comment_15 => ws_char.
		q(nt(56), (nt(55)));
		// _Rws_comment_16 => null.
		q(nt(57), (nul));
		// _Rws_comment_16 => _Rws_comment_15 _Rws_comment_16.
		q(nt(57), (nt(56)+nt(57)));
		// ws_comment => '#' _Rws_comment_16 eol.
		q(nt(54), (t(43)+nt(57)+nt(58)));
		// ws_char => '\t'.
		q(nt(55), (t(44)));
		// ws_char => printable.
		q(nt(55), (nt(5)));
		// eol => '\n'.
		q(nt(58), (t(45)));
		// eol => '\r'.
		q(nt(58), (t(46)));
		// eol => eof.
		q(nt(58), (nt(1)));
		return q;
	}
};
#endif // __TGF_PARSER_H__
