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

// CSV parser tutorial - part 1
//
// In this part we create a very simple parser validating just positive ints.
// Comments are related only to a new or modified code in the part.

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

int main() {
	cout << "Validator for positive integers. "
		"Enter a positive integer per line or Ctrl-D to quit\n";
	// create a map for nonterminal names
	nonterminals nts;
	// to define our grammar programatically we need to
	// - define ps as a grammar productions container
	// - define nonterminals start and digit
	prods ps, start(nts("start")), digit(nts("digit"));
	// - create production rules
	// digit => '0'|'1'|'2'|'3'|'4'|'5'|'6'|'7'|'8'|'9'.
	// note 0 terminal in the following body created as prods for operator|
	ps(digit, prods<>('0')|'1'|'2'|'3'|'4'|'5'|'6'|'7'|'8'|'9');
	// start => digit | start + digit.
	ps(start, digit | (digit + start));
	// create a grammar object from programatically created rules (in ps)
	grammar g(nts, ps, start, {});
	// instantiate a parser object by providing the grammar
	parser p(g);
	// read line by line from standard input until EOF
	string line;
	while (getline(cin, line)) { // and for each entered line do
		cout << "entered: `" << line << "`\n"; // print entered input
		auto res = p.parse(line.c_str(), line.size()); // run parser
		if (!res.found) {
			// print parse error if the parsing was unsuccessful
			cerr << res.parse_error << '\n';
			continue;
		}
		// flatten all parsed terminals and convert them to int
		optional<int_t> i = res.get_terminals_to_int(
						res.get_forest()->root());
		// i has no value if conversion failed, ie. out of range
		if (!i) cerr << "out of range, allowed range is from: 0 to: "
				<< numeric_limits<int_t>::max() << '\n';
		else cout << "parsed integer: " << i.value() << '\n';
	}
}
