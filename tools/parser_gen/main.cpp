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

ostream& help(ostream& os, char* cmd) {
	os << cmd << " <parser name> <TGF file> [ <start> [ <char type> [ <terminal type>"
		" [ decoder [ encoder ] ] ] ] ]\n";
	return os;
}

int error(char* cmd, string msg, bool print_help = true) {
	cerr << cmd << ": " << msg << endl;
	if (print_help) help(cerr, cmd);
	return 1;
}

int main(int argc, char** argv) {
	string start_nt = "start", char_type = "char", terminal_type = "",
		decoder = "", encoder = "";
	if (argc < 3) return error(argv[0], "requires at least 2 arguments");
	if (argc > 7) encoder       = argv[7];
	if (argc > 6) decoder       = argv[6];
	if (argc > 5) terminal_type = argv[5];
	if (argc > 4) char_type     = argv[4];
	if (argc > 3) start_nt      = argv[3];
	string parser_name(argv[1]), tgf_filename(argv[2]);
	generate_parser_cpp_from_file<char>(cout, parser_name, tgf_filename,
		start_nt, char_type, terminal_type, decoder, encoder);
}
