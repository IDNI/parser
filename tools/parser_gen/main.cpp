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
#include <cassert>
#include <string.h>
#include <fstream>
#include "parser_gen.h"
using namespace std;
using namespace idni;

int main(int argc, char** argv) {
	if (argc != 3) return cerr << argv[0] <<
		": requires 2 arguments: parser_name and a tgf_filename\n", 1;
	string parser_name(argv[1]), tgf_filename(argv[2]);
	generate_parser_cpp_from_file<>(cout, parser_name, tgf_filename);
}
