{
	{ U"start", {
		{ U"productions", U"ws", }
	} }, 
	{ U"productions", {
		{ U"ws", U"production", U"productions_rest", }
	} }, 
	{ U"productions_rest", {
		{ U"ws", U"production", U"productions_rest", },
		{ U"\0", }
	} }, 
	{ U"production", {
		{ U"sym", U"ws", U"production_sep", U"ws", U"alts", U"constraints", U"ws", U".", }
	} }, 
	{ U"production_sep", {
		{ U"=>", }
	} }, 
	{ U"alts", {
		{ U"alt", U"alts_rest", }
	} }, 
	{ U"alts_rest", {
		{ U"ws", U"alt_separator", U"ws", U"alt", U"alts_rest", },
		{ U"\0", }
	} }, 
	{ U"alt_separator", {
		{ U"|", }
	} }, 
	{ U"alt", {
		{ U"nt_nt_alt", },
		{ U"nt_t_alt", },
		{ U"t_nt_alt", },
		{ U"t_t_alt", }
	} }, 
	{ U"t_t_alt", {
		{ U"t_t_factor", },
		{ U"t_t_factor", U"ws", U"t_t_alt", },
		{ U"t_t_factor", U"ws", U"nt_t_alt", }
	} }, 
	{ U"t_nt_alt", {
		{ U"t_t_factor", U"ws", U"t_nt_alt", },
		{ U"t_t_factor", U"ws", U"nt_nt_alt", }
	} }, 
	{ U"nt_t_alt", {
		{ U"nt_t_factor", },
		{ U"nt_nt_factor", U"ws_required", U"nt_t_alt", },
		{ U"nt_nt_factor", U"ws", U"t_t_alt", },
		{ U"nt_t_factor", U"ws", U"nt_t_alt", },
		{ U"nt_t_factor", U"ws", U"t_t_alt", }
	} }, 
	{ U"nt_nt_alt", {
		{ U"nt_nt_factor", },
		{ U"nt_nt_factor", U"ws_required", U"nt_nt_alt", },
		{ U"nt_nt_factor", U"ws", U"t_nt_alt", },
		{ U"nt_t_factor", U"ws", U"nt_nt_alt", },
		{ U"nt_t_factor", U"ws", U"t_nt_alt", }
	} }, 
	{ U"factor_group", {
		{ U"(", U"ws", U"alt", U"ws", U")", }
	} }, 
	{ U"factor_optional", {
		{ U"[", U"ws", U"alt", U"ws", U"]", }
	} }, 
	{ U"factor_plus", {
		{ U"{", U"ws", U"alt", U"ws", U"}", }
	} }, 
	{ U"factor_unot", {
		{ U"terminal", U"ws", U"unot", },
		{ U"factor_group", U"ws", U"unot", }
	} }, 
	{ U"t_t_factor", {
		{ U"terminal", },
		{ U"factor_group", },
		{ U"factor_plus", },
		{ U"factor_optional", },
		{ U"factor_unot", }
	} }, 
	{ U"nt_t_factor", {
		{ U"nt_factor_unot", }
	} }, 
	{ U"nt_factor_unot", {
		{ U"nonterminal", U"ws", U"unot", }
	} }, 
	{ U"nt_nt_factor", {
		{ U"nonterminal", }
	} }, 
	{ U"unot", {
		{ U"*", },
		{ U"+", }
	} }, 
	{ U"terminal", {
		{ U"quoted_char", },
		{ U"string", }
	} }, 
	{ U"nonterminal", {
		{ U"sym", }
	} }, 
	{ U"constraints", {
		{ U"ws", U",", U"ws", U"constraint", U"constraints", },
		{ U"\0", }
	} }, 
	{ U"constraint", {
		{ U"preference", }
	} }, 
	{ U"preference", {
		{ U"pref", },
		{ U"priority", }
	} }, 
	{ U"pref", {
		{ U"_pref", }
	} }, 
	{ U"priority", {
		{ U"integer", }
	} }, 
	{ U"integer", {
		{ U"digits", }
	} }, 
	{ U"digits", {
		{ U"digit", U"digits_rest", }
	} }, 
	{ U"digits_rest", {
		{ U"digit", U"digits_rest", },
		{ U"\0", }
	} }, 
	{ U"sym", {
		{ U"chars", }
	} }, 
	{ U"chars", {
		{ U"alpha", U"chars1", },
		{ U"_", U"chars1", }
	} }, 
	{ U"chars1", {
		{ U"alnum", U"chars1", },
		{ U"_", U"chars1", },
		{ U"\0", }
	} }, 
	{ U"printable_chars", {
		{ U"printable", U"printable_chars1", }
	} }, 
	{ U"printable_chars1", {
		{ U"printable", U"printable_chars1", },
		{ U"\0", }
	} }, 
	{ U"ws_comment", {
		{ U"#", U"eol", },
		{ U"#", U"printable_chars", U"eol", }
	} }, 
	{ U"ws_required", {
		{ U"space", U"ws", },
		{ U"ws_comment", U"ws", }
	} }, 
	{ U"ws", {
		{ U"ws_required", },
		{ U"\0", }
	} }, 
	{ U"eol", {
		{ U"\r", },
		{ U"\n", },
		{ U"eof", }
	} }, 
	{ U"string", {
		{ U"\"", U"string_chars", U"\"", }
	} }, 
	{ U"string_chars", {
		{ U"string_char", U"string_chars1", }
	} }, 
	{ U"string_chars1", {
		{ U"string_char", U"string_chars1", },
		{ U"\0", }
	} }, 
	{ U"string_char", {
		{ U"char0", },
		{ U"'", },
		{ U"\\\"", }
	} }, 
	{ U"quoted_char", {
		{ U"'", U"char", U"'", },
		{ U"quoted_char_esc", }
	} }, 
	{ U"quoted_char_esc", {
		{ U"'\\", U"printable", U"'", }
	} }, 
	{ U"char", {
		{ U"char0", },
		{ U"\\'", },
		{ U"\"", }
	} }, 
	{ U"char0", {
		{ U"alnum", },
		{ U"space", },
		{ U"!", },
		{ U"#", },
		{ U"$", },
		{ U"%", },
		{ U"&", },
		{ U"(", },
		{ U")", },
		{ U"*", },
		{ U"+", },
		{ U",", },
		{ U"-", },
		{ U".", },
		{ U"/", },
		{ U":", },
		{ U";", },
		{ U"<", },
		{ U"=", },
		{ U">", },
		{ U"?", },
		{ U"@", },
		{ U"[", },
		{ U"\\", },
		{ U"]", },
		{ U"^", },
		{ U"_", },
		{ U"{", },
		{ U"|", },
		{ U"}", },
		{ U"~", }
	} }
};
