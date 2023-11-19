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

#include <limits>

#include "parser.h"

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
	// parsing and conversion to int is wrapped and returns only value
	// and sets parse_error or out_of_range bool refs if error
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
	// if there is a parse error this method prints details to a stream os
	ostream& print_error(ostream& os) {
		return os << p.get_error().to_str() << '\n';
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
		cout << "entered: `" << line << "`";
		// declare variables to get error information
		bool parse_error, out_of_range;
		// parse the line input and get the result into i
		int_t i = p.parse(line.c_str(), line.size(),
				parse_error, out_of_range);
		// handle errors or print the parsed integer
		if (parse_error) p.print_error(cerr);
		else if (out_of_range) cerr << " out of range, max value is: "
				<< numeric_limits<int_t>::max() << '\n';
		else cout << " parsed integer: " << i << '\n';
	}
}
