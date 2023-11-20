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

// CSV parser tutorial - part 5
//
// In this part we enhance the parser for parsing also strings and null values.
// Strings are quoted by quotes ("") and escaped with backslash (\).
// Simple usage of a forest traversal is also showed to get value of its type.

#include <limits>

#include "parser.h"

using namespace std;
using namespace idni;

struct csv_parser {
	// declare a value variant type which can contain bool (for null),
	// integer or string
	typedef variant<bool, int_t, string> value;
	csv_parser() :
		// add predefined character class function for printables
		cc(predefined_char_classes({ "digit", "printable" }, nts)),
		start(nts("start")), digit(nts("digit")), digits(nts("digits")),
		// create new nonterminals we will use
		integer(nts("integer")), printable(nts("printable")),
		val(nts("val")), nullvalue(nts("nullvalue")),
		escaping(nts("escaping")), escaped(nts("escaped")),
		unescaped(nts("unescaped")), strchar(nts("strchar")),
		strchars(nts("strchars")), str(nts("string")),
		g(nts, rules(), start, cc), p(g) {}
	value parse(const char* data, size_t size,
		bool& parse_error, bool& out_of_range)
	{
		auto f = p.parse(data, size);
		parse_error = !p.found();
		if (parse_error) return ""; // if parse_error return ""
		// return parsed value by calling get_value method
		// which traverses the forest and gets the value
		return get_value(f.get(), out_of_range);
	}
	ostream& print_error(ostream& os) {
		return os << p.get_error().to_str() << '\n';
	}
private:
	nonterminals<> nts{};
	char_class_fns<> cc;
	// add new nonterminals
	prods<> start, digit, digits, integer, printable, val, nullvalue,
		escaping, escaped, unescaped, strchar, strchars, str;
	grammar<> g;
	parser<> p;
	prods<> rules() {
		// define new quote and esc terminals
		prods<> r, minus('-'), quote('"'), esc('\\'),
			nul{ lit() }; // nul(l) literal
		r(digits,     digit | (digits + digit));
		// we rename the start in the previous part to integer
		r(integer,    digits | (minus + digits));
		// we are escaping quote (") or esc (\)
		r(escaping,   quote | esc);
		// unescaped are all printable but not those we are escaping
		r(unescaped,  printable & ~escaping);
		// escaped is escape and what we are escaping
		r(escaped,    esc + escaping);
		// string char is unescaped or escaped
		r(strchar,    unescaped | escaped);
		// strchars is a sequence of string characters or nothing
		r(strchars,   (strchar + strchars) | nul);
		// string is string characters in quotes but can be empty
		r(str,        quote + strchars + quote);
		// null value matches if we match a null literal
		r(nullvalue,  nul);
		// value can be either integer, string (if not integer) or null
		r(val,        integer | str | nullvalue);
		// start can be value
		r(start,      val);
		return r;
	}

	// traverses the parsed forest and reads a parsed value from it.
	value get_value(typename parser<>::pforest* f, bool& out_of_range) {
		value v; // value we will return
		out_of_range = false; // out of integer range error
		// define a callback called when the traversal enters a node n
		auto cb_enter = [&v, &out_of_range, &f, this](const auto& n) {
			// n is a pair of a literal and its range
			// we can compare the literal with literals predefined
			// as members of 'this' object: integer/str/nullvalue
			if (n.first == integer)
				// if integer, flatten the node into an int
				v = terminals_to_int(*f, n, out_of_range);
			else if (n.first == str)
				// if string, flatten the node into a string
				v = terminals_to_str(*f, n);
			else if (n.first == nullvalue)
				v = true; // if null, v is bool
		};
		// run traversal with the enter callback
		f->traverse(cb_enter);
		return v; // return the value
	}
};

int main() {
	cout << "Validator for values of string, integer or NULL. "
		<< "Enter a value per line or Ctrl-D to quit\n";
	csv_parser p;
	string line;
	while (getline(cin, line)) {
		cout << "entered: `" << line << "`";
		bool parse_error, out_of_range;
		// instad of getting just an int we now get a value
		csv_parser::value v = p.parse(line.c_str(), line.size(),
						parse_error, out_of_range);
		if (parse_error) p.print_error(cerr);
		else if (out_of_range) cerr << " out of range, allowed range "
			"is from: " << numeric_limits<int_t>::min() <<
			" to: " << numeric_limits<int_t>::max() << '\n';
		else {
			cout << " parsed value: ";
			// since the value is a variant we can use
			// holds_alternative<T> and get<T> to access the data
			if (holds_alternative<int_t>(v)) cout << get<int_t>(v);
			else if (holds_alternative<bool>(v)) cout << "NULL";
			else cout << get<string>(v);
			cout << '\n';
		}
	}
}
