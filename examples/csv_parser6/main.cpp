// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// CSV parser tutorial - part 6
//
// In this part we introduce parsing values separated by a comma.

#include <optional>
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
	// declare a row type as a vector of values
	typedef vector<value> row;
	csv_parser() :
		cc(predefined_char_classes({ "digit", "printable" }, nts)),
		start(nts("start")), digit(nts("digit")), digits(nts("digits")),
		integer(nts("integer")), printable(nts("printable")),
		val(nts("val")), nullvalue(nts("nullvalue")),
		escaping(nts("escaping")), escaped(nts("escaped")),
		unescaped(nts("unescaped")), strchar(nts("strchar")),
		strchars(nts("strchars")), str(nts("string")),
		// create new nonterminals we will use
		row_(nts("row")), row_rest(nts("row_rest")),
		g(nts, rules(), start, cc), p(g) {}
	// parse now returns a row instead of a value
	optional<row> parse(const char* data, size_t size) {
		auto res = p.parse(data, size);
		if (!res.found)	{
			cerr << res.parse_error << '\n';
			return {};
		}
		// return parsed values as a row by calling get_row method
		return get_row(res);
	}
private:
	nonterminals<> nts;
	char_class_fns<> cc;
	// add new nonterminals
	prods<> start, digit, digits, integer, printable, val, nullvalue,
		escaping, escaped, unescaped, strchar, strchars, str,
		row_, row_rest;
	grammar<> g;
	parser<> p;
	prods<> rules() {
		// define new comma terminal
		prods<> r, minus('-'), quote('"'), esc('\\'), comma(','),
			nul{ lit() };
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
		// define row as a value with the rest of row
		r(row_,       val + row_rest);
		// define rest of row as a comma + val and rowrest or null
		r(row_rest,   (comma + val + row_rest) | nul);
		// start nonterminal is a row
		r(start,      row_);
		return r;
	}
	// traverses the parsed forest and reads a parsed row of values from it.
	row get_row(typename parser<>::result& res) {
		row r; // values we will return
		auto get_int = [&res](const auto& n) -> value {
			auto i = res.get_terminals_to_int(n);
			if (!i) return cerr
				<< "out of range, allowed range is from: "
				<< numeric_limits<int_t>::min() << " to: "
				<< numeric_limits<int_t>::max() << '\n', false;
			return i.value();
		};
		auto cb_enter = [&r, &get_int, &res, this](const parser<>::pnode& n) {
			// n is a pair of a literal and its range
			// we can compare the literal with literals predefined
			// as members of 'this' object: integer/str/nullvalue
			// if integer push the integer into row of values
			if (n.first == integer) r.push_back(get_int(n));
			// if str then push the string into row of values
			else if (n.first == str)
					r.emplace_back(res.get_terminals(n));
			// if null, push the bool
			else if (n.first == nullvalue) r.emplace_back(true);
		};
		// run traversal with the enter callback
		res.get_forest()->traverse(cb_enter);
		return r; // return the value
	}
};

// separate printing of a value into a << operator
ostream& operator<<(ostream& os, const csv_parser::value& v) {
	if (holds_alternative<int_t>(v)) os << get<int_t>(v);
	else if (holds_alternative<bool>(v)) os << "NULL";
	else os << get<string>(v);
	return os;
}

int main() {
	cout << "Validator for rows of comma separated values of string, "
		<<"integer or NULL. Enter a value per line or Ctrl-D to quit\n";
	csv_parser p;
	string line;
	while (getline(cin, line)) {
		cout << "entered: `" << line << "`\n";
		// instead of getting just a value we now get a row of values
		optional<csv_parser::row> ropt =
					p.parse(line.c_str(), line.size());
		if (!ropt) continue;
		auto& r = ropt.value();
		cout << "parsed row: "; // print parsed values
		for (size_t i = 0; i != r.size(); ++i)
			cout << (i ? ", " : "") << r[i];
		cout << '\n';
	}
}
