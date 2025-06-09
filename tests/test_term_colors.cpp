// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.txt

#include <iostream>
#include "utility/term_colors.h"

using namespace std;
using namespace idni::term;


const vector<color> modifier_code{
	BRIGHT, DIM, UNDERLINED,
	BLINK, REVERSE, HIDDEN
};
const vector<color> clear_modifier_code{
	CLEAR_BRIGHT, CLEAR_DIM, CLEAR_UNDERLINED,
	CLEAR_BLINK, CLEAR_REVERSE, CLEAR_HIDDEN
};
const vector<color> color_code{
	BLACK, RED, GREEN, YELLOW,
	BLUE, MAGENTA, CYAN, LIGHT_GRAY, DEFAULT,
	DARK_GRAY, LIGHT_RED, LIGHT_GREEN, LIGHT_YELLOW,
	LIGHT_BLUE, LIGHT_MAGENTA, LIGHT_CYAN, WHITE
};
const vector<color> background_code{
	BG_BLACK, BG_RED, BG_GREEN, BG_YELLOW,
	BG_BLUE, BG_MAGENTA, BG_CYAN, BG_LIGHT_GRAY, BG_DEFAULT,
	BG_DARK_GRAY, BG_LIGHT_RED, BG_LIGHT_GREEN, BG_LIGHT_YELLOW,
	BG_LIGHT_BLUE, BG_LIGHT_MAGENTA, BG_LIGHT_CYAN, BG_WHITE
};

const vector<string> modifier_code_names{
	"BRIGHT", "DIM", "UNDERLINED",
	"BLINK", "REVERSE", "HIDDEN"
};
const vector<string> clear_modifier_code_names{
	"CLEAR_BRIGHT", "CLEAR_DIM", "CLEAR_UNDERLINED",
	"CLEAR_BLINK", "CLEAR_REVERSE", "CLEAR_HIDDEN"
};
const vector<string> color_code_names{
	"BLACK", "RED", "GREEN", "YELLOW",
	"BLUE", "MAGENTA", "CYAN", "LIGHT_GRAY", "DEFAULT",
	"DARK_GRAY", "LIGHT_RED", "LIGHT_GREEN", "LIGHT_YELLOW",
	"LIGHT_BLUE", "LIGHT_MAGENTA", "LIGHT_CYAN", "WHITE"
};
const vector<string> background_code_names{
	"BG_BLACK", "BG_RED", "BG_GREEN", "BG_YELLOW",
	"BG_BLUE", "BG_MAGENTA", "BG_CYAN", "BG_LIGHT_GRAY", "BG_DEFAULT",
	"BG_DARK_GRAY", "BG_LIGHT_RED", "BG_LIGHT_GREEN", "BG_LIGHT_YELLOW",
	"BG_LIGHT_BLUE", "BG_LIGHT_MAGENTA", "BG_LIGHT_CYAN", "BG_WHITE"
};

struct color_group {
	color_group(const vector<color>& codes,
		const vector<string>& names)
		: codes(codes), names(names) {}
	const vector<color>& codes;
	const vector<string>& names;
};

color_group g_modifiers(modifier_code, modifier_code_names);
color_group g_clear_modifiers(clear_modifier_code, clear_modifier_code_names);
color_group g_colors(color_code, color_code_names);
color_group g_backgrounds(background_code, background_code_names);

int main() {
	const string s("Hello World!");
	colors TC;
	auto print_color_ = [&](const string& code, const string& name) {
		for (size_t i = 0; i != 12 - code.size(); ++i) cout << " ";
		cout << code;
		cout << "  " << name << ":";
		for (size_t i = 0; i != 50 - name.size(); ++i) cout << " ";
		cout << TC[COLOR] << code << TC[EOC] << s << TC.CLEAR();
		cout << endl;
	};
	auto print_color = [&](const color_group& cg, size_t c)
	{
		print_color_(TC[cg.codes[c]], cg.names[c]);
	};
	auto print_color2 = [&](const color_group& cg, size_t c,
		const color_group& cg2, size_t c2)
	{
		print_color_(TC[cg.codes[c]] + ";" + TC[cg2.codes[c2]],
			cg.names[c] + " " + cg2.names[c2]);
	};
	auto print_color3 = [&](const color_group& cg, size_t c,
		const color_group& cg2, size_t c2,
		const color_group& cg3, size_t c3)
	{
		print_color_(TC[cg.codes[c]] + ";" + TC[cg2.codes[c2]] + ";"
			+ TC[cg3.codes[c3]],
			cg.names[c] + " " + cg2.names[c2] + " " + cg3.names[c3]);
	};
	auto print = [&](const color_group& cg)
	{
		for (size_t c = 0; c != cg.codes.size(); ++c)
			print_color(cg, c);
	};
	auto print2 = [&](const color_group& cg, const color_group& cg2)
	{
		for (size_t c = 0; c != cg.codes.size(); ++c)
			for (size_t c2 = 0; c2 != cg2.codes.size(); ++c2)
				print_color2(cg, c, cg2, c2);
	};
	auto print3 = [&](const color_group& cg, const color_group& cg2,
		const color_group& cg3)
	{
		for (size_t c = 0; c != cg.codes.size(); ++c)
			for (size_t c2 = 0; c2 != cg2.codes.size(); ++c2)
				for (size_t c3 = 0; c3 != cg3.codes.size(); ++c3)
					print_color3(cg, c, cg2, c2, cg3, c3);
	};
	print(g_modifiers);
	print(g_clear_modifiers);
	print(g_colors);
	print(g_backgrounds);
	print2(g_colors,      g_backgrounds);
	print2(g_colors,      g_modifiers);
	print2(g_backgrounds, g_modifiers);
	print2(g_colors,      g_clear_modifiers);
	print2(g_backgrounds, g_clear_modifiers);
	print3(g_colors,      g_backgrounds,     g_modifiers);
	print3(g_colors,      g_backgrounds,     g_clear_modifiers);
	cout << endl;
	return 0;
}
