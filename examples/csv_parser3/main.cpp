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

// CSV parser tutorial - part 3
//
// In this part we refactor all the parser related code to a struct.

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

// struct csv_parser wrapping the parser
struct csv_parser {
	// constructor takes care of initialization of all its members
	// note the order: cc and nonterminals have to be initializated first
	// then grammar g is created with rules() returning its production rules
	// then parser p is initializated
	csv_parser() :
		cc(predefined_char_classes({ "digit" }, nts)),
		start(nts("start")), digit(nts("digit")),
		g(nts, rules(), start, cc), p(g) {}
	// parse method takes input, runs parser and transforms parsed terminals
	// into int. if parse fails prints parse error.
	// if tranformation into int failse writes out of range error.
	// returns optional int which has no value if parse fails or out of range
	optional<int_t> parse(const char* data, size_t size) {
		auto res = p.parse(data, size);
		optional<int_t> i{};
		if (!res.found)	return cerr << res.parse_error << '\n', i;
		i = res.get_terminals_to_int(res.get_forest()->root());
		if (!i) return cerr
			<< "out of range, allowed range is from: 0 to: "
			<< numeric_limits<int_t>::max() << '\n', i;
		return i;
	}
private:
	// all parsing related variables as members
	nonterminals<> nts;
	char_class_fns<> cc;
	prods<> start, digit;
	grammar<> g;
	parser<> p;
	// method which defines programatically created rules
	// it is used when the grammar is initialized in the constructor
	prods<> rules() {
		prods r;
		r(start, digit | (digit + start));
		return r;
	}
};

int main() {
	cout << "Validator for positive integers. "
		<< "Enter a positive integer per line or Ctrl-D to quit\n";
	csv_parser p;
	string line;
	while (getline(cin, line)) {
		cout << "entered: `" << line << "`\n";
		auto i = p.parse(line.c_str(), line.size());
		if (i)	cout << "parsed integer: " << i.value() << '\n';
	}
}
