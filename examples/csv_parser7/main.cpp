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
	optional<rows> parse(const char* data, size_t size) {
		auto res = p.parse(data, size);
		if (!res.found)	{
			cerr << res.parse_error << '\n';
			return {};
		}
		// return parsed CSV rows
		return get_rows(res);
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
	rows get_rows(typename parser<>::result& res) {
		rows r; // rows of values we will return
		auto get_int = [&res](const auto& n) -> value {
			auto i = res.get_terminals_to_int(n);
			if (!i) return cerr
				<< "out of range, allowed range is from: "
				<< numeric_limits<int_t>::min() << " to: "
				<< numeric_limits<int_t>::max() << '\n', false;
			return i.value();
		};
		auto cb_enter = [&r, &get_int, &res, this](const auto& n) {
			// for every new row we add a new row into r
			if (n.first == row_) r.emplace_back();
			else if (n.first == integer)
				// and we use r.back() to ref the current row
				r.back().push_back(get_int(n));
			else if (n.first == str)
				r.back().push_back(res.get_terminals(n));
			else if (n.first == nullvalue)
				r.back().push_back(true);
		};
		res.get_forest()->traverse(cb_enter);
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
	cout << "entered: `" << input << "`\n";
	// instead of getting just a row we now get rows
	optional<csv_parser::rows> rsopt = p.parse(input.c_str(), input.size());
	if (!rsopt) return 1;
	// print the parsed rows
	for (const csv_parser::row& r : rsopt.value()) {
		cout << "parsed row: ";
		for (size_t i = 0; i != r.size(); ++i)
			cout << (i ? ", " : "") << r[i];
		cout << '\n';
	}
	return 0;
}
