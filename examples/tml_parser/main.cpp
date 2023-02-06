#include <iostream>
#include "tml_parser.generated.h"
using namespace std;
using namespace idni;
int main() {
	tml_parser p;
	string line;
	while (getline(cin, line)) {
		if (line.size() == 0) continue;
		cout << "> " << line << " = ";
		auto s = from_str<char>(line);
		auto f = p.parse(s.c_str(), s.size());
		if (!p.found()) {
			cerr << p.get_error().to_str() << endl;
			return 1;
		}
		auto next_g = [](parser<char>::pforest::graph &fg) {
			auto tree = fg.extract_trees();
			tree->to_print(cout << "\n\n------\n"), cout << endl;
			return true;
		};
		f->extract_graphs(f->root(), next_g);
	}
}
