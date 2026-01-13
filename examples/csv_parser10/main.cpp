// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// CSV parser tutorial - part 10
//
// In this part we use a tgf tool to generate a parser from a TGF file.
//
// This is usefull when you want to avoid parsing TGF to read your grammar every
// time you start your program. tgf tool converts TGF to a programatically
// created grammar and wraps it into a parser struct.
//
// We have moved the TGF grammar from previous part into a separate csv.tgf file
//
// Now we can run the tgf tool with "csv.tgf" filename as a first argument,
// a command "gen" as a second argument and we can use the --name option with
// argument "csv_parser" to name the struct.
// And we redirect the output to a desired header file:
//
//         tgf csv.tgf gen --name csv_parser > csv_parser.generated.h
//
// Generated parser has a similar API as library's parser struct.
// Additionally it provides size_t id(const basic_string<C>&) method to get id
// of a nonterminal.

#include <optional>
#include <limits>

// include the generated header file instead of using the generic parser
#include "csv_parser.generated.h"

#ifdef min
#	undef min
#endif
#ifdef max
#	undef max
#endif

using namespace std;
using namespace idni;

// Renamed csv_parser to csv_reader. csv_reader now uses csv_parser to parse
// input and it takes care of the traversal and extraction of values.
struct csv_reader {
	typedef variant<bool, int_t, string> value;
	typedef vector<value> row;
	typedef vector<row> rows;
	// use csv_parser from the header file with a generated parser
	csv_parser p;
	// get_rows now takes the string input and returns errors as refs
	optional<rows> get_rows(const string& input) {
		auto res = p.parse(input.c_str(), input.size());
		if (!res.found)	{
			cerr << res.parse_error << '\n';
			return {};
		}
		rows r;
		auto get_int = [&res](const auto& n) -> value {
			auto i = res.get_terminals_to_int(n);
			if (!i) return cerr
				<< "out of range, allowed range is from: "
				<< numeric_limits<int_t>::min() << " to: "
				<< numeric_limits<int_t>::max() << '\n', false;
			return i.value();
		};
		auto cb_enter = [&r, &get_int, &res](const auto& n) {
			if (!n->first.nt()) return;
			// generated csv_parser contains enum containing all
			// nonterminals from the grammar with their id values.
			// These can be used to compare with result of call
			// to literal's method n()
			auto id = n->first.n();
			if      (id == csv_parser::row) r.emplace_back();
			else if (id == csv_parser::integer)
				r.back().push_back(get_int(n));
			else if (id == csv_parser::str)
				r.back().push_back(res.get_terminals(n));
			else if (id == csv_parser::nullvalue)
				r.back().push_back(true);
		};
		res.get_forest()->traverse(cb_enter);
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
	cout << "entered: `" << input << "`\n";
	// call csv_reader's get_rows to parse the input and get rows w/ values
	optional<csv_reader::rows> rsopt = csv.get_rows(input);
	if (!rsopt) return 1;
	for (const csv_reader::row& r : rsopt.value()) {
		cout << "parsed row: ";
		for (size_t i = 0; i != r.size(); ++i)
			cout << (i ? ", " : "") << r[i];
		cout << '\n';
	}
	return 0;
}
