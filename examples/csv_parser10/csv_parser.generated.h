// This file is generated from a file examples/csv_parser10/csv.tgf by
//       https://github.com/IDNI/parser/tools/tgf
//
#ifndef __CSV_PARSER_H__
#define __CSV_PARSER_H__
#include <string.h>
#include "parser.h"
struct csv_parser {
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
	csv_parser() :
		nts(load_nonterminals()), cc(load_cc()),
		g(nts, load_prods(), nt(25), cc), p(g, load_opts()) {}
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
		nul, digit, printable, integer, _Rinteger_0, _Rinteger_1, quote, esc, escaping, unescaped, 
		escaped, strchar, str, _Rstr_2, _Rstr_3, nullvalue, val, eol, _Reol_4, row, 
		_Rrow_5, _Rrow_6, rows, _Rrows_7, _Rrows_8, start, __neg_0, 
	};
	size_t id(const std::basic_string<char_type>& name) {
		return nts.get(name);
	}
	const std::basic_string<char_type>& name(size_t id) {
		return nts.get(id);
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
			"", "digit", "printable", "integer", "_Rinteger_0", "_Rinteger_1", "quote", "esc", "escaping", "unescaped", 
			"escaped", "strchar", "str", "_Rstr_2", "_Rstr_3", "nullvalue", "val", "eol", "_Reol_4", "row", 
			"_Rrow_5", "_Rrow_6", "rows", "_Rrows_7", "_Rrows_8", "start", "__neg_0", 
		}) nts.get(nt);
		return nts;
	}
	idni::char_class_fns<terminal_type> load_cc() {
		return idni::predefined_char_classes<char_type, terminal_type>({
			"digit",
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
		// _Rinteger_0 => null.
		q(nt(4), (nul));
		// _Rinteger_0 => '-'.
		q(nt(4), (t(1)));
		// _Rinteger_1 => digit.
		q(nt(5), (nt(1)));
		// _Rinteger_1 => digit _Rinteger_1.
		q(nt(5), (nt(1)+nt(5)));
		// integer => _Rinteger_0 _Rinteger_1.
		q(nt(3), (nt(4)+nt(5)));
		// quote => '"'.
		q(nt(6), (t(2)));
		// esc => '\\'.
		q(nt(7), (t(3)));
		// escaping => quote.
		q(nt(8), (nt(6)));
		// escaping => esc.
		q(nt(8), (nt(7)));
		// __neg_0 => escaping.
		q(nt(26), (nt(8)));
		// unescaped => printable & ~( __neg_0 ).
		q(nt(9), (nt(2)) & ~(nt(26)));
		// escaped => esc escaping.
		q(nt(10), (nt(7)+nt(8)));
		// strchar => unescaped.
		q(nt(11), (nt(9)));
		// strchar => escaped.
		q(nt(11), (nt(10)));
		// _Rstr_2 => null.
		q(nt(13), (nul));
		// _Rstr_2 => strchar _Rstr_2.
		q(nt(13), (nt(11)+nt(13)));
		// _Rstr_3 => _Rstr_2.
		q(nt(14), (nt(13)));
		// str => quote _Rstr_3 quote.
		q(nt(12), (nt(6)+nt(14)+nt(6)));
		// nullvalue => null.
		q(nt(15), (nul));
		// val => integer.
		q(nt(16), (nt(3)));
		// val => str.
		q(nt(16), (nt(12)));
		// val => nullvalue.
		q(nt(16), (nt(15)));
		// _Reol_4 => null.
		q(nt(18), (nul));
		// _Reol_4 => '\r'.
		q(nt(18), (t(4)));
		// eol => _Reol_4 '\n'.
		q(nt(17), (nt(18)+t(5)));
		// _Rrow_5 => ',' val.
		q(nt(20), (t(6)+nt(16)));
		// _Rrow_6 => null.
		q(nt(21), (nul));
		// _Rrow_6 => _Rrow_5 _Rrow_6.
		q(nt(21), (nt(20)+nt(21)));
		// row => val _Rrow_6.
		q(nt(19), (nt(16)+nt(21)));
		// _Rrows_7 => eol row.
		q(nt(23), (nt(17)+nt(19)));
		// _Rrows_8 => null.
		q(nt(24), (nul));
		// _Rrows_8 => _Rrows_7 _Rrows_8.
		q(nt(24), (nt(23)+nt(24)));
		// rows => row _Rrows_8.
		q(nt(22), (nt(19)+nt(24)));
		// start => rows.
		q(nt(25), (nt(22)));
		return q;
	}
};
#endif // __CSV_PARSER_H__
