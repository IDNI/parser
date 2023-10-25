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

// CSV parser tutorial - part 8
//
// In this part we replace programatically defined grammar by a grammar in TGF.

#include <limits>

#include "parser.h"

using namespace std;
using namespace idni;

struct csv_parser {
	// CSV grammar as a TGF string
	// TGF concatenates literals separated by a whitespace
	// contrary to C++ where operator+ is required to concatenate literals
	const char* csv_tgf =
	"	@use_char_class digit, printable. "
	// new quote rule to name the literal and use it in other rules by name
	"	quote      => '\"'. "
	"	digits     => digit | (digits digit). "
	"	integer    => digits | ('-' digits). "
	"	stresc     => '\\' quote. "
	"	strchar    => (printable & ~quote) | stresc. "
	"	strchars   => strchar | (strchars strchar). "
	"	str        => (quote strchars quote) | (quote quote). "
	// null is a reserved word representing a null literal
	"	nullvalue  => null. "
	"	val        => integer | str | nullvalue. "
	// TGF names do not conflict with C++ names so it is safe
	//  to remove _ from row_ and rows_
	"	row        => val row_rest. "
	"	row_rest   => (',' val row_rest) | null. "
	"	eol        => '\n' | \"\r\n\"). "
	"	rows       => row rows_rest. "
	"	rows_rest  => (eol row rows_rest) | null. "
	"	start      => rows. "
	;
	typedef variant<bool, int_t, string> value;
	typedef vector<value> row;
	typedef vector<row> rows;
	// when initializing the CSV parser read the grammar from the TGF string
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
		return os << p.get_error().to_str() << '\n';
	}
private:
	nonterminals<> nts;
	grammar<> g;
	parser<> p;
	// container for char class functions and nonterminals and rules method
	// removed since it is covered by TGF
	rows get_rows(typename parser<>::pforest* f, bool& out_of_range) {
		rows r;
		out_of_range = false;
		auto cb_enter = [&r, &out_of_range, &f, this](const auto& n) {
			// we care only for nonterminals, so skip terminals
			if (!n.first.nt()) return;
			// get name of the nonterminal
			string nt = nts.get(n.first.n());
			// decide what to do according to the name
			// we can remove _ in row_ because no conflict with row
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
	else os << '"' << get<string>(v) << '"';
	return os;
}

int main() {
	cout << "CSV parser. Use comma separated values of string, "
		<< "integer or NULL. Use a new line to separate rows. "
		<< "Enter a CSV and use Ctrl-D to start parsing\n";
	csv_parser p;
	istreambuf_iterator<char> begin(cin), end;
	string input(begin, end);
	cout << "entered: \"" << input << "\"";
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
