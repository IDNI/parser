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
		_Rstart_1, directive, production, start_statement, __, directive_params, directive_param, _Rdirective_params_2, _Rdirective_params_3, sym, 
		expr1, disjunction, expr2, conjunction, expr3, negation, literals, literal, _Rliterals_4, _Rliterals_5, 
		terminal, nonterminal_, terminal_char, terminal_string, nonliteral, sym_start, sym_rest, _Rsym_6, _Rsym_7, group, 
		optional, repeat, plus, multi, unescaped_c, escaped_c, _Rterminal_char_8, _Runescaped_c_9, _Rescaped_c_10, unescaped_s, 
		escaped_s, _Rterminal_string_11, _Rterminal_string_12, _Runescaped_s_13, _Rescaped_s_14, _R__15, comment, _R___16, _Rcomment_17, _Rcomment_18, 
		_Rcomment_19, __neg_0, __neg_1, 
	};
	size_t id(const std::basic_string<char_type>& name) {
		return nts.get(name);
	}
	const std::basic_string<char_type>& name(size_t id) {
		return nts.get(id);
	}
private:
	std::vector<terminal_type> ts{
		'\0', '@', 'u', 's', 'e', '_', 'c', 'h', 'a', 
		'r', 'l', '.', ',', 'n', 'm', 'p', 'b', 'k', 't', 
		'd', 'i', 'g', 'o', 'f', 'w', 'x', '=', '>', '|', 
		'&', '~', '(', ')', '[', ']', '{', '}', '+', '*', 
		'\'', '\\', '/', '"', '\t', '\n', '\r', '#', 
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
			"_Rstart_1", "directive", "production", "start_statement", "__", "directive_params", "directive_param", "_Rdirective_params_2", "_Rdirective_params_3", "sym", 
			"expr1", "disjunction", "expr2", "conjunction", "expr3", "negation", "literals", "literal", "_Rliterals_4", "_Rliterals_5", 
			"terminal", "nonterminal_", "terminal_char", "terminal_string", "nonliteral", "sym_start", "sym_rest", "_Rsym_6", "_Rsym_7", "group", 
			"optional", "repeat", "plus", "multi", "unescaped_c", "escaped_c", "_Rterminal_char_8", "_Runescaped_c_9", "_Rescaped_c_10", "unescaped_s", 
			"escaped_s", "_Rterminal_string_11", "_Rterminal_string_12", "_Runescaped_s_13", "_Rescaped_s_14", "_R__15", "comment", "_R___16", "_Rcomment_17", "_Rcomment_18", 
			"_Rcomment_19", "__neg_0", "__neg_1", 
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
		//       _Rstart_0(9)         => _(7) statement(8).
		q(nt(9), (nt(7)+nt(8)));
		//       _Rstart_1(10)        => null.
		q(nt(10), (nul));
		//       _Rstart_1(10)        => _Rstart_0(9) _Rstart_1(10).
		q(nt(10), (nt(9)+nt(10)));
		//       start(6)             => _Rstart_1(10) _(7).
		q(nt(6), (nt(10)+nt(7)));
		//       statement(8)         => directive(11).
		q(nt(8), (nt(11)));
		//       statement(8)         => production(12).
		q(nt(8), (nt(12)));
		//       start_statement(13)  => _(7) statement(8) _(7).
		q(nt(13), (nt(7)+nt(8)+nt(7)));
		//       directive(11)        => '@' 'u' 's' 'e' '_' 'c' 'h' 'a' 'r' '_' 'c' 'l' 'a' 's' 's' __(14) directive_params(15) _(7) '.'.
		q(nt(11), (t(1)+t(2)+t(3)+t(4)+t(5)+t(6)+t(7)+t(8)+t(9)+t(5)+t(6)+t(10)+t(8)+t(3)+t(3)+nt(14)+nt(15)+nt(7)+t(11)));
		//       _Rdirective_params_2(17) => _(7) ',' _(7) directive_param(16).
		q(nt(17), (nt(7)+t(12)+nt(7)+nt(16)));
		//       _Rdirective_params_3(18) => null.
		q(nt(18), (nul));
		//       _Rdirective_params_3(18) => _Rdirective_params_2(17) _Rdirective_params_3(18).
		q(nt(18), (nt(17)+nt(18)));
		//       directive_params(15) => directive_param(16) _Rdirective_params_3(18).
		q(nt(15), (nt(16)+nt(18)));
		//       directive_param(16)  => 'a' 'l' 'n' 'u' 'm'.
		q(nt(16), (t(8)+t(10)+t(13)+t(2)+t(14)));
		//       directive_param(16)  => 'a' 'l' 'p' 'h' 'a'.
		q(nt(16), (t(8)+t(10)+t(15)+t(7)+t(8)));
		//       directive_param(16)  => 'b' 'l' 'a' 'n' 'k'.
		q(nt(16), (t(16)+t(10)+t(8)+t(13)+t(17)));
		//       directive_param(16)  => 'c' 'n' 't' 'r' 'l'.
		q(nt(16), (t(6)+t(13)+t(18)+t(9)+t(10)));
		//       directive_param(16)  => 'd' 'i' 'g' 'i' 't'.
		q(nt(16), (t(19)+t(20)+t(21)+t(20)+t(18)));
		//       directive_param(16)  => 'e' 'o' 'f'.
		q(nt(16), (t(4)+t(22)+t(23)));
		//       directive_param(16)  => 'g' 'r' 'a' 'p' 'h'.
		q(nt(16), (t(21)+t(9)+t(8)+t(15)+t(7)));
		//       directive_param(16)  => 'l' 'o' 'w' 'e' 'r'.
		q(nt(16), (t(10)+t(22)+t(24)+t(4)+t(9)));
		//       directive_param(16)  => 'p' 'r' 'i' 'n' 't' 'a' 'b' 'l' 'e'.
		q(nt(16), (t(15)+t(9)+t(20)+t(13)+t(18)+t(8)+t(16)+t(10)+t(4)));
		//       directive_param(16)  => 'p' 'u' 'n' 'c' 't'.
		q(nt(16), (t(15)+t(2)+t(13)+t(6)+t(18)));
		//       directive_param(16)  => 's' 'p' 'a' 'c' 'e'.
		q(nt(16), (t(3)+t(15)+t(8)+t(6)+t(4)));
		//       directive_param(16)  => 'u' 'p' 'p' 'e' 'r'.
		q(nt(16), (t(2)+t(15)+t(15)+t(4)+t(9)));
		//       directive_param(16)  => 'x' 'd' 'i' 'g' 'i' 't'.
		q(nt(16), (t(25)+t(19)+t(20)+t(21)+t(20)+t(18)));
		//       production(12)       => sym(19) _(7) '=' '>' _(7) expr1(20) _(7) '.'.
		q(nt(12), (nt(19)+nt(7)+t(26)+t(27)+nt(7)+nt(20)+nt(7)+t(11)));
		//       expr1(20)            => disjunction(21).
		q(nt(20), (nt(21)));
		//       expr1(20)            => expr2(22).
		q(nt(20), (nt(22)));
		//       expr2(22)            => conjunction(23).
		q(nt(22), (nt(23)));
		//       expr2(22)            => expr3(24).
		q(nt(22), (nt(24)));
		//       expr3(24)            => negation(25).
		q(nt(24), (nt(25)));
		//       expr3(24)            => literals(26).
		q(nt(24), (nt(26)));
		//       disjunction(21)      => expr1(20) _(7) '|' _(7) expr2(22).
		q(nt(21), (nt(20)+nt(7)+t(28)+nt(7)+nt(22)));
		//       conjunction(23)      => expr2(22) _(7) '&' _(7) expr3(24).
		q(nt(23), (nt(22)+nt(7)+t(29)+nt(7)+nt(24)));
		//       negation(25)         => '~' _(7) expr3(24).
		q(nt(25), (t(30)+nt(7)+nt(24)));
		//       _Rliterals_4(28)     => __(14) literal(27).
		q(nt(28), (nt(14)+nt(27)));
		//       _Rliterals_5(29)     => null.
		q(nt(29), (nul));
		//       _Rliterals_5(29)     => _Rliterals_4(28) _Rliterals_5(29).
		q(nt(29), (nt(28)+nt(29)));
		//       literals(26)         => literal(27) _Rliterals_5(29).
		q(nt(26), (nt(27)+nt(29)));
		//       literal(27)          => terminal(30).
		q(nt(27), (nt(30)));
		//       literal(27)          => nonterminal_(31).
		q(nt(27), (nt(31)));
		//       terminal(30)         => terminal_char(32).
		q(nt(30), (nt(32)));
		//       terminal(30)         => terminal_string(33).
		q(nt(30), (nt(33)));
		//       nonterminal_(31)     => sym(19).
		q(nt(31), (nt(19)));
		//       nonterminal_(31)     => nonliteral(34).
		q(nt(31), (nt(34)));
		//       _Rsym_6(37)          => sym_rest(36).
		q(nt(37), (nt(36)));
		//       _Rsym_7(38)          => null.
		q(nt(38), (nul));
		//       _Rsym_7(38)          => _Rsym_6(37) _Rsym_7(38).
		q(nt(38), (nt(37)+nt(38)));
		//       sym(19)              => sym_start(35) _Rsym_7(38).
		q(nt(19), (nt(35)+nt(38)));
		//       sym_start(35)        => '_'.
		q(nt(35), (t(5)));
		//       sym_start(35)        => alpha(3).
		q(nt(35), (nt(3)));
		//       sym_rest(36)         => '_'.
		q(nt(36), (t(5)));
		//       sym_rest(36)         => alnum(2).
		q(nt(36), (nt(2)));
		//       nonliteral(34)       => group(39).
		q(nt(34), (nt(39)));
		//       nonliteral(34)       => optional(40).
		q(nt(34), (nt(40)));
		//       nonliteral(34)       => repeat(41).
		q(nt(34), (nt(41)));
		//       nonliteral(34)       => plus(42).
		q(nt(34), (nt(42)));
		//       nonliteral(34)       => multi(43).
		q(nt(34), (nt(43)));
		//       group(39)            => '(' _(7) expr1(20) _(7) ')'.
		q(nt(39), (t(31)+nt(7)+nt(20)+nt(7)+t(32)));
		//       optional(40)         => '[' _(7) expr1(20) _(7) ']'.
		q(nt(40), (t(33)+nt(7)+nt(20)+nt(7)+t(34)));
		//       repeat(41)           => '{' _(7) expr1(20) _(7) '}'.
		q(nt(41), (t(35)+nt(7)+nt(20)+nt(7)+t(36)));
		//       plus(42)             => literal(27) _(7) '+'.
		q(nt(42), (nt(27)+nt(7)+t(37)));
		//       multi(43)            => literal(27) _(7) '*'.
		q(nt(43), (nt(27)+nt(7)+t(38)));
		//       _Rterminal_char_8(46) => unescaped_c(44).
		q(nt(46), (nt(44)));
		//       _Rterminal_char_8(46) => escaped_c(45).
		q(nt(46), (nt(45)));
		//       terminal_char(32)    => '\'' _Rterminal_char_8(46) '\''.
		q(nt(32), (t(39)+nt(46)+t(39)));
		//       _Runescaped_c_9(47)  => '\''.
		q(nt(47), (t(39)));
		//       _Runescaped_c_9(47)  => '\\'.
		q(nt(47), (t(40)));
		//       __neg_0(61)          => _Runescaped_c_9(47).
		q(nt(61), (nt(47)));
		//       unescaped_c(44)      => printable(5) & ~( __neg_0(61) ).	 # conjunctive
		q(nt(44), (nt(5)) & ~(nt(61)));
		//       _Rescaped_c_10(48)   => '\''.
		q(nt(48), (t(39)));
		//       _Rescaped_c_10(48)   => '/'.
		q(nt(48), (t(41)));
		//       _Rescaped_c_10(48)   => '\\'.
		q(nt(48), (t(40)));
		//       _Rescaped_c_10(48)   => 'b'.
		q(nt(48), (t(16)));
		//       _Rescaped_c_10(48)   => 'f'.
		q(nt(48), (t(23)));
		//       _Rescaped_c_10(48)   => 'n'.
		q(nt(48), (t(13)));
		//       _Rescaped_c_10(48)   => 'r'.
		q(nt(48), (t(9)));
		//       _Rescaped_c_10(48)   => 't'.
		q(nt(48), (t(18)));
		//       escaped_c(45)        => '\\' _Rescaped_c_10(48).
		q(nt(45), (t(40)+nt(48)));
		//       _Rterminal_string_11(51) => unescaped_s(49).
		q(nt(51), (nt(49)));
		//       _Rterminal_string_11(51) => escaped_s(50).
		q(nt(51), (nt(50)));
		//       _Rterminal_string_12(52) => null.
		q(nt(52), (nul));
		//       _Rterminal_string_12(52) => _Rterminal_string_11(51) _Rterminal_string_12(52).
		q(nt(52), (nt(51)+nt(52)));
		//       terminal_string(33)  => '"' _Rterminal_string_12(52) '"'.
		q(nt(33), (t(42)+nt(52)+t(42)));
		//       _Runescaped_s_13(53) => '"'.
		q(nt(53), (t(42)));
		//       _Runescaped_s_13(53) => '\\'.
		q(nt(53), (t(40)));
		//       __neg_1(62)          => _Runescaped_s_13(53).
		q(nt(62), (nt(53)));
		//       unescaped_s(49)      => printable(5) & ~( __neg_1(62) ).	 # conjunctive
		q(nt(49), (nt(5)) & ~(nt(62)));
		//       _Rescaped_s_14(54)   => '"'.
		q(nt(54), (t(42)));
		//       _Rescaped_s_14(54)   => '/'.
		q(nt(54), (t(41)));
		//       _Rescaped_s_14(54)   => '\\'.
		q(nt(54), (t(40)));
		//       _Rescaped_s_14(54)   => 'b'.
		q(nt(54), (t(16)));
		//       _Rescaped_s_14(54)   => 'f'.
		q(nt(54), (t(23)));
		//       _Rescaped_s_14(54)   => 'n'.
		q(nt(54), (t(13)));
		//       _Rescaped_s_14(54)   => 'r'.
		q(nt(54), (t(9)));
		//       _Rescaped_s_14(54)   => 't'.
		q(nt(54), (t(18)));
		//       escaped_s(50)        => '\\' _Rescaped_s_14(54).
		q(nt(50), (t(40)+nt(54)));
		//       _R__15(55)           => null.
		q(nt(55), (nul));
		//       _R__15(55)           => __(14).
		q(nt(55), (nt(14)));
		//       _(7)                 => _R__15(55).
		q(nt(7), (nt(55)));
		//       _R___16(57)          => space(4).
		q(nt(57), (nt(4)));
		//       _R___16(57)          => comment(56).
		q(nt(57), (nt(56)));
		//       __(14)               => _R___16(57) _(7).
		q(nt(14), (nt(57)+nt(7)));
		//       _Rcomment_17(58)     => '\t'.
		q(nt(58), (t(43)));
		//       _Rcomment_17(58)     => printable(5).
		q(nt(58), (nt(5)));
		//       _Rcomment_18(59)     => null.
		q(nt(59), (nul));
		//       _Rcomment_18(59)     => _Rcomment_17(58) _Rcomment_18(59).
		q(nt(59), (nt(58)+nt(59)));
		//       _Rcomment_19(60)     => '\n'.
		q(nt(60), (t(44)));
		//       _Rcomment_19(60)     => '\r'.
		q(nt(60), (t(45)));
		//       _Rcomment_19(60)     => eof(1).
		q(nt(60), (nt(1)));
		//       comment(56)          => '#' _Rcomment_18(59) _Rcomment_19(60).
		q(nt(56), (t(46)+nt(59)+nt(60)));
		return q;
	}
};

#endif // __TGF_PARSER_H__
