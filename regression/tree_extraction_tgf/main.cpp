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
#include "tree_extraction.generated.h"

using namespace std;
using namespace idni;

struct main_tree_extraction {

	static constexpr string te_input = "something=0";

	int_t run() {
		tree_extraction parser;
		auto f = parser.parse(te_input.c_str(), te_input.size());
		return 0;
	}
};

int main() {
	main_tree_extraction().run();
}
