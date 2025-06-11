// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.txt

// CSV parser tutorial - part 2
//
// In this part we show using a predefined character class function for digits.

#include <optional>
#include <limits>

#include "parser.h"

#ifdef min
#	undef min
#endif
#ifdef max
#	undef max
#endif

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
		cout << "entered: `" << line << "`\n";
		auto res = p.parse(line.c_str(), line.size());
		if (!res.found) {
			cerr << res.parse_error << '\n';
			continue;
		}
		auto i = res.get_terminals_to_int(res.get_forest()->root());
		if (!i) cerr << "out of range, allowed range is from: 0 to: "
				<< numeric_limits<int_t>::max() << '\n';
		else cout << "parsed integer: " << i.value() << '\n';
	}
}
