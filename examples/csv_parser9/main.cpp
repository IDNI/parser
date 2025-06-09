// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.txt

// CSV parser tutorial - part 9
//
// In this part we simplify the grammar in TGF by using features from EBNF.

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
	const char* csv_tgf =
	"	@use char classes digit, printable. "
	"	esc        => '\\\\'. "
	"	quote      => '\"'. "
	// replaced digits with digit+ for one or more occurances
	// and '-' is optional by wrapping into [ ]
	"	integer    => ['-'] digit+. "
	"	escaping   => quote | esc. "
	"	unescaped  => printable & ~escaping. "
	"	escaped    => esc escaping. "
	"	strchar    => unescaped | escaped. "
	// replaced strchars with strchar* for 0 or any number of occurances
	"	str        => quote strchar* quote. "
	"	nullvalue  => null. "
	"	val        => integer | str | nullvalue. "
	// replaced row_rest with ( ',' val )* for 0-any occurances
	"	row        => val ( ',' val )*. "
	"	eol        => [ '\\r' ] '\\n'. "
	// replaced rows_rest with ( eol row )* for 0-any occurances
	"	rows       => row ( eol row )*. "
	"	start      => rows. "
	;
	typedef variant<bool, int_t, string> value;
	typedef vector<value> row;
	typedef vector<row> rows;
	csv_parser() : g(tgf<>::from_string(nts, csv_tgf)), p(g) {}
	rows parse(const char* data, size_t size) {
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
		auto cb_enter = [&r, &get_int, &res, this](const auto& n) {
			if (!n->first.nt()) return;
			string nt = nts.get(n->first.n());
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
	else os << get<string>(v);
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
