#include "tml_parser.generated.h"
int main() {
	tml_parser p;
	string line;
	while (getline(cin, line)) {
		if (line.size() == 0) continue;
		cout << "> " << line << " = ";
		auto f = p.parse(from_str<char32_t>(line));
		if (!f) return 1;
		auto next_g = [](parser<char32_t>::pforest::graph &fg) {
			auto tree = fg.extract_trees();
			tree->to_print(cout << "\n\n------\n"), cout << endl;
			return true;
		};
		f->extract_graphs(f->root(), next_g);
	}
}
