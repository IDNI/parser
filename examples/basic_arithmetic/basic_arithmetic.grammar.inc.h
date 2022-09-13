{
	{ U"ws_comment", {
		{ U"#", U"eol" },
		{ U"#", U"printable_chars", U"eol" }
	} },
	{ U"ws_required", {
		{ U"space", U"ws" },
		{ U"ws_comment", U"ws" }
	} },
	{ U"ws", {
		{ U"ws_required" },
		{ U"" }
	} },
	{ U"eol", {
		{ U"\r" },
		{ U"\n" },
		{ U"eof" }
	} },
	{ U"printable_chars", {
		{ U"printable", U"printable_chars1" }
	} },
	{ U"printable_chars1", {
		{ U"printable", U"printable_chars1" },
		{ U"" }
	} },
	{ U"sign", {
		{ U"+" },
		{ U"-" }
	} },
	{ U"integer", {
		{ U"digit", U"integer_rest" }
	} },
	{ U"integer_rest", {
		{ U"digit", U"integer_rest" },
		{ U"" }
	} },
	{ U"expr", {
		{ U"expr_op" },
		{ U"term" }
	} },
	{ U"term", {
		{ U"term_op" },
		{ U"factor" }
	} },
	{ U"expr_op", {
		{ U"expr", U"ws", U"addsub", U"ws", U"term" }
	} },
	{ U"term_op", {
		{ U"term", U"ws", U"muldiv", U"ws", U"factor" }
	} },
	{ U"addsub", {
		{ U"+" },
		{ U"-" }
	} },
	{ U"muldiv", {
		{ U"*" },
		{ U"/" }
	} },
	{ U"factor", {
		{ U"(", U"ws", U"expr", U"ws", U")" },
		{ U"sign", U"ws", U"factor" },
		{ U"integer" }
	} },
	{ U"statement", {
		{ U"ws", U"expr", U"eol" }
	} },
	{ U"statements", {
		{ U"statement", U"statements1" }
	} },
	{ U"statements1", {
		{ U"statement", U"statements1" },
		{ U"" }
	} },
	{ U"start", {
		{ U"statements", U"ws" },
		{ U"" }
	} }
};
