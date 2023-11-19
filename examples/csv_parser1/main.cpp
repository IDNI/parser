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

#include <limits>

#include "parser.h"

using namespace std;
using namespace idni;

int main() {
	cout << "Validator for positive integers. "
		<< "Enter a positive integer per line or Ctrl-D to quit\n";
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
		cout << "entered: `" << line << "`"; // print entered input
		auto f = p.parse(line.c_str(), line.size()); // run parse
		// if the parsing was successful
		if (p.found()) {
			// flatten all parsed terminals and convert them to int
			bool err;
			int_t i = terminals_to_int(*f, f->root(), err);
			// err is true if conversion failed, ie. out of range
			if (!err) cout << " parsed integer: " << i << '\n';
			else cerr << " out of range, max value is: "
				<< numeric_limits<int_t>::max() << '\n';
		}
		// or print a parse error to error output
		else cerr << p.get_error().to_str() << '\n';
	}
}
