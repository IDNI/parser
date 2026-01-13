// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// CSV parser tutorial - part 8
//
// In this part we replace programatically defined grammar by a grammar in TGF.

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
	// CSV grammar as a TGF string
	// TGF concatenates literals separated by a whitespace
	// contrary to C++ where operator+ is required to concatenate literals
	const char* csv_tgf =
	"	@use char classes digit, printable. "
	// quote and esc to name the literal and use it in other rules by name
	"	esc        => '\\\\'. "
	"	quote      => '\"'. "
	"	digits     => digit | digits digit. "
	"	integer    => digits | '-' digits. "
	"	escaping   => quote | esc. "
	"	unescaped  => printable & ~escaping. "
	"	escaped    => esc escaping. "
	"	strchar    => unescaped | escaped. "
	"	strchars   => strchar strchars | null. "
	"	str        => quote strchars quote. "
	// null is a reserved word representing a null literal
	"	nullvalue  => null. "
	"	val        => integer | str | nullvalue. "
	// TGF names do not conflict with C++ names so it is safe
	//  to remove _ from row_ and rows_
	"	row        => val row_rest. "
	"	row_rest   => ',' val row_rest | null. "
	"	eol        => '\\n' | \"\\r\\n\". "
	"	rows       => row rows_rest. "
	"	rows_rest  => eol row rows_rest | null. "
	"	start      => rows. "
	;
	typedef variant<bool, int_t, string> value;
	typedef vector<value> row;
	typedef vector<row> rows;
	// when initializing the CSV parser read the grammar from the TGF string
	csv_parser() : g(tgf<>::from_string(nts, csv_tgf)), p(g) {}
	optional<rows> parse(const char* data, size_t size) {
		auto res = p.parse(data, size);
		if (!res.found)	{
			cerr << res.parse_error << '\n';
			return {};
		}
		return get_rows(res);
	}
private:
	nonterminals<> nts;
	grammar<> g;
	parser<> p;
	// container for char class functions and all prods removed since it is
	// covered by TGF
	rows get_rows(typename parser<>::result& res) {
		rows r;
		auto get_int = [&res](const auto& n) -> value {
			auto i = res.get_terminals_to_int(n);
			if (!i) return cerr
				<< "out of range, allowed range is from: "
				<< numeric_limits<int_t>::min() << " to: "
				<< numeric_limits<int_t>::max() << '\n', false;
			return i.value();
		};
		auto cb_enter = [&r, &get_int, &res, this](const parser<>::pnode& n) {
			// we care only for nonterminals, so skip terminals
			if (!n.first.nt()) return;
			// get name of the nonterminal
			string nt = nts.get(n.first.n());
			// decide what to do according to the name
			// we can remove _ in row_ because no conflict with row
			if (nt == "row") r.emplace_back();
			else if (nt == "integer")
				r.back().push_back(get_int(n));
			else if (nt == "str")
				r.back().push_back(res.get_terminals(n));
			else if (nt == "nullvalue")
				r.back().push_back(true);
		};
		res.get_forest()->traverse(cb_enter);
		return r;
	}
};

ostream& operator<<(ostream& os, const csv_parser::value& v) {
	if (holds_alternative<int_t>(v)) os << get<int_t>(v);
	else if (holds_alternative<bool>(v)) os << "NULL";
	else os <<get<string>(v);
	return os;
}

int main() {
	cout << "CSV parser. Use comma separated values of string, "
		<< "integer or NULL. Use a new line to separate rows. "
		<< "Enter a CSV and use Ctrl-D to start parsing\n";
	csv_parser p;
	istreambuf_iterator<char> begin(cin), end;
	string input(begin, end);
	cout << "entered: `" << input << "`\n";
	optional<csv_parser::rows> rsopt = p.parse(input.c_str(), input.size());
	if (!rsopt) return 1;
	for (const csv_parser::row& r : rsopt.value()) {
		cout << "parsed row: ";
		for (size_t i = 0; i != r.size(); ++i)
			cout << (i ? ", " : "") << r[i];
		cout << '\n';
	}
	return 0;
}
