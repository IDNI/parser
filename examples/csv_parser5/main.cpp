// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// CSV parser tutorial - part 5
//
// In this part we enhance the parser for parsing also strings and null values.
// Strings are quoted by quotes ("") and escaped with backslash (\).
// Simple usage of a forest traversal is also showed to get value of its type.

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
	optional<value> parse(const char* data, size_t size) {
		auto res = p.parse(data, size);
		if (!res.found)	{
			cerr << res.parse_error << '\n';
			return {};
		}
		// return parsed value by calling get_value method
		// which traverses the forest and gets the value
		return get_value(res);
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
	value get_value(typename parser<>::result& res) {
		value v; // return value
		// takes node's terminals and converts them to int
		// if conversion fails, prints out of range and returns NULL
		auto get_int = [&res](const auto& n) -> value {
			auto i = res.get_terminals_to_int(n);
			if (!i) return cerr
				<< "out of range, allowed range is from: "
				<< numeric_limits<int_t>::min() << " to: "
				<< numeric_limits<int_t>::max() << '\n', false;
			return i.value();
		};
		// define a callback called when the traversal enters a node n
		auto cb_enter = [&v, &get_int, &res, this](const parser<>::pnode& n) {
			// n is a pair of a literal and its span
			// we can compare the literal with literals predefined
			// as members of 'this' object: integer/str/nullvalue
			// if integer, get int from node's terminals
			if (n.first == integer) v = get_int(n);
			// if string, get node's terminals as a string
			else if (n.first == str) v = res.get_terminals(n);
			// if null, v is bool
			else if (n.first == nullvalue) v = true;
		};
		// run traversal with the enter callback
		res.get_forest()->traverse(cb_enter);
		return v; // return the value
	}
};

int main() {
	cout << "Validator for values of string, integer or NULL. "
		<< "Enter a value per line or Ctrl-D to quit\n";
	csv_parser p;
	string line;
	while (getline(cin, line)) {
		cout << "entered: `" << line << "`\n";
		// instead of getting just an int we now get a value
		optional<csv_parser::value> vopt =
					p.parse(line.c_str(), line.size());
		if (!vopt) continue;
		auto& v = vopt.value();
		cout << "parsed value: ";
		// since the value is a variant we can use
		// holds_alternative<T> and get<T> to access the data
		if (holds_alternative<int_t>(v)) cout << get<int_t>(v);
		else if (holds_alternative<bool>(v)) cout << "NULL";
		else cout << get<string>(v);
		cout << '\n';
	}
}
