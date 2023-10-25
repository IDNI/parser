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

// CSV parser tutorial - part 6
//
// In this part we introduce parsing values separated by a comma.

#include <limits>

#include "parser.h"

using namespace std;
using namespace idni;

struct csv_parser {
	typedef variant<bool, int_t, string> value;
	// declare a row type as a vector of values
	typedef vector<value> row;
	csv_parser() :
		cc(predefined_char_classes({ "digit", "printable" }, nts)),
		start(nts("start")), digit(nts("digit")), digits(nts("digits")),
		integer(nts("integer")), printable(nts("printable")),
		stresc(nts("stresc")), strchar(nts("strchar")),
		strchars(nts("strchars")), str(nts("string")),
		val(nts("val")), nullvalue(nts("nullvalue")),
		// create new nonterminals we will use
		row_(nts("row")), row_rest(nts("row_rest")),
		g(nts, rules(), start, cc), p(g) {}
	// parse now returns a row instead of a value
	row parse(const char* data, size_t size,
		bool& parse_error, bool& out_of_range)
	{
		auto f = p.parse(data, size);
		parse_error = !p.found();
		if (parse_error) return {};
		// return parsed values as a row by calling get_row method
		return get_row(f.get(), out_of_range);
	}
	ostream& print_error(ostream& os) {
		return os << p.get_error().to_str() << '\n';
	}
private:
	nonterminals<> nts;
	char_class_fns<> cc;
	// add new nonterminals
	prods<> start, digit, digits, integer, printable,
		stresc, strchar, strchars, str,	val, nullvalue,
		row_, row_rest;
	grammar<> g;
	parser<> p;
	prods<> rules() {
		// define new comma terminal
		prods<> r, minus('-'), quote('"'), esc('\\'), comma(','),
			nul{ lit() };
		r(digits,     digit | (digits + digit));
		r(integer,    digits | (minus + digits));
		r(stresc,     esc + quote);
		r(strchar,    (printable & ~quote) | stresc);
		r(strchars,   strchar | (strchars + strchar));
		r(str,        (quote + strchars + quote) | (quote + quote));
		r(nullvalue,  nul);
		r(val,        integer | str | nullvalue);
		// define row as a value with the rest of row
		r(row_,       val + row_rest);
		// define rest of row as a comma + val and rowrest or null
		r(row_rest,   (comma + val + row_rest) | nul);
		// start nonterminal is a row
		r(start,      row_);
		return r;
	}
	// traverses the parsed forest and reads a parsed row of values from it.
	row get_row(typename parser<>::pforest* f, bool& out_of_range) {
		row r; // values we will return
		out_of_range = false; // out of integer range error
		// define a callback called when the traversal enters a node n
		auto cb_enter = [&r, &out_of_range, &f, this](const auto& n) {
			// n is a pair of a literal and its range
			// we can compare the literal with literals predefined
			// as members of 'this' object: integer/str/nullvalue
			if (n.first == integer)
				// push the integer into row of values
				r.push_back(
					terminals_to_int(*f, n, out_of_range));
			else if (n.first == str)
				// push the string into row of values
				r.push_back(terminals_to_str(*f, n));
			else if (n.first == nullvalue)
				r.push_back(true); // if null, push the bool
		};
		// run traversal with the enter callback
		f->traverse(cb_enter);
		return r; // return the value
	}
};

// separate printing of a value into a << operator
ostream& operator<<(ostream& os, const csv_parser::value& v) {
	if (holds_alternative<int_t>(v)) os << get<int_t>(v);
	else if (holds_alternative<bool>(v)) os << "NULL";
	else os << '"' << get<string>(v) << '"';
	return os;
}

int main() {
	cout << "Validator for rows of comma separated values of string, "
		<<"integer or NULL. Enter a value per line or Ctrl-D to quit\n";
	csv_parser p;
	string line;
	while (getline(cin, line)) {
		cout << "entered: \"" << line << "\"";
		bool parse_error, out_of_range;
		// instad of getting just an int we now get a value
		csv_parser::row r = p.parse(line.c_str(), line.size(),
						parse_error, out_of_range);
		if (parse_error) p.print_error(cerr);
		else if (out_of_range) cerr << " out of range, allowed range "
			"is from: " << numeric_limits<int_t>::min() <<
			" to: " << numeric_limits<int_t>::max() << '\n';
		else {
			cout << " parsed row: "; // print the parsed values
			for (size_t i = 0; i != r.size(); ++i) {
				if (i) cout << ", ";
				cout << r[i];
			}
			cout << '\n';
		}
	}
}
