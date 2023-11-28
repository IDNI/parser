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

// CSV parser tutorial - part 7
//
// In this part we introduce parsing the CSV rows separated by an end of line.

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
	typedef variant<bool, int_t, string> value;
	typedef vector<value> row;
	// declare a rows type as a vector of rows
	typedef vector<row> rows;
	csv_parser() :
		cc(predefined_char_classes({ "digit", "printable" }, nts)),
		start(nts("start")), digit(nts("digit")), digits(nts("digits")),
		integer(nts("integer")), printable(nts("printable")),
		val(nts("val")), nullvalue(nts("nullvalue")),
		escaping(nts("escaping")), escaped(nts("escaped")),
		unescaped(nts("unescaped")), strchar(nts("strchar")),
		strchars(nts("strchars")), str(nts("string")),
		row_(nts("row")), row_rest(nts("row_rest")),
		// create new nonterminals we will use
		eol(nts("eol")),
		rows_(nts("rows")), rows_rest(nts("rows_rest")),
		g(nts, rules(), start, cc), p(g) {}
	// parse now returns rows instead of a row
	rows parse(const char* data, size_t size,
		bool& parse_error, bool& out_of_range)
	{
		auto f = p.parse(data, size);
		parse_error = !p.found();
		if (parse_error) return {};
		// return parsed CSV rows
		return get_rows(f.get(), out_of_range);
	}
	ostream& print_error(ostream& os) {
		return os << p.get_error().to_str() << '\n';
	}
private:
	nonterminals<> nts{};
	char_class_fns<> cc;
	// add new nonterminals
	prods<> start, digit, digits, integer, printable, val, nullvalue,
		escaping, escaped, unescaped, strchar, strchars, str,
		row_, row_rest, eol, rows_, rows_rest;
	grammar<> g;
	parser<> p;
	prods<> rules() {
		// define carriage return and new line terminals
		prods<> r, minus('-'), quote('"'), esc('\\'), comma(','),
			cr('\r'), nl('\n'), nul{ lit() };
		r(digits,     digit | (digits + digit));
		r(integer,    digits | (minus + digits));
		r(escaping,   quote | esc);
		r(unescaped,  printable & ~escaping);
		r(escaped,    esc + escaping);
		r(strchar,    unescaped | escaped);
		r(strchars,   (strchar + strchars) | nul);
		r(str,        quote + strchars + quote);
		r(nullvalue,  nul);
		r(val,        integer | str | nullvalue);
		r(row_,       val + row_rest);
		r(row_rest,   (comma + val + row_rest) | nul);
		// end of line is a new line or carriage return with a new line
		r(eol,        nl | (cr + nl));
		// define rows as row and rest of rows
		r(rows_,      row_ + rows_rest);
		// rest of rows is null or end of line followed by a row
		// followed by another rest of rows
		r(rows_rest,  (eol + row_ + rows_rest) | nul);
		// start nonterminal is rows
		r(start,      rows_);
		return r;
	}
	rows get_rows(typename parser<>::pforest* f, bool& out_of_range) {
		rows r; // rows of values we will return
		out_of_range = false;
		auto cb_enter = [&r, &out_of_range, &f, this](const auto& n) {
			// for every new row we add a new row into r
			if (n.first == row_) r.emplace_back();
			else if (n.first == integer)
				// and we use r.back() to ref the current row
				r.back().push_back(
					terminals_to_int(*f, n, out_of_range));
			else if (n.first == str)
				r.back().push_back(terminals_to_str(*f, n));
			else if (n.first == nullvalue)
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
	// instead of reading line by line, read the whole standard input
	istreambuf_iterator<char> begin(cin), end;
	string input(begin, end);
	cout << "entered: `" << input << "`";
	bool parse_error, out_of_range;
	// instad of getting just a row we now get all the rows
	csv_parser::rows rs = p.parse(input.c_str(), input.size(),
					parse_error, out_of_range);
	if (parse_error) p.print_error(cerr);
	else if (out_of_range) cerr << " out of range, allowed range "
		"is from: " << numeric_limits<int_t>::min() <<
		" to: " << numeric_limits<int_t>::max() << '\n';
	else {
		// print the parsed rows
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
