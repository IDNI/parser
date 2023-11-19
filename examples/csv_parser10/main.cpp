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

// CSV parser tutorial - part 10
//
// In this part we use a parser gen tool to generate a parser from a TGF file.
//
// This is usefull when you want to avoid parsing TGF to read your grammar every
// time you start your program. parser_gen converts TGF to a programatically
// created grammar and wraps it into a parser struct.
//
// We have moved the TGF grammar from previous part into a separate csv.tgf file
//
// Now we can run the parser_gen tool with argument "csv_parser" to name the
// struct and "csv.tgf" as a name of the tgf file.
// And we redirect the output to a desired header file:
//
//         parser_gen csv_parser csv.tgf > csv_parser.generated.h
//
// Generated parser has a simillar API as the generic parser.
// Additionally it provides size_t id(const basic_string<C>&) method to get id
// of a nonterminal.

#include <limits>

// include the generated header file instead of the generic parser
#include "csv_parser.generated.h"

using namespace std;
using namespace idni;

// Renamed csv_parser to csv_reader. csv_reader now uses csv_parser to parse
// input and takes still it care of the traversal and extraction of the values.
struct csv_reader {
	typedef variant<bool, int_t, string> value;
	typedef vector<value> row;
	typedef vector<row> rows;
	// use csv_parser from the generated header file
	csv_parser p;
	ostream& print_error(ostream& os) {
		return os << p.get_error().to_str() << '\n';
	}
	// get_rows now takes the string input and returns errors as refs
	rows get_rows(string input, bool& parse_error, bool& out_of_range) {
		// run the parser
		auto f = p.parse(input.c_str(), input.size());
		// checks for the error
		if ((parse_error = !p.found()) || f == 0) return {};
		// traverse
		rows r;
		out_of_range = false;
		auto cb_enter = [&r, &out_of_range, &f, this](const auto& n) {
			if (!n.first.nt()) return;
			// Since the nonterminals container is encapsulated in
			// a generated parser, struct provides an id(string)
			// method to get id of a nonterminal.
			// We can then compare id's to nt id in n.firs.n()
			auto id = n.first.n(); // shortcut to the id;
			if      (id == p.id("row")) r.emplace_back();
			else if (id == p.id("integer"))
				r.back().push_back(
					terminals_to_int(*f, n, out_of_range));
			else if (id == p.id("str"))
				r.back().push_back(terminals_to_str(*f, n));
			else if (id == p.id("nullvalue"))
				r.back().push_back(true);
		};
		f->traverse(cb_enter);
		return r;
	}
};

ostream& operator<<(ostream& os, const csv_reader::value& v) {
	if (holds_alternative<int_t>(v)) os << get<int_t>(v);
	else if (holds_alternative<bool>(v)) os << "NULL";
	else os << get<string>(v);
	return os;
}

int main() {
	cout << "CSV parser. Use comma separated values of string, "
		<< "integer or NULL. Use a new line to separate rows. "
		<< "Enter a CSV and use Ctrl-D to start parsing\n";
	// use csv_reader
	csv_reader csv;
	istreambuf_iterator<char> begin(cin), end;
	string input(begin, end);
	cout << "entered: `" << input << "`";
	bool parse_error, out_of_range;
	// call csv_reader's get_rows to parse the input and get rows w/ values
	csv_reader::rows rs = csv.get_rows(input, parse_error, out_of_range);
	if (parse_error) csv.print_error(cerr);
	else if (out_of_range) cerr << " out of range, allowed range "
		"is from: " << numeric_limits<int_t>::min() <<
		" to: " << numeric_limits<int_t>::max() << '\n';
	else {
		for (const csv_reader::row& r : rs) {
			cout << " parsed row: ";
			for (size_t i = 0; i != r.size(); ++i) {
				if (i) cout << ", ";
				cout << r[i];
			}
			cout << '\n';
		}
	}
}
