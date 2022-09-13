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
#include <string.h>
#include "parser.h"
#include "glanguage.h"
using namespace std;
using namespace idni;
using namespace idni::glanguage;
typedef parser<char32_t> parser_t;
typedef parser_t::grammar grammar;
u32string load_stdin() {
	stringstream ss;
	return ss << cin.rdbuf(), to_u32string(ss.str());
}
int main(int argc, char** argv) {
	bool basic_char = argc == 2 &&
		(strcmp(argv[1], "-c") == 0 || strcmp(argv[1], "--char") == 0);
	parser_t p = g_parser<char32_t>();
	if (p.recognize(load_stdin())) {
		grammar_to_cpp<char32_t>(cout,
			transform_parsed_g_to_grammar<char32_t>(p), basic_char);
	} else return
		cerr << "input text does not seem to be a grammar" << endl, 1;
}
