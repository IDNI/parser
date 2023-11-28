// LICENSE
// This software is free for use and redistribution while including this
// license notice, unless:
// 1. is used for commercial or non-personal purposes, or
// 2. used for a product which includes or associated with a blockchain or other
// decentralized database technology, or
// 3. used for a product which includes or associated with the issuance or use
// of cryptographic or electronic currencies/coins/tokens.
// On all of the mentioned cases, an explicit and written permission is required
// from the Author (Ohad Asor).
// Contact ohad@idni.org for requesting a permission. This license may be
// modified over time by the Author.

// CSV parser tutorial - part 9
//
// In this part we simplify the grammar in TGF by using features from EBNF.

#include <limits>

#include "parser.h"

#ifdef min
#	undef min
#endif
#ifdef max
#	undef max
#endif

using namespace std;
using namespace idni;

struct csv_parser {
	const char* csv_tgf =
	"	@use_char_class digit, printable. "
	"	esc        => '\\'. "
	"	quote      => '\"'. "
	// replaced digits with digit+ for one or more occurances
	"	integer    => ['-'] digit+. "
	"	escaping   => quote | esc. "
	"	unescaped  => printable & ~escaping. "
	"	escaped    => esc escaping. "
	"	strchar    => unescaped | escaped. "
	// replaced strchars with (strchar*) for 0 or any number of occurances
	"	str        => quote (strchar*) quote. "
	"	nullvalue  => null. "
	"	val        => integer | str | nullvalue. "
	// replaced row_rest with ( ',' val )* for 0-any occurances
	"	row        => val ( ',' val )*. "
	"	eol        => [ '\r' ] '\n'. "
	// replaced rows_rest with ( eol row )* for 0-any occurances
	"	rows       => row ( eol row )*. "
	"	start      => rows. "
	;
	typedef variant<bool, int_t, string> value;
	typedef vector<value> row;
	typedef vector<row> rows;
	csv_parser() : g(tgf<>::from_string(nts, csv_tgf)), p(g) {}
	rows parse(const char* data, size_t size,
		bool& parse_error, bool& out_of_range)
	{
		auto f = p.parse(data, size);
		parse_error = !p.found();
		if (parse_error) return {};
		return get_rows(f.get(), out_of_range);
	}
	ostream& print_error(ostream& os) {
		using lvl = decltype(p)::error::info_lvl;
		return os << p.get_error().to_str(lvl::INFO_ROOT_CAUSE) << '\n';
	}
private:
	nonterminals<> nts;
	grammar<> g;
	parser<> p;
	rows get_rows(typename parser<>::pforest* f, bool& out_of_range) {
		rows r;
		out_of_range = false;
		auto cb_enter = [&r, &out_of_range, &f, this](const auto& n) {
			if (!n.first.nt()) return;
			string nt = nts.get(n.first.n());
			if (nt == "row") r.emplace_back();
			else if (nt == "integer")
				r.back().push_back(
					terminals_to_int(*f, n, out_of_range));
			else if (nt == "str")
				r.back().push_back(terminals_to_str(*f, n));
			else if (nt == "nullvalue")
				r.back().push_back(true);
		};
		f->traverse(cb_enter);
		return r;
	}
};

ostream& operator<<(ostream& os, const csv_parser::value& v) {
	if (holds_alternative<int_t>(v)) os << get<int_t>(v);
	else if (holds_alternative<bool>(v)) os << "NULL";
	else os << get<string>(v);
	return os;
}

int main() {
	cout << "CSV parser. Use comma separated values of string, "
		<< "integer or NULL. Use a new line to separate rows. "
		<< "Enter a CSV and use Ctrl-D to start parsing\n";
	csv_parser p;
	istreambuf_iterator<char> begin(cin), end;
	string input(begin, end);
	cout << "entered: `" << input << "`";
	bool parse_error, out_of_range;
	csv_parser::rows rs = p.parse(input.c_str(), input.size(),
					parse_error, out_of_range);
	if (parse_error) p.print_error(cerr);
	else if (out_of_range) cerr << " out of range, allowed range "
		"is from: " << numeric_limits<int_t>::min() <<
		" to: " << numeric_limits<int_t>::max() << '\n';
	else {
		for (const csv_parser::row& r : rs) {
			cout << " parsed row: ";
			for (size_t i = 0; i != r.size(); ++i) {
				if (i) cout << ", ";
				cout << r[i];
			}
			cout << '\n';
		}
	}
}
