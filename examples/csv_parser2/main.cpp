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

// CSV parser tutorial - part 2
//
// In this part we show using a predefined character class function for digits.

#include <limits>

#include "parser.h"

using namespace std;
using namespace idni;

int main() {
	cout << "Validator for positive integers. "
		<< "Enter a positive integer per line or Ctrl-D to quit\n";
	nonterminals nts;
	prods ps, start(nts("start")), digit(nts("digit"));
	// get a container with a predefined digit function
	char_class_fns cc = predefined_char_classes({ "digit" }, nts);
	ps(start, digit | (digit + start));
	// when creating a grammar object from programatically created rules
	// add also the digit function (contained in cc)
	grammar g(nts, ps, start, cc);
	parser p(g);
	string line;
	while (getline(cin, line)) {
		cout << "entered: `" << line << "`";
		auto f = p.parse(line.c_str(), line.size());
		if (p.found()) {
			bool err;
			int_t i = terminals_to_int(*f, f->root(), err);
			if (!err) cout << " parsed integer: " << i << '\n';
			else cerr << " out of range, max value is: "
				<< numeric_limits<int_t>::max() << '\n';
		}
		else cerr << p.get_error().to_str() << '\n';
	}
}
