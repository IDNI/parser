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

// CSV parser tutorial - part 4
//
// In this part we enhance the parser for parsing also negative integers.

#include <limits>

#include "parser.h"

using namespace std;
using namespace idni;

struct csv_parser {
	csv_parser() :
		cc(predefined_char_classes({ "digit" }, nts)),
		start(nts("start")), digit(nts("digit")),
		// add digits nonterminal which represents a sequence of digits
		digits(nts("digits")),
		g(nts, rules(), start, cc), p(g) {}
	int_t parse(const char* data, size_t size,
		bool& parse_error, bool& out_of_range)
	{
		int_t i = 0;
		auto f = p.parse(data, size);
		parse_error = !p.found();
		if (!parse_error)
			i = terminals_to_int(*f, f->root(), out_of_range);
		return i;
	}
	ostream& print_error(ostream& os) {
		return os << p.get_error().to_str() << '\n';
	}
private:
	nonterminals<> nts;
	char_class_fns<> cc;
	prods<> start, digit, digits;
	grammar<> g;
	parser<> p;
	prods<> rules() {
		prods<> r, minus('-'); // create a minus terminal for '-'
		// digits is a sequence of digits (was start in previous part)
		r(digits, digit | (digit + digits));
		// start now can be a sequence of digits or the minus terminal
		// followed by a sequence of digits
		r(start,  digits | (minus + digits));
		return r;
	}
};

int main() {
	cout << "Validator for integers. "
		<< "Enter an integer per line or Ctrl-D to quit\n";
	csv_parser p;
	string line;
	while (getline(cin, line)) {
		cout << "entered: `" << line << "`";
		bool parse_error, out_of_range;
		int_t i = p.parse(line.c_str(), line.size(),
				parse_error, out_of_range);
		if (parse_error) p.print_error(cerr);
		else if (out_of_range) cerr << " out of range, allowed range "
			"is from: " << numeric_limits<int_t>::min() <<
			" to: " << numeric_limits<int_t>::max() << '\n';
		else cout << " parsed integer: " << i << '\n';
	}
}
