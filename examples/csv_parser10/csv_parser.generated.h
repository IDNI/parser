// This file is generated from a file examples/csv_parser10/csv.tgf by
//       https://github.com/IDNI/parser/tools/tgf
//
#ifndef __CSV_PARSER_H__
#define __CSV_PARSER_H__

#include <string.h>

#include "parser.h"

struct csv_parser {
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
	static csv_parser& instance() { static csv_parser i; return i; }	csv_parser() :
		nts(load_nonterminals()), cc(load_cc()),
		g(nts, load_prods(), nt(25), cc, load_grammar_opts()),
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
		nul, digit, printable, integer, __E_integer_0, __E_integer_1, quote, esc, escaping, unescaped, 
		escaped, strchar, str, __E_str_2, __E___E_str_2_3, nullvalue, val, eol, __E_eol_4, row, 
		__E_row_5, __E_row_6, rows, __E_rows_7, __E_rows_8, start, __N_0, 
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
		'\0', '-', '"', '\\', '\r', '\n', ',', 
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
			"", "digit", "printable", "integer", "__E_integer_0", "__E_integer_1", "quote", "esc", "escaping", "unescaped", 
			"escaped", "strchar", "str", "__E_str_2", "__E___E_str_2_3", "nullvalue", "val", "eol", "__E_eol_4", "row", 
			"__E_row_5", "__E_row_6", "rows", "__E_rows_7", "__E_rows_8", "start", "__N_0", 
		}) nts.get(nt);
		return nts;
	}
	idni::char_class_fns<terminal_type> load_cc() {
		return idni::predefined_char_classes<char_type, terminal_type>({
			"digit",
			"printable",
		}, nts);
	}
	grammar_type::options load_grammar_opts() {
		grammar_type::options o;
		o.transform_negation = false;
		o.auto_disambiguate = true;
		o.shaping.trim_terminals = false;
		o.shaping.inline_char_classes = false;
		return o;
	}
	options load_opts() {
		options o;
		return o;
	}
	idni::prods<char_type, terminal_type> load_prods() {
		idni::prods<char_type, terminal_type> q,
			nul(symbol_type{});
		//       __E_integer_0(4)     => '-'.
		q(nt(4), (t(1)));
		//       __E_integer_0(4)     => null.
		q(nt(4), (nul));
		//       __E_integer_1(5)     => digit(1).
		q(nt(5), (nt(1)));
		//       __E_integer_1(5)     => digit(1) __E_integer_1(5).
		q(nt(5), (nt(1)+nt(5)));
		//       integer(3)           => __E_integer_0(4) __E_integer_1(5).
		q(nt(3), (nt(4)+nt(5)));
		//       quote(6)             => '"'.
		q(nt(6), (t(2)));
		//       esc(7)               => '\\'.
		q(nt(7), (t(3)));
		//       escaping(8)          => quote(6).
		q(nt(8), (nt(6)));
		//       escaping(8)          => esc(7).
		q(nt(8), (nt(7)));
		//       __N_0(26)            => escaping(8).
		q(nt(26), (nt(8)));
		//       unescaped(9)         => printable(2) & ~( __N_0(26) ).	 # conjunctive
		q(nt(9), (nt(2)) & ~(nt(26)));
		//       escaped(10)          => esc(7) escaping(8).
		q(nt(10), (nt(7)+nt(8)));
		//       strchar(11)          => unescaped(9).
		q(nt(11), (nt(9)));
		//       strchar(11)          => escaped(10).
		q(nt(11), (nt(10)));
		//       __E___E_str_2_3(14)  => null.
		q(nt(14), (nul));
		//       __E___E_str_2_3(14)  => strchar(11) __E___E_str_2_3(14).
		q(nt(14), (nt(11)+nt(14)));
		//       __E_str_2(13)        => __E___E_str_2_3(14).
		q(nt(13), (nt(14)));
		//       str(12)              => quote(6) __E_str_2(13) quote(6).
		q(nt(12), (nt(6)+nt(13)+nt(6)));
		//       nullvalue(15)        => null.
		q(nt(15), (nul));
		//       val(16)              => integer(3).
		q(nt(16), (nt(3)));
		//       val(16)              => str(12).
		q(nt(16), (nt(12)));
		//       val(16)              => nullvalue(15).
		q(nt(16), (nt(15)));
		//       __E_eol_4(18)        => '\r'.
		q(nt(18), (t(4)));
		//       __E_eol_4(18)        => null.
		q(nt(18), (nul));
		//       eol(17)              => __E_eol_4(18) '\n'.
		q(nt(17), (nt(18)+t(5)));
		//       __E_row_5(20)        => ',' val(16).
		q(nt(20), (t(6)+nt(16)));
		//       __E_row_6(21)        => null.
		q(nt(21), (nul));
		//       __E_row_6(21)        => __E_row_5(20) __E_row_6(21).
		q(nt(21), (nt(20)+nt(21)));
		//       row(19)              => val(16) __E_row_6(21).
		q(nt(19), (nt(16)+nt(21)));
		//       __E_rows_7(23)       => eol(17) row(19).
		q(nt(23), (nt(17)+nt(19)));
		//       __E_rows_8(24)       => null.
		q(nt(24), (nul));
		//       __E_rows_8(24)       => __E_rows_7(23) __E_rows_8(24).
		q(nt(24), (nt(23)+nt(24)));
		//       rows(22)             => row(19) __E_rows_8(24).
		q(nt(22), (nt(19)+nt(24)));
		//       start(25)            => rows(22).
		q(nt(25), (nt(22)));
		return q;
	}
};

#endif // __CSV_PARSER_H__
